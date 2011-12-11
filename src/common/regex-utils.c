/* regex-utils.c - Convenience functions for POSIX regular expressions
 *
 * Copyright (C) 1998-2005 Oskar Liljeblad
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdbool.h>		/* POSIX/Gnulib */
#include "regex.h"		/* Gnulib */
#include "xalloc.h"		/* Gnulib */
#include "regex-utils.h"
#include "error.h"

char *
xregerror (int errcode, regex_t *compiled)
{
    size_t length = regerror(errcode, compiled, NULL, 0);
    char *buffer = xmalloc(length);
    regerror(errcode, compiled, buffer, length);
    return buffer;
}

bool
xregexec(const regex_t *pref, const char *string, size_t nmatch, regmatch_t *pmatch,
		int eflags)
{
	int rc = regexec(pref, string, nmatch, pmatch, eflags);

	switch (rc) {
	case 0:
		return true;
	case REG_ESPACE:
		die_memory();
		break;
	case REG_NOMATCH:
	default:
		return false;
	}
}

bool
xregexec_substring(const regex_t *pref, char *string, int start, int end,
		size_t nmatch, regmatch_t *pmatch, int eflags)
{
	bool rc;
	char tmp;

	tmp = string[end];
	string[end] = '\0';

	rc = xregexec(pref, string+start, nmatch, pmatch, eflags);
	string[end] = tmp;

	if (rc && start != 0) {
		for (; nmatch > 0; nmatch--) {
			pmatch[nmatch-1].rm_so += start;
			pmatch[nmatch-1].rm_eo += start;
		}
	}

	return rc;
}
