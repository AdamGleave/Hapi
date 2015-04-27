import matplotlib.pyplot as plt
import numpy as np

import itertools

import config.visualisation as config
from visualisation import analysis

def get_start_time(figconfig):
  return figconfig.get('start_time', config.DEFAULT_INCREMENTAL_START)

def analyse_generic(data, start_time):
  # extract times
  times = analysis.ion_extract_time(data, 'scheduling')
  times = analysis.ion_map_on_iterations(
                            analysis.ion_filter_cluster_time(start_time), times)
  # concatenate all iterations together
  times = analysis.ion_map_on_implementations(analysis.chain_lists, times)
  
  return times
  
def analyse_distribution(data, start_time, index1, group2):
  times = analyse_generic(data, start_time)
  times = analysis.ion_map_on_implementations(analysis.ion_get_scheduler_time,
                                              times)
  
  type, data = times
  data = data[index1]
  data = analysis.flatten_dict(data, [group2])
  
  return (type, data)

def plot_cdf(times, labels, colours, **kwargs):
  # currently am doing it the inefficient way
  # can compute empirical CDF using sampling with scipy
  # see http://stackoverflow.com/questions/3209362/
  # I don't think the datasets you're dealing with are big enough for this to matter
  num_groups = len(times)
  
  for i in range(num_groups):
    x = np.sort(times[i])
    y = np.linspace(0, 1, len(x), endpoint=False)
    label = labels[i]
    plt.plot(x, y, label=label, color=colours[label])
    
def generate_cdf(data, figconfig):
  data = analyse_distribution(data, get_start_time(figconfig),
                              figconfig['trace'], figconfig['implementations'])
  type, times = data
  plot_cdf(times, figconfig['implementations'], figconfig['colours'])
  
  plt.legend(loc='lower right')
  
  plt.xlabel('Scheduling latency (s)')
  plt.ylabel('Cumulative probability')
  plt.title('CDF for scheduling latency')

def plot_hist(times, labels, colours, sum_to_one=True, **kwargs):
  num_groups = len(times)
  
  weights = None 
  if sum_to_one:
    weights = []
    for i in range(len(times)):
      weights.append(np.ones_like(times[i]) / len(times[i]))
      
  colours = analysis.flatten_dict(colours, [labels])
  plt.hist(times, weights=weights, label=labels, color=colours, **kwargs)
  
def generate_hist(data, figconfig):
  data  = analyse_distribution(data, get_start_time(figconfig),
                               figconfig['trace'], figconfig['implementations'])
  type, times = data
  plot_hist(times, figconfig['implementations'], figconfig['colours'],
            histtype='bar', bins=10)
  
  plt.legend(loc='upper right')
  
  plt.xlabel('Scheduling latency (s)')
  plt.ylabel('Probability')
  plt.title('Distribution of scheduling latency')
  
# SOMEDAY: Would be nice to include some indication of variance
def generate_over_time(data, figconfig):
  # get data
  start_time = get_start_time(figconfig)
  data = analyse_generic(data, start_time)
  
  # sort it
  data = analysis.ion_map_on_implementations(lambda l : np.sort(l, order='cluster_time'), data)                                                                                                                                         
  
  cluster_times = analysis.ion_map_on_implementations(analysis.ion_get_cluster_timestamp, data)
  scheduling_latency = analysis.ion_map_on_implementations(analysis.ion_get_scheduler_time, data)
  
  cluster_times = cluster_times[1][figconfig['trace']]
  scheduling_latency = scheduling_latency[1][figconfig['trace']] 
  
  cluster_times = analysis.flatten_dict(cluster_times, [figconfig['implementations']])
  scheduling_latency = analysis.flatten_dict(scheduling_latency, [figconfig['implementations']])
  
  window_size = figconfig.get('window_size', config.DEFAULT_WINDOW_SIZE)
  colours = analysis.flatten_dict(figconfig['colours'], [figconfig['implementations']])
  for i in range(len(figconfig['implementations'])):
    smoothed = analysis.moving_average(scheduling_latency[i], window_size)
    plt.plot(cluster_times[i], smoothed,
             label=figconfig['implementations'][i], color=colours[i])
  
  plt.legend(loc='upper right')
  
  plt.xlabel('Cluster time (s)')
  plt.ylabel('Scheduling latency (s)')
  plt.title('Moving average of scheduling latency against time')