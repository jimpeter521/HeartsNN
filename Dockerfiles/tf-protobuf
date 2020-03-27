FROM floopcz/tensorflow_cc:ubuntu-shared

# Some of these are not required in this docker image, but will be required to achieve
# the final goal image
ENV DEBIAN_FRONTEND noninteractive
RUN apt-get -y update && apt-get -y install \
    apt-utils \
    autoconf \
    automake \
    build-essential \
    curl \
    g++ \
    git \
    libc-ares-dev \
    libssl-dev \
    libtool \
    make \
    pkg-config \
    unzip

# Install protobuf v3.8.0 -- This exact version is required for compatiblity with the tensorflow installed above
RUN mkdir -p /tmp/protobuf
WORKDIR /tmp/protobuf
ADD https://github.com/protocolbuffers/protobuf/releases/download/v3.8.0/protobuf-cpp-3.8.0.tar.gz .
RUN tar -xf protobuf-cpp-3.8.0.tar.gz
WORKDIR /tmp/protobuf/protobuf-3.8.0
RUN ./configure  --disable-shared --prefix=/usr/local && make -j4 && make check && make install && ldconfig
WORKDIR /
RUN rm -rf /tmp/protobuf

# Install cmake v3.16.2
ADD https://github.com/Kitware/CMake/releases/download/v3.16.2/cmake-3.16.2-Linux-x86_64.sh /cmake-Linux-x86_64.sh
RUN mkdir /opt/cmake
RUN sh /cmake-Linux-x86_64.sh --prefix=/usr/local --skip-license
