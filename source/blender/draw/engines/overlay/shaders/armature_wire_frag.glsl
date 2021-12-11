
uniform float alpha = 1.0;
uniform vec2 wireFadeDepth = vec2(0.0, 0.0);

flat in vec4 finalColor;
flat in vec2 edgeStart;
noperspective in vec2 edgePos;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 lineOutput;

void main()
{
  float z_alpha = wire_depth_alpha(gl_FragCoord.z, wireFadeDepth);
  lineOutput = pack_line_data(gl_FragCoord.xy, edgeStart, edgePos);
  fragColor = vec4(finalColor.rgb, finalColor.a * alpha * z_alpha);
}
