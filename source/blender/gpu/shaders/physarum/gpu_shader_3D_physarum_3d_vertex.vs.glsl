
layout(location = 0) in vec4 v_in_f4Position;
layout(location = 1) in vec3 v_in_f3Texcoord;

out vec4 v_out_f4Position;
out vec3 v_out_f3Texcoord;

uniform mat4 u_m4Projection_matrix;
uniform mat4 u_m4View_matrix;
uniform mat4 u_m4Model_matrix;
uniform int u_iTexcoord_map;

void main()
{
  if (u_iTexcoord_map == 0) {
    v_out_f3Texcoord = v_in_f3Texcoord;
  }
  else if (u_iTexcoord_map == 1) {
    v_out_f3Texcoord = vec3(v_in_f3Texcoord.x, v_in_f3Texcoord.z, 1.0 - v_in_f3Texcoord.y);
  }
  else if (u_iTexcoord_map == 2) {
    v_out_f3Texcoord = vec3(1.0 - v_in_f3Texcoord.z, v_in_f3Texcoord.y, v_in_f3Texcoord.x);
  }

  v_out_f4Position = v_in_f4Position;
  gl_Position = u_m4Projection_matrix * u_m4View_matrix * u_m4Model_matrix * v_in_f4Position;
}
