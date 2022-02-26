
uniform mat4 u_m4ModelViewProjectionMatrix;

in vec3 v_in_f3Position;
in vec2 v_in_f2Uv;

out vec2 v_out_f2Uv;

void main(){
  v_out_f2Uv = v_in_f2Uv;
  gl_Position = u_m4ModelViewProjectionMatrix * vec4(v_in_f3Position, 1.0);
}