/* wrap.c - Applying styles and wrapping the output.
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
#include <stdint.h>		/* Gnulib/C99/POSIX */
#include <string.h>		/* C89 */
#include "minmax.h"		/* Gnulib */
#include "common/error.h"
#include "common/strbuf.h"
#include "remark.h"

typedef struct _Wrapper Wrapper;

struct _Wrapper {
	char short_name;
	char *long_name;
	WrapperType type;
};

static Wrapper wrappers[] = {
	{ 'n', "none", WRAPPER_NONE },
	{ 'c', "char", WRAPPER_CHAR },
	{ 'w', "word", WRAPPER_WORD },
};

uint32_t wrap_width = UINT32_MAX;
uint32_t wrap_retain = 0;
WrapperType wrapper = WRAPPER_NONE;
static char *wrapper_args;

bool
identify_wrapper(const char *spec)
{
	int c;
	int len;

	wrapper_args = strchr(spec, ':');
	if (wrapper_args == NULL) {
		len = strlen(spec);
	} else {
		len = wrapper_args - spec;
		wrapper_args++;
	}

	for (c = 0; c < sizeof(wrappers)/sizeof(Wrapper); c++) {
		if ((len == 1 && spec[0] == wrappers[c].short_name)
				|| (len == strlen(wrappers[c].long_name)
				&& strncmp(spec, wrappers[c].long_name, len) == 0)) {
			wrapper = wrappers[c].type;
			if (wrapper != WRAPPER_NONE && wrap_width == UINT32_MAX)
				wrap_width = 80;
			return true;
		}
	}

	return false;
}

void
wrap_line(RemarkScript *script, RemarkInput *input)
{
	Iterator *it = input->mb.styles_it;
	char *retainbuf = NULL;
	char *prestr = strbuf_buffer(input->prepend_mb.buffer);
	char *appstr = strbuf_buffer(input->append_mb.buffer);
	int32_t style_mod = 0;
	uint32_t retainlen = 0;
	uint32_t len = input->mb.bufferlen;
	uint32_t applen = input->append_mb.bufferlen; /* XXX: strbuf_length? */
	uint32_t prelen = input->prepend_mb.bufferlen; /* XXX: strbuf_length? */
	uint32_t width = wrap_width;
	uint32_t sp;

	if (len == 0) {
		apply_styles(&input->mb, INT32_MAX);
		strbuf_append(input->mb.buffer, "\n");
		return;
	}

	/* Prepare retain. */
	if (wrap_retain != 0) {
		MatchBuffer rmb;

		rmb.buffer = strbuf_new_from_substring(strbuf_buffer(input->mb.buffer), 0, wrap_retain);
		rmb.bufferlen = wrap_retain;
		rmb.styles = input->mb.styles;
		rmb.styles_it = input->mb.styles_it;
		apply_styles(&rmb, wrap_retain);

		retainbuf = strbuf_free_to_string(rmb.buffer);
		retainlen = strlen(retainbuf);
	}

	iterator_restart(it);
	for (sp = 0; sp < len; ) {
		StyleRange *last = NULL;
		uint32_t post_last = 0;
		uint32_t deleted = 0;
		uint32_t back = 0;
		uint32_t ep;
		int32_t keep = 0;

		/* Calculate new end position (ep) and delete if necessary */
		ep = MIN(len, sp+width);
		if (ep != len)
			ep -= applen;
		if (wrapper != WRAPPER_NONE && ep < len) {
			if (strbuf_char_at(input->mb.buffer, ep) != ' ') {
				if (wrapper == WRAPPER_WORD) {
					uint32_t nextwidth = (sp == 0 ? width-prelen : width);
					uint32_t t;
					for (t = ep-1; t > sp && strbuf_char_at(input->mb.buffer, t) != ' '; t--);
					if (t > sp) {
						uint32_t u;
						for (u = ep+1; u < len && strbuf_char_at(input->mb.buffer, u) != ' '; u++);
						if (u-t-1 <= nextwidth-applen || (u-t-1 <= nextwidth && u == len))
							ep = t+1;
					}
				}
			} else {
				uint32_t t;
				for (t = ep+1; t < len && strbuf_char_at(input->mb.buffer, t) == ' '; t++);
				if (t < len) {
					deleted = t - ep;
					strbuf_delete(input->mb.buffer, ep, t);
					len -= deleted;
				}
			}
		}

		/* Insert styles into the output buffer */
		while (iterator_has_next(it)) {
			StyleRange *rng = iterator_next(it);
			uint32_t mod;
			uint32_t ss;
			uint32_t es;
			int32_t newkeep;

			if (rng->eo+style_mod > ep)
				back++;

			/* If we're printing the last line (ep == len) and the style
			 * starts on the end position (rng->so+style_mod == ep),
			 * then the style must be empty (rng->so == rng->eo) and
			 * we print it anyway.
			 */
			if (rng->so != rng->eo || rng->so+style_mod > ep) {
				if (ep != len && rng->so+style_mod >= ep)
					break;
			}

			if (last == NULL || last->so != rng->so || last->eo != rng->eo) {
				post_last = 0;
				keep = 0;
			}

			ss = MAX(sp, rng->so+style_mod);
			es = MIN(ep, rng->eo+style_mod);

			newkeep = max((int32_t) (sp-rng->so-style_mod), 0);
			mod = insert_style_pre(rng->style, input->mb.buffer, ss-post_last+keep);
			style_mod += mod;
			len += mod;
			ep += mod;
			mod = insert_style_post(rng->style, input->mb.buffer, es+mod-post_last);
			style_mod += mod;
			len += mod;
			ep += mod;
			post_last += mod;
			last = rng;

			keep += newkeep;
		}
		for (; back > 0; back--)
			iterator_previous(it);

		/* Insert prepend. */
		if (sp != 0 && prestr != NULL && prestr[0] != '\0') {
			strbuf_insert(input->mb.buffer, sp, prestr);
			ep += strlen(prestr);
			len += strlen(prestr);
		}

		/* Insert append. */
		if (ep < len && applen > 0) {
			strbuf_insert(input->mb.buffer, ep, appstr);
			ep += strlen(appstr);
			len += strlen(appstr);
		}

		/* Insert newline into the output buffer */
		strbuf_insert(input->mb.buffer, ep, "\n"); ep++; len++;

		/* Insert retained text, if any */
		if (sp != 0 && retainlen != 0) {
			strbuf_insert(input->mb.buffer, sp, retainbuf);
			ep += retainlen;
			len += retainlen;
			style_mod += retainlen;
		}

		style_mod += 1-deleted;
		if (sp == 0)
			width -= wrap_retain + prelen;

		sp = ep;
	}
}

void
apply_styles(MatchBuffer *mb, uint32_t ep)
{
	StyleRange *last = NULL;
	uint32_t post_last = 0;
	uint32_t style_mod = 0;

	for (iterator_restart(mb->styles_it); iterator_has_next(mb->styles_it); ) {
		StyleRange *rng = iterator_next(mb->styles_it);
		uint32_t mod;
		uint32_t ss;
		uint32_t es;

		if (rng->so+style_mod >= ep)
			break;
		if (last == NULL || last->so != rng->so || last->eo != rng->eo)
			post_last = 0;

		ss = MAX(0, rng->so+style_mod);
		es = MIN(ep, rng->eo+style_mod);

		mod = insert_style_pre(rng->style, mb->buffer, ss-post_last);
		style_mod += mod;
		ep += mod;
		mod = insert_style_post(rng->style, mb->buffer, es+mod-post_last);
		style_mod += mod;
		ep += mod;

		post_last += mod;
		last = rng;
	}
}
