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
    return (r'\second', times)
  else:
    times = [np.multiply(x, 1000) for x in times]
    return (r'\milli\second', times)

def _generate_cdf_helper(data, figconfig, implementations, restrict_means=None,
                         annotate_means=None, target_latency=None,
                         ymin=0, xmax_bound=None):
  if not annotate_means: 
    if 'annotate_means' in figconfig:
      annotate_means = figconfig['annotate_means']
    else:
      annotate_means = {k : None for k in figconfig['implementations']}
  if restrict_means:
    annotate_means = {k : annotate_means[k] for k in restrict_means}
  data = analyse_distribution(data, get_start_time(figconfig),
                              figconfig['trace'], implementations)
  type, times = data
  
  unit, times = adjust_units(times)
  
  def format(mean):
    return r"\SI{{{0:.3}}}{{{1}}}".format(mean, unit)
  plot.cdf(times, implementations, figconfig['colours'], 
           annotate_means=annotate_means, annotate_means_format=format, 
           ymin=ymin)
  
  xmin, xmax = plt.xlim()
  if xmax_bound:
    xmax = min(xmax, xmax_bound)
  xmin = -xmax*0.05
  plt.xlim(xmin, xmax)
  
  plt.legend(loc='lower right')
  
  plt.xlabel('Scheduling latency (\si{{{0}}})'.format(unit))
  
  return unit, times
  
def generate_cdf(data, figconfig): 
  _generate_cdf_helper(data, figconfig, figconfig['implementations'], 
                       restrict_means=figconfig['means'])
  
def generate_incremental_only_cdf(data, figconfig):
  _generate_cdf_helper(data, figconfig, 
                       [figconfig['incremental_implementation']])
  
def generate_incremental_only_target_latency_cdf(data, figconfig):
  ymin = figconfig['target_latency_min_prob']
  y_width = 100 - ymin
  unit, times = _generate_cdf_helper(data, figconfig,
                  [figconfig['incremental_implementation']], {},
                  xmax_bound=figconfig['target_latency_max_latency'], ymin=ymin)
    
  target_latency = figconfig['target_latency']
  plt.axvline(target_latency, color='k', linestyle='dashed')
  annotation = r"\SI{{{0:.3}}}{{{1}}} latency".format(target_latency, unit)
  plt.annotate(annotation, xy=(target_latency, ymin + 0.05*y_width), xycoords='data',
               xytext=(1, 1), textcoords='offset points', rotation='vertical',
               verticalalignment='bottom')
  
  inc_times = np.array(times[0])
  num_meeting_target = np.sum(inc_times <= target_latency)
  percent_meeting_target = float(num_meeting_target) / len(inc_times) * 100.0
  target_annotation = r'{0:.3}\%'.format(percent_meeting_target)
  plt.axhline(percent_meeting_target, color='k', linestyle='dashed')
  plt.annotate(target_annotation,
               xy=(target_latency, percent_meeting_target), xycoords='data',
               xytext=(-1,1), textcoords='offset points',
               horizontalalignment='right')
  
def generate_hist(data, figconfig):
  data  = analyse_distribution(data, get_start_time(figconfig),
                               figconfig['trace'], figconfig['implementations'])
  type, times = data
  plot.hist(times, figconfig['implementations'], figconfig['colours'],
            histtype='bar', bins=10)
  
  plt.legend(loc='upper right')
  
  plt.xlabel('Scheduling latency (\si{\second})')
  plt.ylabel('Probability')
  
# SOMEDAY: Would be nice to include some indication of variance
def generate_over_time(data, figconfig):
  # get data
  start_time = get_start_time(figconfig)
  data = analyse_generic(data, start_time)
  
  # some traces might be empty, filter
  datatype, times = data
  times = {figconfig['trace'] : times[figconfig['trace']]}
  data = datatype, times
  
  # sort it
  data = analysis.ion_map_on_implementations(lambda l : np.sort(l, order='cluster_time'), data)                                                                                                                                         
  
  cluster_times = analysis.ion_map_on_implementations(analysis.ion_get_cluster_timestamp, data)
  scheduling_latency = analysis.ion_map_on_implementations(analysis.ion_get_scheduler_time, data)
  
  cluster_times = cluster_times[1][figconfig['trace']]
  scheduling_latency = scheduling_latency[1][figconfig['trace']] 
  
  cluster_times = analysis.flatten_dict(cluster_times,
                                        [figconfig['implementations']])
  scheduling_latency = analysis.flatten_dict(scheduling_latency,
                                             [figconfig['implementations']])
  
  window_size = figconfig.get('window_size', config.DEFAULT_WINDOW_SIZE)
  window_size, window_type = int(window_size[0:-1]), window_size[-1]
  colours = analysis.flatten_dict(figconfig['colours'],
                                  [figconfig['implementations']])
  
  for i in range(len(figconfig['implementations'])):
    x = scheduling_latency[i]
    t = cluster_times[i]
    if window_type == 'p': # points
      ma_config = {'points_window': window_size}
    elif window_type == 's': # seconds
      ma_config = {'time_window': window_size}
    else:
      assert(false) 
    smoothed_t, smoothed_x = analysis.moving_average(x, t, **ma_config)
    
    plt.plot(smoothed_t, smoothed_x, 
             label=figconfig['implementations'][i], color=colours[i])
  
  ymin, ymax = plt.ylim()
  ymin = -ymax*0.05
  plt.ylim(ymin, ymax)
  
  plt.legend(loc='upper right')
  
  plt.xlabel('Cluster time (\si{\second})')
  plt.ylabel('Scheduling latency (\si{\second})')
#   if window_type == 'p':
#     window_desc = '{0}\:\mathrm{{points}}'.format(window_size)
#   elif window_type == 's':
#     window_desc ='\SI{{ {0} }}{{\second}}'.format(window_size)
#     plt.title('Moving average ($\mathrm{{window}} = {0}$)\nof scheduling latency against time'.format(window_desc))