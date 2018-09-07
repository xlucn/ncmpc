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

#include "Bindings.hxx"
#include "command.hxx"
#include "KeyName.hxx"
#include "i18n.h"
#include "ncmpc_curses.h"
#include "util/Macros.hxx"

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <glib.h>
#include <signal.h>
#include <unistd.h>

const char *
KeyBindings::GetKeyNames(command_t command, bool all) const
{
	const auto &b = key_bindings[size_t(command)];

	static char keystr[80];

	g_strlcpy(keystr, key2str(b.keys[0]), sizeof(keystr));
	if (!all)
		return keystr;

	for (const auto key : b.keys) {
		if (key == 0)
			break;

		g_strlcat(keystr, " ", sizeof(keystr));
		g_strlcat(keystr, key2str(key), sizeof(keystr));
	}
	return keystr;
}

command_t
KeyBindings::FindKey(int key) const
{
	assert(key != 0);

	for (size_t i = 0; i < size_t(CMD_NONE); ++i) {
		for (const auto key2 : key_bindings[i].keys)
			if (key2 == key)
				return command_t(i);
	}

	return CMD_NONE;
}

#ifndef NCMPC_MINI

bool
KeyBindings::Check(char *buf, size_t bufsize) const
{
	bool success = true;

	for (size_t i = 0; i < size_t(CMD_NONE); ++i) {
		for (const auto key : key_bindings[i].keys) {
			if (key == 0)
				break;

			command_t cmd;
			if ((cmd = FindKey(key)) != command_t(i)) {
				if (buf) {
					snprintf(buf, bufsize,
						 _("Key %s assigned to %s and %s"),
						 key2str(key),
						 get_key_command_name(command_t(i)),
						 get_key_command_name(cmd));
				} else {
					fprintf(stderr,
						_("Key %s assigned to %s and %s"),
						key2str(key),
						get_key_command_name(command_t(i)),
						get_key_command_name(cmd));
					fputc('\n', stderr);
				}
				success = false;
			}
		}
	}

	return success;
}

void
KeyBinding::WriteToFile(FILE *f, const command_definition_t &cmd,
			bool comment) const
{
	fprintf(f, "## %s\n", cmd.description);
	if (comment)
		fprintf(f, "#");
	fprintf(f, "key %s = ", cmd.name);

	if (keys.front() == 0) {
		fputs("0\n\n", f);
		return;
	}

	bool first = true;
	for (const auto key : keys) {
		if (key == 0)
			break;

		if (first)
			first = false;
		else
			fprintf(f, ",  ");

		if (key < 256 && (isalpha(key) || isdigit(key)))
			fprintf(f, "\'%c\'", key);
		else
			fprintf(f, "%d", key);
	}
	fprintf(f,"\n\n");
}

bool
KeyBindings::WriteToFile(FILE *f, int flags) const
{
	const auto *cmds = get_command_definitions();

	if (flags & KEYDEF_WRITE_HEADER)
		fprintf(f, "## Key bindings for ncmpc (generated by ncmpc)\n\n");

	for (size_t i = 0; i < size_t(CMD_NONE) && !ferror(f); ++i) {
		if (key_bindings[i].modified || flags & KEYDEF_WRITE_ALL) {
			key_bindings[i].WriteToFile(f, cmds[i],
						    flags & KEYDEF_COMMENT_ALL);
		}
	}

	return ferror(f) == 0;
}

#endif /* NCMPC_MINI */
