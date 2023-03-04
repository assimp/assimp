#!/bin/bash

grep \
  --include=\*.{c,cpp,h} \
  --exclude={pstdint,m3d}.h \
  -rnw include code \
  -e '^\s*printf'

if [ $? -eq 0 ]; then
  echo "Debug statement(s) detected. Please uncomment (using single-line comment), remove, or manually add to exclude filter, if appropriate" 
  exit 1
fi

