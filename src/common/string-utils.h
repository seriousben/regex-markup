/* string-utils.c - Functions dealing with string contents.
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

#ifndef COMMON_STRING_UTILS_H
#define COMMON_STRING_UTILS_H

#include <sys/types.h>		/* POSIX */
#include <stdint.h>		/* POSIX/gnulib */
#include <stdbool.h>		/* POSIX/gnulib */
#include <ctype.h>		/* C89 - for toupper and tolower */
#include "common.h"

int strindex(const char *str, char ch);
void chomp(char *str);
void strip_leading(char *line, int (*check)(int));
int char_index(const char *line, int (*check)(int));
bool ends_with(const char *str, const char *end);
bool starts_with(const char *str, const char *start);
int translate_char(char *str, char from, char to);
void str_convert(char *str, int (*convert)(int));
#define str_toupper(str)	str_convert((str), toupper)
#define str_tolower(str)	str_convert((str), tolower)
int iswordchar(int ch);
bool replace_str(char *str, const char *from, const char *to);
char *cat_files(const char *file, const char *file2);
char *substring(const char *buf, int start, int end);

int string_strip_trailing(char *str, const char *stripchars);
int string_strip_trailing_c(char *str, char stripchar);
/* int string_strip_trailing_f(char *str, int (*function)(int)); */
int string_strip_leading(char *str, const char *stripchars);
int string_strip_leading_c(char *str, char stripchar);
/* int string_strip_leading_f(char *str, int (*function)(int)); */

#define string_index_of_c	strindex
/* int string_index_of(const char *str, const char *findstr);
int string_index_of_char(const char *str, char findchar);
int string_index_of_any_f(const char *str, int (*function)(int)); */
int string_index_of_any(const char *str, const char *findchars);

int word_get_index(const char *str, int pos);
char *word_get(const char *str, int idx);

int uintlen(uint64_t value);
char *xdirname(const char *path);

bool ends_with_nocase(const char *str, const char *end);
bool starts_with_nocase(const char *str, const char *start);

#endif
