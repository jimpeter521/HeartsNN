.PHONY: hearts deal build test disttest analyze all analyze1 analyze2

hearts:
	buck run :hearts

opt:
	buck run @mode/opt :hearts

deal:
	buck run :deal

tournament:
	buck run @mode/opt :tournament

analyze:
	buck build :analyze

build:
	buck build --show_progress :all

test:
	buck test

disttest:
	buck run :disttest

all: opt tournament analyze disttest deal

analyze1: analyze
	analyze -d 2006d6c0864151ba79c30127   # slam dunk shoot moon current player

analyze2: analyze
	analyze -d 21b91f40fb464bc8fdda384d   # other player (2) shoots moon. May indicate bug in empirical score?
