#!/bin/bash

# Asssume Cmake 3.20+


glslang_dist_dir=`pwd`/dist
vk_header_dist_dir=`pwd`/dist
vk_loader_dist_dir=`pwd`/dist

function build_vk_headers() {
	git clone https://github.com/KhronosGroup/Vulkan-Headers

	rm -rf vk_header_build
	mkdir vk_header_build

	cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="${vk_header_dist_dir}" -B vk_header_build Vulkan-Headers
	cmake --build vk_header_build --config Release
	cmake --build vk_header_build --config Release --target install 
}


function build_validation_layer() {
	git clone https://github.com/KhronosGroup/Vulkan-ValidationLayers.git

	cd Vulkan-ValidationLayers

	rm -rf build
	mkdir build
	cd build

	python3 ../scripts/update_deps.py --dir ../external --arch x64 --config debug

	cmake -G Ninja -C ../external/helper.cmake -DCMAKE_BUILD_TYPE=Debug ..

	cmake --build . --config Debug

}

function build_glslang() {
	git clone https://github.com/KhronosGroup/glslang.git

	rm -rf glslang_build
	mkdir glslang_build
	cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="${glslang_dist_dir}" -B glslang_build glslang
	cmake --build glslang_build --config Release
	cmake --build glslang_build --config Release --target install 
}

function build_vk_loader() {

	rm -rf vk_loader_build
	mkdir vk_loader_build

	git clone https://github.com/KhronosGroup/Vulkan-Loader.git

	cmake -S Vulkan-Loader -B vk_loader_build -DCMAKE_INSTALL_PREFIX="${vk_loader_dist_dir}" -DUPDATE_DEPS=On
	cmake --build vk_loader_build
	cmake --build vk_loader_build --config Debug
	cmake --build vk_loader_build --config Debug --target install 
}

build_vk_headers
build_validation_layer
build_glslang
build_vk_loader
