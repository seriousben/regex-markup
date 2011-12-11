/* match.c - Functions for creating and using Match structures.
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

#if HAVE_CONFIG_H
#include <config.h>
#endif
/* C89 */
#include <stdlib.h>
#include <string.h>
/* gnulib */
#include <regex.h>
#include <xalloc.h>
/* Gettext */
#include <gettext.h>
#define _(String) gettext(String)
/* common */
#include "common/regex-utils.h"
#include "common/error.h"
#include "common/strbuf.h"
#include "common/string-utils.h"
#include "common/intutil.h"
/* remark */
#include "remark.h"

static bool expand_variable(StrBuf *buf, const char *in, uint32_t len, MatchState *ms, uint32_t subc, SubmatchSpec *subv);

MatchState *
try_match(Match *match, MatchState *ms, uint32_t *start, uint32_t call)
{
	MatchState *new_ms;

	if (match->type == MATCH_REGEX) {
		RegexMatch *rematch = (RegexMatch *) match;
		uint32_t c;
		uint32_t so = *start;
		uint32_t subc = rematch->regex.re_nsub + 1;
		regmatch_t subv[subc];

		if (call > 0 && (rematch->flags & REGEX_GLOBAL) == 0)
			return NULL;

		if (!xregexec_substring(&rematch->regex, strbuf_buffer(ms->top->buffer), so, ms->subv[0].eo, subc, subv, 0))
			return NULL;

		new_ms = new_match_state(ms->top, subv[0].rm_so, subv[0].rm_eo, subc);
		new_ms->parent = ms;
		for (c = 1; c < subc; c++) {
			new_ms->subv[c].so = subv[c].rm_so;
			new_ms->subv[c].eo = subv[c].rm_eo;
		}

		*start += subv[0].rm_eo - subv[0].rm_so;
		return new_ms;
	}

	if (match->type == MATCH_SUBEX) {
		SubexMatch *submatch = (SubexMatch *) match;
		SubmatchSpec *spec;

		if (call > 0)
			return NULL;
		if (submatch->index >= ms->subc)
			die(_("subexpression index is out of range"));

		spec = &ms->subv[submatch->index];
		if (spec->so == -1 || spec->eo == -1)
			return NULL;

		*start = spec->so;
		new_ms = new_match_state(ms->top, spec->so, spec->eo, 1);
		new_ms->parent = ms;
		return new_ms;
	}

	return NULL;
}

MatchState *
new_match_state(MatchBuffer *mb, uint32_t so, uint32_t eo, uint32_t subc)
{
	MatchState *ms = xmalloc(sizeof(MatchState) + sizeof(SubmatchSpec)*subc);	/* XXX: memory management */
	ms->top = mb;
	ms->parent = NULL;
	ms->subv[0].so = so;
	ms->subv[0].eo = eo;
	ms->subc = subc;
	return ms;
}

void
free_match_state(MatchState *ms)
{
	free(ms);
}

static void *
new_match(MatchType type, size_t size)
{
	Match *match = xmalloc(size);
	match->type = type;
	return match;
}

void
free_match(Match *anymatch)
{
	if (anymatch->type == MATCH_REGEX) {
		RegexMatch *match = (RegexMatch *) anymatch;
		free(match->regex_string);
		regfree(&match->regex);
	}
	else if (anymatch->type == MATCH_SUBEX) {
		/* no operation */
	}

	free(anymatch);
}

Match *
new_regex_match(const char *respec, RegexFlags flags)
{
	RegexMatch *regex = new_match(MATCH_REGEX, sizeof(RegexMatch));
	int init_flags = REG_EXTENDED;
	int rc;

	memset(&regex->regex, 0, sizeof(regex_t));
	regex->regex_string = xstrdup(respec);
	regex->flags = flags;
	if (flags & REGEX_IGNORE_CASE)
		init_flags |= REG_ICASE;

	rc = regcomp(&regex->regex, respec, init_flags);
	if (rc != 0) {
		char *msg = xregerror(rc, &regex->regex);
		regfree(&regex->regex);
		warn("%s", msg);
		free(msg);
		exit(1);
	}

	return (Match *) regex;
}

Match *
new_subex_match(uint32_t index)
{
	SubexMatch *subex = new_match(MATCH_SUBEX, sizeof(SubexMatch));
	subex->index = index;
	return (Match *) subex;
}

/* Note: It is kind of stupid to first call strbuf_free_to_string,
 * then later free (above this function). But with the current API
 * of strbuf_free there's no other way!
 */
char *
expand_substitution(const char *repl, MatchState *ms, uint32_t subc, SubmatchSpec *subv)
{
	StrBuf *buf = strbuf_new(); /* XXX: memory management */
	bool escaped = false;
	uint32_t c;

	for (c = 0; repl[c] != '\0'; c++) {
		if (!escaped && repl[c] == '$') {
			uint32_t d;
			if (repl[c+1] == '{') {
				for (d = c+2; repl[d] != '}' && repl[d] != '\0'; d++);
				if (repl[d] != '\0') {
					if (expand_variable(buf, repl+c+2, d-c-2, ms, subc, subv)) {
						c = d;
						continue;
					}
				}
			} else if (isdigit(repl[c+1])) {
				for (d = c+2; isdigit(repl[d]); d++);
				if (expand_variable(buf, repl+c+1, d-c-1, ms, subc, subv)) {
					c = d-1;
					continue;
				}
			} else if (repl[c+1] != '\0' && strchr("`'&", repl[c+1]) != NULL) {
				expand_variable(buf, repl+c+1, 1, ms, subc, subv);
				c++;
				continue;
			}
		}
		escaped = (!escaped && repl[c] == '\\');
		if (!escaped)
			strbuf_append_char(buf, repl[c]);
	}

	return strbuf_free_to_string(buf);
}

static bool
expand_variable(StrBuf *buf, const char *in, uint32_t len, MatchState *ms, uint32_t subc, SubmatchSpec *subv)
{
	if (len == 1 && in[0] == '`') {
		strbuf_append_substring(buf, strbuf_buffer(ms->top->buffer), ms->subv[0].so, subv[0].so);
	} else if (len == 1 && in[0] == '&') {
		strbuf_append_substring(buf, strbuf_buffer(ms->top->buffer), subv[0].so, subv[0].eo);
	} else if (len == 1 && in[0] == '\'') {
		strbuf_append_substring(buf, strbuf_buffer(ms->top->buffer), subv[0].eo, ms->subv[0].eo);
	} else {
		uint32_t idx = 0;
		int c;
		for (c = 0; c < len; c++) {
			if (!isdigit(in[c]))
				return false;
			idx = idx*10 + (in[c]-'0');
		}
		if (idx < 0 || idx >= subc)
			return true;
		strbuf_append_substring(buf, strbuf_buffer(ms->top->buffer), subv[idx].so, subv[idx].eo);
	}

	return true;
}
