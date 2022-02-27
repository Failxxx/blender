
// Old debug, passing all the matrices to the shader
//uniform mat4 u_m4ModelMatrix;
//uniform mat4 u_m4ViewMatrix;
//uniform mat4 u_m4ProjectionMatrix;
//
//in vec3 v_in_f3Position;
//in vec4 v_in_f4Color;
//
//out vec4 v_out_f4Color;
//
//void main()
//{
//  gl_Position = u_m4ProjectionMatrix * u_m4ViewMatrix * u_m4ModelMatrix * vec4(v_in_f3Position, 1.0);
//  v_out_f4Color = v_in_f4Color;
//}

// Render simple geometry with 3D positions and 2D UVs
uniform mat4 u_m4ModelViewProjectionMatrix;

in vec3 v_in_f3Position;
in vec2 v_in_f2UV;

out vec2 v_out_f2UV;

void main()
{
  v_out_f2UV = v_in_f2UV;
  gl_Position = u_m4ModelViewProjectionMatrix * vec4(v_in_f3Position, 1.0);
}
