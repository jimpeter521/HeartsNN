#!/usr/bin/env python3

import glob
import gzip
import os
import re
import sys
import numpy as np
import tensorflow as tf

import memmap

from constants import *

def readLine(f):
    return next(f).strip()

rankNames = ('2', '3', '4', '5', '6', '7', '8', '9', '10', 'J', 'Q', 'K', 'A')
suitNames = ('♣️', '♦️', '♠️', '♥️')

asSuit = {'?' : '?', '.' : '?'}
for i, s in enumerate(suitNames):
    asSuit[s] = i

asRank = {}
for i, s in enumerate(rankNames):
    asRank[s] = i

cardRegexp = re.compile('([0123456789JQKA]+)\s*((♣️|♦️|♠️|♥️))')

def cardAsSuitAndRank(card):
    suit = card // NUM_RANKS
    rank = card % NUM_RANKS
    return (suit, rank)

def suitAndRankToCard(suit, rank):
    return suit*NUM_RANKS + rank

def parseCard(s):
    if s == '.':
        return -1
    m = cardRegexp.match(s.strip())
    assert m
    [_rank, _suit] = m.group(1, 2)
    assert _rank in asRank
    assert _suit in asSuit
    rank = asRank[_rank]
    suit = asSuit[_suit]
    return suitAndRankToCard(suit, rank)

h1regexp = re.compile('Play (\d+), Player Leading (\d), Current Player (\d), Choices (\d+) TrickSuit (\S+)')

def parseHeader1(f):
    header1 = readLine(f)   # Play 43, Player Leading 0, Current Player 3, Choices 2 TrickSuit ♠️
    m1 = h1regexp.match(header1)
    [play, lead, current, choices, trickSuit] = m1.group(1, 2, 3, 4, 5)
    return {
        'play': int(play),
        'lead': int(lead),
        'current': int(current),
        'choices': int(choices),
        'trickSuit': asSuit[trickSuit]
    }

h2regexp = re.compile('TrickSoFar:\s*(\S+)\s+(\S+)\s+(\S+)\s+(\S+)')

def parseTrickSoFar(f):
    trickSoFar = readLine(f)   # TrickSoFar: 3♠️   J♥️   9♠️  .
    m2 = h2regexp.match(trickSoFar)
    assert m2
    return [parseCard(s) for s in m2.group(1, 2, 3, 4)]

h3regexp = re.compile('PointsSoFar:\s*(\d+)\s+(\d+)\s+(\d+)\s+(\d+)')

def parsePointsSoFar(f):
    pointsSoFar = readLine(f)   # PointsSoFar:3 0 4 17
    m3 = h3regexp.match(pointsSoFar)
    assert m3
    return [int(s) for s in m3.group(1, 2, 3, 4)]

def parseUnplayed(f):
    line = readLine(f)
    parts = line.split()
    card = parseCard(parts[0])
    probs = [float(f) for f in parts[1:]]
    return (card, probs)

def parseDistribution(f, remaining):
    distribution = []
    for _ in range(remaining):
        distribution.append(parseUnplayed(f))
    return distribution

# This method parses the 'labels' or 'y values'
# We have three different types of labels for each card
# 1. The expected score for the card, which is always 0.0 for illegal plays, and in the range [-19.5, 18.5] for legal plays
# 2. The (scalar) probability that the card will win the current trick.
# 3. A probability vector of length MOON_CLASSES, with terms corresponding to p(playerShootsMoon), p(otherShootsMoon), p(regularScore)
# The vector will be (0.0, 0.0, 1.0) for all illegal plays.
# It will also be (0.0, 0.0, 1.0) for all legal plays when points are split.
# The first two values will be non-zero when there is a chance of one player shooting the moon.
# If the current player can force shooting the moon, the vector should be (1.0, 0.0, 0.0)
def parseExpectedOutput(f):
    line = readLine(f)
    parts = line.split()
    assert len(parts) == 7
    card = parseCard(parts[0])
    expected = float(parts[1])
    winTrickProb = float(parts[2])
    assert parts[3] == '|'
    moonProb = [float(f) for f in parts[4:]]
    assert len(moonProb) == MOON_CLASSES
    return (card, expected, winTrickProb, moonProb)

def parseExpectedOutputs(f, choices):
    expectedOutputs = []
    for _ in range(choices):
        expectedOutputs.append(parseExpectedOutput(f))
    return expectedOutputs

dealIndexRegex = re.compile('^[\da-f]+$')

def parseDealIndex(f):
    line = readLine(f)
    assert dealIndexRegex.match(line)

def parseSeparator(f):
    line = readLine(f)
    assert line == '--'

def parseExtraFeatures(f):
    result = []
    for _ in range(7):
        line = readLine(f)
        a = [float(x) for x in line.split()]
        result.extend(a)
    assert len(result) == 33
    return result

def parseOneState(f):
    dealIndex = parseDealIndex(f)
    header = parseHeader1(f)
    trickSoFar = parseTrickSoFar(f)
    pointsSoFar = parsePointsSoFar(f)
    distribution = parseDistribution(f, CARDS_IN_DECK - header['play'])
    parseSeparator(f)
    expectedOutputs = parseExpectedOutputs(f, header['choices'])
    parseSeparator(f)
    extra_features = parseExtraFeatures(f)

    last = readLine(f)
    assert last == '----'

    rest = {
        'trickSoFar': trickSoFar,
        'pointsSoFar': pointsSoFar,
        'distribution': distribution,
        'expectedOutputs': expectedOutputs,
        'extra_features': extra_features,
    }

    return {**header, **rest}

def printState(state):
    print(f'Play:{state["play"]}, Lead:{state["lead"]}, Current:{state["current"]}, Choices:{state["choices"]}, TrickSuit:{state["trickSuit"]}')
    print(f'TrickSoFar:{state["trickSoFar"]}')
    print(f'PointsSoFar:{state["pointsSoFar"]}')
    for l in state['distribution']:
        print(l)
    for l in state['expectedOutputs']:
        print(l)

def expandDistribution(distribution):
    zeros = [0.0 for _ in range(NUM_PLAYERS)]
    expanded = [zeros for _ in range(CARDS_IN_DECK)]
    for row in distribution:
        card, probs = row
        expanded[card] = probs
    distribution = np.array(expanded, dtype=np.float32).T
    assert distribution.shape == (NUM_PLAYERS,CARDS_IN_DECK)
    return distribution

def expandExpectedOutputs(state):
    scores = [0.0 for _ in range(CARDS_IN_DECK)]
    winTrickProbs = [0.0 for _ in range(CARDS_IN_DECK)]
    illegalPlay = [0.0, 0.0, 1.0]
    moonProbs = [illegalPlay for _ in range(CARDS_IN_DECK)]
    for row in state['expectedOutputs']:
        card, expected, winTrickProb, moonProb = row
        # Scale expected scores from range [-19.5, 18.5] to [-1.0, 0.94]
        scores[card] = float(expected) / 19.5
        winTrickProbs[card] = winTrickProb
        moonProbs[card] = [moonProb[0], moonProb[1], moonProb[2]]
    scores = np.array(scores, np.float32)
    assert scores.shape == (CARDS_IN_DECK,)
    winTrickProbs = np.array(winTrickProbs, np.float32)
    assert winTrickProbs.shape == (CARDS_IN_DECK,)
    moonProbs = np.array(moonProbs, np.float32)
    assert moonProbs.shape == (CARDS_IN_DECK, MOON_CLASSES)
    moonProbs = moonProbs.flatten()
    assert moonProbs.shape == MOONPROBS_SHAPE
    return scores, winTrickProbs, moonProbs

def highCardInTrick(trickSoFar):
    trickSuit = None
    for card in trickSoFar:
        if card == -1:
            break
        suit, rank = cardAsSuitAndRank(card)
        if trickSuit is None:
            trickSuit = suit
            highRank = rank
        elif suit == trickSuit and highRank < rank:
            highRank = rank
    return None if trickSuit is None else suitAndRankToCard(trickSuit, highRank)


def makeLegalPlays(expectedOutputs):
    """ Create a one-hot vector for the cards that the current player can legally play. """
    expanded = [0.0 for _ in range(CARDS_IN_DECK)]
    for row in expectedOutputs:
        card, _, _, _ = row
        expanded[card] = 1.0
    legalPlays = np.array(expanded, np.float32)
    assert legalPlays.shape == (CARDS_IN_DECK,)
    return legalPlays

def makeCanCardTakeTrick(trickSoFar, expectedOutputs):
    """ Create a one-hot vector for the legal plays that might possibly take trick.
    Zeros if it is guaranteed card cannot take trick.
    We use 0.5 as a hack when we are leading the trick for every legal play. TODO: fix this in c++
    """
    expanded = [0.0 for _ in range(CARDS_IN_DECK)]
    card = highCardInTrick(trickSoFar)
    if card is None:
        trickSuit = None
        trickHigh = None
    else:
        trickSuit, trickHigh = cardAsSuitAndRank(card)
    for row in expectedOutputs:
        card, _, _, _ = row
        if trickSuit is None:
            # This is suboptimal.
            # TODO: Compute this feature in C++ where we can more easily create correct values
            expanded[card] = 0.5
        else:
            suit, rank = cardAsSuitAndRank(card)
            expanded[card] = oneIfTrue(suit == trickSuit and rank > trickHigh)
    expanded = np.array(expanded, np.float32)
    assert expanded.shape == (CARDS_IN_DECK,)
    return expanded

def cardValue(i):
    if i >= 39:  # all hearts
        return 1.0 / 26.0
    elif i == 36: # the queen of spades
        return 13.0 / 26.0
    else:
        return 0.0

def pointValue():
    v = [cardValue(i) for i in range(52)]
    return np.array(v, np.float32)

POINT_VALUE = pointValue()

def oneIfTrue(b):
    return 1.0 if b else 0.0

# This produces just a vector of length 4 which is not merged into the "distribution"
# Crucially, it needs to be rotated such that the current player is first.
def makePointsSoFar(pointsSoFar, current, trickSoFar):
    pointsSoFar = np.array(pointsSoFar, np.int32)
    if current != 0:
        vec = np.roll(pointsSoFar, current)

    assert len(pointsSoFar) == 4
    total_points = np.sum(pointsSoFar)
    assert total_points >= 0
    assert total_points < 26

    if total_points == 0:
        currentCanShoot = True
        otherCanShoot = True
        pointsSplit = False
    else:
        canShoot = -1
        for i,p in enumerate(pointsSoFar.tolist()):
            if p == total_points:
                canShoot = i
        pointsSplit = canShoot == -1
        currentCanShoot = canShoot == 0
        otherCanShoot = canShoot > 0

    vec = pointsSoFar.astype(np.float32)
    vec = np.divide(vec, 26.0)

    pointsOnTable = 0
    for card in trickSoFar:
        if card != -1:
            assert card >= 0
            assert card < 52
            pointsOnTable += POINT_VALUE[card]

    vec = np.append(vec, pointsOnTable)
    vec = np.append(vec, oneIfTrue(not pointsSplit))
    vec = np.append(vec, oneIfTrue(pointsSplit))
    vec = np.append(vec, oneIfTrue(currentCanShoot))
    vec = np.append(vec, oneIfTrue(otherCanShoot))

    assert vec.shape == POINTS_SO_FAR_SHAPE
    return vec

# Given a 4x52 distribution, roll the distribution so that current player is in position 0
# First axis: Player
# Second axis: rank
def rotate(distribution, current):
    assert distribution.shape == (4,52)
    distribution = np.roll(distribution, current, axis=0)
    return distribution

def toNumpy(state):
    X = np.array([], dtype=np.float32)
    # tranform state into the numpy representation we'll feed to keras/tensorflow
    # returns a pair of arrays (x, label)

    # state.distribution is a sparse array of (card, probs[4])
    # we need to expand it to be a non-sparse array
    distribution = expandDistribution(state['distribution'])
    assert distribution.shape == (NUM_PLAYERS, CARDS_IN_DECK)

    # Create a vector with the number of points each *unplayed* card is worth.
    # points flow from this column, to the points on table scalar, to the players total points so far.
    # We create it here but concatenate it onto the output feature vector below
    pointValueColumn = distribution.T
    assert pointValueColumn.shape == (CARDS_IN_DECK, NUM_PLAYERS)
    pointValueColumn = np.sum(pointValueColumn, axis=-1)
    assert pointValueColumn.shape == (CARDS_IN_DECK,)
    pointValueColumn = np.multiply(pointValueColumn, POINT_VALUE)
    assert pointValueColumn.shape == (CARDS_IN_DECK,)

    distribution = rotate(distribution, state['current'])
    assert distribution.shape == (NUM_PLAYERS, CARDS_IN_DECK)

    distribution = distribution.flatten()
    assert distribution.shape == (NUM_PLAYERS * CARDS_IN_DECK,)

    # Distribution is now in (player, card) order.
    # The current player is in position 0

    # Legal plays is 1.0 for every possible legal play, 0 otherwise
    legalPlays = makeLegalPlays(state['expectedOutputs'])
    assert legalPlays.shape == (CARDS_IN_DECK,)

    canCardTakeTrick = makeCanCardTakeTrick(state['trickSoFar'], state['expectedOutputs'])
    assert canCardTakeTrick.shape == (CARDS_IN_DECK,)

    distribution = np.concatenate((distribution, legalPlays, canCardTakeTrick, pointValueColumn), axis=0)
    assert distribution.shape == (INPUT_FEATURES * CARDS_IN_DECK,)

    # Create a vector of 9 values
    # The first 4 are the points taken by each player in previous tricks of the game
    # These values are rotated so that the current player is first.
    # The fifth value is the sum of the points on the table
    # The remaining 4 values
    pointsSoFar = makePointsSoFar(state['pointsSoFar'], state['current'], state['trickSoFar'])
    assert pointsSoFar.shape == POINTS_SO_FAR_SHAPE

    extra = np.array(state['extra_features'], dtype=np.float32)
    assert extra.shape == (EXTRA_FEATURES,)

    # Finally create the model input vector from distribution and pointsSoFar
    input = np.concatenate((distribution, pointsSoFar, extra), axis=0)
    assert input.shape == MAIN_INPUT_SHAPE

    # These are is the lables (y-values) of the observation
    expectedScores, winTrickProbs, moonProbs = expandExpectedOutputs(state)
    assert expectedScores.shape == SCORES_SHAPE
    assert winTrickProbs.shape == WIN_TRICK_PROBS_SHAPE
    assert moonProbs.shape == MOONPROBS_SHAPE

    return (input, expectedScores, winTrickProbs, moonProbs)


def parseFile(inFilePath, group):
    with open(inFilePath, 'r') as inFile:
        while True:
            try:
                state = parseOneState(inFile)
                oneMain, oneScores, oneWinTrickProbs, oneMoonProbs = toNumpy(state)

                group['main'].append(oneMain)
                group['scores'].append(oneScores)
                group['winTrick'].append(oneWinTrickProbs)
                group['moonProb'].append(oneMoonProbs)

            except (EOFError, StopIteration):
                break


def transformOneFile(inFilePath):
    assert os.path.isfile(inFilePath)
    outDirPath = inFilePath + '.d'

    group = {
        'main': [],
        'scores': [],
        'winTrick': [],
        'moonProb': [],
    }
    parseFile(inFilePath, group)
    memmap.save_group(group, outDirPath)


if __name__ == '__main__':
    np.set_printoptions(linewidth=160)
    inFileName = '??' if len(sys.argv)==1 else sys.argv[1]

    trainingPaths = glob.glob(f'training/{inFileName}')
    validationPaths = glob.glob(f'validation/{inFileName}')
    if len(trainingPaths)>0 and len(trainingPaths) == len(validationPaths):
        print('Transforming:', trainingPaths)
        for path in trainingPaths:
            transformOneFile(path)
        print('Transforming:', validationPaths)
        for path in validationPaths:
            transformOneFile(path)
