
# AC_LTDL_DLLIB
# -------------
AC_DEFUN([AC_LTDL_DLLIB],
[LIBADD_DL=
AC_SUBST(LIBADD_DL)

AC_CHECK_FUNC([shl_load],
      [AC_DEFINE([HAVE_SHL_LOAD], [1],
		 [Define if you have the shl_load function.])],
  [AC_CHECK_LIB([dld], [shl_load],
	[AC_DEFINE([HAVE_SHL_LOAD], [1],
		   [Define if you have the shl_load function.])
	LIBADD_DL="$LIBADD_DL -ldld"],
    [AC_CHECK_LIB([dl], [dlopen],
	  [AC_DEFINE([HAVE_LIBDL], [1],
		     [Define if you have the libdl library or equivalent.])
	  LIBADD_DL="-ldl"],
      [AC_TRY_LINK([#if HAVE_DLFCN_H
#  include <dlfcn.h>
#endif
      ],
	[dlopen(0, 0);],
	    [AC_DEFINE([HAVE_LIBDL], [1],
		       [Define if you have the libdl library or equivalent.])],
	[AC_CHECK_LIB([svld], [dlopen],
	      [AC_DEFINE([HAVE_LIBDL], [1],
			 [Define if you have the libdl library or equivalent.])
	      LIBADD_DL="-lsvld"],
	  [AC_CHECK_LIB([dld], [dld_link],
	        [AC_DEFINE([HAVE_DLD], [1],
			   [Define if you have the GNU dld library.])
	 	LIBADD_DL="$LIBADD_DL -ldld"
          ])
        ])
      ])
    ])
  ])
])

if test "x$ac_cv_func_dlopen" = xyes || test "x$ac_cv_lib_dl_dlopen" = xyes; then
 LIBS_SAVE="$LIBS"
 LIBS="$LIBS $LIBADD_DL"
 AC_CHECK_FUNCS(dlerror)
 LIBS="$LIBS_SAVE"
fi
])# AC_LTDL_DLLIB
