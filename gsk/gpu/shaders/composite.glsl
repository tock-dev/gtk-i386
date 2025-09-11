#ifndef _COMPOSITE_
#define _COMPOSITE_

vec4
composite (vec4 Cs, vec4 Cb, float Fa, float Fb)
{
  float ao = Cs.a * Fa + Cb.a * Fb;
  vec3 co = Fa * Cs.rgb + Fb * Cb.rgb;

  return clamp(vec4(co, ao), 0.0, 1.0);
}

vec4
clear (vec4 Cs, vec4 Cb)
{
  return composite (Cs, Cb, 0.0, 0.0);
}

vec4
copy (vec4 Cs, vec4 Cb)
{
  return composite (Cs, Cb, 1.0, 0.0);
}

vec4
over (vec4 Cs, vec4 Cb)
{
  return composite (Cs, Cb, 1.0, 1.0 - Cs.a);
}

vec4
in_ (vec4 Cs, vec4 Cb)
{
  return composite (Cs, Cb, Cb.a, 0.0);
}

vec4
out_ (vec4 Cs, vec4 Cb)
{
  return composite (Cs, Cb, 1.0 - Cb.a, 0.0);
}

vec4
atop (vec4 Cs, vec4 Cb)
{
  return composite (Cs, Cb, Cb.a, 1.0 - Cs.a);
}

vec4
xor (vec4 Cs, vec4 Cb)
{
  return composite (Cs, Cb, 1.0 - Cb.a, 1.0 - Cs.a);
}

vec4
lighter (vec4 Cs, vec4 Cb)
{
  return composite (Cs, Cb, 1.0, 1.0);
}

vec4
composite_op (vec4 source_color,
              vec4 dest_color,
              uint u_op)
{
  vec4 result;

  if (u_op == 0u)
    result = clear(source_color, dest_color);
  else if (u_op == 1u)
    result = copy(source_color, dest_color);
  else if (u_op == 2u)
    result = over(source_color, dest_color);
  else if (u_op == 3u)
    result = in_(source_color, dest_color);
  else if (u_op == 4u)
    result = out_(source_color, dest_color);
  else if (u_op == 5u)
    result = atop(source_color, dest_color);
  else if (u_op == 6u)
    result = xor(source_color, dest_color);
  else if (u_op == 7u)
    result = lighter(source_color, dest_color);
  else
    result = vec4 (1.0, 0.0, 0.8, 1.0);

  return result;
}

#endif
