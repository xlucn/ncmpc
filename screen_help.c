#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <ncurses.h>

#include "config.h"
#include "libmpdclient.h"
#include "mpc.h"
#include "command.h"
#include "screen.h"
#include "screen_utils.h"
#include "screen_help.h"

typedef struct
{
  char highlight;
  command_t command;
  char *text;
} help_text_row_t;

static help_text_row_t help_text[] = 
{
  { 1, CMD_NONE,  "          Keys   " },
  { 0, CMD_NONE,  "        --------" },
  { 0, CMD_STOP,           "Stop" },
  { 0, CMD_PAUSE,          "Pause" },
  { 0, CMD_TRACK_NEXT,     "Next track" },
  { 0, CMD_TRACK_PREVIOUS, "Prevoius track (back)" },
  { 0, CMD_VOLUME_DOWN,    "Volume down" },
  { 0, CMD_VOLUME_UP,      "Volume up" },
  { 0, CMD_NONE, " " },
  { 0, CMD_LIST_NEXT,      "Move cursor up" },
  { 0, CMD_LIST_PREVIOUS,  "Move cursor down" },
  { 0, CMD_LIST_FIND,      "Find" },
  { 0, CMD_LIST_RFIND,     "find backward" },
  { 0, CMD_LIST_FIND_NEXT, "Find next" },
  { 0, CMD_LIST_RFIND_NEXT,"Find previuos" },
  { 0, CMD_TOGGLE_FIND_WRAP, "Toggle find mode" },
  { 0, CMD_NONE, " " },
  { 0, CMD_SCREEN_NEXT,   "Change screen" },
  { 0, CMD_SCREEN_HELP,   "Help screen" },
  { 0, CMD_SCREEN_PLAY,   "Playlist screen" },
  { 0, CMD_SCREEN_FILE,   "Browse screen" },
  { 0, CMD_QUIT,          "Quit" },
  { 0, CMD_NONE, " " },
  { 0, CMD_NONE, " " },
  { 1, CMD_NONE, "    Keys - Playlist screen " },
  { 0, CMD_NONE, "  --------------------------" },
  { 0, CMD_PLAY,           "Play selected entry" },
  { 0, CMD_DELETE,         "Delete selected entry from playlist" },
  { 0, CMD_SHUFFLE,        "Shuffle playlist" },
  { 0, CMD_CLEAR,          "Clear playlist" },
  { 0, CMD_SAVE_PLAYLIST,  "Save playlist" },
  { 0, CMD_REPEAT,         "Toggle repeat mode" },
  { 0, CMD_RANDOM,         "Toggle random mode" },
  { 0, CMD_NONE, " " },
  { 0, CMD_NONE, " " },
  { 1, CMD_NONE, "    Keys - Browse screen " },
  { 0, CMD_NONE, "  ------------------------" },
  { 0, CMD_PLAY,            "Enter directory/Load playlist" },
  { 0, CMD_SELECT,          "Add/remove song from playlist" },
  { 0, CMD_DELETE_PLAYLIST, "Delete playlist" },
  { 0, CMD_NONE, " " },
  { 0, CMD_NONE, " " },
  { 1, CMD_NONE, " " PACKAGE " version " VERSION },
  { 0, CMD_NONE, NULL }
};

static int help_text_rows = -1;



static char *
list_callback(int index, int *highlight, void *data)
{
  static char buf[256];

  if( help_text_rows<0 )
    {
      help_text_rows = 0;
      while( help_text[help_text_rows].text )
	help_text_rows++;
    }

  *highlight = 0;
  if( index<help_text_rows )
    {
      *highlight = help_text[index].highlight;
      if( help_text[index].command == CMD_NONE )
	return help_text[index].text;
      snprintf(buf, 256, 
	       "%20s : %s", 
	       command_get_keys(help_text[index].command),
	       help_text[index].text);
      return buf;
    }

  return NULL;
}


void 
help_open(screen_t *screen, mpd_client_t *c)
{
}

void 
help_close(screen_t *screen, mpd_client_t *c)
{
}

void 
help_paint(screen_t *screen, mpd_client_t *c)
{
  list_window_t *w = screen->helplist;

  w->clear = 1;
  list_window_paint(screen->helplist, list_callback, NULL);
  wrefresh(screen->helplist->w);
}

void 
help_update(screen_t *screen, mpd_client_t *c)
{  
  list_window_t *w = screen->helplist;
 
  if( w->repaint )
    {
      list_window_paint(screen->helplist, list_callback, NULL);
      wrefresh(screen->helplist->w);
      w->repaint = 0;
    }
}


int 
help_cmd(screen_t *screen, mpd_client_t *c, command_t cmd)
{
  int retval;

  retval = list_window_cmd(screen->helplist, help_text_rows, cmd);
  if( !retval )
    return screen_find(screen, c, 
		       screen->helplist, help_text_rows,
		       cmd, list_callback);

  return retval;
}
