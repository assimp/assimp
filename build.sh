#!/bin/sh
# It uses the same alpine image and therefore creates a musl-based executable

# Build the docker container
docker build -t assimp-alpine-dev-build:1.0 .

# Run the docker container and build the tool
docker run -v ${PWD}:/code  -it assimp-alpine-dev-build:1.0
