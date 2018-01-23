#!/usr/bin/env python

# Read one or more .np.gz data files (as created by transform.py) and created
# a matched set of three numpy memmap files holding all observations,
# with the order of observations permuatated.

import glob
import os
import re
import sys
import numpy as np

from readfile import readGlob

def save_to_memmap(data, path, p):
    print('Writing:', path)
    fp = np.memmap(path, dtype='float32', mode='w+', shape=data.shape)
    data = data[p]
    np.copyto(fp, data)
    return fp


def doOneSet(basedir, globExpr, dataset):
    datasetDir = f'{basedir}/{dataset}'
    os.makedirs(datasetDir, exist_ok=True)

    mainData, scoresData, winTrickProbs, moonProbData = readGlob(f'{basedir}/{globExpr}.np.gz')

    nsamples = len(mainData)
    assert len(scoresData) == nsamples
    assert len(winTrickProbs) == nsamples
    assert len(moonProbData) == nsamples

    print(f'Input files had a total of with {nsamples} observations');

    p = np.random.permutation(nsamples)
    save_to_memmap(mainData, f'{datasetDir}/main_data.np.mmap', p)
    save_to_memmap(scoresData, f'{datasetDir}/scores_data.np.mmap', p)
    save_to_memmap(winTrickProbs, f'{datasetDir}/win_trick_data.np.mmap', p)
    save_to_memmap(moonProbData, f'{datasetDir}/moon_data.np.mmap', p)

if __name__ == '__main__':
    globExpr = '??' if len(sys.argv)==1 else sys.argv[1]
    purpose = 'both' if len(sys.argv)==2 else sys.argv[2]

    assert len(globExpr) == 2
    dataset = 'd' + globExpr.replace('?', 'x')     # dataset names will be 27 -> d27, 2? -> d2x, ?? -> dxx

    if purpose == 'both':
        doOneSet('training', globExpr, dataset)
        doOneSet('validation', globExpr, dataset)
    else:
        doOneSet(purpose, globExpr, dataset)
