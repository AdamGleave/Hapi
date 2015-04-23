import matplotlib.pyplot as plt
import numpy as np

import itertools

import config.visualisation as config
from visualisation import analysis

def plot_cdf(times, labels, colours, **kwargs):
  # currently am doing it the inefficient way
  # can compute empirical CDF using sampling with scipy
  # see http://stackoverflow.com/questions/3209362/
  # I don't think the datasets you're dealing with are big enough for this to matter
  num_groups, num_elts = np.shape(sorted)
  ecdf = np.linspace(0, 1, num_elts, endpoint=False)
  sorted = np.sort(times) 
  
  for i in range(num_groups):
    label = labels[i]
    plt.plot(sorted[i], ecdf, label=label, color=colours[label])
    
def plot_hist(times, labels, colours, sum_to_one=True, **kwargs):
  colour_array = analysis.flatten_dict(colours, [labels])
  
  times = np.transpose(times)
  weights = None
  if sum_to_one:
    # Make sum of histogram heights = one.
    # This differs from normed option of hist which makes *area*
    # under histograms equal to one.
    num_elts, num_groups = np.shape(times)
    weights = np.ones_like(times) / num_elts
  plt.hist(times, weights=weights, label=labels, color=colour_array, **kwargs)

def analyse_distribution(data, index1, group2):
  times = analysis.ion_extract_time(data, 'scheduling')
  times = analysis.ion_drop_cluster_timestamp(times)
  
  # for the CDF, we can just concatenate all iterations together
  times = analysis.ion_map_on_implementations(analysis.chain_lists, times)
  
  times = times[index1]
  times = analysis.flatten_dict(times, [group2])
  
  return times

def generate_cdf(data, figconfig):
  times = analyse_distribution(data, 
                               figconfig['trace'], figconfig['implementations'])
  plot_cdf(times, figconfig['implementations'], figconfig['colours'])
  
  plt.legend(loc='lower right')
  
  plt.xlabel('Scheduling latency (s)')
  plt.ylabel('Cumulative probability')
  plt.title('CDF for scheduling latency')
  
def generate_hist(data, figconfig):
  times = analyse_distribution(data, 
                               figconfig['trace'], figconfig['implementations'])
  plot_hist(times, figconfig['implementations'], figconfig['colours'],
            bins=10)
  
  plt.legend(loc='upper right')
  
  plt.xlabel('Scheduling latency (s)')
  plt.ylabel('Probability')
  plt.title('Distribution of scheduling latency')