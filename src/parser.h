/* A Bison parser, made by GNU Bison 1.875d.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     STYLE = 258,
     PREPEND = 259,
     APPEND = 260,
     SKIP = 261,
     BREAK = 262,
     MACRO = 263,
     INCLUDE = 264,
     SET = 265,
     NUMBER = 266,
     STRING = 267,
     MATCH = 268,
     SUBST = 269
   };
#endif
#define STYLE 258
#define PREPEND 259
#define APPEND 260
#define SKIP 261
#define BREAK 262
#define MACRO 263
#define INCLUDE 264
#define SET 265
#define NUMBER 266
#define STRING 267
#define MATCH 268
#define SUBST 269




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 54 "parser.y"
typedef union YYSTYPE {
	const char *text;
	int32_t number;
	Rule *rule;
	Match *match;
	Macro *macro;
	LList *list;
	Style *style;
	StyleInfo *styleinfo;
	struct {
		const char *text;
		RegexFlags flags;
	} regex;
} YYSTYPE;
/* Line 1285 of yacc.c.  */
#line 80 "parser.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



