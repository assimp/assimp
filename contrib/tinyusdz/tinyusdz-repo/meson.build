# TODO(syoyo): OSD support, audio support, tinyexr support

project('tinyusdz', 'cpp', default_options : ['cpp_std=c++11'])

incdirs = include_directories('src')
library('tinyusdz',
  'src/pxrLZ4/lz4.cpp', 'src/tinyusdz.cc',
  'src/lz4-compression.cc',
  'src/integerCoding.cpp',
  include_directories : incdirs )
