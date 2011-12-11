/* llist.h - A linked list with a container object (unlike GList)
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

#ifndef COMMON_LLIST_H
#define COMMON_LLIST_H

#include <sys/types.h>		/* POSIX */
#include <stdint.h>		/* POSIX/Gnulib */
#include "common.h"
#include "iterator.h"

typedef struct _LList LList;
typedef struct _LNode LNode;

LList *llist_new(void);
void llist_free(LList *list);
uint32_t llist_size(LList *list);
bool llist_is_empty(LList *list);
void llist_clear(LList *list);

#define llist_append	llist_add_last
#define llist_add		llist_add_last
#define llist_push		llist_add_last
#define llist_unshift	llist_add_first
void llist_add_at(LList *list, uint32_t index, void *object);
void llist_add_first(LList *list, void *object);
void llist_add_last(LList *list, void *object);
void llist_add_all(LList *list, LList *list2);

#define llist_pop		llist_remove_last
#define llist_shift		llist_remove_first
bool llist_remove(LList *list, void *object);
void *llist_remove_at(LList *list, uint32_t index);
void *llist_remove_first(LList *list);
void *llist_remove_last(LList *list);

#define llist_peek		llist_get_last
void *llist_get(LList *list, uint32_t index);
void *llist_get_first(LList *list);
void *llist_get_last(LList *list);

bool llist_contains(LList *list, void *object);
int32_t llist_index_of(LList *list, void *data);
int32_t llist_last_index_of(LList *list, void *data);
LList *llist_clone(LList *list);
void **llist_to_array(LList *list);
void **llist_to_null_terminated_array(LList *list);

void llist_iterate(LList *list, IteratorFunc iterator_func);
Iterator *llist_iterator(LList *list);

void llist_reverse(LList *list);

LNode *llist_get_first_node(LList *list);
LNode *llist_get_last_node(LList *list);
LNode *lnode_next(LNode *node);
LNode *lnode_previous(LNode *node);
void lnode_remove(LList *list, LNode *node);
LNode *lnode_add_after(LList *list, LNode *node, void *data);
LNode *lnode_add_before(LList *list, LNode *node, void *data);
bool lnode_is_first(LNode *node);
bool lnode_is_last(LNode *node);
void *lnode_data(LNode *node);

#endif
