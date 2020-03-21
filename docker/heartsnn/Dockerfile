FROM heartsnn/build:latest

RUN git clone --depth=1 \
    --branch=master \
    --recurse-submodules \
    https://github.com/jimlloyd/HeartsNN.git \
    /heartsnn

RUN apt-get -y install libuv1-dev

WORKDIR /heartsnn

RUN chmod +x build-hearts.sh
RUN ./build-hearts.sh

RUN cp /heartsnn/builds/release/apps/analyze /usr/local/bin/
RUN cp /heartsnn/builds/release/apps/deal /usr/local/bin/
RUN cp /heartsnn/builds/release/apps/hearts /usr/local/bin/
RUN cp /heartsnn/builds/release/apps/tournament /usr/local/bin/
RUN cp /heartsnn/builds/release/apps/validate /usr/local/bin/

WORKDIR dlib
RUN python3 setup.py install
WORKDIR /heartsnn

# RUN rm -rf /heartsnn/builds

ENV PATH="/heartsnn/devbin:${PATH}"
CMD /heartsnn/devbin/bld release all_tests
