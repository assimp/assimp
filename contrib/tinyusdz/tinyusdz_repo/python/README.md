# Python binding of TinyUSDZ

Currently in testing stage. Does not work well.

Core part is deletegated to native module(ctinyusd.so)

W.I.P.

## Requirements

* Python 3.7 or later
  * Python 3.10+ recommended
  * For developping and testing, Python 3.8 or later is required.

### Recommended python modules

* numpy
  * For efficient Attribute data handling.
  * `from_numpy` and `to_numpy` method available when `numpy` is installed..
* pandas
  * To support TimeSamples data efficiently(e.g. read/write to CSV, Excel)
* typeguard
  * To do type check at runtime.

## Structure

* `ctinyusdz`: Native C++ module of tinyusdz
  * Python binding on top of C binding of TinyUSDZ.
* `tinyusdz`: Python module. Wraps some functions of `ctinyusdz`

TinyUSDZ's Python binding approach is like numpy: Frontend is written in Python for better Python integration(type hints for lsp(Intellisense), debuggers, exceptions, ...), and calls native C modules as necessary.

## Supported platform

* [x] Linux
  * [x] x86-64
  * [x] aarch64
* [x] Windows
  * [x] 64bit AMD64
  * [x] 32bit x86
  * [ ] ARM64(not intensively tested, but should work)
* [x] macOS
  * [x] arm64
  * [x] Intel

## Features

* T.B.W.

### Optional

* [ ] pxrUSD compatible Python API?(`pxr_compat` folder)

## Install through PyPI

```
$ python -m pip install tinyusdz
```

## For developers. Build from source

Back to tinyusdz's root directory, then

```
# Use `build` module(install it with `python -m pip install build`) 
$ python -m build .
```

If you are working on TinyUSDZ Python module, Using `setup.py` recommended. 

```
# install dependencies
$ python -m pip install setuptools scikit-build cmake ninja
```

```
$ python setup.py build
# Then copy `./_skbuild/<arch>-<version>/cmake-install/tinyusdz/ctinyusdz.*.so/dll to `<tinyusdz>/python` folder.
```

### Re-generate ctinyusdz.py

When TinyUSDZ C API has been updated, need to re-genrerate `<tinyusdz>/python/tinyusdz/ctinyusd.py` 
using ctypesgen https://github.com/ctypesgen/ctypesgen .

```
# if you do not install ctypesgen
$ python -m pip install ctypesgen


$ cd <tinyusdz>/python
$ sh gen-ctypes.sh 
```

### Asan support

If you built ctinyusdz with ASAN enabled, use `LD_PRELOAD` to load asan modules.

```
LD_PRELOAD=/path/to/clang+llvm-14.0.0-x86_64-linux-gnu-ubuntu-18.04/lib/clang/14.0.0/lib/x86_64-unknown-linux-gnu/libclang_rt.asan.so  python tutorial.py
```

Please see https://tobywf.com/2021/02/python-ext-asan/ for more infos.

## License

Apache 2.0 license.

EoL.
