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
        distribution = mainData[:, 0:INPUT_FEATURES*CARDS_IN_DECK]
        distribution = tf.reshape(distribution, [-1, INPUT_FEATURES, CARDS_IN_DECK])
        distribution = tf.transpose(distribution, perm=[0,2,1])
        distribution = tf.reshape(distribution, [-1, NUM_SUITS, NUM_RANKS, INPUT_FEATURES], name='distribution')
    return distribution

def hidden_layers(input, depth, width, activation):
    output = input
    for i in range(depth):
        with tf.variable_scope(f'hidden_{i}'):
            output = tf.layers.dense(output, width, activation=activation)
    return output

def one_conv_layer(input, ranks=2, stride=1, activation=swish, name='', isTraining=True):
    assert input.shape.dims[-1] == 1
    input_height = int(input.shape.dims[-3])
    input_features = int(input.shape.dims[-2])
    kernel_size = (ranks, input_features)
    input_units = input_height * input_features
    strides = (stride, 1)

    kernel_height = ranks
    assert (input_height-kernel_height) % stride == 0
    output_height = ((input_height-kernel_height) // stride) + 1

    SCALE = 0.9
    # With SCALE=1.0, we compute a number of output filters/features that will approximate the number of input features
    # We typically will want to gradually reduce the number of features in each layer, so use a SCALE a little less than 1.0
    filters = int(round(SCALE * float(input_height)*float(input_features)/float(output_height)))
    output_units = output_height * filters
    # print(f'Name: {name}, input_height: {input_height}, input_units: {input_units}, output_units: {output_units}, Kernel size: {kernel_size}')
    assert filters > 0
    conv = input
    with tf.variable_scope(name):
#         print(f'{name} input shape:', input.shape)
        conv = tf.layers.conv2d(conv, filters=filters, kernel_size=kernel_size, strides=strides, padding='valid', activation=activation)
        conv = tf.transpose(conv, [0,1,3,2])
#         print(f'{name} output shape:', conv.shape)

    return conv

def convolution_layers(mainData, activation, isTraining):
    with tf.variable_scope('convolution_layers'):
        distribution = extract_distribution(mainData)

        num_features = int(distribution.shape.dims[-1])
        assert num_features == INPUT_FEATURES
        conv = tf.reshape(distribution, [-1, NUM_RANKS, num_features, 1])

        # We could generalize here and allow stride to be a parameter, but after some experimentation it seems
        # likely that restricting stride to always be 1 is beneficial.
        # The set of "ranks" here (actually kernel_height) is probably near optimal so is unlikely to change
        stride = 1
        ranks = [2, 3, 4, 5, 3]

        for i, r in enumerate(ranks):
            conv = one_conv_layer(conv, ranks=r, stride=stride, name=f'L{i+1}_R{r}', activation=activation, isTraining=isTraining)

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
    elif name == 'leaky':
        return tf.nn.leaky_relu
    else:
        assert False


def model_fn(features, labels, mode, params={}):
    """Model function for Estimator."""

    isTraining = mode != tf.estimator.ModeKeys.PREDICT

    hidden_width = params['hidden_width']
    hidden_depth = params['hidden_depth']
    redundancy = params['redundancy']
    activation = activation_fn(params['activation'])

    mainData = features[MAIN_DATA]
    conv_layers = convolution_layers(mainData, activation, isTraining)

    with tf.variable_scope('legal_plays'):
        legalPlays = mainData[:, CARDS_IN_DECK*4:CARDS_IN_DECK*5]  # extract the 5th feature column

    # print('conv_layers shape:', conv_layers.shape)
    # print('mainData shape:', mainData.shape)

    extra_features = mainData[:, INPUT_FEATURES*CARDS_IN_DECK:]

    with tf.variable_scope('combined'):
        if redundancy == 0:
            combined = tf.concat([conv_layers, extra_features ], axis=-1, name='combined')
        else:
            first_suit = 4-redundancy
            redundant = extract_distribution(mainData)
            assert redundant.shape.as_list() == [None, NUM_SUITS, NUM_RANKS, INPUT_FEATURES]
            redundant = redundant[:, first_suit:, :, :]
            assert redundant.shape.as_list() == [None, redundancy, NUM_RANKS, INPUT_FEATURES]
            redundant = tf.layers.flatten(redundant, name='redundant')
            combined = tf.concat([conv_layers, redundant, extra_features], axis=-1, name='combined')

    combined = hidden_layers(combined, hidden_depth, hidden_width, activation)
    last_common_layer = tf.layers.dense(combined, hidden_width)

    outputs_dict = {}

    # At least one of the three heads must be enabled
    assert SCORE or TRICK or MOON

    if SCORE:
      with tf.variable_scope(EXPECTED_SCORE):
          expectedScoreLogits = tf.layers.dense(last_common_layer, CARDS_IN_DECK, name='logits')
          expectedScoreLogits = tf.multiply(expectedScoreLogits, legalPlays, name=EXPECTED_SCORE)
          tf.summary.histogram("expectedScoreLogits", expectedScoreLogits)
      outputs_dict[EXPECTED_SCORE] = expectedScoreLogits

    if TRICK:
      with tf.variable_scope(WIN_TRICK_PROB):
          winTrickLogits = tf.layers.dense(last_common_layer, CARDS_IN_DECK, name='logits', activation=tf.sigmoid)
          winTrickLogits = tf.multiply(winTrickLogits, legalPlays, name=WIN_TRICK_PROB)
          tf.summary.histogram("winTrickLogits", winTrickLogits)
      outputs_dict[WIN_TRICK_PROB] = winTrickLogits

    if MOON:
      with tf.variable_scope(MOON_PROB):
          moonProbLogits = tf.layers.dense(last_common_layer, MOON_CLASSES*CARDS_IN_DECK, name='logits')
          moonProbLogits = tf.reshape(moonProbLogits, (-1, CARDS_IN_DECK, MOON_CLASSES), name='reshaped')

          # Used only for export_outputs. Backprobagation uses softmax_cross_entropy_with_logits below
          moon_probs = tf.nn.softmax(moonProbLogits, name=MOON_PROB)
      outputs_dict[MOON_PROB] = moon_probs

    all_outputs = tf.estimator.export.PredictOutput(outputs_dict)
    default_out = all_outputs if not SCORE else tf.estimator.export.PredictOutput({EXPECTED_SCORE: expectedScoreLogits})
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

    if SCORE:
      with tf.variable_scope('expected_score_loss'):
          y_expected_score = labels[EXPECTED_SCORE]
          expected_score_loss = tf.losses.mean_squared_error(y_expected_score, expectedScoreLogits)
          expected_score_loss = tf.log(expected_score_loss)
          tf.summary.scalar("expected_score_loss", expected_score_loss)

    if TRICK:
      with tf.variable_scope('win_trick_prob_loss'):
          y_win_trick_prob = labels[WIN_TRICK_PROB]
          win_trick_prob_loss = tf.losses.mean_squared_error(y_win_trick_prob, winTrickLogits)
          win_trick_prob_loss = tf.log(win_trick_prob_loss)
          tf.summary.scalar("win_trick_prob_loss", win_trick_prob_loss)

    if MOON:
      with tf.variable_scope('moon_prob_loss'):
          y_moon_prob = tf.reshape(labels[MOON_PROB], (-1, CARDS_IN_DECK, MOON_CLASSES))
          moon_prob_losses = tf.nn.softmax_cross_entropy_with_logits_v2(labels=tf.stop_gradient(y_moon_prob), logits=moonProbLogits)
          moon_prob_losses = tf.multiply(moon_prob_losses, legalPlays, 'masked')
          moon_prob_loss = tf.reduce_mean(moon_prob_losses, name='moon_prob_mean_loss')
          moon_prob_loss = tf.log(moon_prob_loss)
          tf.summary.scalar("moon_prob_loss", moon_prob_loss)

    with tf.variable_scope('total_loss'):
        total_loss = 0.0
        if SCORE:
          total_loss = tf.add(total_loss, expected_score_loss)
        if TRICK:
          total_loss = tf.add(total_loss, win_trick_prob_loss)
        if MOON:
          total_loss = tf.add(total_loss, moon_prob_loss)

    optimizer = tf.train.AdamOptimizer()
    train_op = optimizer.minimize(loss=total_loss, global_step=tf.train.get_global_step())

    eval_metric_ops = {}

    if SCORE:
      eval_metric_ops['expected_score_loss'] = tf.metrics.mean_squared_error(y_expected_score, expectedScoreLogits)

    if TRICK:
      eval_metric_ops['win_trick_prob_loss'] = tf.metrics.mean_squared_error(y_win_trick_prob, winTrickLogits)

    if MOON:
      eval_metric_ops['moon_prob_loss'] = tf.metrics.mean(moon_prob_loss)

    return tf.estimator.EstimatorSpec(mode=mode, loss=total_loss, train_op=train_op, export_outputs=export_outputs,
                                        eval_metric_ops=eval_metric_ops)
