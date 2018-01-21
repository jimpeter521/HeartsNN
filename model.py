import numpy as np

import tensorflow as tf
from tensorflow.python.estimator.canned.head import _Head

from constants import (MAIN_INPUT_SHAPE, POINTS_SO_FAR_SHAPE, CARDS_IN_DECK, NUM_SUITS
                    , INPUT_FEATURES, NUM_RANKS, MAIN_DATA, EXPECTED_SCORE, WIN_TRICK_PROB, MOON_PROB, MOON_CLASSES)

# We default beta to 5.0 because experimentation shows it to be near optimal for our model.
# This was of course done with the current model, but the model will evolve, and the best beta might change.
def swish(x, beta=5.0):
    return x * tf.sigmoid(beta * x)

def extract_distribution(mainData):
    """
    Extract from mainData the first 6x52 values, and return them reshaped as (4,13,6),
    which is 4 suits by 13 ranks by 6 features. We will apply a conv2d to each of the four planes of 13x6 arrays.
    """
    with tf.variable_scope('extract_distribution'):
        distribution = mainData[:, 0:INPUT_FEATURES*CARDS_IN_DECK]
        distribution = tf.reshape(distribution, [-1, INPUT_FEATURES, CARDS_IN_DECK])
        distribution = tf.transpose(distribution, perm=[0,2,1])
        distribution = tf.reshape(distribution, [-1, NUM_SUITS, NUM_RANKS, INPUT_FEATURES], name='distribution')
    return distribution

def hidden_layers(input, depth, width, activation):
    for i in range(depth):
        with tf.variable_scope(f'hidden_{i}'):
            output = tf.layers.dense(input, width, activation=activation)
            input = output
    return output

def one_conv_layer(input, ranks=2, stride=1, activation=swish, name=''):
    assert input.shape.dims[-1] == 1
    input_features = int(input.shape.dims[-2])
    kernel_size = (ranks, input_features)
    strides = (stride, 1)
    filters = (ranks*input_features * 4) // 5           # 80% reduction in features
    assert filters > 0
    with tf.variable_scope(name):
        # print(f'{name} input shape:', input.shape)
        conv = tf.layers.conv2d(input, filters=filters, kernel_size=kernel_size, strides=strides, padding='valid', activation=activation)
        conv = tf.transpose(conv, [0,1,3,2])
        # print(f'{name} output shape:', conv.shape)

    return conv

def convolution_layers(mainData, activation):
    with tf.variable_scope('convolution_layers'):
        distribution = extract_distribution(mainData)

        num_features = int(distribution.shape.dims[-1])
        assert num_features == INPUT_FEATURES
        conv = tf.reshape(distribution, [-1, NUM_RANKS, num_features, 1])

        conv = one_conv_layer(conv, ranks=2, stride=1, name='L1_R2_S1', activation=activation)
        conv = one_conv_layer(conv, ranks=4, stride=4, name='L2_R4_S4', activation=activation)
        conv = one_conv_layer(conv, ranks=3, stride=1, name='L3_R3_S1', activation=activation)

        assert conv.shape.dims[-1] == 1
        num_features = int(conv.shape.dims[-2])
        num_vranks = int(conv.shape.dims[-3])
        conv = tf.reshape(conv, [-1, NUM_SUITS, num_vranks, num_features])

        # print('Before flattening shape:', conv.shape)
        conv = tf.layers.flatten(conv, 'conv_output')
        # print('Flattened convolution output shape:', conv.shape)

    return conv

def activation_fn(name):
    if name == 'swish5':
        return lambda x: swish(x, beta=5.0)
    elif name == 'swish1':
        return lambda x: swish(x, beta=1.0)
    elif name == 'relu':
        return tf.nn.relu
    elif name == 'leaky':
        return tf.nn.leaky_relu
    else:
        assert False


def model_fn(features, labels, mode, params={}):
    """Model function for Estimator."""

    hidden_width = params['hidden_width']
    hidden_depth = params['hidden_depth']
    activation = activation_fn(params['activation'])

    mainData = features[MAIN_DATA]
    conv_layers = convolution_layers(mainData, activation)

    with tf.variable_scope('legal_plays'):
        # Here we want a pure legal plays vector.
        # The original vector will often (3/4 of the time, whenever current player not leading)
        # have one -1 value that must be set to zero. We will retain the -1 where this vector
        # is used elsewhere in the model.
        legalPlays = tf.clip_by_value(mainData[:, 0:CARDS_IN_DECK], 0.0, 1.0)

    # print('conv_layers shape:', conv_layers.shape)
    # print('mainData shape:', mainData.shape)
    combined = tf.concat([mainData, conv_layers], axis=-1, name='combined')

    input = tf.layers.dense(combined, hidden_width)
    last_common_layer = hidden_layers(input, hidden_depth, hidden_width, activation)

    with tf.variable_scope(EXPECTED_SCORE):
        expectedScoreLogits = tf.layers.dense(last_common_layer, CARDS_IN_DECK, name='logits', activation=tf.tanh)
        expectedScoreLogits = tf.multiply(expectedScoreLogits, legalPlays, name=EXPECTED_SCORE)
        tf.summary.histogram("expectedScoreLogits", expectedScoreLogits)

    with tf.variable_scope(WIN_TRICK_PROB):
        winTrickLogits = tf.layers.dense(last_common_layer, CARDS_IN_DECK, name='logits', activation=tf.sigmoid)
        winTrickLogits = tf.multiply(winTrickLogits, legalPlays, name=EXPECTED_SCORE)
        tf.summary.histogram("winTrickLogits", winTrickLogits)

    with tf.variable_scope(MOON_PROB):
        moonProbLogits = tf.layers.dense(last_common_layer, MOON_CLASSES*CARDS_IN_DECK, name='logits')
        moonProbLogits = tf.reshape(moonProbLogits, (-1, CARDS_IN_DECK, MOON_CLASSES), name='reshaped')

        # Used only for export_outputs. Backprobagation uses softmax_cross_entropy_with_logits below
        moon_probs = tf.nn.softmax(moonProbLogits, name=MOON_PROB)

    outputs_dict = {
        EXPECTED_SCORE: expectedScoreLogits,
        WIN_TRICK_PROB: winTrickLogits,
        MOON_PROB: moon_probs,
    }

    all_outputs = tf.estimator.export.PredictOutput(outputs_dict)
    score_only = tf.estimator.export.PredictOutput({EXPECTED_SCORE: expectedScoreLogits})

    export_outputs = {
        tf.saved_model.signature_constants.DEFAULT_SERVING_SIGNATURE_DEF_KEY: score_only,
        'all_outputs': all_outputs,
    }

    if mode == tf.estimator.ModeKeys.PREDICT:
        return tf.estimator.EstimatorSpec(
            mode=mode,
            predictions=outputs_dict,
            export_outputs=export_outputs)

    assert labels is not None

    with tf.variable_scope('expected_score_loss'):
        y_expected_score = labels[EXPECTED_SCORE]
        expected_score_loss = tf.losses.mean_squared_error(y_expected_score, expectedScoreLogits)

    with tf.variable_scope('win_trick_prob_loss'):
        y_win_trick_prob = labels[WIN_TRICK_PROB]
        win_trick_prob_loss = tf.losses.mean_squared_error(y_win_trick_prob, winTrickLogits)

    with tf.variable_scope('moon_prob_loss'):
        y_moon_prob = tf.reshape(labels[MOON_PROB], (-1, CARDS_IN_DECK, MOON_CLASSES))
        moon_prob_losses = tf.nn.softmax_cross_entropy_with_logits_v2(labels=tf.stop_gradient(y_moon_prob), logits=moonProbLogits)
        moon_prob_losses = tf.multiply(moon_prob_losses, legalPlays, 'masked')
        moon_prob_loss = tf.reduce_mean(moon_prob_losses, name='moon_prob_mean_loss')

    with tf.variable_scope('total_loss'):
        loss_tmp = tf.multiply(expected_score_loss, win_trick_prob_loss)
        total_loss = tf.multiply(loss_tmp, moon_prob_loss, 'loss_product')

    optimizer = tf.train.AdamOptimizer()
    train_op = optimizer.minimize(loss=total_loss, global_step=tf.train.get_global_step())

    eval_metric_ops = {
        'expected_score_loss': tf.metrics.mean_squared_error(y_expected_score, expectedScoreLogits),
        'win_trick_prob_loss': tf.metrics.mean_squared_error(y_win_trick_prob, winTrickLogits),
        'moon_prob_loss': tf.metrics.mean(moon_prob_loss),
    }

    return tf.estimator.EstimatorSpec(mode=mode, loss=total_loss, train_op=train_op, export_outputs=export_outputs,
                                        eval_metric_ops=eval_metric_ops)
