# How to build

Setup Emescripten toolchain.

https://emscripten.org/docs/getting_started/downloads.html

Then build TinyUSDZ with emcmake

```
$ cd <tinyusdz>/sandbox/emscripten
$ emcmake cmake -DCMAKE_BUILD_TYPE=Release -B build_em -S ../../
```

This just build a library. No example yet.

You can also look into `<tinyusdz>/examples/sdlviewer`. `sdlviewer` example has an option to built it with Emscripten.

EoL.

