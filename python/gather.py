#!/usr/bin/env python

from datetime import datetime
from time import time
import glob
import os
import sys
import shutil
from stat import *

def ensureDir(path):
    if not os.path.isdir(path) :
        os.mkdir(path)
    assert os.path.isdir(path)

def gather(DATA, OUT, ARCHIVE, p):
    paths = glob.glob(f'{DATA}/*-{p}')
    with open(f'{OUT}/{p}', mode='a') as outfile:
        for path in paths:
            print(f'...{path}')
            with open(path) as f:
                content = f.read()
                outfile.write(content)
            shutil.move(path, ARCHIVE)

def gather_one_dataset(DATA, OUT, ARCHIVE):
    # The DATA directory must already exist, with data files output from the hearts program
    assert os.path.isdir(DATA)

    # These two directories will be created if necessary, which should only happen on first run
    ensureDir(OUT)
    ensureDir(ARCHIVE)

    for play in range(1,48):
        p = str(play)
        if len(p) == 1:
            p = '0' + p
        gather(DATA, OUT, ARCHIVE, p)

if __name__ == '__main__':
    # These three paths are all assumed to be relative to the root directory of the project
    DATA = 'training'
    OUT  = 'training'  # we'll output one file per play number to here, with all hands at that play number gathered into this file
    ARCHIVE = 'trainingarchive'  # we'll move processed data files to here so that we don't process them in future runs

    gather_one_dataset(DATA, OUT, ARCHIVE)

    DATA = 'validation'
    OUT  = 'validation'  # we'll output one file per play number to here, with all hands at that play number gathered into this file
    ARCHIVE = 'validationarchive'  # we'll move processed data files to here so that we don't process them in future runs

    gather_one_dataset(DATA, OUT, ARCHIVE)
