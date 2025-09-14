#pragma once

#include "gskgpushaderopprivate.h"

#include <graphene.h>

G_BEGIN_DECLS

void                    gsk_gpu_displacement_op                         (GskGpuFrame                    *frame,
                                                                         GskGpuShaderClip                clip,
                                                                         const graphene_rect_t          *rect,
                                                                         const graphene_point_t         *offset,
                                                                         float                           opacity,
                                                                         float                           scale,
                                                                         guint                           x_channel,
                                                                         guint                           y_channel,
                                                                         const GskGpuShaderImage        *child,
                                                                         const GskGpuShaderImage        *map);


G_END_DECLS

