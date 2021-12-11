
uniform float alpha = 1.0;
uniform vec2 wireFadeDepth = vec2(0.0, 0.0);

flat in vec4 finalColor;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 lineOutput;

void main()
{
  float z_alpha = wire_depth_alpha(gl_FragCoord.z, wireFadeDepth);
  fragColor = vec4(finalColor.rgb, finalColor.a * alpha * z_alpha);
  lineOutput = vec4(0.0);
}
