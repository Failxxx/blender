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
#include "GPU_matrix.h"
#include "GPU_glew.h"

#include "WM_api.h"

#include "physarum_intern.h"

void physarum_draw_view(const bContext *C, ARegion *region)
{
  SpacePhysarum *sphys = CTX_wm_space_physarum(C);
  View2D *v2d = &region->v2d;

  /* ----- Setup ----- */

  // Colors
  float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  float red[4] = {1.0f, 0.0f, 0.0f, 1.0f};
  float blue[4] = {0.0f, 0.0f, 1.0f, 1.0f};

  // Geometry data
  float colors[3][4] = {{UNPACK4(white)}, {UNPACK4(red)}, {UNPACK4(blue)}};
  float verts[3][3] = {{-0.5f, -0.5f, 0.0f}, {0.5f, -0.5f, 0.0f}, {0.0f, 0.5f, 0.0f}};
  uint verts_len = 3;
  print_v4("Colors = ", colors[0]);

  // Also known as "stride" (OpenGL), specifies the space between consecutive vertex attributes
  uint pos_comp_len = 3;
  uint col_comp_len = 4;

  GPUVertFormat *format = immVertexFormat();
  uint pos = GPU_vertformat_attr_add(
      format, "v_in_f3Position", GPU_COMP_F32, pos_comp_len, GPU_FETCH_FLOAT);
  uint color = GPU_vertformat_attr_add(
      format, "v_in_f4Color", GPU_COMP_F32, col_comp_len, GPU_FETCH_FLOAT);

  GPUVertBuf *vbo = GPU_vertbuf_create_with_format(format);
  GPU_vertbuf_data_alloc(vbo, verts_len);

  // Fill the vertex buffer with positions
  for (int i = 0; i < verts_len; i++) {
    GPU_vertbuf_attr_set(vbo, pos, i, verts[i]);
    GPU_vertbuf_attr_set(vbo, color, i, colors[i]);
  }

  /* ----- Draw ----- */
  GPU_blend(GPU_BLEND_ALPHA);

  // Init batch
  GPUBatch *batch = GPU_batch_create_ex(GPU_PRIM_TRIS, vbo, NULL, GPU_BATCH_OWNS_VBO);
  // Set shaders
  // GPU_batch_program_set_builtin(batch, GPU_SHADER_3D_FLAT_COLOR);
  // const DRWContextState *draw_ctx = DRW_context_state_get();
  // const GPUShaderConfigData *sh_cfg = &GPU_shader_cfg_data[draw_ctx->sh_cfg];
  GPUShader *shader = GPU_shader_create_from_arrays(
      {.vert = (const char *[]){datatoc_gpu_shader_3D_debug_physarum_vs_glsl, NULL},
       .frag = (const char *[]){datatoc_gpu_shader_3D_debug_physarum_fs_glsl, NULL}});
  GPU_batch_set_shader(batch, shader);

  // Compute matrices
  float modelMatrix[4][4] = {{1.0f, 0.0f, 0.0f, 0.0f},
                             {0.0f, 1.0f, 0.0f, 0.0f},
                             {0.0f, 0.0f, 1.0f, 0.0f},
                             {0.0f, 0.0f, 0.0f, 1.0f}};

  float viewMatrix[4][4] = {{1.0f, 0.0f, 0.0f, 0.0f},
                            {0.0f, 1.0f, 0.0f, 0.0f},
                            {0.0f, 0.0f, 1.0f, 0.0f},
                            {0.0f, 0.0f, 0.0f, 1.0f}};
  translate_m4(viewMatrix, 0.0f, 0.0f, -3.0f);

  float projectionMatrix[4][4] = {{1.0f, 0.0f, 0.0f, 0.0f},
                                  {0.0f, 1.0f, 0.0f, 0.0f},
                                  {0.0f, 0.0f, 1.0f, 0.0f},
                                  {0.0f, 0.0f, 0.0f, 1.0f}};

  adapt_projection_matrix_window_rescale(projectionMatrix);

  // Send uniforms to matrices
  GPU_batch_uniform_mat4(batch, "u_m4ModelMatrix", modelMatrix);
  GPU_batch_uniform_mat4(batch, "u_m4ViewMatrix", viewMatrix);
  GPU_batch_uniform_mat4(batch, "u_m4ProjectionMatrix", projectionMatrix);

  GPU_clear_color(0.227f, 0.227f, 0.227f, 1.0f);
  GPU_batch_draw(batch);

  /* ----- Free resources ----- */
  GPU_shader_free(shader);
  GPU_batch_discard(batch);

  GPU_blend(GPU_BLEND_NONE);
}

/*void initialize_rendering_settings(PRenderingSettings *prs)
{

}*/

/* Updates the projection matrix to adapt to the new aspect ration of the screen space */
void adapt_projection_matrix_window_rescale(float projectionMatrix[4][4])
{
  /* Get viewport information */
  rctf viewport;
  float viewportData[4];
  GPU_viewport_size_get_f(viewportData);
  BLI_rctf_init(&viewport, viewportData[0], viewportData[2], viewportData[1], viewportData[3]);

  /* Adapt projection matrix */
  float aspectRatio = BLI_rctf_size_x(&viewport) / BLI_rctf_size_y(&viewport);
  perspective_m4(projectionMatrix,
                 -0.5f * aspectRatio,
                 0.5f * aspectRatio,
                 -0.5f,
                 0.5f,
                 1.0f,
                 100.f);
}
