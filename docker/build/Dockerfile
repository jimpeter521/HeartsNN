FROM floopcz/tensorflow_cc:ubuntu-shared

RUN apt-get -y update && apt-get -y install \
    autoconf automake libtool curl make g++ unzip

RUN mkdir -p /tmp/protobuf
WORKDIR /tmp/protobuf
ADD https://github.com/protocolbuffers/protobuf/releases/download/v3.8.0/protobuf-cpp-3.8.0.tar.gz .
RUN tar -xf protobuf-cpp-3.8.0.tar.gz
WORKDIR /tmp/protobuf/protobuf-3.8.0
RUN ./configure --prefix=/usr && make && make check && make install && ldconfig
WORKDIR /
RUN rm -rf /tmp/protobuf

RUN apt-get -y update && apt-get -y install \
  libgrpc++-dev

#install latest cmake
ADD https://github.com/Kitware/CMake/releases/download/v3.16.2/cmake-3.16.2-Linux-x86_64.sh /cmake-Linux-x86_64.sh
RUN mkdir /opt/cmake
RUN sh /cmake-Linux-x86_64.sh --prefix=/opt/cmake --skip-license
RUN ln -s /opt/cmake/bin/cmake /usr/local/bin/cmake

# Allow mounting devbin on the path.
ENV PATH="/vol/devbin:${PATH}"
