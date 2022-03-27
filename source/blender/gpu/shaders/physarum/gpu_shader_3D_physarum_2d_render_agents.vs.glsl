
uniform mat4 u_m4ProjectionMatrix;
uniform sampler2D u_s2AgentsData;

layout(location = 0) in vec3 v_in_f3Position; // Unused
layout(location = 1) in vec2 v_in_f2UV;

void main()
{
  vec2 f2UV = texture(u_s2AgentsData, v_in_f2UV).xy; // (x,y) positions of agents are stored in the (r,g) channels
  vec2 f2UVRemapped = f2UV * vec2(2.0) - vec2(1.0);
  gl_Position = u_m4ProjectionMatrix * vec4(f2UVRemapped, 0.0, 1.0);
  gl_PointSize = 1.0;
}
