/** \file
 * \ingroup spphysarum
 */

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

void physarum3DTest_draw_view(Physarum3D *physarum3d, PhysarumRenderingSettings *prs)
{
  P3T_init_GPU_data(physarum3d, prs);

  GPU_batch_set_shader(physarum3d->batch, physarum3d->shader);

  // Send uniforms to shaders
  GPU_batch_uniform_mat4(physarum3d->batch, "u_m4ModelMatrix", prs->modelMatrix);
  GPU_batch_uniform_mat4(physarum3d->batch, "u_m4ViewMatrix", prs->viewMatrix);
  GPU_batch_uniform_mat4(physarum3d->batch, "u_m4ProjectionMatrix", prs->projectionMatrix);

  // Draw vertices
  GPU_batch_draw(physarum3d->batch);

  P3T_free_GPU_data(physarum3d);
}

GPUVertBuf *P3T_get_vbo()
{
  // Colors
  float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  float red[4] = {1.0f, 0.5f, 1.0f, 1.0f};
  float green[4] = {0.0f, 1.0f, 0.0f, 1.0f};

  // Geometry data
  float colors[3][4] = {{UNPACK4(white)}, {UNPACK4(red)}, {UNPACK4(green)}};
  float vertices[3][3] = {{-0.5f, -0.5f, 0.0f}, {0.5f, -0.5f, 0.0f}, {0.0f, 0.5f, 0.0f}};
  uint vertices_length = 3;

  // Also known as "stride" (OpenGL), specifies the space between consecutive vertex attributes
  uint vertice_position_stride = 3;
  uint color_stride = 4;

  GPUVertFormat *format = immVertexFormat();
  uint pos = GPU_vertformat_attr_add(
      format, "v_in_f3Position", GPU_COMP_F32, vertice_position_stride, GPU_FETCH_FLOAT);
  uint color = GPU_vertformat_attr_add(
      format, "v_in_f4Color", GPU_COMP_F32, color_stride, GPU_FETCH_FLOAT);

  GPUVertBuf *vertex_buffer_object = GPU_vertbuf_create_with_format(format);

  GPU_vertbuf_data_alloc(vertex_buffer_object, vertices_length);

  // Fill the vertex buffer with vertices data
  for (int i = 0; i < vertices_length; i++) {
    GPU_vertbuf_attr_set(vertex_buffer_object, pos, i, vertices[i]);
    GPU_vertbuf_attr_set(vertex_buffer_object, color, i, colors[i]);
  }
  return vertex_buffer_object;
}

void P3T_init_GPU_data(Physarum3D *physarum3d)
{
  /* Load shaders */
  physarum3d->shader = GPU_shader_create_from_arrays(
      {.vert = (const char *[]){datatoc_gpu_shader_physarum_test_vertex_vs_glsl, NULL},
       .frag = (const char *[]){datatoc_gpu_shader_physarum_test_pixel_fs_glsl, NULL}});

  GPUVertBuf *vertex_buffer_object = P3T_get_vbo();

   //physarum3d->batch = GPU_batch_create_ex(
   //   GPU_PRIM_TRIS, vertex_buffer_object, NULL, GPU_BATCH_OWNS_VBO);

   physarum3d->batch = GPU_batch_create_ex(
      GPU_PRIM_POINTS, vertex_buffer_object, NULL, GPU_BATCH_OWNS_VBO);
}

void P3T_free_GPU_data(Physarum3D *physarum3d)
{
  // GPU_texture_free(physarum3d->texture);
  GPU_shader_free(physarum3d->shader);
  GPU_batch_discard(physarum3d->batch);
}

/*

float vertices[3][3] = {{-0.5f, -0.5f, 0.0f}, {0.5f, -0.5f, 0.0f}, {0.0f, 0.5f, 0.0f}};
uint vertices_length = 6;
// Also known as "stride" (OpenGL), specifies the space between consecutive vertex attributes
uint vertices_pos_stride = 3;

float texture_coordinates[3][2] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {0.5f, 1.0f}};
uint texture_pos_stride = 2;

float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
float red[4] = {1.0f, 0.0f, 0.0f, 1.0f};
float blue[4] = {0.0f, 0.0f, 1.0f, 1.0f};

// Geometry data
float colors[3][4] = {{UNPACK4(white)}, {UNPACK4(red)}, {UNPACK4(blue)}};
uint color_stride = 4;

GPUVertFormat *format = immVertexFormat();
uint vertice_coordinates_attribute = GPU_vertformat_attr_add(
  format, "v_in_f3Position", GPU_COMP_F32, vertices_pos_stride, GPU_FETCH_FLOAT);

uint color_attribute = GPU_vertformat_attr_add(
  format, "v_in_f4Color", GPU_COMP_F32, color_stride, GPU_FETCH_FLOAT);

// uint texture_coordinates_attribute = GPU_vertformat_attr_add(
//    format, "v_in_f2UV", GPU_COMP_F32, texture_pos_stride, GPU_FETCH_FLOAT);

// Vertex buffers contain data and attributes inside vertex buffers should match the attributes
// of the shader. Before a buffer can be created, the format of the buffer should be defined.
GPUVertBuf *vertex_buffer_object = GPU_vertbuf_create_with_format(format);

// Allocate the memory space on GPU for the vertices into the vertex buffer
GPU_vertbuf_data_alloc(vertex_buffer_object, vertices_length);

// Fill the vertex buffer with vertices data
for (int i = 0; i < vertices_length; i++) {
GPU_vertbuf_attr_set(vertex_buffer_object, vertice_coordinates_attribute, i, vertices[i]);
GPU_vertbuf_attr_set(vertex_buffer_object, color_attribute, i, colors[i]);
// GPU_vertbuf_attr_set(
// vertex_buffer_object, texture_coordinates_attribute, i, texture_coordinates[i]);
}

// Creating texture
const int TEXTURE_SIZE = 1080;
physarum3d->texture = GPU_texture_create_2d(
  "MyTexture", TEXTURE_SIZE, TEXTURE_SIZE, 1, 0, GPU_RGBA32F);

// A frame buffer is a group of textures you can render onto. The first slot is reserved for a
// depth/stencil buffer. The other slots can be filled with regular textures, cube maps, or layer
// textures.
physarum3d->frame_buffer = NULL;
GPU_framebuffer_ensure_config(
  &physarum3d->frame_buffer,
  {
      GPU_ATTACHMENT_NONE,  // Slot reserved for depth/stencil buffer.
      GPU_ATTACHMENT_TEXTURE(physarum3d->texture),
  });

//Get current framebuffer to be able to switch between frame buffers
// GPUFrameBuffer *initial_fb = GPU_framebuffer_active_get();

// Loading shader
physarum3d->shader = GPU_shader_create_from_arrays({
  .vert = (const char *[]){datatoc_gpu_shader_physarum_test_vertex_vs_glsl, NULL},
  .frag = (const char *[]){datatoc_gpu_shader_physarum_test_pixel_fs_glsl, NULL},
});

// Create GPU Batch
physarum3d->batch = GPU_batch_create_ex(
  GPU_PRIM_TRIS, vertex_buffer_object, NULL, GPU_BATCH_OWNS_VBO);
GPU_batch_set_shader(physarum3d->batch, physarum3d->shader);

GPU_batch_uniform_mat4(physarum3d->batch, "u_m4ModelMatrix", prs->modelMatrix);
GPU_batch_uniform_mat4(physarum3d->batch, "u_m4ViewMatrix", prs->viewMatrix);
GPU_batch_uniform_mat4(physarum3d->batch, "u_m4ProjectionMatrix", prs->projectionMatrix);

// Clear color framebuffer
const float transparent[4] = {0.0f, 0.0f, 0.0f, 0.0f};

GPU_framebuffer_bind(physarum3d->frame_buffer);
// Set its viewport to the texture size which we want to draw on
GPU_framebuffer_viewport_set(physarum3d->frame_buffer, 0, 0, TEXTURE_SIZE, TEXTURE_SIZE);
// Don't forget to clear!
GPU_framebuffer_clear(physarum3d->frame_buffer, GPU_COLOR_BIT, transparent, 0, 0);

GPU_batch_draw(physarum3d->batch);

GPU_framebuffer_texture_detach(physarum3d->frame_buffer, physarum3d->texture);

printf("Physarum3DTest: free texture, batch, shader\n");
GPU_texture_free(physarum3d->texture);
GPU_batch_discard(physarum3d->batch);
GPU_shader_free(physarum3d->shader); */



// void PHYS3D_init(Physarum3D *physarum3d, PhysarumRenderingSettings *prs)
//{
//  prs->world_depth = prs->world_height = prs->world_width = 500;
//  physarum3d->spawn_radius = 50.0f;
//  physarum3d->particules_amount = 5000;
//
//  /*PHYS3D_set_particles(physarum3d->particles,
//                       physarum3d->particules_amount,
//                       prs->world_depth,
//                       physarum3d->spawn_radius);*/
//}
//
// void PHYS3D_exit(Physarum3D *physarum3d, PhysarumRenderingSettings *prs)
//{
//}
//
//// Called by the blender view each frame
// void PHYS3D_draw_view(Physarum3D *physarum3d, PhysarumRenderingSettings *prs)
//{
//  PHYS3D_init_GPU();
//
//  GPUVertBuf *vbo = PHYS3D_get_VBO();
//
//  PHYS3D_free_GPU();
//}
//
//// Return a vertex buffer objet for the GPUBatch
// GPUVertBuf *PHYS3D_get_VBO()
//{
//  return;
//}
//
//// Initialise all data to GPU
// GPUVertBuf *PHYS3D_init_GPU()
//{
//  return;
//}
//
//// Free all memory on the GPU
// GPUVertBuf *PHYS3D_free_GPU()
//{
//  return;
//}

// Set all the parameters
// void PHYS3D_set_particles(Physarum3DParticle *particles[],
//                          uint particules_count,
//                          float world_size,
//                          float spawn_radius)
//{
//  const float PI2 = 6.28318530f;
//
//  for (int i = 0; i < particules_count; ++i) {
//    float phi = PHYS3D_random_uniform(0, PI2);
//    float theta = acos(2.0 * PHYS3D_random_uniform(0.0f, 1.0f) - 1.0f);
//    float radius = PHYS3D_random_uniform(0.0f, 1.0f);
//    radius = pow(radius, 1.0f / 3.0f) * spawn_radius;
//
//    particles[i]->x = sin(phi) * sin(theta) * radius + world_size / 2.0f;
//    particles[i]->y = cos(theta) * radius + world_size / 2.0f;
//    particles[i]->z = cos(phi) * sin(theta) * radius + world_size / 2.0f;
//    particles[i]->phi = acos(2.0 * PHYS3D_random_uniform(0.0f, 1.0) - 1.0f);
//    particles[i]->theta = PHYS3D_random_uniform(0.0f, PI2);
//    particles[i]->pair = 100000000;  // particle ID 100 million means no pair.
//  }
//}

// SEE BLI For random-uniform
// float PHYS3D_random_uniform(float low, float high)
//{
//  float normalized = (rand() % 10000) / 10000.0f;
//  float result = normalized * (high - low) + low;
//  return result;
//}

// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------

// https://wiki.blender.org/wiki/Source/EEVEE_%26_Viewport/GPU_Module

/*1. add dans source shaders physarum + cmake
build

build bf dna when chnage in space

*/

//---------------------------------------------------------------------
//-------- Simulation params
//---------------------------------------------------------------------

// uint32_t world_width = 500, world_height = 500, world_depth = 500;
// float spawn_radius = 50.0f;
// const int NUM_PARTICLES = 100000;

// TODO: How to get the value from the interface ?

//---------------------------------------------------------------------
//--------
//---------------------------------------------------------------------

//// Return a vertex buffer objet for the GPUBatch
// GPUVertBuf *physarum3D_make_new_quad_mesh()
//{
//  // Create the vertices array for a cube ?? shown in physarum render
//  float quad_vertices[12][3] = {
//      {-1.0f, -1.0f, 0.0f},
//      {1.0f, 0.0f, 0.0f},
//      {1.0f, 1.0f, 0.0f},
//      {1.0f, 1.0f, 1.0f},
//      {-1.0f, 1.0f, 0.0f},
//      {1.0f, 0.0f, 1.0f},
//      {-1.0f, -1.0f, 0.0f},
//      {1.0f, 0.0f, 0.0f},
//      {1.0f, -1.0f, 0.0f},
//      {1.0f, 1.0f, 0.0f},
//      {1.0f, 1.0f, 0.0f},
//      {1.0f, 1.0f, 1.0f},
//  };
//  uint quad_vertices_len = 12;
//
//  // Also known as "stride" (OpenGL), specifies the space between consecutive vertex attributes
//  uint quad_vertices_stride = sizeof(float) * 6;
//  uint quad_vertices_count = 6;
//
//  float uvs[6][2] = {
//      {0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}};
//
//  uint pos_comp_len = 3;
//  uint uvs_comp_len = 2;
//
//  GPUVertFormat *format = immVertexFormat();
//  uint pos = GPU_vertformat_attr_add(
//      format, "v_in_f3Position", GPU_COMP_F32, quad_vertices_stride, GPU_FETCH_FLOAT);
//  uint uv = GPU_vertformat_attr_add(
//      format, "v_in_f2UV", GPU_COMP_F32, uvs_comp_len, GPU_FETCH_FLOAT);
//
//  GPUVertBuf *vertex_buffer_object = GPU_vertbuf_create_with_format(format);
//  GPU_vertbuf_data_alloc(vertex_buffer_object, quad_vertices_len);
//
//  // Fill the vertex buffer with vertices data
//  for (int i = 0; i < quad_vertices_len; i++) {
//    GPU_vertbuf_attr_set(vertex_buffer_object, pos, i, quad_vertices[i]);
//    GPU_vertbuf_attr_set(vertex_buffer_object, uv, i, uvs[i]);
//  }
//  return vertex_buffer_object;
//}
//
// void physarum3D_init_data()
//{
//}
//
// void physarum3D_draw_view(float projectionMatrix[4][4],
//                          PhysarumGPUData *pgd,
//                          PhysarumRenderingSettings *prs)
//{
// GPU_batch_set_shader(pgd->batch, pgd->shader);

//// Send uniforms to shaders
// GPU_batch_uniform_mat4(pgd->batch, "u_m4ModelMatrix", prs->modelMatrix);
// GPU_batch_uniform_mat4(pgd->batch, "u_m4ViewMatrix", prs->viewMatrix);
// GPU_batch_uniform_mat4(pgd->batch, "u_m4ProjectionMatrix", prs->projectionMatrix);

//// Draw vertices
// GPU_batch_draw(pgd->batch);

// uint32_t world_width = 500, world_height = 500, world_depth = 500;
//  uint32_t world_size = 500;
//  float spawn_radius = 50.0f;
//  const int NUM_PARTICLES = 1000;
//
//  // Get current framebuffer to be able to switch between frame buffers
//  GPUFrameBuffer *initial_fb = GPU_framebuffer_active_get();
//  GPUFrameBuffer *frameBuffer;
//  GPUBatch *batch;
//
//  GPUTexture *texture_trail_A = GPU_texture_create_3d(
//      "trail_A", world_size, world_size, world_size, 1, 0, GPU_RGBA32F, NULL);
//  GPUTexture *texture_trail_B = GPU_texture_create_3d(
//      "trail_B", world_size, world_size, world_size, 1, 0, GPU_RGBA32F, NULL);
//
//  GPUTexture *texture_display = GPU_texture_create_2d(
//      "texture_display", world_size, world_size, 1, 0, GPU_RGBA32F, NULL);
//  GPUTexture *texture_display_int = GPU_texture_create_2d(
//      "texture_display_int", world_size, world_size, 1, 0, GPU_RGBA32F, NULL);
//
//  GPU_framebuffer_ensure_config(
//      &frameBuffer,
//      {
//          GPU_ATTACHMENT_NONE,  // Slot reserved for depth/stencil buffer.
//          GPU_ATTACHMENT_TEXTURE(texture_trail_A),
//          GPU_ATTACHMENT_TEXTURE(texture_trail_B),
//          GPU_ATTACHMENT_TEXTURE(texture_display),
//          GPU_ATTACHMENT_TEXTURE(texture_display_int),
//      });
//
//  GPUShader *shader_2d = GPU_shader_create_from_arrays({
//      .vert = (const char *[]){datatoc_gpu_shader_physarum_vertex_vs_glsl, NULL},
//      .frag = (const char *[]){datatoc_gpu_shader_physarum_pixel_fs_glsl, NULL},
//  });
//
//  GPUShader *shader_3d = GPU_shader_create_from_arrays({
//      .vert = (const char *[]){datatoc_gpu_shader_physarum_vertex_3d_vs_glsl, NULL},
//      .frag = (const char *[]){datatoc_gpu_shader_physarum_pixel_3d_fs_glsl, NULL},
//  });
//
//  // printf("Physarum2D: free particles\n");
//  GPU_texture_free(texture_trail_A);
//  GPU_texture_free(texture_trail_B);
//  GPU_texture_free(texture_display);
//  GPU_texture_free(texture_display_int);
//
//  GPU_batch_discard(batch);
//
//  GPU_shader_free(shader_2d);
//  GPU_shader_free(shader_3d);
//}
//
//// Free the memory in CPU and GPU used by physarum
// void physarum3D_free_data()
//{
//}
