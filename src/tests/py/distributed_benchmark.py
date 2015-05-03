#!/usr/bin/env python2

import sys, os, re

import config.benchmark as config

import redo

REMOTE_ROOT_DIR = "/home/srguser/adam/"
REMOTE_BENCHMARK_DIR = os.path.join(REMOTE_ROOT_DIR, "project/benchmark")
REMOTE_BENCHMARK_CMD = os.path.join(REMOTE_ROOT_DIR, 
                                    "project/src/tests/py/benchmark.py")
LOCAL_BENCHMARK_DIR = "/home/srguser/adam/benchmark_results"
MACHINE_LIST = "/home/srguser/adam/deploy/machines"

USER = "srguser"

POLL = 1

def findTestCases(test_patterns):
  test_cases = set()
  for pattern_regex in test_patterns:
    pattern = re.compile(pattern_regex + "$")
    for test_case in config.TESTS:
      if pattern.match(test_case):
        test_cases.add(test_case)

  return test_cases
        
def createSchedule(test_cases, machines):
  task_schedule = {machine : [] for machine in machines}
  machine_i = 0
  while test_cases:
    test_case = test_cases.pop()
    machine = machines[machine_i]
    task_schedule[machine].append(test_case)
    machine_i = (machine_i + 1) % len(machines)
  
  return task_schedule

def startBuilds(redo, machines, task_schedule):
  build_pids = {}
  for machine in machines:
    if task_schedule[machine]:
      cmdline = [REMOTE_BENCHMARK_CMD, "--build-only"] + task_schedule[machine]
      cmdline = " ".join(cmdline)
      pid = redo[machine].run(cmdline, block=False)
      print machine, " - build launched"
      build_pids[machine] = pid    
  return build_pids

def startTest(redo, task_schedule, machine):
  machine_cases = task_schedule[machine]
  assert(machine_cases) # not empty
  cmdline = [REMOTE_BENCHMARK_CMD] + machine_cases 
  cmdline = " ".join(cmdline)
  return redo[machine].run(cmdline, block=False)
  
def wait(redo, pids):
  while pids:
    for machine, pid in pids.items():
      return_code = redo[machine].wait(pid, timeout=POLL)[0]
      if return_code == None:
        # process has not finished
        continue
      if return_code != 0:
        # process has finished and non-zero return code
        print >>sys.stderr, "ERROR: command failed on ", machine, \
                            " return code ", return_code
        print >>sys.stderr, redo.getoutput(pid)
        sys.exit(1)
      if return_code == 0:
        yield machine
        del pids[machine]

if __name__ == "__main__":
  with open(MACHINE_LIST) as machine_file:
    machines = list(machine_file)
    machines = [x.strip() for x in machines]
    
  redo = redo.Redo(machines, USER)
    
  test_cases = list(findTestCases(sys.argv[1:]))
  print "Running: ", test_cases
  
  task_schedule = createSchedule(test_cases, machines)
  print "Execution schedule: "
  for machine in machines:
    print "\t", machine, " - ", task_schedule[machine]
    
  build_pids = startBuilds(redo, machines, task_schedule)
  benchmark_pids = {}
  for machine_ready in wait(redo, build_pids):
    print machine_ready, " - build finished, starting test"
    pid = startTest(redo, task_schedule, machine_ready)
    benchmark_pids[machine_ready] = pid
    
  for machine_finished in wait(redo, benchmark_pids):
    print machine_ready, " - test finished, ", 
    machine_cases = task_schedule[machine]
    for case in machine_cases:
      fname = case + ".csv"
      redo[machine].copy_from(os.path.join(REMOTE_BENCHMARK_DIR, fname),
                              os.path.join(LOCAL_BENCHMARK_DIR, fname))
    print "files copied"
   
  print "All done!"
