import os
import time

import numpy as np
import six
import dlib

from tensorflow.python.framework import ops
from tensorflow.python.training import session_run_hook
from tensorflow.python.training.session_run_hook import SessionRunArgs

def _as_graph_element(obj):
  """Retrieves Graph element."""
  graph = ops.get_default_graph()
  if not isinstance(obj, six.string_types):
    if not hasattr(obj, "graph") or obj.graph != graph:
      raise ValueError("Passed %s should have graph attribute that is equal "
                       "to current graph %s." % (obj, graph))
    return obj
  if ":" in obj:
    element = graph.as_graph_element(obj)
  else:
    element = graph.as_graph_element(obj + ":0")
    # Check that there is no :1 (e.g. it's single output).
    try:
      graph.as_graph_element(obj + ":1")
    except (KeyError, ValueError):
      pass
    else:
      raise ValueError("Name %s is ambiguous, "
                       "as this `Operation` has multiple outputs "
                       "(at least 2)." % obj)
  return element


class LearningRateHook(session_run_hook.SessionRunHook):
  """Use the technique described here: http://blog.dlib.net/2018/02/automatic-learning-rate-scheduling-that.html

  """

  def __init__(self, tensors, total_loss='total_loss', threshold=500, name='learn_rate'):
    """Initializes a `LearningRateHook`.

    Args:
      tensors: `dict` that maps string-valued tags to tensors/tensor names
    """
    assert isinstance(tensors, dict)
    self._name = name
    self._tag_order = tensors.keys()
    self._tensors = tensors
    self._histories = {tag: [0.0] for tag in tensors}
    self._max_history = threshold+1
    self._total_loss = total_loss
    self._threshold = threshold
    self._iter_count = 0
    self.run = True

  def begin(self):
    # Convert names to tensors if given
    self._current_tensors = {tag: _as_graph_element(tensor)
                             for (tag, tensor) in self._tensors.items()}

  def before_run(self, run_context):  # pylint: disable=unused-argument
    return SessionRunArgs(self._current_tensors)

  def _update_values(self, tensor_values):
    for tag in self._tag_order:
        self._histories[tag].append(tensor_values[tag])

  def after_run(self, run_context, run_values):
    _ = run_context
    self._update_values(run_values.results)
    self._iter_count += 1

  def ok(self):
    t = self._threshold
    for tag in self._tag_order:
        if len(self._histories[tag]) > self._max_history:
            self._histories[tag] = self._histories[tag][-self._max_history:]
            assert len(self._histories[tag]) == self._max_history
        steps1 = dlib.count_steps_without_decrease(self._histories[tag])
        steps2 = dlib.count_steps_without_decrease_robust(self._histories[tag])
        # print('LearningRateHook({}): tag({}), s1({}), s2({}), t({})'.format(self._name, tag, steps1, steps2, t))
        if steps1>t and steps2>t:
            print('Metric {} for {} has flatlined'.format(tag, self._name))
            if self.run and tag == self._total_loss:
                print('REQUESTING STOP', self._name)
                self.run = False
    return self.run

  def end(self, session):
    pass
