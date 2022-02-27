
uniform sampler2D u_s2Data;

in vec2 v_out_f2UV;

out vec4 fragColor;

void main(){
  vec4 f4Source = texture2D(u_s2Data, v_out_f2UV);
  fragColor = vec4(f4Source.ggg, 1.0f);
}
