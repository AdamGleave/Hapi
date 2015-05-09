#!/usr/bin/env python2

import os, sys, math

from config.distributed import *
import redo

GENERATOR_PATH = os.path.join(REMOTE_ROOT_DIR,
                                      "project/src/scripts/approx_generator.sh")
OUTPUT_DIR = "/mnt/data/adam_scratch/approx_quincy"

def wait(pids):
  success = True
  wait_res = redo.wait(pids)
  for i in range(len(wait_res)):
    return_code = wait_res[i]
    if return_code != 0:
      print >>sys.stderr, "WARNING: command failed on ", machines[i], \
                          " return code ", return_code
      success = False
  return success

if __name__ == "__main__":
  if len(sys.argv) != 2:
    print >>sys.stderr, "usage: ", sys.argv[0], "<seed file>"
    sys.exit(1)
    
  seed_fname = sys.argv[1]
  with open(seed_fname) as seed_file:
    n_seeds = len(list(seed_file))
  
  machines = getMachines()
  redo = redo.Redo(machines, USER)
  
  n_machines = len(machines)
  seeds_per_machine = float(n_seeds) / n_machines
  seeds_per_machine = int(math.ceil(seeds_per_machine))
  
  pids = []
  for i in range(len(machines)):
    start = i * seeds_per_machine + 1
    end = min((i + 1) * seeds_per_machine, n_seeds - 1)
    cmdline = ' '.join([GENERATOR_PATH, seed_fname,
                        OUTPUT_DIR, str(start), str(end)])
    machine = machines[i]
    pids.append(redo[machine].run(cmdline, block=False)[0])
    print >>sys.stderr, machine, " - generating ", start, " to ", end
  
  # XXX: Think this won't work?
  
  print "All tasks finished"
  success = wait(pids)
  if not success:
    sys.exit(1)
  
  copy_pids = []
  for i in range(len(machines)):
#     start = i * seeds_per_machine + 1
#     end = min((i + 1) * seeds_per_machine, n_seeds - 1)
#     src_paths = [os.path.join(OUTPUT_DIR, str(i) + ".min") 
#                  for j in range(start,end)]
#     dst_paths = list(src_paths)
    machine = machines[i]
    print "Copying from ", machine
    copy_pids.append(redo[machine].copy_from(OUTPUT_DIR, 
                                             os.path.dirname(OUTPUT_DIR))[0])
    
  success = wait(copy_pids)
  if not success:
    sys.exit(1)