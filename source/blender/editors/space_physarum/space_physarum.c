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

#include "GPU_framebuffer.h"
#include "GPU_immediate.h"

#include "WM_api.h"

#include "UI_resources.h"
#include "UI_view2d.h"

static enum PhysarumColor { GREEN, BLUE, RED };

/* function which is called when a new instance of the editor is created by the user */
static SpaceLink *physarum_create(const ScrArea *UNUSED(area), const Scene *UNUSED(scene))
{
  ARegion *ar;
  SpacePhysarum *sphys;

  sphys = MEM_callocN(sizeof(*sphys), "new physarum");
  sphys->spacetype = SPACE_PHYSARUM;

  /* header */
  ar = MEM_callocN(sizeof(ARegion), "header for physarum");

  BLI_addtail(&sphys->regionbase, ar);
  ar->regiontype = RGN_TYPE_HEADER;
  ar->alignment = RGN_ALIGN_BOTTOM;

  /* main region */
  ar = MEM_callocN(sizeof(ARegion), "main region of physarum");

  BLI_addtail(&sphys->regionbase, ar);
  ar->regiontype = RGN_TYPE_WINDOW;

  return (SpaceLink *)sphys;
}

/* function which initializes the header region of the editor */
static void physarum_header_region_init(wmWindowManager *UNUSED(wm), ARegion *ar)
{
  ED_region_header_init(ar);
}

/* function which initializes the main region of the editor */
static void physarum_main_region_init(wmWindowManager *UNUSED(wm), ARegion *region)
{
  UI_view2d_region_reinit(&region->v2d, V2D_COMMONVIEW_CUSTOM, region->winx, region->winy);
}

/* draw function of the main region */
static void physarum_main_region_draw(const bContext *C, ARegion *ar)
{
  SpacePhysarum *sphys = CTX_wm_space_physarum(C);
  View2D *v2d = &ar->v2d;

  switch (sphys->color) {
    case GREEN:
      GPU_clear_color(0.0, 1.0, 0.0, 1.0);
      break;
    case BLUE:
      GPU_clear_color(0.0, 0.0, 1.0, 1.0);
      break;
    case RED:
      GPU_clear_color(1.0, 0.0, 0.0, 1.0);
      break;
    default:
      GPU_clear_color(0.0, 0.0, 0.0, 1.0);
      break;
  }

  //GPU_clear(GPU_COLOR_BIT);

  /* draw colored rectangles within the mask area of region */
  uint pos = GPU_vertformat_attr_add(immVertexFormat(), "pos", GPU_COMP_I32, 2, GPU_FETCH_INT_TO_FLOAT);
  immBindBuiltinProgram(GPU_SHADER_2D_UNIFORM_COLOR);

  immUniformColor4ub(255, 0, 255, 255);
  immRecti(pos, v2d->mask.xmin + 50, v2d->mask.ymin + 50, v2d->mask.xmax - 50, v2d->mask.ymax - 50);

  immUniformColor4ub(0, 255, 255, 255);
  immRecti(pos, v2d->mask.xmin + 80, v2d->mask.ymin + 80, v2d->mask.xmax - 80, v2d->mask.ymax - 80);

  immUniformColor4ub(255, 255, 0, 255);
  immRecti(pos, v2d->mask.xmin + 110, v2d->mask.ymin + 110, v2d->mask.xmax - 110, v2d->mask.ymax - 110);

  immUnbindProgram();
}

 /* only called once, from space/spacetypes.c */
void ED_spacetype_physarum(void)
{
  SpaceType *st = MEM_callocN(sizeof(SpaceType), "spacetype physarum");
  ARegionType *art;

  st->spaceid = SPACE_PHYSARUM;
  strncpy(st->name, "Physarum", BKE_ST_MAXNAME);
  st->create = physarum_create;
  //st->operatortypes = physarum_operatortypes;

  /* regions: main window */
  art = MEM_callocN(sizeof(ARegionType), "spacetype physarum main window");
  art->regionid = RGN_TYPE_WINDOW;
  //art->init = physarum_main_region_init;
  //art->draw = physarum_main_region_draw;

  BLI_addhead(&st->regiontypes, art);

  /* regions: header */
  art = MEM_callocN(sizeof(ARegionType), "spacetype physarum header region");
  art->regionid = RGN_TYPE_HEADER;
  art->prefsizey = HEADERY;
  art->keymapflag = ED_KEYMAP_UI | ED_KEYMAP_VIEW2D | ED_KEYMAP_HEADER;
  art->init = physarum_header_region_init;
  //art->draw = physarum_headar_region_draw;

  BLI_addhead(&st->regiontypes, art);

  BKE_spacetype_register(st);
}
