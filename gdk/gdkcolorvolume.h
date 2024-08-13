#pragma once

#if !defined (__GDK_H_INSIDE__) && !defined (GTK_COMPILATION)
#error "Only <gdk/gdk.h> can be included directly."
#endif

#include <gdk/gdktypes.h>

G_BEGIN_DECLS

#define GDK_TYPE_COLOR_VOLUME (gdk_color_volume_get_type ())

GDK_AVAILABLE_IN_4_18
GType            gdk_color_volume_get_type (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_4_18
GdkColorVolume * gdk_color_volume_new   (float rx, float ry,
                                         float gx, float gy,
                                         float bx, float by,
                                         float wx, float wy,
                                         float min_lum, float max_lum,
                                         float max_cll, float max_fall);

GDK_AVAILABLE_IN_4_18
GdkColorVolume * gdk_color_volume_ref      (GdkColorVolume *self);

GDK_AVAILABLE_IN_4_18
void             gdk_color_volume_unref    (GdkColorVolume *self);

GDK_AVAILABLE_IN_4_18
gboolean         gdk_color_volume_equal (const GdkColorVolume *v1,
                                         const GdkColorVolume *v2);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(GdkColorVolume, gdk_color_volume_unref);


G_END_DECLS
