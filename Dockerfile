FROM ubuntu:14.04

RUN apt-get update && apt-get install -y \
    git cmake build-essential software-properties-common

RUN add-apt-repository ppa:ubuntu-toolchain-r/test && apt-get update && apt-get install -y gcc-4.9 g++-4.9 && \
    cd /usr/bin && \
    rm gcc g++ cpp && \
    ln -s gcc-4.9 gcc && \
    ln -s g++-4.9 g++ && \
    ln -s cpp-4.9 cpp

WORKDIR /opt

# Build Assimp
RUN git clone https://github.com/assimp/assimp.git /opt/assimp

WORKDIR /opt/assimp

RUN git checkout master \
    && mkdir build && cd build && \
    cmake \
    -DCMAKE_BUILD_TYPE=Release \
    .. && \
    make && make install
