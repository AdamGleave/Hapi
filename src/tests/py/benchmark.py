#!/usr/bin/env python3

import config.benchmark as config
import os, sys, time, sh, shutil, signal, csv, hashlib, re 
import threading

class Alarm(Exception):
    pass

def alarm_handler(signum, frame):
    raise Alarm

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

def implementationKey(instance):
  key = instance["implementation"]
  if "compiler" in instance:
    key += "_" + instance["compiler"]
  return key

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
  implementations = {}
  for case in tests.values():
    case_tests = []
    if case["type"].find("approximate") == 0:
      instance = case.get("test", config.APPROXIMATE_DEFAULT_TEST)
      case_tests.append(instance)
      if case["type"] == "approximate_incremental_hybrid":
        case_tests.append(config.REFERENCE_SOLVER)
    else:
      case_tests += case["tests"].values()
      
    for instance in case_tests:
      name = instance["implementation"]
      key = name
      if "compiler" in instance:
        key += "_" + instance["compiler"]
        
      if key not in implementations:   
        implementation = config.IMPLEMENTATIONS[name].copy()
        # rewrite to canonical version
        implementation["version"] = gitCommitID(implementation["version"])
        if "compiler" in instance:
          implementation["compiler"] = instance["compiler"]
        implementations[key] = implementation
  
  return implementations

def findBuildTargets(implementations):
  build_targets = {}
  for implementation in implementations.values():
    version = implementation["version"]
    compiler = implementation.get("compiler", config.DEFAULT_COMPILER)
    existing_compilers = build_targets.get(version, dict())
    existing_targets = existing_compilers.get(compiler, set())
    existing_targets.add(implementation["target"])
    existing_compilers[compiler] = existing_targets
    build_targets[version] = existing_compilers
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
  
def build(version, to_build):
  saved_cwd = os.getcwd()

  root_directory = os.path.join(config.WORKING_DIRECTORY, version)
  source_directory = os.path.join(root_directory, config.SOURCE_PREFIX)
  
  for (compiler_name, targets) in to_build.items():
    directory = os.path.join(root_directory, compiler_name)
    build_directory = os.path.join(directory, config.BUILD_PREFIX)
    log_directory = os.path.join(directory, "log") 
    
    compiler = config.COMPILERS[compiler_name]
  
    try:
      os.makedirs(build_directory)
      os.mkdir(log_directory)
    except FileExistsError:
      # ignore -- we have already built before, directories exist
      pass
    os.chdir(build_directory)
    
    # run cmake
    c_flags = compiler.get("flags", "") + " " + compiler.get("cflags", "")
    cxx_flags = compiler.get("flags", "") + " " + compiler.get("cxxflags", "")
    sh.cmake(source_directory, "-DCMAKE_BUILD_TYPE=Custom",
             "-DCMAKE_C_COMPILER=" + compiler["cc"], 
             "-DCMAKE_CXX_COMPILER=" + compiler["cxx"],
             "-DCMAKE_C_FLAGS_CUSTOM=" + c_flags,
             "-DCMAKE_CXX_FLAGS_CUSTOM=" + cxx_flags)
    
    # run make 
    sh.make(config.MAKE_FLAGS + list(targets), 
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
  def runCommand(input_path, output_path, *extra_arguments, error_path=None):
    # Due to a quirk of sh's design, having cat pipe the file contents is
    # *considerably* faster than using _in directly (in which case Python
    # will have to read everything in, and then echo it out. Seriously slow.)
    if not error_path:
      return native_command(sh.cat(input_path, _piped="direct"),
                            *extra_arguments, _out=output_path, _iter="err")
    else:
      return native_command(sh.cat(input_path, _piped="direct"),
                            *extra_arguments, _out=output_path, _err=error_path, 
                            _bg=True)
  return runCommand

def createWrapperCommand(exe_path, arguments):
  '''full solver to be used as incremental'''
  snapshots = config.SNAPSHOT_CREATOR_PROGRAM
  command = config.SNAPSHOT_SOLVER_PROGRAM.bake(exe_path, *arguments)
  def runCommand(input_path, output_path, *extra_arguments, error_path=None):
    # See createNativeCommand for why I'm using cat
    if not error_path:
      return command(
                snapshots(sh.cat(input_path, _piped="direct"), _piped="direct"), 
                *extra_arguments, _out=output_path, _iter="err")
    else:
      return command(
                snapshots(sh.cat(input_path, _piped="direct"), _piped="direct"), 
                *extra_arguments, _out=output_path, _err=error_path, _bg=True)
  return runCommand

def helperCreateTestInstance(instance):
  implementation = implementations[implementationKey(instance)]
  
  version_directory = versionDirectory(implementation["version"])
  compiler = implementation.get("compiler", config.DEFAULT_COMPILER)
  exe_path = os.path.join(version_directory, compiler, 
                          config.BUILD_PREFIX, implementation["path"])
  arguments = []
  if "arguments" in implementation:
    arguments += implementation["arguments"]
  if "arguments" in instance:
     arguments += instance["arguments"]
  
  return {"implementation": implementation,
          "version_directory": version_directory,
          "exe_path": exe_path,
          "arguments": arguments}

def createFullTestInstance(instance, extra_arguments=None):
  parameters = helperCreateTestInstance(instance)
  implementation = parameters["implementation"]
  
  arguments = parameters["arguments"].copy()
  if "offline_arguments" in implementation:
    arguments += implementation["offline_arguments"]
  if extra_arguments:
    arguments += extra_arguments
  
  solver_type = implementation["type"]
  if solver_type == "full":
    test_command = createNativeCommand(parameters["exe_path"],
                                       arguments)
    return {"cmd": test_command, 
            "version_directory": parameters["version_directory"]
           }
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

  return {"cmd": test_command,
          "version_directory": parameters["version_directory"]
         }

def runTestInstance(test_name, test_command, log_directory, fname, iteration,
                    timeout, *extra_arguments, log_fname=None):
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
    signal.alarm(timeout)
    try:
      running_command = test_command(input_path, out_path, *extra_arguments)
      
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
          # reset timeout
          signal.alarm(timeout)
        else:
          err_file.write(error_line)
      
      # clear timeout
      signal.alarm(0)  
      if running_command.exit_code != 0:
        raise ExitCodeException(result.exit_code)
    except Alarm:
      yield (("Timeout", "Timeout"))
      running_command.terminate()

class ReadCsv(object):
  def __init__(self, fifo_path, read_events):
    self.fifo_path = fifo_path
    self.read_events = read_events
    self.csv_rows = None
  def read(self):
    with open(self.fifo_path, 'r') as stats_file:
      csv_reader = csv.DictReader(stats_file)
      self.csv_rows = list(csv_reader)
    for e in self.read_events:
      e.set()
    
def wait_for_completion(running_command, events):
  running_command.wait()
  time.sleep(1)
  for e in events:
    e.set()

def runApproximateTestInstance(test_name, test_command, log_directory, fname,
          iteration, timeout, log_fname=None):
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
  
  fifo_path = os.path.join(log_directory, test_name + "-stats_fifo.csv")
  if os.path.exists(fifo_path):
    print("WARNING: removing stale FIFO ", fifo_path)
    os.unlink(fifo_path)
  os.mkfifo(fifo_path)
  arguments = ["--statistics", fifo_path]
  
  try:
    running_command = test_command(input_path, out_path, error_path=err_path,
                                   *arguments)
    
    wake_up = threading.Event()
    
    stats_read = threading.Event()
    stats_reader = ReadCsv(fifo_path, [stats_read, wake_up])
    process_completed = threading.Event()
    
    # spin up threads
    wait_t = threading.Thread(target=wait_for_completion,
                              args=(running_command, [process_completed, wake_up]))
    wait_t.start()
    reader_t = threading.Thread(target=ReadCsv.read, args=(stats_reader,))
    reader_t.setDaemon(True)
    reader_t.start()

    while not process_completed.isSet():
      wake_up.wait()
      wake_up.clear()
      
      if stats_read.isSet():
        # clear timeout 
        signal.alarm(0)
        
        yield stats_reader.csv_rows
        
        stats_read.clear()
        t = threading.Thread(target=ReadCsv.read, args=(stats_reader,))
        t.setDaemon(True)
        t.start()
        
        # reset timeout
        signal.alarm(timeout)
    # process has terminated, end
        
    # clear timeout
    signal.alarm(0)
      
    if running_command.exit_code != 0:
      raise ExitCodeException(result.exit_code)
  except Alarm:
    yield "Timeout"
    running_command.terminate()
  finally:
    os.unlink(fifo_path)

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
    
    timedout = set()
    for i in range(iterations):
      print(i, " ", end="")
      
      for test_name, test_config in test_instances.items():
        if test_name in timedout:
          continue
        
        log_directory = os.path.join(test_config["version_directory"],
                                     "log", case_name)
        timeout = case_config.get("timeout", config.DEFAULT_TIMEOUT)
        run = runTestInstance(test_name, test_config["cmd"], log_directory,
                              fname, i, timeout)
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
        
        if algorithm_time == "Timeout":
          timedout.add(test_name)
        
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
      
      timedout = set()
      for test_name, test_config in test_instances.items():
        if test_name in timedout:
          continue
        
        log_directory = os.path.join(test_config["version_directory"],
                                     "log", case_name)
        
        delta_id = 0
        timeout = case_config.get("timeout", config.DEFAULT_TIMEOUT)
        for (algorithm_time, time_elapsed) in runTestInstance(test_name, 
                                              test_config["cmd"], log_directory, 
                                              fname, i, timeout):
          result = { "test": test_name,
                     "file": fname,
                     "delta_id": delta_id, 
                     "iteration": i,
                     "algorithm_time": algorithm_time,
                     "total_time": time_elapsed }
          result_writer.writerow(result)
          result_file.flush()
          delta_id += 1
          
          if algorithm_time == "Timeout":
            timedout.add(test_name)
        
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
  if "cost_model" in trace_config:
    cost_model = trace_config["cost_model"]
  simulator = simulator.bake("-flow_scheduling_cost_model",
                             config.COST_MODELS[cost_model])
  machine_type = config.FIRMAMENT_DEFAULT_MACHINE
  if "machine" in trace_config:
    machine_type = trace_config["machine"]
  simulator = simulator.bake("-machine_tmpl_file",
                             config.FIRMAMENT_MACHINES[machine_type])
  
  ### Configuration for the solver
  simulator = simulator.bake("-solver", "custom")
  timeout = case_config.get("timeout", config.DEFAULT_TIMEOUT)
  simulator = simulator.bake("-solver_timeout", timeout)
  simulator = simulator.bake("-flow_scheduling_binary", parameters["exe_path"])
  arguments = parameters["arguments"]
  if arguments:
    simulator = simulator.bake("-custom_flow_scheduling_args", 
                               " ".join(arguments))
  # Note "-incremental_flow False" will not work; GFlags will interpret this
  # as setting FLAGS_incremental_flow, and then a positional argument False.
  # So have to construct the argument string ourselves
  simulator = simulator.bake("-incremental_flow=" + str(incremental))
  
  ### Configuration for the trace  
  simulator = simulator.bake("-trace_path", trace_spec["dir"])
  simulator = simulator.bake("-num_files_to_process", trace_spec["num_files"])
  if "runtime" in trace_config:
    simulator = simulator.bake("-runtime", trace_config["runtime"])
  if "num_events" in trace_config:
    simulator = simulator.bake("-max_events", trace_config["num_events"])
  if "scheduling_rounds" in trace_config:
    simulator = simulator.bake("-max_scheduling_rounds",
                               trace_config["scheduling_rounds"])
  if "percentage" in trace_config:
    simulator = simulator.bake("-percentage", trace_config["percentage"])
  
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
    hash = hashlib.md5()
    
    cli = " ".join([parameters["exe_path"]] + parameters["arguments"])
    hash.update(cli.encode('utf-8'))
    hash.update(str(simulator).encode('utf-8'))
    
    delta_file = os.path.join(trace_directory, hash.hexdigest() + ".imin")
    if os.path.exists(delta_file): 
      yield (delta_file, "cached")
      return
    else:
      yield (delta_file, "generating") 
    simulator = simulator.bake("-graph_output_file", delta_file) 
  
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
    
      # SOMEDAY(adam): if simulator dies, test script will hang waiting on
      # reading FIFO, as exception will not be generated whilst blocked on IO
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
  fieldnames = ["test", "file", "delta_id", 
                "iteration", "algorithm_time", "total_time"]
  result_writer = csv.DictWriter(result_file,fieldnames=fieldnames)
  result_writer.writeheader()
  iterations = case_config["iterations"]
  
  tests = case_config["tests"]
  test_instances = {test_name : createIncrementalTestInstance(tests[test_name])
                   for test_name in tests}
  
  for (dataset_name, dataset_config) in case_config["traces"].items():
    trace_name = dataset_config["trace"]
    trace_spec = config.TRACE_DATASET[trace_name]
    trace_files = {}
    
    print("\t", dataset_name, " - online: ", end="")
    
    for (test_name, test_instance) in tests.items(): 
      result = runSimulator(case_name, case_config, test_name, test_instance,
                  trace_name, dataset_config, trace_spec, 0, type="hybrid")
      result = list(result)
      assert(len(result) == 1)
      
      trace_files[test_name], status = result[0]
          
      print(test_name, 
            "*" if status == "cached" else "",
            sep="", end=" ")
        
    print("/ offline: ", end="")
    for i in range(iterations):
      print(i, " ", end="")
      
      timedout = set()
      
      for test_name, test_config in test_instances.items():
        if test_name in timedout:
          continue
        
        log_directory = os.path.join(test_config["version_directory"], 
                                     "log", case_name)
        
        delta_id = 0
        input_graph = trace_files[test_name]
        log_fname = os.path.relpath(os.path.join(log_directory, test_name),
                                    input_graph)
        timeout = case_config.get("timeout", config.DEFAULT_TIMEOUT)
        for (algorithm_time, time_elapsed) in runTestInstance(
                      test_name, test_config["cmd"], log_directory, input_graph, 
                      i, timeout, log_fname=log_fname):
          result = { "test": test_name,
                     "file": trace_name,
                     "delta_id": delta_id, 
                     "iteration": i,
                     "algorithm_time": algorithm_time,
                     "total_time": time_elapsed }
          result_writer.writerow(result)
          result_file.flush()
          delta_id += 1
          
          if algorithm_time == "Timeout":
            timedout.add(test_name)
        
    print("")

def runIncrementalOnlineTest(case_name, case_config, result_file):
  fieldnames = ["test", "dataset", "delta_id", "cluster_timestamp", "iteration",
      "scheduling_latency", "algorithm_time", "flowsolver_time", "total_time",
      "total_changes","new_node","remove_node",
      "new_arc","change_arc","remove_arc"]
  result_writer = csv.DictWriter(result_file,fieldnames=fieldnames)
  result_writer.writeheader()
  
  iterations = case_config["iterations"]
  tests = case_config["tests"]
  
  for (dataset_name, dataset_config) in case_config["traces"].items():
    trace_name = dataset_config["trace"]
    trace_spec = config.TRACE_DATASET[trace_name]
    
    timedout = set()
    
    print("\t", dataset_name, ": ", end="")
    
    for i in range(iterations):
      print(i, " ", end="")
      
      for (test_name, test_instance) in tests.items():
        if test_name in timedout:
          continue
        
        row_number = 0
        for row in runSimulator(case_name, case_config, test_name, test_instance,
                           trace_name, dataset_config, trace_spec, i, type="online"):
          result = { "test": test_name,
                     "dataset": dataset_name,
                     "delta_id": row_number, 
                     "iteration": i}
          fields = ["cluster_timestamp", "scheduling_latency", 
                    "algorithm_time", "flowsolver_time", "total_time",
                    "total_changes","new_node","remove_node",
                    "new_arc","change_arc","remove_arc"]
          result.update({k: row[k] for k in fields})
          result_writer.writerow(result)
          result_file.flush()
          row_number += 1
          
          if row["algorithm_time"] == "Timeout":
            timedout.add(test_name)
        
    print("")

APPROXIMATE_FIELDS = ["refine_iteration", "refine_time", "overhead_time",
                      "epsilon", "cost","task_assignments_changed"]
def runApproximateFullTest(case_name, case_config, result_file):
  fieldnames = ["file", "test_iteration"] + APPROXIMATE_FIELDS
  result_writer = csv.DictWriter(result_file, fieldnames=fieldnames)
  result_writer.writeheader()
  
  iterations = case_config["iterations"]
  instance = case_config.get("test", config.APPROXIMATE_DEFAULT_TEST)
  
  test_instance = createFullTestInstance(instance)
  
  for fname in case_config["files"]:
    print("\t", fname, ": ", end="")
    
    for i in range(iterations):
      print(i, " ", end="")
        
      log_directory = os.path.join(test_instance["version_directory"],
                                   "log", case_name)
      timeout = case_config.get("timeout", config.DEFAULT_TIMEOUT)
      
      run = runApproximateTestInstance("approximate", test_instance["cmd"], 
                                       log_directory, fname, i, timeout)
      test_results = list(run)
      assert(len(test_results) == 1)
      test_result = test_results[0]
      base_output = { "file": fname,
                      "test_iteration": i }
      if test_result == "Timeout":
        output = base_output.copy()
        output.update({"delta_id": 0,
                       "refine_iteration": 0,
                       "refine_time": "Timeout",
                       "overhead_time": "Timeout",
                       "epsilon": -1,
                       "cost": -1,
                       "task_assignments_changed": -1})
        break
      else:
        for row in test_results:
          output = base_output.copy()
          output.update(row)
          result_writer.writerow(output)
      result_file.flush()
        
    print("")
    
def runApproximateIncrementalOfflineTest(case_name, case_config, result_file):
  fieldnames = ["file", "delta_id", "test_iteration"] + APPROXIMATE_FIELDS
  result_writer = csv.DictWriter(result_file, fieldnames=fieldnames)
  result_writer.writeheader()
  
  iterations = case_config["iterations"]
  instance = case_config.get("test", config.APPROXIMATE_DEFAULT_TEST)
  
  test_instance = createIncrementalTestInstance(instance)
  
  for fname in case_config["files"]:
    print("\t", fname, ": ", end="")
    
    for i in range(iterations):
      print(i, " ", end="")
        
      log_directory = os.path.join(test_instance["version_directory"],
                                   "log", case_name)
      timeout = case_config.get("timeout", config.DEFAULT_TIMEOUT)
      
      run = runApproximateTestInstance("approximate", test_instance["cmd"], 
                                       log_directory, fname, i, timeout)
      delta_id = 0
      timedout = False
      for test_result in run:
        base_output = { "file": fname,
                        "delta_id": delta_id,
                        "test_iteration": i }
        if test_result == "Timeout":
          output = base_output.copy()
          output.update({"refine_iteration": 0,
                         "refine_time": "Timeout",
                         "overhead_time": "Timeout",
                         "epsilon": -1,
                         "cost": -1,
                         "task_assignments_changed": -1})
          result_writer.writerow(output)
          result_file.flush()
          timedout = True
          break
        else:
          for row in test_result:
            output = base_output.copy()
            output.update(row)
            result_writer.writerow(output)
          delta_id += 1
          result_file.flush()
        
      if timedout:
        break
        
    print("")
      
def runApproximateIncrementalHybridTest(case_name, case_config, result_file):
  fieldnames = ["file", "delta_id", "test_iteration"] + APPROXIMATE_FIELDS
  result_writer = csv.DictWriter(result_file, fieldnames=fieldnames)
  result_writer.writeheader()
  
  iterations = case_config["iterations"]
  test_instance = case_config.get("test", config.APPROXIMATE_DEFAULT_TEST)
  test_instance = createIncrementalTestInstance(test_instance)
  
  for (dataset_name, dataset_config) in case_config["traces"].items():
    trace_name = dataset_config["trace"]
    trace_spec = config.TRACE_DATASET[trace_name]
    
    print("\t", dataset_name, " - online: ", end="")
    
    result = runSimulator(case_name, case_config, "approximate", config.REFERENCE_SOLVER,
                trace_name, dataset_config, trace_spec, 0, type="hybrid")
    result = list(result)
    assert(len(result) == 1)
    
    trace_file, status = result[0]
          
    print(status, end=" ")
    
    print("/ offline: ", end="")
    for i in range(iterations):
      print(i, " ", end="")
        
      log_directory = os.path.join(test_instance["version_directory"],
                                   "log", case_name)
      timeout = case_config.get("timeout", config.DEFAULT_TIMEOUT)
      log_fname = os.path.relpath(os.path.join(log_directory, "approximate"),
                                  trace_file)
      run = runApproximateTestInstance("approximate", test_instance["cmd"], 
                    log_directory, trace_file, i, timeout, log_fname=log_fname)
      
      delta_id = 0
      timedout = False
      for test_result in run:
        base_output = { "file": dataset_name,
                        "delta_id": delta_id,
                        "test_iteration": i }
        if test_result == "Timeout":
          output = base_output.copy()
          output.update({"refine_iteration": 0,
                         "refine_time": "Timeout",
                         "overhead_time": "Timeout",
                         "epsilon": -1,
                         "cost": -1,
                         "task_assignments_changed": -1})
          result_writer.writerow(output)
          result_file.flush()
          timedout = True
          break
        else:
          for row in test_result:
            output = base_output.copy()
            output.update(row)
            result_writer.writerow(output)
          delta_id += 1
          result_file.flush()
        
      if timedout:
        break
        
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
      elif test_type == "approximate_full":
        runApproximateFullTest(case_name, case_config, result_file)
      elif test_type == "approximate_incremental_offline":
        runApproximateIncrementalOfflineTest(case_name, case_config, result_file)
      elif test_type == "approximate_incremental_hybrid":
        runApproximateIncrementalHybridTest(case_name, case_config, result_file)
      else:
        error("Unrecognised test type: ", test_type)

def findTestCases(test_patterns):
  test_cases = set()
  for pattern_regex in test_patterns:
    pattern = re.compile(pattern_regex + "$")
    for test_case in config.TESTS:
      if pattern.match(test_case):
        test_cases.add(test_case)
        
  return test_cases

if __name__ == "__main__":
  # Unbuffered output. This is needed for progress indicators to display correctly,
  # since a new line is printed only at the end of each test.    
  sys.stdout = flushfile(sys.stdout)
  
  # Install signal handler for timeouts
  signal.signal(signal.SIGALRM, alarm_handler)
      
  build_only = False
  dont_build = False
  test_cases = None
  if len(sys.argv) == 1:
    # no arguments
    test_cases = config.TESTS.keys()
  else:
    if sys.argv[1] == "--build-only":
      build_only = True
      flag_found = True 
    elif sys.argv[1] == "--dont-build":
      dont_build = True
      sys.argv[1:] = sys.argv[2:]
      
    # arguments are list of test patterns
    test_patterns = sys.argv[1:]
    test_cases = findTestCases(test_patterns)
    
  print("Running: ", test_cases)
  tests = {k : config.TESTS[k] for k in test_cases}

  if not dont_build:
    # Dangerous -- assumes that tests have already been built.
    # Should only be used by distributed_benchmark.py, where it's needed to
    # avoid race (two instances running in parallel try to build the same target)
    print("*** Building ***")
    implementations = findImplementations(tests)
    buildImplementations(implementations)
  
  if not build_only:
    print("*** Running tests ***")
    runTests(tests)