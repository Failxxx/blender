
uniform sampler2D u_s2AgentsData;
uniform sampler2D u_s2TrailsData;

uniform vec2 u_f2Resolution;
uniform float u_fTime;
uniform float u_fSA;
uniform float u_fRA;
uniform float u_fSO;
uniform float u_fSS;

in vec2 v_out_f2UV;

// precise location because we write in the render target 0 of the frame buffer
layout(location = 0) out vec4 f4FragColor;  

const float PI = 3.14159265358979323846264;  // PI
const float PI2 = PI * 2.0;
const float RAD = 1.0 / PI;
const float PHI = 1.61803398874989484820459 * 0.1;     // Golden Ratio
const float SQ2 = 1.41421356237309504880169 * 1000.0;  // Square Root of Two

float rand(vec2 coordinate)
{
  return fract(tan(distance(coordinate * (u_fTime + PHI), vec2(PHI, PI * 0.1))) * SQ2);
}

/* Return the position of the current agent in the agents texture */
float getDataValue(vec2 uv)
{
  return texture(u_s2TrailsData, fract(uv)).r;
}

/* Return the trail value of the given pixel */
float getTrailValue(vec2 uv)
{
  return texture(u_s2TrailsData, fract(uv)).g;
}

void main()
{
  // Converts degree to radians (should be done on the CPU)
  float SA = u_fSA * RAD;
  float RA = u_fRA * RAD;

  // Downscales the parameters (should be done on the CPU)
  vec2 res = 1.0 / u_f2Resolution;  // Data trail scale
  vec2 SO = u_fSO * res;
  vec2 SS = u_fSS * res;

  // Where to sample in the data trail texture to get the agent's world position
  vec4 src = texture(u_s2AgentsData, v_out_f2UV);
  vec4 val = src;

  // Agent's heading
  float angle = val.z * PI2;

  // Compute the sensors positions
  vec2 uvFL = val.xy + vec2(cos(angle - SA), sin(angle - SA)) * SO;
  vec2 uvF = val.xy + vec2(cos(angle), sin(angle)) * SO;
  vec2 uvFR = val.xy + vec2(cos(angle + SA), sin(angle + SA)) * SO;

  // Get the values unders the sensors
  float FL = getTrailValue(uvFL);
  float F = getTrailValue(uvF);
  float FR = getTrailValue(uvFR);

  // Original implement not very parallel friendly
  // TODO remove the conditions
  if (F > FL && F > FR) {
  }
  else if (F < FL && F < FR) {
    if (rand(val.xy) > 0.5) {
      angle += RA;
    }
    else {
      angle -= RA;
    }
  }
  else if (FL < FR) {
    angle += RA;
  }
  else if (FL > FR) {
    angle -= RA;
  }

  vec2 offset = vec2(cos(angle), sin(angle)) * SS;
  val.xy += offset;

  //// Condition from the paper: move only if the destination is free
  // if(getDataValue(val.xy) == 1.0){
  //   val.xy = src.xy;
  //   angle = rand(val.xy + u_fTime) * PI2;
  // }

  // Warps the coordinates so they remains in the [0-1] interval
  val.xy = fract(val.xy);
  // Converts the angle back to [0-1]
  val.z = (angle / PI2);

  f4FragColor = val;
}
