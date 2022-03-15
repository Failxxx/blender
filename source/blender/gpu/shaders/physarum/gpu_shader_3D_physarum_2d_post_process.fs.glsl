
uniform sampler2D u_s2TrailsData;

in vec2 v_out_f2UV;

out vec4 fragColor;

void main()
{
  vec4 f4Source = texture(u_s2TrailsData, v_out_f2UV);
  fragColor = vec4(f4Source.ggg, 1.0);
}
