FROM tf-protobuf

ENV DEBIAN_FRONTEND noninteractive
RUN apt-get -y install \
    apt-utils

RUN apt-get -y install \
    libc-ares2 libc-ares-dev

WORKDIR /tmp
RUN git clone https://github.com/grpc/grpc.git
WORKDIR /tmp/grpc
RUN git submodule update --init --recursive

RUN mkdir -p cmake/build && cd cmake/build && cmake \
    -DgRPC_CARES_PROVIDER=module    \
    -DgRPC_PROTOBUF_PROVIDER=package \
    -DgRPC_SSL_PROVIDER=module      \
    -DgRPC_ZLIB_PROVIDER=module     \
    ../.. && make && make install
