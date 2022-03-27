
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_shader_storage_buffer_object : enable

layout(local_size_x = 10, local_size_y = 10, local_size_z = 10) in;
layout(rgba32f, binding = 0) uniform image3D u_in_s3TrailTexture;
//layout(rgba32f, binding = 1) uniform image3D u_out_s3TextureOCC;

uniform float u_fSense_spread;
uniform float u_fSense_distance;
uniform float u_fTurn_angle;
uniform float u_fMove_distance;
uniform float u_fDeposit_value;
uniform float u_fDecay_factor;
//uniform float u_fCollision;
uniform float u_fCenter_attraction;
uniform float u_fMove_sense_coef;
uniform float u_fMove_sense_offset;

uniform int u_iWorld_width;
uniform int u_iWorld_height;
uniform int u_iWorld_depth;

layout(std430, binding = 2) buffer ssbo_particles_x
{
  float particles_x[];
};

layout(std430, binding = 3) buffer ssbo_particles_y
{
  float particles_y[];
};

layout(std430, binding = 4) buffer ssbo_particles_z
{
  float particles_z[];
};

layout(std430, binding = 5) buffer ssbo_particles_phi
{
  float particles_phi[];
};

layout(std430, binding = 6) buffer ssbo_particles_theta
{
  float particles_theta[];
};

uint wang_hash(uint seed)
{
  seed = (seed ^ 61) ^ (seed >> 16);
  seed *= 9;
  seed = seed ^ (seed >> 4);
  seed *= 0x27d4eb2d;
  seed = seed ^ (seed >> 15);
  return seed;
}

vec3 rotate(vec3 v, vec3 a, float angle)
{
  vec3 result = cos(angle) * v + sin(angle) * (cross(a, v)) + dot(a, v) * (1 - cos(angle)) * a;
  return result;
}

float random(int seed)
{
  return float(wang_hash(seed) % 1000) / 1000.0f;
}

void main()
{
  uint idx = gl_LocalInvocationIndex +
             1000 * (gl_WorkGroupID.x + gl_WorkGroupID.y * 10 + gl_WorkGroupID.z * 100);

  const float PI = 3.1415926535897932384626433832795;
  const float PI_2 = 1.57079632679489661923;

  // Fetch current particle state
  float x = particles_x[idx];
  float y = particles_y[idx];
  float z = particles_z[idx];
  float t = particles_theta[idx];
  float ph = particles_phi[idx];

  // Get vector which points in the current particle's direction
  vec3 center_axis = vec3(sin(t) * cos(ph), cos(t), sin(t) * sin(ph));

  // Get base vector which points away from the current particle's direction and will be used
  // to sample environment in other directions
  float sense_theta = t - u_fSense_spread;
  vec3 off_center_base_dir = vec3(
      sin(sense_theta) * cos(ph), cos(sense_theta), sin(sense_theta) * sin(ph));

  // Sample environment straight ahead
  ivec3 p = ivec3(x, y, z);
  vec3 center_sense_pos = center_axis * u_fSense_distance;

  // Sample environment away from the center axis and store max values.
  // float max_value = u_in_s3TrailTexture[ivec3(center_sense_pos) + p];
  float max_value = imageLoad(u_in_s3TrailTexture, ivec3(center_sense_pos) + p).r;

  int max_value_count = 1;
#define SAMPLE_POINTS 8
  int max_values[SAMPLE_POINTS + 1];  // = {0, 0, 0, 0, 0, 0, 0, 0, 0};
  max_values[0] = 0;
  float start_angle = random(int(idx) * 42) * 3.1415 - PI_2;

  for (int i = 1; i < SAMPLE_POINTS + 1; ++i) {

    float angle = start_angle + PI * 2.0 / float(SAMPLE_POINTS) * i;
    vec3 sense_position = rotate(off_center_base_dir, center_axis, angle) * u_fSense_distance;
    // float stuff = u_in_s3TrailTexture[ivec3(sense_position) + p];
    float stuff = imageLoad(u_in_s3TrailTexture, ivec3(sense_position) + p).r;
    // WHAT IS STUFF BRO ?

    if (stuff > max_value) {
      max_value_count = 1;
      max_value = stuff;
      max_values[0] = i;
    }
    else if (stuff == max_value) {
      max_values[max_value_count++] = i;
    }
  }

  // Pick direction with max value sampled.
  uint random_max_value_direction = wang_hash(idx * uint(x) * uint(y) * uint(z)) % max_value_count;
  int direction = max_values[random_max_value_direction];
  float theta_turn = t - u_fTurn_angle;
  vec3 off_center_base_dir_turn = vec3(
      sin(theta_turn) * cos(ph), cos(theta_turn), sin(theta_turn) * sin(ph));

  if (direction > 0) {
    vec3 best_direction = rotate(off_center_base_dir_turn,
                                 center_axis,
                                 direction * PI * 2.0 / float(SAMPLE_POINTS) + start_angle);

    // ph = atan2(best_direction.z, best_direction.x); HLSL atan2 => GLSL atant = diff
    ph = atan(best_direction.x, best_direction.z);
    t = acos(best_direction.y / length(best_direction));
  }

  // Compute rotation applied by force pointing to the center of environment.
  vec3 to_center = vec3(
      u_iWorld_width / 2.0 - x, u_iWorld_height / 2.0 - y, u_iWorld_depth / 2.0 - z);
  float d_center = length(to_center);
  float d_c_turn = clamp((d_center - 50.0) / 150.0, 0, 1) * u_fCenter_attraction;
  vec3 dir = vec3(sin(t) * cos(ph), cos(t), sin(t) * sin(ph));

  vec3 center_dir = normalize(to_center);
  // vec3 center_angle = acos(dot(dir, center_dir));
  vec3 center_angle = vec3(acos(dot(dir, center_dir)));
  float st = 0.1 * d_c_turn;

  dir = sin((1 - st) * center_angle) / sin(center_angle) * dir +
        sin(st * center_angle) / sin(center_angle) * center_dir;

  if (length(dir) > 0.0 && (dir.z != 0.0 || dir.x != 0.0)) {
    t = acos(dir.y / length(dir));
    // ph = atan2(dir.z, dir.x);
    ph = atan(dir.x, dir.z);
  }

  // Make a step
  vec3 dp = vec3(sin(t) * cos(ph), cos(t), sin(t) * sin(ph)) * u_fMove_distance *
            (u_fMove_sense_offset + max_value * u_fMove_sense_coef);
  x += dp.x;
  y += dp.y;
  z += dp.z;

  // Keep the particle inside environment
  x = mod(x, u_iWorld_width);
  y = mod(y, u_iWorld_height);
  z = mod(z, u_iWorld_depth);

  // Check for u_fCollisions
  // uint val = 0;
  //// InterlockedCompareExchange(u_out_s3TextureOCC[uivec3(x, y, z)], 0, uint(u_fCollision), val);
  // atomicCompSwap(texture(u_in_s3TrailTexture, ivec3(x, y, z)).r, uint(u_fCollision), val);

  // if (val == 1.0) {
  //  x = particles_x[idx];
  //  y = particles_y[idx];
  //  z = particles_z[idx];
  //  t = acos(2 * float(wang_hash(idx * uint(x) * uint(y) * uint(z) + 4) % 1000) / 1000.0 - 1);
  //  ph = float(wang_hash(idx * uint(x) * uint(y) * uint(z) + 12) % 1000) / 1000.0 * 3.1415 * 2.0;
  //}

  // Update particle state
  particles_x[idx] = x;
  particles_y[idx] = y;
  particles_z[idx] = z;
  particles_theta[idx] = t;
  particles_phi[idx] = ph;

  float old_trail_value = imageLoad(u_in_s3TrailTexture, ivec3(x, y, z)).r;
  imageStore(u_in_s3TrailTexture, ivec3(x, y, z), vec4(old_trail_value + u_fDeposit_value));
}
