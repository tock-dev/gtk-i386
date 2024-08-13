#pragma once

#include <glib.h>
#include "gdkcolorvolume.h"

struct _GdkColorVolume
{
  gatomicrefcount ref_count;

  float rx, ry;
  float gx, gy;
  float bx, by;
  float wx, wy;
  float min_lum;
  float max_lum;
  float max_cll;
  float max_fall;
};
