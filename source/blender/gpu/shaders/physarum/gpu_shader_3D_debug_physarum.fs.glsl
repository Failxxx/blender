
// Vertices with 3D positions and 4D colors
//in vec4 v_out_f4Color;
//
//out vec4 fragColor;
//
//void main()
//{
//  fragColor = v_out_f4Color;
//}

// Textures
uniform sampler2D u_s2RenderedTexture;

in vec2 v_out_f2UV;

out vec4 fragColor;

void main()
{
  fragColor = texture(u_s2RenderedTexture, v_out_f2UV);
}
