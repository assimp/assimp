#!/bin/bash -eu

# Build directory
mkdir -p build
cd build

# Configure
cmake .. \
  -G Ninja \
  -DCMAKE_C_COMPILER="${CC}" \
  -DCMAKE_CXX_COMPILER="${CXX}" \
  -DCMAKE_C_FLAGS="${CFLAGS}" \
  -DCMAKE_CXX_FLAGS="${CXXFLAGS}" \
  -DASSIMP_BUILD_ZLIB=ON \
  -DASSIMP_BUILD_TESTS=OFF \
  -DASSIMP_BUILD_ASSIMP_TOOLS=OFF \
  -DBUILD_SHARED_LIBS=OFF \
  -DASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT=ON \
  -DASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT=ON

# Build the library
ninja

# Helper function to build fuzzers
build_fuzzer() {
    local fuzzer_name=$1
    local source_file=$2

    echo "Building $fuzzer_name..."
    $CXX $CXXFLAGS -I../include -I../build/include -c "$source_file" -o "${fuzzer_name}.o"

    $CXX $CXXFLAGS $LIB_FUZZING_ENGINE "${fuzzer_name}.o" -o "$OUT/${fuzzer_name}" \
        ./lib/libassimp.a \
        ./contrib/zlib/libzlibstatic.a \
        -lpthread -ldl
}

# 1. Generic Fuzzer
build_fuzzer "assimp_fuzzer" "../fuzz/assimp_fuzzer.cc"
# Corpus for generic fuzzer (all models)
(cd ../test/models && zip -q -r $OUT/assimp_fuzzer_seed_corpus.zip .)
# Dictionary
cp ../fuzz/assimp_fuzzer.dict $OUT/assimp_fuzzer.dict || true


# 2. OBJ Fuzzer
build_fuzzer "assimp_fuzzer_obj" "../fuzz/assimp_fuzzer_obj.cc"
if [ -d "../test/models/OBJ" ]; then
    (cd ../test/models/OBJ && zip -q -r $OUT/assimp_fuzzer_obj_seed_corpus.zip .)
fi
cp ../fuzz/assimp_fuzzer.dict $OUT/assimp_fuzzer_obj.dict || true


# 3. GLTF Fuzzer (text format only, glTF and glTF2 versions)
build_fuzzer "assimp_fuzzer_gltf" "../fuzz/assimp_fuzzer_gltf.cc"
mkdir -p gltf_corpus
[ -d "../test/models/glTF" ] && cp -r ../test/models/glTF/* gltf_corpus/
[ -d "../test/models/glTF2" ] && cp -r ../test/models/glTF2/* gltf_corpus/
if [ -d "gltf_corpus" ] && [ "$(ls -A gltf_corpus)" ]; then
    (cd gltf_corpus && zip -q -r $OUT/assimp_fuzzer_gltf_seed_corpus.zip .)
fi
rm -rf gltf_corpus
cp ../fuzz/assimp_fuzzer.dict $OUT/assimp_fuzzer_gltf.dict || true


# 4. GLB Fuzzer (binary glTF format)
build_fuzzer "assimp_fuzzer_glb" "../fuzz/assimp_fuzzer_glb.cc"
mkdir -p glb_corpus
# GLB files can be found in glTF and glTF2 directories
[ -d "../test/models/glTF" ] && find ../test/models/glTF -name "*.glb" -exec cp {} glb_corpus/ \; 2>/dev/null || true
[ -d "../test/models/glTF2" ] && find ../test/models/glTF2 -name "*.glb" -exec cp {} glb_corpus/ \; 2>/dev/null || true
if [ -d "glb_corpus" ] && [ "$(ls -A glb_corpus)" ]; then
    (cd glb_corpus && zip -q -r $OUT/assimp_fuzzer_glb_seed_corpus.zip .)
fi
rm -rf glb_corpus
cp ../fuzz/assimp_fuzzer.dict $OUT/assimp_fuzzer_glb.dict || true


# 5. FBX Fuzzer
build_fuzzer "assimp_fuzzer_fbx" "../fuzz/assimp_fuzzer_fbx.cc"
if [ -d "../test/models/FBX" ]; then
    (cd ../test/models/FBX && zip -q -r $OUT/assimp_fuzzer_fbx_seed_corpus.zip .)
fi
cp ../fuzz/assimp_fuzzer.dict $OUT/assimp_fuzzer_fbx.dict || true


# 6. Collada Fuzzer
build_fuzzer "assimp_fuzzer_collada" "../fuzz/assimp_fuzzer_collada.cc"
if [ -d "../test/models/Collada" ]; then
    (cd ../test/models/Collada && zip -q -r $OUT/assimp_fuzzer_collada_seed_corpus.zip .)
fi
cp ../fuzz/assimp_fuzzer.dict $OUT/assimp_fuzzer_collada.dict || true


# 7. STL Fuzzer
build_fuzzer "assimp_fuzzer_stl" "../fuzz/assimp_fuzzer_stl.cc"
if [ -d "../test/models/STL" ]; then
    (cd ../test/models/STL && zip -q -r $OUT/assimp_fuzzer_stl_seed_corpus.zip .)
fi
cp ../fuzz/assimp_fuzzer.dict $OUT/assimp_fuzzer_stl.dict || true