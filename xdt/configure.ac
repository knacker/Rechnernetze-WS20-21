dnl Process this file with autoconf to produce a configure script.

AC_INIT(README)

FLAVOUR="`cat FLAVOUR`"
PACKAGE="xdt_$FLAVOUR"
VERSION=`date "+%Y%m%d"`

AM_INIT_AUTOMAKE($PACKAGE, $VERSION)
AM_CONFIG_HEADER(config.h)

AC_ARG_ENABLE(warnings-as-error,
[  --enable-warnings-as-error                	warnings will result in an error],
[case "${enableval}" in
  yes) warn_no_error=false ;;
  no) warn_no_error=true;;
  *) AC_MSG_ERROR(bad value ${enableval} for --enable-warnings-as-error) ;;
esac],[warn_no_error=true])
AM_CONDITIONAL(WARN_NO_ERROR, test x$warn_no_error = xtrue)

AC_ARG_ENABLE(warnings,
[  --disable-warnings                        	no warnings at compile time],
[case "${enableval}" in
  yes) no_warnings=false ;;
  no) no_warnings=true ;;
  *) AC_MSG_ERROR(bad value ${enableval} for --disable-warnings) ;;
esac],[no_warnings=false])
AM_CONDITIONAL(NO_WARNINGS, test x$no_warnings = xtrue)

AC_ARG_ENABLE(profiling,
[  --enable-profiling                    	compile with profile support],
[case "${enableval}" in
  yes) profiling=true ;;
  no) profiling=false ;;
  *) AC_MSG_ERROR(bad value ${enableval} for --enable-profiling) ;;
esac],[profiling=false])

AC_ARG_ENABLE(debug,
[  --disable-debug				disable debug code],
[case "${enableval}" in
  yes) no_debug=false ;;
  no) no_debug=true ;;
  *) AC_MSG_ERROR(bad value ${enableval} for --disable-debug) ;;
esac],[no_debug=false])

dnl Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_RANLIB
AM_PROG_CC_C_O

# use debug code or not
if [[ x$no_debug = xtrue ]]; then
  CFLAGS="$CFLAGS -DNDEBUG"
fi

# compile in profiling information or not
if [[ "$profiling" = "true" ]]; then
  CFLAGS="$CFLAGS -pg -fprofile-arcs"
  LDFLAGS="$LDFLAGS -pg"
fi

# set warning level for GCC
WDEF="-W -Wall -Wmissing-prototypes -Wstrict-prototypes"
WEXT="-Wundef -Wbad-function-cast -Wcast-qual -Wcast-align -Wmissing-declarations -Wredundant-decls -Wnested-externs"
WSUP="-Wshadow -Wconversion"
WUNS="-Wfloat-equal" # not supported by gcc 2.x
if [[ "x$GCC" = "xyes" ]]; then
  if [[ "x$no_warnings" = "xfalse" ]]; then
    CFLAGS="$CFLAGS $WDEF $WEXT"
    if [[ "x$warn_no_error" = "xfalse" ]]; then
      CFLAGS="$CFLAGS -Werror"
    fi
  fi
fi

dnl Checks for libraries.
AC_CHECK_LIB(nsl, gethostname)
AC_CHECK_LIB(socket, socket)
AC_CHECK_LIB(posix4, nanosleep)
AC_CHECK_LIB(rt, nanosleep)

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
#AC_REPLACE_FUNCS(strerror)
	
# Check for PF_LOCAL/AF_LOCAL
# found here: http://cvs.sourceforge.net/viewcvs.py/ggz/ggzd/configure.ac?rev=1.13
AC_MSG_CHECKING([for PF_LOCAL in <sys/socket.h>])
AC_EGREP_CPP(PF_LOCAL,
             [#include <sys/socket.h>
              PF_LOCAL ],
             [AC_MSG_RESULT(no)
              AC_MSG_CHECKING([for PF_UNIX])
              AC_EGREP_CPP(PF_UNIX, [ #include <sys/socket.h>
				      PF_UNIX ],
                           [AC_MSG_RESULT(no)
			    AC_MSG_ERROR([cannot find PF_LOCAL or PF_UNIX])],
                           [AC_MSG_RESULT(yes)
                            AC_DEFINE([PF_LOCAL], PF_UNIX,
				      [PF_UNIX is available])
			    AC_DEFINE([AF_LOCAL], AF_UNIX,
				      [AF_UNIX is available])])], 
             AC_MSG_RESULT(yes))

	
# files to create
AC_OUTPUT(Makefile src/Makefile src/xdt/Makefile src/service/Makefile src/user/Makefile src/examples/Makefile)
