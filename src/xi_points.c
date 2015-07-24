#include "xi_points.h"

XIDriveablePoint*
xi_driveable_point_new()
{
  XIDriveablePoint *point = g_new(XIDriveablePoint, 1);

  point->x           = 0;
  point->y           = 0;
  point->z           = 0;
  point->input_x     = 0;
  point->input_y     = 0;
  point->input_z     = 0;
  point->input_up    = 0;
  point->input_down  = 0;
  point->input_left  = 0;
  point->input_right = 0;
  point->input_in    = 0;
  point->input_out   = 0;
  point->speed_x     = 0;
  point->speed_y     = 0;
  point->speed_z     = 0;
  point->max_speed   = 100;
  point->accel_rate  = 8;
  point->decel_rate  = 5;

  return point;
}

XIDriveablePoint*
xi_driveable_point_copy(XIDriveablePoint *point) {
  if(point == NULL) return NULL;

  XIDriveablePoint *new_point = xi_driveable_point_new(NULL);

  new_point->x          = point->x;
  new_point->y          = point->y;
  new_point->z          = point->z;
  new_point->input_x    = point->input_x;
  new_point->input_y    = point->input_y;
  new_point->input_z    = point->input_z;
  new_point->input_up   = point->input_up;
  new_point->input_down = point->input_down;
  new_point->input_left = point->input_left;
  new_point->input_right= point->input_right;
  new_point->input_in   = point->input_in;
  new_point->input_out  = point->input_out;
  new_point->speed_x    = point->speed_x;
  new_point->speed_y    = point->speed_y;
  new_point->speed_z    = point->speed_z;
  new_point->max_speed  = point->max_speed;
  new_point->accel_rate = point->accel_rate;
  new_point->decel_rate = point->decel_rate;

  return new_point;
}

void
xi_driveable_point_free(XIDriveablePoint *point) {
  if(point == NULL) return;
  g_free(point);
}

XIDriveablePointUpdate*
xi_driveable_point_update_new()
{
  XIDriveablePointUpdate *update = g_new(XIDriveablePointUpdate, 1);
  update->point = NULL;
  update->elapsed = NULL;
  return update;
}

void
xi_driveable_point_update_free(XIDriveablePointUpdate* update)
{
  g_free(update);
}

void
xi_driveable_point_to_g_debug(XIDriveablePoint *point)
{
  g_debug("XIDriveablePoint {");
  g_debug(" x            = %g", point->x);
  g_debug(" y            = %g", point->y);
  g_debug(" z            = %g", point->z);
  g_debug(" input_x      = %g", point->input_x);
  g_debug(" input_y      = %g", point->input_y);
  g_debug(" input_z      = %g", point->input_z);
  g_debug(" input_up     = %g", point->input_up);
  g_debug(" input_down   = %g", point->input_down);
  g_debug(" input_left   = %g", point->input_left);
  g_debug(" input_right  = %g", point->input_right);
  g_debug(" input_in     = %g", point->input_in);
  g_debug(" input_out    = %g", point->input_out);
  g_debug(" speed_x      = %g", point->speed_x);
  g_debug(" speed_y      = %g", point->speed_y);
  g_debug(" speed_z      = %g", point->speed_z);
  g_debug(" max_speed    = %g", point->max_speed);
  g_debug(" accel_rate   = %g", point->accel_rate);
  g_debug(" decel_rate   = %g", point->decel_rate);
  g_debug(" elapsed      = %g", point->elapsed);
  g_debug(" prev_elapsed = %g", point->prev_elapsed);
  g_debug("}");
}

void
xi_driveable_point_update(XIDriveablePointUpdate *update)
{
  g_return_if_fail(update != NULL);
  update->point->prev_elapsed = update->point->elapsed;
  update->point->elapsed = *update->elapsed;
}

/*! \brief Update speed_* by gradually accelerating or decelerating.
*/
void
xi_driveable_point_mover_accel_decel(XIDriveablePoint *point, gdouble *speed_xyz,
                                    gdouble accel)
{
  /* Decel of not accelerating and not already stopped */
  if(accel == 0 && *speed_xyz != 0) {
    /* Decelerate */
    if(*speed_xyz > 0) {
      *speed_xyz -= point->decel_rate;
      if(*speed_xyz < 0) {
        *speed_xyz = 0;
      }
    }else if(*speed_xyz < 0){
      *speed_xyz += point->decel_rate;
      if(*speed_xyz > 0) {
        *speed_xyz = 0;
      }
    }
  }else{
    /* Accelerate */
    *speed_xyz += accel;
  }
}

/*! Enforce max_speed. Effective maximum speed is a percentage of
  max_speed determined by input_*  */
void
xi_driveable_point_mover_enforce_max_speed(XIDriveablePoint *point,
                                          gdouble *speed_xyz,
                                          gdouble *input_xyz)
{
  g_return_if_fail(point     != NULL);
  g_return_if_fail(speed_xyz != NULL);
  g_return_if_fail(input_xyz != NULL);

  gdouble effective_max = (abs(*input_xyz) * point->max_speed);

  if(*speed_xyz > effective_max) {
    *speed_xyz = effective_max;
  }else if(*speed_xyz < -effective_max){
    *speed_xyz = -effective_max;
  }
}

/*
  LEFT_OFF

  LEFT_OFF: TODO: Test this with a joystick that has gradual movement
  as opposed to the keyboard which is 100% or off. With medium
  pressure the max speed should only be about half of the max_speed.
 */
void
xi_driveable_point_mover(XIDriveablePoint *point)
{
  g_return_if_fail(point      != NULL);

  /* How hard we're on the gas. Acceleration should reflect the amount
     of input pressure. Accel slower if less pressure, etc..  */
  gdouble x_accel = point->input_x * point->accel_rate;
  gdouble y_accel = point->input_y * point->accel_rate;
  gdouble z_accel = point->input_z * point->accel_rate;

  /* Determine speed in each direction */
  {
    gdouble prev_speed_x = abs(point->speed_x);
    gdouble prev_speed_y = abs(point->speed_y);
    gdouble prev_speed_z = abs(point->speed_z);

    xi_driveable_point_mover_accel_decel(point, &point->speed_x, x_accel);
    xi_driveable_point_mover_accel_decel(point, &point->speed_y, y_accel);
    xi_driveable_point_mover_accel_decel(point, &point->speed_z, z_accel);

    /* Enforce speed cap (max_speed) only if we are accelerating */
    if(abs(point->speed_x) > prev_speed_x)
      xi_driveable_point_mover_enforce_max_speed(point, &point->speed_x, &point->input_x);
    if(abs(point->speed_y) > prev_speed_y)
      xi_driveable_point_mover_enforce_max_speed(point, &point->speed_y, &point->input_y);
    if(abs(point->speed_z) > prev_speed_z)
      xi_driveable_point_mover_enforce_max_speed(point, &point->speed_z, &point->input_z);
  }

  /* Move x, y, z (according to speed and elapsed time) */
  point->x += point->speed_x * (point->elapsed - point->prev_elapsed);
  point->y += point->speed_y * (point->elapsed - point->prev_elapsed);
  point->x += point->speed_z * (point->elapsed - point->prev_elapsed);

  xi_driveable_point_to_g_debug(point);
}
