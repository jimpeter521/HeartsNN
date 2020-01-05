.PHONY: hearts deal build test disttest analyze all analyze1 analyze2 xcode_clean xcode_pristine

debug:
	mkdir -p debug

debug/Makefile: debug
	cmake/genbuild.sh debug

release:
	mkdir -p release

release/Makefile: release
	cmake/genbuild.sh release

xcode:
	mkdir -p xcode

debug-all: debug/Makefile
	make -C debug -j8 all

release-all: release/Makefile
	make -C release -j8 all

xcode/HeartsNN.xcodeproj: xcode
	cd xcode && cmake -G Xcode -DCMAKE_TOOLCHAIN_FILE=$(POLLY_ROOT)/xcode.cmake ..

xcode_clean:
	rm -rf xcode

xcode_pristine : xcode_clean xcode/HeartsNN.xcodeproj

hearts: debug/Makefile
	make -C debug -j8 hearts
	./debug/hearts

opt: release/Makefile
	make -C release -j8 hearts
	./release/hearts

deal: debug/Makefile
	make -C debug -j8 deal
	./debug/deal

tournament: release/Makefile
	make -C release -j8 tournament
	./release/tournament

dtournament: debug/Makefile
	make -C debug -j8 tournament
	./debug/tournament

analyze: release/Makefile
	make -C release -j8 analyze

analyze1: analyze
	./release/analyze -d 2006d6c0864151ba79c30127   # slam dunk shoot moon current player

analyze2: analyze
	./release/analyze -d 21b91f40fb464bc8fdda384d   # other player (2) shoots moon. May indicate bug in empirical score?

analyze3: analyze
	./release/analyze -d 5c7bd13b420f5774e65cd0b7   # Player 3 either shoots the moon or misses by 1 point

analyze4: analyze
	./release/analyze -d a5a9d06f5c340ca6ab864fb6 # Second player shoots the moon with strong, but I think stoppable hand

disttest: debug/Makefile
	make -C debug -j8 disttest
	./debug/disttest

validate: debug/Makefile
	make -C debug -j8 validate

validate1: validate
	./debug/validate 2006d6c0864151ba79c30127

play: release/Makefile
	make -C release -j8 play

server: debug/Makefile
	make -C debug -j8 server testclient cliclient

all: debug-all release-all opt tournament analyze1 disttest deal validate1
