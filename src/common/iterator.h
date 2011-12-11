/* iterator.h - The iterator support-data structure.
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

#ifndef COMMON_ITERATOR_H
#define COMMON_ITERATOR_H

/* POSIX */
#include <sys/types.h>
/* POSIX/gnulib */
#include <stdbool.h>
/* common */
#include "common.h"

typedef void (*IteratorFunc)(void *);

typedef struct _IteratorClass IteratorClass;
typedef struct _Iterator Iterator;

struct _IteratorClass {
	bool (*has_next)(Iterator *it);
	void *(*next)(Iterator *it);
	void (*remove)(Iterator *it);
	void (*free)(Iterator *it);
	void (*restart)(Iterator *it);
	void *(*previous)(Iterator *it);
	void (*add)(Iterator *it, void *value);
};

struct _Iterator {
	IteratorClass *class;
};

bool iterator_has_next(Iterator *it);
void *iterator_next(Iterator *it);
void iterator_free(Iterator *it);
void iterator_remove(Iterator *it);
void iterator_restart(Iterator *it); /* FIXME: rename to iterator_first?*/
void *iterator_previous(Iterator *it);
void iterator_add(Iterator *it, void *value);
#define iterator_first iterator_restart

#endif
