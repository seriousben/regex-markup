/* strbuf.c - The string buffer data-structure.
 *
 * Copyright (C) 2004-2005 Oskar Liljeblad
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdint.h>		/* Gnulib/C99 */
#include <stdarg.h>		/* C89 */
#include <stdlib.h>		/* C89 */
#include <string.h>		/* C89 */
#include "vasprintf.h"    	/* Gnulib */
#include "xalloc.h"             /* Gnulib */
#include "minmax.h"             /* Gnulib */
#include "strbuf.h"

#define DEFAULT_STRBUF_CAPACITY 16

#define SWAP_INT32(a,b) { int32_t _t = (a); (a) = (b); (b) = _t; }

static int32_t
normalize_strbuf_pos(StrBuf *sb, int32_t pos)
{
    if (pos >= sb->len)
        return sb->len;
    if (pos >= 0)
        return pos;
    pos += 1 + sb->len;
    if (pos >= 0)
        return pos;
    return 0;
}

static int32_t
normalize_str_pos(const char *str, int32_t pos)
{
    if (str == NULL)
        return 0;
    if (pos >= 0)
        return strnlen(str, pos);
    pos += 1 + strlen(str);
    if (pos >= 0)
        return pos;
    return 0;
}

char *
strbuf_buffer(StrBuf *sb)
{
    return sb->buf;
}

uint32_t
strbuf_length(StrBuf *sb)
{
    return sb->len;
}

uint32_t
strbuf_capacity(StrBuf *sb)
{
    return sb->capacity;
}

StrBuf *
strbuf_new(void)
{
    return strbuf_new_with_capacity(DEFAULT_STRBUF_CAPACITY);
}

StrBuf *
strbuf_new_with_capacity(uint32_t capacity)
{
    StrBuf *sb;
    sb = xmalloc(sizeof(StrBuf));
    sb->len = 0;
    sb->capacity = capacity;
    sb->buf = xmalloc(sb->capacity * sizeof(char));
    if (sb->capacity > 0)
    	sb->buf[0] = '\0';
    return sb;
}

StrBuf *
strbuf_new_from_char_n(uint32_t times, char ch)
{
    return strbuf_new_from_substring_n(times, &ch, 0, 1);
}

StrBuf *
strbuf_new_from_substring_n(uint32_t times, const char *substr, int32_t subsp, int32_t subep)
{
    subsp = normalize_str_pos(substr, subsp);
    subep = normalize_str_pos(substr, subep);
    if (subsp > subep)
    	SWAP_INT32(subsp, subep);

    return strbuf_new_from_data_n(times, substr+subsp, subep-subsp);
}

StrBuf *
strbuf_new_from_data_n(uint32_t times, const void *mem, uint32_t len)
{
    StrBuf *sb;

    sb = strbuf_new_with_capacity(len * times + 1);
    sb->len = len * times;
    for (; times > 0; times--)
	memcpy(sb->buf + len*(times-1), mem, len);
    sb->buf[sb->len] = '\0';

    return sb;
}

StrBuf *
strbuf_newf_n(uint32_t times, const char *fmt, ...)
{
    va_list ap;
    StrBuf *buf;

    va_start(ap, fmt);
    buf = strbuf_vnewf_n(times, fmt, ap);
    va_end(ap);

    return buf;
}

StrBuf *
strbuf_vnewf_n(uint32_t times, const char *fmt, va_list ap)
{
    char *str;
    int len;
    StrBuf *buf;

    len = vasprintf(&str, fmt, ap);
    if (len < 0)
        xalloc_die();

    buf = strbuf_new_from_substring_n(times, str, 0, len);
    free(str);
    return buf;
}

void
strbuf_free(StrBuf *sb)
{
    if (sb != NULL) {
    	free(sb->buf);
	free(sb);
    }
}

char *
strbuf_free_to_string(StrBuf *sb)
{
    char *buf;

    buf = sb->buf;
    free(sb);
    return buf;
}

void
strbuf_replace_char_n(StrBuf *sb, int32_t sp, int32_t ep, uint32_t times, char ch)
{
    strbuf_replace_substring_n(sb, sp, ep, times, &ch, 0, 1);
}

void
strbuf_replace_data_n(StrBuf *sb, int32_t sp, int32_t ep, uint32_t times, const void *mem, uint32_t len)
{
    uint32_t addlen;
    uint32_t dellen;

    sp = normalize_strbuf_pos(sb, sp);
    ep = normalize_strbuf_pos(sb, ep);
    if (sp > ep)
    	SWAP_INT32(sp, ep);

    addlen = len * times;
    dellen = ep-sp;
    if (addlen != dellen) {
    	strbuf_ensure_capacity(sb, sb->len+1-dellen+addlen);
	memmove(sb->buf+sp+addlen, sb->buf+ep, sb->len+1-ep);
	sb->len += addlen-dellen;
    }
    if (addlen > 0) {
    	for (; times > 0; times--) {
    	    memcpy(sb->buf+sp, mem, len);
	    sp += len;
	}
    }
}

void
strbuf_replace_substring_n(StrBuf *sb, int32_t sp, int32_t ep, uint32_t times, const char *substr, int32_t subsp, int32_t subep)
{
    subsp = normalize_str_pos(substr, subsp);
    subep = normalize_str_pos(substr, subep);
    if (subsp > subep)
    	SWAP_INT32(subsp, subep);

    strbuf_replace_data_n(sb, sp, ep, times, substr+subsp, subep-subsp);
}

int
strbuf_replacef_n(StrBuf *sb, int32_t sp, int32_t ep, uint32_t times, const char *fmt, ...)
{
    va_list ap;
    int len;

    va_start(ap, fmt);
    len = strbuf_vreplacef_n(sb, sp, ep, times, fmt, ap);
    va_end(ap);

    return len;
}

int
strbuf_vreplacef_n(StrBuf *sb, int32_t sp, int32_t ep, uint32_t times, const char *fmt, va_list ap)
{
    char *str;
    int len;

    sp = normalize_strbuf_pos(sb, sp);
    ep = normalize_strbuf_pos(sb, ep);
    if (sp > ep)
    	SWAP_INT32(sp, ep);

    len = vasprintf(&str, fmt, ap);
    if (len < 0)
        xalloc_die();

    strbuf_replace_substring_n(sb, sp, ep, times, str, 0, len);
    free(str);
    return len;
}

void
strbuf_reverse_substring(StrBuf *sb, int32_t sp, int32_t ep)
{
    sp = normalize_strbuf_pos(sb, sp);
    ep = normalize_strbuf_pos(sb, ep);

    while (sp < ep) {
    	SWAP_INT32(sb->buf[sp], sb->buf[ep]);
    	sp++;
	ep--;
    }
}

void
strbuf_repeat_substring(StrBuf *sb, int32_t sp, int32_t ep, uint32_t times)
{
    int32_t addlen;

    sp = normalize_strbuf_pos(sb, sp);
    ep = normalize_strbuf_pos(sb, ep);

    addlen = (ep-sp) * (times - 1);
    if (addlen != 0) {
    	uint32_t p;

    	strbuf_ensure_capacity(sb, sb->len+1+addlen);
	memmove(sb->buf+sp+addlen, sb->buf+ep, sb->len+1-ep);
	sb->len += addlen;

	p = ep;
    	for (; times > 0; times--) {
    	    memmove(sb->buf+p, sb->buf+sp, ep-sp);
	    p += ep-sp;
	}
    }
}

void
strbuf_set_length(StrBuf *sb, uint32_t new_length)
{
    strbuf_ensure_capacity(sb, new_length+1);
    sb->buf[new_length] = '\0';
    sb->len = new_length;
}

void
strbuf_ensure_capacity(StrBuf *sb, uint32_t min_capacity)
{
    if (min_capacity > sb->capacity) {
	sb->capacity = MAX(min_capacity, sb->len*2+2); /* XXX: MAX -> max */
    	sb->buf = xrealloc(sb->buf, sb->capacity * sizeof(char));
	if (sb->len == 0)
    	    sb->buf[0] = '\0';
    }
}

char *
strbuf_substring(StrBuf *sb, int32_t sp, int32_t ep)
{
    char *str;

    sp = normalize_strbuf_pos(sb, sp);
    ep = normalize_strbuf_pos(sb, ep);
    if (sp > ep)
    	SWAP_INT32(sp, ep);

    str = xmalloc((ep-sp+1) * sizeof(char));
    memcpy(str, sb->buf+sp, (ep-sp+1) * sizeof(char));
    str[ep-sp] = '\0';

    return str;
}

char
strbuf_char_at(StrBuf *sb, int32_t sp)
{
    return sb->buf[normalize_strbuf_pos(sb, sp)];
}
