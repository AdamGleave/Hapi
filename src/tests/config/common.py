import os, sys, glob, itertools, functools, sh

# For reading DIMACS input
BUFFER_SIZE = 4 * 1024

### Directories
# SCRIPT_ROOT = PROJECT_ROOT/src/tests/config/
SCRIPT_ROOT = os.path.dirname(os.path.realpath(__file__))
PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(SCRIPT_ROOT)))
BUILD_PREFIX = "build"
SOURCE_PREFIX = "src"
DATASET_ROOT = os.path.join(PROJECT_ROOT, SOURCE_PREFIX, "graphs")
EXECUTABLE_DIR = os.path.join(PROJECT_ROOT, BUILD_PREFIX, "bin")
EXECUTABLE_SRC_DIR = os.path.join(PROJECT_ROOT, SOURCE_PREFIX, "bin")

### Goldberg's CS2 solver
# Known-working reference implementation
REFERENCE_PROGRAM_PATH = os.path.join(PROJECT_ROOT, BUILD_PREFIX, "cs2", "cs2")
REFERENCE_PROGRAM_ARGUMENTS = []

### Incremental DIMACS

# Program to merge incremental deltas with full graph
SNAPSHOT_CREATOR_PROGRAM_PATH = os.path.join(EXECUTABLE_DIR,
                                             "incremental_snapshots")
SNAPSHOT_CREATOR_PROGRAM = sh.Command(SNAPSHOT_CREATOR_PROGRAM_PATH)

# Program to run full solver repeatedly for each snapshot
SNAPSHOT_SOLVER_PROGRAM_PATH  = os.path.join(EXECUTABLE_DIR,
                                             "snapshot_solver")
SNAPSHOT_SOLVER_PROGRAM = sh.Command(SNAPSHOT_SOLVER_PROGRAM_PATH)

### Convenience functions

#For specifying files
def prefix_list(prefix, fnames):
  return list(map(functools.partial(os.path.join, prefix), fnames))

def graphGlob(pathname):
  fnames = glob.glob(os.path.join(DATASET_ROOT, pathname))
  return list(map(lambda x : os.path.relpath(x, DATASET_ROOT), fnames))

# For merging dictionaries, and tagging elements
def mergeDicts(dicts, tags):
  assert(len(dicts) == len(tags))
  for i in range(len(dicts)):
    for j in range(len(dicts)):
      if i == j:
        continue
      duplicate_keys = set(dicts[i].keys()) & set(dicts[j].keys())
      if duplicate_keys:
        print("ERROR: Same name used in implementation ",
              tags[i], " and ", tags[j], ": ", duplicate_keys, file=sys.stderr)
        sys.exit(1)
      
  # no overlapping keys
  result = {}
  for (dict,tag) in zip(dicts,tags):
    # N.B. Mutates as side-effect, this is OK or even desirable though
    for v in dict.values():
      v.update({"type": tag})
    result.update(dict)
  
  return result