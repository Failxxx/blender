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

void physarum_handle_events(SpacePhysarum *sphys, const bContext *C, ARegion *region)
{
  // Screen resize
  if (region->winx != sphys->screen_width || region->winy != sphys->screen_height) {
    sphys->screen_width = region->winx;
    sphys->screen_height = region->winy;

    // Free old output_image_data
    if (sphys->output_image_data != NULL)
      free(sphys->output_image_data);
    // Reallocate memory for output_image_data
    int bytes = sizeof(unsigned char) * region->winx * region->winy * 3;
    sphys->output_image_data = (unsigned char *)malloc(bytes);
  }
}

void physarum_draw_view(const bContext *C, ARegion *region)
{
  SpacePhysarum *sphys = CTX_wm_space_physarum(C);
  Physarum2D *p2d = sphys->p2d;

  /* ----- Handle events ----- */
  physarum_handle_events(sphys, C, region);

  /* ----- Draw ----- */
  GPU_blend(GPU_BLEND_ALPHA);

  // Background color
  GPU_clear_color(0.227f, 0.227f, 0.227f, 1.0f);

  if(sphys->mode == SP_PHYSARUM_2D) {
    physarum_2d_draw_view(p2d);
    physarum_2d_handle_events(p2d, sphys, C, region);
  }

  // Store pixels to potential export
  glReadPixels(0,
               0,
               sphys->screen_width,
               sphys->screen_height,
               GL_RGB,
               GL_UNSIGNED_BYTE,
               sphys->output_image_data);

  if (sphys->render_frames_counter > 0) {
    PHYSARUM_animation_frame_render(C);
  }

  GPU_blend(GPU_BLEND_NONE);
}

/* Updates the projection matrix to adapt to the new aspect ration of the screen space */
void adapt_projection_matrix_window_rescale(PRenderingSettings *prs)
{
  /* Adapt projection matrix */
  //float aspectRatio = prs->screen_width / prs->screen_height;
  float aspectRatio = 1.0f;
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

GPUVertBuf *make_new_triangle_mesh()
{
  /* Load geometry */
  // Colors
  float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  float red[4] = {1.0f, 0.0f, 0.0f, 1.0f};
  float blue[4] = {0.0f, 0.0f, 1.0f, 1.0f};

  // Geometry data
  float colors[6][4] = {{UNPACK4(white)},
                        {UNPACK4(red)},
                        {UNPACK4(blue)},
                        {UNPACK4(white)},
                        {UNPACK4(red)},
                        {UNPACK4(blue)}};
  float verts[6][3] = {{-1.0f, -1.0f, 0.0f},
                       {1.0f, -1.0f, 0.0f},
                       {-1.0f, 1.0f, 0.0f},  // First triangle
                       {1.0f, -1.0f, 0.0f},
                       {-1.0f, 1.0f, 0.0f},
                       {1.0f, 1.0f, 0.0f}};  // Second triangle
  uint verts_len = 6;

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

  // Fill the vertex buffer with vertices data
  for (int i = 0; i < verts_len; i++) {
    GPU_vertbuf_attr_set(vbo, pos, i, verts[i]);
    GPU_vertbuf_attr_set(vbo, color, i, colors[i]);
  }

  return vbo;
}
