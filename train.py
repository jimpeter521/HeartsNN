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

    print(f'Loaded {nsamples} samples')

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
    global BATCH, STEPS

    mainData, scoresData, winTrickProbs, moonProbData = memmaps

    iterator_initializer_hook = IteratorInitializerHook()

    nsamples = len(mainData)
    samples_this_eval = BATCH * STEPS
    assert samples_this_eval <= nsamples

    evals = nsamples // samples_this_eval

    eval = get_input_fn._eval[name] % evals

    p = get_input_fn._eval['p']
    if p is None:
        get_input_fn._eval['p'] = p = np.random.permutation(nsamples)

    if name == TRAINING and eval == 0:
        get_input_fn._eval['p'] = p = np.random.permutation(nsamples)
        if samples_this_eval*4 < nsamples:
            # The usual case here. We can double the batch size and leave STEPS alone
            BATCH = BATCH*2
        elif STEPS > 3:
            # We shouldn't increase BATCH size without changing STEPS, or we'll stop processing up to half the data
            # This may increase STEPS at first, from say 64 to something less than 128, but it will eventually
            # reduce to 1.
            BATCH = BATCH*2
            STEPS = nsamples // BATCH
            if STEPS <= 1:
                STEPS = 1
                BATCH = nsamples
        if STEPS <= 16:
            BATCH = nsamples // STEPS

        print(f'********** Bumped batch to {BATCH}, steps to {STEPS}  **********')
        get_input_fn._eval[TRAINING] = 0
        get_input_fn._eval[VALIDATION] = 0

    def input_fn():

        start = get_input_fn._eval[name] * BATCH * STEPS
        get_input_fn._eval[name] += 1

        batchSize = BATCH if name == TRAINING else BATCH*STEPS

        print(f'*** {name} input_fn() here with batch {batchSize} and start {start} ***')

        with tf.variable_scope('input_fn'):
            main_data_placeholder = tf.placeholder(tf.float32, batchShape(MAIN_INPUT_SHAPE), name=MAIN_DATA)
            scores_data_placeholder = tf.placeholder(tf.float32, batchShape(SCORES_SHAPE), name=EXPECTED_SCORE)
            win_trick_data_placeholder = tf.placeholder(tf.float32, batchShape(WIN_TRICK_PROBS_SHAPE), name=WIN_TRICK_PROB)
            moon_data_placeholder = tf.placeholder(tf.float32, batchShape(MOONPROBS_SHAPE), name=MOON_PROB)

        p = get_input_fn._eval['p']
        assert p is not None
        dataset = tf.data.Dataset.from_tensor_slices((main_data_placeholder, (scores_data_placeholder, win_trick_data_placeholder, moon_data_placeholder)))
        iterator_initializer_hook.iterator_initializer_func = \
            lambda sess: sess.run(
                iterator.initializer,
                feed_dict={main_data_placeholder: mainData[p][start:],
                           scores_data_placeholder: scoresData[p][start:],
                           win_trick_data_placeholder: winTrickProbs[p][start:],
                           moon_data_placeholder: moonProbData[p][start:]
                           })
        dataset = dataset.repeat(1).batch(batchSize)
        iterator = dataset.make_initializable_iterator()
        (main, (scores, win_trick, moon)) = iterator.get_next()

        return {MAIN_DATA: main}, {EXPECTED_SCORE: scores, WIN_TRICK_PROB: win_trick, MOON_PROB: moon}

    return input_fn, iterator_initializer_hook

get_input_fn._eval = {
    TRAINING: 0,
    VALIDATION: 0,
    'p': None
}

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

    model_dir_path = f'{ROOT_MODEL_DIR}/d{hidden_depth}w{hidden_width}r{redundancy}_{activation}'
    os.makedirs(model_dir_path, exist_ok=True)

    config = tf.estimator.RunConfig(keep_checkpoint_max=20)
    estimator = tf.estimator.Estimator(model_fn=model_fn, params=params, model_dir=model_dir_path, config=config)

    feature_spec = {
        MAIN_DATA: tf.placeholder(dtype=np.float32, shape=batchShape(MAIN_INPUT_SHAPE), name=MAIN_DATA),
    }
    serving_input_receiver_fn = tf.estimator.export.build_raw_serving_input_receiver_fn(feature_spec)

    first_backstep_loss = None
    BACKSTEP_LIMIT = 3
    backsteps = 0
    best_eval_loss = float('inf')

    global BATCH
    BATCH = BATCH // 2   # this is a hack to make the algorithm to increase BATCH size simpler.

    for epoch in range(0, EPOCHS):
        train_input_fn, train_iterator_initializer_hook = get_input_fn(TRAINING, train_memmaps)
        eval_input_fn, eval_iterator_initializer_hook = get_input_fn(VALIDATION, eval_memmaps)

        estimator.train(train_input_fn, steps=STEPS, hooks=[train_iterator_initializer_hook])
        evaluation = estimator.evaluate(eval_input_fn, steps=1, hooks=[eval_iterator_initializer_hook])
        print(f'{epoch+1}/{EPOCHS}: Evaluation:', evaluation, file=sys.stderr)
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

    dataDir = 'xx.m' if len(sys.argv)==1 else sys.argv[1]

    train_dir = f'training/{dataDir}'
    eval_dir = f'validation/{dataDir}'

    train_memmaps = load_memmaps(train_dir)
    eval_memmaps = load_memmaps(eval_dir)

    if len(train_memmaps[0]) > len(eval_memmaps[0]):
        print('Swapping training and eval so that eval is the larger dataset')
        train_memmaps, eval_memmaps = eval_memmaps, train_memmaps
    assert len(train_memmaps[0]) <= len(eval_memmaps[0])

    evals = {}
    for hidden_width in [300]:
        for hidden_depth in [1]:
            for activation in ['swish5']:
                for redundancy in [2]:
                    params = {
                        'hidden_depth': hidden_depth,
                        'hidden_width': hidden_width,
                        'activation': activation,
                        'redundancy': redundancy,
                    }
                    results = train_with_params(train_memmaps, eval_memmaps, params)
                    evals[f'd{hidden_depth}w{hidden_width}r{redundancy}_{activation}'] = results

    for k, v in evals.items():
        print(v, k, file=sys.stderr)
