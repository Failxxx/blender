
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_shader_storage_buffer_object : enable

uniform sampler3D u_in_s3Texture;
layout(rgba32f, binding = 0) uniform image3D u_out_s3TextureOCC;

uniform float u_fDecay_factor;

// const uvec3 gl_WorkGroupSize; // defined by blender params ?
layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

void main()
{
  uvec3 position = gl_GlobalInvocationID.xyz;
  float value = 0.0f;

  for (int dx = -1; dx <= 1; dx++) {
    for (int dy = -1; dy <= 1; dy++) {
      for (int dz = -1; dz <= 1; dz++) {
        value += texture(u_in_s3Texture, ivec3(position) + ivec3(dx, dy, dz)).r * u_fDecay_factor / 27.0;
      }
    }
  }

  imageStore(u_out_s3TextureOCC, ivec3(position), vec4(value));
}
