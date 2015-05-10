import matplotlib.pyplot as plt
import numpy as np

from visualisation import analysis

def cdf(times, labels, colours, annotate_means=set(), annotate_means_format=str,
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
      mean = np.mean(x)
      plt.axvline(mean, color=colour, linestyle='dashed')
      
      annotation = r"$\mu = {0}$".format(annotate_means_format(mean))
      plt.annotate(annotation, xy=(mean, ymin + 0.05*y_width), xycoords='data',
                   xytext=(1, 1), textcoords='offset points', rotation='vertical',
                   verticalalignment='bottom')

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