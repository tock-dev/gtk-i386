#include "config.h"

#include "gdkcolorvolumeprivate.h"

#include <glib.h>

/**
 * GdkColorVolume:
 *
 * The `GdkColorVolume` struct contains information about the subset
 * of the representable colors in a color state that is used in a texture,
 * or displayable by an output device.
 *
 * The subset is specified in the same way that a color state's gamut
 * is defined: with chromaticity coordinates for r, g, b primaries and
 * a white point. Additionally, information about minimum, maximum and
 * average luminances is included to fully define the volume.
 *
 * In the context of video mastering, this data is commonly known as
 * 'HDR metadata' or as 'mastering display color volume'. The relevant
 * specifications for this are SMPTE ST 2086 and CEA-861.3, Appendix A.
 *
 * The information in this struct is used in gamut or tone mapping.
 *
 * Since: 4.18
 */

G_DEFINE_BOXED_TYPE (GdkColorVolume, gdk_color_volume,
                     gdk_color_volume_ref, gdk_color_volume_unref);

/**
 * gdk_color_volume_new:
 * @rx: Chromaticity coordinates for the red primary
 * @ry: Chromaticity coordinates for the red primary
 * @gx: Chromaticity coordinates for the green primary
 * @gy: Chromaticity coordinates for the green primary
 * @bx: Chromaticity coordinates for the blue primary
 * @by: Chromaticity coordinates for the blue primary
 * @wx: Chromaticity coordinates for the white point
 * @wy: Chromaticity coordinates for the white point
 * @min_lum: minimum luminance, in cd/m²
 * @max_lum: maximum luminance, in cd/m²
 * @max_cll: maximum content light level, in cd/m²
 * @max_fall: maximum average frame light level, in cd/m²
 *
 * Creates a new `GdkColorVolume` struct with the given data.
 *
 * Returns: a newly allocated `GdkColorVolume` struct
 *
 * Since: 4.18
 */
GdkColorVolume *
gdk_color_volume_new (float rx, float ry,
                      float gx, float gy,
                      float bx, float by,
                      float wx, float wy,
                      float min_lum, float max_lum,
                      float max_cll, float max_fall)
{
  GdkColorVolume *self;

  self = g_new (GdkColorVolume, 1);

  self->ref_count = 1;

  self->rx = rx; self->ry = ry;
  self->gx = gx; self->gy = gy;
  self->bx = bx; self->by = by;
  self->wx = wx; self->wy = wy;
  self->min_lum = min_lum;
  self->max_lum = max_lum;
  self->max_cll = max_cll;
  self->max_fall = max_fall;

  return self;
}

/**
 * gdk_color_volume_ref:
 * @self: a `GdkColorVolume`
 *
 * Increase the reference count of @self.
 *
 * Returns: @self
 *
 * Since: 4.18
 */
GdkColorVolume *
gdk_color_volume_ref (GdkColorVolume *self)
{
  g_atomic_ref_count_inc (&self->ref_count);

  return self;
}

/**
 * gdk_color_volume_unref:
 * @self: a `GdkColorVolume`
 *
 * Decreate the reference count of @self, and
 * free it if the reference count drops to zero.
 *
 * Since: 4.18
 */
void
gdk_color_volume_unref (GdkColorVolume *self)
{
  if (g_atomic_ref_count_dec (&self->ref_count))
    g_free (self);
}

/**
 * gdk_color_volume_equal:
 * @v1: a `GdkColorVolume`
 * @v2: another `GdkColorVolume`
 *
 * Returns whether @v1 and @v2 contain the same data.
 *
 * Returns: `TRUE` if @v1 and @v2 are equal
 *
 * Since: 4.18
 */
gboolean
gdk_color_volume_equal (const GdkColorVolume *v1,
                        const GdkColorVolume *v2)
{
  if (v1 == v2)
    return TRUE;

  if (v1 == NULL || v2 == NULL)
    return FALSE;

  return v1->rx == v2->rx && v1->ry == v2->ry &&
         v1->gx == v2->gx && v1->gy == v2->gy &&
         v1->bx == v2->bx && v1->by == v2->by &&
         v1->wx == v2->wx && v1->wy == v2->wy &&
         v1->min_lum == v2->min_lum &&
         v1->max_lum == v2->max_lum &&
         v1->max_cll == v2->max_cll &&
         v1->max_fall == v2->max_fall;
}
