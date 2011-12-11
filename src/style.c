/* style.c - Functions for creating and inserting styles.
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
#include <xalloc.h>
/* common */
#include "common/strbuf.h"
/* regex-markup */
#include "remark.h"

static void *
new_styleinfo(StyleInfoType type, const void *value)
{
	StyleInfo *info = xmalloc(sizeof(StyleInfo));
	info->type = type;
	info->value = value;
	return info;
}

StyleInfo *
new_styleinfo_style(Style *style)
{
	return new_styleinfo(STYLEINFO_STYLE, style);
}

StyleInfo *
new_styleinfo_pre(const char *string)
{
	return new_styleinfo(STYLEINFO_PRE, string);
}

StyleInfo *
new_styleinfo_post(const char *string)
{
	return new_styleinfo(STYLEINFO_POST, string);
}

Style *
new_style(const char *name, LList *spec)
{
	Style *style;
	Iterator *it;
	StrBuf *prebuf;
	StrBuf *postbuf;

	style = xmalloc(sizeof(Style));
	style->name = xstrdup(name);
	prebuf = strbuf_new();
	postbuf = strbuf_new();

	for (it = llist_iterator(spec); iterator_has_next(it); ) {
		StyleInfo *info = iterator_next(it);
		if (info->type == STYLEINFO_PRE) {
			strbuf_append(prebuf, info->value);
		} else if (info->type == STYLEINFO_POST) {
			strbuf_prepend(postbuf, info->value);
		} else if (info->type == STYLEINFO_STYLE) {
			const Style *add_style = info->value;
			strbuf_append(prebuf, add_style->pre_string);
			strbuf_prepend(postbuf, add_style->post_string);
		}
	}

	style->pre_string = strbuf_free_to_string(prebuf);
	style->post_string = strbuf_free_to_string(postbuf);
	style->refs = 1;

	return style;
}

void
free_style(Style *style)
{
	if (--style->refs <= 0) {
		free(style->name);
		free(style->pre_string);
		free(style->post_string);
		free(style);
	}
}

uint32_t
insert_style_pre(Style *style, StrBuf *strbuf, uint32_t pos)
{
	strbuf_insert(strbuf, pos, style->pre_string);
	return strlen(style->pre_string);
}

uint32_t
insert_style_post(Style *style, StrBuf *strbuf, uint32_t pos)
{
	strbuf_insert(strbuf, pos, style->post_string);
	return strlen(style->post_string);
}
