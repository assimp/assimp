## Requirements

clang with fuzzer support(`-fsanitize=fuzzer`. at least clang 8.0 should work)

## Status

Currently no open issue found by fuzzer.
(except for third party library, especially stb_image)

## Setup

### Ubuntu 18.04

```
$ sudo apt install clang++-8
$ sudo apt install libfuzzer-8-dev
```

Optionally, if you didn't set `update-alternatives` you can set `clang++` to point to `clang++8`(if your clang version is less than 8.0)

```
$ sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-8 10
$ sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-8 10
```

## How to compile

```
$ CXX=clang++ CC=clang meson build -Db_sanitize=address
$ cd build
$ ninja
```


## How to run

Set input size and run fuzz main.

```
$ ./fuzz_tinyusdz -max_len=128m
```

for fuzzing `fuzz_intcoding_decompress`, capping max memory is required(otherwise oom happens).
(Currently `fuzz_intcoding_decompress` does HARD limit of compressed data up to 2GB)

Use `-rss_limit_mb=8192`(or more if you encounter oom and have enough memory) to limit memory usage.


```
$ ./fuzz_intcoding_decompress -rss_limit_mb=8192 -jobs 4
```

## PoC and regressesions

PoC/regression dataset is managed in separate git repo. https://github.com/lighttransport/usd-fuzz

