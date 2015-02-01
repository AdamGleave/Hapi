def extractSolution(command_res):
    solution = None
    for line in command_res:
        fields = line.split()
        if fields[0] == "s":
            # solution
            solution = int(fields[1])
    return solution