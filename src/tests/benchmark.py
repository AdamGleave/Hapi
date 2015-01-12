#!/usr/bin/env python3

import config, sh, shutil, csv, os, sys, time

def versionDirectory(version):
  return os.path.join(config.WORKING_DIRECTORY, version)

def findImplementations(tests):
  implementation_names = set()
  for case in tests.values():
    for instances in case["tests"].values():
      implementation_names.add(instances["implementation"])
  implementations = {k : config.IMPLEMENTATIONS[k] for k in implementation_names}
  return implementations

def findBuildTargets(implementations):
  build_targets = {}
  for implementation in implementations.values():
    version = implementation["version"]
    existing_targets = build_targets.get(version, set())
    existing_targets.add(implementation["target"])
    build_targets[version] = existing_targets
  return build_targets

def clean():
  try:
    shutil.rmtree(config.WORKING_DIRECTORY)
  except FileNotFoundError:
    # ignore: if directory doesn't exist, no need to delete it
    pass
  os.mkdir(config.WORKING_DIRECTORY)

def gitCheckout(version):
  directory = versionDirectory(version)
  os.mkdir(directory)
  # how to checkout branch to new working directory, see:
  # htp://blog.jessitron.com/2013/10/git-checkout-multiple-branches-at-same.html
  sh.tar(sh.git.archive(version), "-xC", directory)
  
def build(version, targets):
  saved_cwd = os.getcwd()
  
  directory = os.path.join(config.WORKING_DIRECTORY, version)
  build_directory = os.path.join(directory, config.BUILD_ROOT)
  source_directory = os.path.join(directory, config.SOURCE_ROOT) 
  
  os.mkdir(build_directory)
  os.chdir(build_directory)
  sh.cmake(source_directory)
  sh.make(*targets, 
          _out=os.path.join(build_directory, "makefile.out"),
          _err=os.path.join(build_directory, "makefile.err"))
  
  os.chdir(saved_cwd)
  
def buildImplementations(implementations):
  build_targets = findBuildTargets(implementations)
  
  clean()
  for version, targets in build_targets.items():
    print(version, end="")
    gitCheckout(version)
    print(": checked out, ", end="")
    build(version, targets)
    print(" built ", targets)
    
class ExitCodeException(Exception):
  def __init__(self, exit_code):
    self.exit_code = exit_code
    
  def __str__(self):
    return repr(self.exit_code)

def runTestInstance(instance, fname, iteration):
  # create instance
  implementation_name = instance["implementation"]
  implementation = implementations[implementation_name]
  
  version_directory = versionDirectory(implementation["version"])
  exe_path = os.path.join(version_directory,
                          config.BUILD_ROOT,
                          implementation["path"])
  arguments = implementation["arguments"] + instance["arguments"]
  
  test_command = sh.Command(exe_path)
  test_command = test_command.bake(*arguments)
  
  input_path = os.path.join(config.DATASET_ROOT, fname)
  with open(input_path, 'r') as input_file:
    # time it
    log_directory = os.path.join(version_directory, "log")
    # TODO: get rid of this if we don't reuse build directories
    try:
      os.mkdir(log_directory)
    except OSError:
      # directory already exists -- ignore
      pass
    
    prefix = fname + "_" + str(iteration)
    start_time = time.time()
    result = test_command(_in=input_file,
                 _out=os.path.join(log_directory, prefix + ".out"),
                 _err=os.path.join(log_directory, prefix + ".err"))
    end_time = time.time()
    if result.exit_code != 0:
      raise ExitCodeException(result.exit_code)
    
    time_elapsed = end_time - start_time
    return time_elapsed
    
def runTests(tests):
  for case_name, case_config in tests.items():
    print(case_name)
    
    result_fname = os.path.join(config.RESULT_ROOT, case_name + ".csv") 
    with open(result_fname, 'w') as result_file:
      fieldnames = ["test", "file", "iteration", "time_elapsed"]
      result_writer = csv.DictWriter(result_file,fieldnames=fieldnames)
      result_writer.writeheader()
      iterations = case_config["iterations"]
      for fname in case_config["files"]:
        print("\t", fname, ": ", end="")
        for i in range(iterations):
          print(i, " ", end="")
          for test_name, test in case_config["tests"].items():
            time_elapsed = runTestInstance(test, fname, i)
            result = { "test": test_name, 
                       "file": fname, 
                       "iteration": i,
                       "time_elapsed": time_elapsed }
            result_writer.writerow(result)
        print("")
    
    
if len(sys.argv) == 1:
  # no arguments
  tests = config.TESTS
else:
  test_cases = sys.argv[1:]
  tests = {k : config.TESTS[k] for k in test_cases}
    
os.chdir(config.PROJECT_ROOT)

# Unbuffered output. This is needed for progress indicators to display correctly,
# since a new line is printed only at the end of each test.
class flushfile:
    def __init__(self, f):
        self.f = f
    def write(self, x):
        self.f.write(x)
        self.f.flush()
    def flush(self):
      # no-op: always flushed
      pass
        
sys.stdout = flushfile(sys.stdout)

print("*** Building ***")
implementations = findImplementations(tests)
# TODO: uncomment/make an option
#buildImplementations(implementations)

print("*** Running tests ***")
runTests(tests)