import csv

FULL_FIELDNAMES = ["test", "file", "iteration", "algorithm_time", "total_time"]
OFFLINE_FIELDNAMES = ["test", "file", "delta_id", "iteration",
                      "algorithm_time", "total_time"]
CHANGE_FIELDNAMES = ["total_changes","new_node","remove_node",
                     "new_arc","change_arc","remove_arc"]
ONLINE_FIELDNAMES = ["test", "dataset", "delta_id", "cluster_timestamp", 
                     "iteration", "scheduling_latency", "algorithm_time", 
                     "flowsolver_time", "total_time"] + CHANGE_FIELDNAMES

def _parse(fname, expected_fieldnames):
  with open(fname) as csvfile:
    reader = csv.DictReader(csvfile)
    assert(reader.fieldnames == expected_fieldnames)
    data = list(reader)
    return data

def identity(x):
  return x
  
def full(fname, file_filter=identity, test_filter=identity):
  """Returns in format dict of filenames -> dict of implementations 
     -> array of iterations -> dict of times (algo, total)"""
  data = _parse(fname, FULL_FIELDNAMES)
  res = {}
  for row in data:
    file = file_filter(row['file'])
    if not file:
      continue
    
    file_res = res.get(file, {})
    
    test = test_filter(row['test'])
    if not test:
      continue
    
    test_res = file_res.get(test, [])
    test_res.append({'algo': row['algorithm_time'],
                     'total': row['total_time'], 
                     'changes': {
                            }})
    
    file_res[test] = test_res
    res[file] = file_res
    
  return res

def get_changes_dict(row):
  return {'total': row['total_changes'],
          'node': {
            'new': row['new_node'],
            'remove': row['remove_node']
          },
          'arc': {
            'add': row['new_arc'],
            'change': row['change_arc'],
            'remove': row['remove_arc']
          },
        }
  
def incremental_offline(fname, file_filter=identity, test_filter=identity):
  """Returns in format dict of filename/trace -> array indexed by delta IDs -> 
     -> dict of implementations -> array of iterations 
     -> dict of times (algo, total)
     
     Covers offline and hybrid tests."""
  data = _parse(fname, OFFLINE_FIELDNAMES)
  res = {}
  for row in data:
    file = file_filter(row['file'])
    if not file:
      continue
    
    file_res = res.get(file, [])
    delta_id = int(row['delta_id'])
    assert(delta_id <= len(file_res))
    if delta_id == len(file_res):
      file_res.append({})
    delta_res = file_res[delta_id]
    
    test = test_filter(row['test'])
    if not test:
      continue
    
    test_res = delta_res.get(test, [])
    test_res.append({'algo': row['algorithm_time'],
                     'total': row['total_time',
                     'changes': get_changes_dict(row)]})
    
    delta_res[test] = test_res
    file_res[delta_id] = delta_res
    res[file] = file_res
    
  return res

def incremental_online(fname, trace_filter=identity, test_filter=identity):
  """Returns in format dict of filename/trace -> dict of implementations ->  
     -> array of iterations 
     -> array of datums (cluster_timestamp, dict of algo, total, flowsolver, scheduling) 
     
     Covers offline and hybrid tests."""
  data = _parse(fname, ONLINE_FIELDNAMES)
  res = {}
  for row in data:
    trace = trace_filter(row['dataset'])
    if not trace:
      continue
    
    trace_res = res.get(trace, {})
    
    test = test_filter(row['test'])
    if not test:
      continue
    test_res = trace_res.get(test, [])
    
    iteration = int(row['iteration'])
    assert(iteration <= len(test_res))
    if iteration == len(test_res):
      test_res.append([])
    iteration_res = test_res[iteration]    
    iteration_res.append(
      (row['cluster_timestamp'], 
       { 'scheduling': row['scheduling_latency'],
         'algo': row['algorithm_time'], 
         'flowsolver': row['flowsolver_time'], 
         'total': row['total_time'],
         'changes': get_changes_dict(row)
       })
    )
    
    test_res[iteration] = iteration_res
    trace_res[test] = test_res
    res[trace] = trace_res
    
  return res