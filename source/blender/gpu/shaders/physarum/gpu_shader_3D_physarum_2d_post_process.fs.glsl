
uniform sampler2D u_s2Data;

in vec2 v_out_f2UV;

void main(){
  vec4 f4Source = texture2D(u_s2Data, v_out_f2UV);
  gl_FragColor = vec4(f4Source.ggg, 1.0f);
}
