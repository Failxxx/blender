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
GPUVertBuf *make_new_points_mesh(float *points, float *uvs, int nb_points)
{
  /* Points contain 3D positions */

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
    float position[3] = {points[id], points[id + 1], points[id + 2]};
    id = i * 2;
    float uv_coord[2] = {uvs[id], uvs[id + 1]};
    GPU_vertbuf_attr_set(vbo, pos, i, position);
    GPU_vertbuf_attr_set(vbo, uv, i, uv_coord);
  }

  return vbo;
}

void physarum_2d_swap_textures(Physarum2D* pdata_2d)
{
  // Swap trails frame buffers
  GPUTexture *trails_tex_current_temp = pdata_2d->trails_tex_current;
  pdata_2d->trails_tex_current = pdata_2d->trails_tex_next;
  pdata_2d->trails_tex_next = trails_tex_current_temp;

  // Swap agents data frame buffers
  GPUTexture *agents_data_tex_current_temp = pdata_2d->agents_data_tex_current;
  pdata_2d->agents_data_tex_current = pdata_2d->agents_data_tex_next;
  pdata_2d->agents_data_tex_next = agents_data_tex_current_temp;

  // Attach texture to frame buffers
  GPU_framebuffer_ensure_config(&pdata_2d->trails_fb,
                                {
                                    GPU_ATTACHMENT_NONE,
                                    GPU_ATTACHMENT_TEXTURE(pdata_2d->trails_tex_next),
                                });
  GPU_framebuffer_ensure_config(&pdata_2d->agents_data_fb,
                                {
                                    GPU_ATTACHMENT_NONE,
                                    GPU_ATTACHMENT_TEXTURE(pdata_2d->agents_data_tex_next),
                                });
}

void physarum_2d_draw_view(Physarum2D *pdata_2d,
                           float projectionMatrix[4][4],
                           PhysarumRenderingSettings *prs)
{
  /* ----- Setup ----- */

  /* Get current framebuffer to be able to switch between frame buffers */
  GPUFrameBuffer *initial_fb = GPU_framebuffer_active_get();
  // Used for convenience
  GPUBatch *batch;
  GPUFrameBuffer *frameBuffer;

  // Elapsed time since the beginning of the simulation
  float time = get_elapsed_time(pdata_2d->start_time);
  // Texture sizes
  float resolution[2] = {1024.0f, 1024.0f};
  // Clear color framebuffer
  const float transparent[4] = {0.0f, 0.0f, 0.0f, 0.0f};

  /* ----- Compute trails ----- */
  {
    batch = pdata_2d->diffuse_decay_batch;
    frameBuffer = pdata_2d->trails_fb;
    // Set shader
    GPU_batch_set_shader(batch, pdata_2d->diffuse_decay_shader);

    // Send uniforms to shaders
    // Vertex shader
    GPU_batch_uniform_mat4(batch, "u_m4ProjectionMatrix", pdata_2d->projection_matrix);

    // Pixel / Fragment shader
    GPU_batch_texture_bind(batch, "u_s2TrailsData", pdata_2d->trails_tex_current);
    GPU_batch_uniform_1i(batch, "u_s2TrailsData", 0);

    GPU_batch_texture_bind(batch, "u_s2Agents", pdata_2d->agents_tex);
    GPU_batch_uniform_1i(batch, "u_s2Agents", 1);

    GPU_batch_uniform_2f(batch, "u_f2Resolution", resolution[0], resolution[1]);
    GPU_batch_uniform_1f(batch, "u_fDecay", 0.9f);

    // Render to the trails texture
    GPU_framebuffer_bind(frameBuffer);
    GPU_framebuffer_viewport_set(frameBuffer, 0, 0, (int)resolution[0], (int)resolution[1]);
    GPU_framebuffer_clear(frameBuffer, GPU_COLOR_BIT, transparent, 0, 0);  // Don't forget to clear!
    GPU_batch_draw(batch);
  }

  /* ----- Update agents ----- */
  {
    batch = pdata_2d->update_agents_batch;
    frameBuffer = pdata_2d->agents_data_fb;
    // Set shader
    GPU_batch_set_shader(batch, pdata_2d->update_agents_shader);

    // Send uniforms tho shaders
    // Vertex shader
    GPU_batch_uniform_mat4(batch, "u_m4ProjectionMatrix", pdata_2d->projection_matrix);

    // Pixel / Fragment shader
    GPU_batch_texture_bind(batch, "u_s2AgentsData", pdata_2d->agents_data_tex_current);
    GPU_batch_uniform_1i(batch, "u_s2AgentsData", 0);

    GPU_batch_texture_bind(batch, "u_s2TrailsData", pdata_2d->trails_tex_current);
    GPU_batch_uniform_1i(batch, "u_s2TrailsData", 1);

    GPU_batch_uniform_2f(batch, "u_f2Resolution", resolution[0], resolution[1]);
    GPU_batch_uniform_1f(batch, "u_fTime", time);
    GPU_batch_uniform_1f(batch, "u_fSA", 2.0f);
    GPU_batch_uniform_1f(batch, "u_fRA", 4.0f);
    GPU_batch_uniform_1f(batch, "u_fSO", 12.0f);
    GPU_batch_uniform_1f(batch, "u_fSS", 1.1f);

    // Render to the agents data texture
    GPU_framebuffer_bind(frameBuffer);
    GPU_framebuffer_viewport_set(frameBuffer, 0, 0, resolution[0], resolution[1]);
    GPU_framebuffer_clear(frameBuffer, GPU_COLOR_BIT, transparent, 0, 0);
    GPU_batch_draw(batch);
  }

  /* ----- Render agents ----- */
  {
    batch = pdata_2d->render_agents_batch;
    frameBuffer = pdata_2d->agents_fb;
    // Set shader
    GPU_batch_set_shader(batch, pdata_2d->render_agents_shader);

    // Send uniforms tho shaders
    // Vertex shader
    GPU_batch_uniform_mat4(batch, "u_m4ProjectionMatrix", pdata_2d->projection_matrix);
    GPU_batch_texture_bind(batch, "u_s2AgentsData", pdata_2d->agents_data_tex_current);
    GPU_batch_uniform_1i(batch, "u_s2AgentsData", 0);

    // Render to the agents texture
    GPU_framebuffer_bind(frameBuffer);
    GPU_framebuffer_viewport_set(frameBuffer, 0, 0, resolution[0], resolution[1]);
    GPU_framebuffer_clear(frameBuffer, GPU_COLOR_BIT, transparent, 0, 0);
    GPU_batch_draw(batch);
  }

  /* ----- Render final result using post-process ----- */
  {
    batch = pdata_2d->post_process_batch;
    frameBuffer = initial_fb;
    // Set shader
    GPU_batch_set_shader(batch, pdata_2d->post_process_shader);

    // Send uniforms tho shaders
    // Vertex shader
    GPU_batch_uniform_mat4(batch, "u_m4ProjectionMatrix", pdata_2d->projection_matrix);

    // Pixel / Fragment shader
    GPU_batch_texture_bind(batch, "u_s2TrailsData", pdata_2d->trails_tex_current);
    GPU_batch_uniform_1i(batch, "u_s2TrailsData", 0);

    // Draw final result
    GPU_framebuffer_bind(frameBuffer);
    GPU_framebuffer_clear(frameBuffer, GPU_COLOR_BIT, transparent, 0, 0);
    GPU_batch_draw(batch);
  }

  // Swap textures
  physarum_2d_swap_textures(pdata_2d);
}

void physarum_data_2d_free_particles(Physarum2D *pdata_2d)
{
  printf("Physarum2D: free particles\n");
  MEM_freeN(pdata_2d->particle_positions);
  MEM_freeN(pdata_2d->particle_uvs);
  MEM_freeN(pdata_2d->particle_texdata);
}

void physarum_data_2d_free_textures(Physarum2D *pdata_2d)
{
  printf("Physarum2D: free textures\n");
  /* Free textures */
  GPU_texture_free(pdata_2d->trails_tex_current);
  GPU_texture_free(pdata_2d->trails_tex_next);
  GPU_texture_free(pdata_2d->agents_data_tex_current);
  GPU_texture_free(pdata_2d->agents_data_tex_next);
  GPU_texture_free(pdata_2d->agents_tex);
}

void physarum_data_2d_free_shaders(Physarum2D *pdata_2d)
{
  printf("Physarum2D: free shaders\n");
  /* Free shaders */
  GPU_shader_free(pdata_2d->diffuse_decay_shader);
  GPU_shader_free(pdata_2d->update_agents_shader);
  GPU_shader_free(pdata_2d->render_agents_shader);
  GPU_shader_free(pdata_2d->post_process_shader);
}

void physarum_data_2d_free_batches(Physarum2D *pdata_2d)
{
  printf("Physarum2D: free batches\n");
  GPU_batch_discard(pdata_2d->diffuse_decay_batch);
  GPU_batch_discard(pdata_2d->update_agents_batch);
  GPU_batch_discard(pdata_2d->render_agents_batch);
  GPU_batch_discard(pdata_2d->post_process_batch);
}

void physarum_data_2d_free_frame_buffers(Physarum2D *pdata_2d)
{
  printf("Physarum2D: free frame buffers\n");
  GPU_framebuffer_free(pdata_2d->trails_fb);
  GPU_framebuffer_free(pdata_2d->agents_data_fb);
  GPU_framebuffer_free(pdata_2d->agents_fb);
}

void physarum_data_2d_gen_particles(Physarum2D *pdata_2d)
{
  printf("Physarum2D: gen particles\n");
  RNG *rng = BLI_rng_new_srandom(5831); // Arbitrary, random values generator
  /* Allocate memory for particle data buffers */
  int size = floor(sqrt(pdata_2d->nb_particles));
  int particles_count = size * size;

  int bytes = sizeof(float) * 3 * particles_count;
  pdata_2d->particle_positions = MEM_callocN(bytes, "physarum 2d particle pos data");

  bytes = sizeof(float) * 2 * particles_count;
  pdata_2d->particle_uvs = MEM_callocN(bytes, "physarum 2d particle UVs data");

  bytes = sizeof(float) * 4 * particles_count;
  pdata_2d->particle_texdata = MEM_callocN(bytes, "physarum 2d particle texture data");

  /* Fill buffers */
  int id = 0;
  float u = 0;
  float v = 0;
  for (int i = 0; i < particles_count; ++i) {
    // Point cloud vertex
    id = i * 3;
    pdata_2d->particle_positions[id++] = 0.0f;
    pdata_2d->particle_positions[id++] = 0.0f;
    pdata_2d->particle_positions[id++] = 0.0f;

    // Compute UVs
    u = (i % size) / (float)size;
    v = ~~(i / size) / (float)size; // ~ --> bitwise not operator (invert bits of the operand)
    id = i * 2;
    pdata_2d->particle_uvs[id++] = u;
    pdata_2d->particle_uvs[id++] = v;

    // Particle texture values (agents)
    id = i * 4;
    pdata_2d->particle_texdata[id++] = BLI_rng_get_float(rng);  // Normalized position X
    pdata_2d->particle_texdata[id++] = BLI_rng_get_float(rng);  // Normalized position Y
    pdata_2d->particle_texdata[id++] = BLI_rng_get_float(rng);  // Normalized angle
    pdata_2d->particle_texdata[id++] = 1.0f;
  }
  BLI_rng_free(rng);
}

void physarum_data_2d_gen_textures(Physarum2D *pdata_2d)
{
  printf("Physarum2D: gen textures\n");
  /* Generate textures */
  // Textures sizes
  int size = floor(sqrt(pdata_2d->nb_particles));
  printf("Texture size = %d\n", size);

  // Trails textures
  pdata_2d->trails_tex_current = GPU_texture_create_2d(
      "phys2d diffuse decay current", size, size, 0, GPU_RGBA32F, NULL);
  GPU_texture_filter_mode(pdata_2d->trails_tex_current, false); // Use GL_TEXTURE_MAG_FILTER = GL_NEAREST

  pdata_2d->trails_tex_next = GPU_texture_create_2d(
      "phys2d diffuse decay next", size, size, 0, GPU_RGBA32F, NULL);
  GPU_texture_filter_mode(pdata_2d->trails_tex_next, false);

  // Agents data textures
  pdata_2d->agents_data_tex_current = GPU_texture_create_2d(
      "phys2d update agents current", size, size, 0, GPU_RGBA32F, pdata_2d->particle_texdata);
  GPU_texture_filter_mode(pdata_2d->agents_data_tex_current, false);

  pdata_2d->agents_data_tex_next = GPU_texture_create_2d(
      "phys2d update agents next", size, size, 0, GPU_RGBA32F, pdata_2d->particle_texdata);
  GPU_texture_filter_mode(pdata_2d->agents_data_tex_next, false);

  // Agents texture
  pdata_2d->agents_tex = GPU_texture_create_2d(
      "phys2d render agents", size, size, 0, GPU_RGBA32F, NULL);
  GPU_texture_filter_mode(pdata_2d->agents_tex, false);
}

void physarum_data_2d_gen_shaders(Physarum2D *pdata_2d)
{
  printf("Physarum2D: load shaders\n");
  /* Load shaders */
  pdata_2d->diffuse_decay_shader = GPU_shader_create_from_arrays(
      {.vert = (const char *[]){datatoc_gpu_shader_3D_physarum_2d_quad_vs_glsl, NULL},
       .frag = (const char *[]){datatoc_gpu_shader_3D_physarum_2d_diffuse_decay_fs_glsl, NULL}});

  pdata_2d->update_agents_shader = GPU_shader_create_from_arrays(
      {.vert = (const char *[]){datatoc_gpu_shader_3D_physarum_2d_quad_vs_glsl, NULL},
       .frag = (const char *[]){datatoc_gpu_shader_3D_physarum_2d_update_agents_fs_glsl, NULL}});

  pdata_2d->render_agents_shader = GPU_shader_create_from_arrays(
      {.vert = (const char *[]){datatoc_gpu_shader_3D_physarum_2d_render_agents_vs_glsl, NULL},
       .frag = (const char *[]){datatoc_gpu_shader_3D_physarum_2d_render_agents_fs_glsl, NULL}});

  pdata_2d->post_process_shader = GPU_shader_create_from_arrays(
      {.vert = (const char *[]){datatoc_gpu_shader_3D_physarum_2d_quad_vs_glsl, NULL},
       .frag = (const char *[]){datatoc_gpu_shader_3D_physarum_2d_post_process_fs_glsl, NULL}});
}

void physarum_data_2d_gen_batches(Physarum2D *pdata_2d)
{
  printf("Physarum2D: gen batches\n");
  /* Generate geometry data (3d render targets) */
  int size = floor(sqrt(pdata_2d->nb_particles));
  GPUVertBuf *quad_vbo_1 = make_new_quad_mesh();
  GPUVertBuf *quad_vbo_2 = make_new_quad_mesh();
  GPUVertBuf *quad_vbo_3 = make_new_quad_mesh();
  GPUVertBuf *points_vbo = make_new_points_mesh(
      pdata_2d->particle_positions, pdata_2d->particle_uvs, size * size);

  /* Create batches */
  pdata_2d->diffuse_decay_batch = GPU_batch_create_ex(
      GPU_PRIM_TRIS, quad_vbo_1, NULL, GPU_BATCH_OWNS_VBO);
  pdata_2d->update_agents_batch = GPU_batch_create_ex(
      GPU_PRIM_TRIS, quad_vbo_2, NULL, GPU_BATCH_OWNS_VBO);
  pdata_2d->render_agents_batch = GPU_batch_create_ex(
      GPU_PRIM_POINTS, points_vbo, NULL, GPU_BATCH_OWNS_VBO);
  pdata_2d->post_process_batch = GPU_batch_create_ex(
      GPU_PRIM_TRIS, quad_vbo_3, NULL, GPU_BATCH_OWNS_VBO);
}

void physarum_data_2d_gen_frame_buffers(Physarum2D *pdata_2d)
{
  printf("Physarum2D: gen frame buffers\n");
  /* Generate frame buffers */
  GPU_framebuffer_ensure_config(&pdata_2d->trails_fb,
                                {
                                    GPU_ATTACHMENT_NONE,
                                    GPU_ATTACHMENT_TEXTURE(pdata_2d->trails_tex_next),
                                });
  GPU_framebuffer_ensure_config(&pdata_2d->agents_data_fb,
                                {
                                    GPU_ATTACHMENT_NONE,
                                    GPU_ATTACHMENT_TEXTURE(pdata_2d->agents_data_tex_next),
                                });
  GPU_framebuffer_ensure_config(&pdata_2d->agents_fb,
                                {
                                    GPU_ATTACHMENT_NONE,
                                    GPU_ATTACHMENT_TEXTURE(pdata_2d->agents_tex),
                                });
}

void initialize_physarum_data_2d(Physarum2D *pdata_2d)
{
  printf("Physarum2D: initialize data\n");
  pdata_2d->nb_particles = 1024 * 1024;

  orthographic_m4(pdata_2d->projection_matrix, -1.0f, 1.0f, -1.0f, 1.0f, -0.1f, 100.0f);

  physarum_data_2d_gen_particles(pdata_2d);
  physarum_data_2d_gen_textures(pdata_2d);
  physarum_data_2d_gen_batches(pdata_2d);
  physarum_data_2d_gen_shaders(pdata_2d);
  physarum_data_2d_gen_frame_buffers(pdata_2d);

  // timespec struct : time_t tv_sec, long tv_nsec
  pdata_2d->start_time = MEM_callocN(sizeof(time_t) + sizeof(long), "pysarum 2d start time");
  timespec_get(pdata_2d->start_time, TIME_UTC);
  printf("Physarum2D: initialization complete\n");
}

void free_physarum_data_2d(Physarum2D *pdata_2d)
{
  printf("Physarum2D: free data\n");
  physarum_data_2d_free_particles(pdata_2d);
  physarum_data_2d_free_textures(pdata_2d);
  physarum_data_2d_free_batches(pdata_2d);
  physarum_data_2d_free_shaders(pdata_2d);
  physarum_data_2d_free_frame_buffers(pdata_2d);
  MEM_freeN(pdata_2d->start_time);
  printf("Physarum2D: free complete\n");
}
