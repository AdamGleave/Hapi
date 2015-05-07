import matplotlib.pyplot as plt
import numpy as np

from visualisation import analysis

def cdf(times, labels, colours, **kwargs):
  # currently am doing it the inefficient way
  # can compute empirical CDF using sampling with scipy
  # see http://stackoverflow.com/questions/3209362/
  # I don't think the datasets you're dealing with are big enough for this to matter
  num_groups = len(times)
  
  for i in range(num_groups):
    x = np.sort(times[i])
    y = np.linspace(0, 100, len(x), endpoint=False)
    label = labels[i]
    plt.plot(x, y, label=label, color=colours[label])
    
  plt.ylabel('Cumulative probability (\%)')
    
def hist(times, labels, colours, sum_to_one=True, **kwargs):
  num_groups = len(times)
  
  weights = None 
  if sum_to_one:
    weights = []
    for i in range(len(times)):
      weights.append(np.ones_like(times[i]) / len(times[i]))
      
  colours = analysis.flatten_dict(colours, [labels])
  plt.hist(times, weights=weights, label=labels, color=colours, **kwargs)