import re
import os

CARDS_IN_DECK = 52
NUM_SUITS = 4
NUM_RANKS = 13
NUM_PLAYERS = 4
PLAYS_PER_TRICK = 4

MOON_FLAGS_LEN = 4
POINTS_SO_FAR_LEN = PLAYS_PER_TRICK + 1 + MOON_FLAGS_LEN

# 6 features columns. The probabilities for each of the 4 players,
# plus 2 more: legal plays and point value
# We put legal plays first, then the 4 probabilities for the 4 players, then point value.
INPUT_FEATURES = NUM_PLAYERS + 2

EXTRA_FEATURES = 33

TOTAL_SCALAR_FEATURES = CARDS_IN_DECK*INPUT_FEATURES + POINTS_SO_FAR_LEN + EXTRA_FEATURES

DECK_SHAPE = (CARDS_IN_DECK,)

MAIN_INPUT_SHAPE = (TOTAL_SCALAR_FEATURES,)

SUITS_RANKS_SHAPE = (NUM_SUITS, NUM_RANKS)

POINTS_SO_FAR_SHAPE = (POINTS_SO_FAR_LEN,)

SCORES_SHAPE = DECK_SHAPE
WIN_TRICK_PROBS_SHAPE = DECK_SHAPE
MOON_CLASSES = 3
MOONPROBS_SHAPE = (MOON_CLASSES*CARDS_IN_DECK,)

number_re = re.compile(r'^(\d+)([KM]?)$')

default_vals = {
    'DECK_EPOCHS': 100,
    'DECK_BATCH': 64*1024,
}

def env_val(name) :
    if name in os.environ:
        s_val = os.environ[name]
        m = number_re.match(s_val)
        if not m:
            raise ValueError(f'Env var {name} must be integer with optional K or M suffix, given: {name}')
        val = int(m.group(1))
        if m.group(2) == 'K':
            val *= 1024
        elif m.group(2) == 'M':
            val *= 1024*1024
        print(f'Set {name} to {val}')
    else:
        val = default_vals[name]
    return val

BATCH = env_val('DECK_BATCH')
EPOCHS = env_val('DECK_EPOCHS')

MAIN_DATA = 'main_data'
EXPECTED_SCORE = 'expected_score'
WIN_TRICK_PROB = 'win_trick_prob'
MOON_PROB = 'moon_prob'
