/* error.c - Error-management and messaging routines.
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

#ifndef COMMON_ERROR_H
#define COMMON_ERROR_H

#include <stdarg.h>	/* C89 */
#include <stddef.h>	/* C89 */
#include <errno.h>	/* C89 */

extern void (*program_termination_hook)(void);

void internal_error(const char *msg, ...) __attribute__ ((noreturn));
void die(const char *msg, ...) __attribute__ ((noreturn));
void die_errno(const char *msg, ...) __attribute__ ((noreturn));
void warn(const char *msg, ...);
void warn_errno(const char *msg, ...);
void set_message_header(const char *msg, ...);
void restore_message_header(void);
extern inline void die_memory(void) __attribute__ ((noreturn));
extern inline void *check_memory(void *mem);

void set_error(const char *msg, ...);
const char *get_error(void);
char *remove_error(void);
void die_error(void) __attribute__ ((noreturn));

void free_error(void);

#define errstr strerror(errno)

/**
 * @note This function is also defined in error.c
 */
extern inline void
die_memory(void)
{
	errno = ENOMEM;
	die_errno(NULL);
}

/**
 * @note This function is also defined in error.c
 */
extern inline void *
check_memory(void *mem)
{
	if (mem == NULL)
		die_memory();
	return mem;
}

#endif
