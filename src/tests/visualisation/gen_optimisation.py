import matplotlib.pyplot as plt
import numpy as np

def swapFileImpl(data):
  new_data = {} 
  for (file_name, file_res) in data.items():
    for (impl_name, impl_res) in file_res.items():
      new_impl_res = new_data.get(impl_name, {})
      new_file_res = impl_res
      new_impl_res[file_name] = new_file_res
      new_data[impl_name] = new_impl_res
      
  return new_data

def convertTime(time_str):
  if time_str == 'Timeout':
    return float('+inf')
  else:
    return float(time_str)
    
def mapOnFull(func, d):
  res = {k1 : 
           {k2 : func(v2) 
            for k2, v2 in v1.items()}
         for k1, v1 in d.items()}
  return res

def extractTime(d, final_key):
  def timeLambda(x):
    return list(map(lambda x : convertTime(x[final_key]), x))
  return mapOnFull(timeLambda, d)

def summaryStats(d):
  def statsLambda(x):
    return {'mean': np.mean(x), 'sd': np.std(x)}
  return mapOnFull(statsLambda, d)

def extractSummaryStats(l):
  mean = [x['mean'] for x in l]
  sd = [x['sd'] for x in l]
  return (mean, sd)
  
def flattenDict(d, keys):
  return [d[k] for k in keys]

def generate(data, figconfig):
  # get means and standard deviation of each implementation on each dataset
  data = swapFileImpl(data)
  times = extractTime(data, 'algo')
  stats = summaryStats(times)      
  
  fig, ax = plt.subplots()
  
  n_groups = len(figconfig['datasets'])
  index = np.arange(n_groups)
  bar_width = 0.7 / n_groups
  
  opacity = 0.4
  error_config = {'ecolor': '0.3'}
 
  nbar = 0
  for implementation in figconfig['implementations']:
    data = flattenDict(stats[implementation], figconfig['datasets'])
    mean, sd = extractSummaryStats(data)
    
    bars = plt.bar(index + nbar * bar_width, mean, bar_width,
                   log=True,
                   alpha=opacity,
                   color=figconfig['colours'][implementation],
                   yerr=sd,
                   error_kw=error_config,
                   label=implementation)
    
    nbar = nbar + 1
  
  plt.xlabel('Data set')
  plt.ylabel('Runtime (s)')
  plt.title('Runtimes by data set size and version')
  plt.xticks(index + bar_width, figconfig['datasets'])
  plt.legend(loc='upper left')
  
  plt.tight_layout()
  
  return fig