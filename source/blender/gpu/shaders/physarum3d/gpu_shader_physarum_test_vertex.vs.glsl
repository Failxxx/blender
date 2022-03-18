// uniform mat4 u_m4ModelMatrix;
// uniform mat4 u_m4ViewMatrix;
// uniform mat4 u_m4ProjectionMatrix;

in vec3 v_in_f3Position;

out vec4 v_out_f4Color;

void main()
{
  v_out_f4Color = vec4(v_in_f3Position.x, v_in_f3Position.y, v_in_f3Position.z, 1.0f);
}
