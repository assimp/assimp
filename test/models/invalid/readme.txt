
*********************************************************
GENERAL
*********************************************************


The files in this directory are invalid ... some of them are empty, 
others have invalid vertices or faces, others are prepared to make
 assimp allocate a few hundreds gigs of memory ... most are 
actually regression tests, i.e. there was once a bugfix that
fixed the respective loaders.

This test case is successful if the library (and the viewer) don't
crash.


*********************************************************
FILES
*********************************************************

OutOfMemory.off - the number of faces is invalid. There won't be 
  enough memory so std::vector::reserve() will most likely fail. 
  The exception should be caught in Importer.cpp.

empty.<x> - These files are completely empty. The corresponding
   loaders should not crash.

malformed.obj - out-of-range vertex indices
malformed2.obj - non-existent material referenced


