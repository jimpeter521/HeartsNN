FROM heartsnn-build

RUN git clone --depth=1 \
    --branch=chore/build-in-docker.v1 \
    --recurse-submodules \
    https://github.com/jimlloyd/HeartsNN.git \
    /heartsnn

WORKDIR /heartsnn

RUN chmod +x build-hearts.sh
RUN ./build-hearts.sh

RUN cp /heartsnn/builds/release/analyze /usr/local/bin/
RUN cp /heartsnn/builds/release/deal /usr/local/bin/
RUN cp /heartsnn/builds/release/hearts /usr/local/bin/
RUN cp /heartsnn/builds/release/tournament /usr/local/bin/
RUN cp /heartsnn/builds/release/validate /usr/local/bin/

# RUN rm -rf /heartsnn/builds

ENV PATH="/heartsnn/devbin:${PATH}"
CMD /heartsnn/devbin/bld release all_tests
