#pragma once

#include <glib.h>

typedef struct {
  /* Timestamp of the latest wheel click which is the
   * beginning time of the animation.
   */
  gint64 start_time;

  // Scroll position from where the animation starts.
  double start;

  /* Speed of the animation at its beginning. This is
   * the derivative of the animation curve at its
   * start.
   */
  double start_speed;

  /* Target scroll position of the animation. The
   * animation curve must reach this point at
   * start_time + duration.
   */
  double target;

  // How much time the animation will durate (in microseconds).
  double duration;
} GtkScrollInterpolationAnimation;

/*
  Interpolation animation formulas explanations :

  s : start position of the curve
  w : final position of the curve
  v : speed at the start of the curve
  t : duration of the animation

  ax³ + bx² + cx + d    Position formula
  3ax² + 2bx + c        Derivate of the position formula, so it's the "speed" formula

  We have 4 equalities that set the conditions of the system :

  a0³ + b0² + c0 + d = s    At time 0, the position is the desired start position.
  3a0² + 2b0 + c = v        At time 0, the speed is the desired start speed.
  at³ + bt² + ct + d = w    At the end of the animation, the position is the desired target position.
  3at² + 2bt + c = 0        At the end of the animation, the speed is 0 (smooth end).

  "s", "w", "v" and "t" are known values. We need to express "a", "b", "c", and
  "d" from these values.

  a0³ + b0² + c0 + d = s
  d = s

  3a0² + 2b0 + c = v
  c = v

  We have "c" and "d". We're left with "a" and "b".

  at³ + bt² + ct + d = w

  at³ = w - d - ct - bt²

      w - d - ct - bt²
  a = ----------------
             t³

  We have "a" but it is calculated with "b" which is unknown.
  So we reinject this definition of "a" in the last equality.

  3at² + 2bt + c = 0

      w - d - ct - bt²
  3 * ---------------- * t² + 2bt + c = 0
             t³

  ...

      3w - 3d - 2ct
  b = -------------
           t²

  We have expressed "b" with known values only. So "b" become a known value and
  "a" too!
*/
static inline double
gtk_scroll_interpolation_animation_get_current_value (const GtkScrollInterpolationAnimation *animation,
                                                      const double                           current_time)
{
  const double t = animation->duration;
  const double w = animation->target;

  const double d = animation->start;
  const double c = animation->start_speed;

  const double b = (3 * w - 3 * d - 2 * c * t) / (t * t);
  const double a = (w - d - c * t - b * t * t) / (t * t * t);

  const double x = current_time - animation->start_time;

  return a * x * x * x + b * x * x + c * x + d;
}

static inline double
gtk_scroll_interpolation_animation_get_current_speed (const GtkScrollInterpolationAnimation *animation,
                                                      const double                           current_time)
{
  const double t = animation->duration;
  const double w = animation->target;

  const double d = animation->start;
  const double c = animation->start_speed;

  const double b = (3 * w - 3 * d - 2 * c * t) / (t * t);
  const double a = (w - d - c * t - b * t * t) / (t * t * t);

  const double x = current_time - animation->start_time;

  return 3 * a * x * x + 2 * b * x + c;
}