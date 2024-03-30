project('fuzz_tinyusdz', 'c', 'cpp', default_options : ['cpp_std=c++14'])

tinyusdz_sources=[
  '../../src/asset-resolution.cc',
  '../../src/lz4/lz4.c',
  '../../src/tinyusdz.cc',
  '../../src/lz4-compression.cc',
  '../../src/integerCoding.cpp',
  '../../src/io-util.cc',
  '../../src/str-util.cc',
  '../../src/pprinter.cc',
  '../../src/performance.cc',
  '../../src/prim-types.cc',
  '../../src/prim-reconstruct.cc',
  '../../src/usdGeom.cc',
  '../../src/usdShade.cc',
  '../../src/usdSkel.cc',
  '../../src/usdLux.cc',
  '../../src/usdObj.cc',
  '../../src/primvar.cc',
  '../../src/path-util.cc',
  '../../src/ascii-parser.cc',
  '../../src/ascii-parser-basetype.cc',
  '../../src/ascii-parser-timesamples.cc',
  '../../src/ascii-parser-timesamples-array.cc',
  '../../src/usda-reader.cc',
  '../../src/usda-writer.cc',
  '../../src/usdc-reader.cc',
  '../../src/usdc-writer.cc',
  '../../src/crate-reader.cc',
  '../../src/crate-format.cc',
  '../../src/crate-pprint.cc',
  '../../src/value-types.cc',
  '../../src/value-pprint.cc',
  '../../src/image-loader.cc',
  '../../src/image-writer.cc',
  '../../src/image-util.cc',
  '../../src/linear-algebra.cc',
  '../../src/xform.cc',
  '../../src/stage.cc',
  '../../src/tiny-format.cc',
  '../../src/tydra/render-data.cc',
  '../../src/tydra/prim-apply.cc',
  '../../src/tydra/shader-network.cc',
  '../../src/tydra/scene-access.cc',
  # deps
  '../../src/external/fpng.cpp',
  '../../src/external/pystring.cpp',
  #'../../src/external/string_id/string_id.cpp',
  #'../../src/external/string_id/error.cpp',
  #'../../src/external/string_id/database.cpp',
  ]

incdirs = include_directories(['../../src',
  #'../../src/external/fast_float/include',
  #'../../src/external/simple_match/include',
  ])

# TINYUSDZ_FUZZER_BUILD = suppress some debug message
executable('fuzz_tinyusdz',
  'tinyusdz_fuzzmain.cc',
  tinyusdz_sources, 
  install: true,
  include_directories : incdirs,
  cpp_args : ['-fsanitize=address,fuzzer', '-DTINYUSDZ_FUZZER_BUILD=1'],
  link_args : '-fsanitize=address,fuzzer' )

executable('fuzz_pathutil',
  'pathutil_fuzzmain.cc',
  tinyusdz_sources, 
  install: true,
  include_directories : incdirs,
  cpp_args : ['-fsanitize=address,fuzzer', '-DTINYUSDZ_FUZZER_BUILD=1'],
  link_args : '-fsanitize=address,fuzzer' )

executable('fuzz_intcoding_decompress',
  '../../src/lz4/lz4.c',
  'intCoding_decompress_fuzzmain.cc',
  '../../src/lz4-compression.cc',
  '../../src/integerCoding.cpp',
  install: true,
  include_directories : incdirs,
  cpp_args : ['-fsanitize=address,fuzzer', '-DTINYUSDZ_FUZZER_BUILD=1'],
  link_args : '-fsanitize=address,fuzzer' )

executable('fuzz_lz4_decompress',
  '../../src/lz4/lz4.c',
  'lz4_decompress_fuzzmain.cc',
  '../../src/lz4-compression.cc',
  install: true,
  include_directories : incdirs,
  cpp_args : ['-fsanitize=address,fuzzer', '-DTINYUSDZ_FUZZER_BUILD=1'],
  link_args : '-fsanitize=address,fuzzer' )


executable('fuzz_usdaparser',
  'usdaparser_fuzzmain.cc',
  tinyusdz_sources, 
  install: true,
  include_directories : incdirs,
  cpp_args : ['-fsanitize=address,fuzzer', '-DTINYUSDZ_FUZZER_BUILD=1'],
  link_args : '-fsanitize=address,fuzzer' )

executable('fuzz_usdcparser',
  'usdcparser_fuzzmain.cc',
  tinyusdz_sources, 
  install: true,
  include_directories : incdirs,
  cpp_args : ['-fsanitize=address,fuzzer', '-DTINYUSDZ_FUZZER_BUILD=1'],
  link_args : '-fsanitize=address,fuzzer' )

executable('fuzz_dr_mp3_decoding',
  'dr_mp3_decoding_fuzzmain.cc',
  install: true,
  include_directories : incdirs,
  cpp_args : ['-fsanitize=address,fuzzer', '-DTINYUSDZ_FUZZER_BUILD=1'],
  link_args : '-fsanitize=address,fuzzer' )

executable('fuzz_dr_wav_decoding',
  'dr_wav_decoding_fuzzmain.cc',
  install: true,
  include_directories : incdirs,
  cpp_args : ['-fsanitize=address,fuzzer', '-DTINYUSDZ_FUZZER_BUILD=1'],
  link_args : '-fsanitize=address,fuzzer' )

executable('fuzz_wuffs_jpeg_decoding',
  'wuffs_jpeg_decoding_fuzzmain.cc',
  install: true,
  include_directories : incdirs,
  cpp_args : ['-fsanitize=address,fuzzer', '-DTINYUSDZ_FUZZER_BUILD=1'],
  link_args : '-fsanitize=address,fuzzer' )

executable('fuzz_alac_decoding',
  'alac_decoding_fuzzmain.cc',
  '../../src/external/alac/codec/EndianPortable.c',
  '../../src/external/alac/codec/ALACBitUtilities.c',
  '../../src/external/alac/codec/ALACDecoder.cpp',
  '../../src/external/alac/codec/ALACEncoder.cpp',
  '../../src/external/alac/codec/ag_dec.c',
  '../../src/external/alac/codec/ag_enc.c',
  '../../src/external/alac/codec/dp_dec.c',
  '../../src/external/alac/codec/dp_enc.c',
  '../../src/external/alac/codec/matrix_dec.c',
  '../../src/external/alac/codec/matrix_enc.c',
  install: true,
  include_directories : incdirs,
  cpp_args : ['-fsanitize=address,fuzzer', '-DTINYUSDZ_FUZZER_BUILD=1'],
  link_args : '-fsanitize=address,fuzzer' )
