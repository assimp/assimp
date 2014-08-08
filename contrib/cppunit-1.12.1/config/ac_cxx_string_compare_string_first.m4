dnl @synopsis AC_CXX_STRING_COMPARE_STRING_FIRST
dnl
dnl If the standard library string::compare() function takes the
dnl string as its first argument, define FUNC_STRING_COMPARE_STRING_FIRST to 1.
dnl
dnl @author Steven Robbins
dnl
AC_DEFUN([AC_CXX_STRING_COMPARE_STRING_FIRST],
[AC_CACHE_CHECK(whether std::string::compare takes a string in argument 1,
ac_cv_cxx_string_compare_string_first,
[AC_REQUIRE([AC_CXX_NAMESPACES])
 AC_LANG_SAVE
 AC_LANG_CPLUSPLUS
 AC_TRY_COMPILE([#include <string>
#ifdef HAVE_NAMESPACES
using namespace std;
#endif],[string x("hi"); string y("h");
return x.compare(y,0,1) == 0;],
 ac_cv_cxx_string_compare_string_first=yes, 
 ac_cv_cxx_string_compare_string_first=no)
 AC_LANG_RESTORE
])
if test "$ac_cv_cxx_string_compare_string_first" = yes; then
  AC_DEFINE(FUNC_STRING_COMPARE_STRING_FIRST,1,
            [define if library uses std::string::compare(string,pos,n)])
fi
])
