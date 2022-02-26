
uniform sampler2D u_s2InputTexture;

in vec2 v_in_f2UV;

void main(){
  vec2 uv = texture2D(u_s2InputTexture, v_in_f2UV).xy;
  gl_Position = vec4(uv * 2.0 - 1., 0., 1.);
  gl_PointSize = 1.;
}
