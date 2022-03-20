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

#include "GPU_capabilities.h"
#include "GPU_context.h"
#include "GPU_framebuffer.h"
#include "GPU_immediate.h"
#include "GPU_immediate_util.h"

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
  Physarum3D *p3d = sphys->p3d;
  Physarum2D *p2d = sphys->p2d;

  /* ----- Handle events ----- */
  physarum_handle_events(sphys, C, region);

  /* ----- Draw ----- */
  GPU_blend(GPU_BLEND_ALPHA);

  if(sphys->mode == SP_PHYSARUM_2D) {
    physarum_2d_handle_events(p2d, sphys, C, region);
    physarum_2d_draw_view(p2d);
  }
  else if (sphys->mode == SP_PHYSARUM_3D) {
    //physarum_3d_handle_events(p3d, sphys, C, region);
    physarum_3d_draw_view(p3d);
  }

  // Store pixels to potential export
  glReadPixels(0,
               0,
               sphys->screen_width,
               sphys->screen_height,
               GL_RGB,
               GL_UNSIGNED_BYTE,
               sphys->output_image_data);

  // Render animation
  if (sphys->rendering_mode == SP_PHYSARUM_RENDER_ANIMATION) {
    physarum_render_animation(sphys);
  }

  GPU_blend(GPU_BLEND_NONE);
}
