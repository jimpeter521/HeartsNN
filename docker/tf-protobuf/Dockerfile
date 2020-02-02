FROM floopcz/tensorflow_cc:ubuntu-shared

RUN apt-get -y update && apt-get -y install \
    autoconf \
    automake \
    build-essential \
    curl \
    git \
    g++ \
    libtool \
    make \
    pkg-config \
    unzip

RUN mkdir -p /tmp/protobuf
WORKDIR /tmp/protobuf
ADD https://github.com/protocolbuffers/protobuf/releases/download/v3.8.0/protobuf-cpp-3.8.0.tar.gz .
RUN tar -xf protobuf-cpp-3.8.0.tar.gz
WORKDIR /tmp/protobuf/protobuf-3.8.0
RUN ./configure --prefix=/usr && make && make check && make install && ldconfig
WORKDIR /
RUN rm -rf /tmp/protobuf
