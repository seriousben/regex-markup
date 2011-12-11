#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

# Initialization
AC_PREREQ(2.59)
AC_INIT(regex-markup, 0.10.0, oskar@osk.mine.nu)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/remark.c])
AC_CONFIG_HEADER([config.h])

# Enable maintainer mode
AM_MAINTAINER_MODE

# Populate config.h
AC_GNU_SOURCE
AC_DEFINE_DIR(LOCALEDIR, datadir/locale, [Directory where gettext translations are kept.])
AC_DEFINE_DIR(PKGDATADIR, datadir/$PACKAGE, [Directory where system-wide rule files are kept.])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
#AC_PROG_MAKE_SET
AC_PROG_RANLIB
AM_PROG_LEX
AC_PROG_YACC

# Gettext
AM_GNU_GETTEXT([external])

# Gnulib
gl_MODULES([dirname dup2 free getline getopt gettimeofday lstat malloc memcpy memmove memmove memset minmax progname realloc regex stat stdbool stdint strdup strerror strerror strndup strstr strstr strtol vasprintf version-etc xalloc xalloc-die xvasprintf])
gl_EARLY
gl_INIT

# Checks for libraries.

# Checks for header files.
# POSIX: AC_HEADER_DIRENT, AC_HEADER_SYS_WAIT, AC_HEADER_TIME
AC_HEADER_DIRENT
AC_HEADER_SYS_WAIT
AC_HEADER_TIME
# POSIX: AC_CHECK_HEADERS([fcntl.h unistd.h sys/time.h sys/stat.h sys/types.h dirent.h sys/wait.h])
AC_CHECK_HEADERS([fcntl.h unistd.h sys/wait.h sys/time.h])
# always available (?): AC_CHECK_HEADERS([sys/stat.h sys/types.h])
# gnulib: AC_CHECK_HEADERS([stdint.h])
# C89: AC_CHECK_HEADERS([stddef.h stdlib.h string.h locale.h])
# C89: AC_HEADER_STDC

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
# POSIX: AC_TYPE_MODE_T
# POSIX: AC_TYPE_OFF_T
# POSIX: AC_TYPE_PID_T
# C89: AC_TYPE_SIZE_T
# C89: AC_C_CONST
# gnulib: AC_HEADER_STDBOOL
AC_SYS_LARGEFILE

# Checks for library functions.
# POSIX: AC_FUNC_CLOSEDIR_VOID
# POSIX: AC_FUNC_FORK
# C89: AC_FUNC_VPRINTF
# C89: AC_CHECK_FUNCS([strchr strrchr setlocale pow])
# gnulib: AC_FUNC_LSTAT
# gnulib: AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
# gnulib: AC_FUNC_MALLOC
# gnulib: AC_FUNC_REALLOC
# gnulib: AC_FUNC_STAT
# gnulib: AC_CHECK_FUNCS([dup2 gettimeofday memmove regcomp strdup strerror strndup strstr strtol])
AC_CHECK_FUNCS([lstat64])

AC_CONFIG_FILES([Makefile
		 po/Makefile.in
		 lib/Makefile
                 examples/Makefile
                 examples/rules/Makefile
                 src/Makefile
                 src/common/Makefile
                 src/testsuite/Makefile
		 regex-markup.spec])
AC_OUTPUT
