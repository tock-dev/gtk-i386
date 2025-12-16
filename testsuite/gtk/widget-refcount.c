#include <gtk/gtk.h>

static void
check_finalized (gpointer data,
                 GObject *where_the_object_was)
{
  gboolean *did_finalize = (gboolean *)data;

  *did_finalize = TRUE;
}

static void
popover (void)
{
  GtkWidget *button = gtk_menu_button_new ();
  GtkWidget *p = gtk_popover_new ();
  gboolean finalized = FALSE;

  gtk_menu_button_set_popover (GTK_MENU_BUTTON (button), p);

  /* GtkButton is a normal widget and thus floating */
  g_assert_true (g_object_is_floating (button));
  /* GtkPopver sinks itself */
  g_assert_true (!g_object_is_floating (p));

  g_object_weak_ref (G_OBJECT (p), check_finalized, &finalized);

  g_object_ref_sink (button);
  g_object_unref (button);
  /* We do NOT unref p since the only reference held to it gets
   * removed when the button gets disposed. */
  g_assert_true (finalized);
}

static void
popover2 (void)
{
  GtkWidget *button = gtk_menu_button_new ();
  GtkWidget *p = gtk_popover_new ();
  gboolean finalized = FALSE;

  gtk_menu_button_set_popover (GTK_MENU_BUTTON (button), p);

  g_assert_true (g_object_is_floating (button));
  g_assert_true (!g_object_is_floating (p));

  g_object_weak_ref (G_OBJECT (p), check_finalized, &finalized);

  g_object_ref_sink (button);

  gtk_menu_button_set_popover (GTK_MENU_BUTTON (button), NULL);

  g_assert_true (finalized);

  g_object_unref (button);
}

static void
filechooserwidget (void)
{
  /* We use GtkFileChooserWidget simply because it's a complex widget, that's it. */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  GtkWidget *w = gtk_file_chooser_widget_new (GTK_FILE_CHOOSER_ACTION_OPEN);
G_GNUC_END_IGNORE_DEPRECATIONS
  gboolean finalized = FALSE;

  g_assert_true (g_object_is_floating (w));
  g_object_ref_sink (w);
  g_object_weak_ref (G_OBJECT (w), check_finalized, &finalized);

  g_object_unref (w);

  g_assert_true (finalized);
}

static void
window (void)
{
  GtkWidget *w = gtk_window_new ();
  gboolean finalized = FALSE;

  /* GTK holds a ref */
  g_assert_true (!g_object_is_floating (w));
  g_object_weak_ref (G_OBJECT (w), check_finalized, &finalized);

  gtk_window_destroy (GTK_WINDOW (w));

  g_assert_true (finalized);
}

typedef struct
{
  GtkNotebook *notebook;
  GtkWidget *child;
  GtkWidget *tab_label;
  GtkWidget *menu_label;

  gboolean notebook_finalized;
  gboolean child_finalized;
  gboolean tab_label_finalized;
  gboolean menu_label_finalized;
} NotebookPageTest;

static void
notebook_page_test_setup (NotebookPageTest *data)
{
  data->notebook = GTK_NOTEBOOK (gtk_notebook_new ());
  g_assert_true (g_object_is_floating (data->notebook));
  g_object_ref_sink (data->notebook);

  data->notebook_finalized = FALSE;
  g_object_weak_ref (G_OBJECT (data->notebook), check_finalized, &data->notebook_finalized);

  data->child = gtk_label_new ("child");
  g_assert_true (g_object_is_floating (data->child));
  g_object_ref_sink (data->child);

  data->child_finalized = FALSE;
  g_object_weak_ref (G_OBJECT (data->child), check_finalized, &data->child_finalized);

  data->tab_label = gtk_label_new ("tab label");
  g_assert_true (g_object_is_floating (data->tab_label));
  g_object_ref_sink (data->tab_label);

  data->tab_label_finalized = FALSE;
  g_object_weak_ref (G_OBJECT (data->tab_label), check_finalized, &data->tab_label_finalized);

  data->menu_label = gtk_label_new ("menu label");
  g_assert_true (g_object_is_floating (data->menu_label));
  g_object_ref_sink (data->menu_label);

  data->menu_label_finalized = FALSE;
  g_object_weak_ref (G_OBJECT (data->menu_label), check_finalized, &data->menu_label_finalized);
}

static void
notebook_append_page (void)
{
  NotebookPageTest data;
  notebook_page_test_setup (&data);

  gtk_notebook_append_page_menu (data.notebook, data.child, data.tab_label, data.menu_label);

  g_object_unref (data.child);
  g_assert_false (data.child_finalized);

  g_object_unref (data.tab_label);
  g_assert_false (data.tab_label_finalized);

  g_object_unref (data.menu_label);
  g_assert_false (data.menu_label_finalized);

  gtk_notebook_remove_page (data.notebook, -1);

  g_assert_true (data.child_finalized);
  g_assert_true (data.tab_label_finalized);
  g_assert_true (data.menu_label_finalized);

  g_object_unref (data.notebook);
  g_assert_true (data.notebook_finalized);
}

static void
notebook_set_labels (void)
{
  NotebookPageTest data;
  notebook_page_test_setup (&data);

  gtk_notebook_append_page (data.notebook, data.child, NULL);
  gtk_notebook_set_tab_label (data.notebook, data.child, data.tab_label);
  gtk_notebook_set_menu_label (data.notebook, data.child, data.menu_label);

  g_object_unref (data.child);
  g_assert_false (data.child_finalized);

  g_object_unref (data.tab_label);
  g_assert_false (data.tab_label_finalized);

  g_object_unref (data.menu_label);
  g_assert_false (data.menu_label_finalized);

  gtk_notebook_remove_page (GTK_NOTEBOOK (data.notebook), -1);

  g_assert_true (data.child_finalized);
  g_assert_true (data.tab_label_finalized);
  g_assert_true (data.menu_label_finalized);

  g_object_unref (data.notebook);
  g_assert_true (data.notebook_finalized);
}

static void
notebook_unset_labels (void)
{
  NotebookPageTest data;
  notebook_page_test_setup (&data);

  gtk_notebook_append_page_menu (data.notebook, data.child, data.tab_label, data.menu_label);

  g_object_unref (data.child);
  g_assert_false (data.child_finalized);

  g_object_unref (data.tab_label);
  g_assert_false (data.tab_label_finalized);

  g_object_unref (data.menu_label);
  g_assert_false (data.menu_label_finalized);

  gtk_notebook_set_tab_label (data.notebook, data.child, NULL);
  g_assert_true (data.tab_label_finalized);

  gtk_notebook_set_menu_label (data.notebook, data.child, NULL);
  g_assert_true (data.menu_label_finalized);

  g_object_unref (data.notebook);
  g_assert_true (data.notebook_finalized);
  g_assert_true (data.child_finalized);
}

int
main (int argc, char **argv)
{
  (g_test_init) (&argc, &argv, NULL);
  gtk_init ();

  g_test_add_func ("/gtk/widget-refcount/popover", popover);
  g_test_add_func ("/gtk/widget-refcount/popover2", popover2);
  g_test_add_func ("/gtk/widget-refcount/filechoosewidget", filechooserwidget);
  g_test_add_func ("/gtk/widget-refcount/window", window);
  g_test_add_func ("/gtk/widget-refcount/notebook-append-page", notebook_append_page);
  g_test_add_func ("/gtk/widget-refcount/notebook-set-labels", notebook_set_labels);
  g_test_add_func ("/gtk/widget-refcount/notebook-unset-labels", notebook_unset_labels);

  return g_test_run ();
}
