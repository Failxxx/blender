
uniform sampler2D u_s2InputTexture; // Current trails texture
uniform sampler2D u_s2Points; // Agent positions
uniform vec2 u_f2Resolution;
uniform float u_fDecay;

in vec2 v_out_f2UV;

out vec4 fragColor;

void main(){
  // Resolution factor
  vec2 res = 1.0f / u_f2Resolution;
  // The position of the current agent on the final texture is stored in red channel of the points texture
  float pos = texture2D(u_s2Points, v_out_f2UV).r;

  // Accumulator
  float col = 0.0f;
  // Blur box size
  const float dim = 1.0f;
  // Weight
  float weight = 1.0f / pow(2.0f * dim + 1.0f, 2.0f);

  // Compute diffuse
  for(float i = -dim; i <= dim; i++) {
    for(float j = -dim; j <= dim; j++) {
      vec3 val = texture2D(u_s2InputTexture, fract(v_out_f2UV + res * vec2(i, j))).rgb;
      col += val.r * weight + val.g * weight * 0.5f;
    }
  }

  // Compute decay
  vec4 fin = vec4(pos * u_fDecay, col * u_fDecay, 0.5f, 1.0f);
  // Send final color
  fragColor = clamp(fin, 0.01f, 1.0f);

  //if (length(vec2(0.5f, 0.5f) - v_out_f2UV) >= 0.2f) {
  //  fragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
  //}
  //else {
  //  fragColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
  //}
}
