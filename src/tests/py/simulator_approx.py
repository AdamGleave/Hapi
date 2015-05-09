from config.distributed import *
import redo

GENERATOR_PATH = os.path.join(REMOTE_ROOT_DIR,
                                      "project/src/scripts/approx_generator.sh")
OUTPUT_DIR = "/mnt/data/adam_scratch/approx_quincy"

if __name__ == "__main__":
  if len(sys.argv) != 2:
    print >>sys.stderr, "usage: ", sys.argv[0], "<seed file>"
    
  seed_fname = sys.argv[1]
  with open(seed_fname) as seed_file:
    n_seeds = len(list(seed_file))
  
  machines = getMachines()
  redo = redo.Redo(machines, USER)
  
  n_machines = len(machines)
  seeds_per_machine = float(n_seeds) / n_machines
  seeds_per_machine = math.ceil(seeds_per_machine)
  
  pids = []
  for i in range(len(machines)):
    start = i * seeds_per_machine
    end = min((i + 1) * seeds_per_machine, len(seeds))
    cmdline = ' '.join([GENERATOR_PATH, OUTPUT_DIR,
                        seed_file, str(start), str(end)])
    machine = machines[i]
    pids.append(redo[machine].run(cmdline, block=False)[0])
    print >>sys.stderr, machine, " - generating ", start, " to ", end
  
  # XXX: Think this won't work?
  wait_res = redo.wait(pids)
  print "All tasks finished"
  success = True
  for i in range(len(wait_res)):
    return_code = wait_res[i]
    if return_code != 0:
      print >>sys.stderr, "WARNING: command failed on ", machines[i], \
                          " return code ", return_code
      success = False
      
  if not success:
    sys.exit(1)
  
  for i in range(len(machines)):
    start = i * seeds_per_machine
    end = min((i + 1) * seeds_per_machine, len(seeds))
    for i in range(start, end):
      fname = os.path.join(OUTPUT_DIR, str(i) + ".min")
      src_paths.append(fname)
      dst_paths.append(fname)
    machine = machines[i]
    print "Copying from ", machine
    redo[machine].copy_from(src_paths, dst_paths)