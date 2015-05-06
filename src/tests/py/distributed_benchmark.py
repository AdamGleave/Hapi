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
      print >>sys.stderr, machine, " - build launched"
      build_pids[machine] = {"build": pid[0]}    
  return build_pids

def startTests(redo, task_schedule, machine):
  machine_cases = task_schedule[machine]
  pids = {}
  for case in machine_cases:
    cmdline = [REMOTE_BENCHMARK_CMD, "--dont-build"] + [case]
    cmdline = " ".join(cmdline)
    pid = redo[machine].run(cmdline, block=False)
    pids[case] = pid[0]
  return pids
  
def wait(redo, pids):
  while pids:
    for machine, machine_pids in pids.items():
      case_pids = machine_pids.items()
      pids_to_wait = [v for k, v in case_pids]
      return_codes = redo[machine].wait(pids_to_wait, timeout=POLL)
      for (return_code, (case, pid)) in zip(return_codes, case_pids):
        if return_code == None:
          # process has not finished
          continue
        if return_code != 0:
          # process has finished and non-zero return code
          print >>sys.stderr, "WARNING: command failed on ", machine, \
                              " return code ", return_code
          print >>sys.stderr, redo[machine].getoutput([pid])
        if return_code == 0:
          yield (machine, case)
        del pids[machine][case]
        if not pids[machine]: # empty
          del pids[machine]

if __name__ == "__main__":
  with open(MACHINE_LIST) as machine_file:
    machines = list(machine_file)
    machines = [x.strip() for x in machines]
  
  if len(sys.argv) < 3:
    print >>sys.stderr, "usage: ", sys.argv[0], " <working directory> <pattern1> [pattern2] ..."
    sys.exit(1)
  working_directory = sys.argv[1]
  redo = redo.Redo(machines, USER, workdir=working_directory)
    
  test_cases = list(findTestCases(sys.argv[1:]))
  print >>sys.stderr, "Running: ", test_cases
  
  task_schedule = createSchedule(test_cases, machines)
  print >>sys.stderr, "Execution schedule: "
  for machine in machines:
    print >>sys.stderr, "\t", machine, " - ", task_schedule[machine]
    
  build_pids = startBuilds(redo, machines, task_schedule)
  benchmark_pids = {}
  for (machine_ready, _case) in wait(redo, build_pids):
    print >>sys.stderr, machine_ready, " - build finished, starting test"
    pids = startTests(redo, task_schedule, machine_ready)
    benchmark_pids[machine_ready] = pids
    
  for (machine, case) in wait(redo, benchmark_pids):
    print >>sys.stderr, machine, " - test ", case, " finished on machine ", 
    fname = case + ".csv"
    src_path = os.path.join(REMOTE_BENCHMARK_DIR, fname)
    dst_path = os.path.join(LOCAL_BENCHMARK_DIR, fname)
    pid = redo[machine].copy_from(src_path, dst_path)
    return_code = redo[machine].wait(pid)[0]
    if return_code != 0:
      print >>sys.stderr, "ERROR: copying failed on ", machine, \
                          " for ", src_path, "->", dst_path
      print >>sys.stderr, redo[machine].getoutput(pid)
      sys.exit(1)
    print >>sys.stderr, "results copied"
   
  print >>sys.stderr, "All done!"