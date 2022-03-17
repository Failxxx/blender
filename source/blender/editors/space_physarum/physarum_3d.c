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

void P3D_draw(Physarum3D *p3d, PhysarumRenderingSettings *prs)
{
  P3D_init(p3d, 1000, 1000);

  // ------------

  GPUTexture *texture = GPU_texture_create_2d(
      "MyTexture", p3d->texture_size, p3d->texture_size, 1, 0, GPU_RGBA32F);

  // Loading shader
  GPUShader *shader = GPU_shader_create_from_arrays({
      .vert = (const char *[]){datatoc_gpu_shader_physarum_test_vertex_vs_glsl, NULL},
      .frag = (const char *[]){datatoc_gpu_shader_physarum_test_pixel_fs_glsl, NULL},
  });

  uint vertice_position_stride = 3;
  uint color_stride = 4;

  GPUVertFormat *format = immVertexFormat();
  uint pos = GPU_vertformat_attr_add(
      format, "v_in_f3Position", GPU_COMP_F32, vertice_position_stride, GPU_FETCH_FLOAT);
  uint color = GPU_vertformat_attr_add(
      format, "v_in_f4Color", GPU_COMP_F32, color_stride, GPU_FETCH_FLOAT);

  GPUVertBuf *vertex_buffer_object = GPU_vertbuf_create_with_format(format);

  GPUFrameBuffer *frame_buffer = NULL;
  GPU_framebuffer_ensure_config(
      &frame_buffer,
      {
          GPU_ATTACHMENT_NONE,  // Slot reserved for depth/stencil buffer.
          GPU_ATTACHMENT_TEXTURE(texture),
      });

  GPUBatch *batch = GPU_batch_create_ex(
      GPU_PRIM_POINTS, vertex_buffer_object, NULL, GPU_BATCH_OWNS_VBO);
  GPU_batch_set_shader(batch, shader);

  // ------------

  GPU_texture_free(texture);
  GPU_shader_free(shader);
  GPU_batch_discard(batch);

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

  for (int i = 0; i < p3d->particules_amount; i++) {
    p3d->particles_position[i] = BLI_rng_get_float(rng);
    // p3d->particles_position[i] = 0;
    printf("Particle %d  / position : %f \n", i, p3d->particles_position[i]);
  }

  BLI_rng_free(rng);
}
