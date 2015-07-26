#ifndef __XI_DATA_H_
#define __XI_DATA_H_

/*
pragma GCC diagnostic ignored "-Woverride-init"
pragma GCC diagnostic ignored "-Wmissing-field-initializers"
*/


#include <stdlib.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <libintl.h>
/* Gettext '_(String)' macro pretty much recommended:
   http://www.gnu.org/software/gettext/manual/html_node/Mark-Keywords.html */
#define _(String) gettext (String)

#include "xi_enums.h"
#include "xi_error.h"
#include "xi_points.h"

/*****************************************************************************
Primary Structs
******************************************************************************/


/* TODO: LEFT_OFF making camera controllable.

   Camera rules:

   - Toggle never zoom-out/pan beyond root_seq

   - Parallax for panning and zooming.

   - Zooming beyond a Drawable makes it not drawn. Zooming beyond Z
   makes it disappear behind you. However the sound, events and other
   things should remain.

   - No Axis turning.

   - Certain images should be except. Maybe the best way to accomplish
   this is to put them in a different 'layer_name'.

   - Enforce max acceleration to get smoothing camera movement event
   when it is controlled by a keyboard.

*/
typedef union XICamera {
  XIDriveablePoint *point;
} XICamera;

typedef struct XIInput_XY {
  gdouble  up;    /*! Input pressure. Percentage: 0.0 - 1.0 */
  gboolean up_changed;
  gdouble  down;  /*! Input pressure. Percentage: 0.0 - 1.0 */
  gboolean down_changed;
  gdouble  left;  /*! Input pressure. Percentage: 0.0 - 1.0 */
  gboolean left_changed;
  gdouble  right;  /*! Input pressure. Percentage: 0.0 - 1.0 */
  gboolean right_changed;
} XIInput_XY;

typedef struct XIInput_Z {
  gdouble z; /*! Percentage: 0.0 - 100.0 */
} XIInput_Z;

typedef struct XIRect {
  gdouble x;
  gdouble y;
  gdouble h;
  gdouble w;
} XIRect;

// LEFT_OFF: Using something like this for movements
typedef struct XIPoint {
  gdouble x;
  gdouble y;
  gdouble z;
} XIPoint;

typedef struct XIMovement {
  gchar *instance_name;
  gchar *name;
  gchar *type; /*!< TODO: Ideas: "absolute_path",
                 "relative_to_position", etc */
  guint steps_col_count;
  guint steps_row_count;
  gdouble **steps;
  guint curr_step;
  guint prev_step;
  guint speed;
} XIMovement;

/*! The x, y, z values should be treated as being relative to the
    *_relative_to values. The screen_* are values on the screen after
    all calculations are done.
*/
typedef struct XIPosition {
  gchar *layer_name; /*!< Inteded to for grouping (i.e. interactions) */
  gdouble            x; /*!< Relative position */
  gdouble            y; /*!< Relative position */
  gdouble            z; /*!< Relative position */
  gdouble     screen_x; /*!< Not relative */
  gdouble     screen_y; /*!< Not relative */
  gdouble     screen_z; /*!< Not relative */
  struct XIPosition *x_relative_to; /*!< Pointer to another position */
  struct XIPosition *y_relative_to; /*!< Pointer to another position */
  struct XIPosition *z_relative_to; /*!< Pointer to another position */
  gchar             *x_relative_str; /*!< "center|top_left|bot_right...|x" */
  gchar             *y_relative_str; /*!< "center|top_left|bot_right...|y" */
  gchar             *z_relative_str; /*!< "center|top_left|bot_right...|z" */
} XIPosition;

/*!
 */
typedef struct XISound {
  gchar *instance_name; /*!< unique instance name */
  gchar *name; /*!< file name */
  XIPosition *pos; /*!< Where the sound is coming from */
} XISound;

/*! A Sequence possible the most important piece of data. It
    represents anything that can be animated, drawn, heard, acted
    upon, etc. Sequences can be grouped together by making them
    children of a parent sequence. Sequences are event driven; they
    fire and respond to events. Sequences can interact with other
    sequences, or anything else for that matter, through events.
*/
typedef struct XISequence {
  gchar *instance_name;
  gchar *name;
  struct XISequence *parent; /*!< Pointer to parent sequence */
  GHashTable *children; /*!< Sequences can contain sequences */
  gchar *state_name;
  GHashTable *state_specific; /*!< State specific sequences */
  GHashTable *drawables; /*!< Items to be drawn across all states */
  GHashTable *movements; /*!< Movements applied across all states */
  GHashTable *sounds; /*!< Sounds applied across all states */
  GHookList *hooks; /*!< Used to update sequence */
  XIPosition *pos; /*!< Position of this seq */
  guint natural_w;
  guint natural_h;
  gchar *scale_mode; /*!< "crop", "stretch", "letterbox", NULL */
  XICamera *camera; /*!< A POV within the sequence */
  gdouble start_at; /*!< Delay after 'start_on' or parent start */
  gchar *start_on; /*!< String that specifies "seq-name:event-name" */
  gboolean start_on_fired; /*!< TRUE when start_on was just fired */
  gboolean start_asap; /*!< Start as soon as possible */
  gboolean started;
  gboolean restartable;
  // TODO: Idea: gdboule was_started_at;
  // TODO: Idea: gdouble was_done_at;
  gboolean done;
  gdouble duration;
  XIDurationType duration_type;
  gdouble rate; /*!< 1.0 means no adjustment and is default */
  gdouble elapsed;
  gdouble prev_elapsed;
  GHashTable *listeners; /*!< key=event-name, value=hooks-to-call */
  gint event_mask; /*!< types of events listened for in listeners */
} XISequence;

/*! A Drawable could be an image, but does not contain an actual
  image, this is because images specific to the target
  platform. Rather they name the image resource/asset and describe
  it's position, alpha and visual adjustments in a platform agnostic
  way.
*/
typedef struct XIDrawable {
  XISequence *owner_seq;
  gchar *instance_name; /*!< Unique instance name */
  gchar *name; /*!< Unique asset name */
  gchar *type; /*!< Something like: "image" or "shape" */
  XIPosition *pos;
  gboolean use_alpha; /*!< Full image transparency */
  gboolean use_alpha_channel; /*!<  Full image and alpha channel transparency */
  guint8 alpha; /*!< Can be 0-255 */
  guint8 prev_alpha; /*!< Used to detect when alpha changes */
  gboolean use_colorkey; /*!< Another means of transparency */
  guint8 colorkey_red; /*!< Can be 0-255 */
  guint8 colorkey_blue; /*!< Can be 0-255 */
  guint8 colorkey_green; /*!< Can be 0-255 */
  XIRect *rect; /*!< The part of the image to draw. NULL means draw all. */
  gchar *frames_name; /*!< Lookup key for frames_by_name */
  GHashTable *frames_by_name; /*!< Table of XIDrawableFrames */
} XIDrawable;

typedef struct XITimedFrame {
  /* TODO: Would be nice to use a stdc c11 union or gcc '-fms-extensions'
     to embed/extend an XIRect */
  gdouble duration;
  gdouble x;
  gdouble y;
  gdouble h;
  gdouble w;
} XITimedFrame;

typedef struct XIDrawableFrames {
  gchar *drawable_name;
  gchar *series_name;
  gsize count;
  gsize offset;
  gdouble elapsed_in_frame; /*!< time into current frame at offset */
  XITimedFrame **frames; /*!< Array of g_malloc'd XITimedFrame */
} XIDrawableFrames;

typedef struct XIEvent {
  XIEventType type; /*! XI_SEQ_EVENT, XI_INPUT_EVENT, etc. */
  gint subtype; /*!< XI_INPUT_XY, etc. */
  XISequence *source_seq; /*!< Sequence that fired the event. Origin seq. */
  gchar *name; /*!< Name of event. Like "done". */
  gdouble when; /*!< Adjustment for when event should have fired. Negative. */
  gdouble x; /*!< X position of event */
  gdouble y; /*!< Y position of event */
  gdouble z; /*!< Z position of event */
  gdouble speed; /*!< How fast the source was moving */
  gpointer type_data; /*!< Changes per type and subtype */
  GDestroyNotify type_data_destroy; /*!< How to destroy type_data */
  gpointer handler_data; /*!< Pass-through data for event handler */
  GDestroyNotify handler_data_destroy; /*!< How to destroy handler_data */
} XIEvent;

/*! A Story contains one or many sequences that link between
    themselves to create a potentially non-linearly story. */
typedef struct XIStory {
  gchar *name;
  gchar *title;
  gchar *revision;
  guint natural_w;
  guint natural_h;
  guint natural_bpp;
  guint curr_w;
  guint curr_h;
  guint curr_bpp;
  gchar *scale_mode;
  XISequence *root_seq;
  gdouble elapsed;
  GHashTable *named_buckets; /*!< Table of tables. key=name,
                               value=hash-table. Use this kinda like
                               a cache. Other sub-systems use this
                               to store things against anything with
                               a name. */
} XIStory;


/*****************************************************************************
Functions
******************************************************************************/

/* New/Copy/Free */

/* Story Building Functions */
XISequence* xi_sequence_add_child_copy(XISequence *parent, XISequence *child_to_copy);
XIStory* xi_story_new_from_template(XIStory* src);
XISequence* xi_deep_copy_sequence(XISequence* src);
XIPosition* xi_deep_copy_position(XIPosition* src);
void xi_dump_ghashtable(GHashTable *table);
void xi_dump_sequence(XISequence *seq);
void xi_drawable_free(XIDrawable *drawable);
void xi_sound_free(XISound *sound);
void xi_movement_free(XIMovement *move);
void xi_position_free(XIPosition *pos);
void xi_sequence_free(XISequence *seq);
void xi_event_free(XIEvent *evt);
void xi_rect_free(XIRect *rect);
void xi_timed_frame_free(XITimedFrame *frame);
void xi_sequence_update(XISequence *seq, gdouble elapsed);
XIDrawable* xi_drawable_add_copy(XISequence *owner_seq, XIDrawable *drawable_to_copy);
gchar* xi_position_to_string(XIPosition *pos);
XIRect* xi_rect_copy(XIRect *rect);
XIRect* xi_drawable_frames_next(XIDrawableFrames *frames, gdouble elapsed_since_prev);
void xi_drawable_update(XIDrawable *drawable, gdouble elapsed_since_prev);
XITimedFrame* xi_drawable_frames_set_copy(XIDrawableFrames *dframes,
                                          guint index,
                                          XITimedFrame *frame);
XIRect* xi_timed_frame_rect_copy(XITimedFrame *frame);
XIDrawableFrames* xi_drawable_add_frames_copy(XIDrawable *drawable,
                                              gchar const *frames_name,
                                              XIDrawableFrames *frames);
gboolean xi_position_adjust_for_camera(XIPosition *pos, XICamera *cam);
void xi_sequence_set_camera(XISequence *seq, gdouble x, gdouble y, gdouble z);
gboolean xi_sequence_connect_input_to_camera_xy(XISequence *seq);
GHook* xi_sequence_add_listener(XISequence     *seq,
                                XIEventType     evt_type,
                                gint            evt_subtype,
                                gchar const    *event_name,
                                GHookFunc       event_handler,
                                gpointer        handler_data,
                                GDestroyNotify  handler_data_destroy);
void xi_sequence_fire_input_event(XIEvent *input_event);


/* Functions for player/interpreter */
gboolean xi_init_story(XIStory *story, guint curr_w, guint curr_h,
                       guint curr_bpp);
void xi_update(XIStory *story, gdouble elapsed);
void xi_story_free(XIStory *story);
gboolean xi_sequence_is_marked_start_asap(XISequence *seq);
gboolean xi_sequence_is_marked_started(XISequence *seq);
gboolean xi_sequence_is_marked_restartable(XISequence *seq);
gboolean xi_sequence_is_marked_done(XISequence *seq);
gint xi_event_input_subtype_for_listener_key(gchar const *key);
XIEvent* xi_event_new_empty(XIEventType event_type);
XIInput_XY* xi_input_xy_new();


/*****************************************************************************
Primary Structs Convenience Macros - clean-up syntax and supply defaults
******************************************************************************/
#define xi_position_new(...)                    \
  xi_deep_copy_position                         \
  (&(XIPosition)                                \
   {   .layer_name     = NULL,                  \
       .x              = 0.0,                   \
       .y              = 0.0,                   \
       .z              = 0.0,                   \
       .screen_x       = 0.0,                   \
       .screen_y       = 0.0,                   \
       .screen_z       = 0.0,                   \
       .x_relative_to  = NULL,                  \
       .y_relative_to  = NULL,                  \
       .z_relative_to  = NULL,                  \
       .x_relative_str = NULL,                  \
       .y_relative_str = NULL,                  \
       .z_relative_str = NULL,                  \
       __VA_ARGS__})

#define xi_drawable_add(owner_sequence, ...)    \
  xi_drawable_add_copy                          \
  ((owner_sequence),                            \
   &(XIDrawable)                                \
   {   .owner_seq         = NULL,               \
       .instance_name     = NULL,               \
       .name              = NULL,               \
       .type              = NULL,               \
       .pos               = xi_position_new(),  \
       .use_alpha         = TRUE,               \
       .use_alpha_channel = TRUE,               \
       .alpha             = 255,                \
       .prev_alpha        = 255,                \
       .use_colorkey      = FALSE,              \
       .colorkey_red      = 0,                  \
       .colorkey_green    = 0,                  \
       .colorkey_blue     = 0,                  \
       .rect              = NULL,               \
       .frames_name       = NULL,               \
       .frames_by_name    = NULL,               \
       __VA_ARGS__})

#define xi_drawable_frames_set(drawable_frames, \
                               frames_index,    \
                               ...)             \
  xi_drawable_frames_set_copy                   \
  ((drawable_frames),                           \
   (frames_index),                              \
   &(XITimedFrame)                              \
   {.duration = 0,                              \
       .x = 0,                                  \
       .y = 0,                                  \
       .w = 0,                                  \
       .h = 0,                                  \
       __VA_ARGS__})

#define xi_drawable_add_drawable_frames(drawable_instance,              \
                                        drawable_frames_series_name,    \
                                        drawable_frames_count,          \
                                        ...)                            \
  xi_drawable_add_frames_copy                                           \
  ((drawable_instance),                                                 \
   (drawable_frames_series_name),                                       \
   &(XIDrawableFrames)                                                  \
   {.series_name         = (drawable_frames_series_name),               \
       .count            = (drawable_frames_count),                     \
       .offset           = 0,                                           \
       .elapsed_in_frame = 0,                                           \
       .frames           = NULL,                                        \
       __VA_ARGS__})

#define xi_sequence_add_child(parent_sequence,      \
                              child_instance_name,  \
                              ...)                  \
  xi_sequence_add_child_copy                        \
  ((parent_sequence),                               \
   &(XISequence)                                    \
   {   .instance_name  = (child_instance_name),     \
       .name           = NULL,                      \
       .parent         = NULL,                      \
       .children       = NULL,                      \
       .state_name     = NULL,                      \
       .state_specific = NULL,                      \
       .drawables      = NULL,                      \
       .movements      = NULL,                      \
       .sounds         = NULL,                      \
       .hooks          = NULL,                      \
       .pos            = xi_position_new(),         \
       .natural_w      = 0,                         \
       .natural_h      = 0,                         \
       .scale_mode     = "stretch",                 \
       .camera         = NULL,                      \
       .start_at       = 0,                         \
       .start_on       = NULL,                      \
       .start_on_fired = TRUE,                      \
       .start_asap     = FALSE,                     \
       .started        = FALSE,                     \
       .restartable    = FALSE,                     \
       .done           = FALSE,                     \
       .duration       = 0,                         \
       .duration_type  = XI_DURATION_CONTINUOUS,    \
       .rate           = 1.0,                       \
       .elapsed        = 0,                         \
       .prev_elapsed   = 0,                         \
       .listeners      = NULL,                      \
       .event_mask     = 0,                         \
       __VA_ARGS__})

#define xi_story_new(...)                       \
  xi_story_new_from_template                    \
  (&(XIStory)                                   \
   {   .name          = "default_story",        \
       .title         = "No Title",             \
       .revision      = "0.0.0",                \
       .natural_w     = 800,                    \
       .natural_h     = 600,                    \
       .natural_bpp   = 32,                     \
       .curr_w        = 0,                      \
       .curr_h        = 0,                      \
       .curr_bpp      = 0,                      \
       .scale_mode    = "stretch",              \
       .root_seq      = NULL,                   \
       .elapsed       = 0,                      \
       .named_buckets = NULL,                   \
       __VA_ARGS__})

#define xi_event_new(source_sequence,               \
                     event_type, event_subtype,     \
                     ...)                           \
  xi_event_shallow_copy                             \
  (&(XIEvent)                                       \
   {   .type                 = (event_type),        \
       .subtype              = (event_subtype),     \
       .source_seq           = (source_sequence),   \
       .name                 = NULL,                \
       .when                 = 0,                   \
       .x                    = 0,                   \
       .y                    = 0,                   \
       .z                    = 0,                   \
       .speed                = 0,                   \
       .type_data            = NULL,                \
       .type_data_destroy    = NULL,                \
       .handler_data         = NULL,                \
       .handler_data_destroy = NULL,                \
       __VA_ARGS__})

/*****************************************************************************
Effects Data Structures
******************************************************************************/

typedef struct XIData_fade_to_black {
  gchar *instance_name;
  XISequence *owner_seq;
  XISequence *target_seq; /*!< The seq that will be effected */
  guint8 alpha; /*!< Can be 0-255 */
  gdouble start_at; /*!< Delay after 'start_on' or parent start */
  gchar *start_on; /*!< Event string to start on */
  gboolean start_on_fired;
  gboolean start_asap;
  gboolean restartable;
  gdouble duration;
  gdouble rate;
  gdouble fps;
  gboolean fade_from_black;
} XIData_fade_to_black;

/*****************************************************************************
Effects Convenience Macros - clean-up syntax and supply defaults
******************************************************************************/

#define xi_fade_to_black(add_to_sequence,       \
                         fade_inst_name, ...)   \
  xi_add_fade_to_black_copy                     \
  ((add_to_sequence),                           \
   &(XIData_fade_to_black)                      \
   {   .instance_name = (fade_inst_name),       \
       .owner_seq     = NULL,                   \
       .target_seq    = NULL,                   \
       .alpha         = 0,                      \
       .start_at      = 0,                      \
       .start_on      = NULL,                   \
       .start_on_fired = FALSE,                 \
       .start_asap    = FALSE,                  \
       .restartable   = FALSE,                  \
       .duration      = 2.0,                    \
       .rate          = 10.0,                   \
       .fps           = 30.0,                   \
       .fade_from_black = FALSE,                \
       __VA_ARGS__})

#define xi_fade_from_black(add_to_sequence,     \
                           fade_inst_name, ...) \
  xi_add_fade_to_black_copy                     \
  ((add_to_sequence),                           \
   &(XIData_fade_to_black)                      \
   {   .instance_name = (fade_inst_name),       \
       .owner_seq     = NULL,                   \
       .target_seq    = NULL,                   \
       .alpha         = 255,                    \
       .start_at      = 0,                      \
       .start_on      = NULL,                   \
       .start_on_fired = FALSE,                 \
       .start_asap    = FALSE,                  \
       .restartable   = FALSE,                  \
       .duration      = 2.0,                    \
       .rate          = 10.0,                   \
       .fps           = 30.0,                   \
       .fade_from_black = TRUE,                 \
       __VA_ARGS__})


/*****************************************************************************
Effects functions
******************************************************************************/

void xi_add_fade_to_black_copy(XISequence *seq, XIData_fade_to_black *data);


/*****************************************************************************
Other
******************************************************************************/

gboolean     xi_get_value     (GHashTable *hash_table, gconstpointer lookup_key, gpointer *value, gpointer *default_value, GError **err);
gboolean     xi_get_int       (GHashTable *hash_table, gconstpointer lookup_key, gint     *value, gint      default_value, GError **err);

gboolean xi_drawable_select_frame_series(XIDrawable *drawable, gchar const *series_name);
void xi_start_story_asap(XIStory *story);

#endif
