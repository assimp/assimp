# Install

```
$ python -m pip install ctypesgen
```

Then, generate ctypes binding using `gen-ctypes.sh`

## Run with ASAN build of libc-tinyusd.so

```
LD_PRELOAD=/mnt/data/local/clang+llvm-14.0.0-x86_64-linux-gnu-ubuntu-18.04/lib/clang/14.0.0/lib/x86_64-unkno  wn-linux-gnu/libclang_rt.asan.so python ctinyusd_test.py
```

Edit path to libclang_rt.asan.so to fit in your environment.

Note that there are few memory leaks(false positives?) exist in Python side(leaks reported even when running empty Python script).

https://github.com/python/cpython/issues/87469

## Recommended module

- numpy
  - currently numpy is a required module to develop/test Python binding of TinyUSDZ.
- typeguard
  - for better runtime type check
