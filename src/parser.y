/* parser.y - The yacc/bison parsing engine for the rule files.
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
%{

#include <config.h>
/* C89 */
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
/* Gettext */
#include <gettext.h>
#define _(String) gettext(String)
/* gnulib */
#include <dirname.h>
#include <xalloc.h>
#include <progname.h>
/* common */
#include "common/string-utils.h"
#include "common/intutil.h"
#include "common/error.h"
#include "common/strbuf.h"
#include "common/llist.h"
#include "common/hmap.h"
/* regex-markup */
#include "remark.h"

static RemarkFile *file = NULL;
static RemarkScript *script;

static Rule *include_script(const char *filename);
static void script_die(const char *msg, ...);
static void yyerror(char *msg);

%}

%union {
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
}

%token STYLE PREPEND APPEND SKIP BREAK MACRO INCLUDE SET
%token NUMBER STRING MATCH SUBST
%type <rule> match_body match_stmt match_decl
%type <list> match_items match_stmts style_body style_stmts
%type <styleinfo> style_stmt
%type <match> match_item
%type <macro> macro_decl
%type <style> style_decl
%type <regex> MATCH
%type <text> STRING SUBST
%type <number> NUMBER

%%
input:			input input_item
				| /* empty */
				;

input_item:		style_decl 				{ if (hmap_contains_key(script->styles, $1->name))
										      script_die(_("style `%s' already defined"), $1->name);
										  hmap_put(script->styles, $1->name, $1); }
				| macro_decl 			{ if (hmap_contains_key(script->macros, $1->name))
										      script_die(_("macro `%s' already defined"), $1->name);
										  hmap_put(script->macros, $1->name, $1); }
				| match_stmt 			{ llist_add(file->rules, $1); }
				| PREPEND match_body	{ if (script->prepend_rule != NULL)
											  script_die(_("prepend rule already specifed"));
										  script->prepend_rule = $2; }
				| APPEND match_body		{ if (script->append_rule != NULL)
											  script_die(_("append rule already specifed"));
										  script->append_rule = $2; }
				;

style_decl:	  	STYLE STRING style_body	{ $$ = new_style($2, $3); }
				;
style_body:		'{' style_stmts '}'		{ $$ = $2; }
				| style_stmt			{ $$ = llist_new(); llist_add($$, $1); }
				;
style_stmts:	style_stmts style_stmt	{ llist_add($1, $2); $$ = $1; }
				| /* empty */			{ $$ = llist_new(); }
				;
style_stmt:		STRING					{ Style *style = hmap_get(script->styles, $1);
						  				  if (style == NULL)
						      				  script_die(_("no such style `%s'"), $1);
										  $$ = new_styleinfo_style(style);
										}
				| PREPEND STRING		{ $$ = new_styleinfo_pre($2); }
				| APPEND STRING			{ $$ = new_styleinfo_post($2); }
				;

macro_decl:		MACRO STRING match_body	{ $$ = new_macro($2, $3); }
				;

match_decl:		match_items match_body	{ $$ = new_match_rule($1, $2); }
				;

match_items:	match_items ','	match_item	{ llist_add($1, $3); $$ = $1; }
				| match_item				{ $$ = llist_new(); llist_add($$, $1); }
				;
match_item:		MATCH					{ $$ = new_regex_match($1.text, $1.flags); }
				| NUMBER				{ $$ = new_subex_match($1); }
				;

match_body:		'{' match_stmts	'}'		{ $$ = new_multi_rule($2); }
				| match_stmt
				;
match_stmts:	match_stmts	match_stmt	{ llist_add($1, $2); $$ = $1; }
				| /* empty */			{ $$ = llist_new(); }
				;

match_stmt:		INCLUDE	STRING			{ $$ = include_script($2); }
				| STRING				{ if (hmap_contains_key(script->macros, $1)) {
											  $$ = new_macro_rule(hmap_get(script->macros, $1));
										  } else if (hmap_contains_key(script->styles, $1)) {
											  $$ = new_style_rule(hmap_get(script->styles, $1));
										  } else {
											  script_die(_("no such macro or style `%s'"), $1);
										  }
										}
				| SUBST	MATCH			{ $$ = new_substitution_rule($1, $2.text, $2.flags); }
				| SET STRING			{ $$ = new_set_rule($2); }
				| SKIP					{ $$ = new_action_rule(ACTION_SKIP); }
				| BREAK					{ $$ = new_action_rule(ACTION_BREAK); }
				| match_decl
				;

%%

RemarkScript *
parse_script(const char *filename)
{
	script = xmalloc(sizeof(RemarkScript));
	script->styles = hmap_new();
	script->macros = hmap_new();
	script->prepend_rule = NULL;
	script->append_rule = NULL;
	script->rule = (filename == NULL ? NULL : include_script(filename));
	return script;
}

void
free_script(RemarkScript *script)
{
	hmap_foreach_value(script->styles, (IteratorFunc) free_style);
	hmap_free(script->styles);
	hmap_foreach_value(script->macros, (IteratorFunc) free_rule);
	hmap_free(script->macros);
	free_rule(script->prepend_rule);
	free_rule(script->append_rule);
	free_rule(script->rule);
	free(script);
}

static Rule *
include_script(const char *filename)
{
	RemarkFile rf;
	int old_yychar = yychar;
	int old_yylineno = yylineno;

	if (filename[0] == '/') {
		rf.file = fopen(filename, "r");
		if (rf.file == NULL)
			script_die(_("cannot open `%s': %s"), filename, strerror(errno));
	} else {
		rf.file = NULL;
		if (file != NULL) {
			char *name = cat_files(file->directory, filename);
			rf.file = fopen(name, "r");
			if (rf.file == NULL && errno != ENOENT)
				script_die(_("cannot open `%s': %s"), name, strerror(errno));
			free(name);
		} else {
			rf.file = fopen(filename, "r");
			if (rf.file == NULL && errno != ENOENT)
				script_die(_("cannot open `%s': %s"), filename, strerror(errno));
		}
		if (rf.file == NULL) {
			char *dir = cat_files(getenv("HOME"), PKGUSERDIR);
			char *name = cat_files(dir, filename);
			rf.file = fopen(name, "r");
			if (rf.file == NULL && errno != ENOENT)
				script_die(_("cannot open `%s': %s"), name, strerror(errno));
			free(name);
			free(dir);
		}
		if (rf.file == NULL) {
			char *name = cat_files(PKGDATADIR, filename);
			rf.file = fopen(name, "r");
			if (rf.file == NULL && errno != ENOENT)
				script_die(_("cannot open `%s': %s"), name, strerror(errno));
			free(name);
		}
		if (rf.file == NULL)
			script_die(_("cannot open `%s': %s"), filename, strerror(errno));

	}
	assert(rf.file != NULL);

	yychar = '\0';
	yylineno = 1;
	rf.filename = filename;
	rf.directory = dir_name(filename);
	rf.rules = llist_new();
	rf.tokens = llist_new();
	rf.previous = file;
	file = &rf;

	lexer_set_buffer(&rf);
	yyparse();
	lexer_restore_buffer(rf.previous);

	file = rf.previous;
	llist_iterate(rf.tokens, (IteratorFunc) free);
	llist_free(rf.tokens);
	fclose(rf.file);
	free(rf.directory);
	yylineno = old_yylineno;
	yychar = old_yychar;

	return new_multi_rule(rf.rules);
}

static void
script_die(const char *msg, ...)
{
	va_list ap;
	
	va_start(ap, msg);
	if (file != NULL)
		fprintf(stderr, "%s:%d: ", file->filename, yylineno);
	else
		fprintf(stderr, "%s: ", program_name);
	vfprintf(stderr, msg, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	exit(1);
}

static void
yyerror(char *msg)
{
	script_die("%s", msg);
}
