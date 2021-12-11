
uniform float alpha = 1.0;
uniform vec2 wireFadeDepth = vec2(0.0, 0.0);

noperspective in float colorFac;
flat in vec4 finalWireColor;
flat in vec4 finalInnerColor;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 lineOutput;

void main()
{
  float fac = smoothstep(1.0, 0.2, colorFac);
  float z_alpha = wire_depth_alpha(gl_FragCoord.z, wireFadeDepth);
  fragColor.rgb = mix(finalInnerColor.rgb, finalWireColor.rgb, fac);
  fragColor.a = alpha * z_alpha;
  lineOutput = vec4(0.0);
}
