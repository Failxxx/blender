
uniform float alpha = 0.6;
uniform vec2 wireFadeDepth = vec2(0.0, 0.0);

in vec4 finalColor;
flat in int inverted;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 lineOutput;

void main()
{
  /* Manual back-face culling. Not ideal for performance
   * but needed for view clarity in X-ray mode and support
   * for inverted bone matrices. */
  if ((inverted == 1) == gl_FrontFacing) {
    discard;
  }
  float z_alpha = wire_depth_alpha(gl_FragCoord.z, wireFadeDepth);
  fragColor = vec4(finalColor.rgb, alpha * z_alpha);
  lineOutput = vec4(0.0);
}
