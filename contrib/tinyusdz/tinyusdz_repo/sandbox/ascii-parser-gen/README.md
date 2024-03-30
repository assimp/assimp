# Simple Ascii parser code gen experiment.

## Why

* C macro is not enough
* C++ template is too overkill
  * Slow compilation
  * sometimes it generates lots of code sections which fails to compile(need /bigobj switch in MSVC)

Want a something like between C macro and C++ template. So this codegen approach will fill the gap.

## How to use

Simply run 

```
$ python gen.py
```

Put generated .h and .cc to ../../src/

