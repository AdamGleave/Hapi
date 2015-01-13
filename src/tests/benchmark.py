#!/usr/bin/env python3

import config, sh, shutil, csv, os, sys, time

# wrapper around file object, flushing after every write
class flushfile:
    def __init__(self, f):
        self.f = f
    def write(self, x):
        self.f.write(x)
        self.f.flush()
    def flush(self):
      # no-op: always flushed
      pass

def versionDirectory(version):
  return os.path.join(config.WORKING_DIRECTORY, version)

def gitCommitID(version):
  # Make sure we're inside the git repository
  os.chdir(config.PROJECT_ROOT)
  # We run git show -s --pretty=%H <version>
  # git show displays git objects
  # -s suppresses the normal patch output, just showing the commit message
  # --pretty=%H specifies the format, giving just the SHA1 ID
  # Tell sh not to emulate TTY for STDOUT: otherwise, git will add escape codes
  git_result = sh.git.show(version,s=True,pretty="%H",_tty_out=False)
  commit_id = git_result.stdout.strip()
  return commit_id.decode("utf-8")

def findImplementations(tests):
  implementation_names = set()
  for case in tests.values():
    for instances in case["tests"].values():
      implementation_names.add(instances["implementation"])
  
  implementations = {}
  for name in implementation_names:
    implementation = config.IMPLEMENTATIONS[name]
    # rewrite to canonical version
    implementation["version"] = gitCommitID(implementation["version"])
    implementations[name] = implementation
  
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

# returns true if version has been successfully checked out to working directory;
# false if version was already checked out. Raises exception on error. 
def gitCheckout(version):
  directory = versionDirectory(version)
  if os.path.exists(directory):
    # already checked out
    # (since version is always a commit ID, the code can never have changed)
    return False
  else:
    os.makedirs(directory)
    # Make sure we're inside the git repository first
    os.chdir(config.PROJECT_ROOT)
    # how to checkout branch to new working directory, see:
    # http://blog.jessitron.com/2013/10/git-checkout-multiple-branches-at-same.html
    sh.tar(sh.git.archive(version), "-xC", directory)
    return True
  
def build(version, targets):
  saved_cwd = os.getcwd()
  
  directory = os.path.join(config.WORKING_DIRECTORY, version)
  build_directory = os.path.join(directory, config.BUILD_ROOT)
  source_directory = os.path.join(directory, config.SOURCE_ROOT)
  log_directory = os.path.join(directory, "log") 
  
  try:
    os.mkdir(build_directory)
    os.mkdir(log_directory)
    os.chdir(build_directory)
    sh.cmake(source_directory)
  except FileExistsError:
    # ignore -- we have already built before, no need to regenerate cmake
    pass
  # always re-run make: we might be building a new target.
  # (If not, it's harmless: make will do nothing for targets already built.)
  os.chdir(build_directory)
  sh.make(*targets, 
          _out=os.path.join(log_directory, "makefile.out"),
          _err=os.path.join(log_directory, "makefile.err"))
  
  os.chdir(saved_cwd)
  
def buildImplementations(implementations):
  build_targets = findBuildTargets(implementations)
  
  # TODO: make sure this is safe
  #clean()
  for version, targets in build_targets.items():
    print(version, end="")
    if gitCheckout(version):
      print(": checked out, ", end="")
    else:
      print(": skipped checkout, ", end="")
    build(version, targets)
    print(" built ", targets)
    
class ExitCodeException(Exception):
  def __init__(self, exit_code):
    self.exit_code = exit_code
    
  def __str__(self):
    return repr(self.exit_code)

def createTestInstance(instance):
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
  
  return (test_command, version_directory)

def runTestInstance(test_command, log_directory, fname, iteration):
  input_path = os.path.join(config.DATASET_ROOT, fname)
  with open(input_path, 'r') as input_file:
    try:
      os.mkdir(log_directory)
    except OSError:
      # directory already created if not first time we've been run
      pass
    
    prefix = fname + "_" + str(iteration)
    out_path = os.path.join(log_directory, prefix + ".out")
    err_path = os.path.join(log_directory, prefix + ".err")
    with open(err_path, 'w') as err_file:
      start_time = time.time()
      result = test_command(_in=input_file,
                   _out=os.path.join(log_directory, prefix + ".out"),
                   _iter="err")
      algorithm_running_time = None
      prefix = "ALGOTIME: "
      for error_line in result:
        if error_line.find(prefix) == 0:
          algorithm_running_time = error_line[len(prefix):].strip()
        else:
          err_file.write(error_line)
    
    end_time = time.time()
    if result.exit_code != 0:
      raise ExitCodeException(result.exit_code)
    
    time_elapsed = end_time - start_time
    return (algorithm_running_time, time_elapsed)

# SOMEDAY: Ugly how logic and IO are intermixed here. You could perhaps switch
# to putting the logic in a generator, and iterate over the results yielded
def runTests(tests):
  for case_name, case_config in tests.items():
    print(case_name)
    
    result_fname = os.path.join(config.RESULT_ROOT, case_name + ".csv") 
    with open(result_fname, 'w') as result_file:
      fieldnames = ["test", "file", "iteration", "algorithm_time", "total_time"]
      result_writer = csv.DictWriter(result_file,fieldnames=fieldnames)
      result_writer.writeheader()
      iterations = case_config["iterations"]
      
      for fname in case_config["files"]:
        print("\t", fname, ": ", end="")
        
        for i in range(iterations):
          print(i, " ", end="")
          
          tests = case_config["tests"]
          test_instances = {test_name : createTestInstance(tests[test_name])
                           for test_name in tests}
          for test_name, (test_command, version_directory) in test_instances.items():
            log_directory = os.path.join(version_directory, "log", case_name)
            algorithm_time, time_elapsed = runTestInstance(test_command,
                                                        log_directory, fname, i)
            
            result = { "test": test_name, 
                       "file": fname, 
                       "iteration": i,
                       "algorithm_time": algorithm_time,
                       "total_time": time_elapsed }
            result_writer.writerow(result)
            
        print("")

if __name__ == "__main__":
  # Unbuffered output. This is needed for progress indicators to display correctly,
  # since a new line is printed only at the end of each test.    
  sys.stdout = flushfile(sys.stdout)
      
  if len(sys.argv) == 1:
    # no arguments
    tests = config.TESTS
  else:
    # arguments are list of tests
    test_cases = sys.argv[1:]
    tests = {k : config.TESTS[k] for k in test_cases}
  
  print("*** Building ***")
  implementations = findImplementations(tests)
  # TODO: uncomment/make an option
  buildImplementations(implementations)
  
  print("*** Running tests ***")
  runTests(tests)