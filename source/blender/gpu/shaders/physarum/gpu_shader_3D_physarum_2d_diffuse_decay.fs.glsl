
uniform sampler2D u_s2TrailsData;
uniform sampler2D u_s2Agents;
uniform vec2 u_f2Resolution;
uniform float u_fDecay;

in vec2 v_out_f2UV;

// precise location because we write in the render target 0 of the frame buffer
layout(location = 0) out vec4 f4FragColor;  

void main()
{
  // Resolution factor
  vec2 res = 1.0 / u_f2Resolution;
  // The position of the current agent on the final texture is stored in red channel of the points
  // texture (rendered agents, as red dots)
  float pos = texture(u_s2Agents, v_out_f2UV).r;

  // Accumulator
  float col = 0.0;
  // Blur box size
  const float dim = 1.0;
  // Weight
  float weight = 1.0 / pow(2.0 * dim + 1.0, 2);

  // Compute diffuse
  for (float i = -dim; i <= dim; i++) {
    for (float j = -dim; j <= dim; j++) {
      vec3 val = texture(u_s2TrailsData, fract(v_out_f2UV + res * vec2(i, j))).rgb;
      col += val.r * weight + val.g * weight * 0.5;
    }
  }

  // Compute decay
  vec4 fin = vec4(pos * u_fDecay, col * u_fDecay, 0.5, 1.0);

  f4FragColor = clamp(fin, 0.01f, 1.0);
}
