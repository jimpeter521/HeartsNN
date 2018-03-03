#!/usr/bin/env python3

# Read one or more numpy datasets (as created by hearts.cpp)
# and merge them into a larger dataset in the numpy memmap format.
# This pipeline currently can only produce an 'xx.m' dataset,
# i.e. one containing gamestates for all 47 possible play numbers.

import glob
import memmap
import numpy as np
import os
import re
import sys

def extract_hash(path):
    m = re.match(r'data/([\da-f]+)-main.npy', path)
    assert m is not None
    return m[1]

def merge_kind(group, purpose, kind, hashes):
    group[kind] = []
    for hash in hashes:
        a = np.load('data/{}-{}.npy'.format(hash, kind))
        group[kind].append(a)
    group[kind] = np.concatenate(group[kind])
    print(kind, group[kind].shape)

def merge_dataset(purpose, hashes):
    group = {}
    for kind in ['main', 'moon', 'score', 'trick']:
        merge_kind(group, purpose, kind, hashes)

    N = len(group['main'])
    assert N == len(group['moon'])
    assert N == len(group['score'])
    assert N == len(group['trick'])

    memmap.save_group(group, purpose + '/xx.m', 2*1024*1024)

if __name__ == '__main__':

    main_files = glob.glob('data/*-main.npy')
    hashes = [extract_hash(path) for path in main_files]

    num_datasets = len(hashes)
    N = num_datasets // 2

    training = hashes[:N]
    validation = hashes[N:]

    print('Training:', training)
    print('Validation:', validation)

    merge_dataset('training', training)
    merge_dataset('validation', validation)
