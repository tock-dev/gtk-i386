/* GTK - The GIMP Toolkit
 * Copyright (C) 2021 Red Hat, Inc.
 *
 * Authors:
 * - Matthias Clasen <mclasen@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gtkpanedhandleprivate.h"
#include "gtkwidgetprivate.h"
#include "gtkcssnodeprivate.h"
#include "gtkcssstyleprivate.h"
#include "gtkrendericonprivate.h"
#include "gtkcssboxesprivate.h"
#include "gtkprivate.h"

#include "gtkpaned.h"


G_DEFINE_TYPE (GtkPanedHandle, gtk_paned_handle, GTK_TYPE_WIDGET);

static void
gtk_paned_handle_snapshot (GtkWidget   *widget,
                           GtkSnapshot *snapshot)
{
  GtkCssStyle *style = gtk_css_node_get_style (gtk_widget_get_css_node (widget));
  int width, height;

  width = gtk_widget_get_width (widget);
  height = gtk_widget_get_height (widget);

  if (width > 0 && height > 0)
    gtk_css_style_snapshot_icon (style, snapshot, width, height);
}

static void
gtk_paned_handle_class_init (GtkPanedHandleClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->snapshot = gtk_paned_handle_snapshot;

  gtk_widget_class_set_css_name (widget_class, I_("separator"));
}

static void
gtk_paned_handle_init (GtkPanedHandle *self)
{
}

GtkWidget *
gtk_paned_handle_new (void)
{
  return g_object_new (GTK_TYPE_PANED_HANDLE, NULL);
}
