#include "snappaintable.h"

#include <gtk/gtk.h>

struct _SnapPaintable
{
  GObject parent_instance;
  GFile *file;
  gsize width;
  gsize height;
  GskRectSnap snap;
  int zoom;
  gboolean tiles;
  GskRenderNode *node;
  gboolean processing;
};

/* {{{ Utilities */

static void
set_processing (SnapPaintable *self,
                gboolean       processing)
{
  if (self->processing == processing)
    return;

  self->processing = processing;

  g_object_notify (G_OBJECT (self), "processing");
}

static float
get_zoom (SnapPaintable *self)
{
  return powf (1.2, self->zoom);
}

/* }}} */
/* {{{ Creating nodes */

typedef struct
{
  GFile *file;
  GskRectSnap snap;
  gboolean tiles;
} LoadInput;

static void
load_input_free (gpointer data)
{
  LoadInput *d = data;
  g_object_unref (d->file);
  g_free (d);
}

typedef struct
{
  gsize width;
  gsize height;
  GskRenderNode *node;
} LoadResult;

static void
load_result_free (gpointer data)
{
  LoadResult *d = data;
  gsk_render_node_unref (d->node);
  g_free (d);
}

static void
load_texture (GTask        *task,
              gpointer      source_object,
              gpointer      task_data,
              GCancellable *cable)
{
  LoadInput *input = task_data;
  LoadResult *result;
  GBytes *bytes;
  GtkSnapshot *snapshot;
  gsize size, width, height, stride;
  GdkTexture *texture;
  GdkTextureDownloader *downloader;
  GskRenderNode *node;
  GError *error = NULL;

  texture = gdk_texture_new_from_file (input->file, &error);
  if (!texture)
    {
      g_task_return_error (task, error);
      return;
    }

  width = gdk_texture_get_width (texture);
  height = gdk_texture_get_height (texture);

  downloader = gdk_texture_downloader_new (texture);

  bytes = gdk_texture_downloader_download_bytes (downloader, &stride);
  size = g_bytes_get_size (bytes);

  gdk_texture_downloader_free (downloader);
  g_object_unref (texture);

  snapshot = gtk_snapshot_new ();
  gtk_snapshot_set_snap (snapshot, input->snap);

  if (input->tiles)
    {
      for (gsize y = 0; y < height; y += 64)
        {
          for (gsize x = 0; x < width; x += 64)
            {
              gsize w, h;
              gsize offset;
              GBytes *b;

              w = MIN (64, width - x);
              h = MIN (64, height - y);

              offset = y * stride + x * 4;

              b = g_bytes_new_from_bytes (bytes, offset, size - offset);

              texture = gdk_memory_texture_new (w, h, GDK_MEMORY_DEFAULT, b, stride);
              gtk_snapshot_append_texture (snapshot,
                                           texture,
                                           &GRAPHENE_RECT_INIT (0, 0,
                                                                gdk_texture_get_width (texture),
                                                                gdk_texture_get_height (texture)));
              gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (64.0, 0.0));

              g_object_unref (texture);
              g_bytes_unref (b);
            }
          gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (-64.0 * ceil (width / 64.0), 64.0));
        }
      gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (0.0, -64.0 * ceil (height / 64.0)));
    }
  else /* pixels */
    {
      const guchar *data;

      data = g_bytes_get_data (bytes, &size);

      for (gsize y = 0; y < height; y++)
        {
          const guchar *row = data + y * stride;

          for (gsize x = 0; x < width; x++)
            {
              float b = row[4*x] / 255.0;
              float g = row[4*x + 1] / 255.0;
              float r = row[4*x + 2] / 255.0;
              float a = row[4*x + 3] / 255.0;

              gtk_snapshot_append_color (snapshot,
                                         &(GdkRGBA) { r, g, b, a },
                                         &GRAPHENE_RECT_INIT (x, y, 1, 1));
            }
        }
    }

  node = gtk_snapshot_free_to_node (snapshot);

  g_bytes_unref (bytes);

  result = g_new0 (LoadResult, 1);

  result->width = width;
  result->height = height;
  result->node = node;

  g_task_return_pointer (task, result, load_result_free);
}

static void
texture_loaded (GObject      *source,
                GAsyncResult *result,
                gpointer      data)
{
  SnapPaintable *self = SNAP_PAINTABLE (source);
  LoadResult *res;
  GError *error = NULL;

  res = g_task_propagate_pointer (G_TASK (result), &error);

  if (!res)
    {
      g_print ("%s\n", error->message);
      g_error_free (error);
      return;
    }

  g_clear_pointer (&self->node, gsk_render_node_unref);
  self->node = gsk_render_node_ref (res->node);
  self->width = res->width;
  self->height = res->height;

  load_result_free (res);

  set_processing (self, FALSE);
  gdk_paintable_invalidate_size (GDK_PAINTABLE (self));
  gdk_paintable_invalidate_contents (GDK_PAINTABLE (self));
}

static void
recreate_node (SnapPaintable *self)
{
  LoadInput *input;
  GTask *task;

  if (self->processing)
    return;

  set_processing (self, TRUE);

  input = g_new0 (LoadInput, 1);
  input->file = g_object_ref (self->file);
  input->snap = self->snap;
  input->tiles = self->tiles;

  task = g_task_new (self, NULL, texture_loaded, NULL);
  g_task_set_task_data (task, input, load_input_free);
  g_task_run_in_thread (task, load_texture);
  g_object_unref (task);
}

/* }}} */
/* {{{ GdkPaintable implementation */

static void
snap_paintable_snapshot (GdkPaintable *paintable,
                         GdkSnapshot  *snapshot,
                         double        width,
                         double        height)
{
  SnapPaintable *self = SNAP_PAINTABLE (paintable);

  if (!self->node)
    return;

  gtk_snapshot_push_clip (snapshot, &GRAPHENE_RECT_INIT (0, 0, width, height));
  gtk_snapshot_save (snapshot);
  gtk_snapshot_scale (snapshot, get_zoom (self), get_zoom (self));
  gtk_snapshot_append_node (snapshot, self->node);
  gtk_snapshot_restore (snapshot);
  gtk_snapshot_pop (snapshot);
}

static int
snap_paintable_get_intrinsic_width (GdkPaintable *paintable)
{
  SnapPaintable *self = SNAP_PAINTABLE (paintable);

  return ceil (get_zoom (self) * self->width);
}

static int
snap_paintable_get_intrinsic_height (GdkPaintable *paintable)
{
  SnapPaintable *self = SNAP_PAINTABLE (paintable);

  return ceil (get_zoom (self) * self->height);
}

static void
snap_paintable_init_interface (GdkPaintableInterface *iface)
{
  iface->snapshot = snap_paintable_snapshot;
  iface->get_intrinsic_width = snap_paintable_get_intrinsic_width;
  iface->get_intrinsic_height = snap_paintable_get_intrinsic_height;
}

/* }}} */
/* {{{ GObject boilerplate */

struct _SnapPaintableClass
{
  GObjectClass parent_class;
};

enum {
  PROP_FILE = 1,
  PROP_SNAP,
  PROP_ZOOM,
  PROP_TILES,
  PROP_PROCESSING,
  NUM_PROPERTIES
};

G_DEFINE_TYPE_WITH_CODE (SnapPaintable, snap_paintable, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GDK_TYPE_PAINTABLE,
                                                snap_paintable_init_interface))

static void
snap_paintable_init (SnapPaintable *self)
{
}

static void
snap_paintable_dispose (GObject *object)
{
  SnapPaintable *self = SNAP_PAINTABLE (object);

  g_clear_object (&self->file);
  g_clear_pointer (&self->node, gsk_render_node_unref);

  G_OBJECT_CLASS (snap_paintable_parent_class)->dispose (object);
}

static void
snap_paintable_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  SnapPaintable *self = SNAP_PAINTABLE (object);

  switch (prop_id)
    {
    case PROP_FILE:
      snap_paintable_set_file (self, g_value_get_object (value));
      break;

    case PROP_SNAP:
      snap_paintable_set_snap (self, g_value_get_uint (value));
      break;

    case PROP_ZOOM:
      snap_paintable_set_zoom (self, g_value_get_int (value));
      break;

    case PROP_TILES:
      snap_paintable_set_tiles (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
snap_paintable_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  SnapPaintable *self = SNAP_PAINTABLE (object);

  switch (prop_id)
    {
    case PROP_FILE:
      g_value_set_object (value, self->file);
      break;

    case PROP_SNAP:
      g_value_set_uint (value, self->snap);
      break;

    case PROP_ZOOM:
      g_value_set_int (value, self->snap);
      break;

    case PROP_TILES:
      g_value_set_boolean (value, self->tiles);
      break;

    case PROP_PROCESSING:
      g_value_set_boolean (value, self->processing);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
snap_paintable_class_init (SnapPaintableClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->dispose = snap_paintable_dispose;
  object_class->set_property = snap_paintable_set_property;
  object_class->get_property = snap_paintable_get_property;

  g_object_class_install_property (object_class, PROP_FILE,
                                   g_param_spec_object ("file", NULL, NULL,
                                                        G_TYPE_FILE,
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_SNAP,
                                   g_param_spec_uint ("snap", NULL, NULL,
                                                      0, G_MAXUINT, GSK_RECT_SNAP_NONE,
                                                      G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_ZOOM,
                                   g_param_spec_int ("zoom", NULL, NULL,
                                                      G_MININT, G_MAXINT, 0,
                                                      G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_TILES,
                                   g_param_spec_boolean ("tiles", NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_PROCESSING,
                                   g_param_spec_boolean ("processing", NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READABLE));
}

/* }}} */
/* {{{ Public API */

SnapPaintable *
snap_paintable_new (GFile *file)
{
  return g_object_new (SNAP_TYPE_PAINTABLE,
                       "file", file,
                       NULL);
}

void
snap_paintable_set_file (SnapPaintable *self,
                         GFile         *file)
{
  g_return_if_fail (SNAP_IS_PAINTABLE (self));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  if (!g_set_object (&self->file, file))
    return;

  recreate_node (self);

  g_object_notify (G_OBJECT (self), "file");
}

GskRectSnap
snap_paintable_get_snap (SnapPaintable *self)
{
  g_return_val_if_fail (SNAP_IS_PAINTABLE (self), GSK_RECT_SNAP_NONE);

  return self->snap;
}

void
snap_paintable_set_snap (SnapPaintable *self,
                         GskRectSnap    snap)
{
  g_return_if_fail (SNAP_IS_PAINTABLE (self));

  if (self->snap == snap)
    return;

  self->snap = snap;

  recreate_node (self);

  g_object_notify (G_OBJECT (self), "snap");
}

int
snap_paintable_get_zoom (SnapPaintable *self)
{
  g_return_val_if_fail (SNAP_IS_PAINTABLE (self), 0);

  return self->zoom;
}

void
snap_paintable_set_zoom (SnapPaintable *self,
                         int            zoom)
{
  g_return_if_fail (SNAP_IS_PAINTABLE (self));

  if (self->zoom == zoom)
    return;

  self->zoom = zoom;

  gdk_paintable_invalidate_size (GDK_PAINTABLE (self));
  gdk_paintable_invalidate_contents (GDK_PAINTABLE (self));
  g_object_notify (G_OBJECT (self), "zoom");
}

gboolean
snap_paintable_get_tiles (SnapPaintable *self)
{
  g_return_val_if_fail (SNAP_IS_PAINTABLE (self), FALSE);

  return self->tiles;
}

void
snap_paintable_set_tiles (SnapPaintable *self,
                          gboolean       tiles)
{
  g_return_if_fail (SNAP_IS_PAINTABLE (self));

  if (self->tiles == tiles)
    return;

  self->tiles = tiles;

  recreate_node (self);

  g_object_notify (G_OBJECT (self), "tiles");
}

/* }}} */

/* vim:set foldmethod=marker: */
