#!/usr/bin/env python

from datetime import datetime
from time import time, sleep
import glob
import os
import re
import sys
import shutil
from stat import *

# These three paths are all assumed to be relative to the root directory of the project
DATA = 'data'
TRAIN = 'training'  # we'll output one file per play number to here, with all hands at that play number gathered into this file
EVAL = 'validation'  # we'll output one file per play number to here, with all hands at that play number gathered into this file

# We will skip any data files that have been modified recently, so that we don't process partial files.
# It's necessary for the min age to be more than 60 seconds, even though at least one file in the set of 47
# files being actively written by one process will be updated every second or so. It may be worth checking
# the age of all files in a set and using the age of the more recently modified file as a proxy for the age
# of the group.
MIN_AGE = 30

# The DATA directory must already exist, with data files output from the hearts program
assert os.path.isdir(DATA)

def ensureDir(path):
    if not os.path.isdir(path) :
        os.mkdir(path)
    assert os.path.isdir(path)

def move_paths(paths, dest):
    for path in paths:
        mtime = os.path.getmtime(path)
        age = time() - mtime
        if age > MIN_AGE:
            print(f'...{path} {age}')
            shutil.move(path, dest)
        else:
            print(f'Skipping {path} with age {age}')

def get_ages(paths):
    ages = {}
    counts = {}
    for path in paths:
        filename = os.path.basename(path)
        m = re.match('([0-9a-f]*)-(\d{2})', filename)
        if not m:
            print(f'Path {path} not valid')
        else:
            grouphash = m.group(1)
            mtime = os.path.getmtime(path)
            age = time() - mtime
            if grouphash in ages:
                ages[grouphash] = min(ages[grouphash], age)
                counts[grouphash] = counts[grouphash] + 1
            else:
                ages[grouphash] = age
                counts[grouphash] = 1
    return ages, counts

def move_group(grouphash):
    shard = TRAIN if grouphash[-1] in '01234567' else EVAL
    for play in range(1, 48):
        p = str(play)
        if len(p) == 1:
            p = '0' + p
        path = f'{DATA}/{grouphash}-{p}'
        try:
            shutil.move(path, shard)
        except:
            print(f'Failed to move {path} as it already exists in ${shard}')

def run_once():
    paths = glob.glob(f'{DATA}/*-??')
    ages, counts = get_ages(paths)

    for grouphash, age in ages.items():
        count = counts[grouphash]
        if count == 47 and age > MIN_AGE:
            print(f'{grouphash} Moving the group')
            move_group(grouphash)
        elif age <= MIN_AGE:
            print(f'{grouphash} Group recently modified: {age}')
        elif count != 47:
            print(f'{grouphash} * Wrong number of files in group: {count} *')
        else:
            pass # this can't happen, but I like making the above condition clear

if __name__ == '__main__':
    # These two directories will be created if necessary, which should only happen on first run
    ensureDir(TRAIN)
    ensureDir(EVAL)

    while True:
        run_once()
        sleep(300)
