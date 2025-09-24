#define GSK_N_TEXTURES 2

#include "common.glsl"
#include "color.glsl"

PASS(0) vec2 _pos;
PASS_FLAT(1) Rect _child_rect;
PASS(2) vec2 _child_coord;
PASS(3) vec2 _map_coord;
PASS_FLAT(4) float _opacity;
PASS_FLAT(5) float _scale;


#ifdef GSK_VERTEX_SHADER

IN(0) vec4 in_rect;
IN(1) vec4 in_child_rect;
IN(2) vec4 in_map_rect;
IN(3) float in_opacity;
IN(4) float in_scale;

void
run (out vec2 pos)
{
  Rect r = rect_from_gsk (in_rect);

  pos = rect_get_position (r);

  _pos = pos;
  _opacity = in_opacity;
  _scale = in_scale;

  Rect child_rect = rect_from_gsk (in_child_rect);
  _child_rect = child_rect;
  _child_coord = rect_get_coord (child_rect, pos);

  Rect map_rect = rect_from_gsk (in_map_rect);
  _map_coord = rect_get_coord (map_rect, pos);
}

#endif



#ifdef GSK_FRAGMENT_SHADER

#define CHANNELS(x,y) ((x << 2) | y)
#define R 0u
#define G 1u
#define B 2u
#define A 3u

vec2
get_displacement(vec4 map_pixel, uint channels)
{
  if (channels == CHANNELS(R,R))
    return map_pixel.rr;
  else if (channels == CHANNELS(R,G))
    return map_pixel.rg;
  else if (channels == CHANNELS(R,B))
    return map_pixel.rb;
  else if (channels == CHANNELS(R,A))
    return map_pixel.ra;
  else if (channels == CHANNELS(G,R))
    return map_pixel.gr;
  else if (channels == CHANNELS(G,G))
    return map_pixel.gg;
  else if (channels == CHANNELS(G,B))
    return map_pixel.gb;
  else if (channels == CHANNELS(G,A))
    return map_pixel.ga;
  else if (channels == CHANNELS(B,R))
    return map_pixel.br;
  else if (channels == CHANNELS(B,G))
    return map_pixel.bg;
  else if (channels == CHANNELS(B,B))
    return map_pixel.bb;
  else if (channels == CHANNELS(B,A))
    return map_pixel.ba;
  else if (channels == CHANNELS(A,R))
    return map_pixel.ar;
  else if (channels == CHANNELS(A,G))
    return map_pixel.ag;
  else if (channels == CHANNELS(A,B))
    return map_pixel.ab;
  else if (channels == CHANNELS(A,A))
    return map_pixel.aa;
  else
    return vec2(0.5,0.5);
}

#undef CHANNELS
#undef R
#undef G
#undef B
#undef A

void
run (out vec4 color,
     out vec2 position)
{
  vec4 map_pixel = texture (GSK_TEXTURE1, _map_coord);

  color_unpremultiply (map_pixel);

  vec2 d = get_displacement (map_pixel, GSK_VARIATION);

  vec2 coord = rect_get_coord (_child_rect, _pos + (d - vec2(0.5, 0.5)) * _scale);

  vec4 child_color = texture (GSK_TEXTURE0, coord);

  color = output_color_alpha (child_color, rect_coverage (_child_rect, _pos) * _opacity);

  position = _pos;
}

#endif
