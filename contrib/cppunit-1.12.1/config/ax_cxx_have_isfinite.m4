dnl @synopsis AX_CXX_HAVE_ISFINITE
dnl
dnl If isfinite() is available to the C++ compiler:
dnl   define HAVE_ISFINITE
dnl   add "-lm" to LIBS
dnl
AC_DEFUN([AX_CXX_HAVE_ISFINITE],
  [ax_cxx_have_isfinite_save_LIBS=$LIBS
   LIBS="$LIBS -lm"

   AC_CACHE_CHECK(for isfinite, ax_cv_cxx_have_isfinite,
    [AC_LANG_SAVE
     AC_LANG_CPLUSPLUS
     AC_LINK_IFELSE(
       [AC_LANG_PROGRAM(
         [[#include <math.h>]],
         [[int f = isfinite( 3 );]])],
       [ax_cv_cxx_have_isfinite=yes],
       [ax_cv_cxx_have_isfinite=no])
     AC_LANG_RESTORE])

   if test "$ax_cv_cxx_have_isfinite" = yes; then
     AC_DEFINE([HAVE_ISFINITE],1,[define if compiler has isfinite])
   else
     LIBS=$ax_cxx_have_isfinite_save_LIBS
   fi
])
