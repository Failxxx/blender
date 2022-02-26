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

#include "MEM_guardedalloc.h"

#include "BLI_blenlib.h"
#include "BLI_utildefines.h"

#include "BKE_context.h"
#include "BKE_screen.h"

#include "ED_screen.h"
#include "ED_space_api.h"

#include "GPU_immediate.h"
#include "GPU_immediate_util.h"
#include "GPU_capabilities.h"
#include "GPU_context.h"
#include "GPU_framebuffer.h"

#include "WM_api.h"

#include "physarum_intern.h"

float randf(const float a, const float b)
{
  return a + (float)rand() / (float)(RAND_MAX / (b - a));
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
  for (int i = 0; i < nb_points; i++) {
    float position[3] = {points[i], points[i + 1], points[i + 2]};
    float uv_coord[2] = {uvs[i], uvs[i + 1]};
    GPU_vertbuf_attr_set(vbo, pos, i, position);
    GPU_vertbuf_attr_set(vbo, uv, i, uv_coord);
  }

  return vbo;
}

/* Compute the modelViewProjectionMatrix from the given projection matrix */
void physarum_2d_compute_matrix(PhysarumData2D *pdata_2d, float projectionMatrix[4][4])
{
  float viewProjectionMatrix[4][4];
  mul_m4_m4m4(viewProjectionMatrix, projectionMatrix, pdata_2d->viewMatrix);
  mul_m4_m4m4(pdata_2d->modelViewProjectionMatrix, viewProjectionMatrix, pdata_2d->modelMatrix);
}

void physarum_2d_draw_view(PhysarumData2D *pdata_2d, float projectionMatrix[4][4])
{
  // Set shaders
  GPU_batch_set_shader(pdata_2d->diffuse_decay_batch, pdata_2d->post_process_shader);

  // Compute model view projection matrix
  physarum_2d_compute_matrix(pdata_2d, projectionMatrix);

  // Send uniforms to shaders
  GPU_batch_uniform_mat4(pdata_2d->diffuse_decay_batch,
                         "u_m4ModelViewProjectionMatrix",
                         pdata_2d->modelViewProjectionMatrix);

  // Draw vertices
  GPU_batch_draw(pdata_2d->diffuse_decay_batch);
}

void physarum_data_2d_free_particles(PhysarumData2D *pdata_2d)
{
  printf("Physarum2D: free particles\n");
  MEM_freeN(pdata_2d->particle_positions);
  MEM_freeN(pdata_2d->particle_uvs);
  MEM_freeN(pdata_2d->particle_texdata);
}

void physarum_data_2d_free_textures(PhysarumData2D *pdata_2d)
{
  printf("Physarum2D: free textures\n");
  /* Free textures */
  GPU_texture_free(pdata_2d->diffuse_decay_tex_current);
  GPU_texture_free(pdata_2d->diffuse_decay_tex_next);
  GPU_texture_free(pdata_2d->update_agents_tex_current);
  GPU_texture_free(pdata_2d->update_agents_tex_next);
  GPU_texture_free(pdata_2d->render_agents_tex);
}

void physarum_data_2d_free_shaders(PhysarumData2D *pdata_2d)
{
  printf("Physarum2D: free shaders\n");
  /* Free shaders */
  GPU_shader_free(pdata_2d->diffuse_decay_shader);
  GPU_shader_free(pdata_2d->update_agents_shader);
  GPU_shader_free(pdata_2d->render_agents_shader);
  GPU_shader_free(pdata_2d->post_process_shader);
}

void physarum_data_2d_free_batches(PhysarumData2D *pdata_2d)
{
  printf("Physarum2D: free batches\n");
  GPU_batch_discard(pdata_2d->diffuse_decay_batch);
  //GPU_batch_discard(pdata_2d->update_agents_batch);
  GPU_batch_discard(pdata_2d->render_agents_batch);
  //GPU_batch_discard(pdata_2d->post_process_batch);
}

void physarum_data_2d_gen_particles(PhysarumData2D *pdata_2d)
{
  printf("Physarum2D: gen particles\n");
  /* Allocate memory for particle data buffers */
  int size = floor(sqrt(pdata_2d->nb_particles));
  int particles_count = size * size;

  int bytes = sizeof(float) * 3 * particles_count;
  pdata_2d->particle_positions = MEM_callocN(bytes, "physarum 2d particle pos data");

  bytes = sizeof(float) * 3 * particles_count;
  pdata_2d->particle_uvs = MEM_callocN(bytes, "physarum 2d particle UVs data");

  bytes = sizeof(float) * 4 * particles_count;
  pdata_2d->particle_texdata = MEM_callocN(bytes, "physarum 2d particle texture data");

  /* Fill buffers */
  int id = 0;
  int u = 0;
  int v = 0;
  for (int i = 0; i < particles_count; ++i) {
    // Point cloud vertex
    id = i * 3;
    pdata_2d->particle_positions[id++] = 0;
    pdata_2d->particle_positions[id++] = 0;
    pdata_2d->particle_positions[id++] = 0;

    // Compute UVs
    u = (i % size) / size;
    v = ~~(i / size) / size; // ~ --> bitwise not operator (invert bits of the operand)
    id = i * 2;
    pdata_2d->particle_uvs[id++] = u;
    pdata_2d->particle_uvs[id++] = v;

    // Particle texture values (agents)
    id = i * 4;
    pdata_2d->particle_texdata[id++] = randf(0.0, 1.0); // Normalized position X
    pdata_2d->particle_texdata[id++] = randf(0.0, 1.0); // Normalized position Y
    pdata_2d->particle_texdata[id++] = randf(0.0, 1.0); // Normalized angle
    pdata_2d->particle_texdata[id++] = 1.0f;
  }
}

void physarum_data_2d_gen_textures(PhysarumData2D *pdata_2d)
{
  printf("Physarum2D: gen textures\n");
  /* Generate textures */
  // Textures sizes
  int size = floor(sqrt(pdata_2d->nb_particles));

  // Diffuse/Decay textures
  pdata_2d->diffuse_decay_tex_current = GPU_texture_create_2d(
      "phys2d diffuse decay current", size, size, 1, GPU_RGBA16F, NULL);
  pdata_2d->diffuse_decay_tex_next = GPU_texture_create_2d(
      "phys2d diffuse decay next", size, size, 1, GPU_RGBA16F, NULL);

  // Update agents textures
  pdata_2d->update_agents_tex_current = GPU_texture_create_2d(
      "phys2d update agents current", size, size, 1, GPU_RGBA16F, pdata_2d->particle_texdata);
  pdata_2d->update_agents_tex_next = GPU_texture_create_2d(
      "phys2d update agents next", size, size, 1, GPU_RGBA16F, pdata_2d->particle_texdata);

  // Render agents texture
  pdata_2d->render_agents_tex = GPU_texture_create_2d(
      "phys2d render agents", size, size, 1, GPU_RGBA16F, NULL);
}

void physarum_data_2d_gen_shaders(PhysarumData2D *pdata_2d)
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

void physarum_data_2d_gen_batches(PhysarumData2D *pdata_2d)
{
  printf("Physarum2D: gen batches\n");
  /* Generate geometry data (3d render targets) */
  int size = floor(sqrt(pdata_2d->nb_particles));
  GPUVertBuf *quad_vbo = make_new_quad_mesh();
  GPUVertBuf *points_vbo = make_new_points_mesh(
      pdata_2d->particle_positions, pdata_2d->particle_uvs, size * size);

  /* Create batches */
  pdata_2d->diffuse_decay_batch = GPU_batch_create_ex(
      GPU_PRIM_TRIS, quad_vbo, NULL, GPU_BATCH_OWNS_VBO);
  //pdata_2d->update_agents_batch = GPU_batch_create_ex(GPU_PRIM_TRIS, quad_vbo, NULL, GPU_BATCH_OWNS_VBO);
  pdata_2d->render_agents_batch = GPU_batch_create_ex(
      GPU_PRIM_POINTS, points_vbo, NULL, GPU_BATCH_OWNS_VBO);
  //pdata_2d->post_process_batch  = GPU_batch_create_ex(GPU_PRIM_TRIS, quad_vbo, NULL, GPU_BATCH_OWNS_VBO);
}

void initialize_physarum_data_2d(PhysarumData2D *pdata_2d)
{
  printf("Physarum2D: initialize data\n");
  pdata_2d->nb_particles = 512 * 512;

  const float idMatrix[4][4] = {{1.0f, 0.0f, 0.0f, 0.0f},
                                {0.0f, 1.0f, 0.0f, 0.0f},
                                {0.0f, 0.0f, 1.0f, 0.0f},
                                {0.0f, 0.0f, 0.0f, 1.0f}};

  copy_m4_m4(pdata_2d->modelViewProjectionMatrix, idMatrix);
  // Here, model and view matrices are the identiy matrix
  copy_m4_m4(pdata_2d->modelMatrix, idMatrix);
  copy_m4_m4(pdata_2d->viewMatrix, idMatrix);
  translate_m4(pdata_2d->viewMatrix, 0.0f, 0.0f, -3.0f);

  physarum_data_2d_gen_particles(pdata_2d);
  physarum_data_2d_gen_textures(pdata_2d);
  physarum_data_2d_gen_batches(pdata_2d);
  physarum_data_2d_gen_shaders(pdata_2d);
}

void free_physarum_data_2d(PhysarumData2D *pdata_2d)
{
  printf("Physarum2D: free data\n");
  physarum_data_2d_free_particles(pdata_2d);
  physarum_data_2d_free_textures(pdata_2d);
  physarum_data_2d_free_batches(pdata_2d);
  physarum_data_2d_free_shaders(pdata_2d);
}
