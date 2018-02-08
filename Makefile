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

analyze1: analyze
	analyze -d 2006d6c0864151ba79c30127   # slam dunk shoot moon current player

analyze2: analyze
	analyze -d 21b91f40fb464bc8fdda384d   # other player (2) shoots moon. May indicate bug in empirical score?

analyze3: analyze
	analyze -d 5c7bd13b420f5774e65cd0b7   # Player 3 either shoots the moon or misses by 1 point

analyze4: analyze
		analyze -d a5a9d06f5c340ca6ab864fb6 # Second player shoots the moon with strong, but I think stoppable hand


validate:
	buck build :validate

validate1: validate
	validate 2006d6c0864151ba79c30127

all: opt tournament analyze disttest deal validate
