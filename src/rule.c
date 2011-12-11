/* rule.c - The Rule structure.
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
/* common */
#include "common/regex-utils.h"
#include "common/error.h"
/* regex-markup */
#include "remark.h"

static void *
new_rule(RuleType type, size_t size)
{
	Rule *rule = xmalloc(size);
	rule->type = type;
	return rule;
}

void
dump_rule(Rule *anyrule, int indent)
{
	int c;

	for (c=0;c<indent;c++)printf("  ");

	if (anyrule->type == RULE_MATCH) {
		MatchRule *rule = (MatchRule *) anyrule;
		printf("match rule\n");
		dump_rule(rule->rule, indent+1);
	}
	else if (anyrule->type == RULE_MACRO) {
		MacroRule *rule = (MacroRule *) anyrule;
		printf("macro rule\n");
		dump_rule(rule->macro->rule, indent+1);
	}
	else if (anyrule->type == RULE_STYLE) {
		StyleRule *rule = (StyleRule *) anyrule;
		printf("style rule. style=%s\n", rule->style->name);
	}
	else if (anyrule->type == RULE_MULTI) {
		MultiRule *rule = (MultiRule *) anyrule;
		printf("multi rule\n");
		for (c = 0; c < rule->rule_count; c++)
			dump_rule(rule->rules[c], indent+1);
	}
	else if (anyrule->type == RULE_ACTION) {
		ActionRule *rule = (ActionRule *) anyrule;
		printf("action rule. action=");
		switch (rule->action) {
		case ACTION_SKIP: 		puts("skip"); break;
		case ACTION_CONTINUE:	puts("continue"); break;
		case ACTION_BREAK:		puts("break"); break;
		}
	}
	else if (anyrule->type == RULE_SUBSTITUTION) {
		SubstitutionRule *rule = (SubstitutionRule *) anyrule;
		printf("substitution rule. repl=%s\n", rule->replacement);
	}
	else if (anyrule->type == RULE_SET) {
		SetRule *rule = (SetRule *) anyrule;
		printf("set rule. repl=%s\n", rule->replacement);
	}

}

void
free_rule(Rule *anyrule)
{
	int c;

	if (anyrule == NULL) {
 		return;
	}
	else if (anyrule->type == RULE_MATCH) {
		MatchRule *rule = (MatchRule *) anyrule;
		for (c = 0; c < rule->match_count; c++)
			free_match(rule->matches[c]);
		free(rule->matches);
	}
	else if (anyrule->type == RULE_MACRO) {
		MacroRule *rule = (MacroRule *) anyrule;
		free_macro(rule->macro);
	}
	else if (anyrule->type == RULE_STYLE) {
		StyleRule *rule = (StyleRule *) anyrule;
		free_style(rule->style);
	}
	else if (anyrule->type == RULE_MULTI) {
		MultiRule *rule = (MultiRule *) anyrule;
		for (c = 0; c < rule->rule_count; c++)
			free_rule(rule->rules[c]);
		free(rule->rules);
	}
	else if (anyrule->type == RULE_ACTION) {
		/* no operation */
	}
	else if (anyrule->type == RULE_SUBSTITUTION) {
		SubstitutionRule *rule = (SubstitutionRule *) anyrule;
		regfree(&rule->regex);
		free(rule->replacement);
	}
	else if (anyrule->type == RULE_SET) {
		SetRule *rule = (SetRule *) anyrule;
		free(rule->replacement);
	}

	free(anyrule);
}

Macro *
new_macro(const char *name, Rule *rule)
{
	Macro *macro = xmalloc(sizeof(Macro));
	macro->name = xstrdup(name);
	macro->rule = rule;
	macro->refs = 1;
	return macro;
}

void
free_macro(Macro *macro)
{
	if (--macro->refs <= 0) {
		free(macro->name);
		free_rule(macro->rule);
		free(macro);
	}
}

Rule *
new_macro_rule(Macro *macro)
{
	MacroRule *rule = new_rule(RULE_MACRO, sizeof(MacroRule));
	rule->macro = macro;
	macro->refs++;
	return (Rule *) rule;
}

Rule *
new_style_rule(Style *style)
{
	StyleRule *rule = new_rule(RULE_STYLE, sizeof(StyleRule));
	rule->style = style;
	style->refs++;
	return (Rule *) rule;
}

Rule *
new_multi_rule(LList *rules)
{
	uint32_t count = llist_size(rules);
	
	if (count == 1) {
		Rule *rule = llist_get_first(rules);
		llist_free(rules);
		return rule;
	} else {
		MultiRule *rule = new_rule(RULE_MULTI, sizeof(MultiRule));
		rule->rule_count = llist_size(rules);
		rule->rules = (Rule **) llist_to_array(rules);
		llist_free(rules);
		return (Rule *) rule;
	}
}

Rule *
new_match_rule(LList *matches, Rule *subrule)
{
	MatchRule *rule = new_rule(RULE_MATCH, sizeof(MatchRule));
	rule->match_count = llist_size(matches);
	rule->matches = (Match **) llist_to_array(matches);
	rule->rule = subrule;
	llist_free(matches);
	return (Rule *) rule;
}

Rule *
new_action_rule(ActionType action)
{
	ActionRule *rule = new_rule(RULE_ACTION, sizeof(ActionRule));
	rule->action = action;
	return (Rule *) rule;
}

Rule *
new_substitution_rule(const char *match, const char *repl, RegexFlags flags)
{
	SubstitutionRule *rule = new_rule(RULE_SUBSTITUTION, sizeof(SubstitutionRule));
	int init_flags = REG_EXTENDED;
	int rc;

	memset(&rule->regex, 0, sizeof(regex_t));
	rule->replacement = xstrdup(repl);
	rule->flags = flags;
	if (flags & REGEX_IGNORE_CASE)
		init_flags |= REG_ICASE;

	rc = regcomp(&rule->regex, match, init_flags);
	if (rc != 0) {
		char *msg = xregerror(rc, &rule->regex);
		regfree(&rule->regex);
		warn("cannot compile regex: %s", msg);
		free(msg);
		exit(1);
	}

	return (Rule *) rule;
}

Rule *
new_set_rule(const char *replacement)
{
	SetRule *rule = new_rule(RULE_SET, sizeof(SetRule));

	rule->replacement = xstrdup(replacement);

	return (Rule *) rule;
}
