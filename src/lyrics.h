/* ncmpc
 * Copyright (C) 2008 Max Kellermann <max@duempel.org>
 * This project's homepage is: http://www.musicpd.org
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
 */

#ifndef LYRICS_H
#define LYRICS_H

#include <glib.h>

enum lyrics_loader_result {
	LYRICS_SUCCESS,
	LYRICS_BUSY,
	LYRICS_FAILED
};

struct lyrics_loader;

void lyrics_init(void);

void lyrics_deinit(void);

struct lyrics_loader *
lyrics_load(const char *artist, const char *title);

void
lyrics_free(struct lyrics_loader *loader);

enum lyrics_loader_result
lyrics_result(struct lyrics_loader *loader);

const GString *
lyrics_get(struct lyrics_loader *loader);

#endif
