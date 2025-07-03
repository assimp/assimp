FROM gcc:1.5.1.0

RUN apt-get update \
    apt-get install --no-install-recommends -y ninja-build cmake zlib1g-dev

WORKDIR /app

COPY . .

RUN mkdir build && cd build && \
    cmake -G 'Ninja' \
    -DCMAKE_BUILD_TYPE=Release \
    -DASSIMP_BUILD_ASSIMP_TOOLS=ON \
    .. && \
    ninja -j4 && ninja install

CMD ["/app/build/bin/unit"]
