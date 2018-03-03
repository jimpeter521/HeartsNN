#!/usr/bin/env python3
# train.py

import glob
import math
import numpy as np
import os
import shutil
import sys

import tensorflow as tf

from model import model_fn
from constants import *
from learning_rate_hook import LearningRateHook

ROOT_MODEL_DIR = './model_dir'

class IteratorInitializerHook(tf.train.SessionRunHook):
    """Hook to initialise data iterator after Session is created."""

    def __init__(self):
        super(IteratorInitializerHook, self).__init__()
        self.iterator_initializer_func = None

    def after_create_session(self, session, coord):
        """Initialise the iterator after the session has been created."""
        self.iterator_initializer_func(session)


def batchShape(shape):
    return (None,) + shape

def npBatchShape(shape):
    return (-1,) + shape

def load_memmap(filePath, rowShape):
    data = np.memmap(filePath, mode='r', dtype=np.float32)
    data = np.reshape(data, npBatchShape(rowShape))
    return data

def load_memmaps(dirPath):
    mainData = load_memmap(dirPath + '/main_data.np.mmap', MAIN_INPUT_SHAPE)
    scoresData = load_memmap(dirPath + '/score_data.np.mmap', SCORES_SHAPE)
    winTrickProbs = load_memmap(dirPath + '/trick_data.np.mmap', WIN_TRICK_PROBS_SHAPE)
    moonProbData = load_memmap(dirPath + '/moon_data.np.mmap', MOONPROBS_SHAPE)

    nsamples = len(mainData)
    assert len(scoresData) == nsamples
    assert len(winTrickProbs) == nsamples
    assert len(moonProbData) == nsamples

    print('Loaded {} samples'.format(nsamples))

    return mainData, scoresData, winTrickProbs, moonProbData

# We load all training and validation data into (virtual) RAM by using numpy memmap files.
# We can fairly easily generate plenty of data and still easily fit it all into RAM on a machine with 32Gb of RAM.
# The amount of data is much larger than a reasonable batch size.
#
# We make sequential passes through the data.
# For training, we instruct estimator.train() to do STEPS steps with batch-size BATCH, and then return
# so that we can run evaluation. We evaluate (from the validation data) the same total number of samples BATCH*STEPS,
# but we tell estimator.evaluate() that we're doing 1 step with batch size BATCH*STEPS.
# We call one iteration of processing BATCH*STEPS samples processed an "evaluation", and the number of samples
# an "eval_size"
#
# At the end of each sequential pass through the data, we increase the batch size. In the usual case we simply
# double the BATCH size, and leave STEPS unchanged. But eventually this will make the eval_size greater than the
# number of samples available. We also need to be careful that rounding down doesn't leave us processing only
# slightly more than half of the available data.
#
# The code below to achieve this is a little messy, but works as is. I'm defering making it prettier as I want
# to move on to other improvements.

def get_input_fn(name, memmaps):
    mainData, scoresData, winTrickProbs, moonProbData = memmaps

    iterator_initializer_hook = IteratorInitializerHook()

    nsamples = len(mainData)
    assert nsamples == len(scoresData)
    assert nsamples == len(winTrickProbs)
    assert nsamples == len(moonProbData)

    def input_fn():
        p = np.random.permutation(nsamples)

        with tf.variable_scope('input_fn'):
            main_data_placeholder = tf.placeholder(tf.float32, batchShape(MAIN_INPUT_SHAPE), name=MAIN_DATA)
            scores_data_placeholder = tf.placeholder(tf.float32, batchShape(SCORES_SHAPE), name=EXPECTED_SCORE)
            win_trick_data_placeholder = tf.placeholder(tf.float32, batchShape(WIN_TRICK_PROBS_SHAPE), name=WIN_TRICK_PROB)
            moon_data_placeholder = tf.placeholder(tf.float32, batchShape(MOONPROBS_SHAPE), name=MOON_PROB)

        dataset = tf.data.Dataset.from_tensor_slices((main_data_placeholder, (scores_data_placeholder, win_trick_data_placeholder, moon_data_placeholder)))
        iterator_initializer_hook.iterator_initializer_func = \
            lambda sess: sess.run(
                iterator.initializer,
                feed_dict={main_data_placeholder: mainData[p],
                           scores_data_placeholder: scoresData[p],
                           win_trick_data_placeholder: winTrickProbs[p],
                           moon_data_placeholder: moonProbData[p]
                           })
        iterator = dataset.batch(BATCH).make_initializable_iterator()

        (main, (scores, win_trick, moon)) = iterator.get_next()

        return {MAIN_DATA: main}, {EXPECTED_SCORE: scores, WIN_TRICK_PROB: win_trick, MOON_PROB: moon}

    return input_fn, iterator_initializer_hook

def save_checkpoint(estimator, model_dir_path, serving_input_receiver_fn):
    export_dir_base = model_dir_path + '/savedmodel'
    os.makedirs(export_dir_base, exist_ok=True)
    checkpoint = estimator.latest_checkpoint()
    print('Saving checkpoint:', checkpoint, file=sys.stderr)
    estimator.export_savedmodel(export_dir_base, serving_input_receiver_fn, checkpoint_path=checkpoint)

def train_with_params(train_memmaps, eval_memmaps, params, serving_input_receiver_fn=None):
    hidden_depth = params['hidden_depth']
    hidden_width = params['hidden_width']
    activation = params['activation']
    num_batches = params['num_batches']
    threshold = params['threshold']

    model_dir_path = '{}/d{}w{}_{}'.format(ROOT_MODEL_DIR, hidden_depth, hidden_width, activation)
    os.makedirs(model_dir_path, exist_ok=True)

    params['model_dir_path'] = model_dir_path

    train_input_fn, train_iterator_initializer_hook = get_input_fn(TRAINING, train_memmaps)
    eval_input_fn, eval_iterator_initializer_hook = get_input_fn(VALIDATION, eval_memmaps)

    config = tf.estimator.RunConfig(keep_checkpoint_max=20, save_summary_steps=num_batches, save_checkpoints_steps=num_batches)

    losses = {
        'total_loss': 'total_loss/total_loss:0',
        'win_trick_prob_loss': 'win_trick_prob_loss/win_trick_prob_loss:0',
        'moon_prob_loss': 'moon_prob_loss/moon_prob_loss:0',
        'expected_score_loss': 'expected_score_loss/expected_score_loss:0'
        }

    backsteps = 0
    best_eval_loss = math.inf

    train_learning_rate_hook = LearningRateHook(losses, threshold=threshold, name='train')
    eval_learning_rate_hook = LearningRateHook(losses, threshold=threshold, name='eval')

    estimator = tf.estimator.Estimator(model_fn=model_fn, params=params, model_dir=model_dir_path, config=config)
    while train_learning_rate_hook.ok() and eval_learning_rate_hook.ok():
        estimator.train(train_input_fn, steps=num_batches, hooks=[train_iterator_initializer_hook, train_learning_rate_hook])
        evaluation = estimator.evaluate(eval_input_fn, steps=num_batches, hooks=[eval_iterator_initializer_hook, eval_learning_rate_hook])
        eval_loss = evaluation['loss']
        if best_eval_loss > eval_loss:
            backsteps = 0
            best_eval_loss = eval_loss
            best_eval = evaluation
            save_checkpoint(estimator, model_dir_path, serving_input_receiver_fn)
            print("New best eval loss:", best_eval_loss)
        elif backsteps >= 5:
            break
        else:
            backsteps += 1
            print("Backstep {}: Current loss {} is worse that prior best loss {}".format(backsteps, eval_loss, best_eval_loss))

    return best_eval

if __name__ == '__main__':
    tf.logging.set_verbosity(tf.logging.INFO)

    # TODO: Make this a command line option
    # if os.path.isdir(ROOT_MODEL_DIR):
    #     shutil.rmtree(ROOT_MODEL_DIR)

    dataDir = 'xx.m' if len(sys.argv)==1 else sys.argv[1]

    train_dir = 'training/' + dataDir
    eval_dir = 'validation/' + dataDir

    train_memmaps = load_memmaps(train_dir)
    eval_memmaps = load_memmaps(eval_dir)

    if len(train_memmaps[0]) > len(eval_memmaps[0]):
        print('Swapping training and eval so that eval is the larger dataset')
        train_memmaps, eval_memmaps = eval_memmaps, train_memmaps
    assert len(train_memmaps[0]) <= len(eval_memmaps[0])

    feature_spec = {
        MAIN_DATA: tf.placeholder(dtype=np.float32, shape=batchShape(MAIN_INPUT_SHAPE), name=MAIN_DATA),
    }
    serving_input_receiver_fn = tf.estimator.export.build_raw_serving_input_receiver_fn(feature_spec)

    num_batches = (len(eval_memmaps[0]) + BATCH - 1) // BATCH
    threshold = num_batches*5
    print('num_batches, threshold:', num_batches, threshold)

    evals = {}
    for hidden_depth in [2]:
        for hidden_width in [200]:
            for activation in ['relu']:
                params = {
                    'hidden_depth': hidden_depth,
                    'hidden_width': hidden_width,
                    'activation': activation,
                    'num_batches': num_batches,
                    'threshold': threshold,
                }
                results = train_with_params(train_memmaps, eval_memmaps, params, serving_input_receiver_fn=serving_input_receiver_fn)
                evals['d{}w{}_{}'.format(hidden_depth, hidden_width, activation)] = results

    for k, v in evals.items():
        print(v, k, file=sys.stderr)
