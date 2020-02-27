import numpy as np

import tensorflow as tf
from tensorflow.python.estimator.canned.head import _Head
from constants import *

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
        distribution = tf.reshape(mainData, [-1, NUM_SUITS, NUM_RANKS, INPUT_FEATURES], name='distribution')
    return distribution

def dense_norm(x, n, isTraining=False):
    with tf.variable_scope('dense_norm'):
      x = tf.layers.dense(x, n, activation=tf.nn.relu)
      x = tf.keras.layers.BatchNormalization()(x, training=isTraining)
    return x

def residual_layer(x0, width, isTraining=False):
    assert x0.shape.dims[-1] == width
    with tf.variable_scope('d0'):
      x = dense_norm(x0, width, isTraining)
    with tf.variable_scope('d1'):
      x = dense_norm(x, width, isTraining)
    return tf.add(x, x0)

def residual_layers(x, width, depth, isTraining):
    with tf.variable_scope('neck'):
      for i in range(depth):
        with tf.variable_scope('neck_' + str(i)):
          x = residual_layer(x, width, isTraining)
    return x

def one_conv_layer(input, ranks=2, name='', isTraining=True):
    assert input.shape.dims[-1] == 1
    input_height = int(input.shape.dims[-3])
    input_features = int(input.shape.dims[-2])
    kernel_size = (ranks, input_features)
    input_units = input_height * input_features

    stride = 1
    strides = (1, 1)

    kernel_height = ranks
    assert (input_height-kernel_height) % stride == 0
    output_height = ((input_height-kernel_height) // stride) + 1

    SCALE = .95
    # With SCALE=1.0, we compute a number of output filters/features that will approximate the number of input features
    # We typically will want to gradually reduce the number of features in each layer, so use a SCALE a little less than 1.0
    filters = int(round(SCALE * float(input_height)*float(input_features)/float(output_height)))
    output_units = output_height * filters
    # print(f'Name: {name}, input_height: {input_height}, input_units: {input_units}, output_units: {output_units}, Kernel size: {kernel_size}')
    assert filters > 0
    conv = input
    with tf.variable_scope(name):
#         print(f'{name} input shape:', input.shape)
        conv = tf.layers.conv2d(conv, filters=filters, kernel_size=kernel_size, strides=strides, padding='valid', activation=tf.nn.relu)
        conv = tf.keras.layers.BatchNormalization()(conv, training=isTraining)
        conv = tf.transpose(conv, [0,1,3,2])
#         print(f'{name} output shape:', conv.shape)

    return conv

def convolution_layers(mainData, activation, isTraining):
    with tf.variable_scope('convolution_layers'):
        distribution = extract_distribution(mainData)

        num_features = int(distribution.shape.dims[-1])
        assert num_features == INPUT_FEATURES
        conv = tf.reshape(distribution, [-1, NUM_RANKS, num_features, 1])

        ranks = [2, 3, 4, 5, 3]

        for i, r in enumerate(ranks):
            conv = one_conv_layer(conv, ranks=r, name='L{}_R{}'.format(i+1, r), isTraining=isTraining)

        assert conv.shape.dims[-1] == 1
        num_features = int(conv.shape.dims[-2])
        num_vranks = int(conv.shape.dims[-3])
        conv = tf.reshape(conv, [-1, NUM_SUITS, num_vranks, num_features])

#         print('Before flattening shape:', conv.shape)
        conv = tf.layers.flatten(conv, 'conv_output')
#         print('Flattened convolution output shape:', conv.shape)

    return conv

def activation_fn(name):
    if name == 'swish5':
        return lambda x: swish(x, beta=5.0)
    elif name == 'swish1':
        return lambda x: swish(x, beta=1.0)
    elif name == 'relu':
        return tf.nn.relu
    elif name == 'elu':
        return tf.nn.elu
    elif name == 'leaky':
        return tf.nn.leaky_relu
    else:
        assert False

def head_layers(last_common_layer, num_layers, width=CARDS_IN_DECK, isTraining=False):
    layer = dense_norm(last_common_layer, width, isTraining)
    for i in range(num_layers):
      with tf.variable_scope('head_layer_' + str(i)):
        layer = residual_layer(layer, width, isTraining)
    return layer

def model_fn(features, labels, mode, params={}):
    """Model function for Estimator."""

    isTraining = mode != tf.estimator.ModeKeys.PREDICT

    hidden_width = params['hidden_width']
    hidden_depth = params['hidden_depth']
    num_batches = params['num_batches']
    activation = activation_fn(params['activation'])

    mainData = features[MAIN_DATA]

    with tf.variable_scope('legal_plays'):
        legalPlays = mainData[:, :, 0]  # extract the first feature column

    last_common_layer = convolution_layers(mainData, activation, isTraining)

    outputs_dict = {}

    # At least one of the three heads must be enabled
    assert SCORE or TRICK or MOON

    # We have 52 total cards, but only a small subset of them are legal plays.
    # Any metric that computes a mean by dividing by 52 is a distorted metric.
    # We should instead compute our own sum and divide by the number of legal plays
    num_legal = tf.reduce_sum(legalPlays, name='num_legal')

    num_head_layers = 2
    if SCORE:
      with tf.variable_scope(EXPECTED_SCORE):
          expectedScoreLogits = head_layers(last_common_layer, num_head_layers, width=CARDS_IN_DECK, isTraining=isTraining)
          expectedScoreLogits = tf.nn.relu(expectedScoreLogits, name=EXPECTED_SCORE)
      outputs_dict[EXPECTED_SCORE] = expectedScoreLogits

    if TRICK:
      with tf.variable_scope(WIN_TRICK_PROB):
          winTrickLogits = head_layers(last_common_layer, num_head_layers, width=CARDS_IN_DECK, isTraining=isTraining)
          winTrickLogits = tf.sigmoid(winTrickLogits, name=WIN_TRICK_PROB)
          tf.summary.histogram("winTrickLogits", winTrickLogits)
      outputs_dict[WIN_TRICK_PROB] = winTrickLogits

    if MOON:
      with tf.variable_scope(MOON_PROB):
          moonProbLogits = head_layers(last_common_layer, num_head_layers, width=MOON_CLASSES*CARDS_IN_DECK, isTraining=isTraining)
          moonProbLogits = tf.reshape(moonProbLogits, (-1, CARDS_IN_DECK, MOON_CLASSES), name='reshaped')
          # Used only for export_outputs. Backprobagation uses softmax_cross_entropy_with_logits below
          moon_probs = tf.nn.softmax(moonProbLogits, name=MOON_PROB)
      outputs_dict[MOON_PROB] = moon_probs

    all_outputs = tf.estimator.export.PredictOutput(outputs_dict)
    default_out = all_outputs
    export_outputs = {
      tf.saved_model.signature_constants.DEFAULT_SERVING_SIGNATURE_DEF_KEY: default_out,
      'all_outputs': all_outputs,
    }

    if mode == tf.estimator.ModeKeys.PREDICT:
        return tf.estimator.EstimatorSpec(
            mode=mode,
            predictions=outputs_dict,
            export_outputs=export_outputs)

    assert labels is not None

    scalars = {}

    if SCORE:
      with tf.variable_scope('expected_score_loss'):
          y_expected_score = labels[EXPECTED_SCORE]
          expected_score_diff = tf.subtract(y_expected_score, expectedScoreLogits)
          expected_score_diff = tf.square(expected_score_diff)
          expected_score_diff = tf.multiply(expected_score_diff, legalPlays, 'masked')
          tf.summary.histogram("expectedScoreL2s", expected_score_diff)
          expected_score_squared_sum = tf.reduce_sum(expected_score_diff)
          expected_score_loss = tf.divide(expected_score_squared_sum, num_legal)
          expected_score_loss = tf.log(expected_score_loss)
          # Finally divide the log by 2 so we are using log of RMSE.
          expected_score_loss = tf.divide(expected_score_loss, 2.0, name='expected_score_loss')
          tf.summary.scalar('expected_score_loss', expected_score_loss)
          scalars[EXPECTED_SCORE] = expected_score_loss

    if TRICK:
      with tf.variable_scope('win_trick_prob_loss'):
          y_win_trick_prob = labels[WIN_TRICK_PROB]
          win_trick_prob_diff = tf.subtract(y_win_trick_prob, winTrickLogits)
          win_trick_prob_diff = tf.square(win_trick_prob_diff)
          win_trick_prob_diff = tf.multiply(win_trick_prob_diff, legalPlays, 'masked')
          tf.summary.histogram("winTrickL2s", win_trick_prob_diff)
          win_trick_prob_squared_sum = tf.reduce_sum(win_trick_prob_diff)
          win_trick_prob_loss = tf.divide(win_trick_prob_squared_sum, num_legal)
          win_trick_prob_loss = tf.log(win_trick_prob_loss)
          # Finally divide the log by 2 so we are using log of RMSE.
          win_trick_prob_loss = tf.divide(win_trick_prob_loss, 2.0, name='win_trick_prob_loss')
          tf.summary.scalar('win_trick_prob_loss', win_trick_prob_loss)
          scalars[WIN_TRICK_PROB] = win_trick_prob_loss

    if MOON:
      with tf.variable_scope('moon_prob_loss'):
          y_moon_prob = tf.reshape(labels[MOON_PROB], (-1, CARDS_IN_DECK, MOON_CLASSES))
          kldiverg = tf.keras.losses.kullback_leibler_divergence(y_moon_prob, moon_probs)
          moon_prob_losses = tf.multiply(kldiverg, legalPlays, 'masked')
          tf.summary.histogram("moonProbDivergences", moon_prob_losses)
          moon_prob_loss = tf.reduce_sum(moon_prob_losses, name='moon_prob_loss_sum')
          moon_prob_loss = tf.divide(moon_prob_loss, num_legal, name='moon_prob_loss_mean')
          moon_prob_loss = tf.log(moon_prob_loss, name='moon_prob_loss')
          tf.summary.scalar('moon_prob_loss', moon_prob_loss)
          scalars[MOON_PROB] = moon_prob_loss

    with tf.variable_scope('total_loss'):
        total_loss = 0.0
        if SCORE:
          total_loss = tf.add(total_loss, expected_score_loss)
        if TRICK:
          total_loss = tf.add(total_loss, win_trick_prob_loss)
        if MOON:
          total_loss = tf.add(total_loss, moon_prob_loss)
        total_loss = tf.add(total_loss, 0.0, name='total_loss')
        tf.summary.scalar('total_loss', total_loss)

    # scalars['total_loss'] = total_loss

    optimizer = tf.train.AdamOptimizer()

    extra_ops = tf.get_collection(tf.GraphKeys.UPDATE_OPS)
    with tf.control_dependencies(extra_ops):
      train_op = optimizer.minimize(loss=total_loss, global_step=tf.train.get_global_step())

    model_dir_path = params['model_dir_path']
    assert model_dir_path is not None

    logging_hook = tf.train.LoggingTensorHook(scalars, every_n_iter=num_batches)

    summary_hook = tf.train.SummarySaverHook(output_dir= model_dir_path + '/eval', save_steps=num_batches,
        scaffold=tf.train.Scaffold(summary_op=tf.summary.merge_all()))

    return tf.estimator.EstimatorSpec(mode=mode, loss=total_loss, train_op=train_op, export_outputs=export_outputs,
                                      evaluation_hooks=[summary_hook, logging_hook])
