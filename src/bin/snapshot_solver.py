#!/usr/bin/env python3

import sys, sh

# usage: <program> <path to solver> [solver args]
#

USAGE = """<path to solver> [solver args]
    Reads a stream of flow network snapshots, executing the specified solver
    with specified arguments for each snapshot.
    
    Snapshots are in standard DIMACS form. Each snapshot must be separated by
    a comment, "c EOI". The output of the solver is also separated by this
    comment.
    
    If the program terminates abnormally for any snapshot, this program 
    terminates immediately, with the same exit code."""

def processInput(file):
  first_line = file.readline()
  if first_line == '': 
    # EOF
    return None
  else:
    def inputGenerator(file, first_line):
      yield first_line
      for line in file:
        if line == "c EOI\n":
          return
        else:
          yield line
    return inputGenerator(file, first_line)
             
if __name__ == "__main__":
  if len(sys.argv) < 2:
    print(USAGE, file=sys.stderr)
    sys.exit(1)
  
  SOLVER = sh.Command(sys.argv[1]).bake(*(sys.argv[2:]))
  
  while True:
    input = processInput(sys.stdin)
    if input == None:
      break
    # Intersperse STDOUT with c EOI
    # Pass through STDERR
    # Note we need to use the underlying binary buffers, as sh will just pipe
    # the binary input directly
    program = SOLVER(_in=input, 
                     _out=sys.stdout.buffer, 
                     _err=sys.stderr.buffer)
    program.wait()
    
    # Make sure ALGOTIME, output on stderr, is never reordered between c EOI.
    # (Wouldn't actually matter to programs parsing the output, but would be 
    # confusing if just looking at logs.)
    sys.stdout.buffer.flush()
    sys.stderr.buffer.flush()
    
    print("c EOI")