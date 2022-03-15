
// Vertices with 3D positions and 4D colors
//in vec4 v_out_f4Color;
//
//out vec4 fragColor;
//
//void main()
//{
//  fragColor = v_out_f4Color;
//}

uniform sampler2D u_s2Texture;

in vec2 v_out_f2UV;

out vec4 f4FragColor;

void main()
{
  f4FragColor = texture(u_s2Texture, v_out_f2UV);
  //f4FragColor = vec4(texture(u_s2Texture, v_out_f2UV).b, 0.0, 0.0, 1.0);
}
