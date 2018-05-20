# Build Instructions

## Requirements

I've done all of my development on a Mac. The instructions below are mac-specific
because they refer to the use of [Homebrew](https://brew.sh/), but it isn't
difficult to translate these instructions to work for Linux. For now, I leave
that as an exercise for the reader.

## Installing Tensorflow for Mac OS

You can install by following the instructions Google provides here:

1. https://www.tensorflow.org/install/install_mac
2. https://www.tensorflow.org/install/install_sources

The first page assumes you're installing for python only, but you will need to
install from sources, so both pages are required.

The steps below are the actual concrete steps I followed to install from sources.
It's possible (likely?) that I am missing some dependencies. If you run into any,
you should assume that I installed the missing dependency with homebrew.
Please LMK of any dependencies you had to fill in.

### Install Python 3.6

NOTE: You must use python 3.6 if you want to use my python scripts to run training.
I personally just installed `python3` with homebrew, but you can probably use `anaconda`
or `virtualenv` instead.

    brew install python3

If you use pyenv to manage your python versions, follow these steps:

    pyenv install 3.6.3
    pyenv local 3.6.3

### Install Python packages required by Tensorflow

Tensorflow requires these packages.
I don't use `sudo`, because I chown'd /usr/local to be owned by me.

    pip3 install six numpy wheel

(You may be able to use 'pip' instead of 'pip3' depending on how you have python installed.)

### Clone the tensorflow repository

    git clone https://github.com/tensorflow/tensorflow.git
    cd tensorflow

### Checkout a specific release tag

The current latest release is v1.8.0. You can probably use any release from v1.4.0 on, but you might as well use the latest.

    git co v1.8.0

### Run the Tensorflow configuration script

The configure script is interactive, and I don't show the session here. I used the default
response for nearly all questions.

    ./configure

I configure for CPU-only (no GPU) since my Mac doesn't have a usable GPU. For the `--config=opt` setting I use these four flags, which work well on recent Macs

    -march=native -mavx -mfma -msse4.2

### You must install Bazel. The homebrew install is good:

    brew install bazel

### Build tensorflow components

Use Bazel to build these tensorflow components. This will take 30 minutes or so.

    bazel build --config=opt //tensorflow/tools/pip_package:build_pip_package //tensorflow:libtensorflow.so //tensorflow:libtensorflow_cc.so

### Build the PIP package

The above built the program `build_pip_package`. Use it to build a pip package in `/tmp`:

    bazel-bin/tensorflow/tools/pip_package/build_pip_package /tmp/tensorflow_pkg

### Locate the pip package and install it

Look in `/tmp` to find name of the exact package that was built. Install it with `pip3 install`:

    pip3 install /tmp/tensorflow_pkg/tensorflow-*-macosx_*_x86_64.whl

### Arrange for all required build artifacts to be installed into `/usr/local/...`

Everything is now built. We need to install headers and libs into `/usr/local/...` so that C++
applications have access to them. The steps I have below are a bit hacky, but they work:

##### Locate the python3 site packages directory:

    SITE=$(python3 -c "import site; print(site.getsitepackages()[0])")

##### Make a directory in /usr/local/include to receive all of the tensorflow headers:

    mkdir /usr/local/include/tf/

##### Copy headers into `/usr/local/include/tf/...`

    cp -r $SITE/tensorflow/include/* /usr/local/include/tf/
    rsync -avh tensorflow/cc/ /usr/local/include/tf/tensorflow/cc
    rsync -avh bazel-genfiles/external/ /usr/local/include/tf/external
    rsync -avh bazel-genfiles/tensorflow/ /usr/local/include/tf/tensorflow
    sudo cp bazel-bin/tensorflow/libtensorflow*.so /usr/local/lib/

### Building the HeartsNN components

##### Install the necessary Mac OS build tools

The build assumes you have installed `Xcode`, and uses `make`, `cmake`. The `Xcode` install provides `make`, but you probably need to install `cmake`.

	brew install cmake

##### Build all of the HeartsNN components

You should now finally be able to build all `HeartsNN` components with `make all` (after updating submodules):

	cd HeartsNN
    git submodule update --init
	make all

This should build all components. This first build will install and build additional external components such as [DLib](http://dlib.net/) [Google's gRpc](https://grpc.io/), and will take about half an hour.

The final step will be to run several of the components to test the build.

##### Trying out HeartsNN with the client-server architecture in `play_hearts`

If there are no errors, you should be able to play the game of hearts using the server `play_hearts/server`
and the client application `play_hearts/cliclient`. First, you'll need a model. A set of models are available via Dropbox here:

    https://www.dropbox.com/sh/3cxt5vjdat0bxon/AACjHfgF6LBLohH6ipXVaDXJa?dl=0

From that directory, download one of the models, e.g. `round4.model`

You can then start the server with this command in one terminal session:

    debug/play_hearts/server/server round4.model

Then, in another terminal session:

    debug/play_hearts/cliclient/cliclient

(TODO: Document how the models were built. Note that `round5` is a larger model that should be better than `round4`, but the improvement is marginal at best.)

The UI of `cliclient` is pure console I/O

Card choices in `cliclient` app are two letter combinations (case insensitive).

The first letter is the rank, and must be one of `23456789TJQKA`. Note `T` for 10

The second letter is the suit, and must be one of `CDSH`.

Note that I have not implemented the first part of the game, i.e. choosing 3 cards to pass to an opponent. For now, you must always play the 13 cards dealt to you.

##### Comparing models with the application `tournament`.

If you download more than one model, you can two models against each other in a kind of "tournament". First, make sure that the application is built:

    make tournament

Then run it like this:

    release/tournament --champion round2.model --opponent round4.model -g 10

(Note: the terms "champion" and "opponent" are purely arbitrary labels.)

This will play ten "matches", where each match is six games/hands, each played from the same deal of cards, but using the six possible distinct permuations of the two players playing the four possible positions at the table.

The intention is to try to have a completely fair evaluation of how two strategies play against each other, controlling for the variability of good vs bad hands. Hearts is a game where even a perfect player will be unable to win given some deals. In any given deal, it is common for one or two players to have distinctly "good" hands and one or two players to have distinctly "bad" hands. The six-game match is designed to make sure that each player strategy has to not only play each of the hands, but to also do so against the three possible ways of facing a copy of himself and two copies of the other player strategy.

The match is score by summing for each player the scores across all games. Note that this is NOT team play, none of the players has any understanding of the match structure, and plays purely to optimize only his own outcome.

Note that the models, when played as above, are purely deterministic, i.e. they do not use pseudorandom numbers as part of their play. If the same model is played against itself, the match will always result in a perfect tie.

Also, it is possible to choose in advance the deals that will be used in a match. If the same set of deals is played with the same two players, the outcomes will be identical.
