FROM floopcz/tensorflow_cc:ubuntu-static

#install latest cmake
ADD https://github.com/Kitware/CMake/releases/download/v3.16.2/cmake-3.16.2-Linux-x86_64.sh /cmake-Linux-x86_64.sh
RUN mkdir /opt/cmake
RUN sh /cmake-Linux-x86_64.sh --prefix=/opt/cmake --skip-license
RUN ln -s /opt/cmake/bin/cmake /usr/local/bin/cmake

# docker build -t heartsnn .
# docker run heartsnn cmake --version

# Allow mounting devbin on the path.
ENV PATH="/vol/devbin:${PATH}"
