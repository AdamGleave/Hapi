import os, glob, itertools, functools, sh

# For reading DIMACS input
BUFFER_SIZE = 4 * 1024

# Directories
# SCRIPT_ROOT = PROJECT_ROOT/src/tests/config/
SCRIPT_ROOT = os.path.dirname(os.path.realpath(__file__))
PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(SCRIPT_ROOT)))
BUILD_ROOT = "build"
SOURCE_ROOT = "src"
DATASET_ROOT = os.path.join(PROJECT_ROOT, SOURCE_ROOT, "graphs")
EXECUTABLE_DIR = os.path.join(PROJECT_ROOT, BUILD_ROOT, "bin")

# Goldberg's CS2 solver
# Known-working reference implementation
REFERENCE_PROGRAM_PATH = os.path.join(PROJECT_ROOT, "build", "cs2", "cs2")
REFERENCE_PROGRAM = sh.Command(REFERENCE_PROGRAM_PATH)

# Convenience functions for specifying files
def prefix_list(prefix, fnames):
  return list(map(functools.partial(os.path.join, prefix), fnames))

def graphGlob(pathname):
  fnames = glob.glob(os.path.join(DATASET_ROOT, pathname))
  return list(map(lambda x : os.path.relpath(x, DATASET_ROOT), fnames))