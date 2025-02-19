/*
 * Copyright (c) 2024-2025 Marvin Wissfeld
 * Copyright (c) 2025 Florian "sp1rit" <sp1rit@disroot.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include "gdkglcontextprivate.h"
#include "gdkdisplayprivate.h"

#include "gdkandroid.h"

#include "gdkandroidinit-private.h"
#include "gdkandroidsurface-private.h"
#include "gdkandroiddnd-private.h"
#include "gdkandroidglcontext-private.h"

#include <epoxy/egl.h>

struct _GdkAndroidGLContext
{
  GdkGLContext parent_instance;
  union {
    struct {
      GLuint fbo;
      GLuint fbtex;
      gint width;
      gint height;
    } drag;
  };
};

struct _GdkAndroidGLContextClass
{
  GdkGLContextClass parent_class;
};

G_DEFINE_TYPE (GdkAndroidGLContext, gdk_android_gl_context, GDK_TYPE_GL_CONTEXT)

static void
gdk_android_gl_context_constructed (GObject *object)
{
  GdkAndroidGLContext *self = (GdkAndroidGLContext *)object;
  G_OBJECT_CLASS (gdk_android_gl_context_parent_class)->constructed (object);

  GdkSurface *surface = gdk_draw_context_get_surface ((GdkDrawContext *)self);
  if (GDK_IS_ANDROID_DRAG_SURFACE (surface))
    {
      self->drag.width = -1;
      self->drag.height = -1;
    }
}

static void
gdk_android_gl_context_dispose (GObject *object)
{
  GdkAndroidGLContext *self = (GdkAndroidGLContext *)object;
  GdkSurface *surface = gdk_draw_context_get_surface ((GdkDrawContext *)self);
  if (GDK_IS_ANDROID_DRAG_SURFACE (surface))
    if (self->drag.fbo)
      {
        gdk_gl_context_make_current ((GdkGLContext *)self);
        glDeleteTextures(1, &self->drag.fbtex);
        glDeleteFramebuffers(1, &self->drag.fbo);
        self->drag.fbo = 0;
      }
  G_OBJECT_CLASS (gdk_android_gl_context_parent_class)->dispose (object);
}

static void
gdk_android_gl_context_begin_frame (GdkDrawContext  *draw_context,
                                    GdkMemoryDepth   depth,
                                    cairo_region_t  *region,
                                    GdkColorState  **out_color_state,
                                    GdkMemoryDepth  *out_depth)
{
  GdkAndroidGLContext *self = (GdkAndroidGLContext *)draw_context;
  GdkSurface *surface = gdk_draw_context_get_surface (draw_context);

  GDK_DRAW_CONTEXT_CLASS (gdk_android_gl_context_parent_class)->begin_frame (draw_context, depth, region, out_color_state, out_depth);
  if (GDK_IS_ANDROID_DRAG_SURFACE (surface))
    glBindFramebuffer(GL_FRAMEBUFFER, self->drag.fbo);
}

static void
gdk_android_gl_context_end_frame (GdkDrawContext *draw_context,
                                  cairo_region_t *painted)
{
  GdkAndroidGLContext *self = (GdkAndroidGLContext *)draw_context;
  GdkSurface *surface = gdk_draw_context_get_surface (draw_context);

  if (GDK_IS_ANDROID_DRAG_SURFACE (surface))
  {
    GdkAndroidDragSurface *drag_surface = (GdkAndroidDragSurface *)surface;
    GdkAndroidSurface *initiator = GDK_ANDROID_SURFACE (gdk_drag_get_surface (GDK_DRAG (drag_surface->drag)));

    gdk_gl_context_make_current ((GdkGLContext *)self);
    glBindFramebuffer(GL_FRAMEBUFFER, self->drag.fbo);

    JNIEnv *env = gdk_android_get_env();
    (*env)->PushLocalFrame(env, 4);

    jintArray buffer = (*env)->NewIntArray(env, self->drag.width * self->drag.height);
    jint* native_buffer = (*env)->GetIntArrayElements(env, buffer, NULL);
    gdk_gl_context_download ((GdkGLContext *)self,
                             self->drag.fbtex, GDK_MEMORY_DEFAULT, GDK_COLOR_STATE_SRGB,
                             (guchar *)native_buffer, self->drag.width*sizeof(jint),
                             GDK_MEMORY_DEFAULT, GDK_COLOR_STATE_SRGB,
                             self->drag.width, self->drag.height);
    (*env)->ReleaseIntArrayElements(env, buffer, native_buffer, 0);

    g_info("New DragShadow: actual %dx%d", self->drag.width, self->drag.height);
    jobject bitmap = (*env)->CallStaticObjectMethod (env, gdk_android_get_java_cache ()->a_bitmap.klass,
                                                     gdk_android_get_java_cache ()->a_bitmap.create_from_array,
                                                     buffer,
                                                     self->drag.width,
                                                     self->drag.height,
                                                     gdk_android_get_java_cache ()->a_bitmap.argb8888);
    jobject flipped_bitmap = (*env)->CallStaticObjectMethod (env, gdk_android_get_java_cache ()->clipboard_bitmap_drag_shadow.klass,
                                                             gdk_android_get_java_cache ()->clipboard_bitmap_drag_shadow.vflip,
                                                             bitmap);
    jobject shadow = (*env)->NewObject (env,
                                        gdk_android_get_java_cache ()->clipboard_bitmap_drag_shadow.klass,
                                        gdk_android_get_java_cache ()->clipboard_bitmap_drag_shadow.constructor,
                                        initiator->surface,
                                        flipped_bitmap, drag_surface->hot_x, drag_surface->hot_y);

    (*env)->CallVoidMethod (env, initiator->surface,
                            gdk_android_get_java_cache()->surface.update_dnd,
                            shadow);

    (*env)->PopLocalFrame(env, NULL);
  }

  GDK_DRAW_CONTEXT_CLASS (gdk_android_gl_context_parent_class)->end_frame (draw_context, painted);
}

static void
gdk_android_gl_context_empty_frame (GdkDrawContext *draw_context)
{
  GdkSurface *surface = gdk_draw_context_get_surface (draw_context);

  if (GDK_IS_ANDROID_DRAG_SURFACE (surface))
    { /* no-op */ }
  else
    { /* no-op */ }
}

static GdkMemoryDepth
gdk_android_gl_context_ensure_egl_surface (GdkGLContext *gl_context, GdkMemoryDepth depth)
{
  GdkAndroidGLContext *self = (GdkAndroidGLContext *)gl_context;
  GdkSurface *surface = gdk_gl_context_get_surface (gl_context);
  if (GDK_IS_ANDROID_DRAG_SURFACE (surface))
    {
      GdkAndroidDragSurface *surface_impl = (GdkAndroidDragSurface *)surface;
      GdkAndroidSurface *initiator = (GdkAndroidSurface *)gdk_drag_get_surface ((GdkDrag *)surface_impl->drag);

      gdk_gl_context_make_current (gl_context);

      gint scaled_width = ceilf(surface->width * initiator->cfg.scale);
      gint scaled_height = ceilf(surface->height * initiator->cfg.scale);

      if (self->drag.fbo && (self->drag.width != scaled_width || self->drag.height != scaled_height))
        {
          glDeleteTextures(1, &self->drag.fbtex);
          glDeleteFramebuffers(1, &self->drag.fbo);
          self->drag.fbo = 0;
        }

      if (self->drag.fbo == 0)
        {
          self->drag.width = scaled_width;
          self->drag.height = scaled_height;

          glGenFramebuffers(1, &self->drag.fbo);
          glBindFramebuffer(GL_FRAMEBUFFER, self->drag.fbo);

          glGenTextures(1, &self->drag.fbtex);
          glBindTexture(GL_TEXTURE_2D, self->drag.fbtex);

          GLint internal_format, internal_srgb_format, swizzle[4];
          GLenum format, type;
          gdk_memory_format_gl_format (GDK_MEMORY_DEFAULT, TRUE, &internal_format, &internal_srgb_format, &format, &type, swizzle);

          glTexImage2D(GL_TEXTURE_2D, 0, internal_format, self->drag.width, self->drag.height, 0, format, type, NULL);
          glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, self->drag.fbtex, 0);
        }

      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      return GDK_MEMORY_U8;
    }
  else
    return gdk_surface_ensure_egl_surface (surface, depth);
}

static EGLSurface
gdk_android_gl_context_get_egl_surface (GdkGLContext *gl_context)
{
  GdkSurface *surface = gdk_gl_context_get_surface (gl_context);
  if (GDK_IS_ANDROID_DRAG_SURFACE (surface))
    return EGL_NO_SURFACE;
  else
    return gdk_surface_get_egl_surface (surface);
}

static GLuint
gdk_android_gl_context_get_default_framebuffer (GdkGLContext *gl_context)
{
  GdkAndroidGLContext *self = (GdkAndroidGLContext *)gl_context;
  GdkSurface *surface = gdk_gl_context_get_surface (gl_context);
  if (GDK_IS_ANDROID_DRAG_SURFACE (surface))
    return self->drag.fbo;
  return GDK_GL_CONTEXT_CLASS (gdk_android_gl_context_parent_class)->get_default_framebuffer (gl_context);
}

static void
gdk_android_gl_context_class_init (GdkAndroidGLContextClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GdkDrawContextClass *draw_context_class = GDK_DRAW_CONTEXT_CLASS (class);
  GdkGLContextClass *gl_context_class = GDK_GL_CONTEXT_CLASS (class);

  object_class->constructed = gdk_android_gl_context_constructed;
  object_class->dispose = gdk_android_gl_context_dispose;
  draw_context_class->begin_frame = gdk_android_gl_context_begin_frame;
  draw_context_class->end_frame = gdk_android_gl_context_end_frame;
  draw_context_class->empty_frame = gdk_android_gl_context_empty_frame;
  gl_context_class->backend_type = GDK_GL_EGL;
  gl_context_class->get_default_framebuffer = gdk_android_gl_context_get_default_framebuffer;
  gl_context_class->ensure_egl_surface = gdk_android_gl_context_ensure_egl_surface;
  gl_context_class->get_egl_surface = gdk_android_gl_context_get_egl_surface;
}

static void
gdk_android_gl_context_init (GdkAndroidGLContext *self)
{}

/**
 * gdk_android_display_get_egl_display:
 * @display: (transfer none): the display
 *
 * Retrieves the EGL display connection object for the given GDK display.
 *
 * Returns: (nullable): the EGL display
 *
 * Since: 4.18
 */
gpointer
gdk_android_display_get_egl_display (GdkAndroidDisplay *display)
{
  g_return_val_if_fail (GDK_IS_ANDROID_DISPLAY (display), NULL);
  return gdk_display_get_egl_display ((GdkDisplay *)display);
}
