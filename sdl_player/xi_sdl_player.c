#include "xi_sdl_player.h"
#include "../demo/xieps_demo.h" /* TODO: hard-coded demo stuff */


static SDL_Surface *display = NULL;
static gchar const * const assets_root_dir = "story/data/assets/";
static FILE *logfile = NULL;
static char *log_filename = "xieps_sdl_player.log";
static gchar const * const SEQS_WANTING_INPUT_KEY = "seqs_wanting_input";

void
xisp_blit(XIStory *story, XIDrawable *drawable) {
  g_return_if_fail(drawable       != NULL);
  g_return_if_fail(drawable->name != NULL);
  g_return_if_fail(drawable->pos  != NULL);
  SDL_Rect stack_rect;
  SDL_Rect *source_rect = NULL;
  if(drawable->rect != NULL) {
    source_rect = &stack_rect;
    source_rect->x = drawable->rect->x;
    source_rect->y = drawable->rect->y;
    source_rect->h = drawable->rect->h;
    source_rect->w = drawable->rect->w;
  }
  gchar *path = g_strconcat(assets_root_dir, drawable->name, NULL);
  xi_sdl_show_img(display,
                  story->named_buckets,
                  drawable->instance_name,
                  path,
                  drawable->use_alpha,
                  drawable->use_alpha_channel,
                  drawable->alpha,
                  drawable->prev_alpha,
                  drawable->use_colorkey,
                  drawable->colorkey_red,
                  drawable->colorkey_green,
                  drawable->colorkey_blue,
                  drawable->pos->screen_x,
                  drawable->pos->screen_y,
                  source_rect,
                  NULL);
  g_free(path);

  /*
    Alpha bookkeeping here is important for performance. The image
    SDL_Surface will be converted using SDL_DisplayFormat each time
    the alpha is detected to have changed.

    TODO: Move this assignment elsewhere
   */
  drawable->prev_alpha = drawable->alpha;
}

gint
xisp_sort_dawables_by_z(gconstpointer drawableA,
                        gconstpointer drawableB,
                        gpointer use_data)
{
  g_return_val_if_fail(drawableA != NULL, 0);
  g_return_val_if_fail(drawableB != NULL, 0);

  XIDrawable *a = (XIDrawable*)drawableA;
  XIDrawable *b = (XIDrawable*)drawableB;

  if(a->pos != NULL && b->pos != NULL) {
    if(a->pos->z > b->pos->z) {
      return 1;
    }else if(a->pos->z < b->pos->z) {
      return -1;
    }
  }

  return 0; /* Default to equal */
}

void
xisp_draw_all(XIStory *story, GSequence *drawables)
{
  g_return_if_fail(drawables != NULL);
  if(g_sequence_get_length(drawables) <= 0) {
    return;
  }

  GSequenceIter *next = g_sequence_get_begin_iter(drawables);
  XIDrawable *draw = NULL;
  guint len = g_sequence_get_length(drawables);
  guint i = 0;
  while(!g_sequence_iter_is_end(next)) {
    i++;
    draw = g_sequence_get(next);
    if(draw != NULL) {
      g_debug(_("Drawing (%d of %d): '%s'"), i, len, draw->instance_name);
      xisp_blit(story, draw);
    }else{
      g_debug(_("%s encountered NULL drawable"), __FUNCTION__);
    }
    next = g_sequence_iter_next(next);
  }
  xi_sdl_show_framerate(display);
  xi_sdl_update_screen(display);
}

void
xisp_sound_all(XIStory *story, GSequence *sounds)
{
  // TODO: write this function
  /*
    - Sounds: Make a list of sounds to play. Check their status in async player
    per unique id perhaps generated by: (play:sequence+elapsed+sound_file or stop:sequence+elapsed+sound_file),
    to ensure that a sound file play event is only run once.
  */
}

void
xisp_get_action_lists(XISequence *seq,
                      GSequence *drawables,
                      GSequence *sounds)
{
  g_return_if_fail(seq       != NULL);
  g_return_if_fail(drawables != NULL);
  g_return_if_fail(sounds    != NULL);

  if(!xi_sequence_is_marked_started(seq) || xi_sequence_is_marked_done(seq)) {
    return;
  }

  /* Process drawables of current sequence */
  if(seq->drawables != NULL && g_hash_table_size(seq->drawables) > 0) {
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, seq->drawables);
    while(g_hash_table_iter_next(&iter, &key, &value)) {
      if(value != NULL) {
        g_sequence_append(drawables, value);
      }else{
        g_debug(_("Seq '%s' contains a NULL drawable."), seq->instance_name);
      }
    }
  }

  /* Process sounds of current sequence */
  if(seq->sounds != NULL && g_hash_table_size(seq->sounds) > 0) {
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, seq->sounds);
    while(g_hash_table_iter_next(&iter, &key, &value)) {
      if(value != NULL) {
        g_sequence_append(sounds, value);
      }else{
        g_debug(_("Sequence '%s' contains a NULL sound."), seq->instance_name);
      }
    }
  }

  /* Recur on state_specific */
  if(seq->state_name != NULL && g_hash_table_size(seq->state_specific) > 0) {
    XISequence *sss = g_hash_table_lookup(seq->state_specific, seq->state_name);
    if(sss != NULL) {
      xisp_get_action_lists(sss, drawables, sounds);
    }else{
      g_debug(_("Sequence '%s' state_name '%s' names a state_specific sequence that does not exist."),
              seq->name, seq->state_name);
    }
  }

  /* Recur on children */
  if(seq->children != NULL && g_hash_table_size(seq->children) > 0) {
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, seq->children);
    while(g_hash_table_iter_next(&iter, &key, &value)) {
      xisp_get_action_lists((XISequence*)value, drawables, sounds);
    }
  }

  /* Sort Drawables so they have proper overlapping */
  g_sequence_sort(drawables, xisp_sort_dawables_by_z, NULL);

}

void
xisp_update(XIStory *story)
{
  g_return_if_fail(story != NULL);
  g_return_if_fail(story->root_seq != NULL);

  GSequence *drawables = g_sequence_new(NULL);
  GSequence *sounds = g_sequence_new(NULL);

  xisp_get_action_lists(story->root_seq, drawables, sounds);

  xisp_draw_all(story, drawables);
  xisp_sound_all(story, sounds);

  g_sequence_free(drawables);
  g_sequence_free(sounds);
}

void
xisp_iteration(gdouble elapsed, XIStory *story)
{
  xi_update(story, elapsed);
  xisp_update(story);
}

/*
  TODO: Use GQuarks not strings. Match with XI_INPUT_XY via Quark.

  \return XI_EVENT_SUBTYPE_NA if no match found.
 */
gint
xisp_event_subtype_for_listener_key(gchar * const key)
{
  if(g_strcmp0(key, "input_xy") == 0) {
    return XI_INPUT_XY;
  }
  return XI_EVENT_SUBTYPE_NA;
}


gboolean
xisp_sdl_event_is_keyboard_directional(SDL_Event *sdl_event)
{
  g_return_val_if_fail(sdl_event != NULL, FALSE);

  int sym = sdl_event->key.keysym.sym;
  return (sym == SDLK_LEFT || sym == SDLK_RIGHT || sym == SDLK_UP ||
          sym == SDLK_DOWN);
}

XIEvent*
xisp_event_convert_keyboard_directional_to_xi_input_xy(SDL_Event *sdl_event)
{
  g_return_val_if_fail(sdl_event != NULL, NULL);
  g_return_val_if_fail(xisp_sdl_event_is_keyboard_directional(sdl_event), NULL);

  XIInput_XY *input_xy = xi_input_xy_new();;
  SDLKey sym = sdl_event->key.keysym.sym;
  Uint8 key_state = sdl_event->key.state;

  if(sym == SDLK_LEFT) {
    if(key_state == SDL_PRESSED)
      input_xy->x = 1.0;
    else
      input_xy->x = 0.0;
  }else if(sym == SDLK_RIGHT) {
    if(key_state == SDL_PRESSED)
      input_xy->x = -1.0;
    else
      input_xy->x = 0.0;
  }else if(sym == SDLK_UP) {
    if(key_state == SDL_PRESSED)
      input_xy->y = 1.0;
    else
      input_xy->y = 0.0;
  }else if(sym == SDLK_DOWN) {
    if(key_state == SDL_PRESSED)
      input_xy->y = -1.0;
    else
      input_xy->y = 0.0;
  }

  XIEvent *event = xi_event_new_empty(XI_INPUT_EVENT);
  event->name = g_strdup("input_xy");
  event->type = XI_INPUT_EVENT;
  event->subtype = XI_INPUT_XY;
  event->type_data = input_xy;
  event->type_data_destroy = g_free;

  return event;
}

XIEvent*
xisp_sdl_event_to_xi_event(SDL_Event *sdl_event)
{
  g_return_val_if_fail(sdl_event != NULL, NULL);

  if(xisp_sdl_event_is_keyboard_directional(sdl_event)) {
    return xisp_event_convert_keyboard_directional_to_xi_input_xy(sdl_event);
  }

  return NULL;
}

/*! This function accepts an SDL_Event and knows how to convert it and
  send it along to Xieps.

  \return TRUE if event was handled
*/
gboolean
xisp_handle_event(gdouble elapsed, XIStory *story, SDL_Event *sdl_event)
{
  g_return_val_if_fail(story                != NULL, FALSE);
  g_return_val_if_fail(story->named_buckets != NULL, FALSE);

  GHashTable *seqs_wanting_input = g_hash_table_lookup(story->named_buckets,
                                                       SEQS_WANTING_INPUT_KEY);
  if(seqs_wanting_input == NULL || g_hash_table_size(seqs_wanting_input) == 0) {
    return FALSE;
  }

  XIEvent *event = xisp_sdl_event_to_xi_event(sdl_event);
  if(event == NULL) return FALSE;

  GHashTableIter iter;
  gpointer key, value;
  g_hash_table_iter_init(&iter, seqs_wanting_input);
  while(g_hash_table_iter_next(&iter, &key, &value)) {
    XISequence *seq = value;
    if(seq->event_mask & XI_INPUT_EVENT) {
      event->source_seq = seq;
      xi_sequence_fire_input_event(event);
      return TRUE;
    }
  }


  return FALSE;
}

/*
    Ideas:
    - To pause: playback_rate = 0.0
    - To fast forward: playback_rate > 1.0
    - To slow down: playback_rate < 1.0
*/
void
xisp_play(XIStory *story, gdouble playback_rate, gdouble record_frame_rate,
          gdouble future_buffer_rate)
{
  xi_sdl_start_main_loop((XISdlLoopFn)xisp_iteration,
                         (XISdlEventFn)xisp_handle_event,
                         (gpointer)story);
}

gboolean
xisp_init_story(XIStory *story, guint curr_w, guint curr_h, guint curr_bpp)
{
  return xi_init_story(story, curr_w, curr_h, curr_bpp);
}


/*! If there is a listener for any input subtype listeners then
  update the seq 'event_mask' to indicate that it listens to
  input events */
void
xisp_sequence_event_mask_update_by_examination(XISequence *seq)
{
  g_return_if_fail(seq != NULL);

  if(seq->listeners == NULL && g_hash_table_size(seq->listeners) == 0) {
    return;
  }

  GHashTableIter iter;
  gpointer key, value;
  g_hash_table_iter_init(&iter, seq->listeners);
  while(g_hash_table_iter_next(&iter, &key, &value)) {
    gint subtype = 0;
    if((subtype = xisp_event_subtype_for_listener_key(key))
       != XI_EVENT_SUBTYPE_NA) {
      seq->event_mask = seq->event_mask | XI_INPUT_XY;
    }
  }
}

void
xisp_update_list_of_seqs_wanting_input(XIStory *story, XISequence *seq)
{
  g_return_if_fail(story                != NULL);
  g_return_if_fail(story->named_buckets != NULL);
  g_return_if_fail(seq                  != NULL);

  /* Get seqs_wanting_input table. If not exists then create it */
  GHashTable *seqs_wanting_input =
    g_hash_table_lookup(story->named_buckets, SEQS_WANTING_INPUT_KEY);
  if(seqs_wanting_input == NULL) {
    seqs_wanting_input = g_hash_table_new(g_str_hash, g_str_equal);
    g_hash_table_insert(story->named_buckets,
                        g_strdup(SEQS_WANTING_INPUT_KEY),
                        seqs_wanting_input);
  }

  /* Ignore seqs that are have not started or are done */
  // TODO: Why does this check not work? Not getting marked as started?
  if(!xi_sequence_is_marked_started(seq) || xi_sequence_is_marked_done(seq)) {
    // LEFT_OFF
    //return; TODO: should return if this check fails.
  }

  /* Check listeners of current sequence for wanting input */
  if(seq->listeners != NULL && seq->event_mask & XI_INPUT_EVENT) {
    g_hash_table_insert(seqs_wanting_input, seq->instance_name, seq);
  }

  /* Recur on state_specific */
  if(seq->state_name != NULL && g_hash_table_size(seq->state_specific) > 0) {
    XISequence *sss = g_hash_table_lookup(seq->state_specific, seq->state_name);
    if(sss != NULL) {
      xisp_update_list_of_seqs_wanting_input(story, sss);
    }else{
      g_debug(_("Sequence '%s' state_name '%s' names a state_specific sequence that does not exist."),
              seq->name, seq->state_name);
    }
  }

  /* Recur on children */
  if(seq->children != NULL && g_hash_table_size(seq->children) > 0) {
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, seq->children);
    while(g_hash_table_iter_next(&iter, &key, &value)) {
      xisp_update_list_of_seqs_wanting_input(story, (XISequence*)value);
    }
  }
}

SDL_Surface*
xisp_init(XIStory *story)
{
  GError *err = NULL;
  SDL_Surface *display = xi_sdl_init(story->natural_w,
                                     story->natural_h,
                                     story->natural_bpp, &err);
  if(err != NULL) {
    /* TODO: Print or propagate error correctly */
    g_error(_("Problem initializing SDL: '%s'"), err->message);
    return FALSE;
  }
  if(!xisp_init_story(story,
                      story->natural_w,
                      story->natural_h,
                      story->natural_bpp)) {
    SDL_FreeSurface(display);
    display = NULL;
  }
  xisp_update_list_of_seqs_wanting_input(story, story->root_seq);

  // TODO: LEFT_OFF
  //SDL_EnableKeyRepeat(200, 200);

  return display;
}

/*****************************************************************************/
void xisp_log_to_file(gchar const *log_domain, GLogLevelFlags log_level,
                      gchar const *message, gpointer user_data)
{
  /* TODO: Are there Glib functions to replace these fopen, printf,
     fprintf, fclose ones? The answer I think is GLib IO Channels:
     https://developer.gnome.org/glib/stable/glib-IO-Channels.html */
  if(logfile == NULL) {
    logfile = fopen(log_filename, "a");
  }
  if (logfile == NULL) {
    printf(_("Cannot log to file: %s\n"), log_filename);
    printf(_("Rerouted to console: %s\n"), message);
    return;
  }
  fprintf (logfile, "%s\n", message);
}

void
xisp_setup_logging()
{
  g_log_set_handler(NULL,
                    G_LOG_LEVEL_MESSAGE | G_LOG_LEVEL_WARNING
                    | G_LOG_LEVEL_ERROR | G_LOG_LEVEL_DEBUG
                    | G_LOG_LEVEL_INFO | G_LOG_LEVEL_CRITICAL,
                    xisp_log_to_file, NULL);
}

void
xisp_cleanup() {
  if(display != NULL) {
    SDL_FreeSurface(display);
    display = NULL;
  }
  if(logfile != NULL) {
    fclose(logfile);
  }
}

int main(int argc, char **argv)
{
  xisp_setup_logging();

  /* TODO: Loading of the demo is hard-coded. */
  XIStory *demo_story = demo_build_entire_story();
  g_return_val_if_fail(demo_story != NULL, 1);

  /* Play */
  if(display = xisp_init(demo_story)) {
    xisp_play(demo_story, 1.0, 0.01, 1.5);
  }else{
    g_critical(_("Story init failed."));
  }

  /* Clean-up */
  xi_story_free(demo_story);
  xisp_cleanup();
  xi_sdl_cleanup();
}