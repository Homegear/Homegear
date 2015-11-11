#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(homegear, m4_esyscmd_s([./getVersion.sh]), sathya@laufers.net)
AC_CONFIG_AUX_DIR(cfg)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/main.cpp])
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
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h memory.h netdb.h netinet/in.h stddef.h stdlib.h string.h sys/file.h sys/param.h sys/socket.h sys/time.h sys/timeb.h syslog.h unistd.h])

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
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([clock_gettime ftime getcwd gettimeofday memmove memset mkdir rmdir select setenv socket strchr strerror strrchr strstr])
AM_PROG_CC_C_O

AM_CONDITIONAL(MACOSSYSTEM, false)
AM_CONDITIONAL(LINUXSYSTEM, false)
AM_CONDITIONAL(BSDSYSTEM, false)

AC_CANONICAL_HOST
case $host_os in
	darwin* )
		CPPFLAGS="$CPPFLAGS -DMACOSSYSTEM"
		AM_CONDITIONAL(MACOSSYSTEM, true)
		;;
	linux*)
		CPPFLAGS="$CPPFLAGS -DLINUXSYSTEM"
		AM_CONDITIONAL(LINUXSYSTEM, true)
		;;
	*BSD*)
		CPPFLAGS="$CPPFLAGS -DBSDSYSTEM -D_GLIBCXX_USE_C99 -D_GLIBCXX_USE_C99_MATH -D_GLIBCXX_USE_C99_MATH_TR1 -D_WITH_DPRINTF"
		AM_CONDITIONAL(BSDSYSTEM, true)
		;;
	*)
	AC_MSG_ERROR([Your platform is not currently supported])
        ;;
esac

AC_ARG_WITH([scriptengine], [AS_HELP_STRING([--without-scriptengine], [Compile without script engine (removes the requirement for PHP)])], [with_scriptengine=no], [])
AS_IF([test "x$with_scriptengine" = "xno"], [
	AM_CONDITIONAL(WITH_SCRIPTENGINE, false)
	], [
	AM_CONDITIONAL(WITH_SCRIPTENGINE, true)
	AC_DEFINE(SCRIPTENGINE, [], [Enables compilation of the PHP script engine])
	])

AC_ARG_WITH([eventhandler], [AS_HELP_STRING([--without-eventhandler], [Compile without event handler])], [with_eventhandler=no], [])
AS_IF([test "x$with_eventhandler" != "xno"], [	
	AC_DEFINE(EVENTHANDLER, [], [Enables compilation of Homegear's event handler])
	])

AC_ARG_WITH([openssl], [AS_HELP_STRING([--without-openssl], [Compile without OpenSSL (disables SSL in MQTT)])], [with_openssl=no], [])
AS_IF([test "x$with_openssl" = "xno"], [
	AM_CONDITIONAL(WITH_OPENSSL, false)
	], [
	AM_CONDITIONAL(WITH_OPENSSL, true)
	AC_DEFINE(OPENSSL, [], [Enables OpenSSL support in MQTT])
	])

AC_OUTPUT(Makefile src/Makefile homegear-miscellaneous/src/Makefile)
