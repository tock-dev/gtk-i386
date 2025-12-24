/* GSK - The GTK Scene Kit
 *
 * Copyright 2025 Red Hat, Inc.
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

#include "gskarithmeticnode.h"

#include "gskrendernodeprivate.h"
#include "gskrenderreplay.h"
#include "gskcontainernode.h"

#include "gdk/gdkcairoprivate.h"

/**
 * GskArithmeticNode:
 *
 * A render node applying the 'arithmetic' composite operator,
 * as defined in the
 * [CSS filter effects](https://www.w3.org/TR/filter-effects-1/)
 * spec,
 *
 *     result = k1 * i1 * i1 + k2 * i1 + k3 * i3 + k4
 */
struct _GskArithmeticNode
{
  GskRenderNode render_node;

  union {
    GskRenderNode *children[2];
    struct {
      GskRenderNode *first;
      GskRenderNode *second;
    };
  };
  float factors[4];
};

static void
gsk_arithmetic_node_finalize (GskRenderNode *node)
{
  GskArithmeticNode *self = (GskArithmeticNode *) node;
  GskRenderNodeClass *parent_class = g_type_class_peek (g_type_parent (GSK_TYPE_ARITHMETIC_NODE));

  gsk_render_node_unref (self->first);
  gsk_render_node_unref (self->second);

  parent_class->finalize (node);
}

static void
gsk_arithmetic_node_draw (GskRenderNode *node,
                          cairo_t       *cr,
                          GskCairoData  *data)
{
  GskArithmeticNode *self = (GskArithmeticNode *) node;
  int width, height;
  cairo_surface_t *surface1, *surface2, *surface;
  cairo_t *cr2;
  gsize stride;
  guchar *pixels1, *pixels2, *pixels;
  guint32 pixel1, pixel2, pixel;
  float r1, g1, b1, a1, r2, g2, b2, a2, r, g, b, a;
  float k1, k2, k3, k4;
  cairo_pattern_t *pattern;

  if (gdk_cairo_is_all_clipped (cr))
    return;

  k1 = self->factors[0];
  k2 = self->factors[1];
  k3 = self->factors[2];
  k4 = self->factors[3];

  if (!gdk_color_state_equal (data->ccs, GDK_COLOR_STATE_SRGB))
    g_warning ("arithmetic node in non-srgb colorstate isn't implemented yet.");

  width = ceil (node->bounds.size.width);
  height = ceil (node->bounds.size.height);

  surface1 = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);

  cr2 = cairo_create (surface1);
  gsk_render_node_draw_full (self->first, cr2, data);
  cairo_destroy (cr2);

  pixels1 = cairo_image_surface_get_data (surface1);
  stride = cairo_image_surface_get_stride (surface1);

  surface2 = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);

  cr2 = cairo_create (surface2);
  gsk_render_node_draw_full (self->second, cr2, data);
  cairo_destroy (cr2);

  pixels2 = cairo_image_surface_get_data (surface2);

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);

  pixels = cairo_image_surface_get_data (surface);

  for (guint y = 0; y < height; y++)
    {
      for (guint x = 0; x < width; x++)
        {
          pixel1 = *(guint32 *)(pixels1 + y * stride + 4 * x);

          a1 = ((pixel1 >> 24) & 0xff) / 255.;
          r1 = ((pixel1 >> 16) & 0xff) / 255.;
          g1 = ((pixel1 >> 8) & 0xff) / 255.;
          b1 = ((pixel1 >> 0) & 0xff) / 255.;

          if (a1 != 0)
            {
              r1 /= a1;
              g1 /= a1;
              b1 /= a1;
            }

          pixel2 = *(guint32 *)(pixels2 + y * stride + 4 * x);

          a2 = ((pixel2 >> 24) & 0xff) / 255.;
          r2 = ((pixel2 >> 16) & 0xff) / 255.;
          g2 = ((pixel2 >> 8) & 0xff) / 255.;
          b2 = ((pixel2 >> 0) & 0xff) / 255.;

          if (a2 != 0)
            {
              r2 /= a2;
              g2 /= a2;
              b2 /= a2;
            }

          a = k1 * a1 * a2 + k2 * a1 + k3 * a2 + k4;
          r = k1 * r1 * r2 + k2 * r1 + k3 * r2 + k4;
          g = k1 * g1 * g2 + k2 * g1 + k3 * g2 + k4;
          b = k1 * b1 * b2 + k2 * b1 + k3 * b2 + k4;

          a = CLAMP (a, 0, 1);
          r = CLAMP (r, 0, 1);
          g = CLAMP (g, 0, 1);
          b = CLAMP (b, 0, 1);

          r *= a;
          g *= a;
          b *= a;

          pixel = CLAMP ((int) roundf (a * 255), 0, 255) << 24 |
                  CLAMP ((int) roundf (r * 255), 0, 255) << 16 |
                  CLAMP ((int) roundf (g * 255), 0, 255) << 8 |
                  CLAMP ((int) roundf (b * 255), 0, 255) << 0;

          *(guint32 *)(pixels + y * stride + 4 * x) = pixel;
        }
    }
  cairo_surface_destroy (surface1);
  cairo_surface_destroy (surface2);

  cairo_surface_mark_dirty (surface);

  pattern = cairo_pattern_create_for_surface (surface);
  cairo_pattern_set_extend (pattern, CAIRO_EXTEND_PAD);

  cairo_set_source (cr, pattern);
  cairo_pattern_destroy (pattern);
  cairo_surface_destroy (surface);

  gdk_cairo_rect (cr, &node->bounds);
  cairo_fill (cr);
}

static void
gsk_arithmetic_node_diff (GskRenderNode *node1,
                          GskRenderNode *node2,
                          GskDiffData   *data)
{
  GskArithmeticNode *self1 = (GskArithmeticNode *) node1;
  GskArithmeticNode *self2 = (GskArithmeticNode *) node2;

  if (self1->factors[0] == self2->factors[0] &&
      self1->factors[1] == self2->factors[1] &&
      self1->factors[2] == self2->factors[2] &&
      self1->factors[3] == self2->factors[3])
    {
      gsk_render_node_diff (self1->first, self2->first, data);
      gsk_render_node_diff (self1->second, self2->second, data);
    }
  else
    {
      gsk_render_node_diff_impossible (node1, node2, data);
    }
}

static GskRenderNode **
gsk_arithmetic_node_get_children (GskRenderNode *node,
                                  gsize         *n_children)
{
  GskArithmeticNode *self = (GskArithmeticNode *) node;

  *n_children = G_N_ELEMENTS (self->children);

  return self->children;
}

static GskRenderNode *
gsk_arithmetic_node_replay (GskRenderNode   *node,
                            GskRenderReplay *replay)
{
  GskArithmeticNode *self = (GskArithmeticNode *) node;
  GskRenderNode *result, *first, *second;

  first = gsk_render_replay_filter_node (replay, self->first);
  second = gsk_render_replay_filter_node (replay, self->second);

  if (first == NULL)
    {
      if (second == NULL)
        return NULL;

      first = gsk_container_node_new (NULL, 0);
    }
  else if (second == NULL)
    {
      second = gsk_container_node_new (NULL, 0);
    }

  if (first == self->first && second == self->second)
    result = gsk_render_node_ref (node);
  else
    result = gsk_arithmetic_node_new (first, second, self->factors);

  gsk_render_node_unref (first);
  gsk_render_node_unref (second);

  return result;
}

static void
gsk_arithmetic_node_class_init (gpointer g_class,
                                gpointer class_data)
{
  GskRenderNodeClass *node_class = g_class;

  node_class->node_type = GSK_ARITHMETIC_NODE;

  node_class->finalize = gsk_arithmetic_node_finalize;
  node_class->draw = gsk_arithmetic_node_draw;
  node_class->diff = gsk_arithmetic_node_diff;
  node_class->get_children = gsk_arithmetic_node_get_children;
  node_class->replay = gsk_arithmetic_node_replay;
}

GSK_DEFINE_RENDER_NODE_TYPE (GskArithmeticNode, gsk_arithmetic_node)

/**
 * gsk_arithmetic_node_new:
 * @first: The first node to be composited
 * @second: The second node to be composited
 * @factors: the 4 factors to use
 *
 * Creates a `GskRenderNode` that will composite the
 * @first and @second nodes arithmetically.
 *
 * Returns: (transfer full) (type GskArithmeticNode): A new `GskRenderNode`
 */
GskRenderNode *
gsk_arithmetic_node_new (GskRenderNode *first,
                         GskRenderNode *second,
                         float          factors[4])
{
  GskArithmeticNode *self;
  GskRenderNode *node;

  g_return_val_if_fail (GSK_IS_RENDER_NODE (first), NULL);
  g_return_val_if_fail (GSK_IS_RENDER_NODE (second), NULL);

  self = gsk_render_node_alloc (GSK_TYPE_ARITHMETIC_NODE);
  node = (GskRenderNode *) self;

  self->first = gsk_render_node_ref (first);
  self->second = gsk_render_node_ref (second);
  memcpy (self->factors, factors, sizeof (float) * 4);

  graphene_rect_union (&first->bounds, &second->bounds, &node->bounds);

  node->preferred_depth = gdk_memory_depth_merge (gsk_render_node_get_preferred_depth (first),
                                                  gsk_render_node_get_preferred_depth (second));
  node->is_hdr = gsk_render_node_is_hdr (first) ||
                 gsk_render_node_is_hdr (second);

  return node;
}

/**
 * gsk_arithmetic_node_get_first_child:
 * @node: (type GskArithmeticNode): a `GskRenderNode`
 *
 * Retrieves the first `GskRenderNode` child of the @node.
 *
 * Returns: (transfer none): the first child node
 */
GskRenderNode *
gsk_arithmetic_node_get_first_child (const GskRenderNode *node)
{
  const GskArithmeticNode *self = (const GskArithmeticNode *) node;

  return self->first;
}

/**
 * gsk_arithmetic_node_get_second_child:
 * @node: (type GskArithmeticNode): a `GskRenderNode`
 *
 * Retrieves the second `GskRenderNode` child of the @node.
 *
 * Returns: (transfer none): the second child node
 */
GskRenderNode *
gsk_arithmetic_node_get_second_child (const GskRenderNode *node)
{
  const GskArithmeticNode *self = (const GskArithmeticNode *) node;

  return self->second;
}

/**
 * gsk_arithmetic_node_get_factors:
 * @node: (type GskArithmeticNode): a `GskRenderNode`
 * @factors: return location for the 4 factors
 *
 * Retrieves the factors used by @node.
 */
void
gsk_arithmetic_node_get_factors (const GskRenderNode *node,
                                 float                factors[4])
{
  const GskArithmeticNode *self = (const GskArithmeticNode *) node;

  memcpy (factors, self->factors, sizeof (float) * 4);
}
