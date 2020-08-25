FROM ubuntu:20.04

ARG TOOLCHAIN_NAME=gcc-9-cxx17-fpic
ARG BUILD_TYPE=RelWithDebInfo

RUN apt-get update && apt-get install -y --no-install-recommends \
    cmake \
    gcc-9 \
    g++-9 \
    ca-certificates \
    make

WORKDIR /root/assimp
COPY ./cmake ./cmake
COPY ./cmake-modules ./cmake-modules
COPY ./code ./code
COPY ./include ./include
COPY ./samples ./samples
COPY ./test ./test
COPY ./tools ./tools
COPY ./*.in ./
COPY CMakeLists.txt ./

RUN cmake -DBUILD_SHARED_LIBS=OFF -DASSIMP_BUILD_ASSIMP_TOOLS=ON -DASSIMP_BUILD_SAMPLES=OFF -DASSIMP_BUILD_TESTS=ON -DASSIMP_INSTALL_PDB=OFF -DASSIMP_IGNORE_GIT_HASH=ON -DASSIMP_HUNTER_ENABLED=ON -H. -B_builds/${TOOLCHAIN_NAME}-${BUILD_TYPE} -DCMAKE_TOOLCHAIN_FILE=./cmake/polly/${TOOLCHAIN_NAME}.cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE}

RUN make -C _builds/${TOOLCHAIN_NAME}-${BUILD_TYPE} -j 4
