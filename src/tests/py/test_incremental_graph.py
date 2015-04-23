#!/usr/bin/env python3

import os
import sys
import sh

import config.incremental as config
import common

FIFO_PATH = os.path.join(config.PROJECT_ROOT, config.BUILD_PREFIX, "tests", 
                         config.FIFO_FNAME)

def runSingleTest(testcase, name, program):
  if os.path.exists(FIFO_PATH):
    print("WARNING: removing stale FIFO ", FIFO_PATH)
    os.unlink(FIFO_PATH)
  os.mkfifo(FIFO_PATH)

  compare_running = config.SNAPSHOTS_COMPARE(testcase(_piped="direct"),
                                             "/dev/stdin", # delta input
                                             FIFO_PATH, # snapshot input
                                             _bg=True)

  with open(FIFO_PATH, 'w') as output:
    # Note must open here rather than passing filename to _out
    # In the latter case, sh will keep the file open, and so the reader of the
    # FIFO will never receive open
    test_running = program(testcase(_piped="direct"),
                           _out=output.buffer, _bg=True)
  
    try:
      test_prog_result = test_running.wait()
      print("Test finished")
    except sh.ErrorReturnCode:
      print("FAIL - ", name, file=sys.stderr)
      print("Snapshot generator ", program, "terminated abnormally.")
      raise
      
  try:
    compare_result = compare_running.wait()
    print("PASS - ", name)
  except sh.ErrorReturnCode_42:
    print("FAIL - ", name, file=sys.stderr)
    print("Snapshots differ", file=sys.stderr)
    sys.exit(1)
  except sh.ErrorReturnCode: 
    print("Snapshot comparator terminated abnormally", file=sys.stderr)
    raise
  
  os.unlink(FIFO_PATH)
    
def runTest(testcase, test_programs):
  for name, program in test_programs.items():
    runSingleTest(testcase, name, program)

if __name__ == "__main__":
  for (main_graph_fname, delta_fnames) in config.TEST_CASES.items():
    print("***", main_graph_fname, "***")
    
    main_graph_path = os.path.join(config.DATASET_ROOT, main_graph_fname)
    if delta_fnames:
      # we have deltas to apply
      for delta_fname in delta_fnames:
        print("- ", delta_fname)
        
        delta_path = os.path.join(config.DATASET_ROOT, delta_fname)
        runTest(sh.cat.bake(main_graph_path, delta_path),
                config.GRAPH_TEST_PROGRAMS)
    else:
      # We're not concatenating everything here, but we still use the feature
      # that concatenateFiles rewinds the file for each test
      runTest(sh.cat.bake(main_graph_path), config.GRAPH_TEST_PROGRAMS)