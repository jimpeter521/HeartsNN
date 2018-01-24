.PHONY: hearts deal build test disttest analyze all analyze1 analyze2

hearts:
	bazel build :hearts && bazel-bin/hearts

opt:
	bazel build :hearts -c opt && time bazel-bin/hearts 2

deal:
	bazel run :deal

tournament:
	bazel build :tournament -c opt && bazel-bin/tournament 1

analyze:
	bazel build :analyze

build:
	bazel build --show_progress :all

test:
	bazel test --test_output=all :all

disttest:
	bazel run :disttest

all: opt tournament analyze disttest deal

analyze1: analyze
	bazel-bin/analyze -d 2006d6c0864151ba79c30127   # slam dunk shoot moon current player

analyze2: analyze
	bazel-bin/analyze -d 21b91f40fb464bc8fdda384d   # other player (2) shoots moon. May indicate bug in empirical score?
