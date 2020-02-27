FROM heartsnn-build

RUN git clone --depth=1 \
    --branch=master \
    --recurse-submodules \
    https://github.com/jimlloyd/HeartsNN.git \
    /heartsnn

WORKDIR /heartsnn

RUN chmod +x build-hearts.sh
RUN ./build-hearts.sh

RUN cp /heartsnn/builds/release/apps/analyze /usr/local/bin/
RUN cp /heartsnn/builds/release/apps/deal /usr/local/bin/
RUN cp /heartsnn/builds/release/apps/hearts /usr/local/bin/
RUN cp /heartsnn/builds/release/apps/tournament /usr/local/bin/
RUN cp /heartsnn/builds/release/apps/validate /usr/local/bin/

# RUN rm -rf /heartsnn/builds

ENV PATH="/heartsnn/devbin:${PATH}"
CMD /heartsnn/devbin/bld release all_tests
