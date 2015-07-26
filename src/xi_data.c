#include "xi_data.h"

/*
  LEFT_OFF: Point-to-Point Smooth Camera movements.
  Example: Clicking a link will smoothly send the camera to the link
  location which would be a point to center on.
*/


static gboolean XI_EVENT_CASCADING = TRUE; /*!< Prevent gaps between an event

                                             firing and being handled. */

void
xi_sequence_setup_hooks_if_null(XISequence *seq)
{
  g_return_if_fail(seq != NULL);
  if(seq->hooks == NULL) {
    seq->hooks = g_new(GHookList, 1);
    g_hook_list_init(seq->hooks, sizeof(GHook));
  }
}

/*!

  TODO: Use Quarks instead

  \return XI_EVENT_SUBTYPE_NA if no match.
 */
gint
xi_event_input_subtype_for_listener_key(gchar const *key)
{
  if(key == NULL) return XI_EVENT_SUBTYPE_NA;

  if(g_strcmp0(key, "input_xy") == 0) {
    return XI_INPUT_XY;
  }

  return XI_EVENT_SUBTYPE_NA;
}

void
xi_sequence_setup_listeners(XISequence *seq)
{
  g_return_if_fail(seq != NULL);

  if(seq->listeners == NULL) {
    seq->listeners =
      g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
                            (GDestroyNotify)g_hook_list_clear);
  }
}

XICamera*
xi_camera_new()
{
  XICamera *cam = g_new(XICamera, 1);

  cam->point = xi_driveable_point_new();

  return cam;
}

XIInput_XY*
xi_input_xy_new()
{
  XIInput_XY *ixy = g_new(XIInput_XY, 1);
  ixy->up    = 0;
  ixy->down  = 0;
  ixy->left  = 0;
  ixy->right = 0;
  ixy->up_changed    = FALSE;
  ixy->down_changed  = FALSE;
  ixy->left_changed  = FALSE;
  ixy->right_changed = FALSE;
  return ixy;
}

/*!
  Where XIInput_XY and XIDriveablePoint meet.
 */
void
xi_handle_input_xy_for_driveable_point(XIEvent *event)
{
  g_return_if_fail(event != NULL);

  if(event->type    == XI_INPUT_EVENT &&
     event->subtype == XI_INPUT_XY) {

    XIInput_XY       *input_xy = (XIInput_XY*)      event->type_data;
    XIDriveablePoint *point    = (XIDriveablePoint*)event->handler_data;

    g_return_if_fail(input_xy != NULL);
    g_return_if_fail(point != NULL);

    if(input_xy->up_changed == TRUE) {
      point->input_y = input_xy->up - point->input_down;
      point->input_up = input_xy->up;
    }
    if(input_xy->down_changed == TRUE) {
      point->input_y = -input_xy->down + point->input_up;
      point->input_down = input_xy->down;
    }
    if(input_xy->left_changed == TRUE) {
      point->input_x = input_xy->left - point->input_right;
      point->input_left = input_xy->left;
    }
    if(input_xy->right_changed == TRUE) {
      point->input_x = -input_xy->right + point->input_left;
      point->input_right = input_xy->right;
    }

    g_debug("%s: point now {input_x=%g, input_y=%g, input_z=%g}",
	    __FUNCTION__, point->input_x, point->input_y, point->input_z);
  }
}

/*!
  \brief Allow XIDriveablePoint to be controlled by input
  \param point the point that will be updated by input
  \return TRUE on success
 */
gboolean
xi_connect_input_xy_to_driveable_point(XISequence *seq, XIDriveablePoint *point)
{
  g_return_val_if_fail(seq   != NULL, FALSE);
  g_return_val_if_fail(point != NULL, FALSE);

  /* Add a listener that will receive input events */
  // TODO: match with XI_INPUT_XY via Quark
  GHook *event_hook =
    xi_sequence_add_listener(seq, XI_INPUT_EVENT, XI_INPUT_XY,
                             g_strdup("input_xy"),
                             (GHookFunc)xi_handle_input_xy_for_driveable_point,
                             point, NULL);

  /* Add a hook to sequence hooks that will update the point */
  xi_sequence_setup_hooks_if_null(seq);

  XIDriveablePointUpdate *update = xi_driveable_point_update_new();
  update->point = point;
  update->elapsed = &seq->elapsed;
  GHook *update_point_hook   = g_hook_alloc(seq->hooks);
  update_point_hook->func    = xi_driveable_point_update;
  update_point_hook->data    = update;
  update_point_hook->destroy = xi_driveable_point_update_free;
  g_hook_append(seq->hooks, update_point_hook);

  GHook *point_hook   = g_hook_alloc(seq->hooks);
  point_hook->func    = xi_driveable_point_mover;
  point_hook->data    = (gpointer)point;
  point_hook->destroy = NULL;
  g_hook_append(seq->hooks, point_hook);

  return(event_hook != NULL);
}

gboolean
xi_sequence_connect_input_to_camera_xy(XISequence *seq)
{
  g_return_val_if_fail(seq != NULL, FALSE);

  if(seq->camera == NULL) {
    seq->camera = xi_camera_new();
  }

  xi_connect_input_xy_to_driveable_point(seq, seq->camera->point);
}

/*! \brief Sets the camera position on seq. New camera is created if NULL.
 */
void
xi_sequence_set_camera(XISequence *seq, gdouble x, gdouble y, gdouble z) {
  g_return_if_fail(seq != NULL);

  if(seq->camera == NULL) {
    seq->camera = xi_camera_new();
  }

  seq->camera->point->x = x;
  seq->camera->point->y = y;
  seq->camera->point->z = z;
}

void
xi_camera_free(XICamera *camera) {
  if(camera == NULL) return;
  xi_driveable_point_free(camera->point);
  g_free(camera);
}

XICamera*
xi_camera_copy(XICamera *camera) {
  if(camera == NULL) return NULL;

  XICamera *new_camera = g_new(XICamera, 1);

  new_camera->point = xi_driveable_point_copy(camera->point);

  return new_camera;
}

/*!
  TODO: Make this work with a negative elapsed.
  \return newly allocated rect
*/
XIRect*
xi_drawable_frames_next(XIDrawableFrames *dframes, gdouble elapsed_since_prev)
{
  g_return_val_if_fail(dframes != NULL, NULL);
  g_return_val_if_fail(dframes->frames != NULL, NULL);

  gdouble safety_sum = 0;
  for(guint i = 0; i < dframes->count; i++) {
    safety_sum += dframes->frames[i]->duration;
  }
  if(safety_sum <= 0) {
    g_warning(_("%s: For %s/%s, sum of frame durations is not greater than 0."),
              __FUNCTION__, dframes->drawable_name, dframes->series_name);
  }

  gdouble remaining_in_frame =
    dframes->frames[dframes->offset]->duration - dframes->elapsed_in_frame;

  if(remaining_in_frame > elapsed_since_prev) {
    /* Offset does not change */
    dframes->elapsed_in_frame += elapsed_since_prev;
  }else{
    /* Move offset ahead as far as is needed */
    guint max_i = 0;
    guint sane_limit = 1000000; /* Just in case */
    gdouble sum = remaining_in_frame;
    guint i = dframes->offset + 1;
    do {
      if(i >= dframes->count) i = 0;
      sum += dframes->frames[i]->duration;
      if(sum > elapsed_since_prev || max_i > sane_limit) break;
      i++;
      if(++max_i > sane_limit) break;
    }while(1);
    dframes->offset = i;
    dframes->elapsed_in_frame =
      elapsed_since_prev - (sum - dframes->frames[i]->duration);
  }

  return xi_timed_frame_rect_copy(dframes->frames[dframes->offset]);
}

/*! \brief return position as string
 */
gchar*
xi_position_to_string(XIPosition *pos)
{
  if(pos == NULL) {
    return NULL;
  }

  gchar *message256 = g_malloc_n(256, sizeof(gchar));
  g_sprintf(message256, "{screen_x=%g, screen_y=%g, screen_z=%g, x=%g, y=%g, z=%g}",
            pos->screen_x, pos->screen_y, pos->screen_z,
            pos->x,        pos->y,        pos->z);

  return message256;
}

/*
  Fields not free'd:

  - parent: No upward traversal of sequence tree for free'ing.

*/
void
xi_sequence_free(XISequence *seq)
{
  if(seq == NULL) return;

  g_free(seq->instance_name);
  g_free(seq->name);

  /* 'parent' intentionally not free'd */

  /* Free children */
  if(seq->children != NULL) {
    g_hash_table_destroy(seq->children);
  }

  g_free(seq->state_name);

  /* Free state_specific */
  if(seq->state_specific != NULL) {
    g_hash_table_destroy(seq->state_specific);
  }

  /* Free drawables */
  if(seq->drawables != NULL) {
    g_hash_table_destroy(seq->drawables);
  }

  /* Free movements */
  if(seq->movements != NULL) {
    g_hash_table_destroy(seq->movements);
  }

  /* Free sounds */
  if(seq->sounds != NULL) {
    g_hash_table_destroy(seq->sounds);
  }

  /* Free hooks */
  if(seq->hooks != NULL) {
    g_hook_list_clear(seq->hooks);
    g_free(seq->hooks);
  }

  xi_position_free(seq->pos);
  g_free(seq->scale_mode);
  xi_camera_free(seq->camera);
  g_free(seq->start_on);

  /* Free listeners */
  if(seq->listeners != NULL) {
    g_hash_table_destroy(seq->listeners);
  }

  g_free(seq);
}

/*!
 */
void
xi_story_free(XIStory *story)
{
  if(story == NULL) return;
  g_free(story->name);
  g_free(story->title);
  g_free(story->revision);
  g_free(story->scale_mode);
  xi_sequence_free(story->root_seq);

  /* Free named_buckets */
  if(story->named_buckets != NULL) {
    g_hash_table_destroy(story->named_buckets);
  }

  g_free(story);
}

/*!
  \brief Free event, except 'seq' and 'data' fields.
*/
void
xi_event_free(XIEvent *evt)
{
  g_return_if_fail(evt != NULL);

  if(evt->type_data != NULL) {
    if(evt->type_data_destroy != NULL) {
      evt->type_data_destroy(evt->type_data);
    }else{
      g_debug(_("%s: Event '%s' 'type_data' had no 'type_data_destroy'."),
              __FUNCTION__, evt->name);
    }
  }

  if(evt->handler_data != NULL) {
    if(evt->handler_data_destroy != NULL) {
      evt->handler_data_destroy(evt->handler_data);
    }else{
      g_debug(_("%s: Event '%s' 'handler_data' had no 'handler_data_destroy'."),
              __FUNCTION__, evt->name);
    }
  }

  if(evt->name != NULL) {
    g_free(evt->name);
  }

  g_free(evt);
}

/*!
  \brief Pointers copied for 'source_seq', 'type_data*', 'handler_data*'
*/
XIEvent*
xi_event_shallow_copy(XIEvent *evt)
{
  XIEvent *new_evt = g_new(XIEvent, 1);
  new_evt->type                 = evt->type;
  new_evt->subtype              = evt->subtype;
  new_evt->source_seq           = evt->source_seq;
  new_evt->name                 = g_strdup(evt->name);
  new_evt->when                 = evt->when;
  new_evt->x                    = evt->x;
  new_evt->y                    = evt->y;
  new_evt->z                    = evt->z;
  new_evt->speed                = evt->speed;
  new_evt->type_data            = evt->type_data;
  new_evt->type_data_destroy    = evt->type_data_destroy;
  new_evt->handler_data         = evt->handler_data;
  new_evt->handler_data_destroy = evt->handler_data_destroy;
  return new_evt;
}

XIEvent*
xi_event_new_empty(XIEventType event_type)
{
  xi_event_new(NULL, event_type, XI_EVENT_SUBTYPE_NA);
}

/*!
  \brief Add hook list of given event_name into listeners if does not exist.
  \param seq The listeners of this seq will be examined/modified.
  \param event_name is g_strdup()'d if used as new hash table key.
  \return New or existing hook list
 */
GHookList*
xi_sequence_setup_listeners_hook_list(XISequence *seq, gchar const *event_name)
{
  g_return_val_if_fail(seq        != NULL, NULL);
  g_return_val_if_fail(event_name != NULL, NULL);

  xi_sequence_setup_listeners(seq);

  GHookList *hook_list = NULL;
  hook_list = g_hash_table_lookup(seq->listeners, event_name);
  if(hook_list == NULL) {
    hook_list = g_new(GHookList, 1);
    g_hook_list_init(hook_list, sizeof(GHook));
    g_hash_table_insert(seq->listeners, g_strdup(event_name), hook_list);
  }

  return hook_list;
}

/*!
  Listeners should g_free() event when it is received.

  \param seq sequence to add event listener to.
  \param evt_type Event type
  \param evt_subtype Event subtype
  \param event_name Name of event to listen for. Is g_strdup()'d.
  \param type_data Will be attached to event.
  \param type_data_destroy attached to the event. Destroy type_data.
  \param event_handler Reciever of the event.
  \param handler_data Will be attached to event.
  \param handler_data_destroy attached to the event. Destroy handler_data.
  \return GHook that will be called when event fires. NULL on error.
*/
GHook*
xi_sequence_add_listener(XISequence     *seq,
                         XIEventType     evt_type,
                         gint            evt_subtype,
                         gchar const    *event_name,
                         GHookFunc       event_handler,
                         gpointer        handler_data,
                         GDestroyNotify  handler_data_destroy)
{
  g_debug(_("%s: for seq '%s' event_name '%s'"),
          __FUNCTION__, seq->instance_name, event_name);

  g_return_val_if_fail(seq                  != NULL, NULL);
  g_return_val_if_fail(seq->instance_name   != NULL, NULL);
  g_return_val_if_fail(event_name           != NULL, NULL);
  g_return_val_if_fail(event_handler        != NULL, NULL);

  seq->event_mask = seq->event_mask | evt_type;

  XIEvent *event = xi_event_new(seq, evt_type, evt_subtype,
                                .name=g_strdup(event_name),
                                .when=0, .x=0, .y=0, .z=0, .speed=0,
                                .handler_data         = handler_data,
                                .handler_data_destroy = handler_data_destroy);

  GHookList *hook_list = xi_sequence_setup_listeners_hook_list(seq, event_name);

  GHook *hook      = g_hook_alloc(hook_list);
  hook->data       = event;
  hook->func       = event_handler;
  hook->destroy    = (GDestroyNotify)xi_event_free;
  g_hook_append(hook_list, hook);

  g_debug(_("%s: finished for seq '%s' event_name '%s'. Result GHook = %p"),
          __FUNCTION__, seq->instance_name, event_name, hook);

  return hook;
}

XISequence*
xi_find_root_sequence(XISequence *seq)
{
  if(seq == NULL) return NULL;
  if(seq->parent == NULL) return seq;
  return xi_find_root_sequence(seq->parent);
}

XISequence*
xi_find_sequence_r(XISequence *seq, gchar *instance_name)
{
  g_debug(_("%s: In seq '%s', looking for '%s'"),
          __FUNCTION__, seq->instance_name, instance_name);

  if(seq == NULL) return NULL;
  if(instance_name == NULL) return NULL;

  if(g_strcmp0(seq->instance_name, instance_name) == 0) {
    return seq;
  }

  if(seq->children != NULL) {
    XISequence *result = NULL;
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, seq->children);
    while(g_hash_table_iter_next(&iter, &key, &value)) {
      result = xi_find_sequence_r((XISequence*)value, instance_name);
      if(result != NULL) {
        return result;
      }
    }
  }

  if(seq->state_specific != NULL) {
    XISequence *result = NULL;
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, seq->state_specific);
    while(g_hash_table_iter_next(&iter, &key, &value)) {
      result = xi_find_sequence_r((XISequence*)value, instance_name);
      if(result != NULL) {
        return result;
      }
    }
  }

  return NULL;
}

XISequence*
xi_find_sequence(XISequence *seq, gchar *instance_name)
{
  g_debug(_("%s: Finding seq: '%s'"), __FUNCTION__, seq->instance_name);
  g_return_val_if_fail(seq           != NULL, NULL);
  g_return_val_if_fail(instance_name != NULL, NULL);

  if(g_strcmp0(seq->instance_name, instance_name) == 0) {
    return seq;
  }

  g_debug(_("%s: Now recursively searching for: '%s'"),
          __FUNCTION__, instance_name);
  return xi_find_sequence_r(xi_find_root_sequence(seq), instance_name);
}

/*!
  \param any_seq Can be any seq in the seq tree.
  \param event_str Example: "foo-seq-instance-name:done"
  \param event_handler Event handler
  \param handler_data Value of event->handler_data of event that will be passed to event_handler
  \param handler_data_destroy Functions to destroy handler_data. Can be NULL.
  \return The GHook that was created and added to as the event handler. NULL on error.
*/
GHook*
xi_sequence_add_seq_listener_from_str(XISequence *any_seq, gchar const *event_str,
                                      GHookFunc event_handler,
                                      gpointer handler_data,
                                      GDestroyNotify handler_data_destroy)
{
  g_debug(_("%s: Entered"), __FUNCTION__);

  GHook *hook = NULL;
  gchar **strings = g_strsplit(event_str, ":", 2);

  if(strings == NULL) goto cleanup;
  if(*strings == NULL) goto cleanup;
  gchar *part1 = *strings;

  if(strings + 1 == NULL) goto cleanup;
  if(*(strings + 1) == NULL) goto cleanup;
  gchar *part2 = *(strings + 1);

  g_debug(_("%s: Split '%s' into '%s', '%s'"),
          __FUNCTION__, event_str, part1, part2);

  XISequence *seq = xi_find_sequence(any_seq, part1);
  if(seq == NULL) {
    g_critical(_("Failed to add event listener. Seq '%s' not found."), part1);
    goto cleanup;
  }else{
    g_debug(_("%s: found seq '%s'"), __FUNCTION__, seq->instance_name);
  }

  hook = xi_sequence_add_listener(seq, XI_SEQ_EVENT, XI_EVENT_SUBTYPE_NA,
                                  part2, event_handler, handler_data,
                                  handler_data_destroy);

 cleanup:
  g_strfreev(strings);
  g_debug(_("%s: Cleaned up"), __FUNCTION__);
  g_debug(_("%s: Leaving"), __FUNCTION__);
  return hook;
}

/*!
  \param target seq
  \param out the result
  \return TRUE on success
*/
gboolean
xi_sequence_remaining_time_full(XISequence *seq, gdouble *out)
{
  g_return_val_if_fail(seq != NULL, FALSE);
  *out = seq->duration - seq->elapsed;
  return TRUE;
}

/*!
  \brief calls xi_sequence_remaining_time_full()
  \return 0 or more.
*/
gdouble
xi_sequence_remaining_time(XISequence *seq)
{
  gdouble out = 0;
  if(!xi_sequence_remaining_time_full(seq, &out)) {
    return 0;
  }
  return out;
}


/*! Get remaining time until end of duration. If duration has been
  exceeded, get remaining time between prev_elapsed and end of
  duration and return TRUE, else FALSE prev_elapsed is also beyond
  duration.

  \brief Get remaining time until end of duration.
  \param seq Sequence to examine
  \param out The result. Always 0 or more (positive).
  \return TRUE if result is (duration - prev_elapsed). FALSE if result (duration - elapsed) or error occurred.
*/
gboolean
xi_sequence_duration_remainder(XISequence *seq, gdouble *out)
{
  *out = 0;
  g_return_val_if_fail(seq != NULL, FALSE);
  gdouble curr_remainder = seq->duration - seq->elapsed;
  if(curr_remainder > 0) {
    *out = curr_remainder;
  }else{
    gdouble prev_remainder = seq->duration - seq->prev_elapsed;
    if(prev_remainder > 0) {
      *out = prev_remainder;
      return TRUE;
    }
  }
  return FALSE;
}

/*
  \param seq Sequence to examine
  \return TRUE can and did end between previous and current elapsed. Also FALSE if seq is NULL.
*/
gboolean
xi_sequence_just_ended(XISequence *seq)
{
  g_return_val_if_fail(seq != NULL, TRUE);

  if(seq->duration_type == XI_DURATION_CONTINUOUS) {
    return FALSE;
  }

  gdouble ignore = 0;
  gboolean just_ended = xi_sequence_duration_remainder(seq, &ignore);

  if(!just_ended) {
    return (seq->duration     == 0 &&
            seq->prev_elapsed == 0 &&
            seq->elapsed       > 0);
  }

  return just_ended;
}

/*! Replace the existing hook->data, which should be an XIEvent, with
  the one given as a parameter. Several fields should not change
  however and are ignored on the new event and copied from the old
  one. Old event is destroyed except for the fields that were
  preserved.

  Fields that should not change:

  - source_seq           : Should be NULL. Copied from old event.
  - name                 : Should be NULL. Copied from old event. g_free()'d if not NULL.
  - handler_data         : Should be NULL. Copied from old event.
  - handler_data_destroy : Should be NULL. Copied from old event.

  \brief Replaces event data with given 'event' parameter.
  \param hook that will have its 'data' field modified
  \param The new event. Not all values are copied, however.
*/
void
xi_sequence_listeners_hook_marshal_for_event(GHook *hook, gpointer new_event)
{
  g_return_if_fail(hook       != NULL);
  g_return_if_fail(new_event  != NULL);
  g_return_if_fail(hook->data != NULL);

  XIEvent *new_evt = (XIEvent*)new_event;
  XIEvent *prev_evt = hook->data;

  if(new_evt->name != NULL) {
    g_free(new_evt->name);
  }

  new_evt->source_seq           = prev_evt->source_seq;
  new_evt->name                 = prev_evt->name;
  new_evt->handler_data         = prev_evt->handler_data;
  new_evt->handler_data_destroy = prev_evt->handler_data_destroy;

  hook->data = (gpointer)new_evt;

  prev_evt->source_seq           = NULL;
  prev_evt->name                 = NULL;
  prev_evt->handler_data         = NULL;
  prev_evt->handler_data_destroy = NULL;

  xi_event_free(prev_evt);
}

void
xi_event_hook_list_marshal_for_new_event(GHookList *hook_list, XIEvent *evt)
{
  g_hook_list_marshal(hook_list,
                      FALSE,
                      xi_sequence_listeners_hook_marshal_for_event,
                      evt);
}

/*
  \param source_seq Where the event came from.
  \param evt_type Event type
  \param evt_subtype Event subtype
  \param event_name 'done', 'started', etc.
  \param when Should be negative. Current elapsed + 'when' = when event fired.
  \param x Can be 0.
  \param y Can be 0.
  \param z Can be 0.
  \param speed Can be 0.
  \param type_data Specific to each type
  \param type_data_destroy How to destroy type_data
*/
void
xi_sequence_fire_event_full(XISequence *source_seq,
                            XIEventType evt_type, gint evt_subtype,
                            gchar const *event_name,
                            gdouble when,
                            gdouble x, gdouble y, gdouble z,
                            gdouble speed,
                            gpointer       type_data,
                            GDestroyNotify type_data_destroy)
{
  g_return_if_fail(source_seq != NULL);
  g_return_if_fail(event_name != NULL);

  g_debug(_("%s: Firing event '%s' from source_seq '%s'"),
          __FUNCTION__, event_name, source_seq->instance_name);

  if(source_seq->listeners == NULL) return;
  GHookList *hook_list = g_hash_table_lookup(source_seq->listeners, event_name);
  if(hook_list == NULL) return;

  g_debug(_("%s: there are listeners"), __FUNCTION__);

  // TODO: When are event->name and evt->name getting destroyed?
  // Is it correct to g_strdup(event_name) here?

  XIEvent *evt = xi_event_new(source_seq, evt_type, evt_subtype,
                              .name=g_strdup(event_name),
                              .when=when, .x=x, .y=y, .z=z, .speed=speed,
                              .type_data         = type_data,
                              .type_data_destroy = type_data_destroy);

  g_debug(_("%s: created event '%s' for source_seq '%s' of type %d[%d]"),
          __FUNCTION__, evt->name, source_seq->instance_name,
          evt_type, evt_subtype);

  g_debug(_("%s: marshaling event hook list"), __FUNCTION__);
  xi_event_hook_list_marshal_for_new_event(hook_list, evt);

  g_debug(_("%s: invoking hooks list"), __FUNCTION__);
  g_hook_list_invoke(hook_list, FALSE);

  g_debug(_("%s: Leaving"), __FUNCTION__);
}

void
xi_sequence_fire_seq_event(XISequence *source_seq, gchar const *event_name,
                           gdouble when)
{
  xi_sequence_fire_event_full(source_seq, XI_SEQ_EVENT, XI_EVENT_SUBTYPE_NA,
                              event_name, when,
                              0, 0, 0, 0, NULL, NULL);
}

void
xi_sequence_fire_input_event(XIEvent *input_event)
{
  g_return_if_fail(input_event                        != NULL);
  g_return_if_fail(input_event->source_seq            != NULL);
  g_return_if_fail(input_event->source_seq->listeners != NULL);
  g_return_if_fail(input_event->name                  != NULL);
  g_return_if_fail(input_event->type                  == XI_INPUT_EVENT);

  XISequence *source_seq = input_event->source_seq;

  g_debug(_("%s: Firing input event '%s' from source_seq '%s'"),
          __FUNCTION__, input_event->name, source_seq->instance_name);

  GHookList *hook_list = g_hash_table_lookup(source_seq->listeners,
                                             input_event->name);
  if(hook_list == NULL) return;

  g_debug(_("%s: there are listeners on '%s' for input event '%s'"),
          __FUNCTION__, source_seq->instance_name, input_event->name);

  g_debug(_("%s: marshaling input event listener hook list"), __FUNCTION__);
  xi_event_hook_list_marshal_for_new_event(hook_list, input_event);

  g_debug(_("%s: invoking hooks list"), __FUNCTION__);
  g_hook_list_invoke(hook_list, FALSE);

  g_debug(_("%s: Leaving"), __FUNCTION__);
}

void
xi_sequence_fire_event(XISequence *source_seq,
                       XIEventType evt_type, gint evt_subtype,
                       gchar const *event_name,
                       gdouble when,
                       gpointer       type_data,
                       GDestroyNotify type_data_destroy)
{
  xi_sequence_fire_event_full(source_seq, evt_type, evt_subtype,
                              event_name, when,
                              0, 0, 0, 0,
                              type_data,
                              type_data_destroy);
}


/*****************************************************************************
New/Copy/Free functions for:
- XIStory
- XISequence
- XIDrawable
- XISound
- XIPosition
- XIMovement
- XIRect
******************************************************************************/

void
xi_rect_free(XIRect *rect)
{
  if(rect == NULL) return;

  g_free(rect);
}

void
xi_timed_frame_free(XITimedFrame *frame)
{
  if(frame == NULL) return;

  g_free(frame);
}

void
xi_position_free(XIPosition *pos)
{
  if(pos == NULL) return;
  g_free(pos->layer_name);
  g_free(pos->x_relative_str);
  g_free(pos->y_relative_str);
  g_free(pos->z_relative_str);
  g_free(pos);
}


void
xi_movement_free(XIMovement *move)
{
  if(move == NULL) return;
  g_free(move->instance_name);
  g_free(move->name);
  g_free(move->type);
  g_free(move->steps); /* TODO: Does this work for arrays */
  g_free(move);
}

void
xi_drawable_free(XIDrawable *drawable)
{
  if(drawable == NULL) return;
  g_free(drawable->instance_name);
  g_free(drawable->name);
  g_free(drawable->type);
  xi_position_free(drawable->pos);
  xi_rect_free(drawable->rect);
  g_free(drawable->frames_name);
  if(drawable->frames_by_name) {
    g_hash_table_destroy(drawable->frames_by_name);
  }
  g_free(drawable);
}

void
xi_sound_free(XISound *sound)
{
  if(sound == NULL) return;
  g_free(sound->instance_name);
  g_free(sound->name);
  xi_position_free(sound->pos);
  g_free(sound);
}

XIRect*
xi_rect_copy(XIRect *rect)
{
  if(rect == NULL) return NULL;
  XIRect *new_rect = g_new(XIRect, 1);
  new_rect->x = rect->x;
  new_rect->y = rect->y;
  new_rect->h = rect->h;
  new_rect->w = rect->w;
  return new_rect;
}

XIRect*
xi_timed_frame_rect_copy(XITimedFrame *frame)
{
  return xi_rect_copy(&(XIRect)
                      {   .x=frame->x,
                          .y=frame->y,
                          .h=frame->h,
                          .w=frame->w});
}

XITimedFrame*
xi_timed_frame_copy(XITimedFrame *frame)
{
  if(frame == NULL) return NULL;
  XITimedFrame *new_frame = g_new(XITimedFrame, 1);
  new_frame->duration = frame->duration;
  new_frame->x = frame->x;
  new_frame->y = frame->y;
  new_frame->h = frame->h;
  new_frame->w = frame->w;
  return new_frame;
}

XIMovement*
xi_deep_copy_movement(XIMovement *src) {
  XIMovement *new_m = g_new(XIMovement, 1);

  new_m->instance_name   = g_strdup(src->instance_name);
  new_m->name            = g_strdup(src->name);
  new_m->type            = g_strdup(src->type);
  new_m->steps_col_count = src->steps_col_count;
  new_m->steps_row_count = src->steps_row_count;

  if(new_m->steps_row_count > 0 && new_m->steps_col_count > 0) {
    guint col_count = new_m->steps_col_count;
    guint row_count = new_m->steps_row_count;
    new_m->steps = g_malloc_n(row_count, sizeof(gdouble));
    for(guint i = 0; i < row_count; i++) {
      new_m->steps[i] = g_malloc_n(col_count, sizeof(gdouble));
      for(guint j = 0; j < col_count; j++) {
        gdouble val = src->steps[i][j];
        new_m->steps[i][j] = val;
      }
    }
  }
  return new_m;
}

/*!
  \brief Copies everything except the *_relative_to values, in that case it
  just copies the pointer. We don't want a copy of the *_relative_to
  positions because we are following them.
*/
XIPosition*
xi_deep_copy_position(XIPosition *src)
{
  if(src == NULL){
    return NULL;
  }

  XIPosition* new_pos = g_new(XIPosition, 1);

  new_pos->layer_name     = g_strdup(src->layer_name);
  new_pos->x              =          src->x;
  new_pos->y              =          src->y;
  new_pos->z              =          src->z;
  new_pos->screen_x       =          src->screen_x;
  new_pos->screen_y       =          src->screen_y;
  new_pos->screen_z       =          src->screen_z;
  new_pos->x_relative_to  =          src->x_relative_to;
  new_pos->y_relative_to  =          src->y_relative_to;
  new_pos->z_relative_to  =          src->z_relative_to;
  new_pos->x_relative_str = g_strdup(src->x_relative_str);
  new_pos->y_relative_str = g_strdup(src->y_relative_str);
  new_pos->z_relative_str = g_strdup(src->z_relative_str);

  return new_pos;
}

/*!
  \param pos The position to make relative
  \param relative_to If null returns FALSE and gives warning
  \return TRUE on success
*/
gboolean
xi_position_set_relative_to(XIPosition *pos, XIPosition *relative_to) {
  g_return_val_if_fail(pos         != NULL, FALSE);
  g_return_val_if_fail(relative_to != NULL, FALSE);
  pos->x_relative_to = relative_to;
  pos->y_relative_to = relative_to;
  pos->x_relative_to = relative_to;
  return TRUE;
}

XITimedFrame*
xi_drawable_frames_set_copy(XIDrawableFrames *dframes, guint index,
                            XITimedFrame *frame)
{
  g_return_val_if_fail(dframes != NULL,           NULL);
  g_return_val_if_fail(index   <  dframes->count, NULL);
  g_return_val_if_fail(frame   != NULL,           NULL);

  if(dframes->frames == NULL) {
    dframes->frames = g_malloc_n(dframes->count, sizeof(XITimedFrame*));
  }

  dframes->frames[index] = xi_timed_frame_copy(frame);

  return dframes->frames[index];
}

XISound*
xi_deep_copy_sound(XISound *src)
{
  XISound *new_snd = g_new(XISound, 1);
  new_snd->instance_name = g_strdup(src->instance_name);
  new_snd->name          = g_strdup(src->name);
  new_snd->pos           = xi_deep_copy_position(src->pos);
  return new_snd;
}

/*
  Fields not copied:

  - hooks: Set to NULL instead of being copied

  - frames_by_name: Set to NULL instead of being copied
*/
XIDrawable*
xi_deep_copy_drawable(XIDrawable *src)
{
  XIDrawable *new_draw = g_new(XIDrawable, 1);
  new_draw->instance_name     = g_strdup(              src->instance_name);
  new_draw->name              = g_strdup(              src->name);
  new_draw->type              = g_strdup(              src->type);
  new_draw->pos               = xi_deep_copy_position( src->pos);
  new_draw->use_alpha         =                        src->use_alpha;
  new_draw->use_alpha_channel =                        src->use_alpha_channel;
  new_draw->alpha             =                        src->alpha;
  new_draw->prev_alpha        =                        src->prev_alpha;
  new_draw->use_colorkey      =                        src->use_colorkey;
  new_draw->colorkey_red      =                        src->colorkey_red;
  new_draw->colorkey_green    =                        src->colorkey_green;
  new_draw->colorkey_blue     =                        src->colorkey_blue;
  new_draw->rect              =          xi_rect_copy( src->rect);
  new_draw->frames_name       = g_strdup(              src->frames_name);
  new_draw->frames_by_name    = NULL; // TODO: copy this field
  return new_draw;
}

/*! If drawable_to_copy must have an 'instance_name'. If 'name' is NULL
  'name' will be assigned a g_strdup copy of 'instance_name'.

  \param owner_seq can be NULL to create a new drawable with no owner_seq.
  \param drawable_to_copy will be duplicated.
  \return copy of drawable_to_copy
*/
XIDrawable*
xi_drawable_add_copy(XISequence *owner_seq, XIDrawable *drawable_to_copy)
{
  g_return_val_if_fail(drawable_to_copy                != NULL, NULL);
  g_return_val_if_fail(drawable_to_copy->instance_name != NULL, NULL);

  g_debug(_("%s: for drawable '%s'"),
          __FUNCTION__, drawable_to_copy->instance_name);

  XIDrawable *new_drawable = xi_deep_copy_drawable(drawable_to_copy);
  new_drawable->owner_seq = owner_seq;

  if(owner_seq != NULL) {
    xi_position_set_relative_to(new_drawable->pos, owner_seq->pos);
  }

  if(owner_seq != NULL) {
    if(owner_seq->drawables == NULL) {
      owner_seq->drawables =
        g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
                              (GDestroyNotify)xi_drawable_free);
    }
    g_hash_table_insert(owner_seq->drawables,
                        g_strdup(new_drawable->instance_name),
                        new_drawable);
  }

  if(owner_seq == NULL) {
    g_debug(_("%s: Leaving (owner_seq %p, drawable '%s')"), __FUNCTION__,
            owner_seq, new_drawable->instance_name);
  }else{
    g_debug(_("%s: Leaving (owner_seq '%s', drawable '%s')"), __FUNCTION__,
            owner_seq->instance_name, new_drawable->instance_name);
  }
  return new_drawable;
}

void
xi_drawable_frames_free(XIDrawableFrames *dframes)
{
  if(dframes == NULL) return;

  g_free(dframes->drawable_name);
  g_free(dframes->series_name);
  g_free(dframes->frames); /* TODO: Does this work for arrays */

  g_free(dframes);
}

XIDrawableFrames*
xi_drawable_frames_deep_copy(XIDrawableFrames* dframes)
{
  g_return_val_if_fail(dframes != NULL, NULL);

  XIDrawableFrames* new_dframes = g_new(XIDrawableFrames, 1);

  new_dframes->drawable_name    = g_strdup(dframes->drawable_name);
  new_dframes->series_name      = g_strdup(dframes->series_name);
  new_dframes->count            =          dframes->count;
  new_dframes->offset           =          dframes->offset;
  new_dframes->elapsed_in_frame =          dframes->elapsed_in_frame;
  new_dframes->frames           = NULL;

  /* Copy all frames */
  if(dframes->frames != NULL && new_dframes->count > 0) {
    new_dframes->frames = g_malloc_n(new_dframes->count, sizeof(XITimedFrame*));
    for(guint i = 0; i < new_dframes->count; i++) {
      new_dframes->frames[i] = xi_timed_frame_copy(dframes->frames[i]);
    }
  }

  return new_dframes;
}

/*
  \return TRUE if selected
*/
gboolean
xi_drawable_select_frame_series(XIDrawable *drawable, gchar const *series_name)
{
  if(drawable->frames_name != NULL) {
    g_free(drawable->frames_name);
  }
  drawable->frames_name = g_strdup(series_name);
}

/*
  \param drawable To add frames to
  \param frames_name key that will be added to frames_by_name
  \param frames are deep copied
*/
XIDrawableFrames*
xi_drawable_add_frames_copy(XIDrawable *drawable,
                            gchar const *frames_name,
                            XIDrawableFrames *frames)
{
  g_return_val_if_fail(drawable    != NULL, NULL);
  g_return_val_if_fail(frames_name != NULL, NULL);
  g_return_val_if_fail(frames      != NULL, NULL);

  if(drawable->frames_by_name == NULL) {
    drawable->frames_by_name =
      g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
                            (GDestroyNotify)xi_drawable_frames_free);
  }

  XIDrawableFrames *new_dframes = xi_drawable_frames_deep_copy(frames);
  if(new_dframes == NULL) {
    g_warning(_("%s: Frame copying failed when adding to drawable '%s'"),
              __FUNCTION__, drawable->instance_name);
  }

  /* Add to drawable */
  g_hash_table_insert(drawable->frames_by_name,
                      g_strdup(frames_name),
                      new_dframes);

  /* If nothing selected, go ahead and set this to the current */
  if(drawable->frames_name == NULL) {
    xi_drawable_select_frame_series(drawable, frames_name);
  }

  return new_dframes;
}


/*! Make a mostly complete duplicate copy of a sequence and all of its
  sub data structures. There are some fields that cannot be copied and
  probably should not be copied in most circumstances.

  Fields not copied:

  - parent: Is set to NULL. That means the function calling this
  function should set the 'parent' field, if any is needed. For
  childen of this sequence their parent is set to this sequence.

  - hooks: Copying not yet implemented. One difficulty is copying the
  'gpointer data' field of each hook in the hook list. You'll have to
  know the type to cast the data to then call the appropriate copy
  function. This could be done by looking at the 'func' field as an
  indicator of type or the 'flags' field could be used to hold type
  information as well.

  - listeners: Copying not yet implemented. Set to NULL.

  TODO: Detect infinite recursion due to cyclical references.
*/
XISequence*
xi_deep_copy_sequence(XISequence* src)
{
  g_return_val_if_fail(src != NULL, NULL);

  g_debug(_("%s: Entered for '%s'"), __FUNCTION__, src->instance_name);

  XISequence* new_seq = g_new(XISequence, 1);

  new_seq->instance_name    = g_strdup(              src->instance_name);
  new_seq->name             = g_strdup(              src->name);
  new_seq->parent           = NULL;
  new_seq->children         = NULL;
  new_seq->state_name       = g_strdup(              src->state_name);
  new_seq->state_specific   = NULL;
  new_seq->drawables        = NULL;
  new_seq->movements        = NULL;
  new_seq->sounds           = NULL;
  new_seq->hooks            = NULL;
  new_seq->pos              = xi_deep_copy_position( src->pos);
  new_seq->natural_w        =                        src->natural_w;
  new_seq->natural_h        =                        src->natural_h;
  new_seq->scale_mode       = g_strdup(              src->scale_mode);
  new_seq->camera           = xi_camera_copy(        src->camera);
  new_seq->start_at         =                        src->start_at;
  new_seq->start_on         = g_strdup(              src->start_on);
  new_seq->start_on_fired   =                        src->start_on_fired;
  new_seq->start_asap       =                        src->start_asap;
  new_seq->started          =                        src->started;
  new_seq->restartable      =                        src->restartable;
  new_seq->done             =                        src->done;
  new_seq->duration         =                        src->duration;
  new_seq->duration_type    =                        src->duration_type;
  new_seq->rate             =                        src->rate;
  new_seq->elapsed          =                        src->elapsed;
  new_seq->prev_elapsed     =                        src->prev_elapsed;
  new_seq->listeners        = NULL;
  new_seq->event_mask       =                        src->event_mask;

  /* Copy children */
  if(src->children != NULL) {
    new_seq->children = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
                                              (GDestroyNotify)xi_sequence_free);
    if(g_hash_table_size(src->children) > 0) {
      GHashTableIter iter;
      gpointer key, value;
      g_hash_table_iter_init(&iter, src->children);
      while(g_hash_table_iter_next(&iter, &key, &value)) {
        XISequence *copied = xi_deep_copy_sequence(value);
        copied->parent = new_seq;
        g_hash_table_insert(new_seq->children,
                            g_strdup(copied->instance_name),
                            (gpointer)copied);
      }
    }
  }

  /* Copy state_specific */
  if(src->state_specific != NULL) {
    new_seq->state_specific =
      g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
                            (GDestroyNotify)xi_sequence_free);
    if(g_hash_table_size(src->state_specific) > 0) {
      GHashTableIter iter;
      gpointer key, value;
      g_hash_table_iter_init(&iter, src->state_specific);
      while(g_hash_table_iter_next(&iter, &key, &value)) {
        XISequence *copied = xi_deep_copy_sequence(value);
        copied->parent = new_seq;
        g_hash_table_insert(new_seq->state_specific,
                            g_strdup(copied->instance_name),
                            (gpointer)copied);
      }
    }
  }

  /* Copy drawables */
  if(src->drawables != NULL) {
    new_seq->drawables =
      g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
                            (GDestroyNotify)xi_drawable_free);
    if(g_hash_table_size(src->drawables) > 0) {
      GHashTableIter iter;
      gpointer key, value;
      g_hash_table_iter_init(&iter, src->drawables);
      while(g_hash_table_iter_next(&iter, &key, &value)) {
        XIDrawable *copied = xi_deep_copy_drawable(value);
        g_hash_table_insert(new_seq->drawables,
                            g_strdup(copied->instance_name),
                            (gpointer)copied);
      }
    }
  }

  /* Copy movements */
  if(src->movements != NULL) {
    new_seq->movements =
      g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
                            (GDestroyNotify)xi_movement_free);
    if(g_hash_table_size(src->movements) > 0) {
      GHashTableIter iter;
      gpointer key, value;
      g_hash_table_iter_init(&iter, src->movements);
      while(g_hash_table_iter_next(&iter, &key, &value)) {
        XIMovement *copied = xi_deep_copy_movement(value);
        g_hash_table_insert(new_seq->movements,
                            g_strdup(copied->instance_name),
                            (gpointer)copied);
      }
    }
  }

  /* Copy sounds */
  if(src->sounds != NULL) {
    new_seq->sounds = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
                                            (GDestroyNotify)xi_sound_free);
    if(g_hash_table_size(src->sounds) > 0) {
      GHashTableIter iter;
      gpointer key, value;
      g_hash_table_iter_init(&iter, src->sounds);
      while(g_hash_table_iter_next(&iter, &key, &value)) {
        XISound *copied = xi_deep_copy_sound(value);
        g_hash_table_insert(new_seq->sounds,
                            g_strdup(copied->instance_name),
                            (gpointer)copied);
      }
    }
  }

  /* TODO: Copy hooks */
  if(src->hooks != NULL) {
    new_seq->hooks = NULL;
    /* TODO */
  }

  return new_seq;
}


/**
  Elapsed Time:

  Should be the time elapsed since the start of this sequence.

  Zero is allowed. The update should continue even if the elapsed
  time is zero. What this means for each piece is respective to that
  piece, but each piece should take a zero duration into account
  even if it just means to do nothing.

  \return elapsed Any non-negative result represents an elapsed time
*/
gboolean
xi_reached_start_at(gdouble *out_elapsed, XISequence *seq, gdouble parent_elapsed)
{
  g_return_val_if_fail(seq != NULL, FALSE);

  gboolean result = FALSE;
  gdouble elapsed = parent_elapsed - seq->start_at;

  if(seq->start_at > 0 && elapsed >= 0) {
    result = TRUE;
  }

  if(out_elapsed != NULL) *out_elapsed = elapsed; // TODO: does this copy the value?

  return result;
}

/*! Start the sequence that is the event->handler_data.

  \param event that should have an XISequence for the 'handler_data' field.
*/
void
xi_sequence_start_standard_event_handler(gpointer event)
{
  g_debug(_("%s: Entering"), __FUNCTION__);
  g_return_if_fail(event != NULL);

  XIEvent *evt = (XIEvent*)event;
  if(evt->handler_data != NULL) {
    XISequence *hook_seq = (XISequence*)evt->handler_data;
    hook_seq->start_on_fired = TRUE;
    if(hook_seq->parent != NULL) {

      /* 'start_at' will be the current elapsed time of the parent
         minus an optional adjustment 'when' (negative) to factor in
         how long ago the event fired (usually a small fraction of a
         second).
      */
      hook_seq->start_at += evt->when + hook_seq->parent->elapsed;

      g_debug(_("%s: event hook_seq={'%s', start_at=%g, elapsed=%g, parent->elapsed=%g, when=%g}"),
              __FUNCTION__, hook_seq->instance_name,
              hook_seq->start_at, hook_seq->elapsed,
              hook_seq->parent->elapsed, evt->when);
      if(XI_EVENT_CASCADING) {

        /* This update call has the effect of the sequence starting
           immediately after the event is fired and not the next
           iteration. This prevents unwanted spaces and delays.
        */
        xi_sequence_update(hook_seq, hook_seq->parent->elapsed);
      }
    }
    g_debug(_("%s: hook_seq('%s') remaining time: %g"), __FUNCTION__,
            hook_seq->instance_name, xi_sequence_remaining_time(hook_seq));
  }else{
    g_debug(_("%s: event->handler_data was NULL"), __FUNCTION__);
  }
  xi_event_free(evt);
  g_debug(_("%s: Leaving"), __FUNCTION__);
}

void
xi_sequence_setup(XISequence *seq)
{
  g_return_if_fail(seq != NULL);
  if(seq->start_on != NULL) {
    g_debug(_("%s: Seq '%s' listening for '%s'"), __FUNCTION__,
            seq->instance_name, seq->start_on);
    xi_sequence_add_seq_listener_from_str(seq,
                                          seq->start_on,
                                          xi_sequence_start_standard_event_handler,
                                          (gpointer)seq,
                                          NULL);
  }
}

/*! If child_to_copy must have an 'instance_name'. If 'name' is NULL
  'name' will be assigned a g_strdup copy of 'instance_name'.

  \param parent can be NULL to create a new sequence with no parent.
  \param child_to_copy will be duplicated.
  \return copy of child_to_copy
*/
XISequence*
xi_sequence_add_child_copy(XISequence *parent, XISequence *child_to_copy)
{
  g_return_val_if_fail(child_to_copy                != NULL, NULL);
  g_return_val_if_fail(child_to_copy->instance_name != NULL, NULL);

  g_debug(_("%s: for child '%s'"), __FUNCTION__, child_to_copy->instance_name);

  XISequence *new_child = xi_deep_copy_sequence(child_to_copy);
  new_child->parent = parent;

  if(parent != NULL) {
    xi_position_set_relative_to(new_child->pos, parent->pos);
  }

  if(parent != NULL) {
    if(parent->children == NULL) {
      parent->children =
        g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
                              (GDestroyNotify)xi_sequence_free);
    }
    g_hash_table_insert(parent->children,
                        g_strdup(new_child->instance_name),
                        new_child);
  }

  xi_sequence_setup(new_child);

  if(parent == NULL) {
    g_debug(_("%s: Leaving (parent %p, child '%s')"), __FUNCTION__,
            parent, new_child->instance_name);
  }else{
    g_debug(_("%s: Leaving (parent '%s', child '%s')"), __FUNCTION__,
            parent->instance_name, new_child->instance_name);
  }
  return new_child;
}


/*! \brief Make a partial copy of a story. Strings fields are g_strdup'd.

  Fields not copied:

  - root_seq: Set to a new root_seq
  - named_buckets: Set to a new hash table
*/
XIStory*
xi_story_new_from_template(XIStory* src)
{
  g_return_val_if_fail(src != NULL, NULL);

  XIStory *new_story = g_new(XIStory, 1);
  new_story->name          = g_strdup(              src->name);
  new_story->title         = g_strdup(              src->title);
  new_story->revision      = g_strdup(              src->revision);
  new_story->natural_w     =                        src->natural_w;
  new_story->natural_h     =                        src->natural_h;
  new_story->natural_bpp   =                        src->natural_bpp;
  new_story->curr_w        =                        src->curr_w;
  new_story->curr_h        =                        src->curr_h;
  new_story->curr_bpp      =                        src->curr_bpp;
  new_story->scale_mode    = g_strdup(              src->scale_mode);
  new_story->root_seq      = xi_sequence_add_child(NULL, "root_seq");
  new_story->elapsed       =                        src->elapsed;
  new_story->named_buckets =
    g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
                          (GDestroyNotify)g_hash_table_destroy);

  return new_story;
}

/*****************************************************************************
Effects:
- Fade To Black (TODO: Why does fade-to-black take so much memory?)
******************************************************************************/

/*!
  \param fade_seq the seq created for the fade. Its instance_name is read.
  \return should be freed with g_free()
*/
gchar*
xi_fade_to_black_image_name(XISequence *fade_seq)
{
  g_return_val_if_fail(fade_seq != NULL, NULL);
  return g_strconcat(fade_seq->instance_name, "-img", NULL);
}

/*!
    List Should likely be called indirectly when the GHook in the
    owner_seq is destroyed which happens when the owner_seq is
    destroyed which happens when the owner_seq is removed from the
    target_seq's children. So, the best way to call this is with
    g_hash_table_remove(target_seq->children, instance_name) which
    also takes care of removing the entry from the
    target_seq->children.

    \brief Likely to be set on a GHook->destroy field.
 */
void
xi_fade_to_black_free(XIData_fade_to_black *data)
{
  g_return_if_fail(data != NULL);
  g_free(data->instance_name);
  g_free(data->start_on);
  g_free(data);
}

void
xi_f_fade_to_black(XIData_fade_to_black *data)
{
  g_return_if_fail(data                       != NULL);
  g_return_if_fail(data->target_seq           != NULL);
  g_return_if_fail(data->owner_seq            != NULL);
  g_return_if_fail(data->owner_seq->drawables != NULL);

  /* An image by this name should exist. It should have been added
     when the fade effect was added to the sequence. */
  gchar *drawable_inst_name = xi_fade_to_black_image_name(data->owner_seq);
  XIDrawable *img = g_hash_table_lookup(data->owner_seq->drawables,
                                        drawable_inst_name);
  g_free(drawable_inst_name);
  g_return_if_fail(img != NULL);

  gboolean adjust_alpha = TRUE;
  if(data->fade_from_black && img->alpha <= 0) {
    adjust_alpha = FALSE;
  }else if(!data->fade_from_black && img->alpha >= 255) {
    adjust_alpha = FALSE;
  }

  if(adjust_alpha) {
    img->prev_alpha = img->alpha;
    gdouble no_overflow =
      (data->owner_seq->elapsed *
       data->owner_seq->rate *
       data->fps);
    g_debug("%s: {owner_seq->elapsed=%g,owner_seq->rate=%g,data->fps=%g,owner_seq->duration=%g,owner_seq->duration_type=%d} multiplied=%g",
            __FUNCTION__, data->owner_seq->elapsed, data->owner_seq->rate,
            data->fps, data->owner_seq->duration,
            data->owner_seq->duration_type, no_overflow);
    if(no_overflow >= 255) {
      if(data->fade_from_black) {
        img->alpha = 0;
      }else{
        img->alpha = 255;
      }
      /* Cannot adjust alpha any further */
    }else{
      if(data->fade_from_black) {
        img->alpha = 255 - (guint8)no_overflow;
      }else{
        img->alpha = (guint8)no_overflow;
      }
    }
  }
}

/*!
  Fields not copied:

  - owner_seq: Set to NULL. Should be explicity set if need be on
  returned value.

  - target_seq: Set to NULL. Should be explicity set if need be on
  returned value.
*/
XIData_fade_to_black*
xi_deep_copy_fade_to_black_data(XIData_fade_to_black* data)
{
  XIData_fade_to_black *data_copy = g_new(XIData_fade_to_black, 1);
  data_copy->instance_name   = g_strdup(data->instance_name);
  data_copy->owner_seq       = NULL;
  data_copy->target_seq      = NULL;
  data_copy->alpha           = data->alpha;
  data_copy->start_at        = data->start_at;
  data_copy->start_on        = g_strdup(data->start_on);
  data_copy->start_on_fired  = data->start_on_fired;
  data_copy->start_asap      = data->start_asap;
  data_copy->restartable     = data->restartable;
  data_copy->duration        = data->duration;
  data_copy->rate            = data->rate;
  data_copy->fps             = data->fps;
  data_copy->fade_from_black = data->fade_from_black;
  return data_copy;
}

void
xi_add_fade_to_black_copy(XISequence *seq, XIData_fade_to_black *data)
{
  g_debug(_("%s: Entered"), __FUNCTION__);
  g_return_if_fail(seq                != NULL);
  g_return_if_fail(seq->instance_name != NULL);
  g_return_if_fail(data               != NULL);

  XIData_fade_to_black *data_copy = xi_deep_copy_fade_to_black_data(data);

  gchar *name = NULL;
  if(data_copy->fade_from_black) {
    name = "fade_from_black";
  }else{
    name = "fade_to_black";
  }

  XISequence *fade_seq =
    xi_sequence_add_child(seq, data_copy->instance_name,
                          .name          = name,
                          .start_at      = data_copy->start_at,
                          .start_on      = data_copy->start_on,
                          .start_on_fired = data_copy->start_on_fired,
                          .start_asap    = data_copy->start_asap,
                          .restartable   = data_copy->restartable,
                          .duration      = data_copy->duration,
                          .duration_type = XI_DURATION_TRUNCATE,
                          .rate          = data_copy->rate);
  g_debug(_("%s: created/added fade_seq:"), __FUNCTION__);

  data_copy->owner_seq  = fade_seq;
  data_copy->target_seq = seq;

  gchar *drawable_inst_name = xi_fade_to_black_image_name(fade_seq);
  g_return_if_fail(drawable_inst_name != NULL);
  if(fade_seq->drawables == NULL
     || g_hash_table_lookup(fade_seq->drawables, drawable_inst_name)) {
    /*
      TODO: This image needs to be stretched to cover the screen in
      different screen resolution.

      TODO: Use a primitive instead of a BMP; allow for other colors
      There exists a: SDL_FillRect(SDL_Surface, SDL_Rect, Uint32 color)

      TODO: Generalize fade_to_black to work with solid colors or
      images and call it something like, alpha_fade or just fade_in,
      fade_out. Maybe a continuous strobe effect would be nice.
    */
    XIDrawable *img = NULL;
    img = xi_drawable_add(fade_seq,
                          .instance_name     = drawable_inst_name,
                          .name              = "black_800x600.bmp",
                          .use_alpha         = TRUE,
                          .use_alpha_channel = FALSE,
                          .alpha             = data_copy->alpha,
                          .use_colorkey      = FALSE);
    img->pos->z = 1000000000000; // Really high, but leave some room
  }
  g_free(drawable_inst_name);

  xi_sequence_setup_hooks_if_null(fade_seq);

  GHook *fade_hook      = g_hook_alloc(fade_seq->hooks);
  fade_hook->data       = (gpointer)data_copy;
  fade_hook->func       = xi_f_fade_to_black;
  fade_hook->destroy    = (GDestroyNotify)xi_fade_to_black_free;
  g_hook_append(fade_seq->hooks, fade_hook);

  g_debug("%s: seq '%s' now has a fade_(to or from)_black effect.",
          __FUNCTION__, seq->instance_name);
  g_debug(_("%s: Leaving"), __FUNCTION__);
}

/*****************************************************************************
Other
******************************************************************************/
/*!
  \param pos target of modifcation
  \param camera an optional camera factor
  \return TRUE if either screen_x, screen_y or screen_z were modified.
*/
gboolean
xi_position_update(XIPosition *pos, XICamera *camera)
{
  g_return_val_if_fail(pos != NULL, FALSE);

  gdouble prev_x = pos->screen_x;
  if(pos->x_relative_to != NULL) {
    pos->screen_x = pos->x_relative_to->screen_x + pos->x;
  }else{
    pos->screen_x = pos->x;
  }

  gdouble prev_y = pos->screen_y;
  if(pos->y_relative_to != NULL) {
    pos->screen_y = pos->y_relative_to->screen_y + pos->y;
  }else{
    pos->screen_y = pos->y;
  }

  gdouble prev_z = pos->screen_z;
  if(pos->z_relative_to != NULL) {
    pos->screen_z = pos->z_relative_to->screen_z + pos->z;
  }else{
    pos->screen_z = pos->z;
  }

  gboolean changed = prev_x != pos->screen_x || prev_y != pos->screen_y || prev_z != pos->screen_z;

  gboolean cam = xi_position_adjust_for_camera(pos, camera);

  return changed || cam;
}

void
xi_dump_ghashtable(GHashTable *table)
{
  if(table == NULL) {
    g_message(_("Printing GHashTable: {address=%p}"), table);
    return;
  }
  g_message(_("Printing GHashTable: {address=%p, count=%d}"),
            table, g_hash_table_size(table));
  GHashTableIter iter;
  gpointer key, value;
  g_hash_table_iter_init(&iter, table);
  while(g_hash_table_iter_next(&iter, &key, &value)) {
    g_message(" [%p] {address=%p}", key, value);
  }
}

void
xi_dump_sequence(XISequence *seq)
{
  g_return_if_fail(seq != NULL);
  g_message(_("Dumping sequence: {address=%p}"), seq);
  g_message("XISequence(");
  g_message(" gchar *instance_name         = \"%s\"", seq->instance_name);
  g_message(" gchar *name                  = \"%s\"", seq->name);
  g_message(" XISequence *parent           = {address=%p}", seq->parent);
  g_message(" gdouble start_at             = %g", seq->start_at);
  g_message(" gchar *start_on              = %s", seq->start_on);
  g_message(" gboolean start_on_fired      = %d", seq->start_on_fired);
  g_message(" gboolean start_asap          = %d", seq->start_asap);
  g_message(" gboolean started             = %d", seq->started);
  g_message(" gboolean restartable         = %d", seq->restartable);
  g_message(" gboolean done                = %d", seq->done);
  g_message(" gdouble duration             = %g", seq->duration);
  g_message(" XIDurationType duration_type = %d", seq->duration_type);
  g_message(" gdouble rate                 = %g", seq->rate);
  g_message(" gdouble elapsed              = %g", seq->elapsed);
  g_message(" gdouble prev_elapsed         = %g", seq->prev_elapsed);
  g_message(" GHashTable *children:");
  xi_dump_ghashtable(seq->children);
  g_message(" gchar *state_name    = \"%s\"", seq->state_name);
  g_message(" GHashTable *state_specific:");
  xi_dump_ghashtable(seq->state_specific);
  g_message(" GHashTable *drawables:");
  xi_dump_ghashtable(seq->drawables);
  g_message(" GHashTable *movements:");
  xi_dump_ghashtable(seq->movements);
  g_message(" GHashTable *sounds:");
  xi_dump_ghashtable(seq->sounds);
  if(seq->hooks != NULL) {
    g_message(" GHookList *hooks: {seq_id=%lu, hook_size=%d, is_setup=%d}",
              seq->hooks->seq_id, seq->hooks->hook_size, seq->hooks->is_setup);
  }else{
    g_message(" GHookList *hooks: {address=%p}", seq->hooks);
  }
  if(seq->pos == NULL) {
    g_message(" XIPosition *pos: {address=NULL}");
  }else{
    g_message(" XIPosition *pos: {address=%p, x=%g, y=%g, z=%g}",
              seq->pos, seq->pos->x, seq->pos->y, seq->pos->z);
  }
  g_message(" XICamera *camera = {%g, %g, %g}",
            seq->camera->point->x, seq->camera->point->y,
            seq->camera->point->z);
  g_message(" guint natural_w = %d", seq->natural_w);
  g_message(" guint natural_h = %d", seq->natural_h);
  g_message(" GHashTable *listeners:");
  xi_dump_ghashtable(seq->listeners);
  g_message(" gint event_mask = %d", seq->event_mask);
  g_message(")");
}

gboolean
xi_init_story(XIStory *story, guint curr_w, guint curr_h, guint curr_bpp)
{
  story->curr_w   = curr_w;
  story->curr_h   = curr_h;
  story->curr_bpp = curr_bpp;

  return TRUE;
}

void xi_start_story_asap(XIStory *story) {
  g_return_if_fail(story != NULL);
  g_return_if_fail(story->root_seq != NULL);
  story->root_seq->start_asap = TRUE;
}

void
xi_drawable_update(XIDrawable *drawable, gdouble elapsed_since_prev)
{
  g_return_if_fail(drawable != NULL);

  if(drawable->frames_name == NULL || drawable->frames_by_name == NULL) {
    return;
  }

  XIDrawableFrames *frames = g_hash_table_lookup(drawable->frames_by_name,
                                                 drawable->frames_name);

  if(frames != NULL) {
    xi_rect_free(drawable->rect);
    drawable->rect = xi_drawable_frames_next(frames, elapsed_since_prev);
  }
}

/*!
  \brief call xi_position_update() on drawable in seq
  \param seq Update drawable positions of this seq
  \return TRUE if any drawable moved.
*/
gboolean
xi_sequence_drawables_position_update(XISequence *seq)
{
  g_return_val_if_fail(seq != NULL, FALSE);

  gboolean result = FALSE;

  if(seq->drawables != NULL) {
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, seq->drawables);
    while(g_hash_table_iter_next(&iter, &key, &value)) {
      XIDrawable *d = value;
      if(d != NULL) {
        if(xi_position_update(d->pos, NULL)) {
          result = TRUE;
	}
      }else{
        g_warning(_("%s: NULL drawable found in seq '%s'"),
                  __FUNCTION__, seq->instance_name);
      }
    }
  }

  return result;
}

/*!
  \return TRUE if position screen_x,y,z changed.
 */
gboolean
xi_position_adjust_for_camera(XIPosition *pos, XICamera *cam) {
  g_return_val_if_fail(pos != NULL, FALSE);
  if(cam        == NULL) return FALSE;
  if(cam->point == NULL) return FALSE;

  pos->screen_x -= cam->point->x;
  pos->screen_y -= cam->point->y;
  pos->screen_z -= cam->point->z;

  return(cam->point->x != 0 || cam->point->y != 0 || cam->point->z != 0);
}

/*
  \param seq Sequence to examine
  \return value of seq->start_asap else FALSE
*/
gboolean
xi_sequence_is_marked_start_asap(XISequence *seq)
{
  g_return_val_if_fail(seq != NULL, FALSE);
  return seq->start_asap;
}

/*
  \param seq Sequence to examine
  \return value of seq->started else FALSE
*/
gboolean
xi_sequence_is_marked_started(XISequence *seq)
{
  g_return_val_if_fail(seq != NULL, FALSE);
  return seq->started;
}

/*
  \param seq Sequence to examine
  \return value of seq->restartable, else FALSE
*/
gboolean
xi_sequence_is_marked_restartable(XISequence *seq)
{
  g_return_val_if_fail(seq != NULL, FALSE);
  return seq->restartable;
}

/*
  \param seq Sequence to examine
  \return value of seq->done else FALSE
*/
gboolean
xi_sequence_is_marked_done(XISequence *seq)
{
  g_return_val_if_fail(seq != NULL, FALSE);
  return seq->done;
}

/*
  \brief Sequence elapsed bookkeeping
*/
void
xi_sequence_update_elapsed(XISequence *seq, gdouble elapsed)
{
  g_return_if_fail(seq != NULL);

  seq->prev_elapsed = seq->elapsed;
  seq->elapsed = elapsed;
}

/*
  \brief To be called at least once every game-loop iteration
  \param seq The sequences to recursively update
  \param parent_elapsed The elapsed of the parent sequence
*/
void
xi_sequence_update(XISequence *seq, gdouble parent_elapsed)
{
  g_return_if_fail(seq != NULL);

  g_debug(_("%s: Entered for seq '%s'. parent_elapsed=%g, seq={done=%d, elapsed=%g, start_at=%g, start_on=%s, start_on_fired=%d, start_asap=%d, started=%d}"),
          __FUNCTION__, seq->instance_name, parent_elapsed,
          seq->done, seq->elapsed, seq->start_at, seq->start_on, seq->start_on_fired, seq->start_asap, seq->started);

  gdouble elapsed = 0;
  if(seq->started || seq->start_asap) {
    elapsed = parent_elapsed - seq->start_at;
  }else if((seq->start_on != NULL && seq->start_on_fired)
           || (seq->start_on == NULL && seq->start_at > 0)) {

    /* Was start_at reached? */

    if(xi_reached_start_at(&elapsed, seq, parent_elapsed)) {
      seq->start_asap = TRUE;
      g_debug(_("%s: Elapsed being reached started seq '%s'. {elapsed=%f, start_asap=%d}"), __FUNCTION__, seq->instance_name, elapsed, seq->start_asap);
    }
  }  

  /* Are we waiting for a start event? */
  if(!seq->started && !seq->start_asap) {
    return;
  }

  /* Warning about incorrect state */
  if (seq->started && seq->done) {
    g_warning(_("%s: sequence marked started (%d) and done (%d) simultaneously."),
              __FUNCTION__, seq->started, seq->done);
  }

  /* Do not perform this update if it was already performed. This is
     can be an important check for efficiency since sequence update
     attempts often happen multiple times per iteration due to
     cascading effects of events that get fired. If seq is already in
     the correct state, no need to update. The exception is zero. */
  if(seq->started && seq->elapsed == elapsed && elapsed != 0) {
    g_debug(_("%s: Skipping update for elapsed (%g). Was already performed."),
            __FUNCTION__, elapsed);
    return;
  }

  /*****
        Ok, we're ready to update this sequence
  ******/
  g_debug(_("%s: updating sequence: '%s'"), __FUNCTION__, seq->instance_name);

  /*

    Time and Events:

    Visually it makes sense to ignore inaccurate start and end times
    because this is how the real world functions. For example, if
    something flies by your face fast enough, your eyes might not
    catch it right as it enters your vision or right as it exists. If
    it's fast enough you won't see it at all. The probems with this
    arises with logic that is dependent on knowing about certain
    events. We need to have a complete explanation about how events
    are handled in every scenario.

    Here are a few scenarios that explain the problem:

    - The most extreme scenario: If the elapsed time since the last
    update exceeds the total duration length of a sequence then a
    sequence may never happen. This can happen if the framerate is
    low enough and the sequence duration is short enough. I will
    attempt to depict this in this ascii graphic timeline:

    <---------update^----[---duration---]-----update^-------->

    A few observations:

    1. The sequence duration is fully skipped and is done though it
    never started.

    2. A done event would likely get fired, but what about a start
    event? And what about all the events in-between the start and
    end? Should we skip them or fire them? How do we ensure events
    get fired if they are important and should not be skipped?
    Should there be event types that imply different handling?
    There could be a 'mandatory' event that gets fired no matter
    what. An 'ignore-if-skipped event'. A 'superseded-by' feature of
    an event for cases where two events get fired but one supersedes
    the other (perhaps they're the same event fired at different
    times and the later event should always win).

    - The common scenario: In almost every case the end of a duration
    falls between two update calls. This is also true for the start
    of a duration.

    <----------^--^-[^---^--^-^-^---^---^-]-^---^-^--^------->
    <----------------|----------------------|---------------->
    <--- Start event ^---------- Done event-^---------------->

    At the start of a duration the sequence will be updated
    according to the elapsed time into the duration, which is
    intuitive. It perhaps fires a 'started' event.

    When an update attempt happens beyond the end of the duration
    the seq considers itself expired and perhaps fires a 'done'
    event.

    A few observations:

    1. The 'done' event gets fired after sequence is done, but not
    precisely at the end of the duration. The start and end events
    will be delayed. The event object should perhaps include an
    elapsed time firing to better inform the event handler. (Or the
    sequence object could be examined to determine the same thing
    provided it includes the data necessary to make such a
    determination.)

    2. The small tail-end of the duration falls between updates and
    is therefore skipped.

  */

  /* Move ahead the elapsed time record on the sequence */
  xi_sequence_update_elapsed(seq, elapsed);

  /* Did sequence just start? */
  if(!seq->started) {
    seq->started = TRUE;
    seq->start_asap = FALSE;
    g_debug("v-v-v-v-v-- %s STARTED (elapsed %g, prev_elapsed=%g) --v-v-v-v",
            seq->instance_name, elapsed, seq->prev_elapsed);
    xi_sequence_fire_seq_event(seq, "started", -elapsed);
  }

  /* Did sequence just end? */
  if(xi_sequence_just_ended(seq)) {
    seq->done = TRUE;
    seq->started = FALSE;
    g_debug("^-^-^-^-^--  %s DONE (elapsed %g, prev_elapsed=%g)   --^-^-^-^",
            seq->instance_name, elapsed, seq->prev_elapsed);
    xi_sequence_fire_seq_event(seq, "done", seq->duration - elapsed);
    return;
  }

  /* Update drawable frames */
  if(seq->drawables != NULL && g_hash_table_size(seq->drawables) > 0) {
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, seq->drawables);
    while(g_hash_table_iter_next(&iter, &key, &value)) {
      xi_drawable_update((XIDrawable*)value, elapsed - seq->prev_elapsed);
    }
  }

  /* Invoke the sequence's behavior */
  if(seq->hooks != NULL) {
    g_hook_list_invoke(seq->hooks, FALSE);
    /* TODO: Is this call async? If so, what are the consequences? */
  }

  /* Post-behavioral seq position bookkeeping */
  gboolean seq_moved = xi_position_update(seq->pos, seq->camera);
  gboolean drawables_moved = xi_sequence_drawables_position_update(seq);
  g_debug(_("%s: updating position for seq '%s'. Seq moved? = %d. Drawables moved? = %d"),
          __FUNCTION__, seq->instance_name, seq_moved, drawables_moved);

  /* Recur on state_specific */
  XISequence *state_seq = NULL;
  if(seq->state_name != NULL
     && seq->state_specific != NULL
     && (state_seq = g_hash_table_lookup(seq->state_specific,
                                         seq->state_name)) != NULL) {
    xi_sequence_update(state_seq, elapsed);
  }

  /* Recur on children */
  if(seq->children != NULL && g_hash_table_size(seq->children) > 0) {
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, seq->children);
    while(g_hash_table_iter_next(&iter, &key, &value)) {
      xi_sequence_update((XISequence*)value, elapsed);
    }
  }
}

void
xi_update(XIStory *story, gdouble elapsed)
{
  g_return_if_fail(story != NULL);
  if(story->root_seq == NULL) {
    g_critical(_("No root_seq. Story might not be initialized."));
  }
  g_return_if_fail(story->root_seq != NULL);
  g_return_if_fail(elapsed >= 0); /* allow zero */

  xi_sequence_update(story->root_seq, elapsed);
}

/*!
  \brief Safely get a value from a GHashTable
  \param hash_table where to find the value
  \param lookup_key to perform the lookup
  \param value return location for the value associated with the key. NULL not allowed.
  \param default_value the result if lookup_key does not match
  \param err used like Glib recommends, can be NULL.
  \return TRUE if key found in GHashTable
  \sa g_hash_table_lookup_extended
*/
gboolean
xi_get_value(GHashTable *hash_table, gconstpointer lookup_key,
             gpointer   *value,      gpointer      *default_value,
             GError **err)
{
  if(hash_table == NULL) {
    g_set_error(err, xi_error_quark(), XI_NULL_PARAM,
                _("Hash table parameter is NULL."));
    return FALSE;
  }else if(lookup_key == NULL) {
    g_set_error(err, xi_error_quark(), XI_NULL_PARAM,
                _("Lookup key is NULL, which is prohibited."));
    return FALSE;
  }else if(value == NULL) {
    g_set_error(err, xi_error_quark(), XI_NULL_PARAM,
                _("Value parameter is NULL."));
    return FALSE;
  }

  gboolean found_p = g_hash_table_lookup_extended(hash_table, lookup_key,
                                                  NULL, value);
  if(found_p) {
    g_debug(_("%s: found value for key: '%s'"),
            __FUNCTION__, (char *)lookup_key);
  }else if(value != NULL && default_value != NULL){
    *value = *default_value;
    g_debug(_("%s: '%s' not found. Using default value provided."),
            __FUNCTION__, (char *)lookup_key);
  }
  return found_p;
}

/*!
  \brief Safely get a gint from a GHashTable. All gint values should be stored with GPOINTER_TO_INT.
  \param hash_table where to find the value
  \param lookup_key to perform the lookup
  \param value the result found. NULL not allowed.
  \param default_value the result if lookup_key does not match
  \param err used like Glib recommends, can be NULL.
  \return TRUE if key found in GHashTable
  \sa g_hash_table_lookup_extended
*/
gboolean
xi_get_int(GHashTable *hash_table, gconstpointer lookup_key,
           gint       *value,      gint          default_value,
           GError    **err)
{
  if(hash_table == NULL) {
    g_set_error(err, xi_error_quark(), XI_NULL_PARAM,
                _("Hash table parameter is NULL."));
    return FALSE;
  }else if(lookup_key == NULL) {
    g_set_error(err, xi_error_quark(), XI_NULL_PARAM,
                _("Lookup key is NULL, which is prohibited."));
    return FALSE;
  }else if(value == NULL) {
    g_set_error(err, xi_error_quark(), XI_NULL_PARAM,
                _("Value parameter is NULL."));
    return FALSE;
  }

  GError *tmp_err = NULL;
  gpointer value_tmp = NULL;
  gboolean found_p = xi_get_value(hash_table, lookup_key, &value_tmp,
                                  NULL,&tmp_err);
  if(found_p) {
    g_debug(_("%s: Calling GPOINTER_TO_INT(%p)"), __FUNCTION__,
            value_tmp);
    *value = GPOINTER_TO_INT(value_tmp);
  }else{
    *value = default_value;
    g_debug(_("%s: '%p' not found. Defaulted to: %d"), __FUNCTION__,
            lookup_key, *value);
  }
  if(tmp_err != NULL) {
    g_propagate_error(err, tmp_err);
  }
  return found_p;
}
