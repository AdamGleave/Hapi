import matplotlib.pyplot as plt
import numpy as np

from visualisation import analysis

def cdf(times, labels, colours, annotate_means={}, annotate_means_format=str,
        ymin=0, **kwargs):
  # currently am doing it the inefficient way
  # can compute empirical CDF using sampling with scipy
  # see http://stackoverflow.com/questions/3209362/
  # I don't think the datasets you're dealing with are big enough for this to matter
  num_groups = len(times)
  
  y_width = 100 - ymin
  for i in range(num_groups):
    x = np.sort(times[i])
    y = np.linspace(0, 100, len(x))
    label = labels[i]
    colour = colours[label]
    plt.plot(x, y, label=label, color=colour)
    
    # optional: add line indicating mean of this distribution
    if label in annotate_means:
      config = annotate_means[label]
      mean = np.mean(x)
      plt.axvline(mean, color=colour, linestyle='dashed')
      
      if not config:
        config = {}
      if 'y_loc' in config:
        config = config.copy()
        y_loc = config['y_loc']
        del config['y_loc']
      else:
        y_loc = 0.05
      config['xytext'] = config.get('xytext', (1.5,0))
      annotation = r"$\mu = {0}$".format(annotate_means_format(mean))
      plt.annotate(annotation, xy=(mean, ymin + y_loc * y_width), xycoords='data',
                   textcoords='offset points', rotation='vertical',
                   verticalalignment='bottom', **config)

  plt.ylim(ymin, 100)
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