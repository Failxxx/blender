
out vec4 fragColor;

void main(){
  float d = 1. - length(.5 - gl_PointCoord.xy);
  fragColor = vec4(d, 0., 0., 1.);
}
