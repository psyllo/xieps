#include "xi_sdl.h"

gboolean loop_running = TRUE;
gdouble elapsed = 0.0;
Uint32 elapsed_diff = 0;
char message256[256];
gdouble last_sprintf_framerate = 0.0;
gint framerate = 30; /* 200 seems to be max. 201 slows it down badly. */



GQuark
xi_sdl_error_quark()
{
  g_quark_from_string("xi-sdl-error-quark");
}

void
xi_sdl_print_video_modes()
{
  SDL_Rect **modes;

  /* Get available fullscreen/hardware modes */
  modes = SDL_ListModes(NULL, SDL_FULLSCREEN | SDL_HWSURFACE);

  /* Check is there are any modes available */
  if(modes == (SDL_Rect **)0) {
    g_error(_("No modes available!"));
  }

  /* Check if our resolution is restricted */
  if(modes == (SDL_Rect **)-1) {
    g_message(_("All resolutions available."));
  }else{
    /* Print valid modes */
    g_message(_("Available Modes"));
    for(int i = 0; modes[i]; ++i) {
      g_message("  %d x %d", modes[i]->w, modes[i]->h);
    }
  }
}

SDL_Surface *
xi_sdl_init(gint preferred_w, gint preferred_h, gint preferred_bpp, GError **err)
{
  if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) < 0) {
    gchar *xi_msg = _("SDL init error");
    gchar *sdl_msg = SDL_GetError();
    g_warning("%s: %s", xi_msg, sdl_msg);
    g_return_val_if_fail(err == NULL || *err == NULL, NULL);
    g_set_error(err, xi_sdl_error_quark(), XI_SDL_INIT_ERROR, "%s: %s", xi_msg, sdl_msg);
    return NULL;
  }
  atexit(SDL_Quit);

  GError *tmp_err = NULL;
  gint suggested_w   = preferred_w;
  gint suggested_h   = preferred_h;
  gint suggested_bpp = preferred_bpp;

  /* TODO: Validate requested mode.
     Use: SDL_VideoModeOK(int width, int height, int bpp, Uint32 flags)
     See: http://sdl.beuc.net/sdl.wiki/SDL_VideoModeOK */
  {
    suggested_bpp = SDL_VideoModeOK(preferred_w, preferred_h, preferred_bpp, SDL_HWSURFACE);
    if(suggested_bpp == 0) {
      /* Unsupported mode */
      g_warning(_("Unsupported mode %dx%d."), preferred_w, preferred_h);
      /* TODO: Attempt different mode */
    }else if(suggested_bpp != preferred_bpp){
      g_message(_("SDL recommended %dx%d@%d instead of %dx%d@%d."),
                suggested_w, suggested_h, suggested_bpp,
                preferred_w, preferred_h, preferred_bpp);
    }else{
      g_message(_("The video mode you wanted was accepted by SDL: %dx%d@%d."),
                suggested_w, suggested_h, suggested_bpp);
    }
  }
  g_message(_("Using video mode: %dx%d@%d."), suggested_w, suggested_h, suggested_bpp);

  SDL_Surface *screen;
  {
    screen = SDL_SetVideoMode(suggested_w, suggested_h, suggested_bpp,
                              SDL_DOUBLEBUF
                              | SDL_HWSURFACE
                              // | SDL_SWSURFACE
                              | SDL_ASYNCBLIT
                              // | SDL_RESIZABLE
                              //| SDL_NOFRAME
                              );
    if (screen == NULL) {
      gchar *xi_msg = _("SDL unable to set video mode desired.");
      gchar *sdl_msg = SDL_GetError();
      g_warning("%s: %s", xi_msg, sdl_msg);
      g_return_val_if_fail(err == NULL || *err == NULL, NULL);
      g_set_error(err, xi_sdl_error_quark(), XI_SDL_INIT_ERROR, "%s: %s", xi_msg, sdl_msg);
      return NULL;
    }
  }

  xi_sdl_print_video_modes();

  return screen;
}

void
handle_exit()
{
  loop_running = FALSE;
}

void
xi_sdl_cleanup()
{
  SDL_Quit();
}

void
handle_key_down(SDLKey sym, SDLMod mod, Uint16 unicode) {
  switch(sym) {
  case SDLK_ESCAPE:
    handle_exit();
    break;
  default:
    break;
  }
}

void
xi_sdl_handle_event(SDL_Event *event)
{
  switch(event->type) {
  case SDL_KEYDOWN:
    handle_key_down(event->key.keysym.sym,
                    event->key.keysym.mod,
                    event->key.keysym.unicode);
    break;
  }
}

/*!
  \param loop_fn called every interation of the loop
  \param event_fn called whenever there is an event (like mouse, kb input)
  \param data passed as an argument to the callback functions arguments
*/
void
xi_sdl_start_main_loop(XISdlLoopFn  loop_fn,
                       XISdlEventFn event_fn,
                       gpointer     data)
{
  SDL_Event event;
  FPSmanager fpsm;

  SDL_initFramerate(&fpsm);
  SDL_setFramerate(&fpsm, framerate);
  framerate = SDL_getFramerate(&fpsm);

  gint i = 0;
  while(loop_running == TRUE) {
    g_debug(_("%s: loop iteration %d"), __FUNCTION__, i++);
    elapsed_diff = SDL_framerateDelay(&fpsm); /* delay to fixed rate */
    elapsed += elapsed_diff / 1000.0;
    while(SDL_PollEvent(&event)) {
      if(event_fn == NULL || !event_fn(elapsed, data, &event)) {
        xi_sdl_handle_event(&event);
      }
    }
    if(loop_fn != NULL) {
      loop_fn(elapsed, data);
    }
  }
  return;
}


void
xi_sdl_show_framerate(SDL_Surface *surface)
{
  last_sprintf_framerate += elapsed_diff;
  if(last_sprintf_framerate >= framerate) {
    last_sprintf_framerate = 0.0;
    gint fps = 1000 / elapsed_diff;
    sprintf(message256, "Framerate: %3d/sec", fps);
    g_debug("Framerate: %d", fps);
  }
  stringColor(surface, 10, 20, message256, 0xFFFFFFFF);
}

void
xi_sdl_destroy_surface(gpointer surface)
{
  SDL_FreeSurface(surface);
}

void
xi_sdl_show_img(SDL_Surface *screen   , GHashTable *named_buckets,
                gchar const *name     , gchar const *path,
                gboolean use_alpha    , gboolean use_alpha_channel,
                guint8 new_alpha      , guint8 prev_alpha,
                gboolean use_colorkey , guint8 colorkey_red,
                guint8 colorkey_green , guint8 colorkey_blue,
                gint16 dest_x, gint16 dest_y, SDL_Rect *src,
                GError **err)
{
  gboolean image_pulled_from_cache = FALSE;
  SDL_Surface *image = NULL;
  SDL_Rect dest;

  g_return_if_fail(screen        != NULL);
  g_return_if_fail(named_buckets != NULL);
  g_return_if_fail(name          != NULL);

  gchar *cache_key = "SDL_Surface_Cache";
  GHashTable *cache = g_hash_table_lookup(named_buckets, cache_key);
  if(cache == NULL) {
    /* Create a new cache and add it to named_buckets */
    cache = g_hash_table_new_full(g_str_hash, g_str_equal,
                                  g_free, xi_sdl_destroy_surface);
    g_hash_table_insert(named_buckets, g_strdup(cache_key), cache);
  }

  /* Pull image from cache or load file */
  image = g_hash_table_lookup(cache, name);
  if(image == NULL) {
    image = IMG_Load(path);
  }else{
    image_pulled_from_cache = TRUE;
  }

  if(image == NULL) {
    gchar *xi_msg = _("SDL unable to load bitmap");
    gchar *sdl_msg = SDL_GetError();
    g_warning("%s: %s", xi_msg, sdl_msg);
    g_return_if_fail(err == NULL || *err == NULL);
    g_set_error(err, xi_sdl_error_quark(), XI_SDL_INIT_ERROR, "%s: %s", xi_msg, sdl_msg);
    return;
  }


  /*
    NOTE: There should be a file in this project called
    'docs/alpha-performance.org' that contains helpful performance
    related observations regarding alpha settings for images.
  */

  gboolean is_new_image     = !image_pulled_from_cache;
  gboolean alpha_changed    = new_alpha    != prev_alpha;
  gboolean colorkey_set     = is_new_image && use_colorkey;

  if(colorkey_set) {
    int colorkey_failure = SDL_SetColorKey(image, SDL_SRCCOLORKEY,
                                           SDL_MapRGB(image->format,
                                                      colorkey_red,
                                                      colorkey_green,
                                                      colorkey_blue));
    if(colorkey_failure) {
      gchar *xi_msg = _("SDL: SDL_SetColorKey");
      gchar *sdl_msg = SDL_GetError();
      g_warning("%s: %s", xi_msg, sdl_msg);
      g_return_if_fail(err == NULL || *err == NULL);
      g_set_error(err, xi_sdl_error_quark(), XI_SDL_INIT_ERROR, "%s: %s", xi_msg, sdl_msg);
      return;
    }
  }

  gboolean needs_conversion = is_new_image || alpha_changed || colorkey_set;

  if(needs_conversion) {

    if(use_alpha || use_alpha_channel) {
      SDL_SetAlpha(image, SDL_SRCALPHA | SDL_RLEACCEL, new_alpha);
    }

    SDL_Surface *tmp = NULL;
    if(use_alpha_channel) {
      tmp = SDL_DisplayFormatAlpha(image);
    }else if(use_alpha){
      tmp = SDL_DisplayFormat(image);
    }else{
      tmp = SDL_DisplayFormat(image);
    }
    image = tmp;

    /* If prev image exists in cache it will be free'd by
       'value_destroy_func' set on the hash table. The new key passed
       in will be destroyed by 'key_destroy_func' if a key already
       exists. */
    g_hash_table_insert(cache, g_strdup(name), image);

    g_debug(_("%s: image converted and cached: '%s' (use_alpha_channel=%d)"),
            __FUNCTION__, name, use_alpha_channel);
  }else{
    g_debug(_("%s: image '%s' used AS IS (no conversion performed)."),
            __FUNCTION__, name);
  }

  /* The SDL blitting function needs to know how
     much data to copy. We provide this with
     SDL_Rect structures, which define the
     source and destination rectangles. The
     areas should be the same; SDL does not
     currently handle image stretching. */
  SDL_Rect stack_rect;
  if(src == NULL) {
    src = &stack_rect;
    src->x = 0;
    src->y = 0;
    src->h = image->h;
    src->w = image->w;
  }
  dest.x = dest_x;
  dest.y = dest_y;
  dest.w = src->w;
  dest.h = src->h;

  SDL_BlitSurface(image, src, screen, &dest);
}

void
xi_sdl_update_screen(SDL_Surface *screen) {
  // When not using double buffer, use update rect:
  //SDL_UpdateRect(screen, 0, 0, 0, 0);

  // When using double buffer, use flip:
  SDL_Flip(screen);
}

/* TODO: Use Glib event system
   https://developer.gnome.org/glib/stable/glib-The-Main-Event-Loop.html */
