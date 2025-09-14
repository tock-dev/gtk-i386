#include "config.h"

#include "gskgpudisplacementopprivate.h"

#include "gskenumtypes.h"
#include "gskgpuframeprivate.h"
#include "gskgpuprintprivate.h"
#include "gskrectprivate.h"

#include "gpu/shaders/gskgpudisplacementinstance.h"

typedef struct _GskGpuDisplacementOp GskGpuDisplacementOp;

struct _GskGpuDisplacementOp
{
  GskGpuShaderOp op;
};

static void
gsk_gpu_displacement_op_print_instance (GskGpuShaderOp *shader,
                                        gpointer        instance_,
                                        GString        *string)
{
  GskGpuDisplacementInstance *instance = (GskGpuDisplacementInstance *) instance_;

  gsk_gpu_print_rect (string, instance->rect);
  gsk_gpu_print_image (string, shader->images[0]);
  gsk_gpu_print_image (string, shader->images[1]);
}

static const GskGpuShaderOpClass GSK_GPU_DISPLACEMENT_OP_CLASS = {
  {
    GSK_GPU_OP_SIZE (GskGpuDisplacementOp),
    GSK_GPU_STAGE_SHADER,
    gsk_gpu_shader_op_finish,
    gsk_gpu_shader_op_print,
#ifdef GDK_RENDERING_VULKAN
    gsk_gpu_shader_op_vk_command,
#endif
    gsk_gpu_shader_op_gl_command
  },
  "gskgpudisplacement",
  gsk_gpu_displacement_n_textures,
  sizeof (GskGpuDisplacementInstance),
#ifdef GDK_RENDERING_VULKAN
  &gsk_gpu_displacement_info,
#endif
  gsk_gpu_displacement_op_print_instance,
  gsk_gpu_displacement_setup_attrib_locations,
  gsk_gpu_displacement_setup_vao
};

void
gsk_gpu_displacement_op (GskGpuFrame             *frame,
                         GskGpuShaderClip         clip,
                         const graphene_rect_t   *rect,
                         const graphene_point_t  *offset,
                         float                    opacity,
                         float                    scale,
                         guint                    x_channel,
                         guint                    y_channel,
                         const GskGpuShaderImage *child,
                         const GskGpuShaderImage *map)
{
  GskGpuDisplacementInstance *instance;

  gsk_gpu_shader_op_alloc (frame,
                           &GSK_GPU_DISPLACEMENT_OP_CLASS,
                           gsk_gpu_color_states_create_equal (TRUE, TRUE),
                           (x_channel << 2) | y_channel,
                           clip,
                           (GskGpuImage *[2]) { child->image, map->image },
                           (GskGpuSampler[2]) { child->sampler, map->sampler },
                           &instance);

  gsk_gpu_rect_to_float (rect, offset, instance->rect);
  gsk_gpu_rect_to_float (child->bounds, offset, instance->child_rect);
  gsk_gpu_rect_to_float (map->bounds, offset, instance->map_rect);
  instance->opacity = opacity;
  instance->scale = scale;
}
