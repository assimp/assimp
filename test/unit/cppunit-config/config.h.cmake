/* Inspired by config/config.h.in, config.h.cmake is used by CMake. */

/* define if library uses std::string::compare(string,pos,n) */
//Not used #undef FUNC_STRING_COMPARE_STRING_FIRST

/* define to 1 if the library defines strstream */
#cmakedefine01 CPPUNIT_HAVE_CLASS_STRSTREAM

/* Define to 1 if you have the <cmath> header file. */
#cmakedefine01 CPPUNIT_HAVE_CMATH

/* Define if you have the GNU dld library. */
//Not used, dld library is obsolete anyway #undef HAVE_DLD

/* Define to 1 if you have the `dlerror' function. */
//Not used #undef HAVE_DLERROR

/* Define to 1 if you have the <dlfcn.h> header file. */
#cmakedefine01 CPPUNIT_HAVE_DLFCN_H

/* Define to 1 if you have the `finite' function. */
#cmakedefine01 CPPUNIT_HAVE_FINITE

/* Define to 1 if you have the `_finite' function. */
#cmakedefine01 CPPUNIT_HAVE__FINITE

/* define to 1 if the compiler supports GCC C ABI name demangling */
//Not used #undef CPPUNIT_HAVE_GCC_ABI_DEMANGLE

/* Define to 1 if you have the <inttypes.h> header file. */
//Not used #undef HAVE_INTTYPES_H

/* define if compiler has isfinite */
#cmakedefine CPPUNIT_HAVE_ISFINITE

/* Define if you have the libdl library or equivalent. */
#cmakedefine CPPUNIT_HAVE_LIBDL

/* Define to 1 if you have the <memory.h> header file. */
//Not used #undef HAVE_MEMORY_H

/* define to 1 if the compiler implements namespaces */
#cmakedefine01 CPPUNIT_HAVE_NAMESPACES

/* define to 1 if the compiler supports Run-Time Type Identification */
#cmakedefine01 CPPUNIT_HAVE_RTTI

/* Define if you have the shl_load function. */
#cmakedefine CPPUNIT_HAVE_SHL_LOAD

/* define to 1 if the compiler has stringstream */
#cmakedefine01 CPPUNIT_HAVE_SSTREAM

/* Define to 1 if you have the <stdint.h> header file. */
//Not used #undef HAVE_STDINT_H

/* Define to 1 if you have the <stdlib.h> header file. */
//Not used #undef HAVE_STDLIB_H

/* Define to 1 if you have the <strings.h> header file. */
//Not used #undef HAVE_STRINGS_H

/* Define to 1 if you have the <string.h> header file. */
//Not used #undef HAVE_STRING_H

/* Define to 1 if you have the <strstream> header file. */
#cmakedefine01 CPPUNIT_HAVE_STRSTREAM

/* Define to 1 if you have the <sys/stat.h> header file. */
//Not used #undef HAVE_SYS_STAT_H

/* Define to 1 if you have the <sys/types.h> header file. */
//Not used #undef HAVE_SYS_TYPES_H

/* Define to 1 if you have the <unistd.h> header file. */
//Not used #undef HAVE_UNISTD_H

/* Name of package */
//Not used #undef PACKAGE

/* Define to the address where bug reports for this package should be sent. */
//Not used #undef PACKAGE_BUGREPORT

/* Define to the full name of this package. */
//Not used #undef PACKAGE_NAME

/* Define to the full name and version of this package. */
//Not used #undef PACKAGE_STRING

/* Define to the one symbol short name of this package. */
//Not used #undef PACKAGE_TARNAME

/* Define to the version of this package. */
//Not used #undef PACKAGE_VERSION

/* Define to 1 if you have the ANSI C header files. */
//Not used #undef STDC_HEADERS

/* Define to 1 to use type_info::name() for class names */
#cmakedefine01 CPPUNIT_USE_TYPEINFO_NAME

/* Version number of package */
#define  CPPUNIT_VERSION @CppUnit_VERSION@
