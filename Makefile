.PHONY: hearts deal build test disttest analyze all analyze1 analyze2 xcode_clean xcode_pristine

BUILDS=builds
BUILDNAME ?= debug
BUILDPATH = ${BUILDS}/${BUILDNAME}

${BUILDPATH}:
	mkdir -p $@

${BUILDPATH}/Makefile: ${BUILDPATH}
	cmake/genbuild.sh ${BUILDNAME}

all: ${BUILDPATH}/Makefile
	make -C ${BUILDPATH} -j8 all

hearts: ${BUILDPATH}/Makefile
	make -C ${BUILDPATH} -j8 hearts
	${BUILDPATH}/hearts

deal: ${BUILDPATH}/Makefile
	make -C ${BUILDPATH} -j8 deal
	${BUILDPATH}/deal

tournament: ${BUILDPATH}/Makefile
	make -C ${BUILDPATH} -j8 tournament
	${BUILDPATH}/tournament

analyze: ${BUILDPATH}/Makefile
	make -C ${BUILDPATH} -j8 analyze

analyze1: analyze
	${BUILDPATH}/analyze -d 2006d6c0864151ba79c30127   # slam dunk shoot moon current player

analyze2: analyze
	${BUILDPATH}/analyze -d 21b91f40fb464bc8fdda384d   # other player (2) shoots moon. May indicate bug in empirical score?

analyze3: analyze
	${BUILDPATH}/analyze -d 5c7bd13b420f5774e65cd0b7   # Player 3 either shoots the moon or misses by 1 point

analyze4: analyze
	$${BUILDPATH}/analyze -d a5a9d06f5c340ca6ab864fb6 # Second player shoots the moon with strong, but I think stoppable hand

disttest: ${BUILDPATH}/Makefile
	make -C ${BUILDPATH} -j8 disttest
	${BUILDPATH}/disttest

validate: ${BUILDPATH}/Makefile
	make -C ${BUILDPATH} -j8 validate

validate1: validate
	${BUILDPATH}/validate 2006d6c0864151ba79c30127

play: ${BUILDPATH}/Makefile
	make -C ${BUILDPATH} -j8 play

server: ${BUILDPATH}/Makefile
	make -C ${BUILDPATH} -j8 server testclient cliclient
