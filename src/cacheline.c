/* cacheline.c - Delays printing of last newline
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
#include <getopt.h>		/* Gnulib/GNU Libc/POSIX */
#include <stdbool.h>		/* Gnulib/C99/POSIX */
#include <stdlib.h>		/* C89 */
#include <stdio.h>		/* C89 */
#include "gettext.h"		/* Gnulib */
#define _(String) gettext(String)
#include "getline.h"		/* Gnulib/GNU Libc */
#include "version-etc.h"	/* Gnulib */

enum {
    HELP_OPT = 256,
    VERSION_OPT
};

static struct option long_opts[] = {
    { "help", no_argument, NULL, HELP_OPT },
    { "version", no_argument, NULL, VERSION_OPT },
    { 0, 0, 0, 0 }
};

const char version_etc_copyright[] = "Copyright (C) 2001-2005 Oskar Liljeblad";

int
main(int argc, char **argv)
{
    char *buf = NULL;
    size_t buflen = 0;
    bool keep = false;

    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);

    while (true) {
        int c;

        c = getopt_long(argc, argv, "", long_opts, NULL);
        if (c == -1)
            break;

        switch (c) {
        case HELP_OPT:
            printf(_("Usage: %s [OPTION]...\n"), argv[0]);
            puts(_("Read from in and write to standard out, delaying newline until a complete\n"
                 "line has been read.\n"));
            printf(_("      --help     display this help and exit\n"));
            printf(_("      --version  output version information and exit\n"));
            printf(_("\nReport bugs to <%s>.\n"), PACKAGE_BUGREPORT);
            exit(EXIT_SUCCESS);
        case VERSION_OPT:
            version_etc(stdout, "cacheline", PACKAGE, VERSION, "Oskar Liljeblad", NULL);
            exit(EXIT_SUCCESS);
        }
    }

    for (;;) {
        ssize_t linelen;

        linelen = getline(&buf, &buflen, stdin);
        if (linelen < 0) {
	    if (ferror(stdin)) {
	    	perror(argv[0]);
	    	exit(1);
	    }
	    break;
	}

	if (linelen > 0) {
	    if (keep) {
	    	fputc('\n', stdout);
		keep = false;
	    }
	    if (buf[linelen-1] == '\n') {
	    	keep = true;
	    	linelen--;
	    }
	    fwrite(buf, linelen, 1, stdout);
	    fflush(stdout);
	}
    }
    
    exit(0);
}
