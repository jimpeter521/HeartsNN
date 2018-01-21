#!/usr/bin/env python3
# validate.py

import sys
import os
import numpy as np
import gzip

from model import createModel
from readfile import readPath
from constants import MAIN_INPUT_SHAPE, POINTS_SO_FAR_SHAPE, CARDS_IN_DECK, EPOCHS

def error_summary(expected, actual, legal):
    assert len(expected) == CARDS_IN_DECK
    assert len(actual) == CARDS_IN_DECK
    err_legal = 0.0
    err_illegal = 0.0
    for i in range(CARDS_IN_DECK):
        e = expected[i]
        a = actual[i]
        L = legal[i]
        d = np.abs(e-a)
        if L == 0.0:
            assert d == 0.0
        else:
            assert L == 1.0
            err_legal += d
            print(f'e:{e} a:{a} d:{d}')
    return err_legal, err_illegal

if __name__ == '__main__':
    np.set_printoptions(linewidth=200)
    np.set_printoptions(precision=4)

    model = createModel()

    assert len(sys.argv) >= 1
    weightsFilePath = sys.argv[1]
    assert os.path.isfile(weightsFilePath)
    print('Loading weights')
    model.load_weights(weightsFilePath)

    inFilePath = 'out/27.npy.gz' if len(sys.argv)<=2 else sys.argv[2]
    assert os.path.isfile(inFilePath)

    mainData, pointsData, outputData = readPath(inFilePath)
    print(mainData.shape, pointsData.shape, outputData.shape)

    nsamples = len(mainData)
    assert nsamples == len(pointsData)
    assert nsamples == len(outputData)

    outputs = {}
    # column zero from outputData is the best play probability vector P(best_outcome|card)
    outputs['best_play_softmax'] = outputData[:,0]
    # We currently ignore column 1 (expected score)
    # The remaining columns 2: are the 'bucket' proability distribution for each card
    # We treat them as 52 separate outputs, one per card.
    outputs.update({ f'b{c}': outputData[:,2:,c] for c in range(52)})

    loss = model.evaluate({'main_input': mainData, 'points_input': pointsData},
              outputs, verbose=1)
    print('\nLoss:', loss)

    NEXAMPLES = 10
    p = np.random.permutation(nsamples)[:NEXAMPLES]
    sampleMain = mainData[p]
    samplePoints = pointsData[p]
    sampleOutput = outputs['best_play_softmax'][p]
    assert len(sampleOutput[0]) == 52

    assert len(sampleMain) == NEXAMPLES
    assert len(samplePoints) == NEXAMPLES
    assert len(sampleOutput) == NEXAMPLES

    legalPlays = sampleMain[:,:,4,:]
    print('Shape legal:', legalPlays.shape)
    legalPlays = np.reshape(legalPlays, (-1,52))
    assert(len(legalPlays)) == NEXAMPLES
    assert(len(legalPlays[0])) == 52

    predictions = model.predict_on_batch({'main_input': sampleMain, 'points_input': samplePoints})
    assert len(predictions) == 53

    # for i, p in enumerate(predictions):
    #     print(i)
    #     if 'shape' in p:
    #         print(p.shape)
    #     print(p)
    #     print('--')

    predSoftmax = predictions[-1]
    print(len(predSoftmax))
    print(predSoftmax)
    assert len(predSoftmax) == 10

    for i in range(NEXAMPLES):
        print(f'sampleOutput[{i}]',  sampleOutput[i])
        print(f'predSoftmax[{i}]',  predSoftmax[i])

        err_legal, err_illegal = error_summary(sampleOutput[i], predSoftmax[i], legalPlays[i])
        print('err_legal, err_illegal:', err_legal, err_illegal)
