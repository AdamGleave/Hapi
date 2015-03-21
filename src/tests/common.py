import config.common

def extractSolution(command_res):
  solutions = []
  for line in command_res:
      fields = line.split()
      if fields[0] == "s":
          # solution
          solutions.append(int(fields[1]))
  return solutions
  
def runTest(testcase, test_programs, runReferenceCommand, runCommand):
  reference_solution = runReferenceCommand(testcase)
  if reference_solution == None:
      print("ERROR: Reference command failed on %s", testcase)
  else:
      print("Reference solution: ", reference_solution)
  
  for name, program in test_programs.items():
      solution = runCommand(testcase, program)
      if solution == reference_solution:
          print("PASS - ", name)
      else:
          print("FAIL - ", name, file=sys.stderr)
          print("Expected {0}, actual {1}".
                format(reference_solution, solution), file=sys.stderr)
          sys.exit(1)