#ifndef __XI_SDL_H__
#define __XI_SDL_H__

#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#include <SDL/SDL_framerate.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_image.h>
#include <glib.h>
#include <stdlib.h>
#include <libintl.h>
/* Gettext '_(String)' macro pretty much recommended:
   http://www.gnu.org/software/gettext/manual/html_node/Mark-Keywords.html */
#define _(String) gettext (String)

#include "../src/xi_enums.h"
#include "../src/xi_data.h"

typedef void    (*XISdlLoopFn) (gdouble elapsed, gpointer data);
typedef gboolean(*XISdlEventFn)(gdouble elapsed, gpointer data,
                                SDL_Event *event);


GQuark  xi_sdl_error_quark();
void    xi_sdl_start_main_loop(XISdlLoopFn  loop_fn,
                               XISdlEventFn event_fn,
                               gpointer     data);
SDL_Surface* xi_sdl_init(gint preferred_w, gint preferred_h,
                         gint preferred_bpp, GError **err);
void xi_sdl_cleanup();

void
xi_sdl_show_img(SDL_Surface *screen   , GHashTable *named_buckets,
                gchar const *name     , gchar const *path,
                gboolean use_alpha    , gboolean use_alpha_channel,
                guint8 new_alpha      , guint8 prev_alpha,
                gboolean use_colorkey , guint8 colorkey_red,
                guint8 colorkey_green , guint8 colorkey_blue,
                gint16 dest_x, gint16 dest_y, SDL_Rect *src,
                GError **err);

void xi_sdl_update_screen(SDL_Surface *screen);
void xi_sdl_show_framerate(SDL_Surface *display);


#endif
