/*                                                                -*- C -*-
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2007 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Stig Sæther Bakken <ssb@php.net>                             |
   +----------------------------------------------------------------------+
*/

/* $Id$ */

#define CONFIGURE_COMMAND " './configure'  '--with-libdir=lib/x86_64-linux-gnu' '--prefix=/var/lib/homegear/modules/php' '--enable-embed=shared' '--with-config-file-path=/etc/homegear' '--sysconfdir=/etc' '--localstatedir=/var' '--mandir=/var/lib/homegear/modules/php/man' '--disable-debug' '--with-regex=php' '--disable-rpath' '--disable-static' '--with-pic' '--with-layout=GNU' '--with-pear=/var/lib/homegear/modules/php' '--enable-calendar' '--enable-sysvsem' '--enable-sysvshm' '--enable-sysvmsg' '--enable-bcmath' '--with-bz2' '--enable-ctype' '--without-gdbm' '--with-iconv' '--enable-exif' '--enable-ftp' '--with-gettext' '--enable-mbstring' '--with-pcre-regex=/usr' '--enable-shmop' '--enable-sockets' '--enable-wddx' '--with-libxml-dir=/usr' '--with-zlib' '--with-kerberos=/usr' '--with-openssl=/usr' '--enable-soap' '--enable-zip' '--with-mhash=yes' '--with-mysql-sock=/var/run/mysqld/mysqld.sock' '--enable-dtrace' '--without-mm' '--with-curl=shared,/usr' '--with-enchant=shared,/usr' '--with-zlib-dir=/usr' '--with-gd=shared,/usr' '--enable-gd-native-ttf' '--with-gmp=shared,/usr' '--with-jpeg-dir=shared,/usr' '--with-xpm-dir=shared,/usr/X11R6' '--with-png-dir=shared,/usr' '--with-freetype-dir=shared,/usr' '--with-vpx-dir=shared,/usr' '--enable-intl=shared' '--without-t1lib' '--with-ldap=shared,/usr' '--with-ldap-sasl=/usr' '--with-mysql=shared,/usr' '--with-mysqli=shared,/usr/bin/mysql_config' '--with-pspell=shared,/usr' '--with-unixODBC=shared,/usr' '--with-recode=shared,/usr' '--with-xsl=shared,/usr' '--with-snmp=shared,/usr' '--with-sqlite3=shared,/usr' '--with-pdo-sqlite=shared,/usr' '--with-mssql=shared,/usr' '--with-tidy=shared,/usr' '--with-xmlrpc=shared,/usr' '--with-libxml-dir=/usr/lib/x86_64-linux-gnu' '--with-pgsql=shared,/usr' '--enable-maintainer-zts' 'LDFLAGS=-L/usr/lib/x86_64-linux-gnu'"
#define PHP_ADA_INCLUDE		""
#define PHP_ADA_LFLAGS		""
#define PHP_ADA_LIBS		""
#define PHP_APACHE_INCLUDE	""
#define PHP_APACHE_TARGET	""
#define PHP_FHTTPD_INCLUDE      ""
#define PHP_FHTTPD_LIB          ""
#define PHP_FHTTPD_TARGET       ""
#define PHP_CFLAGS		"$(CFLAGS_CLEAN) -prefer-pic"
#define PHP_DBASE_LIB		""
#define PHP_BUILD_DEBUG		""
#define PHP_GDBM_INCLUDE	""
#define PHP_IBASE_INCLUDE	""
#define PHP_IBASE_LFLAGS	""
#define PHP_IBASE_LIBS		""
#define PHP_IFX_INCLUDE		""
#define PHP_IFX_LFLAGS		""
#define PHP_IFX_LIBS		""
#define PHP_INSTALL_IT		"$(mkinstalldirs) $(INSTALL_ROOT)$(prefix)/lib; $(INSTALL) -m 0755 libs/libphp5.so $(INSTALL_ROOT)$(prefix)/lib"
#define PHP_IODBC_INCLUDE	""
#define PHP_IODBC_LFLAGS	""
#define PHP_IODBC_LIBS		""
#define PHP_MSQL_INCLUDE	""
#define PHP_MSQL_LFLAGS		""
#define PHP_MSQL_LIBS		""
#define PHP_MYSQL_INCLUDE	"-I/usr/include/mysql"
#define PHP_MYSQL_LIBS		"-L/usr/lib/x86_64-linux-gnu -lmysqlclient_r "
#define PHP_MYSQL_TYPE		"external"
#define PHP_ODBC_INCLUDE	"-I/usr/include"
#define PHP_ODBC_LFLAGS		"-L/usr/lib/x86_64-linux-gnu"
#define PHP_ODBC_LIBS		"-lodbc"
#define PHP_ODBC_TYPE		"unixODBC"
#define PHP_OCI8_SHARED_LIBADD 	""
#define PHP_OCI8_DIR			""
#define PHP_OCI8_ORACLE_VERSION		""
#define PHP_ORACLE_SHARED_LIBADD 	"@ORACLE_SHARED_LIBADD@"
#define PHP_ORACLE_DIR				"@ORACLE_DIR@"
#define PHP_ORACLE_VERSION			"@ORACLE_VERSION@"
#define PHP_PGSQL_INCLUDE	""
#define PHP_PGSQL_LFLAGS	""
#define PHP_PGSQL_LIBS		""
#define PHP_PROG_SENDMAIL	""
#define PHP_SOLID_INCLUDE	""
#define PHP_SOLID_LIBS		""
#define PHP_EMPRESS_INCLUDE	""
#define PHP_EMPRESS_LIBS	""
#define PHP_SYBASE_INCLUDE	""
#define PHP_SYBASE_LFLAGS	""
#define PHP_SYBASE_LIBS		""
#define PHP_DBM_TYPE		""
#define PHP_DBM_LIB		""
#define PHP_LDAP_LFLAGS		""
#define PHP_LDAP_INCLUDE	""
#define PHP_LDAP_LIBS		""
#define PHP_BIRDSTEP_INCLUDE     ""
#define PHP_BIRDSTEP_LIBS        ""
#define PEAR_INSTALLDIR         "/var/lib/homegear/modules/php"
#define PHP_INCLUDE_PATH	".:/var/lib/homegear/modules/php"
#define PHP_EXTENSION_DIR       "/var/lib/homegear/modules/php/lib/php/20131226-zts"
#define PHP_PREFIX              "/var/lib/homegear/modules/php"
#define PHP_BINDIR              "/var/lib/homegear/modules/php/bin"
#define PHP_SBINDIR             "/var/lib/homegear/modules/php/sbin"
#define PHP_MANDIR              "/var/lib/homegear/modules/php/man"
#define PHP_LIBDIR              "/var/lib/homegear/modules/php/lib/php"
#define PHP_DATADIR             "/var/lib/homegear/modules/php/share/php"
#define PHP_SYSCONFDIR          "/etc"
#define PHP_LOCALSTATEDIR       "/var"
#define PHP_CONFIG_FILE_PATH    "/etc/homegear"
#define PHP_CONFIG_FILE_SCAN_DIR    ""
#define PHP_SHLIB_SUFFIX        "so"
