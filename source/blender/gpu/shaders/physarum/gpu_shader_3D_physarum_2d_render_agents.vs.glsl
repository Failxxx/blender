
uniform mat4 u_m4ModelViewProjectionMatrix;
uniform sampler2D u_s2InputTexture;

in vec3 v_in_f3Position; // Unused
in vec2 v_in_f2UV;

void main(){
  // The position of the current agent is defined in the red and green channels of the input texture
  vec2 uv = texture2D(u_s2InputTexture, v_in_f2UV).xy;
  gl_Position = u_m4ModelViewProjectionMatrix * vec4(uv * 2.0f - 1.0f, 0.0f, 1.0f);
  gl_PointSize = 1.0f;
}
