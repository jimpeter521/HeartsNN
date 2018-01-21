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
from constants import (MAIN_INPUT_SHAPE, SCORES_SHAPE, WIN_TRICK_PROBS_SHAPE, MOONPROBS_SHAPE, CARDS_IN_DECK
                    , EPOCHS, BATCH, MAIN_DATA, EXPECTED_SCORE, MOON_PROB, WIN_TRICK_PROB)

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
    mainData = load_memmap(f'{dirPath}/main_data.np.mmap', MAIN_INPUT_SHAPE)
    scoresData = load_memmap(f'{dirPath}/scores_data.np.mmap', SCORES_SHAPE)
    winTrickProbs = load_memmap(f'{dirPath}/win_trick_data.np.mmap', WIN_TRICK_PROBS_SHAPE)
    moonProbData = load_memmap(f'{dirPath}/moon_data.np.mmap', MOONPROBS_SHAPE)

    nsamples = len(mainData)
    assert len(scoresData) == nsamples
    assert len(winTrickProbs) == nsamples
    assert len(moonProbData) == nsamples

    # TODO: Make this a parameter
    MAX_BATCHES = 40
    batches = min(MAX_BATCHES, max(nsamples // BATCH, 1))
    N = min(batches * BATCH, nsamples)
    print(f'Loaded data from {dirPath}, will do {batches} batches for total of {N} samples')
    return mainData[:N], scoresData[:N], winTrickProbs[:N], moonProbData[:N]

def get_input_fn(memmaps, shuffle=False):
    mainData, scoresData, winTrickProbs, moonProbData = memmaps

    iterator_initializer_hook = IteratorInitializerHook()

    def input_fn():

        with tf.variable_scope('input_fn'):
            main_data_placeholder = tf.placeholder(tf.float32, batchShape(MAIN_INPUT_SHAPE), name=MAIN_DATA)
            scores_data_placeholder = tf.placeholder(tf.float32, batchShape(SCORES_SHAPE), name=EXPECTED_SCORE)
            win_trick_data_placeholder = tf.placeholder(tf.float32, batchShape(WIN_TRICK_PROBS_SHAPE), name=WIN_TRICK_PROB)
            moon_data_placeholder = tf.placeholder(tf.float32, batchShape(MOONPROBS_SHAPE), name=MOON_PROB)

        dataset = tf.data.Dataset.from_tensor_slices((main_data_placeholder, (scores_data_placeholder, win_trick_data_placeholder, moon_data_placeholder)))
        iterator_initializer_hook.iterator_initializer_func = \
            lambda sess: sess.run(
                iterator.initializer,
                feed_dict={main_data_placeholder: mainData,
                           scores_data_placeholder: scoresData,
                           win_trick_data_placeholder: winTrickProbs,
                           moon_data_placeholder: moonProbData
                           })
        dataset = dataset.repeat(1).batch(BATCH)
        if shuffle:
            dataset = dataset.shuffle(BATCH*3)
        iterator = dataset.make_initializable_iterator()
        (main, (scores, win_trick, moon)) = iterator.get_next()
        return {MAIN_DATA: main}, {EXPECTED_SCORE: scores, WIN_TRICK_PROB: win_trick, MOON_PROB: moon}

    return input_fn, iterator_initializer_hook

def save_checkpoint(estimator, model_dir_path, serving_input_receiver_fn):
    export_dir_base = f'{model_dir_path}/savedmodel'
    os.makedirs(export_dir_base, exist_ok=True)
    checkpoint = estimator.latest_checkpoint()
    print('Saving checkpoint:', checkpoint, file=sys.stderr)
    estimator.export_savedmodel(export_dir_base, serving_input_receiver_fn, checkpoint_path=checkpoint)

def train_with_params(train_memmaps, eval_memmaps, params):
    hidden_depth = params['hidden_depth']
    hidden_width = params['hidden_width']
    activation = params['activation']

    model_dir_path = f'{ROOT_MODEL_DIR}/d{hidden_depth}w{hidden_width}_{activation}'
    os.makedirs(model_dir_path, exist_ok=True)

    config = tf.estimator.RunConfig(keep_checkpoint_max=20)
    estimator = tf.estimator.Estimator(model_fn=model_fn, params=params, model_dir=model_dir_path, config=config)

    feature_spec = {
        MAIN_DATA: tf.placeholder(dtype=np.float32, shape=batchShape(MAIN_INPUT_SHAPE), name=MAIN_DATA),
    }
    serving_input_receiver_fn = tf.estimator.export.build_raw_serving_input_receiver_fn(feature_spec)

    first_backstep_loss = None
    BACKSTEP_LIMIT = 10
    backsteps = 0
    best_eval_loss = float('inf')

    train_input_fn, train_iterator_initializer_hook = get_input_fn(train_memmaps, shuffle=True)
    eval_input_fn, eval_iterator_initializer_hook = get_input_fn(eval_memmaps)

    for batch in range(1, EPOCHS+1):
        estimator.train(train_input_fn, steps=None, hooks=[train_iterator_initializer_hook])
        evaluation = estimator.evaluate(eval_input_fn, steps=None, hooks=[eval_iterator_initializer_hook])
        print(f'{batch}/{EPOCHS}: Evaluation:', evaluation, file=sys.stderr)
        eval_loss = evaluation['loss']
        if best_eval_loss > eval_loss:
            best_eval_loss = eval_loss
            best_eval = evaluation
            if backsteps > 0:
                print(f'Recovered from backstep with new best loss:{best_eval_loss}', file=sys.stderr)
                backsteps = 0
                save_checkpoint(estimator, model_dir_path, serving_input_receiver_fn)
        elif backsteps < BACKSTEP_LIMIT:
            backsteps += 1
            print('Evaluation backstep:', backsteps, file=sys.stderr)
            if first_backstep_loss is None:
                first_backstep_loss = best_eval_loss
        else:
            break

    save_checkpoint(estimator, model_dir_path, serving_input_receiver_fn)
    delta = None if first_backstep_loss is None else 100 * (first_backstep_loss - best_eval_loss) / first_backstep_loss
    return {'best_eval': best_eval, 'first_backstep_loss': first_backstep_loss, 'delta': delta}

if __name__ == '__main__':
    tf.logging.set_verbosity(tf.logging.INFO)

    # TODO: Make this a command line option
    # if os.path.isdir(ROOT_MODEL_DIR):
    #     shutil.rmtree(ROOT_MODEL_DIR)

    dataDir = 'dxx' if len(sys.argv)==1 else sys.argv[1]

    train_dir = f'training/{dataDir}'
    eval_dir = f'validation/{dataDir}'

    train_memmaps = load_memmaps(train_dir)
    eval_memmaps = load_memmaps(eval_dir)

    evals = {}
    for hidden_width in [340]:
        for hidden_depth in [5, 6, 7]:
            for activation in ['swish5']:
                params = {
                    'hidden_depth': hidden_depth,
                    'hidden_width': hidden_width,
                    'activation': activation
                }
                results = train_with_params(train_memmaps, eval_memmaps, params)
                evals[f'd{hidden_depth}w{hidden_width}_{activation}'] = results

    for k, v in evals.items():
        print(v, k, file=sys.stderr)
