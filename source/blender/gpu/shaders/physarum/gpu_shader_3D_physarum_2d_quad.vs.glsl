
uniform mat4 u_m4ProjectionMatrix;

in vec3 v_in_f3Position;
in vec2 v_in_f2UV;

out vec2 v_out_f2UV;

void main(){
  v_out_f2UV = v_in_f2UV;
  gl_Position = u_m4ProjectionMatrix * vec4(v_in_f3Position, 1.0);
}
