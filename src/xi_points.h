#ifndef __XI_POINTS_H_
#define __XI_POINTS_H_

#include <stdlib.h>
#include <glib.h>
#include <libintl.h>
/* Gettext '_(String)' macro pretty much recommended:
   http://www.gnu.org/software/gettext/manual/html_node/Mark-Keywords.html */
#define _(String) gettext (String)


// TODO: Rename input_* to pressure_*
/* "Drive" a point (like a car) around using gas pedals (input_*) */
typedef struct XIDriveablePoint {
  struct XISequence *seq; /* Needed for elapsed time */
  gdouble x; /* position */
  gdouble y; /* position */
  gdouble z; /* position */
  gdouble input_x; /* Input pressure. percentage: -1.0 to 1.0 */
  gdouble input_y; /* Input pressure. percentage: -1.0 to 1.0 */
  gdouble input_z; /* Input pressure. percentage: -1.0 to 1.0 */
  gdouble input_up;    /* Input pressure. percentage: 0.0 to 1.0 */
  gdouble input_down;  /* Input pressure. percentage: 0.0 to 1.0 */
  gdouble input_left;  /* Input pressure. percentage: 0.0 to 1.0 */
  gdouble input_right; /* Input pressure. percentage: 0.0 to 1.0 */
  gdouble input_in;    /* Input pressure. percentage: 0.0 to 1.0 */
  gdouble input_out;   /* Input pressure. percentage: 0.0 to 1.0 */
  gdouble speed_x; /* Current speed */
  gdouble speed_y; /* Current speed */
  gdouble speed_z; /* Current speed */
  gdouble max_speed; /* Max speed in any direction */
  gdouble accel_rate; /* Max acceleration in any direction */
  gdouble decel_rate; /* Max deceleration in any direction */
  gdouble elapsed;
  gdouble prev_elapsed;
} XIDriveablePoint;

typedef struct XIDriveablePointUpdate {
  XIDriveablePoint *point;
  gdouble *elapsed;
} XIDriveablePointUpdate;


XIDriveablePoint* xi_driveable_point_new();
void              xi_driveable_point_free(XIDriveablePoint *point);
XIDriveablePoint* xi_driveable_point_copy(XIDriveablePoint *point);
void              xi_driveable_point_update(XIDriveablePointUpdate *update);
void              xi_driveable_point_mover(XIDriveablePoint *point);

XIDriveablePointUpdate* xi_driveable_point_update_new();
void                    xi_driveable_point_update_free();

#endif
