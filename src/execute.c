/* execute.c - Functions for executing plans - matching, coloring etc.
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

#include <config.h>
#include <stdlib.h>		/* C89 */
#include <string.h>		/* C89 */
#include <assert.h>		/* C89 */
#include <stdint.h>		/* Gnulib/C99/POSIX */
#include "xalloc.h"		/* Gnulib */
#include "minmax.h"		/* Gnulib */
#include "common/string-utils.h"
#include "common/regex-utils.h"
#include "common/strbuf.h"
#include "remark.h"

static bool execute_rule(Rule *rule, MatchBuffer *mb);
static ActionType execute_substitution_rule(SubstitutionRule *rule, MatchState *ms);
static ActionType execute_match_rule(MatchRule *rule, MatchState *ms);
static ActionType execute_multi_rule(MultiRule *rule, MatchState *state);
static ActionType execute_set_rule(SetRule *rule, MatchState *ms);
static ActionType execute_any_rule(Rule *rule, MatchState *ms);

static void update_positions(MatchState *ms, int32_t so, int32_t diff);
static StyleRange *new_style_range(Style *style, uint32_t so, uint32_t eo);
static void insert_style(MatchState *ms, StyleRange *s1);
static LNode *insort_style1(LList *list, LNode *node, StyleRange *orig, StyleRange *a);
static LNode *insort_style2(LList *list, LNode *node, StyleRange *orig, StyleRange *a, StyleRange *b);
static LNode *insort_style3(LList *list, LNode *node, StyleRange *orig, StyleRange *b);

static void init_match_buffer(MatchBuffer *mb);
static void free_match_buffer(MatchBuffer *mb);

bool
execute_script(RemarkScript *script, RemarkInput *input)
{
	if (script->rule != NULL) {
		if (!execute_rule(script->rule, &input->mb))
			return false;
	}

	if (script->append_rule != NULL) {
		execute_rule(script->append_rule, &input->append_mb);
		apply_styles(&input->append_mb, INT32_MAX);
		llist_clear(input->append_mb.styles);
	}

	if (script->prepend_rule != NULL) {
		execute_rule(script->prepend_rule, &input->prepend_mb);
		apply_styles(&input->prepend_mb, INT32_MAX);
		llist_clear(input->prepend_mb.styles);
	}

	wrap_line(script, input);
	llist_clear(input->mb.styles);

	return true;
}

static bool
execute_rule(Rule *rule, MatchBuffer *mb)
{
	MatchState *ms;

	assert(strbuf_length(mb->buffer) == mb->bufferlen);
	ms = new_match_state(mb, 0, mb->bufferlen, 1);
	if (execute_any_rule(rule, ms) != ACTION_SKIP) {
		assert(strbuf_length(mb->buffer) == ms->subv[0].eo);
		mb->bufferlen = ms->subv[0].eo;
		free_match_state(ms);
		return true;
	}

	mb->bufferlen = 0;
	free_match_state(ms);
	return false;
}

static ActionType
execute_substitution_rule(SubstitutionRule *subst, MatchState *ms)
{
	uint32_t so = ms->subv[0].so;
	uint32_t eo = ms->subv[0].eo;
	bool last;

	do {
		uint32_t replen;
		uint32_t subc = subst->regex.re_nsub + 1;
		regmatch_t subv_re[subc];
		SubmatchSpec subv[subc];
		char *repl;
		uint32_t c;

		last = (strbuf_char_at(ms->top->buffer, so) == '\0');
		if (!xregexec_substring(&subst->regex, strbuf_buffer(ms->top->buffer), so, eo, subc, subv_re, 0))
			break;

    	    	for (c = 0; c < subc; c++) {
		    subv[c].so = subv_re[c].rm_so;
		    subv[c].eo = subv_re[c].rm_eo;
		}

		repl = expand_substitution(subst->replacement, ms, subc, subv); /* XXX: memory management */
		strbuf_replace(ms->top->buffer, subv[0].so, subv[0].eo, repl);
		replen = strlen(repl);

		update_positions(ms, subv[0].so, replen - (subv[0].eo-subv[0].so));
		so += replen + (subv[0].so == subv[0].eo ? 1 : 0);
		free(repl);
	} while (!last && (subst->flags & REGEX_GLOBAL) != 0);

	return ACTION_CONTINUE;
}

static ActionType
execute_set_rule(SetRule *rule, MatchState *ms)
{
    char *repl;
    uint32_t so = ms->subv[0].so;
    uint32_t eo = ms->subv[0].eo;

    repl = expand_substitution(rule->replacement, ms, ms->subc, ms->subv); /* XXX: memory management */
    strbuf_replace(ms->top->buffer, so, eo, repl);
    update_positions(ms, 0, strlen(repl) - (eo-so));
    free(repl);

    return ACTION_CONTINUE;
}

static ActionType
execute_match_rule(MatchRule *cmd, MatchState *ms)
{
	int c;
	ActionType outer_action = ACTION_CONTINUE;

	for (c = 0; c < cmd->match_count; c++) {
		Match *match = cmd->matches[c];
		MatchState *new_ms;
		ActionType action;
		uint32_t so = ms->subv[0].so;
		uint32_t call = 0;

		while ((new_ms = try_match(match, ms, &so, call++)) != NULL) {
			action = execute_any_rule(cmd->rule, new_ms);
			if (action == ACTION_BREAK)
				outer_action = ACTION_BREAK;
			if (action == ACTION_SKIP) {
				free_match_state(new_ms);
				return ACTION_SKIP;
			}

			so = new_ms->subv[0].eo;
			free_match_state(new_ms);
		}
	}

	return outer_action;
}

static ActionType
execute_any_rule(Rule *anyrule, MatchState *ms)
{
	if (anyrule->type == RULE_MACRO) {
		MacroRule *rule = (MacroRule *) anyrule;
		return execute_any_rule(rule->macro->rule, ms);
	}
	else if (anyrule->type == RULE_MULTI) {
		MultiRule *rule = (MultiRule *) anyrule;
		return execute_multi_rule(rule, ms);
	}
	else if (anyrule->type == RULE_STYLE) {
		StyleRule *rule = (StyleRule *) anyrule;
		insert_style(ms, new_style_range(rule->style, ms->subv[0].so, ms->subv[0].eo));
		return ACTION_CONTINUE;
	}
	else if (anyrule->type == RULE_ACTION) {
		ActionRule *rule = (ActionRule *) anyrule;
		return rule->action;
	}
	else if (anyrule->type == RULE_SUBSTITUTION) {
		SubstitutionRule *rule = (SubstitutionRule *) anyrule;
		return execute_substitution_rule(rule, ms);
	}
	else if (anyrule->type == RULE_MATCH) {
		MatchRule *rule = (MatchRule *) anyrule;
		return execute_match_rule(rule, ms);
	}
	else if (anyrule->type == RULE_SET) {
		SetRule *rule = (SetRule *) anyrule;
		return execute_set_rule(rule, ms);
	}

	return ACTION_CONTINUE;
}

static ActionType
execute_multi_rule(MultiRule *rule, MatchState *ms)
{
	int c;

	for (c = 0; c < rule->rule_count; c++) {
		ActionType action = execute_any_rule(rule->rules[c], ms);
		if (action != ACTION_CONTINUE)
			return action;
	}

	return ACTION_CONTINUE;
}

static void
update_positions(MatchState *ms, int32_t so, int32_t diff)
{
	int c;

	if (ms->parent == NULL) {
		Iterator *it = ms->top->styles_it;
		for (iterator_restart(it); iterator_has_next(it); ) {
			StyleRange *rng = iterator_next(it);

			if (rng->so == rng->eo && rng->so == so) {
				rng->eo = MAX((int32_t) rng->eo + diff, so);
			} else {
				if (rng->eo > so || (rng->eo >= so && so == ms->subv[0].eo))
					rng->eo = MAX((int32_t) rng->eo + diff, so);
				if (rng->so > so)
					rng->so = MAX((int32_t) rng->so + diff, so);
			}
		}
	}

	for (c = 0; c < ms->subc; c++) {
		if (ms->subv[c].so == ms->subv[c].eo && ms->subv[c].so == so) {
			ms->subv[c].eo = MAX((int32_t) ms->subv[c].eo + diff, so);
		} else {
			if (ms->subv[c].eo > so || (ms->subv[c].eo >= so && c == 0 && ms->parent == NULL))
				ms->subv[c].eo = MAX((int32_t) ms->subv[c].eo + diff, so);
			if (ms->subv[c].so > so)
				ms->subv[c].so = MAX((int32_t) ms->subv[c].so + diff, so);
		}
	}

	if (ms->parent != NULL)
		update_positions(ms->parent, so, diff);
}

static void
insert_style(MatchState *ms, StyleRange *s1)
{
	LList *list = ms->top->styles;
	LNode *node = llist_get_first_node(list);
	uint32_t old_so = s1->so;

	for (; node != NULL; node = lnode_next(node)) {
		StyleRange *s2 = lnode_data(node);

		if (s1->so == s1->eo && s1->so == s2->eo) {							/* Case 5 */
			insort_style1(list, node, s2, s1);
			return;
		}
		else if (s1->so == s1->eo && s2->so < s1->so && s1->so < s2->eo) {	/* Case 6 */
			insort_style2(list, node, s2, s1, NULL);
			return;
		}
		else if (s2->so == s2->eo && s1->so == s2->so) {					/* Case 7 */
			if (s1->so > old_so)
				node = insort_style1(list, node, s2, new_style_range(s1->style, s2->so, s2->eo));
		}
		else if (s1->eo <= s2->so) {
			lnode_add_before(list, node, s1);
			return;
		}
		else if (s1->so == s2->so && s1->eo == s2->eo) {					/* Case 1 */
			insort_style1(list, node, s2, s1);
			return;
		}
		else if (s1->so == s2->so && s1->eo < s2->eo) {						/* Case 2a */
			insort_style2(list, node, s2, s1, NULL);
			return;
		}
		else if (s1->so > s2->so && s1->eo == s2->eo) {						/* Case 2b */
			insort_style2(list, node, s2, NULL, s1);
			return;
		}
		else if (s1->so > s2->so && s1->eo < s2->eo) {						/* Case 2c */
			insort_style3(list, node, s2, s1);
			return;
		}
		else if (s1->so == s2->so && s1->eo > s2->eo) {						/* Case 3a */
			node = insort_style1(list, node, s2, new_style_range(s1->style, s2->so, s2->eo));
			s1->so = s2->eo;
		}
		else if (s1->so < s2->so && s1->eo == s2->eo) {						/* Case 3b */
			lnode_add_before(list, node, new_style_range(s1->style, s1->so, s2->so));
			s1->so = s2->so;
			insort_style1(list, node, s2, s1);
			return;
		}
		else if (s1->so < s2->so && s1->eo > s2->eo) {						/* Case 3c */
			lnode_add_before(list, node, new_style_range(s1->style, s1->so, s2->so));
			node = insort_style1(list, node, s2, new_style_range(s1->style, s2->so, s2->eo));
			s1->so = s2->eo;
		}
		else if (s1->so < s2->so && s1->eo < s2->eo && s1->eo > s2->so) {	/* Case 4a */
			lnode_add_before(list, node, new_style_range(s1->style, s1->so, s2->so));
			s1->so = s2->so;
			insort_style2(list, node, s2, s1, NULL);
			return;
		}
		else if (s2->so < s1->so && s2->eo < s1->eo && s2->eo > s1->so) {	/* Case 4b */
			uint32_t t = s2->eo;
			node = insort_style2(list, node, s2, NULL, new_style_range(s1->style, s1->so, s2->eo));
			s1->so = t;
		}
	}

	assert(node == NULL);
	llist_add(list, s1);
}

static LNode *
insort_style1(LList *list, LNode *node, StyleRange *orig, StyleRange *a)
{
	while (!lnode_is_last(node)) {
		StyleRange *t;
		node = lnode_next(node);
		t = lnode_data(node);
		if (t->so != orig->so || t->eo != orig->eo) {
			node = lnode_previous(node);
			break;
		}
	}

	return lnode_add_after(list, node, a);
}

static LNode *
insort_style2(LList *list, LNode *node, StyleRange *orig, StyleRange *a, StyleRange *b)
{
	LNode *mid;
	LNode *last;
	StyleRange *t;
	uint32_t p2 = (a != NULL ? a->eo : b->so);
	uint32_t p3 = orig->eo;

	while (!lnode_is_last(node)) {
		node = lnode_next(node);
		t = lnode_data(node);
		if (t->so != orig->so || t->eo != orig->eo) {
			node = lnode_previous(node);
			break;
		}
	}

	t = lnode_data(node);
	mid = lnode_add_after(list, node, new_style_range(t->style, p2, p3));
	t->eo = p2;
	last = mid;

	while (lnode_data(node) != orig) {
		node = lnode_previous(node);
		t = lnode_data(node);
		mid = lnode_add_before(list, mid, new_style_range(t->style, p2, p3));
		t->eo = p2;
	}

	if (a != NULL)
		lnode_add_before(list, mid, a);
	if (b != NULL)
		last = lnode_add_after(list, last, b);

	return last;
}

static LNode *
insort_style3(LList *list, LNode *node, StyleRange *orig, StyleRange *b)
{
	LNode *mid;
	LNode *mid2;
	LNode *last;
	StyleRange *t;
	uint32_t p2 = b->so;
	uint32_t p3 = b->eo;
	uint32_t p4 = orig->eo;

	while (!lnode_is_last(node)) {
		node = lnode_next(node);
		t = lnode_data(node);
		//printf(" t=%d,%d,%s\n",t->so,t->eo, t->style->name);
		if (t->so != orig->so || t->eo != orig->eo) {
			node = lnode_previous(node);
			break;
		}
	}

	t = lnode_data(node);
	mid = lnode_add_after(list, node, new_style_range(t->style, p2, p3));
	mid2 = lnode_add_after(list, mid, new_style_range(t->style, p3, p4));
	t->eo = p2;
	last = mid;

	while (lnode_data(node) != orig) {
		node = lnode_previous(node);
		t = lnode_data(node);
		mid = lnode_add_before(list, mid, new_style_range(t->style, p2, p3));
		mid2 = lnode_add_before(list, mid2, new_style_range(t->style, p3, p4));
		t->eo = p2;
	}

	last = lnode_add_after(list, last, b);

	return last;
}

static StyleRange *
new_style_range(Style *style, uint32_t so, uint32_t eo)
{
	StyleRange *rng = xmalloc(sizeof(StyleRange));	/* XXX: memory management */
	rng->style = style;
	rng->so = so;
	rng->eo = eo;
	return rng;
}

static void
init_match_buffer(MatchBuffer *mb)
{
	mb->buffer = strbuf_new();
	mb->bufferlen = 0;	/* XXX: necessary? */
	mb->styles = llist_new();
	mb->styles_it = llist_iterator(mb->styles);
}

static void
free_match_buffer(MatchBuffer *mb)
{
	iterator_free(mb->styles_it);
	llist_iterate(mb->styles, (IteratorFunc) free);
	llist_free(mb->styles);
}

void
init_input(RemarkInput *input)
{
	init_match_buffer(&input->mb);
	init_match_buffer(&input->append_mb);
	init_match_buffer(&input->prepend_mb);
}

void
free_input(RemarkInput *input)
{
	free_match_buffer(&input->mb);
	free_match_buffer(&input->append_mb);
	free_match_buffer(&input->prepend_mb);
}
