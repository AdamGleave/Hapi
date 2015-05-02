import os, sys, glob, itertools, functools, sh

# For reading DIMACS input
BUFFER_SIZE = 4 * 1024

### Directories
# SCRIPT_ROOT = PROJECT_ROOT/src/tests/py/config/
SCRIPT_ROOT = os.path.dirname(os.path.realpath(__file__))
PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(SCRIPT_ROOT))))
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
SNAPSHOT_CREATOR_PROGRAM_ARGUMENTS = ["quiet"]
SNAPSHOT_CREATOR_PROGRAM = sh.Command(SNAPSHOT_CREATOR_PROGRAM_PATH) \
                             .bake(*SNAPSHOT_CREATOR_PROGRAM_ARGUMENTS)
# Program to run full solver repeatedly for each snapshot
SNAPSHOT_SOLVER_PROGRAM_PATH  = os.path.join(EXECUTABLE_DIR,
                                             "snapshot_solver")
SNAPSHOT_SOLVER_PROGRAM = sh.Command(SNAPSHOT_SOLVER_PROGRAM_PATH)
                            

### Convenience functions

#For specifying files
def prefix_list(prefix, fnames):
  return list(map(functools.partial(os.path.join, prefix), fnames))

def prefix_dict(prefix, d):
  return {k : os.path.join(prefix, v) for (k,v) in d.items()}

def graphGlob(pathname):
  fnames = glob.glob(os.path.join(DATASET_ROOT, pathname))
  return list(map(lambda x : os.path.relpath(x, DATASET_ROOT), fnames))

# For merging dictionaries, and tagging elements
def mergeDicts(dicts, prefix, tags=None):
  assert(len(dicts) == len(prefix))
  tags_enabled = bool(tags)
  if tags_enabled:
    assert(len(dicts) == len(tags))
  else:
    tags = [None] * len(dicts) # for zip
  
  result = {}
  for (dict,tag,prefix) in zip(dicts,tags,prefix):
    new_dict = {k : v.copy() for (k,v) in dict.items()}
    if tags_enabled:
      for v in new_dict.values():
        v.update({"type": tag})
    new_dict = {prefix + "_" + k : v for (k,v) in new_dict.items()}
    result.update(new_dict)
  
  return result

def extendDict(d1, d2):
  d = d1.copy()
  d.update(d2)
  return d