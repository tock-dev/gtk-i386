/*
 * Copyright © 2013 Canonical Limited
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the licence, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#include "config.h"

#include "gtkapplicationprivate.h"

#ifdef GDK_WINDOWING_X11
#include <gdk/x11/gdkx.h>
#endif

#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/wayland/gdkwayland.h>
#endif

#ifdef GDK_WINDOWING_MACOS
#include <gdk/macos/gdkmacos.h>
#endif

G_DEFINE_TYPE (GtkApplicationImpl, gtk_application_impl, G_TYPE_OBJECT)

enum
{
  PROP_APPLICATION = 1,
  PROP_DISPLAY,
  N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { 0, };

static void
gtk_application_impl_init (GtkApplicationImpl *impl)
{
}

static void
gtk_application_impl_finalize (GObject *object)
{
  GtkApplicationImpl *self = (GtkApplicationImpl *)object;
  g_clear_weak_pointer (&self->application);
  g_clear_weak_pointer (&self->display);
  G_OBJECT_CLASS (gtk_application_impl_parent_class)->finalize (object);
}

static void
gtk_application_impl_get_property (GObject *object,
                                   guint prop_id,
                                   GValue *value,
                                   GParamSpec *pspec)
{
  GtkApplicationImpl *self = (GtkApplicationImpl *)object;
  switch (prop_id)
    {
    case PROP_APPLICATION:
      g_value_set_object (value, self->application);
      break;
    case PROP_DISPLAY:
      g_value_set_object (value, self->display);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gtk_application_impl_set_property (GObject *object,
                                   guint prop_id,
                                   const GValue *value,
                                   GParamSpec *pspec)
{
  GtkApplicationImpl *self = (GtkApplicationImpl *)object;
  switch (prop_id)
    {
    case PROP_APPLICATION:
      g_clear_weak_pointer (&self->application);
      self->application = g_value_get_object (value);
      if (self->application)
        g_object_add_weak_pointer ((GObject*)self->application,
                                   (gpointer*)&self->application);
      break;
    case PROP_DISPLAY:
      g_clear_weak_pointer (&self->display);
      self->display = g_value_get_object (value);
      if (self->display)
        g_object_add_weak_pointer ((GObject*)self->display,
                                   (gpointer*)&self->display);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static guint do_nothing (void) { return 0; }
static gboolean return_false (void) { return FALSE; }

static void
gtk_application_impl_class_init (GtkApplicationImplClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS(class);
  object_class->finalize = gtk_application_impl_finalize;
  object_class->get_property = gtk_application_impl_get_property;
  object_class->set_property = gtk_application_impl_set_property;

  /* NB: can only 'do_nothing' for functions with integer or void return */
  class->startup = (gpointer) do_nothing;
  class->shutdown = (gpointer) do_nothing;
  class->before_emit = (gpointer) do_nothing;
  class->window_added = (gpointer) do_nothing;
  class->window_removed = (gpointer) do_nothing;
  class->active_window_changed = (gpointer) do_nothing;
  class->handle_window_realize = (gpointer) do_nothing;
  class->handle_window_map = (gpointer) do_nothing;
  class->set_app_menu = (gpointer) do_nothing;
  class->set_menubar = (gpointer) do_nothing;
  class->inhibit = (gpointer) do_nothing;
  class->uninhibit = (gpointer) do_nothing;
  class->prefers_app_menu = (gpointer) return_false;

  obj_properties[PROP_APPLICATION] =
      g_param_spec_object ("application", NULL, NULL,
                           GTK_TYPE_APPLICATION,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
  obj_properties[PROP_DISPLAY] =
      g_param_spec_object ("display", NULL, NULL,
                           GDK_TYPE_DISPLAY,
                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS);
  g_object_class_install_properties (object_class, N_PROPERTIES, obj_properties);
}

void
gtk_application_impl_startup (GtkApplicationImpl *impl,
                              gboolean            register_session)
{
  GTK_APPLICATION_IMPL_GET_CLASS (impl)->startup (impl, register_session);
}

void
gtk_application_impl_shutdown (GtkApplicationImpl *impl)
{
  GTK_APPLICATION_IMPL_GET_CLASS (impl)->shutdown (impl);
}

void
gtk_application_impl_before_emit (GtkApplicationImpl *impl,
                                  GVariant           *platform_data)
{
  GTK_APPLICATION_IMPL_GET_CLASS (impl)->before_emit (impl, platform_data);
}

void
gtk_application_impl_window_added (GtkApplicationImpl *impl,
                                   GtkWindow          *window)
{
  GTK_APPLICATION_IMPL_GET_CLASS (impl)->window_added (impl, window);
}

void
gtk_application_impl_window_removed (GtkApplicationImpl *impl,
                                     GtkWindow          *window)
{
  GTK_APPLICATION_IMPL_GET_CLASS (impl)->window_removed (impl, window);
}

void
gtk_application_impl_active_window_changed (GtkApplicationImpl *impl,
                                            GtkWindow          *window)
{
  GTK_APPLICATION_IMPL_GET_CLASS (impl)->active_window_changed (impl, window);
}

void
gtk_application_impl_handle_window_realize (GtkApplicationImpl *impl,
                                            GtkWindow          *window)
{
  GTK_APPLICATION_IMPL_GET_CLASS (impl)->handle_window_realize (impl, window);
}

void
gtk_application_impl_handle_window_map (GtkApplicationImpl *impl,
                                        GtkWindow          *window)
{
  GTK_APPLICATION_IMPL_GET_CLASS (impl)->handle_window_map (impl, window);
}

void
gtk_application_impl_set_app_menu (GtkApplicationImpl *impl,
                                   GMenuModel         *app_menu)
{
  GTK_APPLICATION_IMPL_GET_CLASS (impl)->set_app_menu (impl, app_menu);
}

void
gtk_application_impl_set_menubar (GtkApplicationImpl *impl,
                                  GMenuModel         *menubar)
{
  GTK_APPLICATION_IMPL_GET_CLASS (impl)->set_menubar (impl, menubar);
}

guint
gtk_application_impl_inhibit (GtkApplicationImpl         *impl,
                              GtkWindow                  *window,
                              GtkApplicationInhibitFlags  flags,
                              const char                 *reason)
{
  return GTK_APPLICATION_IMPL_GET_CLASS (impl)->inhibit (impl, window, flags, reason);
}

void
gtk_application_impl_uninhibit (GtkApplicationImpl *impl,
                                guint               cookie)
{
  GTK_APPLICATION_IMPL_GET_CLASS (impl)->uninhibit (impl, cookie);
}

gboolean
gtk_application_impl_prefers_app_menu (GtkApplicationImpl *impl)
{
  return GTK_APPLICATION_IMPL_GET_CLASS (impl)->prefers_app_menu (impl);
}

GtkApplicationImpl *
gtk_application_impl_new (GtkApplication *application,
                          GdkDisplay     *display)
{
  GtkApplicationImpl *impl;
  GType impl_type;

  impl_type = gtk_application_impl_get_type ();

#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_DISPLAY (display))
    impl_type = gtk_application_impl_x11_get_type ();
#endif

#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_DISPLAY (display))
    impl_type = gtk_application_impl_wayland_get_type ();
#endif

#ifdef GDK_WINDOWING_MACOS
  if (GDK_IS_MACOS_DISPLAY (display))
    impl_type = gtk_application_impl_quartz_get_type ();
#endif

  impl = g_object_new (impl_type, "application", application, "display", display, NULL);

  return impl;
}
