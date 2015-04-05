#!/usr/bin/env python3

import config.benchmark as config
import os, sys, time 
import csv, hashlib, sh, shutil 

def error(*args):
  print(*args, file=sys.stderr)
  sys.exit(1)
  
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
  build_directory = os.path.join(directory, config.BUILD_PREFIX)
  source_directory = os.path.join(directory, config.SOURCE_PREFIX)
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
  sh.make(config.MAKE_FLAGS, *targets, 
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

def createNativeCommand(exe_path, arguments):
  native_command = sh.Command(exe_path).bake(*arguments)
  def runCommand(input_path, output_path):
    # Due to a quirk of sh's design, having cat pipe the file contents is
    # *considerably* faster than using _in directly (in which case Python
    # will have to read everything in, and then echo it out. Seriously slow.)
    return native_command(sh.cat(input_path, _piped="direct"),
                          _out=output_path, _iter="err")
  return runCommand

def createWrapperCommand(exe_path, arguments):
  '''full solver to be used as incremental'''
  snapshots = config.SNAPSHOT_CREATOR_PROGRAM
  command = config.SNAPSHOT_SOLVER_PROGRAM.bake(exe_path, *arguments)
  def runCommand(input_path, output_path):
    # See createNativeCommand for why I'm using cat
    return command(
                snapshots(sh.cat(input_path, _piped="direct"), _piped="direct"), 
                _out=output_path, _iter="err")
  return runCommand

def helperCreateTestInstance(instance):
  implementation_name = instance["implementation"]
  implementation = implementations[implementation_name]
  
  version_directory = versionDirectory(implementation["version"])
  exe_path = os.path.join(version_directory,
                          config.BUILD_PREFIX,
                          implementation["path"])
  arguments = []
  if "arguments" in implementation:
    arguments += implementation["arguments"]
  if "arguments" in instance:
     arguments += instance["arguments"]
  
  return {"implementation": implementation,
          "version_directory": version_directory,
          "exe_path": exe_path,
          "arguments": arguments}

def createFullTestInstance(instance):
  parameters = helperCreateTestInstance(instance)
  
  arguments = parameters["arguments"].copy()
  if "offline_arguments" in implementation:
    arguments += implementation["offline_arguments"]
  
  solver_type = parameters["implementation"]["type"]
  if solver_type == "full":
    test_command = createNativeCommand(parameters["exe_path"],
                                       arguments)
    return (test_command, parameters["version_directory"])
  else:
    error("Illegal solver type ", solver_type, "for full test")

def createIncrementalTestInstance(instance):
  parameters = helperCreateTestInstance(instance)
  
  implementation = parameters["implementation"]
  solver_type = implementation["type"]
  exe_path = parameters["exe_path"]
  arguments = parameters["arguments"].copy()
  if "offline_arguments" in implementation:
    arguments += implementation["offline_arguments"]
  
  test_command = None
  if solver_type == "full":
    test_command = createWrapperCommand(exe_path, arguments)
  elif solver_type == "incremental":
    test_command = createNativeCommand(exe_path, arguments)
  else: 
    error("Unrecognised implementation type ", implementation)
      
  return (test_command, parameters["version_directory"])

def runTestInstance(test_name, test_command, log_directory, fname, iteration,
                    log_fname=None):
  input_path = os.path.join(config.DATASET_ROOT, fname)
  
  if not log_fname:
    log_fname = fname
  log_subpath = os.path.join(config.DATASET_ROOT, log_fname)
  dirname = os.path.relpath(log_subpath, config.DATASET_ROOT)
  log_directory = os.path.join(log_directory, dirname)
  
  if not(os.path.exists(input_path)):
    print("Cannot open ", input_path, "for reading", file=sys.stderr)
    sys.exit(1)
    
  try:
    os.makedirs(log_directory)
  except OSError:
    # directory already created if not first time we've been run
    pass
  
  prefix = test_name + "-offline_" + str(iteration)
  out_path = os.path.join(log_directory, prefix + ".out")
  err_path = os.path.join(log_directory, prefix + ".err")
  with open(err_path, 'w') as err_file:      
    running_command = test_command(input_path, out_path)
    
    start_time = time.time()
    
    algotime_prefix = "ALGOTIME: "
    for error_line in running_command:
      if error_line == "STARTTIME\n":
        # Reset timer. We will not always receive STARTTIME, only when there
        # is some overhead we want to discount. For example, snapshot_solver
        # outputs STARTTIME immediately before launching the solver. This way,
        # we can discount the time taken to generate the snapshots themselves.
        start_time = time.time()
      elif error_line.find(algotime_prefix) == 0:
        # record algorithm running time
        algorithm_running_time = error_line[len(algotime_prefix):].strip()
        
        # record total time (includes parsing, etc)          
        end_time = time.time()
        # TODO: Is this an accurate way of measuring the total times?
        time_elapsed = end_time - start_time
        
        yield((algorithm_running_time, time_elapsed))
        
        # reset timer
        start_time = time.time()
      else:
        err_file.write(error_line)
    
  if running_command.exit_code != 0:
    raise ExitCodeException(result.exit_code)

# SOMEDAY: Ugly how logic and IO are intermixed here. You could perhaps switch
# to putting the logic in a generator, and iterate over the results yielded

def runFullTest(case_name, case_config, result_file):      
  fieldnames = ["test", "file", "iteration", "algorithm_time", "total_time"]
  result_writer = csv.DictWriter(result_file,fieldnames=fieldnames)
  result_writer.writeheader()
  iterations = case_config["iterations"]
  
  tests = case_config["tests"]
  test_instances = {test_name : createFullTestInstance(tests[test_name])
                   for test_name in tests}
  
  for fname in case_config["files"]:
    print("\t", fname, ": ", end="")
    
    for i in range(iterations):
      print(i, " ", end="")
      
      for test_name, (test_command, version_directory) in test_instances.items():
        log_directory = os.path.join(version_directory, "log", case_name)
        run = runTestInstance(test_name, test_command, log_directory, fname, i)
        times = list(run)
        
        assert(len(times) == 1)
        algorithm_time, time_elapsed = times[0]
        
        result = { "test": test_name,
                   "file": fname,
                   "iteration": i,
                   "algorithm_time": algorithm_time,
                   "total_time": time_elapsed }
        result_writer.writerow(result)
        result_file.flush()
        
    print("")

def runIncrementalOfflineTest(case_name, case_config, result_file):
  fieldnames = ["test", "file", "delta_id", 
                "iteration", "algorithm_time", "total_time"]
  result_writer = csv.DictWriter(result_file,fieldnames=fieldnames)
  result_writer.writeheader()
  iterations = case_config["iterations"]
  
  tests = case_config["tests"]
  test_instances = {test_name : createIncrementalTestInstance(tests[test_name])
                   for test_name in tests}
  
  for fname in case_config["files"]:
    print("\t", fname, ": ", end="")
    
    for i in range(iterations):
      print(i, " ", end="")
      
      for test_name, (test_command, version_directory) in test_instances.items():
        log_directory = os.path.join(version_directory, "log", case_name)
        
        delta_id = 0
        for (algorithm_time, time_elapsed) in \
              runTestInstance(test_name, test_command, log_directory, fname, i):
          result = { "test": test_name,
                     "file": fname,
                     "delta_id": delta_id, 
                     "iteration": i,
                     "algorithm_time": algorithm_time,
                     "total_time": time_elapsed }
          result_writer.writerow(result)
          result_file.flush()
          delta_id += 1
        
    print("")

def runSimulator(case_name, case_config, test_name, test_instance,
                 trace_name, trace_config, trace_spec, iteration, type):
  assert(type == "hybrid" or type == "online")
  
  parameters = helperCreateTestInstance(test_instance)
  simulator = config.GOOGLE_TRACE_SIMULATOR
  
  solver_type = parameters["implementation"]["type"]
  incremental = None
  if solver_type == "full":
    incremental = False
  elif solver_type == "incremental":
    incremental = True
  else:
    error("Unrecognised solver type ", solver_type)
    
  ### Set up logging
  log_directory = os.path.join(parameters["version_directory"], "log",
                               case_name, trace_name)
  try:
    os.makedirs(log_directory)
  except OSError:
    # directory already created if not first time we've been run
    pass
  
  prefix = test_name + "-online_" + str(iteration)
  out_path = os.path.join(log_directory, prefix + ".out")
  err_path = os.path.join(log_directory, prefix + ".err")
  
  ### Record deltas for offline tests
  if type == "hybrid":
    trace_directory = os.path.join(parameters["version_directory"], "trace", 
                                   trace_name, 
                                   parameters["implementation"]["target"])
    try:
      os.makedirs(trace_directory)
    except OSError:
      # directory already created if not first time we've been run
      pass
    
    # We must never generate the same trace path for two invocations which could
    # produce *different* results. Different algorithms may certainly produce  
    # different solutions, and hence different traces. Parameters may also 
    # effect this. 
    cli = " ".join([parameters["exe_path"]] + parameters["arguments"])
    hash = hashlib.md5(cli.encode('utf-8'))
    
    delta_file = os.path.join(trace_directory, hash.hexdigest() + ".imin")
    if os.path.exists(delta_file): 
      yield (delta_file, "cached")
    else:
      yield (delta_file, "generating") 
    simulator = simulator.bake("-graph_output_file", delta_file) 
  
  ### Configuration for the trace  
  simulator = simulator.bake("-trace_path", trace_spec["dir"])
  simulator = simulator.bake("-runtime", trace_config["runtime"])
  simulator = simulator.bake("-num_files_to_process", trace_spec["num_files"])
  if "percentage" in trace_config:
    simulator = simulator.bake("-percentage", trace_config["percentage"])
  
  ### General parameters
  if type == "hybrid":
    batch_step = config.DEFAULT_BATCH_STEP
    if "batch_step" in case_config:
      batch_step = case_config["batch_step"]
    simulator = simulator.bake("-batch_step", batch_step)
  else:
    online_factor = config.DEFAULT_ONLINE_FACTOR
    if "online_factor" in case_config:
      online_factor = case_config["online_factor"]
    simulator = simulator.bake("-online_factor", online_factor)
    
    online_max_time = config.DEFAULT_ONLINE_MAX_TIME
    if "online_max_time" in case_config:
      online_max_time = case_config["online_max_time"]
    simulator = simulator.bake("-online_max_time", online_max_time)
  cost_model = config.DEFAULT_COST_MODEL
  if "cost_model" in case_config:
    cost_model = case_config["cost_model"]
  simulator = simulator.bake("-flow_scheduling_cost_model",
                             config.COST_MODELS[cost_model])
  
  ### Configuration for the solver
  simulator = simulator.bake("-solver", "custom")
  simulator = simulator.bake("-flow_scheduling_binary", parameters["exe_path"])
  arguments = parameters["arguments"]
  if arguments:
    simulator = simulator.bake("-custom_flow_scheduling_args", 
                               " ".join(arguments))
  # Note "-incremental_flow False" will not work; GFlags will interpret this
  # as setting FLAGS_incremental_flow, and then a positional argument False.
  # So have to construct the argument string ourselves
  simulator = simulator.bake("-incremental_flow=" + str(incremental))
  
  ### Setup pipe for statistics output from the simulator
  if type == "online":
    fifo_path = os.path.join(log_directory, prefix + ".stats_fifo")
    if os.path.exists(fifo_path):
      print("WARNING: removing stale FIFO ", fifo_path)
      os.unlink(fifo_path)
    os.mkfifo(fifo_path)
    simulator = simulator.bake("-stats_file", fifo_path)
  
  ### Run the simulator and parse output
  with open(err_path, 'w') as err_file:
    print("Executing ", simulator, file=err_file)
    err_file.flush()
    
    with open(out_path, 'w') as out_file:
      # WARNING: Do not replace this to just  call _out=out_path directly.
      # Normally this would work OK, but out_file is a FIFO. sh is annoying
      # and doesn't close FDs. So we'd never read EOF, and would get stuck. 
      running_simulator = simulator(_out=out_file.buffer, _err=err_file.buffer,
                                    _bg=True)
    
      if type == "online":
        with open(fifo_path, 'r') as stats_file:
          csv_reader = csv.DictReader(stats_file)
          for row in csv_reader:
            if type == "online":
              yield row
      
      running_simulator.wait()
  
  ### Clean up  
  if type == "online":
    os.unlink(fifo_path)
  
  if running_simulator.exit_code != 0:
    raise ExitCodeException(result.exit_code)

def runIncrementalHybridTest(case_name, case_config, result_file): 
  fieldnames = ["test", "trace", "delta_id", 
                "iteration", "algorithm_time", "total_time"]
  result_writer = csv.DictWriter(result_file,fieldnames=fieldnames)
  result_writer.writeheader()
  iterations = case_config["iterations"]
  
  tests = case_config["tests"]
  test_instances = {test_name : createIncrementalTestInstance(tests[test_name])
                   for test_name in tests}
  
  for trace_config in case_config["traces"]:
    trace_name = trace_config["name"]
    trace_spec = config.TRACE_DATASET[trace_name]
    trace_files = {}
    
    print("\t", trace_name, " - online: ", end="")
    
    for (test_name, test_instance) in tests.items(): 
      result = runSimulator(case_name, case_config, test_name, test_instance,
                  trace_name, trace_config, trace_spec, 0, type="hybrid")
      result = list(result)
      assert(len(result) == 1)
      
      trace_files[test_name], status = result[0]
          
      print(test_name, 
            "*" if status == "cached" else "",
            sep="", end=" ")
        
    print("/ offline: ", end="")
    for i in range(iterations):
      print(i, " ", end="")
      
      for test_name, (test_command, version_directory) in test_instances.items():
        log_directory = os.path.join(version_directory, "log", case_name)
        
        delta_id = 0
        input_graph = trace_files[test_name]
        log_fname = os.path.relpath(os.path.join(log_directory, test_name),
                                    input_graph)
        for (algorithm_time, time_elapsed) in runTestInstance(
                                         test_name, test_command, log_directory,
                                         input_graph, i, log_fname=log_fname):
          result = { "test": test_name,
                     "trace": trace_name,
                     "delta_id": delta_id, 
                     "iteration": i,
                     "algorithm_time": algorithm_time,
                     "total_time": time_elapsed }
          result_writer.writerow(result)
          result_file.flush()
          delta_id += 1
        
    print("")

def runIncrementalOnlineTest(case_name, case_config, result_file):
  fieldnames = ["test", "trace", "delta_id", "cluster_timestamp", "iteration",
      "scheduling_latency", "algorithm_time", "flowsolver_time", "total_time"]
  result_writer = csv.DictWriter(result_file,fieldnames=fieldnames)
  result_writer.writeheader()
  
  iterations = case_config["iterations"]
  tests = case_config["tests"]
  
  for trace_config in case_config["traces"]:
    trace_name = trace_config["name"]
    trace_spec = config.TRACE_DATASET[trace_name]
    
    print("\t", trace_name, ": ", end="")
    
    for i in range(iterations):
      print(i, " ", end="")
      
      for (test_name, test_instance) in tests.items():
        row_number = 0
        for row in runSimulator(case_name, case_config, test_name, test_instance,
                           trace_name, trace_config, trace_spec, i, type="online"):
          result = { "test": test_name,
                     "trace": trace_name,
                     "delta_id": row_number, 
                     "iteration": i,
                     "cluster_timestamp": row["cluster_timestamp"],
                     "scheduling_latency": row["scheduling_latency"],
                     "algorithm_time": row["algorithm_time"],
                     "flowsolver_time": row["flowsolver_time"],
                     "total_time": row["total_time"] }
          result_writer.writerow(result)
          result_file.flush()
          row_number += 1
        
    print("")

def runTests(tests):
  for case_name, case_config in tests.items():
    print(case_name)
    
    result_fname = os.path.join(config.RESULT_ROOT, case_name + ".csv") 
    with open(result_fname, 'w') as result_file:
      test_type = case_config["type"]
      if test_type == "full":
        runFullTest(case_name, case_config, result_file)
      elif test_type == "incremental_offline":
        runIncrementalOfflineTest(case_name, case_config, result_file)
      elif test_type == "incremental_hybrid":
        runIncrementalHybridTest(case_name, case_config, result_file)
      elif test_type == "incremental_online":
        runIncrementalOnlineTest(case_name, case_config, result_file)
      else:
        error("Unrecognised test type: ", test_type)

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