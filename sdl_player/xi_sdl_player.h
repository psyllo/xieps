#ifndef __XI_SDL_PLAYER_H__
#define __XI_SDL_PLAYER_H__

#include "SDL/SDL.h"
#include "SDL/SDL_thread.h"
#include "SDL/SDL_framerate.h"
#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <libintl.h>
/* Gettext '_(String)' macro pretty much recommended:
   http://www.gnu.org/software/gettext/manual/html_node/Mark-Keywords.html */
#define _(String) gettext (String)

#include "../src/xi_data.h"
#include "xi_sdl.h"

/** Primay control functions:
   - These functions collaborate with the xieps API
*/
void       xisp_play(XIStory* story, gdouble playback_rate, gdouble record_frame_rate, gdouble future_buffer_rate);

/* IDEAS: */
/* void       xisp_freeze(XIStory* story, gboolean bln); // stop all updating, only allow render */
/* void       xisp_recorded_frame_goto_closest_relative(XIStory* story, gdouble plus_or_minus_seconds); */
/* void       xisp_recorded_frame_goto_closest_at_time(XIStory* story, gdouble elapsed_time_in_seconds); */
/* void       xisp_recorded_frame_history_clear(XIStory* story); */

#endif
