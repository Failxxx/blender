/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2008 Blender Foundation.
 * All rights reserved.
 */

/** \file
 * \ingroup spphysarum
 */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "MEM_guardedalloc.h"

#include "BLI_blenlib.h"
#include "BLI_utildefines.h"
#include "BLI_rand.h"

#include "BKE_context.h"
#include "BKE_screen.h"

#include "ED_screen.h"
#include "ED_space_api.h"

#include "GPU_immediate.h"
#include "GPU_immediate_util.h"
#include "GPU_capabilities.h"
#include "GPU_context.h"
#include "GPU_framebuffer.h"
#include "GPU_viewport.h"
#include "GPU_texture.h"

#include "glew-mx.h"

#include "WM_api.h"

#include "physarum_intern.h"

/* Utility functions */

float get_elapsed_time(struct timespec *start)
{
  struct timespec now;
  timespec_get(&now, TIME_UTC);
  return (now.tv_sec + now.tv_nsec / 1e9) - (start->tv_sec + start->tv_sec / 1e9);
}

/* Generate geometry data for a quad mesh */
GPUVertBuf *make_new_quad_mesh()
{
  float verts[6][3] = {{-1.0f, -1.0f, 0.0f},  // First triangle
                       {1.0f, -1.0f, 0.0f},
                       {-1.0f, 1.0f, 0.0f},
                       {1.0f, -1.0f, 0.0f},  // Second triangle
                       {-1.0f, 1.0f, 0.0f},
                       {1.0f, 1.0f, 0.0f}};
  float uvs[6][2] = {
      {0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}};
  uint verts_len = 6;

  // Also known as "stride" (OpenGL), specifies the space between consecutive vertex attributes
  uint pos_comp_len = 3;
  uint uvs_comp_len = 2;

  GPUVertFormat *format = immVertexFormat();
  uint pos = GPU_vertformat_attr_add(
      format, "v_in_f3Position", GPU_COMP_F32, pos_comp_len, GPU_FETCH_FLOAT);
  uint uv = GPU_vertformat_attr_add(
      format, "v_in_f2UV", GPU_COMP_F32, uvs_comp_len, GPU_FETCH_FLOAT);

  GPUVertBuf *vbo = GPU_vertbuf_create_with_format(format);
  GPU_vertbuf_data_alloc(vbo, verts_len);

  // Fill the vertex buffer with vertices data
  for (int i = 0; i < verts_len; i++) {
    GPU_vertbuf_attr_set(vbo, pos, i, verts[i]);
    GPU_vertbuf_attr_set(vbo, uv, i, uvs[i]);
  }

  return vbo;
}

/* Generate geometry data for a points mesh */
GPUVertBuf *make_new_points_mesh(float *positions, float *uvs, int nb_points)
{
  // Also known as "stride" (OpenGL), specifies the space between consecutive vertex attributes
  uint pos_comp_len = 3;
  uint uvs_comp_len = 2;

  GPUVertFormat *format = immVertexFormat();
  uint pos = GPU_vertformat_attr_add(
      format, "v_in_f3Position", GPU_COMP_F32, pos_comp_len, GPU_FETCH_FLOAT);
  uint uv = GPU_vertformat_attr_add(
      format, "v_in_f2UV", GPU_COMP_F32, uvs_comp_len, GPU_FETCH_FLOAT);

  GPUVertBuf *vbo = GPU_vertbuf_create_with_format(format);
  GPU_vertbuf_data_alloc(vbo, nb_points);

  // Fill the vertex buffer with vertices data
  int id = 0;
  for (int i = 0; i < nb_points; i++) {
    id = i * 3;
    float position[3] = {positions[id], positions[id + 1], positions[id + 2]};
    id = i * 2;
    float uv_coord[2] = {uvs[id], uvs[id + 1]};
    GPU_vertbuf_attr_set(vbo, pos, i, position);
    GPU_vertbuf_attr_set(vbo, uv, i, uv_coord);
  }

  return vbo;
}

/* Free data functions */

void physarum_2d_free_textures(Physarum2D *p2d)
{
  printf("Physarum2D: free textures\n");
  /* Free textures */
  GPU_texture_free(p2d->trails_tex_current);
  GPU_texture_free(p2d->trails_tex_next);
  GPU_texture_free(p2d->agents_data_tex_current);
  GPU_texture_free(p2d->agents_data_tex_next);
  GPU_texture_free(p2d->agents_tex);
}

void physarum_2d_free_shaders(Physarum2D *p2d)
{
  printf("Physarum2D: free shaders\n");
  /* Free shaders */
  GPU_shader_free(p2d->diffuse_decay_shader);
  GPU_shader_free(p2d->update_agents_shader);
  GPU_shader_free(p2d->render_agents_shader);
  GPU_shader_free(p2d->post_process_shader);
}

void physarum_2d_free_batches(Physarum2D *p2d)
{
  printf("Physarum2D: free batches\n");
  GPU_batch_discard(p2d->diffuse_decay_batch);
  GPU_batch_discard(p2d->update_agents_batch);
  GPU_batch_discard(p2d->render_agents_batch);
  GPU_batch_discard(p2d->post_process_batch);
}

void physarum_2d_free_frame_buffers(Physarum2D *p2d)
{
  printf("Physarum2D: free frame buffers\n");
  GPU_FRAMEBUFFER_FREE_SAFE(p2d->diffuse_decay_fb);
  GPU_FRAMEBUFFER_FREE_SAFE(p2d->update_agents_fb);
  GPU_FRAMEBUFFER_FREE_SAFE(p2d->render_agents_fb);
}

void free_physarum_2d(Physarum2D *p2d)
{
  printf("Physarum2D: free data\n");
  physarum_2d_free_textures(p2d);
  physarum_2d_free_batches(p2d);
  physarum_2d_free_shaders(p2d);
  //physarum_2d_free_frame_buffers(p2d); TODO: fix free error
  MEM_freeN(p2d->start_time);
  printf("Physarum2D: free complete\n");
}

/* Generate data functions */

void physarum_2d_gen_particles_data(Physarum2D *p2d, float *pos, float *uvs)
{
  printf("Physarum2D: gen particles data\n");
  RNG *rng = BLI_rng_new_srandom(5831); // Arbitrary, random values generator

  /* Fill buffers */
  int id = 0;
  float u = 0;
  float v = 0;
  for (int i = 0; i < p2d->nb_particles; ++i) {
    // Point cloud vertex
    id = i * 3;
    pos[id++] = 0.0f;
    pos[id++] = 0.0f;
    pos[id++] = 0.0f;

    // Compute UVs
    u = (i % p2d->tex_width) / (float)p2d->tex_width;
    v = ~~(i / p2d->tex_height) / (float)p2d->tex_height;
    id = i * 2;
    uvs[id++] = u;
    uvs[id++] = v;
  }

  BLI_rng_free(rng);
}

void physarum_2d_gen_texture_data(Physarum2D* p2d, float *texture_data) {
  printf("Physarum2D: gen particles texture data\n");
  RNG *rng = BLI_rng_new_srandom(2236);  // Arbitrary, random values generator

  /* Fill buffer */
  int id = 0;
  for (int i = 0; i < p2d->nb_particles; ++i) {
    id = i * 4;
    texture_data[id++] = BLI_rng_get_float(rng);  // Normalized position X
    texture_data[id++] = BLI_rng_get_float(rng);  // Normalized position Y
    texture_data[id++] = BLI_rng_get_float(rng);  // Normalized angle
    texture_data[id++] = 1.0f;
  }

  BLI_rng_free(rng);
}

void physarum_2d_gen_textures(Physarum2D *p2d)
{
  printf("Physarum2D: gen textures\n");
  /* Generate textures */
  int bytes = sizeof(float) * 4 * p2d->nb_particles;
  float *agents_data = (float *)malloc(bytes);
  physarum_2d_gen_texture_data(p2d, agents_data);

  // Trails textures
  p2d->trails_tex_current = GPU_texture_create_2d(
      "phys2d trails tex current", p2d->tex_width, p2d->tex_height, 0, GPU_RGBA32F, NULL);
  GPU_texture_filter_mode(p2d->trails_tex_current, false); // Use GL_TEXTURE_MAG_FILTER = GL_NEAREST

  p2d->trails_tex_next = GPU_texture_create_2d(
      "phys2d trails tex next", p2d->tex_width, p2d->tex_height, 0, GPU_RGBA32F, NULL);
  GPU_texture_filter_mode(p2d->trails_tex_next, false);

  // Agents data textures
  p2d->agents_data_tex_current = GPU_texture_create_2d("phys2d agents data tex current",
                                                       p2d->tex_width,
                                                       p2d->tex_height,
                                                       0,
                                                       GPU_RGBA32F,
                                                       agents_data);
  GPU_texture_filter_mode(p2d->agents_data_tex_current, false);

  p2d->agents_data_tex_next = GPU_texture_create_2d("phys2d agents data tex next",
                                                    p2d->tex_width,
                                                    p2d->tex_height,
                                                    0,
                                                    GPU_RGBA32F, agents_data);
  GPU_texture_filter_mode(p2d->agents_data_tex_next, false);

  // Agents texture
  p2d->agents_tex = GPU_texture_create_2d(
      "phys2d agents tex", p2d->tex_width, p2d->tex_height, 0, GPU_RGBA32F, NULL);
  GPU_texture_filter_mode(p2d->agents_tex, false);

  free(agents_data);
}

void physarum_2d_gen_shaders(Physarum2D *p2d)
{
  printf("Physarum2D: load shaders\n");
  /* Load shaders */
  p2d->diffuse_decay_shader = GPU_shader_create_from_arrays(
      {.vert = (const char *[]){datatoc_gpu_shader_3D_physarum_2d_quad_vs_glsl, NULL},
       .frag = (const char *[]){datatoc_gpu_shader_3D_physarum_2d_diffuse_decay_fs_glsl, NULL}});

  p2d->update_agents_shader = GPU_shader_create_from_arrays(
      {.vert = (const char *[]){datatoc_gpu_shader_3D_physarum_2d_quad_vs_glsl, NULL},
       .frag = (const char *[]){datatoc_gpu_shader_3D_physarum_2d_update_agents_fs_glsl, NULL}});

  p2d->render_agents_shader = GPU_shader_create_from_arrays(
      {.vert = (const char *[]){datatoc_gpu_shader_3D_physarum_2d_render_agents_vs_glsl, NULL},
       .frag = (const char *[]){datatoc_gpu_shader_3D_physarum_2d_render_agents_fs_glsl, NULL}});

  p2d->post_process_shader = GPU_shader_create_from_arrays(
      {.vert = (const char *[]){datatoc_gpu_shader_3D_physarum_2d_quad_vs_glsl, NULL},
       .frag = (const char *[]){datatoc_gpu_shader_3D_physarum_2d_post_process_fs_glsl, NULL}});
}

void physarum_2d_gen_batches(Physarum2D *p2d)
{
  printf("Physarum2D: gen batches\n");
  /* Generate geometry data (3d render targets) */
  GPUVertBuf *quad_vbo_1 = make_new_quad_mesh();
  GPUVertBuf *quad_vbo_2 = make_new_quad_mesh();
  GPUVertBuf *quad_vbo_3 = make_new_quad_mesh();

  /* Allocate memory for particles data */
  int bytes = sizeof(float) * 3 * p2d->nb_particles;
  float *positions = (float *)malloc(bytes);
  bytes = sizeof(float) * 2 * p2d->nb_particles;
  float *uvs = (float *)malloc(bytes);

  /* Generate particles VBO */
  physarum_2d_gen_particles_data(p2d, positions, uvs);
  GPUVertBuf *points_vbo = make_new_points_mesh(positions, uvs, p2d->nb_particles);

  /* Create batches */
  p2d->diffuse_decay_batch = GPU_batch_create_ex(
      GPU_PRIM_TRIS, quad_vbo_1, NULL, GPU_BATCH_OWNS_VBO);
  p2d->update_agents_batch = GPU_batch_create_ex(
      GPU_PRIM_TRIS, quad_vbo_2, NULL, GPU_BATCH_OWNS_VBO);
  p2d->render_agents_batch = GPU_batch_create_ex(
      GPU_PRIM_POINTS, points_vbo, NULL, GPU_BATCH_OWNS_VBO);
  p2d->post_process_batch = GPU_batch_create_ex(
      GPU_PRIM_TRIS, quad_vbo_3, NULL, GPU_BATCH_OWNS_VBO);

  // Free particles data
  free(positions);
  free(uvs);
}

void physarum_2d_gen_frame_buffers(Physarum2D *p2d)
{
  printf("Physarum2D: gen frame buffers\n");
  /* Generate frame buffers */
  GPU_framebuffer_ensure_config(&p2d->diffuse_decay_fb,
                                {
                                    GPU_ATTACHMENT_NONE,
                                    GPU_ATTACHMENT_TEXTURE(p2d->trails_tex_next),
                                });
  GPU_framebuffer_ensure_config(&p2d->update_agents_fb,
                                {
                                    GPU_ATTACHMENT_NONE,
                                    GPU_ATTACHMENT_TEXTURE(p2d->agents_data_tex_next),
                                });
  GPU_framebuffer_ensure_config(&p2d->render_agents_fb,
                                {
                                    GPU_ATTACHMENT_NONE,
                                    GPU_ATTACHMENT_TEXTURE(p2d->agents_tex),
                                });
}

void initialize_physarum_2d(Physarum2D *p2d)
{
  printf("Physarum2D: initialize data\n");
  /* Default values */
  p2d->screen_width = 1024;
  p2d->screen_height = 1024;
  p2d->tex_width = 1024;
  p2d->tex_height = 1024;

  p2d->nb_particles = p2d->tex_width * p2d->tex_height;
  p2d->max_particles = p2d->nb_particles;
  p2d->min_particles = 1;

  p2d->sensor_angle = 2.0f;
  p2d->sensor_distance = 12.0f;
  p2d->move_distance = 1.1f;
  p2d->rotation_angle = 4.0f;

  orthographic_m4(p2d->projection_matrix, -1.0f, 1.0f, -1.0f, 1.0f, -0.1f, 100.0f);

  physarum_2d_gen_textures(p2d);
  physarum_2d_gen_batches(p2d);
  physarum_2d_gen_shaders(p2d);
  physarum_2d_gen_frame_buffers(p2d);

  // timespec struct : time_t tv_sec, long tv_nsec
  p2d->start_time = MEM_callocN(sizeof(time_t) + sizeof(long), "pysarum 2d start time");
  timespec_get(p2d->start_time, TIME_UTC);
  printf("Physarum2D: initialization complete\n");
}

/* Draw functions */

void physarum_2d_swap_textures(Physarum2D *p2d)
{
  // Swap trails frame buffers
  GPUTexture *trails_tex_current_temp = p2d->trails_tex_current;
  p2d->trails_tex_current = p2d->trails_tex_next;
  p2d->trails_tex_next = trails_tex_current_temp;

  // Swap agents data frame buffers
  GPUTexture *agents_data_tex_current_temp = p2d->agents_data_tex_current;
  p2d->agents_data_tex_current = p2d->agents_data_tex_next;
  p2d->agents_data_tex_next = agents_data_tex_current_temp;

  // Attach texture to frame buffers
  GPU_framebuffer_ensure_config(&p2d->diffuse_decay_fb,
                                {
                                    GPU_ATTACHMENT_NONE,
                                    GPU_ATTACHMENT_TEXTURE(p2d->trails_tex_next),
                                });
  GPU_framebuffer_ensure_config(&p2d->update_agents_fb,
                                {
                                    GPU_ATTACHMENT_NONE,
                                    GPU_ATTACHMENT_TEXTURE(p2d->agents_data_tex_next),
                                });
}

void physarum_2d_draw_view(Physarum2D *p2d)
{
  /* ----- Setup ----- */

  /* Get current framebuffer to be able to switch between frame buffers */
  GPUFrameBuffer *initial_fb = GPU_framebuffer_active_get();
  // Used for convenience
  GPUBatch *batch;
  GPUFrameBuffer *frameBuffer;

  // Elapsed time since the beginning of the simulation
  float time = get_elapsed_time(p2d->start_time);
  // Clear color framebuffer
  const float transparent[4] = {0.0f, 0.0f, 0.0f, 0.0f};

  /* ----- Compute trails ----- */
  {
    batch = p2d->diffuse_decay_batch;
    frameBuffer = p2d->diffuse_decay_fb;
    // Set shader
    GPU_batch_set_shader(batch, p2d->diffuse_decay_shader);

    // Send uniforms to shaders
    // Vertex shader
    GPU_batch_uniform_mat4(batch, "u_m4ProjectionMatrix", p2d->projection_matrix);

    // Pixel / Fragment shader
    GPU_batch_texture_bind(batch, "u_s2TrailsData", p2d->trails_tex_current);
    GPU_batch_uniform_1i(batch, "u_s2TrailsData", 0);

    GPU_batch_texture_bind(batch, "u_s2Agents", p2d->agents_tex);
    GPU_batch_uniform_1i(batch, "u_s2Agents", 1);

    GPU_batch_uniform_2f(batch, "u_f2Resolution", (float)p2d->tex_width, (float)p2d->tex_height);
    GPU_batch_uniform_1f(batch, "u_fDecay", p2d->decay);

    // Render to the trails texture
    GPU_framebuffer_bind(frameBuffer);
    GPU_framebuffer_viewport_set(frameBuffer, 0, 0, p2d->tex_width, p2d->tex_height);
    GPU_framebuffer_clear(frameBuffer, GPU_COLOR_BIT, transparent, 0, 0);
    GPU_batch_draw(batch);
  }

  /* ----- Update agents ----- */
  {
    batch = p2d->update_agents_batch;
    frameBuffer = p2d->update_agents_fb;
    // Set shader
    GPU_batch_set_shader(batch, p2d->update_agents_shader);

    // Send uniforms tho shaders
    // Vertex shader
    GPU_batch_uniform_mat4(batch, "u_m4ProjectionMatrix", p2d->projection_matrix);

    // Pixel / Fragment shader
    GPU_batch_texture_bind(batch, "u_s2AgentsData", p2d->agents_data_tex_current);
    GPU_batch_uniform_1i(batch, "u_s2AgentsData", 0);

    GPU_batch_texture_bind(batch, "u_s2TrailsData", p2d->trails_tex_current);
    GPU_batch_uniform_1i(batch, "u_s2TrailsData", 1);

    GPU_batch_uniform_2f(batch, "u_f2Resolution", (float)p2d->tex_width, (float)p2d->tex_height);
    GPU_batch_uniform_1f(batch, "u_fTime", time);
    GPU_batch_uniform_1f(batch, "u_fSA", p2d->sensor_angle);
    GPU_batch_uniform_1f(batch, "u_fRA", p2d->rotation_angle);
    GPU_batch_uniform_1f(batch, "u_fSO", p2d->sensor_distance);
    GPU_batch_uniform_1f(batch, "u_fSS", p2d->move_distance);

    // Render to the agents data texture
    GPU_framebuffer_bind(frameBuffer);
    GPU_framebuffer_viewport_set(frameBuffer, 0, 0, p2d->tex_width, p2d->tex_height);
    GPU_framebuffer_clear(frameBuffer, GPU_COLOR_BIT, transparent, 0, 0);
    GPU_batch_draw(batch);
  }

  /* ----- Render agents ----- */
  {
    batch = p2d->render_agents_batch;
    frameBuffer = p2d->render_agents_fb;
    // Set shader
    GPU_batch_set_shader(batch, p2d->render_agents_shader);

    // Send uniforms tho shaders
    // Vertex shader
    GPU_batch_uniform_mat4(batch, "u_m4ProjectionMatrix", p2d->projection_matrix);
    GPU_batch_texture_bind(batch, "u_s2AgentsData", p2d->agents_data_tex_current);
    GPU_batch_uniform_1i(batch, "u_s2AgentsData", 0);

    // Render to the agents texture
    GPU_framebuffer_bind(frameBuffer);
    GPU_framebuffer_viewport_set(frameBuffer, 0, 0, p2d->tex_width, p2d->tex_height);
    GPU_framebuffer_clear(frameBuffer, GPU_COLOR_BIT, transparent, 0, 0);
    GPU_batch_draw(batch);
  }

  /* ----- Render final result using post-process ----- */
  {
    batch = p2d->post_process_batch;
    frameBuffer = initial_fb;
    // Set shader
    GPU_batch_set_shader(batch, p2d->post_process_shader);

    // Send uniforms tho shaders
    // Vertex shader
    GPU_batch_uniform_mat4(batch, "u_m4ProjectionMatrix", p2d->projection_matrix);

    // Pixel / Fragment shader
    GPU_batch_texture_bind(batch, "u_s2TrailsData", p2d->trails_tex_current);
    GPU_batch_uniform_1i(batch, "u_s2TrailsData", 0);

    // Draw final result
    GPU_framebuffer_bind(frameBuffer);
    GPU_framebuffer_clear(frameBuffer, GPU_COLOR_BIT, transparent, 0, 0);
    GPU_batch_draw(batch);
  }

  // Swap textures
  physarum_2d_swap_textures(p2d);
}

/* Handle events functions */

void physarum_2d_update_particles(Physarum2D *p2d)
{
  /* Free old data */
  GPU_batch_discard(p2d->render_agents_batch);

  /* Generate new particles */
  int bytes = sizeof(float) * 3 * p2d->nb_particles;
  float *positions = (float *)malloc(bytes);
  bytes = sizeof(float) * 2 * p2d->nb_particles;
  float *uvs = (float *)malloc(bytes);

  physarum_2d_gen_particles_data(p2d, positions, uvs);
  GPUVertBuf *particles = make_new_points_mesh(positions, uvs, p2d->nb_particles);
  p2d->render_agents_batch = GPU_batch_create_ex(
      GPU_PRIM_POINTS, particles, NULL, GPU_BATCH_OWNS_VBO);

  // Free particles data
  free(positions);
  free(uvs);
}

void physarum_2d_handle_events(Physarum2D *p2d,
                               SpacePhysarum *sphys,
                               const bContext *C,
                               ARegion *region)
{
  // Update simulation parameters
  p2d->rotation_angle = sphys->rotation_angle;
  p2d->move_distance = sphys->move_distance;
  p2d->sensor_distance = sphys->sensor_distance;
  p2d->sensor_angle = sphys->sensor_angle;
  p2d->decay = sphys->decay_factor;

  p2d->screen_height = region->winx;
  p2d->screen_width = region->winy;

  // Compute new number of particles
  int new_nb_particles = 0;
  if (sphys->particles_population_factor > 0.0f) {
    float max_particles = p2d->max_particles - p2d->min_particles;
    new_nb_particles = p2d->min_particles + sphys->particles_population_factor * max_particles;
  } else {
    new_nb_particles = p2d->min_particles;
  }

  // If number of particles has changed, update
  if (p2d->nb_particles != new_nb_particles) {
    p2d->nb_particles = new_nb_particles;
    physarum_2d_update_particles(p2d);
  }
}
