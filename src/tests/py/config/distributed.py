REMOTE_ROOT_DIR = "/home/srguser/adam/"
MACHINE_LIST = "/home/srguser/adam/deploy/machines"
USER = "srguser"

def getMachines():
  with open(MACHINE_LIST) as machine_file:
    machines = list(machine_file)
    machines = [x.strip() for x in machines]
  return machines