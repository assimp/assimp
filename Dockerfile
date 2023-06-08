FROM ubuntu:22.04

RUN apt-get update && apt-get install -y ninja-build \
    git cmake build-essential software-properties-common

RUN add-apt-repository ppa:ubuntu-toolchain-r/test && apt-get update 

WORKDIR /opt

# Build Assimp
RUN git clone https://github.com/assimp/assimp.git /opt/assimp

WORKDIR /opt/assimp

RUN git checkout master \
    && mkdir build && cd build && \
    cmake -G 'Ninja' \
    -DCMAKE_BUILD_TYPE=Release \
    -DASSIMP_BUILD_ASSIMP_TOOLS=ON \
    .. && \
    ninja -j4 && ninja install
