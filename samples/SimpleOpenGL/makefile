
# ---------------------------------------------------------------------------
# Makefile for assimp/Sample_SimpleOpenGL
# aramis_acg@users.sourceforge.net
#
# Usage: make <target> <macros>

# TARGETS:
#   all                  Build the sample + assimp itself
#   clean                Cleanup all object files
# ---------------------------------------------------------------------------

VPATH = ./usr/include/
# Include flags for gcc
INCLUDEFLAGS = -I../../include 

# Library flags for gcc
LIBFLAGS = -L../../bin/gcc

# Output name of executable
OUTPUT = samplegl

all:	$(OBJECTS)
	cd ../../code/ && $(MAKE) static 
	gcc -o $(OUTPUT) $(INCLUDEFLAGS) -s Sample_SimpleOpenGL.c $(LIBFLAGS) -Wl,-Bstatic -lassimp -Wl,-Bdynamic -lstdc++ -lglut
.PHONY: clean
clean:
	-rm *.o
