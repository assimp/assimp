#!/bin/sh

PATHS="include code"
FILTER_INCLUDE='\*.{c,cpp,h}'
FILTER_EXCLUDE="{include/assimp/Compiler/pstdint.h,code/AssetLib/M3D/m3d.h}"

PATTERN='^\s*printf'

grep \
  --include=\*.{c,cpp,h} \
  --exclude={include/assimp/Compiler/pstdint.h,code/AssetLib/M3D/m3d.h} \
  -rnw include code \
  -e '^\s*printf'

if [ $? ]
then
  echo "Debug statement(s) detected. Please remove, or manually add to exclude filter, if appropriate" 
  exit 1
fi

