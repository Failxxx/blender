
// precise location because we write in the render target 0 of the frame buffer
layout(location = 0) out vec4 f4FragColor;

void main(){
  float fDistance = 1.0 - length(0.5 - gl_PointCoord.xy);
  f4FragColor = vec4(fDistance, 0.0, 0.0, 1.0);
}
