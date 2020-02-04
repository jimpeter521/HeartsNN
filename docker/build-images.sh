#!/bin/bash

function build-image {
    docker build -t $1 $1
}

docker pull floopcz/tensorflow_cc:ubuntu-shared
build-image "tf-protobuf"
build-image "tf-protobuf-grpc"
build-image "heartsnn-build"
