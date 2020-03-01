# HeartsNN Python Applications

This directory contains a couple python applications as well as some common library modules.

## merge_datasets.py

This is an appliction that merges the batches of data produced by the C++ `hearts` data generator
into files suitable for training.

Recall that `hearts` generates batches of observations into a directory named `data/`. The files are
are all numpy files that can be mapped into memory as numpy arrays. There are four numpy arrays per
batch, named `main`, `score`, `trick`, and `moon`.

All batches are identified using a 32-char hex string generated from a random 128-bit integer. A representative batch might be these four files:

    aeda7c2931bb7162cccaa2156acf31a5-main.npy
    aeda7c2931bb7162cccaa2156acf31a5-moon.npy
    aeda7c2931bb7162cccaa2156acf31a5-score.npy
    aeda7c2931bb7162cccaa2156acf31a5-trick.npy

Each batch is generated from 100 games. For one player, an observation is written whenever it is the player's turn and the player has more than one legal play. On average, this happens about 9.7 times per game, so there are approximately
970 observations per batch.

The types of data are:

1. *main*
 * A representation of the knowable state for the play.
 * Shape: (N, 52, 10)
2. *score*
 * The expected score vector -- the monte carlo estimated score for each card in the deck
 * Shape: (N, 52)
3. *trick*
 * The estimated probability that playing the card will lead to the player winning this trick
 * Shape: (N, 52)
4. *moon*
 * For each card, for each player, the estimated probability that the player can  
 * Shape: (N, 52, 3)
shoot the moon

## train.py

The tensorflow application that optimizes the model in `model.py`. The model is trained with the data in `training/...`
and validated with the data in `validation/...`. Since we are limited only by compute time for the amount of data available, we always use equal sized sets for training and validation. Training is stopped when the loss function for the validation set is determined to have flatlined.

## Library modules used by the two applications

1. constants.py	           -- Constants and also some values that should be CLI args
2. learning\_rate_hook.py  -- A tensorflow hook used in train.py (note: requires dlib python package)
3. memmap.py               -- Utilities for reading and writing numpy memmap files
4. model.py                -- this is the tensorflow model used by train.py
