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
AC_CHECK_FUNCS([clock_gettime ftime getcwd gettimeofday memmove memset mkdir rmdir select setenv socket strchr strerror strrchr strstr])
AM_PROG_CC_C_O

AM_CONDITIONAL(MACOSSYSTEM, false)
AM_CONDITIONAL(LINUXSYSTEM, false)
AM_CONDITIONAL(BSDSYSTEM, false)

AC_CANONICAL_HOST
case $host_os in
	darwin*)
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

# Check for libraries
AC_CHECK_LIB([mysqlclient], [mysql_init], [AM_CONDITIONAL(HAVE_MYSQLCLIENT, true)], [AM_CONDITIONAL(HAVE_MYSQLCLIENT, false)])
AC_CHECK_HEADERS([curl/curl.h], [AM_CONDITIONAL(HAVE_CURL_HEADERS, true)], [AM_CONDITIONAL(HAVE_CURL_HEADERS, false)])
AC_CHECK_LIB([curl], [curl_easy_send], [AM_CONDITIONAL(HAVE_CURL, true)], [AM_CONDITIONAL(HAVE_CURL, false)])
AC_CHECK_HEADERS([php7-homegear/main/php_config.h], [AM_CONDITIONAL(HAVE_PHP_HEADERS, true)], [AM_CONDITIONAL(HAVE_PHP_HEADERS, false)])

AC_DEFINE_UNQUOTED([GITCOMMITSHABASE], [`./getGitCommitBase.sh`], [Define git commit SHA of libhomegear-base])
AC_DEFINE_UNQUOTED([GITCOMMITSHAHOMEGEAR], [`./getGitCommitHomegear.sh`], [Define git commit SHA of Homegear])
AC_DEFINE_UNQUOTED([GITBRANCHBASE], [`./getGitBranchBase.sh`], [Define git branch of libhomegear-base])
AC_DEFINE_UNQUOTED([GITBRANCHHOMEGEAR], [`./getGitBranchHomegear.sh`], [Define git branch of Homegear])

AC_ARG_WITH([scriptengine], [AS_HELP_STRING([--without-scriptengine], [Compile without script engine (removes the requirement for PHP)])], [with_scriptengine=no], [])
AS_IF([test "x$with_scriptengine" = "xno"], [
	AM_CONDITIONAL(WITH_SCRIPTENGINE, false)
	AC_DEFINE(NO_SCRIPTENGINE, [], [Disables compilation of the PHP script engine])
	], [
	AM_CONDITIONAL(WITH_SCRIPTENGINE, true)
	AC_DEFINE(SCRIPTENGINE, [], [Enables compilation of the PHP script engine])
	])

AC_ARG_WITH([eventhandler], [AS_HELP_STRING([--without-eventhandler], [Compile without event handler])], [with_eventhandler=no], [])
AS_IF([test "x$with_eventhandler" != "xno"], [
	AC_DEFINE(EVENTHANDLER, [], [Enables compilation of Homegear's event handler])
	])

AC_ARG_WITH([ccu2], [AS_HELP_STRING([--with-ccu2], [Compile for CCU2])], [with_ccu2=yes], [])
AS_IF([test "x$with_ccu2" = "xyes"], [
	AC_DEFINE(CCU2, [], [Enables features specific for CCU2])
	])

#AC_ARG_WITH([curl], [AS_HELP_STRING([--with-curl], [Use this option when PHP is compiled with --with-curl.])], [with_openssl=yes], [])
#AS_IF([test "x$with_openssl" = "xyes"], [
#	AM_CONDITIONAL(WITH_CURL, true)
#	], [
#	AM_CONDITIONAL(WITH_CURL, false)
#	])

AC_ARG_WITH([i2c], [AS_HELP_STRING([--without-i2c], [Compile without I2C support (disables all I2C interfaces)])], [with_i2c=no], [])
AS_IF([test "x$with_i2c" != "xno"], [
	AC_DEFINE(I2CSUPPORT, [], [Enables compilation of all I2C interfaces and I2C support in PHP])
	])

AC_OUTPUT(Makefile src/Makefile homegear-miscellaneous/src/Makefile)
