#!/bin/bash

function build-image {
    docker build -t heartsnn/$1 docker/$1
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
docker build --no-cache -t heartsnn/heartsnn docker/heartsnn
# build-image "heartsnn"

# In the future there will be more docker images and some docker-compose scripts
# for doing multiple rounds of reinforcement learning, and for playing hearts using the build models

# build-image "generation-0"
