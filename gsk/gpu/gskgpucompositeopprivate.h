#pragma once

#include "gskgpushaderopprivate.h"

#include <graphene.h>

G_BEGIN_DECLS

void                    gsk_gpu_composite_op                            (GskGpuFrame                    *frame,
                                                                         GskGpuShaderClip                clip,
                                                                         const graphene_rect_t          *rect,
                                                                         const graphene_point_t         *offset,
                                                                         float                           opacity,
                                                                         GskCompositeOperator            op,
                                                                         const GskGpuShaderImage        *source,
                                                                         const GskGpuShaderImage        *dest);


G_END_DECLS

