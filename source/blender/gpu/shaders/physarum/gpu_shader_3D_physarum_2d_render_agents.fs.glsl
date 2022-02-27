
out vec4 fragColor;

void main(){
  float d = 1.0f - length(0.5f - gl_PointCoord.xy);
  fragColor = vec4(d, 0.0f, 0.0f, 1.0f);
}
