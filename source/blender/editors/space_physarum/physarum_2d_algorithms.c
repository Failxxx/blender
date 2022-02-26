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

void initialize_physarum_data_2d(PhysarumData2D *pdata_2d)
{
  pdata_2d->nb_particles = 1e4;



}

void physarum_data_2d_gen_batches(PhysarumData2D *pdata_2d)
{
  /* Load geometry */

  //// Geometry data
  //float colors[6][4] = {{UNPACK4(white)},
  //                      {UNPACK4(red)},
  //                      {UNPACK4(blue)},
  //                      {UNPACK4(white)},
  //                      {UNPACK4(red)},
  //                      {UNPACK4(blue)}};
  //float verts[6][3] = {{-1.0f, -1.0f, 0.0f},
  //                     {1.0f, -1.0f, 0.0f},
  //                     {-1.0f, 1.0f, 0.0f},  // First triangle
  //                     {1.0f, -1.0f, 0.0f},
  //                     {-1.0f, 1.0f, 0.0f},
  //                     {1.0f, 1.0f, 0.0f}};  // Second triangle
  //uint verts_len = 6;

  //// Also known as "stride" (OpenGL), specifies the space between consecutive vertex attributes
  //uint pos_comp_len = 3;
  //uint col_comp_len = 4;

  //GPUVertFormat *format = immVertexFormat();
  //uint pos = GPU_vertformat_attr_add(
  //    format, "v_in_f3Position", GPU_COMP_F32, pos_comp_len, GPU_FETCH_FLOAT);
  //uint color = GPU_vertformat_attr_add(
  //    format, "v_in_f4Color", GPU_COMP_F32, col_comp_len, GPU_FETCH_FLOAT);

  //GPUVertBuf *vbo = GPU_vertbuf_create_with_format(format);
  //GPU_vertbuf_data_alloc(vbo, verts_len);

  //// Fill the vertex buffer with vertices data
  //for (int i = 0; i < verts_len; i++) {
  //  GPU_vertbuf_attr_set(vbo, pos, i, verts[i]);
  //  GPU_vertbuf_attr_set(vbo, color, i, colors[i]);
  //}

  //pgd->batch = GPU_batch_create_ex(GPU_PRIM_TRIS, vbo, NULL, GPU_BATCH_OWNS_VBO);
}

void physarum_data_2d_gen_texture_data(PhysarumData2D *pdata_2d)
{

}

void free_physarum_data_2d(PhysarumData2D *pdata_2d)
{
  
}
