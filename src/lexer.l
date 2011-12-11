/* lexer.y - Lexer for the rule files.
 *
 * Copyright (C) 2001-2005 Oskar Liljeblad
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

%top{
#include <config.h>
/* POSIX */
#include <stdint.h>
/* gnulib */
#include <xalloc.h>
/* common */
#include "common/intutil.h"
#include "common/strbuf.h"
/* regex-markup */
#include "remark.h"
#include "parser.h"

static RemarkFile *file;

static void fix_reflags(void);
static void fix_string(bool retain_backslashes, int skip_count, char endchar);
static void set_string(char *str);

}

ESCSTR (\\.|[^\\\"])*
ESCRE (\\.|[^\\/])*
REFLGS [ig]*
%option yylineno
%option noyywrap

%%

[[:space:]]+
"#"[^\n]*\n
[{},]					return yytext[0];
"\""{ESCSTR}"\""		fix_string(false, 0, '"'); return STRING;
"/"{ESCRE}"/"{REFLGS}	fix_reflags(); fix_string(true, 0, '/'); return MATCH;
"s/"{ESCRE}"/"			fix_string(true, 1, '/'); unput('/'); return SUBST;
[0-9]+					parse_int32(yytext, &yylval.number); return NUMBER;
"macro"					return MACRO;
"style"					return STYLE;
"skip"					return SKIP;
"break"					return BREAK;
"pre"|"prepend"			return PREPEND;
"post"|"append"			return APPEND;
"include"				return INCLUDE;
"set"					return SET;
[^[:space:],{}/\"]+		set_string(xstrdup(yytext)); return STRING;
<<EOF>>					return EOF;

%%

static void
fix_reflags(void)
{
	int c;

	yylval.regex.flags = 0;
	for (c = strlen(yytext)-1; yytext[c] != '/'; c--) {
		if (yytext[c] == 'i') {
			yylval.regex.flags |= REGEX_IGNORE_CASE;
		} else if (yytext[c] == 'g') {
			yylval.regex.flags |= REGEX_GLOBAL;
		}
	}
	yytext[c+1] = '\0';
}

static void
fix_string(bool retain_backslashes, int skip_count, char endchar)
{
	StrBuf *out = strbuf_new();
	int c;

	for (c = 1+skip_count; yytext[c] != endchar; c++) {
		char ch = yytext[c];

		if (ch == '\\' && c+1 < yyleng) {
			ch = yytext[++c];
			if (retain_backslashes) {
				strbuf_append_char(out, '\\');
			} else {
				switch (ch) {
				case 'a': ch = '\a'; break;
				case 'b': ch = '\b'; break;
				case 't': ch = '\t'; break;
				case 'n': ch = '\n'; break;
				case 'v': ch = '\v'; break;
				case 'f': ch = '\f'; break;
				case 'r': ch = '\r'; break;
				case 'e':
				case 'E': ch = 27; break;
				}
			}
		}

		strbuf_append_char(out, ch);
	}

	set_string(strbuf_free_to_string(out));
}

static void
set_string(char *str)
{
	yylval.text = str;
	llist_add(file->tokens, str);
	yyleng = 0;
	yytext[0] = '\0';
}

void
lexer_set_buffer(RemarkFile *rf)
{
	yyin = rf->file;
	file = rf;
	yy_switch_to_buffer(yy_create_buffer(rf->file, YY_BUF_SIZE));
	rf->lex_buffer = YY_CURRENT_BUFFER;
}

void
lexer_restore_buffer(RemarkFile *rf)
{
	file = rf;
	yy_delete_buffer(YY_CURRENT_BUFFER);
	if (rf != NULL)
		yy_switch_to_buffer(rf->lex_buffer);
}
