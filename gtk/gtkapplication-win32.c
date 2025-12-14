/*
 * GtkApplication implementation for Win32.
 *
 * SPDX-FileCopyrightText: 2025 GNOME Foundation
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

/*
 * ## LOGOFF inhibition
 * https://learn.microsoft.com/en-us/windows/win32/shutdown/logging-off
 * https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ms700677(v=vs.85)
 * It seems that only visible windows are able to block logout requests,
 * so delegate the events handling to the HWND of each application window.
 * Requests counters are managed on GdkWin32 side.
 * Because we need to track added and removed windows, only windows attached
 * to the application are supported.
 *
 * ## IDLE and SUSPEND inhibition
 * https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-powersetrequest
 * Requests counters are automatically managed by PowerSetRequest().
 * The underlying Win32 APIs need a reason string, if not provided the
 * inhibition will be ignored.
 */

#define WIN32_LEAN_AND_MEAN

#include "config.h"

#include "gtkapplicationprivate.h"
#include "gtknative.h"

#include "win32/gdkprivate-win32.h"

#include <windows.h>


typedef GtkApplicationImplClass GtkApplicationImplWin32Class;

typedef struct
{
  GtkApplicationImpl impl;
  GSList *inhibitors;
  guint next_cookie;
} GtkApplicationImplWin32;

typedef struct
{
  guint cookie;
  GtkApplicationInhibitFlags flags;
  HANDLE pwr_handle;
  GdkSurface *surface;
} GtkApplicationWin32Inhibitor;

G_DEFINE_TYPE (GtkApplicationImplWin32, gtk_application_impl_win32, GTK_TYPE_APPLICATION_IMPL)


static void
win32app_inhibitor_free (GtkApplicationWin32Inhibitor *inhibitor)
{
  CloseHandle (inhibitor->pwr_handle);
  g_clear_object (&inhibitor->surface);
  g_free (inhibitor);
}

static void
cb_win32app_session_query_end (void)
{
  GApplication *application = g_application_get_default ();

  if (GTK_IS_APPLICATION (application))
    g_signal_emit_by_name (application, "query-end");
}

static void
cb_win32app_session_end (void)
{
  GApplication *application = g_application_get_default ();

  if (G_IS_APPLICATION (application))
    g_application_quit (application);
}

static void
gtk_application_impl_win32_shutdown (GtkApplicationImpl *impl)
{
  GtkApplicationImplWin32 *win32app = (GtkApplicationImplWin32 *) impl;
  GList *iter;

  for (iter = gtk_application_get_windows (impl->application); iter; iter = iter->next)
    {
      if (!gtk_widget_get_realized (GTK_WIDGET (iter->data)))
        continue;

      gdk_win32_surface_set_session_callbacks (gtk_native_get_surface (GTK_NATIVE (iter->data)),
                                               NULL,
                                               NULL);
    }

  g_slist_free_full (win32app->inhibitors, (GDestroyNotify) win32app_inhibitor_free);
  win32app->inhibitors = NULL;
}

static void
gtk_application_impl_win32_handle_window_realize (GtkApplicationImpl *impl,
                                                  GtkWindow          *window)
{
  g_return_if_fail (GTK_IS_WINDOW (window));

  gdk_win32_surface_set_session_callbacks (gtk_native_get_surface (GTK_NATIVE (window)),
                                           cb_win32app_session_query_end,
                                           cb_win32app_session_end);
}

static void
gtk_application_impl_win32_window_added (GtkApplicationImpl *impl,
                                         GtkWindow          *window,
                                         GVariant           *state)
{
  g_return_if_fail (GTK_IS_WINDOW (window));

  if (!gtk_widget_get_realized (GTK_WIDGET (window)))
    return;   /* no surface, delay to handle_window_realize */

  gdk_win32_surface_set_session_callbacks (gtk_native_get_surface (GTK_NATIVE (window)),
                                           cb_win32app_session_query_end,
                                           cb_win32app_session_end);
}

static void
gtk_application_impl_win32_window_removed (GtkApplicationImpl *impl,
                                           GtkWindow          *window)
{
  GtkApplicationImplWin32 *win32app = (GtkApplicationImplWin32 *) impl;
  GdkSurface *surface = gtk_native_get_surface (GTK_NATIVE (window));
  GSList *iter;

  if (!surface)
    return;

  gdk_win32_surface_set_session_callbacks (surface,
                                           NULL,
                                           NULL);

  /* Ensure we don't keep a ref of the surface in the inhibitors,
   * should the window get removed before uninhibit is called.
   */
  for (iter = win32app->inhibitors; iter; iter = iter->next)
    {
      GtkApplicationWin32Inhibitor *inhibitor = iter->data;

      if ((inhibitor->flags & GTK_APPLICATION_INHIBIT_LOGOUT) &&
          (inhibitor->surface == surface))
        {
          gdk_win32_surface_uninhibit_logout (inhibitor->surface);
          inhibitor->flags &= ~GTK_APPLICATION_INHIBIT_LOGOUT;
          g_clear_object (&inhibitor->surface);
        }
    }
}

static guint
gtk_application_impl_win32_inhibit (GtkApplicationImpl         *impl,
                                    GtkWindow                  *window,
                                    GtkApplicationInhibitFlags  flags,
                                    const char                 *reason)
{
  GtkApplicationImplWin32 *win32app = (GtkApplicationImplWin32 *) impl;
  GtkApplicationWin32Inhibitor *inhibitor = g_new0 (GtkApplicationWin32Inhibitor, 1);
  wchar_t *reason_w = g_utf8_to_utf16 (reason, -1, NULL, NULL, NULL);

  inhibitor->cookie = ++win32app->next_cookie;

  if (flags & GTK_APPLICATION_INHIBIT_LOGOUT)
    {
      if (GTK_IS_WINDOW (window) &&
          impl->application == gtk_window_get_application (window))
        {
          GdkSurface *surface = gtk_native_get_surface (GTK_NATIVE (window));

          if (gdk_win32_surface_inhibit_logout (surface, reason_w))
            {
              inhibitor->surface = g_object_ref (surface);
              inhibitor->flags |= GTK_APPLICATION_INHIBIT_LOGOUT;
            }
        }
      else
        g_warning ("Logout inhibition is only supported on application windows");
    }

  if (flags & (GTK_APPLICATION_INHIBIT_SUSPEND | GTK_APPLICATION_INHIBIT_IDLE))
    {
      REASON_CONTEXT context;

      memset (&context, 0, sizeof (context));
      context.Version = POWER_REQUEST_CONTEXT_VERSION;
      context.Flags = POWER_REQUEST_CONTEXT_SIMPLE_STRING;
      context.Reason.SimpleReasonString = reason_w;

      inhibitor->pwr_handle = PowerCreateRequest (&context);

      if (flags & GTK_APPLICATION_INHIBIT_SUSPEND)
        {
          if (PowerSetRequest (inhibitor->pwr_handle, PowerRequestSystemRequired))
            inhibitor->flags |= GTK_APPLICATION_INHIBIT_SUSPEND;
          else
            g_warning ("Failed to apply suspend inhibition");
        }

      if (flags & GTK_APPLICATION_INHIBIT_IDLE)
        {
          if (PowerSetRequest (inhibitor->pwr_handle, PowerRequestDisplayRequired))
            inhibitor->flags |= GTK_APPLICATION_INHIBIT_IDLE;
          else
            g_warning ("Failed to apply idle inhibition");
        }
    }

  g_free (reason_w);

  if (inhibitor->flags)
    {
      win32app->inhibitors = g_slist_prepend (win32app->inhibitors, inhibitor);
      return inhibitor->cookie;
    }

  win32app_inhibitor_free (inhibitor);
  return 0;
}

static void
gtk_application_impl_win32_uninhibit (GtkApplicationImpl *impl, guint cookie)
{
  GtkApplicationImplWin32 *win32app = (GtkApplicationImplWin32 *) impl;
  GSList *iter;

  for (iter = win32app->inhibitors; iter; iter = iter->next)
    {
      GtkApplicationWin32Inhibitor *inhibitor = iter->data;

      if (inhibitor->cookie == cookie)
        {
          if (inhibitor->flags & GTK_APPLICATION_INHIBIT_LOGOUT)
            gdk_win32_surface_uninhibit_logout (inhibitor->surface);

          if (inhibitor->flags & GTK_APPLICATION_INHIBIT_SUSPEND)
            PowerClearRequest (inhibitor->pwr_handle, PowerRequestSystemRequired);

          if (inhibitor->flags & GTK_APPLICATION_INHIBIT_IDLE)
            PowerClearRequest (inhibitor->pwr_handle, PowerRequestDisplayRequired);

          win32app_inhibitor_free (inhibitor);
          win32app->inhibitors = g_slist_delete_link (win32app->inhibitors, iter);
          return;
        }
    }

  g_warning ("Invalid inhibitor cookie: %d", cookie);
}

static void
gtk_application_impl_win32_class_init (GtkApplicationImplClass *klass)
{
  klass->shutdown = gtk_application_impl_win32_shutdown;
  klass->handle_window_realize = gtk_application_impl_win32_handle_window_realize;
  klass->window_added = gtk_application_impl_win32_window_added;
  klass->window_removed = gtk_application_impl_win32_window_removed;
  klass->inhibit = gtk_application_impl_win32_inhibit;
  klass->uninhibit = gtk_application_impl_win32_uninhibit;
}

static void
gtk_application_impl_win32_init (GtkApplicationImplWin32 *win32app)
{
  win32app->next_cookie = 0;
  win32app->inhibitors = NULL;
}
