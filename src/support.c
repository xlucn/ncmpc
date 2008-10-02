/* 
 * $Id$
 *
 * (c) 2004 by Kalle Wallin <kaw@linux.se>
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

#include "support.h"
#include "i18n.h"
#include "config.h"

#include <assert.h>
#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFSIZE 1024

extern void screen_status_printf(const char *format, ...);

static gboolean noconvert = TRUE;

size_t
my_strlen(const char *str)
{
	assert(str != NULL);

	if (g_utf8_validate(str, -1, NULL)) {
		size_t len = g_utf8_strlen(str, -1);
		size_t width = 0;
		gunichar c;

		while (len--) {
			c = g_utf8_get_char(str);
			width += g_unichar_iswide(c) ? 2 : 1;
			str += g_unichar_to_utf8(c, NULL);
		}

		return width;
	} else
		return strlen(str);
}

char *
remove_trailing_slash(char *path)
{
	int len;

	if (path == NULL)
		return NULL;

	len = strlen(path);
	if (len > 1 && path[len - 1] == '/')
		path[len - 1] = '\0';

	return path;
}

char *
lowerstr(char *str)
{
	gsize i;
	gsize len = strlen(str);

	if (str == NULL)
		return NULL;

	i = 0;
	while (i < len && str[i]) {
		str[i] = tolower(str[i]);
		i++;
	}
	return str;
}


#ifndef HAVE_BASENAME
char *
basename(char *path)
{
	char *end;

	assert(path != NULL);

	path = remove_trailing_slash(path);
	end = path + strlen(path);

	while (end > path && *end != '/')
		end--;

	if (*end == '/' && end != path)
		return end+1;

	return path;
}
#endif /* HAVE_BASENAME */


#ifndef HAVE_STRCASESTR
char *
strcasestr(const char *haystack, const char *needle)
{
	assert(haystack != NULL);
	assert(needle != NULL);

	return strstr(lowerstr(haystack), lowerstr(needle));
}
#endif /* HAVE_STRCASESTR */

// FIXME: utf-8 length
char *
strscroll(char *str, char *separator, int width, scroll_state_t *st)
{
	gchar *tmp, *buf;
	gsize len, size;

	assert(str != NULL);
	assert(separator != NULL);
	assert(st != NULL);

	if( st->offset==0 ) {
		st->offset++;
		return g_strdup(str);
	}

	/* create a buffer containing the string and the separator */
	size = strlen(str)+strlen(separator)+1;
	tmp = g_malloc(size);
	g_strlcpy(tmp, str, size);
	g_strlcat(tmp, separator, size);
	len = my_strlen(tmp);

	if (st->offset >= len)
		st->offset = 0;

	/* create the new scrolled string */
	size = width+1;
	if (g_utf8_validate(tmp, -1, NULL) ) {
		int ulen;
		buf = g_malloc(size*6);// max length of utf8 char is 6
		g_utf8_strncpy(buf, g_utf8_offset_to_pointer(tmp,st->offset), size);
		if( (ulen = g_utf8_strlen(buf, -1)) < width )
			g_utf8_strncpy(buf+strlen(buf), tmp, size - ulen - 1);
	} else {
		buf = g_malloc(size);
		g_strlcpy(buf, tmp+st->offset, size);
		if (strlen(buf) < (size_t)width)
			g_strlcat(buf, tmp, size);
	}
	if( time(NULL)-st->t >= 1 ) {
		st->t = time(NULL);
		st->offset++;
	}
	g_free(tmp);
	return buf;
}

void
charset_init(gboolean disable)
{
  noconvert = disable;
}

char *
utf8_to_locale(const char *utf8str)
{
	gchar *str;
	gsize rb, wb;
	GError *error;

	assert(utf8str != NULL);

	if (noconvert)
		return g_strdup(utf8str);

	rb = 0; /* bytes read */
	wb = 0; /* bytes written */
	error = NULL;
	str = g_locale_from_utf8(utf8str,
				 strlen(utf8str),
				 &wb, &rb,
				 &error);
	if (error) {
		const char *charset;

		g_get_charset(&charset);
		screen_status_printf(_("Error: Unable to convert characters to %s"),
				     charset);
		g_error_free(error);
		return g_strdup(utf8str);
	}

	return str;
}

char *
locale_to_utf8(const char *localestr)
{
	gchar *str;
	gsize rb, wb;
	GError *error;

	assert(localestr != NULL);

	if (noconvert)
		return g_strdup(localestr);

	rb = 0; /* bytes read */
	wb = 0; /* bytes written */
	error = NULL;
	str = g_locale_to_utf8(localestr,
			       strlen(localestr),
			       &wb, &rb,
			       &error);
	if (error) {
		screen_status_printf(_("Error: Unable to convert characters to UTF-8"));
		g_error_free(error);
		return g_strdup(localestr);
	}

	return str;
}
