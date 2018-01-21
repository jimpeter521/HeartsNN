import numpy as np
import os
import gzip
import glob
import re
import sys

from constants import MAIN_INPUT_SHAPE, SCORES_SHAPE, MOONPROBS_SHAPE, WIN_TRICK_PROBS_SHAPE

limit_re = re.compile(r'^(\d+)([KM]?)$')

def limit() :
    if 'DECK_LIM' in os.environ:
        s_lim = os.environ['DECK_LIM']
        m =limit_re.match(s_lim)
        if not m:
            raise ValueError(f'Env var DECK_LIM must be integer with optional K or M suffix, given:{DECK_LIM}')
        lim = int(m.group(1))
        if m.group(2) == 'K':
            lim *= 1024
        elif m.group(2) == 'M':
            lim *= 1024*1024
    else:
        lim = sys.maxsize    # effectively infinite, but still an integer
    return lim

def readFile(inFile):
    startpos = inFile.tell()
    endpos = inFile.seek(0, 2)
    inFile.seek(startpos, 0)

    mainData = []
    scoresData = []
    winTrickProbData = []
    moonProbData = []

    LIM = limit()
    obs = 0
    while inFile.tell() < endpos and obs < LIM:
        start_pos = inFile.tell()
        oneMain = np.load(inFile)
        oneScores = np.load(inFile)
        oneWinTrickProbs = np.load(inFile)
        oneMoonProbs = np.load(inFile)
        assert oneMain.shape == MAIN_INPUT_SHAPE
        assert oneScores.shape == SCORES_SHAPE
        assert oneWinTrickProbs.shape == WIN_TRICK_PROBS_SHAPE
        assert oneMoonProbs.shape == MOONPROBS_SHAPE
        assert not np.any(np.isnan(oneMain))
        assert not np.any(np.isnan(oneScores))
        assert not np.any(np.isnan(oneWinTrickProbs))
        assert not np.any(np.isnan(oneMoonProbs))
        mainData.append(oneMain)
        scoresData.append(oneScores)
        winTrickProbData.append(oneWinTrickProbs)
        moonProbData.append(oneMoonProbs)
        obs += 1
        num_bytes = inFile.tell() - start_pos

    return np.asarray(mainData, dtype=np.float32), np.asarray(scoresData, dtype=np.float32), np.asarray(winTrickProbData, dtype=np.float32), np.asarray(moonProbData, dtype=np.float32)

def readPath(inFilePath):
    paths = glob.glob(inFilePath)
    if len(paths) > 1:
        return readGlob(inFilePath)

    assert os.path.isfile(inFilePath)
    if inFilePath.endswith('.gz'):
        opener = gzip.open
    else:
        opener = open
    with opener(inFilePath, 'rb') as inFile:
        mainData, scoresData, winTrickProbData, moonProbData  = readFile(inFile)
    return mainData, scoresData, winTrickProbData, moonProbData

def readGlob(inFileGlob):
    paths = glob.glob(inFileGlob)

    main = []
    scores = []
    winTrickProbs = []
    moonProbs = []
    for path in paths:
        mainData, scoresData, winTrickProbData, moonProbsData = readPath(path)
        obs = len(mainData)
        print(f'Read {obs} from {path}')
        main.append(mainData)
        scores.append(scoresData)
        winTrickProbs.append(winTrickProbData)
        moonProbs.append(moonProbsData)
    return np.concatenate(main), np.concatenate(scores), np.concatenate(winTrickProbs), np.concatenate(moonProbs)
