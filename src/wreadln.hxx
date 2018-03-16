/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2018 The Music Player Daemon Project
 * Project homepage: http://musicpd.org
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef WREADLN_H
#define WREADLN_H

#include "config.h"
#include "History.hxx"
#include "ncmpc_curses.h"

class Completion;

/* Note, wreadln calls curs_set() and noecho(), to enable cursor and
 * disable echo. wreadln will not restore these settings when exiting! */
char *
wreadln(WINDOW *w,            /* the curses window to use */
	const char *prompt, /* the prompt string or nullptr */
	const char *initial_value, /* initial value or nullptr for a empty line
				    * (char *) -1 = get value from history */
	unsigned x1,              /* the maximum x position or 0 */
	History *history, /* a pointer to a history list or nullptr */
	Completion *completion    /* a GCompletion structure or nullptr */
	);

char *
wreadln_masked(WINDOW *w,
	       const char *prompt,
	       const char *initial_value,
	       unsigned x1);

#endif
