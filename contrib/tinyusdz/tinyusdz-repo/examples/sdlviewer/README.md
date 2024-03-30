# Simple viewer with SDL2

This viewer uses NanoRT(SW ray tracer) to render the model and display it using SDL2
(So no OpenGL dependency)

## Requirements 

* C++14 compiler
  * external library imgui_sdl requires C++14
* cmake
* X11 related package(Linux only)

## Build

```
$ mkdir build
$ cd build
$ cmake ..
$ make
```

## Run

```
$ ./usdz_view <input.usdz>
```

## WebAssembly demo

Setup emcc, then

```
$ ./bootstrap-emscripten-linux.sh
$ cd build_emcc/
$ make
```

Then run http server(e.g. `python -m http.server`) and open `usdz_view.html`.


## TODO

* [ ] Subdivision surface

## Third party libraries

* imgui : MIT license.
* SDL2 : zlib license.
* nativefiledialog-extended : zlib license.
