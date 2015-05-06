#!/usr/bin/env python3

import sys, os, re, sh, shutil

# SCRIPT_ROOT = PROJECT_ROOT/src/scripts/
SCRIPT_ROOT = os.path.dirname(os.path.realpath(__file__))
PROJECT_ROOT = os.path.dirname(os.path.dirname(SCRIPT_ROOT))

if len(sys.argv) < 3:
  print("usage: {0} <input dir> <output dir>".format(sys.argv[0]), file=sys.stderr)
  sys.exit(1)

input_dir = sys.argv[1]
output_dir = sys.argv[2]

FIRST_SNAPSHOT_COMMAND = sh.Command("/home/srguser/adam/project/src/scripts/cut_snapshot.sh")
LAST_SNAPSHOT_COMMAND = sh.Command("/home/srguser/adam/project/build/bin/incremental_snapshots").bake("last_only")

file_names = ['small.imin', 'medium.imin', 'large.imin', 'full_size.imin']
pattern = re.compile('.*\.imin$')

matching_files = []
for fname in os.listdir(input_dir):
  if pattern.match(fname):
    matching_files.append(fname)

sizes = [(os.stat(os.path.join(input_dir, fname)).st_size, fname)
         for fname in matching_files]
sizes.sort() # ascending order of size

for ((size,old_fname),new_fname) in zip(sizes, file_names):
  shutil.move(os.path.join(input_dir, old_fname), 
              os.path.join(output_dir, new_fname))

for fname in file_names:
  prefix = fname[:-5] # strip out .imin
  first_snapshot_fname = prefix + "_first.min"
  first_snapshot_fname = os.path.join(output_dir, first_snapshot_fname)
  last_snapshot_fname = prefix + "_last.min"
  last_snapshot_fname = os.path.join(output_dir, last_snapshot_fname)
  input_path = os.path.join(output_dir, fname)
  
  FIRST_SNAPSHOT_COMMAND("1", input_path, _out=first_snapshot_fname)
  with open(input_path) as f:
    SNAPSHOT_COMMAND(_in=f, _out=last_snapshot_fname)
