#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([urbackupserver_bdbplugin], [0.1], [martin@urbackup.org])
AC_CONFIG_SRCDIR([dllmain.cpp])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

AC_ARG_ENABLE([fortify],
     AS_HELP_STRING([--disable-fortify], [Disable GCC/runtime fortification]))
AM_CONDITIONAL(WITH_FORTIFY, test "x$enable_fortify" != xno)

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for libraries.

#AX_BERKELEY_DB(5.1,, AC_MSG_ERROR([Sorry, you need BerkleyDB. Please look for distribution packages(libdb-dev) or get it from http://www.oracle.com/technetwork/database/berkeleydb/]))
AX_BERKELEY_DB_SQL(5.2,, [
    AC_MSG_ERROR([Sorry, you need BerkleyDB with SQL support. Please look for distribution packages(libdb-sql-dev) or get it from http://www.oracle.com/technetwork/database/berkeleydb/])
])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
