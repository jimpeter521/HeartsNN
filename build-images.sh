#!/bin/bash

NO_CACHE=$1

git fetch
if [[ -z $(git status -uno -s) ]]
then
    echo "The working tree is clean and up to date with origin"
else
    echo "Can't build in dirty working tree. Must be up to date with origin."
    exit 1
fi

LOCALBRANCH=$(git rev-parse --abbrev-ref HEAD)
REMOTEBRANCH=$(git rev-parse --abbrev-ref --symbolic-full-name @{u})

if git diff --exit-code ${LOCALBRANCH} ${REMOTEBRANCH};
then
    echo "All is well"
else
    echo "The local branch is not identical to the tracking branch"
    exit 1
fi

shopt -s extglob
TAG=$(git describe)
pattern='^v[0-9]\.[0-9](\.[0-9]+)?$'

function build-image {
    echo "-------"
    echo "Building heartsnn/$1 ${NO_CACHE}"
    docker build ${NO_CACHE} -t heartsnn/$1:${TAG} -t heartsnn/$1 -f Dockerfiles/$1 https://github.com/jimlloyd/HeartsNN.git#${TAG}

    if [[ "${TAG}" =~ ${pattern} ]]
    then
        docker push heartsnn/$1:${TAG}
    fi
}

# Everything starts with this image from the awesome floopcz/tensorflow project https://github.com/FloopCZ/tensorflow_cc
# We use the `ubuntu-shared` image here, but may switch to `ubuntu-shared-cuda` in the future.
# docker pull floopcz/tensorflow_cc:ubuntu-shared
# We don't really need to pull here, it will happen automatically when building tf-protobuf

# These two images are intermediate layers build seprately to faciliate incremental builds.
# HeartsNN depends on these components, and they do not depend on HeartsNN in any way.
build-image "tf-protobuf"
build-image "tf-protobuf-grpc"

# This is the true base docker image, the only one needed if for generating data and building models
build-image "heartsnn"

# This generation-0 container will run the zero-th generation of reinforcement learning.
# The data generated for training and evalution is almost pure monte-carlo,
# using whatever statistical regularities that can be inferred from many random "roll-outs".
build-image "generation-0"

# In the future there will be generation-1 .. generation-N for as many iterations of RL as
# are practical and lead to improved play.
# We have already done several iterations with earlier versions of this project that had good results.
# The main development since then has been to convert to using Docker so that this project will be
# easier to use by others.
# Building these higher generation models is more computationally expensive.
# We plan to use Kubernetes to make it possible to build those generations using a cluster of compute nodes.

# This image is work-in-progress towards making it possible to play against HeartsNN using an
# application in web browser.
build-image "websocketserver"
