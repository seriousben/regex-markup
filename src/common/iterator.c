/* iterator.c - The iterator support-data structure.
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
#include <stdlib.h>	/* C89 */
#include "gettext.h"	/* Gnulib/gettext */
#define _(String) gettext(String)
#include "iterator.h"
#include "error.h"

bool
iterator_has_next(Iterator *it)
{
	return it->class->has_next(it);
}

void *
iterator_next(Iterator *it)
{
	return it->class->next(it);
}

void *
iterator_previous(Iterator *it)
{
	if (it->class->previous == NULL)
		internal_error(_("Called iterator_previous on iterator that doesn't support it"));
	return it->class->previous(it);
}

void
iterator_remove(Iterator *it)
{
	if (it->class->remove == NULL)
		internal_error(_("Called iterator_remove on iterator that doesn't support it"));
	it->class->remove(it);
}

void
iterator_add(Iterator *it, void *value)
{
	if (it->class->add == NULL)
		internal_error("Called iterator_add on iterator that doesn't support it");
	it->class->add(it, value);
}

void
iterator_restart(Iterator *it)
{
	it->class->restart(it);
}

void
iterator_free(Iterator *it)
{
	if (it->class->free == NULL)
		free(it);
	else 
		it->class->free(it);
}
