#!/bin/bash

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
pattern='^v[0-9]\.[0-9](\.[0-9])?$'

function build-image {
    BRANCH=$(git rev-parse --abbrev-ref HEAD)
    NO_CACHE=$2
    echo "-------"
    echo "Building heartsnn/$1 ${NO_CACHE}"
    docker build ${NO_CACHE} -t heartsnn/$1:${TAG} -f Dockerfiles/$1 https://github.com/jimlloyd/HeartsNN.git#${BRANCH}

    if [[ "${TAG}" =~ ${pattern} ]]
    then
        docker push heartsnn/$1:${TAG}
    fi

}

# Everything starts with this image from the awesome floopcz/tensorflow project https://github.com/FloopCZ/tensorflow_cc
# We use the `ubuntu-shared` image here, but may switch to `ubuntu-shared-cuda` in the future.
# docker pull floopcz/tensorflow_cc:ubuntu-shared
# We don't really need to pull here, it will happen automatically when building tf-protobuf

# These three images are intermediate layers build seprately to faciliate incremental builds.
# They are primarily useful to anyone who wants to enhance the heartsnn project in any way.
build-image "tf-protobuf"
build-image "tf-protobuf-grpc"
build-image "build"

# This is the true base docker image, the only one needed if for generating data and building models
build-image "heartsnn"

# In the future there will be more docker images and some docker-compose scripts
# for doing multiple rounds of reinforcement learning, and for playing hearts using the build models

build-image "generation-0"
