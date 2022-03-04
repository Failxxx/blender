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

#include "glew-mx.h"

#include "physarum_intern.h"

void physarum_draw_view(const bContext *C, ARegion *region)
{
  SpacePhysarum *sphys = CTX_wm_space_physarum(C);
  PhysarumRenderingSettings *prs = sphys->prs;
  PhysarumGPUData *pgd = sphys->pgd;

  /* ----- Setup ----- */
  prs->screen_width  = BLI_rcti_size_x(&region->winrct);
  prs->screen_height = BLI_rcti_size_y(&region->winrct);
  adapt_projection_matrix_window_rescale(prs);

  /* ----- Draw ----- */
  GPU_blend(GPU_BLEND_ALPHA);

  // Set shaders
  GPU_batch_set_shader(pgd->batch, pgd->shader);

  // Send uniforms to shaders
  GPU_batch_uniform_mat4(pgd->batch, "u_m4ModelMatrix", prs->modelMatrix);
  GPU_batch_uniform_mat4(pgd->batch, "u_m4ViewMatrix", prs->viewMatrix);
  GPU_batch_uniform_mat4(pgd->batch, "u_m4ProjectionMatrix", prs->projectionMatrix);

  // Draw vertices
  GPU_clear_color(0.227f, 0.227f, 0.227f, 1.0f);
  GPU_batch_draw(pgd->batch);

  // Pixel for frame export
  sphys->image_data = (unsigned char *)malloc((int)(prs->screen_width * prs->screen_height * (3)));
  glReadPixels(0, 0, prs->screen_width, prs->screen_height, GL_RGB, GL_UNSIGNED_BYTE, sphys->image_data);

  if (sphys->counter_rendering_frame > 0) {
    PHYSARUM_animation_frame_render(C);
  }

  GPU_blend(GPU_BLEND_NONE);
}

/* Updates the projection matrix to adapt to the new aspect ration of the screen space */
void adapt_projection_matrix_window_rescale(PRenderingSettings *prs)
{
  /* Adapt projection matrix */
  float aspectRatio = prs->screen_width / prs->screen_height;
  perspective_m4(
      prs->projectionMatrix, -0.5f * aspectRatio, 0.5f * aspectRatio, -0.5f, 0.5f, 1.0f, 1000.0f);
}

/* Initializes the PhysarumRenderingSettings struct with default values */
void initialize_physarum_rendering_settings(PRenderingSettings *prs)
{
  const float idMatrix[4][4] = {{1.0f, 0.0f, 0.0f, 0.0f},
                                {0.0f, 1.0f, 0.0f, 0.0f},
                                {0.0f, 0.0f, 1.0f, 0.0f},
                                {0.0f, 0.0f, 0.0f, 1.0f}};

  prs->texcoord_map = 0; // Default value ?
  prs->show_grid = 0; // Default value ?
  prs->dof_size = 0.1f;
  prs->dof_distribution = 1.0f;

  prs->focal_distance = 2.0f;
  prs->focal_depth = 1.0f;
  prs->iterations = 1;
  prs->break_distance = 10.0f;

  prs->world_width = 480.0f;
  prs->world_height = 480.0f;
  prs->world_depth = 480.0f;
  prs->screen_width = 1280.0f;
  prs->screen_height = 720.0f;

  prs->sample_weight = 1.0f / 32.0f;

  prs->filler1 = 0; // Default value ?
  prs->filler2 = 1; // Default value ?

  // Projection matrix
  adapt_projection_matrix_window_rescale(prs);
  // Model matrix
  copy_m4_m4(prs->modelMatrix, idMatrix);
  // View matrix
  copy_m4_m4(prs->viewMatrix, idMatrix);
  translate_m4(prs->viewMatrix, 0.0f, 0.0f, -3.0f);
}

/* Initializes GPU data (VBOs and shaders) */
void initialize_physarum_gpu_data(PhysarumGPUData *pgd)
{
  /* Load shaders */
  pgd->shader = GPU_shader_create_from_arrays({
    .vert = (const char *[]){datatoc_gpu_shader_3D_debug_physarum_vs_glsl, NULL},
    .frag = (const char *[]){datatoc_gpu_shader_3D_debug_physarum_fs_glsl, NULL}
  });

  /* Load geometry */
  // Colors
  float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  float red[4] = {1.0f, 0.0f, 0.0f, 1.0f};
  float blue[4] = {0.0f, 0.0f, 1.0f, 1.0f};

  // Geometry data
  float colors[3][4] = {{UNPACK4(white)}, {UNPACK4(red)}, {UNPACK4(blue)}};
  float verts[3][3] = {{-0.5f, -0.5f, 0.0f}, {0.5f, -0.5f, 0.0f}, {0.0f, 0.5f, 0.0f}};
  uint verts_len = 3;

  // Also known as "stride" (OpenGL), specifies the space between consecutive vertex attributes
  uint pos_comp_len = 3;
  uint col_comp_len = 4;

  GPUVertFormat *format = immVertexFormat();
  uint pos = GPU_vertformat_attr_add(format, "v_in_f3Position", GPU_COMP_F32, pos_comp_len, GPU_FETCH_FLOAT);
  uint color = GPU_vertformat_attr_add(format, "v_in_f4Color", GPU_COMP_F32, col_comp_len, GPU_FETCH_FLOAT);

  GPUVertBuf *vbo = GPU_vertbuf_create_with_format(format);
  GPU_vertbuf_data_alloc(vbo, verts_len);

  // Fill the vertex buffer with vertices data
  for (int i = 0; i < verts_len; i++) {
    GPU_vertbuf_attr_set(vbo, pos, i, verts[i]);
    GPU_vertbuf_attr_set(vbo, color, i, colors[i]);
  }

  pgd->batch = GPU_batch_create_ex(GPU_PRIM_TRIS, vbo, NULL, GPU_BATCH_OWNS_VBO);
}


/* Free memory of the PhysarumGPUData strucure */
void free_gpu_data(SpacePhysarum *sphys)
{
  GPU_shader_free(sphys->pgd->shader);
  GPU_batch_discard(sphys->pgd->batch);
}
