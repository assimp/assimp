# pxr USD library API experiment

## Build pxr USD

Build pxrUSD in some way.
We recommend to use monolithic build of pxrUSD.

## Build

### Linux

Edit pxrUSD and other settings in `bootstrap-linux.sh`, then

```
$ ./bootstrap-linux.sh
$ cd build
$ cmake
```

### Windows(Visual Studio)

```
> rmdir /s /q build
> mkdir build
> cmake.exe -G "Visual Studio 17 2022" -A x64 -B build [USD_FLAGS] -S .
```

See `bootstrap-linux.sh` for USD_FLAGS

EoL.
