/* Notifications
 * #Keywords: GNotification, notification, system
 *
 * This demo showcases the capabilities of the GNotification object, by showing
 * a "messaging app simulation".
 *
 * You can receive any kind of notifications, such as incoming calls
 * (with urgent priority), incoming messages (with normal priority), and you
 * can also set a silent mode (with low priority).
 */

#include <gtk/gtk.h>

#include "notifications.h"

GtkWidget *
do_notifications (GtkWidget *toplevel);

/* Forward declaration, solves cyclic dependency below */
static void
activate (GApplication *app);

static GtkWidget *window = NULL,
                 *messages_box,
                 *stack,
                 *dropdown,
                 *last_call_stack = NULL;
static const char *const message_css =
".message {\n"
"  --bg-color: #fafafa;\n"
"  background-color: var(--bg-color);\n"
"  border-radius: 16px;\n"
"  box-shadow: 0 1px 2px rgba(0, 0, 0, 0.1);\n"
"  padding: 16px;\n"
"}";

static void
append_pair_messages (void)
{
  GtkBuilder *builder;
  GtkWidget *new;

  builder = gtk_builder_new_from_resource ("/notifications/notifications.ui");

  new = GTK_WIDGET (gtk_builder_get_object (builder, "user_message"));

  gtk_box_append (GTK_BOX (messages_box), new);

  new = GTK_WIDGET (gtk_builder_get_object (builder, "response_message"));

  gtk_box_append (GTK_BOX (messages_box), new);

  g_object_unref (builder);
}

/* Additionally, it sets static `last_call_stack` to the `call_stack` object in
 * the builder
 */
static void
append_pair_calls (void)
{
  GtkBuilder *builder;
  GtkWidget *new;

  builder = gtk_builder_new_from_resource ("/notifications/notifications.ui");

  new = GTK_WIDGET (gtk_builder_get_object (builder, "user_request_call"));

  gtk_box_append (GTK_BOX (messages_box), new);

  new = GTK_WIDGET (gtk_builder_get_object (builder, "response_call"));

  gtk_box_append (GTK_BOX (messages_box), new);

  last_call_stack = GTK_WIDGET (gtk_builder_get_object (builder, "call_stack"));

  g_object_unref (builder);
}

static void
activate_send (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  GApplication *app = user_data;
  GObject *selected;
  GNotification *notification;
  GVariant *silent_mode;
  const char *s;

  selected = gtk_drop_down_get_selected_item (GTK_DROP_DOWN (dropdown));
  s = gtk_string_object_get_string (GTK_STRING_OBJECT (selected));

  if (g_strcmp0 (s, "Send message") == 0)
    {
      append_pair_messages ();

      silent_mode = g_action_group_get_action_state (G_ACTION_GROUP (app), "silent-mode");

      notification = g_notification_new ("New message from Jesús R.");
      g_notification_set_body (notification, "Hi! I'm doing great, thanks!");
      g_notification_set_default_action (notification, "app.message");
      g_notification_set_category (notification, "im.received");
      if (g_variant_get_boolean (silent_mode))
        g_notification_set_priority (notification, G_NOTIFICATION_PRIORITY_LOW);
      g_application_send_notification (G_APPLICATION (app), NULL, notification);
    }
  else if (g_strcmp0 (s, "Call me") == 0)
    {
      if (last_call_stack)
        gtk_stack_set_visible_child_name (GTK_STACK (last_call_stack), "missed");

      append_pair_calls ();

      silent_mode = g_action_group_get_action_state (G_ACTION_GROUP (app), "silent-mode");

      notification = g_notification_new ("Incoming call from Jesús R.");
      g_notification_add_button (notification, "Answer", "app.call(true)");
      g_notification_add_button (notification, "Reject", "app.call(false)");
      g_notification_set_category (notification, "call.incoming");
      if (g_variant_get_boolean (silent_mode))
        g_notification_set_priority (notification, G_NOTIFICATION_PRIORITY_LOW);
      else
        g_notification_set_priority (notification, G_NOTIFICATION_PRIORITY_URGENT);
      g_application_send_notification (G_APPLICATION (app), "current-call", notification);
    }
  else
    {
      g_critical ("Unknown send command was selected: %s", s);
      return;
    }

  gtk_stack_set_visible_child_name (GTK_STACK (stack), "messages");
  g_variant_unref (silent_mode);
  g_object_unref (notification);
}

static gboolean
selected_to_text_entry_cb (GBinding *binding,
                           const GValue *source,
                           GValue *target,
                           gpointer user_data)
{
  GObject *str_object;
  const char *s;

  str_object = g_value_get_object (source);
  s = gtk_string_object_get_string (GTK_STRING_OBJECT (str_object));

  if (g_strcmp0 (s, "Send message") == 0)
    g_value_set_string (target, "Hi! How are you doing?");
  else if (g_strcmp0 (s, "Call me") == 0)
    g_value_set_string (target, "Could you please call me back? I ran out of call minutes :-(");
  else
    return FALSE;

  return TRUE;
}

/* This has lifetime app, window may be NULL */
static void
activate_message (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  GApplication *app = user_data;

  if (!window)
    {
      activate (app);

      append_pair_messages ();

      gtk_stack_set_visible_child_name (GTK_STACK (stack), "messages");
    }

  gtk_window_present (GTK_WINDOW (window));
}

/* This has lifetime app, window may be NULL */
static void
activate_call (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
  GApplication *app = user_data;

  if (!window)
    {
      activate (app);

      append_pair_calls ();

      gtk_stack_set_visible_child_name (GTK_STACK (stack), "messages");
    }

  if (g_variant_get_boolean (parameter))
    gtk_stack_set_visible_child_name (GTK_STACK (last_call_stack), "answered");
  else
    gtk_stack_set_visible_child_name (GTK_STACK (last_call_stack), "rejected");

  last_call_stack = NULL;

  gtk_window_present (GTK_WINDOW (window));
}

static void
destroy_cb (gpointer user_data)
{
  GApplication *app = user_data;

  g_application_withdraw_notification (app, "current-call");

  window = NULL;
}

static void
adjustment_changed_cb (GtkAdjustment *adjustment)
{
  /* FIXME: There is a visual bug with the scrolled window,
   * where it shows that it is at the end, but is not really.
   */
  gtk_adjustment_set_value (adjustment, gtk_adjustment_get_upper (adjustment));
}

static void
activate (GApplication *app)
{
  GtkBuilder *builder;
  GtkWidget *text_entry, *messages_scrolled_window;
  GtkAdjustment *adjustment;
  GtkCssProvider *provider;
  GActionEntry win_actions[] = {
    { "send", activate_send, NULL, NULL, NULL },
  };

  provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_string (provider, message_css);

  builder = gtk_builder_new_from_resource ("/notifications/notifications.ui");

  window = gtk_application_window_new (GTK_APPLICATION (app));
  gtk_window_set_titlebar (GTK_WINDOW (window), GTK_WIDGET (gtk_builder_get_object (builder, "header_bar")));
  gtk_window_set_default_size (GTK_WINDOW (window), 700, 600);
  gtk_style_context_add_provider_for_display (gtk_widget_get_display (window),
                                              GTK_STYLE_PROVIDER (provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  g_signal_connect_swapped (window, "destroy", G_CALLBACK (destroy_cb), app);

  gtk_window_set_child (GTK_WINDOW (window),
                        GTK_WIDGET (gtk_builder_get_object (builder, "window_child")));

  dropdown = GTK_WIDGET (gtk_builder_get_object (builder, "dropdown"));
  text_entry = GTK_WIDGET (gtk_builder_get_object (builder, "text_entry"));
  messages_box = GTK_WIDGET (gtk_builder_get_object (builder, "messages_box"));
  stack = GTK_WIDGET (gtk_builder_get_object (builder, "stack"));
  messages_scrolled_window = GTK_WIDGET (gtk_builder_get_object (builder, "messages_scrolled_window"));
  adjustment = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (messages_scrolled_window));
  g_signal_connect (adjustment, "changed", G_CALLBACK (adjustment_changed_cb), NULL);

  g_object_bind_property_full (dropdown, "selected-item",
                               text_entry, "text",
                               G_BINDING_SYNC_CREATE,
                               selected_to_text_entry_cb, NULL, NULL, NULL);

  g_action_map_add_action_entries (G_ACTION_MAP (window),
                                   win_actions, G_N_ELEMENTS (win_actions),
                                   app);

  gtk_window_present (GTK_WINDOW (window));

  g_object_unref (builder);
  g_object_unref (provider);
}

static int
command_line (GApplication *app, GApplicationCommandLine *command_line)
{
  GVariantDict *dict;
  gboolean notifications_enabled;

  dict = g_application_command_line_get_options_dict (command_line);
  g_variant_dict_lookup(dict, "notifications-demo", "b", &notifications_enabled);
  if (notifications_enabled)
    activate (app);
  else
    g_signal_emit_by_name (app, "activate");

  return 0;
}

void
demo_notifications_init_app (GApplication *app)
{
  GSettings *settings;
  GAction *action;
  GOptionEntry entries[] = {
    { "notifications-demo", 0, G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE, NULL, NULL, NULL, },
    { NULL, },
  };
  GActionEntry app_actions[] = {
    { "message", activate_message, NULL, NULL, NULL, },
    /* Parameter: TRUE when it's answered, FALSE when hung up */
    { "call", activate_call, "b", NULL, NULL, },
  };

  settings = g_settings_new ("org.gtk.Demo4");
  action = g_settings_create_action (settings, "silent-mode");

  g_action_map_add_action (G_ACTION_MAP (app), action);

  g_application_add_main_option_entries (G_APPLICATION (app), entries);
  g_signal_connect (app, "command-line", G_CALLBACK (command_line), NULL);
  g_action_map_add_action_entries (G_ACTION_MAP (app),
                                   app_actions, G_N_ELEMENTS (app_actions),
                                   app);

  g_object_unref (action);
  g_object_unref (settings);
}

/* The code below is a modified version of the Application example */

static gboolean name_seen;
static GtkWidget *placeholder;

static void
on_name_appeared (GDBusConnection *connection,
                  const char      *name,
                  const char      *name_owner,
                  gpointer         user_data)
{
  name_seen = TRUE;
}

static void
on_name_vanished (GDBusConnection *connection,
                  const char      *name,
                  gpointer         user_data)
{
  if (!name_seen)
    return;

  g_clear_object (&placeholder);
}

#ifdef G_OS_WIN32
#define APP_EXTENSION ".exe"
#else
#define APP_EXTENSION
#endif

GtkWidget *
do_notifications (GtkWidget *toplevel)
{
  static guint watch = 0;

  if (watch == 0)
    watch = g_bus_watch_name (G_BUS_TYPE_SESSION,
                              "org.gtk.Demo4.App",
                              0,
                              on_name_appeared,
                              on_name_vanished,
                              NULL, NULL);

  if (placeholder == NULL)
    {
      const char *command;
      GError *error = NULL;

      if (g_file_test ("./gtk4-demo-application" APP_EXTENSION, G_FILE_TEST_IS_EXECUTABLE))
        command = "./gtk4-demo-application" APP_EXTENSION " --notifications-demo";
      else
        command = "gtk4-demo-application --notifications-demo";

      if (!g_spawn_command_line_async (command, &error))
        {
          g_warning ("%s", error->message);
          g_error_free (error);
        }

      placeholder = gtk_label_new ("");
      g_object_ref_sink (placeholder);
    }
  else
    g_dbus_connection_call_sync (g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL),
                                 "org.gtk.Demo4.App",
                                 "/org/gtk/Demo4/App",
                                 "org.gtk.Actions",
                                 "Activate",
                                 g_variant_new ("(sava{sv})", "quit", NULL, NULL),
                                 NULL,
                                 0,
                                 G_MAXINT,
                                 NULL, NULL);

  return placeholder;
}
