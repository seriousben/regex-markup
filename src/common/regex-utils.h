/* regex-utils.h - Convenience functions for POSIX regular expressions
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

#ifndef REGEX_H
#define REGEX_H

#include <stddef.h>	/* C89 */
#include <stdbool.h>	/* POSIX/gnulib */
#include "regex.h"	/* gnulib */

char *xregerror (int errcode, regex_t *compiled);
bool xregexec(const regex_t *pref, const char *string,
		size_t nmatch, regmatch_t *pmatch, int eflags);
bool xregexec_substring(const regex_t *pref, char *string,
		int start, int end, size_t nmatch, regmatch_t *pmatch,
		int eflags);

#endif
