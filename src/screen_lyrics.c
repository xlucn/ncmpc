/*
 * (c) 2006 by Kalle Wallin <kaw@linux.se>
 * Copyright (C) 2008 Max Kellermann <max@duempel.org>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "config.h"
#ifndef DISABLE_LYRICS_SCREEN
#include <sys/stat.h>
#include "ncmpc.h"
#include "options.h"
#include "mpdclient.h"
#include "command.h"
#include "screen.h"
#include "screen_utils.h"
#include "strfsong.h"
#include "lyrics.h"
#include "gcc.h"

#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <ncurses.h>
#include <unistd.h>
#include <stdio.h>

static list_window_t *lw = NULL;
static int lyrics_text_rows = -1;

static struct {
	const struct mpd_song *song;

	char *artist, *title;

	struct lyrics_loader *loader;

	GPtrArray *lines;
} current;

static void
screen_lyrics_abort(void)
{
	if (current.loader != NULL) {
		lyrics_free(current.loader);
		current.loader = NULL;
	}

	if (current.artist != NULL) {
		g_free(current.artist);
		current.artist = NULL;
	}

	if (current.title != NULL) {
		g_free(current.title);
		current.artist = NULL;
	}

	current.song = NULL;
}

static void
screen_lyrics_clear(void)
{
	guint i;

	assert(current.loader == NULL ||
	       lyrics_result(current.loader) == LYRICS_SUCCESS);

	current.song = NULL;

	for (i = 0; i < current.lines->len; ++i)
		g_free(g_ptr_array_index(current.lines, i));

	g_ptr_array_set_size(current.lines, 0);
}

static void
screen_lyrics_set(const GString *str)
{
	const char *p, *eol, *next;

	screen_lyrics_clear();

	p = str->str;
	while ((eol = strchr(p, '\n')) != NULL) {
		char *line;

		next = eol + 1;

		/* strip whitespace at end */

		while (eol > p && (unsigned char)eol[-1] <= 0x20)
			--eol;

		/* create copy and append it to current.lines*/

		line = g_malloc(eol - p + 1);
		memcpy(line, p, eol - p);
		line[eol - p] = 0;

		g_ptr_array_add(current.lines, line);

		/* reset control characters */

		for (eol = line + (eol - p); line < eol; ++line)
			if ((unsigned char)*line < 0x20)
				*line = ' ';

		p = next;
	}

	if (*p != 0)
		g_ptr_array_add(current.lines, g_strdup(p));
}

static int
screen_lyrics_poll(void)
{
	assert(current.loader != NULL);

	switch (lyrics_result(current.loader)) {
	case LYRICS_BUSY:
		return 0;

	case LYRICS_SUCCESS:
		screen_lyrics_set(lyrics_get(current.loader));
		lyrics_free(current.loader);
		current.loader = NULL;
		return 1;

	case LYRICS_FAILED:
		lyrics_free(current.loader);
		current.loader = NULL;
		screen_status_message (_("No lyrics"));
		return -1;
	}

	assert(0);
	return -1;
}

static void
screen_lyrics_load(struct mpd_song *song)
{
	char buffer[MAX_SONGNAME_LENGTH];

	assert(song != NULL);

	screen_lyrics_abort();
	screen_lyrics_clear();

	strfsong(buffer, sizeof(buffer), "%artist%", song);
	current.artist = g_strdup(buffer);

	strfsong(buffer, sizeof(buffer), "%title%", song);
	current.title = g_strdup(buffer);

	current.loader = lyrics_load(current.artist, current.title);
}

static void lyrics_paint(screen_t *screen, mpdclient_t *c);

static FILE *create_lyr_file(const char *artist, const char *title)
{
	char path[1024];

	snprintf(path, 1024, "%s/.lyrics",
		 getenv("HOME"));
	mkdir(path, S_IRWXU);

	snprintf(path, 1024, "%s/.lyrics/%s - %s.txt",
		 getenv("HOME"), artist, title);

	return fopen(path, "w");
}

static int store_lyr_hd(void)
{
	FILE *lyr_file;
	unsigned i;

	lyr_file = create_lyr_file(current.artist, current.title);
	if (lyr_file == NULL)
		return -1;

	for (i = 0; i < current.lines->len; ++i)
		fprintf(lyr_file, "%s\n",
			(const char*)g_ptr_array_index(current.lines, i));

	fclose(lyr_file);
	return 0;
}

static const char *
list_callback(unsigned idx, mpd_unused int *highlight, mpd_unused void *data)
{
	if (current.lines == NULL || idx >= current.lines->len)
		return "";

	return g_ptr_array_index(current.lines, idx);
}


static void
lyrics_screen_init(WINDOW *w, int cols, int rows)
{
	current.lines = g_ptr_array_new();
	lw = list_window_init(w, cols, rows);
	lw->flags = LW_HIDE_CURSOR;
}

static void
lyrics_resize(int cols, int rows)
{
	lw->cols = cols;
	lw->rows = rows;
}

static void
lyrics_exit(void)
{
	list_window_free(lw);

	screen_lyrics_abort();
	screen_lyrics_clear();

	g_ptr_array_free(current.lines, TRUE);
	current.lines = NULL;
}

static void
lyrics_open(mpd_unused screen_t *screen, mpdclient_t *c)
{
	if (c->song != NULL && c->song != current.song)
		screen_lyrics_load(c->song);
	else if (current.loader != NULL)
		screen_lyrics_poll();
}


static const char *
lyrics_title(char *str, size_t size)
{
	if (current.loader != NULL)
		return "Lyrics (loading)";
	else if (current.artist != NULL && current.title != NULL &&
		 current.lines->len > 0) {
		snprintf(str, size, "Lyrics: %s - %s",
			 current.artist, current.title);
		return str;
	} else
		return "Lyrics";
}

static void
lyrics_paint(mpd_unused screen_t *screen, mpd_unused mpdclient_t *c)
{
	lw->clear = 1;
	list_window_paint(lw, list_callback, NULL);
	wrefresh(lw->w);
}


static void
lyrics_update(mpd_unused screen_t *screen, mpd_unused mpdclient_t *c)
{
	if( lw->repaint ) {
		list_window_paint(lw, list_callback, NULL);
		wrefresh(lw->w);
		lw->repaint = 0;
	}
}


static int
lyrics_cmd(screen_t *screen, mpdclient_t *c, command_t cmd)
{
	lw->repaint=1;
	switch(cmd) {
	case CMD_LIST_NEXT:
		if (current.lines != NULL && lw->start+lw->rows < current.lines->len+1)
			lw->start++;
		return 1;
	case CMD_LIST_PREVIOUS:
		if( lw->start >0 )
			lw->start--;
		return 1;
	case CMD_LIST_FIRST:
		lw->start = 0;
		return 1;
	case CMD_LIST_LAST:
		if ((unsigned)lyrics_text_rows > lw->rows)
			lw->start = lyrics_text_rows - lw->rows;
		else
			lw->start = 0;
		return 1;
	case CMD_LIST_NEXT_PAGE:
		lw->start = lw->start + lw->rows - 1;
		if (lw->start + lw->rows >= (unsigned)lyrics_text_rows + 1) {
			if ((unsigned)lyrics_text_rows + 1 > lw->rows)
				lw->start = lyrics_text_rows + 1 - lw->rows;
			else
				lw->start = 0;
		}
		return 1;
	case CMD_LIST_PREVIOUS_PAGE:
		if (lw->start > lw->rows)
			lw->start -= lw->rows;
		else
			lw->start = 0;
		return 1;
	case CMD_SELECT:
		/* XXX */
		if (current.loader != NULL) {
			int ret = screen_lyrics_poll();
			if (ret != 0)
				lyrics_paint(NULL, NULL);
		}
		return 1;
	case CMD_INTERRUPT:
		if (current.loader != NULL) {
			screen_lyrics_abort();
			screen_lyrics_clear();
		}
		return 1;
	case CMD_ADD:
		if (current.loader == NULL && current.artist != NULL &&
		    current.title != NULL && store_lyr_hd() == 0)
			screen_status_message (_("Lyrics saved!"));
		return 1;
	case CMD_LYRICS_UPDATE:
		if (c->song != NULL) {
			screen_lyrics_load(c->song);
			lyrics_paint(NULL, NULL);
		}
		return 1;
	default:
		break;
	}

	lw->selected = lw->start+lw->rows;
	if (screen_find(screen,
			lw,  lyrics_text_rows,
			cmd, list_callback, NULL)) {
		/* center the row */
		lw->start = lw->selected - (lw->rows / 2);
		if (lw->start + lw->rows > (unsigned)lyrics_text_rows) {
			if (lw->rows < (unsigned)lyrics_text_rows)
				lw->start = lyrics_text_rows - lw->rows;
			else
				lw->start = 0;
		}
		return 1;
	}

	return 0;
}

static list_window_t *
lyrics_lw(void)
{
  return lw;
}

screen_functions_t *
get_screen_lyrics(void)
{
  static screen_functions_t functions;

  memset(&functions, 0, sizeof(screen_functions_t));
  functions.init   = lyrics_screen_init;
  functions.exit   = lyrics_exit;
  functions.open = lyrics_open;
  functions.close  = NULL;
  functions.resize = lyrics_resize;
  functions.paint  = lyrics_paint;
  functions.update = lyrics_update;
  functions.cmd    = lyrics_cmd;
  functions.get_lw = lyrics_lw;
  functions.get_title = lyrics_title;

  return &functions;
}
#endif /* ENABLE_LYRICS_SCREEN */
