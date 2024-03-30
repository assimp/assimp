#if defined(TINYEXR_USE_MINIZ) && (!TINYEXR_USE_MINIZ)
// Use system's zlib
#include <zlib.h>
#endif

#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"
