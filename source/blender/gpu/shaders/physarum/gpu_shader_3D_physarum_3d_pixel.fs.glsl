
in vec3 v_out_f3Texcoord;

out vec4 FragColor;

uniform sampler3D u_s3TrailsData;

const float N = 30.0f;
const float GRID_SIZE = 5.0f;
const float GRID_OPACITY = 0.5f;

// https://www.iquilezles.org/www/articles/filterableprocedurals/filterableprocedurals.htm
float grid(vec3 p, vec3 dpdx, vec3 dpdy)
{
  vec3 w = max(abs(dpdx), abs(dpdy)) + 0.01f;
  vec3 a = p + 0.5f * w;
  vec3 b = p - 0.5f * w;
  vec3 i = (floor(a) + min(fract(a) * N, 1.0) - floor(b) - min(fract(b) * N, 1.0)) / (N * w);
  return i.x * i.y * i.z;
}

void main()
{
  float v = texture(u_s3TrailsData, v_out_f3Texcoord.xyz).r / 5.0;
  vec3 p = v_out_f3Texcoord.xyz * (GRID_SIZE + 1.0f / N);
  vec3 dx = dFdx(p);
  vec3 dy = dFdy(p);
  float grid_color = grid(p, dx, dy) * GRID_OPACITY;
  v = max(grid_color, v);

  FragColor = vec4(1.0f, 1.0f, 1.0f, v);
}
