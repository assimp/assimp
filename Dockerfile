FROM gcc:latest

RUN apt-get update && apt-get install --no-install-recommends -y ninja-build cmake 

WORKDIR /app
RUN apt install zlib1g-dev

COPY . .

RUN mkdir build && cd build && \
    cmake -G 'Ninja' \
    -DCMAKE_BUILD_TYPE=Release \
    -DASSIMP_BUILD_ASSIMP_TOOLS=ON \
    .. && \
    ninja -j4 && ninja install

CMD ["/app/build/bin/unit"]
