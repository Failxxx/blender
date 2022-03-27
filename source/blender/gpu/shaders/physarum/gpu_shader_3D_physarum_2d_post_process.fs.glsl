
uniform sampler2D u_s2TrailsData;
uniform vec4 u_f4ParticlesColor;

in vec2 v_out_f2UV;

out vec4 fragColor;

void main()
{
  vec4 f4TrailValue = texture(u_s2TrailsData, v_out_f2UV);
  float alpha = (f4TrailValue.g > 0.0) ? f4TrailValue.g * u_f4ParticlesColor.a :
                                         u_f4ParticlesColor.a;
  fragColor = vec4(f4TrailValue.g * u_f4ParticlesColor.rgb, alpha);
}
