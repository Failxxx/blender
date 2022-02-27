
uniform sampler2D u_s2InputTexture;
uniform sampler2D u_s2Points;
uniform vec2 u_f2Resolution;
uniform float u_fDecay;

in vec2 v_out_f2_UV;

void main(){
  vec2 res = 1. / u_f2Resolution;
  float pos = texture2D(u_s2Points, v_out_f2_UV).r;
    
  // Accumulator
  float col = 0.;
  // Blur box size
  const float dim = 1.;
  // Weight
  float weight = 1. / pow(2. * dim + 1., 2.);

  for(float i = -dim; i <= dim; i++) {
    for(float j = -dim; j <= dim; j++) {
      vec3 val = texture2D(u_s2InputTexture, fract(v_out_f2_UV + res * vec2(i, j))).rgb;
      col += val.r * weight + val.g * weight * .5;
    }
  }

  vec4 fin = vec4(pos * u_fDecay, col * u_fDecay, .5, 1.);
  gl_FragColor = clamp(fin, 0.01, 1.);
}
