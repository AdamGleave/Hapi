#!/usr/bin/env python3

import sys, os, re, sh, shutil

if len(sys.argv) < 3:
  print("usage: {0} <input dir> <output dir>".format(sys.argv[0]), file=sys.stderr)
  sys.exit(1)

input_dir = sys.argv[1]
output_dir = sys.argv[2]

SNAPSHOT_COMMAND = sh.Command("/home/srguser/adam/project/build/bin/incremental_snapshots").bake("last_only")

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
  snapshot_fname = os.path.join(output_dir, prefix + ".min")
  with open(os.path.join(output_dir, fname)) as f:
    SNAPSHOT_COMMAND(_in=f, _out=snapshot_fname)
