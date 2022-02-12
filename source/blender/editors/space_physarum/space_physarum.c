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
#include "GPU_framebuffer.h"

#include "WM_api.h"

#include "UI_interface.h"
#include "UI_resources.h"
#include "UI_view2d.h"
#include "../interface/interface_intern.h"

#include "physarum_intern.h"

static void physarum_init(struct wmWindowManager *wm, struct ScrArea *sa)
{
  SpacePhysarum *sphys = (SpacePhysarum *)sa->spacedata.first;
}

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

  /* ui region */
  ar = MEM_callocN(sizeof(ARegion), "ui region of physarum");
  BLI_addtail(&sphys->regionbase, ar);
  ar->regiontype = RGN_TYPE_UI;

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

/* function which initializes the ui region of the editor */
static void physarum_ui_region_init(wmWindowManager *wm, ARegion *ar)
{
  ED_region_panels_init(wm, ar);
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

static void draw_buttons(uiBlock *block, uiLayout *layout)
{
  static int value = 100;

  struct wmOperatorType *ot = WM_operatortype_find("SPACE_PHYSARUM_OT_red_region", true);
  struct uiBut *but = uiDefBut(block, UI_BTYPE_BUT_TOGGLE, 1, "RED", 100, 2, 30, 19, (void*)&value, 0., 0., 0., 0., "");
  but->optype = ot;

  ot = WM_operatortype_find("SPACE_PHYSARUM_OT_green_region", true);
  but = uiDefBut(block, UI_BTYPE_BUT_TOGGLE, 1, "GREEN", 200, 5, 75, 19, (void*)&value, 0., 0., 0., 0., "");
  but->optype = ot;

  ot = WM_operatortype_find("SPACE_PHYSARUM_OT_blue_region", true);
  but = uiDefBut(block, UI_BTYPE_BUT_TOGGLE, 1, "BLUE", 300, 5, 75, 19, (void *)&value, 0., 0., 0., 0., "");
  but->optype = ot;
}

static void physarum_header_region_draw(const bContext *C, ARegion *ar)
{
  uiStyle *style = UI_style_get_dpi();
  uiBlock *block;
  uiLayout *layout;
  bool region_layout_based = ar->flag & RGN_FLAG_DYNAMIC_SIZE;

  /* Height of buttons and scaling needed to achieve it */
  const int buttony = min_ii(UI_UNIT_Y, ar->winy - 2 * UI_DPI_FAC);
  const float buttony_scale = buttony / (float)UI_UNIT_Y;

  /* Vertically center button */
  int xco = UI_HEADER_OFFSET;
  int yco = buttony + (ar->winy - buttony) / 2;
  int maxco = xco;

  /* Set view2d view matrix for scrolling (without scrollers) */
  UI_view2d_view_ortho(&ar->v2d);

  block = UI_block_begin(C, ar, "", UI_EMBOSS);
  layout = UI_block_layout(block, UI_LAYOUT_HORIZONTAL, UI_LAYOUT_HEADER, xco, yco, buttony, 1, 0, style);

  if (buttony_scale != 1.0f) {
    uiLayoutSetScaleY(layout, buttony_scale);
  }

  /* Buttons */
  draw_buttons(block, layout);

  UI_block_layout_resolve(block, &xco, &yco);

  /* For view2d */
  if (xco > maxco) {
    maxco = xco;
  }

  int new_sizex = (maxco + UI_HEADER_OFFSET) / UI_DPI_FAC;

  if (region_layout_based && (ar->sizex != new_sizex)) {
    /* region size is layout based and needs to be updated */
    ScrArea *sa = CTX_wm_area(C);

    ar->sizex = new_sizex;
    sa->flag |= AREA_FLAG_REGION_SIZE_UPDATE;
  }

  UI_block_end(C, block);

  if (!region_layout_based) {
    maxco += UI_HEADER_OFFSET;
  }

  /* always at last */
  UI_view2d_totRect_set(&ar->v2d, maxco, ar->winy);

  /* restore view matrix */
  UI_view2d_view_restore(C);

  /* clear */
  UI_ThemeClearColor(TH_HEADER);
  //GPU_clear(GPU_COLOR_BIT);

  UI_view2d_view_ortho(&ar->v2d);

  /* View2D matrix might have changed due to dynamic sized regions */
  UI_blocklist_update_window_matrix(C, &ar->uiblocks);

  /* draw blocks */
  UI_blocklist_draw(C, &ar->uiblocks);

  /* restore view matrix */
  UI_view2d_view_restore(C);
}

void physarum_operatortypes(void)
{
  WM_operatortype_append(SPACE_PHYSARUM_OT_red_region);
  WM_operatortype_append(SPACE_PHYSARUM_OT_green_region);
  WM_operatortype_append(SPACE_PHYSARUM_OT_blue_region);
}

 /* only called once, from space/spacetypes.c */
void ED_spacetype_physarum(void)
{
  SpaceType *st = MEM_callocN(sizeof(SpaceType), "spacetype physarum");
  ARegionType *art;

  st->spaceid = SPACE_PHYSARUM;
  strncpy(st->name, "Physarum", BKE_ST_MAXNAME);
  st->init = physarum_init;
  st->create = physarum_create;
  st->operatortypes = physarum_operatortypes;

  /* regions: main window */
  art = MEM_callocN(sizeof(ARegionType), "spacetype physarum main window");
  art->regionid = RGN_TYPE_WINDOW;
  art->init = physarum_main_region_init;
  art->draw = physarum_main_region_draw;

  BLI_addhead(&st->regiontypes, art);

  /* regions: header */
  art = MEM_callocN(sizeof(ARegionType), "spacetype physarum header region");
  art->regionid = RGN_TYPE_HEADER;
  art->prefsizey = HEADERY;
  art->keymapflag = ED_KEYMAP_UI | ED_KEYMAP_VIEW2D | ED_KEYMAP_HEADER;
  art->init = physarum_header_region_init;
  art->draw = physarum_header_region_draw;

  BLI_addhead(&st->regiontypes, art);

  /* regions: header */
  art = MEM_callocN(sizeof(ARegionType), "spacetype physarum buttons region");
  art->regionid = RGN_TYPE_UI;
  art->keymapflag = ED_KEYMAP_UI | ED_KEYMAP_VIEW2D | ED_KEYMAP_HEADER;
  art->init = physarum_ui_region_init;
  //art->draw = physarum_ui_region_draw;
  art->prefsizex = UI_SIDEBAR_PANEL_WIDTH;
  art->keymapflag = ED_KEYMAP_UI | ED_KEYMAP_FRAMES;

  BLI_addhead(&st->regiontypes, art);

  BKE_spacetype_register(st);
}
