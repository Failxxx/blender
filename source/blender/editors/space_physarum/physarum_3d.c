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
#include "GPU_context.h"
#include "GPU_framebuffer.h"
#include "GPU_immediate.h"
#include "GPU_immediate_util.h"
#include "GPU_viewport.h"
#include "GPU_compute.h"

#include "WM_api.h"

#include "physarum_intern.h"

/* Free data functions */

void P3D_free_particles_ssbo(Physarum3D *p3d)
{
  printf("Physarum 3D: free particles ssbo\n");
  GPU_vertbuf_discard(p3d->ssbo);
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
  GPU_texture_free(p3d->texture_occ);
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
  printf("Physarum 3D: free complete\n");
}

/* Generate data functions */

void P3D_generate_particles_ssbo(Physarum3D *p3d)
{
  printf("Physarum 3D: generate particles ssbo\n");
  GPUVertFormat *format = immVertexFormat();
  uint x_pos = GPU_vertformat_attr_add(format, "particles_x", GPU_COMP_F32, 1, GPU_FETCH_FLOAT);
  uint y_pos = GPU_vertformat_attr_add(format, "particles_y", GPU_COMP_F32, 1, GPU_FETCH_FLOAT);
  uint z_pos = GPU_vertformat_attr_add(format, "particles_z", GPU_COMP_F32, 1, GPU_FETCH_FLOAT);
  uint phi_pos = GPU_vertformat_attr_add(format, "particles_phi", GPU_COMP_F32, 1, GPU_FETCH_FLOAT);
  uint theta_pos = GPU_vertformat_attr_add(format, "particles_theta", GPU_COMP_F32, 1, GPU_FETCH_FLOAT);

  p3d->ssbo = GPU_vertbuf_create_with_format(format);
  GPU_vertbuf_data_alloc(p3d->ssbo, p3d->nb_particles);

  // Fill the vertex buffer with vertices data
  for (int i = 0; i < p3d->nb_particles; i++) {
    float x_val[1] = {p3d->particles.x[i]};
    float y_val[1] = {p3d->particles.y[i]};
    float z_val[1] = {p3d->particles.z[i]};
    float phi_val[1] = {p3d->particles.phi[i]};
    float theta_val[1] = {p3d->particles.theta[i]};

    GPU_vertbuf_attr_set(p3d->ssbo, x_pos, i, x_val);
    GPU_vertbuf_attr_set(p3d->ssbo, y_pos, i, y_val);
    GPU_vertbuf_attr_set(p3d->ssbo, z_pos, i, z_val);
    GPU_vertbuf_attr_set(p3d->ssbo, phi_pos, i, phi_val);
    GPU_vertbuf_attr_set(p3d->ssbo, theta_pos, i, theta_val);
  }
}

void P3D_load_shaders(Physarum3D *p3d)
{
  printf("Physarum 3D: load shaders\n");
  p3d->shader_particle_3d = GPU_shader_create_compute(
      (const char *[]){datatoc_gpu_shader_3D_physarum_3d_particle_cs_glsl, NULL},
      NULL,
      NULL,
      "p3d_gpu_shader_compute_particles_3d");

  p3d->shader_decay = GPU_shader_create_compute(
      (const char *[]){datatoc_gpu_shader_3D_physarum_3d_decay_cs_glsl, NULL},
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

  p3d->texture_occ = GPU_texture_create_3d("physarum 3d occ tex",
                                               p3d->world_width,
                                               p3d->world_height,
                                               p3d->world_depth,
                                               0,
                                               GPU_RGBA32UI,
                                               GPU_DATA_UINT,
                                               NULL);
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
    phi = BLI_rng_get_float(rng) * M_2_PI;
    theta = acos(2.0 * BLI_rng_get_float(rng) - 1.0);
    radius = pow(BLI_rng_get_float(rng), 0.333333f) * p3d->spawn_radius;

    p3d->particles.x[i] = sin(phi) * sin(theta) * radius + p3d->world_width / 2.0f;
    p3d->particles.y[i] = cos(theta) * radius + p3d->world_height / 2.0f;
    p3d->particles.z[i] = cos(phi) * sin(theta) * radius + p3d->world_depth / 2.0f;
    p3d->particles.phi[i] = BLI_rng_get_float(rng) * M_2_PI;
    p3d->particles.theta[i] = acos(2.0f * BLI_rng_get_float(rng) - 1.0f);
    p3d->particles.pair[i] = 1e8;  // 100 000 000 means no pair
  }

  BLI_rng_free(rng);
}

void initialize_physarum_3d(Physarum3D *p3d)
{
  printf("Physarum 3D: initialize data\n");
  const float idMatrix[4][4] = {{1.0f, 0.0f, 0.0f, 0.0f},
                                {0.0f, 1.0f, 0.0f, 0.0f},
                                {0.0f, 0.0f, 1.0f, 0.0f},
                                {0.0f, 0.0f, 0.0f, 1.0f}};

  perspective_m4(p3d->projection_matrix, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1000.0f);
  copy_m4_m4(p3d->model_matrix, idMatrix);
  copy_m4_m4(p3d->view_matrix, idMatrix);
  translate_m4(p3d->view_matrix, 0.0f, 0.0f, -3.0f);
  p3d->tex_coord_map = 0;

  p3d->nb_particles = 1024 * 1024;
  p3d->texture_size = 1024;
  p3d->screen_width = 1024;
  p3d->screen_height = 1024;

  p3d->world_width = 500.0f;
  p3d->world_height = 500.0f;
  p3d->world_depth = 500.0f;
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
  printf("Physarum 3D: initialization complete\n");
}

/* Draw functions */

void physarum_3d_draw_view(Physarum3D *p3d)
{
  // Clear color framebuffer
  const float transparent[4] = {0.0f, 0.0f, 0.0f, 0.0f};

  /* Compute update particles */
  {
    GPU_shader_bind(p3d->shader_particle_3d);
    GPU_vertbuf_bind_as_ssbo(p3d->ssbo, p3d->ssbo_binding);

    GPU_shader_uniform_1f(p3d->shader_particle_3d, "sense_spread", p3d->sensor_spread);
    GPU_shader_uniform_1f(p3d->shader_particle_3d, "sense_distance", p3d->sensor_distance);
    GPU_shader_uniform_1f(p3d->shader_particle_3d, "turn_angle", p3d->turn_angle);
    GPU_shader_uniform_1f(p3d->shader_particle_3d, "move_distance", p3d->move_distance);
    GPU_shader_uniform_1f(p3d->shader_particle_3d, "deposit_value", p3d->deposit_value);
    GPU_shader_uniform_1f(p3d->shader_particle_3d, "decay_factor", p3d->decay_factor);
    GPU_shader_uniform_1f(p3d->shader_particle_3d, "collision", p3d->collision);
    GPU_shader_uniform_1f(p3d->shader_particle_3d, "center_attraction", p3d->center_attraction);
    GPU_shader_uniform_1i(p3d->shader_particle_3d, "world_width", (int)p3d->world_width);
    GPU_shader_uniform_1i(p3d->shader_particle_3d, "world_height", (int)p3d->world_height);
    GPU_shader_uniform_1i(p3d->shader_particle_3d, "world_depth", (int)p3d->world_depth);
    GPU_shader_uniform_1f(p3d->shader_particle_3d, "move_sense_coef", p3d->move_sensor_coef);
    GPU_shader_uniform_1f(p3d->shader_particle_3d, "move_sense_offset", p3d->move_sensor_offset);

    GPU_compute_dispatch(p3d->shader_particle_3d, 10, 10, 10); // Launch compute
    GPU_memory_barrier(GPU_BARRIER_SHADER_STORAGE); // Check if compute has been done

  }
}

/* Handle events functions */

void physarum_3d_handle_events(Physarum3D *p3d)
{
  
}
