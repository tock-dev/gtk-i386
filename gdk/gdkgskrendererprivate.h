#pragma once

/* Hack. We don't include gsk/gsk.h in gdk to avoid a build order problem
 * with the generated header gskenumtypes.h, so we need to hack around
 * a bit to access the gsk api we need.
 */

typedef struct _GskRenderer GskRenderer;

extern GskRenderer *   gsk_gl_renderer_new                      (void);
extern GskRenderer *   gsk_vulkan_renderer_new                  (void);
extern gboolean        gsk_renderer_realize_for_display         (GskRenderer  *renderer,
                                                                 GdkDisplay   *display,
                                                                 GError      **error);


