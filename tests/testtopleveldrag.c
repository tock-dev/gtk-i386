/* simple.c
 * Copyright (C) 2017  Red Hat, Inc
 * Author: Benjamin Otte
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */
#include "config.h"
#include <gtk/gtk.h>


static void
hello (void)
{
  g_print ("hello world\n");
}

static void
quit_cb (GtkWidget *widget,
         gpointer   data)
{
  gboolean *done = data;

  *done = TRUE;

  g_main_context_wakeup (NULL);
}

static gboolean
hide_icon (gpointer data)
{
  GdkSurface *surface = data;

  gdk_surface_hide (surface);

  return G_SOURCE_REMOVE;
}

static void
drag_begin (GtkDragSource *source,
            GdkDrag       *drag)
{
  GtkWidget *window;
  GdkSurface *surface;

  gtk_drag_source_set_icon (source, NULL, 0, 0);

  window = gtk_window_new ();
  gtk_window_set_title (GTK_WINDOW (window), "hello toplevel drag");
  gtk_window_present (GTK_WINDOW (window));

  surface = gtk_native_get_surface (GTK_NATIVE (window));

  if (!gdk_drag_attach_toplevel (drag, GDK_TOPLEVEL (surface), 20, 20))
    {
      g_print ("that didn't work\n");
      gtk_window_destroy (GTK_WINDOW (window));
    }

  g_idle_add (hide_icon, gdk_drag_get_drag_surface (drag));
}

int
main (int argc, char *argv[])
{
  GtkWidget *window, *button;
  gboolean done = FALSE;

  gtk_init ();

  window = gtk_window_new ();
  gtk_window_set_title (GTK_WINDOW (window), "hello world");
  gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);

  button = gtk_button_new ();
  gtk_button_set_label (GTK_BUTTON (button), "hello world");
  gtk_widget_set_margin_top (button, 10);
  gtk_widget_set_margin_bottom (button, 10);
  gtk_widget_set_margin_start (button, 10);
  gtk_widget_set_margin_end (button, 10);

  GtkDragSource *drag_source = gtk_drag_source_new ();

  GValue value = G_VALUE_INIT;

  g_value_init (&value, G_TYPE_STRING);
  g_value_set_string (&value, "blabla");

  GdkContentProvider *content = gdk_content_provider_new_for_value (&value);

  gtk_drag_source_set_content (drag_source, content);
  g_object_unref (content);

  g_signal_connect (drag_source, "drag-begin", G_CALLBACK (drag_begin), NULL);

  gtk_widget_add_controller (button, GTK_EVENT_CONTROLLER (drag_source));

  gtk_window_set_child (GTK_WINDOW (window), button);

  gtk_window_present (GTK_WINDOW (window));

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
