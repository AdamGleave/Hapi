import csv

FULL_FIELDNAMES = ["test", "file", "iteration", "algorithm_time", "total_time"]
OFFLINE_FIELDNAMES = ["test", "file", "delta_id", "iteration",
                      "algorithm_time", "total_time"]

CHANGE_FIELDNAMES = ["total_changes","new_node","remove_node",
                     "new_arc","change_arc","remove_arc"]
ONLINE_FIELDNAMES = ["test", "dataset", "delta_id", "cluster_timestamp", 
                     "iteration", "scheduling_latency", "algorithm_time", 
                     "flowsolver_time", "total_time"] + CHANGE_FIELDNAMES
                     
APPROXIMATE_FIELDNAMES = ["refine_iteration", "refine_time", "overhead_time",
                          "epsilon", "cost","task_assignments_changed"]
APPROXIMATE_FULL_FIELDNAMES = ["file", "test_iteration"] + APPROXIMATE_FIELDNAMES
APPROXIMATE_INCREMENTAL_OFFLINE_FIELDNAMES = \
                 ["file", "delta_id", "test_iteration"] + APPROXIMATE_FIELDNAMES

def _parse(fname, expected_fieldnames):
  with open(fname) as csvfile:
    reader = csv.DictReader(csvfile)
    assert(reader.fieldnames == expected_fieldnames)
    data = list(reader)
    return data

def identity(x):
  return x

def _helper_full_or_offline(type, fname,
                            file_filter=identity, test_filter=identity):
  """Returns in format dict of filenames -> dict of implementations 
     -> array of iterations -> dict of times (algo, total)"""
  data = None
  if type == "full":
    data = _parse(fname, FULL_FIELDNAMES)
  else:
    data = _parse(fname, OFFLINE_FIELDNAMES)
  
  res = {}
  for row in data:
    file = file_filter(row['file'])
    if not file:
      continue
    
    file_res = res.get(file, {})
    
    delta_id = None
    if type == "full":
      dict_of_implementations = file_res
    else:
      delta_id = int(row['delta_id'])
      assert(delta_id <= len(file_res))
      if delta_id == len(file_res):
        file_res.append({})
      dict_of_implementations = file_res[delta_id]
    
    test = test_filter(row['test'])
    if not test:
      continue
    
    test_res = dict_of_implementations.get(test, [])
    test_res.append({'algo': row['algorithm_time'],
                     'total': row['total_time']})
    
    dict_of_implementations[test] = test_res
    if type == 'incremental_offline':
      file_res[delta_id] = dict_of_implementations
    res[file] = file_res
    
  return (type, res)
  
def full(*args, **kwargs):
  """Returns in format dict of filenames -> dict of implementations 
     -> array of iterations -> dict of times (algo, total)"""
  return _helper_full_or_offline('full', *args, **kwargs)

def incremental_offline(fname, file_filter=identity, test_filter=identity):
  """Returns in format dict of filename/trace -> array indexed by delta IDs -> 
     -> dict of implementations -> array of iterations 
     -> dict of times (algo, total)
     
     Covers offline and hybrid tests."""
  return _helper_full_or_offline('incremental_offline', *args, **kwargs)

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
  
  return ('incremental_online', res)
  return res

def time_conversion(s):
  if s == "Timeout":
    return float('+inf')
  else:
    # time is in microseconds
    return float(s) / (1000 * 1000)
APPROXIMATE_CONVERSIONS = {"file": str,
                           "delta_id": int,
                           "test_iteration": int,
                           "refine_iteration": int,
                           "epsilon": int,
                           "cost": int,
                           "task_assignments_changed": int,
                           "refine_time": time_conversion,
                           "overhead_time": time_conversion}

def _helper_approximate_full_or_offline(type, fname, file_filter=identity):
  data = None
  if type == "full":
    data = _parse(fname, APPROXIMATE_FULL_FIELDNAMES)
  else:
    data = _parse(fname, APPROXIMATE_INCREMENTAL_OFFLINE_FIELDNAMES)
  
  res = {}
  delta_id_map_of_files = {}
  for row in data:
    row = {k : APPROXIMATE_CONVERSIONS[k](v) for k,v in row.items()}
    file = file_filter(row['file'])
    if not file:
      continue
    file_res = res.get(file, [])
    
    if type == "full":
      array_of_iterations = file_res
    else:
      delta_id_map = delta_id_map_of_files.get(file, {})
      delta_id = int(row['delta_id'])
      if delta_id not in delta_id_map:
        index = len(delta_id_map)
        delta_id_map[delta_id] = index
      index = delta_id_map[delta_id]
      assert(index <= len(file_res))
      if index == len(file_res):
        file_res.append([])
      array_of_iterations = file_res[index]
      delta_id_map_of_files[file] = delta_id_map
    
    test_iteration = int(row['test_iteration'])
    assert(test_iteration <= len(array_of_iterations))
    if test_iteration == len(array_of_iterations):
      array_of_iterations.append([])
    refine_iteration_res = array_of_iterations[test_iteration] 
    
    assert(int(row['refine_iteration']) == len(refine_iteration_res))
    output_fieldnames = APPROXIMATE_FIELDNAMES[1:] # chop off refine_iteration
    refine_iteration_res.append({k : row[k] for k in output_fieldnames})
    
    array_of_iterations[test_iteration] = refine_iteration_res
    if type == "incremental_offline":
      file_res[index] = array_of_iterations
    res[file] = file_res
  
  return ('approximate_' + type, res)

def approximate_full(*args, **kwargs):
  """Returns in format dict of filenames -> array of test iterations 
     -> array of refine iterations -> dict of parameters 
     (refine_time, overhead_time, epsilon, cost, task_assignments_changed)"""
  return _helper_approximate_full_or_offline('full', *args, **kwargs)

def approximate_incremental_offline(*args, **kwargs):
  """Returns in format dict of filenames -> array of delta IDs
     -> array of test iterations -> array of refine iterations 
     -> dict of parameters (refine_time, overhead_time, epsilon, 
     cost, task_assignments_changed)"""
  return _helper_approximate_full_or_offline('incremental_offline', *args, **kwargs)