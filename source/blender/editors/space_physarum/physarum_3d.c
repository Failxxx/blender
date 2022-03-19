/** \file
 * \ingroup spphysarum
 */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "MEM_guardedalloc.h"

#include "BLI_blenlib.h"
#include "BLI_rand.h"
#include "BLI_utildefines.h"

#include "BKE_context.h"
#include "BKE_screen.h"

#include "ED_screen.h"
#include "ED_space_api.h"

#include "GPU_capabilities.h"
#include "GPU_compute.h"
#include "GPU_context.h"
#include "GPU_framebuffer.h"
#include "GPU_immediate.h"
#include "GPU_immediate_util.h"
#include "GPU_viewport.h"

#include "WM_api.h"

#include "physarum_intern.h"

/* Generate geometry data for a super quad mesh */
GPUVertBuf *make_new_super_quad_mesh()
{
  float verts[6][4] = {{-1.0f, -1.0f, 0.0f, 1.0f},  // First triangle
                       {1.0f, 1.0f, 0.0f, 1.0f},
                       {-1.0f, 1.0f, 0.0f, 1.0f},
                       {-1.0f, -1.0f, 0.0f, 1.0f},  // Second triangle
                       {1.0f, -1.0f, 0.0f, 1.0f},
                       {1.0f, 1.0f, 0.0f, 1.0f}};
  float uvs[6][3] = {{0.0f, 1.0f, 0.0f},
                     {1.0f, 0.0f, 0.0f},
                     {0.0f, 0.0f, 0.0f},
                     {0.0f, 1.0f, 0.0f},
                     {1.0f, 1.0f, 0.0f},
                     {1.0f, 0.0f, 0.0f}};
  uint verts_len = 6;

  // Also known as "stride" (OpenGL), specifies the space between consecutive vertex attributes
  uint pos_comp_len = 4;
  uint uvs_comp_len = 3;

  GPUVertFormat *format = immVertexFormat();
  uint pos = GPU_vertformat_attr_add(
      format, "v_in_f4Position", GPU_COMP_F32, pos_comp_len, GPU_FETCH_FLOAT);
  uint uv = GPU_vertformat_attr_add(
      format, "v_in_f3Texcoord", GPU_COMP_F32, uvs_comp_len, GPU_FETCH_FLOAT);

  GPUVertBuf *vbo = GPU_vertbuf_create_with_format(format);
  GPU_vertbuf_data_alloc(vbo, verts_len);

  // Fill the vertex buffer with vertices data
  for (int i = 0; i < verts_len; i++) {
    GPU_vertbuf_attr_set(vbo, pos, i, verts[i]);
    GPU_vertbuf_attr_set(vbo, uv, i, uvs[i]);
  }

  return vbo;
}

void get_m4_identity(float m4_dest[4][4])
{
  const float m4_identity[4][4] = {{1.0f, 0.0f, 0.0f, 0.0f},
                                   {0.0f, 1.0f, 0.0f, 0.0f},
                                   {0.0f, 0.0f, 1.0f, 0.0f},
                                   {0.0f, 0.0f, 0.0f, 1.0f}};
  copy_m4_m4(m4_dest, m4_identity);
}

void get_perspective_projection(
    float dest[4][4], const float fov, const float aspect_ratio, const float near, const float far)
{
  const float cot_fov = 1.0f / tan(fov / 2.0f);
  dest[0][0] = cot_fov / aspect_ratio;
  dest[1][1] = cot_fov;
  dest[2][2] = -1.0f * far / (far - near);
  dest[3][2] = -1.0f;
  dest[2][3] = -1.0f * near * far / (far - near);
}

void get_look_at(float dest[4][4], const float eye[3], const float target[3], const float up[3])
{
  float z[3];
  sub_v3_v3v3(z, eye, target);
  normalize_v3(z);
  float x[3];
  cross_v3_v3v3(x, up, z);
  normalize_v3(x);
  float y[3];
  cross_v3_v3v3(y, z, x);
  normalize_v3(y);

  dest[0][0] = x[0];
  dest[1][0] = y[0];
  dest[2][0] = z[0];
  dest[3][0] = 0.0f;
  dest[0][1] = x[1];
  dest[1][1] = y[1];
  dest[2][1] = z[1];
  dest[3][1] = 0.0f;
  dest[0][2] = x[2];
  dest[1][2] = y[2];
  dest[2][2] = z[2];
  dest[3][2] = 0.0f;
  dest[0][3] = -1.0f * dot_v3v3(x, eye);
  dest[1][3] = -1.0f * dot_v3v3(y, eye);
  dest[2][3] = -1.0f * dot_v3v3(z, eye);
  dest[3][3] = 1.0f;
}

/* Free data functions */

void P3D_free_batches(Physarum3D *p3d)
{
  printf("Physarum 3D: free batches\n");
  GPU_batch_discard(p3d->batch);
}

void P3D_free_particles_ssbo(Physarum3D *p3d)
{
  printf("Physarum 3D: free particles ssbo\n");
  GPU_vertbuf_discard(p3d->ssbo_particles_x);
  GPU_vertbuf_discard(p3d->ssbo_particles_y);
  GPU_vertbuf_discard(p3d->ssbo_particles_z);
  GPU_vertbuf_discard(p3d->ssbo_particles_phi);
  GPU_vertbuf_discard(p3d->ssbo_particles_theta);
}

void P3D_free_shaders(Physarum3D *p3d)
{
  printf("Physarum 3D: free shaders\n");
  GPU_shader_free(p3d->shader_decay);
  GPU_shader_free(p3d->shader_particle_3d);
  GPU_shader_free(p3d->shader_render);
}

void P3D_free_textures(Physarum3D *p3d)
{
  printf("Physarum 3D: free textures\n");
  GPU_texture_free(p3d->texture_trail_A);
  GPU_texture_free(p3d->texture_trail_B);
  //GPU_texture_free(p3d->texture_occ);
}

void P3D_free_particles(Physarum3D *p3d)
{
  printf("Physarum 3D: free particles\n");
  free(p3d->particles.x);
  free(p3d->particles.y);
  free(p3d->particles.z);
  free(p3d->particles.phi);
  free(p3d->particles.theta);
  free(p3d->particles.pair);
}

void free_physarum_3d(Physarum3D *p3d)
{
  printf("Physarum 3D: free data\n");
  P3D_free_particles(p3d);
  P3D_free_particles_ssbo(p3d);
  P3D_free_shaders(p3d);
  P3D_free_textures(p3d);
  P3D_free_batches(p3d);
  printf("Physarum 3D: free complete\n");
}

/* Generate data functions */

void P3D_generate_batches(Physarum3D *p3d)
{
  printf("Physarum 3D: generate batches\n");
  GPUVertBuf *super_quad_mesh = make_new_super_quad_mesh();

  p3d->batch = GPU_batch_create_ex(GPU_PRIM_TRIS, super_quad_mesh, NULL, GPU_BATCH_OWNS_VBO);
}

void P3D_generate_particles_ssbo(Physarum3D *p3d)
{
  printf("Physarum 3D: generate particles ssbo\n");

  GPUVertFormat *format_x = immVertexFormat();
  GPUVertFormat *format_y = immVertexFormat();
  GPUVertFormat *format_z = immVertexFormat();
  GPUVertFormat *format_phi = immVertexFormat();
  GPUVertFormat *format_theta = immVertexFormat();

  uint x_pos = GPU_vertformat_attr_add(format_x, "particles_x", GPU_COMP_F32, 1, GPU_FETCH_FLOAT);
  uint y_pos = GPU_vertformat_attr_add(format_y, "particles_y", GPU_COMP_F32, 1, GPU_FETCH_FLOAT);
  uint z_pos = GPU_vertformat_attr_add(format_z, "particles_z", GPU_COMP_F32, 1, GPU_FETCH_FLOAT);
  uint phi_pos = GPU_vertformat_attr_add(
      format_phi, "particles_phi", GPU_COMP_F32, 1, GPU_FETCH_FLOAT);
  uint theta_pos = GPU_vertformat_attr_add(
      format_theta, "particles_theta", GPU_COMP_F32, 1, GPU_FETCH_FLOAT);

  p3d->ssbo_particles_x = GPU_vertbuf_create_with_format(format_x);
  p3d->ssbo_particles_y = GPU_vertbuf_create_with_format(format_y);
  p3d->ssbo_particles_z = GPU_vertbuf_create_with_format(format_z);
  p3d->ssbo_particles_phi = GPU_vertbuf_create_with_format(format_phi);
  p3d->ssbo_particles_theta = GPU_vertbuf_create_with_format(format_theta);

  GPU_vertbuf_data_alloc(p3d->ssbo_particles_x, p3d->nb_particles);
  GPU_vertbuf_data_alloc(p3d->ssbo_particles_y, p3d->nb_particles);
  GPU_vertbuf_data_alloc(p3d->ssbo_particles_z, p3d->nb_particles);
  GPU_vertbuf_data_alloc(p3d->ssbo_particles_phi, p3d->nb_particles);
  GPU_vertbuf_data_alloc(p3d->ssbo_particles_theta, p3d->nb_particles);

  //// Fill the vertex buffer with vertices data
  for (int i = 0; i < p3d->nb_particles; i++) {
    float x_val[1] = {p3d->particles.x[i]};
    float y_val[1] = {p3d->particles.y[i]};
    float z_val[1] = {p3d->particles.z[i]};
    float phi_val[1] = {p3d->particles.phi[i]};
    float theta_val[1] = {p3d->particles.theta[i]};

    GPU_vertbuf_attr_set(p3d->ssbo_particles_x, x_pos, i, x_val);
    GPU_vertbuf_attr_set(p3d->ssbo_particles_y, y_pos, i, y_val);
    GPU_vertbuf_attr_set(p3d->ssbo_particles_z, z_pos, i, z_val);
    GPU_vertbuf_attr_set(p3d->ssbo_particles_phi, phi_pos, i, phi_val);
    GPU_vertbuf_attr_set(p3d->ssbo_particles_theta, theta_pos, i, theta_val);
  }
}

void P3D_load_shaders(Physarum3D *p3d)
{
  printf("Physarum 3D: load shaders\n");
  p3d->shader_particle_3d = GPU_shader_create_compute(
      datatoc_gpu_shader_3D_physarum_3d_particle_cs_glsl,
      NULL,
      NULL,
      "p3d_gpu_shader_compute_particles_3d");

  p3d->shader_decay = GPU_shader_create_compute(datatoc_gpu_shader_3D_physarum_3d_decay_cs_glsl,
                                                NULL,
                                                NULL,
                                                "p3d_gpu_shader_compute_diffuse_decay");

  p3d->shader_render = GPU_shader_create_from_arrays({
      .vert = (const char *[]){datatoc_gpu_shader_3D_physarum_3d_vertex_vs_glsl, NULL},
      .frag = (const char *[]){datatoc_gpu_shader_3D_physarum_3d_pixel_fs_glsl, NULL},
  });
}

void P3D_generate_textures(Physarum3D *p3d)
{
  printf("Physarum 3D: generate textures\n");
  p3d->texture_trail_A = GPU_texture_create_3d("physarum 3d trail A tex",
                                               p3d->world_width,
                                               p3d->world_height,
                                               p3d->world_depth,
                                               0,
                                               GPU_RGBA32F,
                                               GPU_DATA_FLOAT,
                                               NULL);

  p3d->texture_trail_B = GPU_texture_create_3d("physarum 3d trail B tex",
                                               p3d->world_width,
                                               p3d->world_height,
                                               p3d->world_depth,
                                               0,
                                               GPU_RGBA32F,
                                               GPU_DATA_FLOAT,
                                               NULL);

  //p3d->texture_occ = GPU_texture_create_3d("physarum 3d occ tex",
  //                                         p3d->world_width,
  //                                         p3d->world_height,
  //                                         p3d->world_depth,
  //                                         0,
  //                                         GPU_RGBA32UI,
  //                                         GPU_DATA_UINT,
  //                                         NULL);
}

void P3D_generate_particles_data(Physarum3D *p3d)
{
  printf("Physarum 3D: generate particles data\n");
  RNG *rng = BLI_rng_new_srandom(5831);  // Arbitrary, random values generator
  float phi, theta, radius;

  // Allocate memory for particles
  int bytes = sizeof(float) * p3d->nb_particles;
  p3d->particles.x = (float *)malloc(bytes);
  p3d->particles.y = (float *)malloc(bytes);
  p3d->particles.z = (float *)malloc(bytes);
  p3d->particles.phi = (float *)malloc(bytes);
  p3d->particles.theta = (float *)malloc(bytes);
  p3d->particles.pair = (float *)malloc(bytes);

  for (int i = 0; i < p3d->nb_particles; ++i) {
    phi = BLI_rng_get_float(rng) * 2.0 * M_PI;
    theta = acos(2.0 * BLI_rng_get_float(rng) - 1.0);
    radius = pow(BLI_rng_get_float(rng), 0.333333f) * p3d->spawn_radius;

    p3d->particles.x[i] = sin(phi) * sin(theta) * radius + p3d->world_width / 2.0f;
    p3d->particles.y[i] = cos(theta) * radius + p3d->world_height / 2.0f;
    p3d->particles.z[i] = cos(phi) * sin(theta) * radius + p3d->world_depth / 2.0f;
    p3d->particles.phi[i] = acos(2.0 * BLI_rng_get_float(rng) - 1.0);
    p3d->particles.theta[i] = BLI_rng_get_float(rng) * 2.0 * M_PI;
    p3d->particles.pair[i] = 1e8;  // 100 000 000 means no pair
  }

  BLI_rng_free(rng);
}

void initialize_physarum_3d(Physarum3D *p3d)
{
  printf("Physarum 3D: initialize data\n");
  /* Setup matrices */
  zero_m4(p3d->projection_matrix);
  get_perspective_projection(p3d->projection_matrix, DEG2RAD(60.0f), 16.0f / 9.0f, 0.01f, 10.0f);
  get_m4_identity(p3d->model_matrix);
  float azimuth = 0.0f;
  float polar = M_PI_2;
  float radius = 2.0f;
  float eye_pos[3] = {
      cos(azimuth) * sin(polar) * radius, cos(polar) * radius, sin(azimuth) * sin(polar) * radius};
  float target[3] = {0.0f, 0.0f, 0.0f};
  float up[3] = {0.0f, 1.0f, 0.0f};
  get_look_at(p3d->view_matrix, eye_pos, target, up);

  p3d->tex_coord_map = 0;

  p3d->screen_width = 1024;
  p3d->screen_height = 1024;
  p3d->nb_particles = 100;

  p3d->world_width = 480.0f;
  p3d->world_height = 480.0f;
  p3d->world_depth = 480.0f;
  p3d->spawn_radius = 50.0f;

  p3d->sensor_spread = 0.48f;
  p3d->sensor_distance = 23.0f;
  p3d->turn_angle = 0.63f;
  p3d->move_distance = 2.77f;
  p3d->deposit_value = 5.0f;
  p3d->decay_factor = 0.32f;
  p3d->collision = 0.0f;
  p3d->center_attraction = 1.0f;
  p3d->move_sensor_coef = 0.0f;
  p3d->move_sensor_offset = 1.0f;

  P3D_generate_particles_data(p3d);
  P3D_generate_particles_ssbo(p3d);
  P3D_load_shaders(p3d);
  P3D_generate_textures(p3d);
  P3D_generate_batches(p3d);
  printf("Physarum 3D: initialization complete\n");
}

/* Draw functions */

void physarum_3d_draw_view(Physarum3D *p3d)
{
  // Clear color framebuffer
  const float transparent[4] = {0.0f, 0.2f, 0.0f, 1.0f};

  /* Compute update particles */
  if (1) {

    // Switch trails map
    p3d->is_trail_A = (p3d->is_trail_A) ? 0 : 1;

    GPU_shader_bind(p3d->shader_particle_3d);

    p3d->ssbo_binding_particles_x = GPU_shader_get_ssbo(p3d->shader_particle_3d,
                                                        "ssbo_particles_x");
    p3d->ssbo_binding_particles_y = GPU_shader_get_ssbo(p3d->shader_particle_3d,
                                                        "ssbo_particles_y");
    p3d->ssbo_binding_particles_z = GPU_shader_get_ssbo(p3d->shader_particle_3d,
                                                        "ssbo_particles_z");
    p3d->ssbo_binding_particles_phi = GPU_shader_get_ssbo(p3d->shader_particle_3d,
                                                          "ssbo_particles_phi");
    p3d->ssbo_binding_particles_theta = GPU_shader_get_ssbo(p3d->shader_particle_3d,
                                                            "ssbo_particles_theta");

    GPU_vertbuf_bind_as_ssbo(p3d->ssbo_particles_x, p3d->ssbo_binding_particles_x);
    GPU_vertbuf_bind_as_ssbo(p3d->ssbo_particles_y, p3d->ssbo_binding_particles_y);
    GPU_vertbuf_bind_as_ssbo(p3d->ssbo_particles_z, p3d->ssbo_binding_particles_z);
    GPU_vertbuf_bind_as_ssbo(p3d->ssbo_particles_phi, p3d->ssbo_binding_particles_phi);
    GPU_vertbuf_bind_as_ssbo(p3d->ssbo_particles_theta, p3d->ssbo_binding_particles_theta);

    int texture_binding;
    if (p3d->is_trail_A) {
      texture_binding = GPU_shader_get_texture_binding(p3d->shader_decay, "u_in_s3TrailTexture");
      GPU_texture_image_bind(p3d->texture_trail_A, texture_binding);
    }
    else {
      texture_binding = GPU_shader_get_texture_binding(p3d->shader_decay, "u_in_s3TrailTexture");
      GPU_texture_image_bind(p3d->texture_trail_B, texture_binding);
    }

    GPU_shader_uniform_1f(p3d->shader_particle_3d, "sense_spread", p3d->sensor_spread);
    GPU_shader_uniform_1f(p3d->shader_particle_3d, "sense_distance", p3d->sensor_distance);
    GPU_shader_uniform_1f(p3d->shader_particle_3d, "turn_angle", p3d->turn_angle);
    GPU_shader_uniform_1f(p3d->shader_particle_3d, "move_distance", p3d->move_distance);
    GPU_shader_uniform_1f(p3d->shader_particle_3d, "deposit_value", p3d->deposit_value);
    GPU_shader_uniform_1f(p3d->shader_particle_3d, "decay_factor", p3d->decay_factor);
    // GPU_shader_uniform_1f(p3d->shader_particle_3d, "collision", p3d->collision);
    GPU_shader_uniform_1f(p3d->shader_particle_3d, "center_attraction", p3d->center_attraction);
    GPU_shader_uniform_1i(p3d->shader_particle_3d, "world_width", (int)p3d->world_width);
    GPU_shader_uniform_1i(p3d->shader_particle_3d, "world_height", (int)p3d->world_height);
    GPU_shader_uniform_1i(p3d->shader_particle_3d, "world_depth", (int)p3d->world_depth);
    GPU_shader_uniform_1f(p3d->shader_particle_3d, "move_sense_coef", p3d->move_sensor_coef);
    GPU_shader_uniform_1f(p3d->shader_particle_3d, "move_sense_offset", p3d->move_sensor_offset);

    GPU_compute_dispatch(p3d->shader_particle_3d, 10, 10, 10);  // Launch compute
    GPU_memory_barrier(GPU_BARRIER_SHADER_STORAGE);             // Check if compute has been done

    if (p3d->is_trail_A)
      GPU_texture_image_unbind(p3d->texture_trail_A);
    else
      GPU_texture_image_unbind(p3d->texture_trail_B);
  }

  /* Compute diffuse / decay */
  {
    GPU_shader_bind(p3d->shader_decay);

    // Bind trails data textures
    int texture_binding;
    if (p3d->is_trail_A) {
      texture_binding = GPU_shader_get_texture_binding(p3d->shader_decay, "u_in_s3TextureTrail");
      GPU_texture_image_bind(p3d->texture_trail_A, texture_binding);
      texture_binding = GPU_shader_get_texture_binding(p3d->shader_decay, "u_out_s3TextureTrail");
      GPU_texture_bind(p3d->texture_trail_B, texture_binding);
    }
    else {
      texture_binding = GPU_shader_get_texture_binding(p3d->shader_decay, "u_in_s3TextureTrail");
      GPU_texture_image_bind(p3d->texture_trail_B, texture_binding);
      texture_binding = GPU_shader_get_texture_binding(p3d->shader_decay, "u_out_s3TextureTrail");
      GPU_texture_bind(p3d->texture_trail_A, texture_binding);
    }

    // Send uniforms
    GPU_shader_uniform_1f(p3d->shader_decay, "u_fDecay_factor", p3d->decay_factor);

    GPU_compute_dispatch(p3d->shader_decay,
                         (int)p3d->world_width / 8,
                         (int)p3d->world_height / 8,
                         (int)p3d->world_depth / 8);
    GPU_memory_barrier(GPU_BARRIER_TEXTURE_FETCH);

    // Unbind textures
    GPU_texture_image_unbind(p3d->texture_trail_A);
    GPU_texture_image_unbind(p3d->texture_trail_B);
  }

  /* Render final result */
  {
    GPU_batch_set_shader(p3d->batch, p3d->shader_render);
    GPU_framebuffer_clear(GPU_framebuffer_active_get(), GPU_COLOR_BIT, transparent, 0, 0);

    GPU_batch_uniform_mat4(p3d->batch, "u_m4Projection_matrix", p3d->projection_matrix);
    GPU_batch_uniform_mat4(p3d->batch, "u_m4View_matrix", p3d->view_matrix);
    if (p3d->is_trail_A) {
      GPU_batch_texture_bind(p3d->batch, "u_s3TrailsData", p3d->texture_trail_A);
    }
    else {
      GPU_batch_texture_bind(p3d->batch, "u_s3TrailsData", p3d->texture_trail_B);
    }
    GPU_batch_uniform_1i(p3d->batch, "u_s3TrailsData", 0);

    axis_angle_to_mat4_single(p3d->model_matrix, 'X', M_PI_2);
    GPU_batch_uniform_mat4(p3d->batch, "u_m4Model_matrix", p3d->model_matrix);
    GPU_batch_uniform_1i(p3d->batch, "u_iTexcoord_map", 2);

    GPU_batch_draw(p3d->batch);  // First pass

    axis_angle_to_mat4_single(p3d->model_matrix, 'Y', -1.0 * M_PI_2);
    GPU_batch_uniform_mat4(p3d->batch, "u_m4Model_matrix", p3d->model_matrix);
    GPU_batch_uniform_1i(p3d->batch, "u_iTexcoord_map", 1);

    GPU_batch_draw(p3d->batch);  // Second pass

    get_m4_identity(p3d->model_matrix);
    GPU_batch_uniform_mat4(p3d->batch, "u_m4Model_matrix", p3d->model_matrix);
    GPU_batch_uniform_1i(p3d->batch, "u_iTexcoord_map", 0);

    GPU_batch_draw(p3d->batch);  // Third pass
  }
}

/* Handle events functions */

void physarum_3d_handle_events(Physarum3D *p3d,
                               SpacePhysarum *sphys,
                               const bContext *C,
                               ARegion *region)
{
  // Update simulation parameters
  p3d->sensor_spread = sphys->sensor_angle;
  p3d->sensor_distance = sphys->sensor_distance;
  p3d->turn_angle = sphys->rotation_angle;
  p3d->move_distance = sphys->move_distance;
  p3d->deposit_value = sphys->deposit_value;
  p3d->decay_factor = sphys->decay_factor;
  p3d->center_attraction = sphys->center_attraction;

  // Resize event
  if (region->winx != p3d->screen_width && region->winy != p3d->screen_height) {
    p3d->screen_width = region->winx;
    p3d->screen_height = region->winy;
  }
}
