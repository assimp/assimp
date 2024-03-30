rm -rf build
mkdir -p build

cd build

cmake -DOptiX_INSTALL_DIR=${HOME}/local/NVIDIA-OptiX-SDK-7.6.0-linux64-x86_64 \
  ..


