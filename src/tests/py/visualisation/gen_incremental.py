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
  sorted = np.sort(times)
  num_groups, num_elts = np.shape(sorted) 
  ecdf = np.arange(num_elts) / float(num_elts)
  
  # matplotlib expects column-by-row, but we have row-by-columns
  print(np.shape(sorted), np.shape(ecdf))
  x = np.transpose(sorted)
  y = np.transpose(ecdf)
  print(np.shape(x),np.shape(y))
  
  plt.plot(x, y)

def generate_cdf(data, figconfig):
  times = analysis.ion_extract_time(data, 'algo')
  times = analysis.ion_drop_cluster_timestamp(times)
  
  # for the CDF, we can just concatenate all iterations together
  times = analysis.ion_map_on_implementations(analysis.chain_lists, times)
  
  times = times[figconfig['trace']]
  times = analysis.flatten_dict(times, [figconfig['implementations']]) 
  
  plot_cdf(times, figconfig['implementations'], figconfig['colours'])