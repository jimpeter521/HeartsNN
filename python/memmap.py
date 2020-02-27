#!/usr/bin/env python

# Utilities for reading and writing numpy memmap files

import glob
import os
import re
import sys
import numpy as np

from constants import MAIN_INPUT_SHAPE, SCORES_SHAPE, WIN_TRICK_PROBS_SHAPE, MOONPROBS_SHAPE

def npBatchShape(shape):
    return (-1,) + shape

def save_to_memmap(data, path, p=None):
    """ Save a numpy array `data` as a memmap file to the given `path`. Optionally permute the data first."""
    if p is not None:
        assert len(p) <= len(data)
        data = data[p]
    print('Writing {} with shape {}'.format(path, data.shape))
    fp = np.memmap(path, dtype='float32', mode='w+', shape=data.shape)
    np.copyto(fp, data)
    del fp

def as_numpy(group):
    mainData = group['main']
    scoresData = group['score']
    winTrickProbs = group['trick']
    moonProbData = group['moon']

    nsamples = len(mainData)
    assert len(scoresData) == nsamples
    assert len(winTrickProbs) == nsamples
    assert len(moonProbData) == nsamples

    mainData = np.asarray(mainData, dtype=np.float32)
    scoresData = np.asarray(scoresData, dtype=np.float32)
    winTrickProbs = np.asarray(winTrickProbs, dtype=np.float32)
    moonProbData = np.asarray(moonProbData, dtype=np.float32)

    return mainData, scoresData, winTrickProbs, moonProbData

def save_group(group, datasetDir, lim=None):

    mainData, scoresData, winTrickProbs, moonProbData = as_numpy(group)

    os.makedirs(datasetDir, exist_ok=True)

    # Apply the same permutation to every array in the group.
    # We shuffle data here to ensure that the files
    # in the dataset always use the same order.
    nsamples = len(mainData)
    p = np.random.permutation(nsamples)

    if lim is not None and lim < nsamples:
        p = p[0:lim]

    save_to_memmap(mainData, f'{datasetDir}/main_data.np.mmap', p)
    save_to_memmap(scoresData, f'{datasetDir}/score_data.np.mmap', p)
    save_to_memmap(winTrickProbs, f'{datasetDir}/trick_data.np.mmap', p)
    save_to_memmap(moonProbData, f'{datasetDir}/moon_data.np.mmap', p)

def load_memmap(filePath, rowShape):
    data = np.memmap(filePath, mode='r', dtype=np.float32)
    data = np.reshape(data, npBatchShape(rowShape))
    return data

def load_dataset(dirPath):
    mainData = load_memmap(f'{dirPath}/main_data.np.mmap', MAIN_INPUT_SHAPE)
    scoresData = load_memmap(f'{dirPath}/score_data.np.mmap', SCORES_SHAPE)
    winTrickProbs = load_memmap(f'{dirPath}/trick_data.np.mmap', WIN_TRICK_PROBS_SHAPE)
    moonProbData = load_memmap(f'{dirPath}/moon_data.np.mmap', MOONPROBS_SHAPE)

    nsamples = len(mainData)
    assert len(scoresData) == nsamples
    assert len(winTrickProbs) == nsamples
    assert len(moonProbData) == nsamples

    return mainData, scoresData, winTrickProbs, moonProbData
