#!/usr/bin/env python

# Read one or more memmap datasets (as created by transform.py)
# and merge them into a larger dataset.

import glob
import os
import re
import sys
import numpy as np
import memmap

def doOneSet(basedir, globExpr, outDatasetName):
    outDatasetDir = f'{basedir}/{outDatasetName}'
    os.makedirs(outDatasetDir, exist_ok=True)

    group = {
        'main': [],
        'scores': [],
        'winTrick': [],
        'moonProb': [],
    }

    inDatasetGlob = f'{basedir}/{globExpr}.d'
    inputs = glob.glob(inDatasetGlob)
    assert len(inputs) > 1

    for inDatasetPath in inputs:
        mainData, scoresData, winTrickProbs, moonProbData = memmap.load_dataset(inDatasetPath)
        group['main'].append(mainData)
        group['scores'].append(scoresData)
        group['winTrick'].append(winTrickProbs)
        group['moonProb'].append(moonProbData)

    group['main'] = np.concatenate(group['main'], axis=0)
    group['scores'] = np.concatenate(group['scores'], axis=0)
    group['winTrick'] = np.concatenate(group['winTrick'], axis=0)
    group['moonProb'] = np.concatenate(group['moonProb'], axis=0)

    N = len(group['main'])
    print(f'Merging {N} total samples')

    memmap.save_group(group, outDatasetDir)


if __name__ == '__main__':
    globExpr = '??' if len(sys.argv)==1 else sys.argv[1]
    purpose = 'both' if len(sys.argv)==2 else sys.argv[2]

    assert len(globExpr) == 2
    outDatasetName = globExpr.replace('?', 'x') + '.m'     # dataset names will be 27 -> 27.d, 2? -> 2x.d, ?? -> xx.d
    print('outDatasetName:', outDatasetName)

    if purpose == 'both':
        doOneSet('training', globExpr, outDatasetName)
        doOneSet('validation', globExpr, outDatasetName)
    else:
        doOneSet(purpose, globExpr, outDatasetName)
