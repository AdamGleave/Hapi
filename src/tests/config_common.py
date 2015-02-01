import os
import sh

# For reading DIMACS input
BUFFER_SIZE = 4 * 1024

SCRIPT_ROOT = os.path.dirname(os.path.realpath(__file__))
BASE_DIR = os.path.dirname(os.path.dirname(SCRIPT_ROOT))
EXECUTABLE_DIR = os.path.join(BASE_DIR, "build", "bin")

# Goldberg's CS2 solver
# Known-working reference implementation
REFERENCE_PROGRAM_PATH = os.path.join(BASE_DIR, "build", "cs2", "cs2")
REFERENCE_PROGRAM = sh.Command(REFERENCE_PROGRAM_PATH)