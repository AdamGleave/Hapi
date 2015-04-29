import itertools

import matplotlib.pyplot as plt
import numpy as np

import config.visualisation as config
from visualisation import analysis, plot

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

def adjust_units(times):
  reduced_times = np.concatenate(times)
  max_time = np.max(reduced_times)
  if max_time >= 1:
    return (r'\si{\second}', times)
  else:
    times = np.multiply(times, 1000)
    return (r'\si{\milli\second}', times)

def _generate_cdf_helper(data, figconfig, incremental_only):
  if incremental_only:
    implementations = [figconfig['incremental_implementation']]
  else:
    implementations = figconfig['implementations']
  data = analyse_distribution(data, get_start_time(figconfig),
                              figconfig['trace'], implementations)
  type, times = data
  
  unit, times = adjust_units(times)
  
  plot.cdf(times, implementations, figconfig['colours'])
  
  xmin, xmax = plt.xlim()
  xmin = -xmax*0.05
  plt.xlim(xmin, xmax)
  
  plt.legend(loc='lower right')
  
  plt.xlabel('Scheduling latency ({0})'.format(unit))
  plt.ylabel('Cumulative probability')
  plt.title('CDF for scheduling latency')
  
def generate_cdf(data, figconfig):
  _generate_cdf_helper(data, figconfig, False)
  
def generate_incremental_only_cdf(data, figconfig):
  _generate_cdf_helper(data, figconfig, True)
  
def generate_hist(data, figconfig):
  data  = analyse_distribution(data, get_start_time(figconfig),
                               figconfig['trace'], figconfig['implementations'])
  type, times = data
  plot.hist(times, figconfig['implementations'], figconfig['colours'],
            histtype='bar', bins=10)
  
  plt.legend(loc='upper right')
  
  plt.xlabel('Scheduling latency (\si{\second})')
  plt.ylabel('Probability')
  plt.title('Distribution of scheduling latency')
  
# SOMEDAY: Would be nice to include some indication of variance
def generate_over_time(data, figconfig):
  # get data
  start_time = get_start_time(figconfig)
  data = analyse_generic(data, start_time)
  
  # some traces might be empty, filter
  type, times = data
  times = {figconfig['trace'] : times[figconfig['trace']]}
  data = type, times
  
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
    #return(scheduling_latency[i])
    smoothed = analysis.moving_average(scheduling_latency[i], window_size)
    plt.plot(cluster_times[i], smoothed,
             label=figconfig['implementations'][i], color=colours[i])
  
  plt.legend(loc='upper right')
  
  plt.xlabel('Cluster time (\si{\second})')
  plt.ylabel('Scheduling latency (\si{\second})')
  plt.title('Moving average ($\mathrm{{window}} = {0}$) of scheduling latency against time'.format(window_size))