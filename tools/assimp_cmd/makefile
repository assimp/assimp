
# ---------------------------------------------------------------------------
# Makefile for assimp_cmd 
# aramis_acg@users.sourceforge.net
#
# Usage: make <target> <macros>

# TARGETS:
#   all                  Build assimp_cmd tool and assimp if necessary
#   clean                Cleanup all object files, including those from core
#   cleanme              Cleanup only my object files

# MACROS: (make clean before you change one)
#   NOBOOST=1            Build Assimp against boost workaround
#   SINGLETHREADED=1     Build Assimp single-threaded library
#   DEBUG=1              Build debug build of Assimp library
# ---------------------------------------------------------------------------


# C++ object files
OBJECTS   := $(patsubst %.cpp,%.o,  $(wildcard *.cpp)) 


# Include flags for gcc
INCLUDEFLAGS = -I../../include

# Library flags for gcc
LIBRARYFLAGS = -L../../bin/gcc/

# Preprocessor defines for gcc
DEFINEFLAGS =  

# GCC compiler flags 
CPPFLAGS=-Wall


# Setup environment for debug build
ifeq ($(DEBUG),1)
	DEFINEFLAGS   += -D_DEBUG -DDEBUG
else
	CPPFLAGS      += -o3
	DEFINEFLAGS   += -DNDEBUG -D_NDEBUG
endif

# Output path of executable
OUTPUT = ../../bin/gcc/assimp


all:	$(OBJECTS)
	cd ../../code/ && $(MAKE) static 
	gcc -s -o$(OUTPUT) $(OBJECTS) $(LIBRARYFLAGS) -lassimp -lstdc++ 

%.o:%.cpp
	$(CXX) -g -c  $(CPPFLAGS) $? -o $@ $(INCLUDEFLAGS) $(DEFINEFLAGS)

.PHONY: clean
clean:
	-del *.o
	cd ../../code/ && $(MAKE) clean

.PHONY: cleanme
cleanme:
	-del *.o