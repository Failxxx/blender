
// Vertices with 3D positions and 4D colors
in vec4 v_out_f4Color;

uniform sampler2D u_s2Texture;

in vec2 v_out_f2UV;

out vec4 f4FragColor;

void main()
{
  f4FragColor = texture(u_s2Texture, v_out_f2UV);
  //f4FragColor = vec4(texture(u_s2Texture, v_out_f2UV).b, 0.0, 0.0, 1.0);
}

// Textures
//uniform sampler2D u_s2RenderedTexture;
//
//in vec2 v_out_f2UV;
//
//out vec4 fragColor;
//
//void main()
//{
//  fragColor = texture(u_s2RenderedTexture, v_out_f2UV);
//  //if (length(vec2(0.5f, 0.5f) - v_out_f2UV) >= 0.2f) {
//  //  fragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
//  //}
//  //else {
//  //  fragColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
//  //}
//}
