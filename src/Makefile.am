SUBDIRS = common . testsuite

AM_CPPFLAGS = \
	-I$(top_srcdir)/lib \
	-I$(top_builddir)/lib
AM_CFLAGS = -Wall
AM_YFLAGS = -d

bin_PROGRAMS = remark cacheline
man_MANS = remark.1 cacheline.1
EXTRA_DIST = $(man_MANS)

remark_SOURCES = \
	rule.c \
	match.c \
	style.c \
	parser.y \
	parser.h \
	lexer.l \
	execute.c \
	remark.c \
	remark.h \
	wrap.c

remark_LDADD = \
	common/lib_common.a \
	../lib/libgnu.a

cacheline_SOURCES = \
	cacheline.c

cacheline_LDADD = \
	common/lib_common.a \
	../lib/libgnu.a

lexer.o: lexer.c
	$(COMPILE) -c $< -Wno-unused
