/* remark.c - Option-parsing and main function for regex-markup.
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
/* POSIX */
#include <unistd.h>
/* C89 */
#include <signal.h>
#include <stdlib.h>
#include <string.h>
/* gnulib */
#include <getopt.h>
#include "dirname.h"
#include "progname.h"
#include "getline.h"
#include "version-etc.h"
/* Gettext */
#include <gettext.h>
#define _(String) gettext(String)
/* common */
#include "common/strbuf.h"
#include "common/string-utils.h"
#include "common/io-utils.h"
#include "common/error.h"
#include "common/llist.h"
#include "common/intutil.h"
/* regex-markup */
#include "remark.h"

#define PROGRAM "remark"

enum {
	VERSION_OPT	= 1000,
	HELP_OPT,
};

static uint32_t prepend_len = 0;
static uint32_t append_len = 0;
static char *prepend_string = "";
static char *append_string = "";

static char *short_opts = "a:f:p:r:w:";
static struct option long_opts[] = {
	{ "prepend",   required_argument, NULL, 'p' },
	{ "append",    required_argument, NULL, 'a' },
	{ "retain",    required_argument, NULL, 'r' },
	{ "width",     required_argument, NULL, 'w' },
	{ "wrap",      required_argument, NULL, 'f' },
	{ "version",   no_argument,	  NULL, VERSION_OPT },
	{ "help",      no_argument,	  NULL, HELP_OPT },
	{ 0 },
};

static void display_help(void);
static void display_version(void);

const char version_etc_copyright[] = "Copyright (C) 2004-2005 Oskar Liljeblad";

static void
display_version(void)
{
	version_etc(stdout, PROGRAM, PACKAGE, VERSION, "Oskar Liljeblad", NULL);
}

static void
display_help(void)
{
	printf(_("Usage: %s [OPTION]... FILE [TEXT]\n\
Read lines of text from standard in (or use TEXT as input string), highlight\n\
according to rules defined in FILE, and print to standard out.\n\
\n\
  -p, --prepend=STRING       string to prepend to all split lines (except last)\n\
  -a, --append=STRING        string to append to all split lines (except first)\n\
  -r, --retain=COUNT         copy characters from first line to wrapped ones\n\
  -w, --width=COLUMNS        wrapping width\n\
  -f, --wrap=TYPE            specifies wrapping type (word/char/none).\n\
      --help                 display this help and exit\n\
      --version              output version information and exit\n\
\n\
Report bugs to <%s>.\n"), program_name, PACKAGE_BUGREPORT);
}

static void
try_line(RemarkScript *script, RemarkInput *input, char *text)
{
	strbuf_set(input->mb.buffer, text);
	input->mb.bufferlen = strlen(text);
	strbuf_set(input->append_mb.buffer, append_string);
	input->append_mb.bufferlen = append_len;
	strbuf_set(input->prepend_mb.buffer, prepend_string);
	input->prepend_mb.bufferlen = prepend_len;

	if (execute_script(script, input)) {
		fputs(strbuf_buffer(input->mb.buffer), stdout);
		fflush(stdout);
	}
}

int
main(int argc, char **argv)
{
	struct sigaction action;
	RemarkInput input;
	int c;

	set_program_name(argv[0]);

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	memset(&action, 0, sizeof(struct sigaction));
	/* 2003-04-11 : I don't know why this is good in the first place... */
	/*if (!isatty(STDIN_FILENO)) {
		action.sa_handler = SIG_IGN;
		action.sa_flags = SA_RESTART;
		if (sigaction(SIGINT, &action, NULL) < 0)
			die_errno(NULL);
	}*/

	while ((c = getopt_long (argc, argv, short_opts, long_opts, NULL)) != -1) {
		switch (c) {
		case 'p':
			prepend_string = optarg;
			break;
		case 'a':
			append_string = optarg;
			break;
		case 'r':
			if (!parse_uint32(optarg, &wrap_retain))
				die(_("invalid retain value: %s"), optarg);
			break;
		case 'w':
			if (!parse_uint32(optarg, &wrap_width))
				die(_("invalid width: %s"), optarg);
			break;
		case 'f':
			if (!identify_wrapper(optarg))
				die(_("invalid wrapping type: %s"), optarg);
			break;
		case VERSION_OPT:
			display_version();
			exit(0);
			break;
		case HELP_OPT:
			display_help();
			exit(0);
			break;
		case '?':
			exit(1);
		}
	}

	append_len = strlen(append_string);
	prepend_len = strlen(prepend_string);
	if (wrapper == 0)
		wrapper = WRAPPER_CHAR;
	if (wrap_retain + append_len + prepend_len >= wrap_width)
		die(_("retain, append and prepend too long"));

	init_input(&input);

	if (argc-optind >= 2) {
		RemarkScript *script = parse_script(argv[optind]);
		for (c = optind+1; c < argc; c++)
			try_line(script, &input, argv[c]);
		free_script(script);
	}
	else {
		RemarkScript *script = NULL;
		size_t linesize = 0;
		char *line = NULL;

		script = parse_script(argc > optind ? argv[optind] : NULL);
		while (getline(&line, &linesize, stdin) != -1) {
			chomp(line);
			try_line(script, &input, line);
		}
		free(line);
		free_script(script);

		if (ferror(stdin))
			die_errno(_("cannot read from standard in"));
	}

	free_input(&input);

	exit(0);
}
