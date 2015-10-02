#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(libhomegear-base, 0.6.0, sathya@laufers.net)
AC_CONFIG_AUX_DIR(cfg)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/BaseLib.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

dnl AM_MAINTAINER_MODE

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL

# Libraries
LT_INIT

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h asm/types.h dirent.h errno.h fcntl.h gcrypt.h gnutls/gnutls.h gnutls/x509.h grp.h ifaddrs.h linux/netlink.h linux/rtnetlink.h netdb.h net/if.h netinet/ether.h netinet/in.h netinet/tcp.h poll.h pwd.h signal.h stdint.h stdio.h stdlib.h string.h sys/ioctl.h sys/resource.h sys/socket.h sys/stat.h sys/types.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_C_CONST
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_CHOWN
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_CHECK_FUNCS([floor memchr memset pow select socket strchr strerror strstr strtol])

AC_CANONICAL_HOST
case $host_os in
	darwin* )
		CPPFLAGS="$CPPFLAGS -DMACOSSYSTEM"
		;;
	linux*)
		CPPFLAGS="$CPPFLAGS -DLINUXSYSTEM"
		;;
	*BSD*)
		CPPFLAGS="$CPPFLAGS -DBSDSYSTEM -D_GLIBCXX_USE_C99 -D_GLIBCXX_USE_C99_MATH -D_GLIBCXX_USE_C99_MATH_TR1 -D_WITH_DPRINTF"
		;;
	*)
		AC_MSG_ERROR([Your platform is not currently supported])
	        ;;
esac

#AC_ARG_ENABLE(debug, AS_HELP_STRING([--enable-debug], [enable debugging, default: no]), [case "${enableval}" in yes) debug=true ;; no)  debug=false ;; *)   AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;; esac], [debug=false])
#AM_CONDITIONAL(DEBUG, test x"$debug" = x"true")

AC_OUTPUT(Makefile src/Makefile)
