WASI experiment.

## Build procedure

Setup `wasi-sdk` https://github.com/WebAssembly/wasi-sdk


Edit WASI_VERSION and path to `wasi-sdk` in `bootstrap-wasi.sh`, then

```
$ ./bootstrap-wasi.sh
$ cd build
$ make
```

## Note

### Binary size

Currently it is less than 4MB when compiled with `-DCMAKE_BUILD_TYPE=MinSizeRel` (650 KB when gzip compressed).

## Run

Currently tested by running app with `wasmtime`.

https://github.com/bytecodealliance/wasmtime

https://github.com/bytecodealliance/wasmtime/blob/main/docs/WASI-tutorial.md

Install `wasmtime`, then copy example USD file to `build` folder(where `tinyusdz_wasi` is located. Otherwise add path using `--dir` option)

```bash
$ cp ../../../models/cube-previewsurface.usda .
$ wasmtime --dir=. tinyusdz_wasi cube-previewsurface.usda
```

## TODO

* [ ] Unify CMakeList.txt with <tinyusdz>/CMakelist.txt
* [ ] Write WASI specific AssetResolution/FileSystemHandler example.
