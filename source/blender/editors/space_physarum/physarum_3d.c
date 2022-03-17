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

#include "WM_api.h"

#include "physarum_intern.h"

void P3D_init(Physarum3D *p3d, int particles_amount, int texture_size)
{
  p3d->particules_amount = particles_amount;
  p3d->texture_size = texture_size;
  P3D_particles_generate(p3d);
}

// void GPU_vertbuf_bind_as_ssbo(struct GPUVertBuf *verts, int binding)

void P3D_draw(Physarum3D *p3d, PhysarumRenderingSettings *prs)
{
  P3D_init(p3d, 100, 1000);

  // Clear color framebuffer
  const float transparent[4] = {0.0f, 1.0f, 0.0f, 1.0f};

  GPUFrameBuffer *initial_fb = GPU_framebuffer_active_get();

  //-----------------------------------------------------------------------------
  //----- COMPUTE

  GPUVertBuf *vertex_buffer_object = P3D_get_display_VBO();

  // Create and send particule position and infos to GPU as SSBO
  GPUVertBuf *ssbo = P3D_get_data_VBO(p3d);
  GPU_vertbuf_bind_as_ssbo(ssbo, NULL);

  GPUTexture *texture = GPU_texture_create_2d(
      "MyTexture", p3d->texture_size, p3d->texture_size, 0, GPU_RGBA32F, NULL);

  GPUFrameBuffer *frame_buffer = NULL;
  GPU_framebuffer_ensure_config(
      &frame_buffer,
      {
          GPU_ATTACHMENT_NONE,  // Slot reserved for depth/stencil buffer.
          GPU_ATTACHMENT_TEXTURE(texture),
      });

  GPUBatch *batch = GPU_batch_create_ex(
      GPU_PRIM_TRIS, vertex_buffer_object, NULL, GPU_BATCH_OWNS_VBO);

  // Loading shader
  GPUShader *shader = GPU_shader_create_from_arrays({
      .vert = (const char *[]){datatoc_gpu_shader_physarum_test_vertex_vs_glsl, NULL},
      .frag = (const char *[]){datatoc_gpu_shader_physarum_test_pixel_fs_glsl, NULL},
  });
  GPU_batch_set_shader(batch, shader);

  GPU_framebuffer_bind(frame_buffer);
  // Set its viewport to the texture size which we want to draw on
  GPU_framebuffer_viewport_set(frame_buffer, 0, 0, p3d->texture_size, p3d->texture_size);
  // Don't forget to clear!
  GPU_framebuffer_clear(frame_buffer, GPU_COLOR_BIT, transparent, 0, 0);
  GPU_batch_draw(batch);

  //-----------------------------------------------------------------------------
  //----- RENDER ON VIEWPORT

  GPUShader *shader_render = GPU_shader_create_from_arrays({
      .vert = (const char *[]){datatoc_gpu_shader_physarum_test_vertex_vs_glsl, NULL},
      .frag = (const char *[]){datatoc_gpu_shader_physarum_test_pixel_fs_glsl, NULL},
  });
  GPU_batch_set_shader(batch, shader_render);

  GPU_framebuffer_bind(initial_fb);
  // Set its viewport to the texture size which we want to draw on
  GPU_framebuffer_viewport_set(frame_buffer, 0, 0, p3d->texture_size, p3d->texture_size);
  // Don't forget to clear!
  GPU_framebuffer_clear(initial_fb, GPU_COLOR_BIT, transparent, 0, 0);
  GPU_batch_draw(batch);
  // GPU_framebuffer_texture_detach(frame_buffer, texture);

  //-----------------------------------------------------------------------------

  GPU_texture_free(texture);
  GPU_batch_discard(batch);
  GPU_shader_free(shader);
  GPU_shader_free(shader_render);

  P3D_free(p3d);
}

void P3D_free(Physarum3D *p3d)
{
  MEM_freeN(p3d->particles_position);
}

//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------

void P3D_particles_generate(Physarum3D *p3d)
{
  printf("--- Generate Particule Data --- \n");

  RNG *rng = BLI_rng_new_srandom(5831);  // Arbitrary, random values generator
  int bytes = sizeof(float) * 3 * p3d->particules_amount;
  p3d->particles_position = MEM_callocN(bytes, "physarum 3d particle position data");

  for (int i = 0; i < p3d->particules_amount * 3; i++) {
    p3d->particles_position[i] = BLI_rng_get_float(rng);
    // p3d->particles_position[i] = 0;
    printf("Particle %d  / position : %f \n", i, p3d->particles_position[i]);
  }
  BLI_rng_free(rng);
}

// Generate geometry data for a quad mesh
GPUVertBuf *P3D_get_display_VBO()
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

// Generate VBO with Particles data ; will be send to GPU as a SSBO
GPUVertBuf *P3D_get_data_VBO(Physarum3D *p3d)
{

  // Also known as "stride" (OpenGL), specifies the space between consecutive vertex attributes
  uint pos_comp_len = 3;

  GPUVertFormat *format = immVertexFormat();
  uint pos = GPU_vertformat_attr_add(
      format, "particulePosition", GPU_COMP_F32, pos_comp_len, GPU_FETCH_FLOAT);

  GPUVertBuf *vbo = GPU_vertbuf_create_with_format(format);
  GPU_vertbuf_data_alloc(vbo, p3d->particules_amount);

  // Fill the vertex buffer with vertices data
  for (int i = 0; i < p3d->particules_amount; i += 3) {
    float position[3] = {p3d->particles_position[i],
                         p3d->particles_position[i + 1],
                         p3d->particles_position[i + 2]};
    GPU_vertbuf_attr_set(vbo, pos, i, position);
  }

  return vbo;
}

//  /* Free old data */
// GPU_batch_discard(p2d->render_agents_batch);
//
///* Generate new particles */
// int bytes = sizeof(float) * 3 * p2d->nb_particles;
// float *positions = (float *)malloc(bytes);
// bytes = sizeof(float) * 2 * p2d->nb_particles;
// float *uvs = (float *)malloc(bytes);
//
// physarum_2d_gen_particles_data(p2d, positions, uvs);
// GPUVertBuf *particles = make_new_points_mesh(positions, uvs, p2d->nb_particles);
// p2d->render_agents_batch = GPU_batch_create_ex(
//    GPU_PRIM_POINTS, particles, NULL, GPU_BATCH_OWNS_VBO);
//
//// Free particles data
// free(positions);
// free(uvs);
