#!/bin/bash

grep \
  --include=\*.{c,cpp,h} \
  --exclude={pstdint,m3d}.h \
  -rnw include code \
  -e '^\s*printf'

if [ $? ]
then
  echo "Debug statement(s) detected. Please remove, or manually add to exclude filter, if appropriate" 
  exit 1
fi

