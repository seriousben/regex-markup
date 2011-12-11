/* remark.h - Common headers for all source files of regex-markup.
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

#ifndef REMARK_H
#define REMARK_H

#include <stdint.h>		/* POSIX */
#include <regex.h>		/* gnulib */
#include <stdio.h>		/* C89 */
#include "common/llist.h"
#include "common/hmap.h"
#include "common/strbuf.h"

#define PKGUSERDIR ".remark"

typedef enum _ActionType ActionType;
typedef enum _MatchType MatchType;
typedef enum _RuleType RuleType;
typedef enum _StyleInfoType StyleInfoType;
typedef enum _RegexFlags RegexFlags;
typedef enum _WrapperType WrapperType;
typedef struct _StyleInfo StyleInfo;
typedef struct _Match Match;
typedef struct _SubexMatch SubexMatch;
typedef struct _RegexMatch RegexMatch;
typedef struct _Rule Rule;
typedef struct _Macro Macro;
typedef struct _Style Style;
typedef struct _MacroRule MacroRule;
typedef struct _StyleRule StyleRule;
typedef struct _MultiRule MultiRule;
typedef struct _MatchRule MatchRule;
typedef struct _ActionRule ActionRule;
typedef struct _SubstitutionRule SubstitutionRule;
typedef struct _SetRule SetRule;
typedef struct _MatchState MatchState;
typedef struct _MatchBuffer MatchBuffer;
typedef struct _SubmatchSpec SubmatchSpec;
typedef struct _StyleRange StyleRange;
typedef struct _RemarkInput RemarkInput;
typedef struct _RemarkFile RemarkFile;
typedef struct _RemarkScript RemarkScript;

enum _ActionType {
	ACTION_CONTINUE,
	ACTION_BREAK,
	ACTION_SKIP
};

enum _MatchType {
	MATCH_REGEX,
	MATCH_SUBEX,
};

enum _RuleType {
	RULE_MATCH,
	RULE_MACRO,
	RULE_STYLE,
	RULE_MULTI,
	RULE_ACTION,
	RULE_SUBSTITUTION,
	RULE_SET,
};

enum _StyleInfoType {
	STYLEINFO_STYLE,
	STYLEINFO_PRE,
	STYLEINFO_POST,
};

enum _RegexFlags {
	REGEX_GLOBAL		= 1 << 0,
	REGEX_IGNORE_CASE	= 1 << 1,
};

enum _WrapperType {
	WRAPPER_NONE,
	WRAPPER_CHAR,
	WRAPPER_WORD,
};

struct _StyleInfo {
	StyleInfoType type;
	const void *value;
};

struct _Match {
	MatchType type;
};

struct _RegexMatch {
	Match m;
	regex_t regex;
	char *regex_string;
	RegexFlags flags;
};

struct _SubexMatch {
	Match m;
	uint32_t index;
};

struct _Rule {
	RuleType type;
};

struct _Macro {
	char *name;
	Rule *rule;
	uint32_t refs;
};

struct _Style {
	char *name;
	char *pre_string;
	char *post_string;
	uint32_t refs;
};

struct _MacroRule {
	Rule r;
	Macro *macro;
};

struct _StyleRule {
	Rule r;
	Style *style;
};

struct _MultiRule {
	Rule r;
	uint32_t rule_count;
	Rule **rules;
};

struct _MatchRule {
	Rule r;
	uint32_t match_count;
	Match **matches;
	Rule *rule;
};

struct _ActionRule {
	Rule r;
	ActionType action;
};

struct _SubstitutionRule {
	Rule r;
	regex_t regex;
	RegexFlags flags;
	char *replacement;
};

struct _SetRule {
	Rule r;
	char *replacement;
};

struct _MatchBuffer {
	StrBuf *buffer;
	uint32_t bufferlen;
	LList *styles;
	Iterator *styles_it;
};

struct _SubmatchSpec {
	uint32_t so;
	uint32_t eo;
};

struct _MatchState {
	MatchBuffer *top;
	MatchState *parent;
	uint32_t subc;
	SubmatchSpec subv[0];
};

struct _StyleRange {
	uint32_t so;
	uint32_t eo;
	Style *style;
	uint32_t pre_length;
	uint32_t post_length;
};

struct _RemarkInput {
	MatchBuffer mb;
	MatchBuffer append_mb;
	MatchBuffer prepend_mb;
};

struct _RemarkFile {
	RemarkFile *previous;
	FILE *file;
	const char *filename;
	char *directory;
	LList *rules;
	LList *tokens;
	void *lex_buffer;
};

struct _RemarkScript {
	HMap *styles;
	HMap *macros;
	Rule *prepend_rule;
	Rule *append_rule;
	Rule *rule;
};

/* style.c */
StyleInfo *new_styleinfo_style(Style *style);
StyleInfo *new_styleinfo_pre(const char *string);
StyleInfo *new_styleinfo_post(const char *string);
Style *new_style(const char *name, LList *spec);
void free_style(Style *style);
uint32_t insert_style_pre(Style *style, StrBuf *strbuf, uint32_t pos);
uint32_t insert_style_post(Style *style, StrBuf *strbuf, uint32_t pos);

/* rule.c */
Macro *new_macro(const char *name, Rule *rule);
void free_macro(Macro *macro);
Rule *new_macro_rule(Macro *macro);
Rule *new_style_rule(Style *style);
Rule *new_multi_rule(LList *rules);
Rule *new_match_rule(LList *matches, Rule *rule);
Rule *new_action_rule(ActionType action);
void free_rule(Rule *anyrule);
Rule *new_substitution_rule(const char *match, const char *repl, RegexFlags flags);
Rule *new_prepend_rule(Rule *rule);
Rule *new_append_rule(Rule *rule);
Rule *new_set_rule(const char *replacement);

/* match.c */
void free_match(Match *anymatch);
Match *new_regex_match(const char *respec, RegexFlags flags);
Match *new_subex_match(uint32_t index);
MatchState *new_match_state(MatchBuffer *mb, uint32_t so, uint32_t eo, uint32_t subc);
MatchState *try_match(Match *match, MatchState *ms, uint32_t *start, uint32_t call);
void free_match_state(MatchState *ms);
char *expand_substitution(const char *repl, MatchState *ms, uint32_t subc, SubmatchSpec *subv);

/* lexer.l */
int yylex(void);
void lexer_set_buffer(RemarkFile *rf);
void lexer_restore_buffer(RemarkFile *file);
extern int yylineno;

/* parser.y */
RemarkScript *parse_script(const char *filename);
void free_script(RemarkScript *script);

/* execute.c */
bool execute_script(RemarkScript *script, RemarkInput *input);
void init_input(RemarkInput *input);
void free_input(RemarkInput *input);

/* wrap.c */
extern uint32_t wrap_retain;
extern uint32_t wrap_width;
extern WrapperType wrapper;
bool identify_wrapper(const char *spec);

void check_args(void);
void wrap_line(RemarkScript *script, RemarkInput *input);
void apply_styles(MatchBuffer *mb, uint32_t ep);

#endif
