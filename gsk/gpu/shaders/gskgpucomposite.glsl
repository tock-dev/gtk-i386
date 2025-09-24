#define GSK_N_TEXTURES 2

#include "common.glsl"
#include "composite.glsl"

PASS(0) vec2 _pos;
PASS_FLAT(1) Rect _source_rect;
PASS_FLAT(2) Rect _dest_rect;
PASS(3) vec2 _source_coord;
PASS(4) vec2 _dest_coord;
PASS_FLAT(5) float _opacity;


#ifdef GSK_VERTEX_SHADER

IN(0) vec4 in_rect;
IN(1) vec4 in_source_rect;
IN(2) vec4 in_dest_rect;
IN(3) float in_opacity;

void
run (out vec2 pos)
{
  Rect r = rect_from_gsk (in_rect);

  pos = rect_get_position (r);

  _pos = pos;
  _opacity = in_opacity;

  Rect source_rect = rect_from_gsk (in_source_rect);
  _source_rect = source_rect;
  _source_coord = rect_get_coord (source_rect, pos);

  Rect dest_rect = rect_from_gsk (in_dest_rect);
  _dest_rect = dest_rect;
  _dest_coord = rect_get_coord (dest_rect, pos);
}

#endif



#ifdef GSK_FRAGMENT_SHADER

void
run (out vec4 color,
     out vec2 position)
{
  vec4 source_color = texture (GSK_TEXTURE0, _source_coord);
  source_color = output_color_alpha (source_color, rect_coverage (_source_rect, _pos));

  vec4 dest_color = texture (GSK_TEXTURE1, _dest_coord);
  dest_color = output_color_alpha (dest_color, rect_coverage (_dest_rect, _pos));

  color = composite_op (source_color, dest_color, GSK_VARIATION);
  color = output_color_alpha (color, _opacity);

  position = _pos;
}

#endif
