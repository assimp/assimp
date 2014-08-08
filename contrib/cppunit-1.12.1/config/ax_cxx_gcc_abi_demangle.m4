dnl @synopsis AX_CXX_GCC_ABI_DEMANGLE
dnl
dnl If the compiler supports GCC C++ ABI name demangling (has header cxxabi.h 
dnl and abi::__cxa_demangle() function), define HAVE_GCC_ABI_DEMANGLE
dnl
dnl Adapted from AC_CXX_RTTI by Luc Maisonobe
dnl
dnl @version $Id: ax_cxx_gcc_abi_demangle.m4,v 1.1 2004/02/18 20:45:36 blep Exp $
dnl @author Neil Ferguson <nferguso@eso.org>
dnl
AC_DEFUN([AX_CXX_GCC_ABI_DEMANGLE],
[AC_CACHE_CHECK(whether the compiler supports GCC C++ ABI name demangling, 
ac_cv_cxx_gcc_abi_demangle,
[AC_LANG_SAVE
 AC_LANG_CPLUSPLUS
 AC_TRY_COMPILE([#include <typeinfo>
#include <cxxabi.h>
#include <string>

template<typename TYPE>
class A {};
],[A<int> instance;
int status = 0;
char* c_name = 0;

c_name = abi::__cxa_demangle(typeid(instance).name(), 0, 0, &status);
        
std::string name(c_name);
free(c_name);

return name == "A<int>";
],
 ac_cv_cxx_gcc_abi_demangle=yes, ac_cv_cxx_gcc_abi_demangle=no)
 AC_LANG_RESTORE
])
if test "$ac_cv_cxx_gcc_abi_demangle" = yes; then
  AC_DEFINE(HAVE_GCC_ABI_DEMANGLE,1,
            [define if the compiler supports GCC C++ ABI name demangling]) 
fi
])
