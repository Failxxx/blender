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

#include "WM_api.h"

#include "../interface/interface_intern.h"
#include "UI_interface.h"
#include "UI_resources.h"
#include "UI_view2d.h"

#include "physarum_intern.h"

static void physarum_init(struct wmWindowManager *wm, struct ScrArea *area)
{
  // SpacePhysarum *sphys = (SpacePhysarum *)area->spacedata.first;
}

/* Called when a new instance of the editor is created by the user */
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

  /* properties region */
  ar = MEM_callocN(sizeof(ARegion), "properties region (sidebar) for physarum");

  BLI_addtail(&sphys->regionbase, ar);
  ar->regiontype = RGN_TYPE_UI;
  ar->alignment = RGN_ALIGN_RIGHT;

  /* Initialize physarum properties */
  sphys->sense_spread = 0.48;
  sphys->sense_distance = 23.0;
  sphys->turn_angle = 0.63;
  sphys->move_distance = 2.77;
  sphys->deposit_value = 50;
  sphys->decay_factor = 0.32;
  sphys->spawn_radius = 50.0;
  sphys->center_attraction = 1.0;

  sphys->number_frame = 50;

  /* main region */
  // WARNING! Keep this here, do not move on top or bottom. Order matters.
  ar = MEM_callocN(sizeof(ARegion), "main region of physarum");

  BLI_addtail(&sphys->regionbase, ar);
  ar->regiontype = RGN_TYPE_WINDOW;

  /* Allocate memory for PhysarumRenderingSettings */
  sphys->prs = MEM_callocN(sizeof(PhysarumRenderingSettings), "physarum rendering settings");
  initialize_physarum_rendering_settings(sphys->prs);

  /* Allocate memory fo PhysarumGPUData */
  sphys->pgd = MEM_callocN(sizeof(PhysarumGPUData), "physarum gpu data");
  initialize_physarum_gpu_data(sphys->pgd);
  return (SpaceLink *)sphys;
}

static void physarum_free(SpaceLink *sl)
{
  SpacePhysarum *sphys = (SpacePhysarum *)sl;
  /* Free memory for PhysarumRenderingSettings */
  MEM_freeN(sphys->prs);
  /* Free memory for PhysarumGPUData */
  free_gpu_data(sphys);
  MEM_freeN(sphys->pgd);
}

void physarum_operatortypes(void)
{
  WM_operatortype_append(PHYSARUM_OT_single_render);
  WM_operatortype_append(PHYSARUM_OT_animation_render);
  WM_operatortype_append(PHYSARUM_OT_draw_2D);
  WM_operatortype_append(PHYSARUM_OT_draw_3D);
}

/****************** Main region ******************/

static void physarum_main_region_init(wmWindowManager *UNUSED(wm), ARegion *region)
{
  UI_view2d_region_reinit(&region->v2d, V2D_COMMONVIEW_CUSTOM, region->winx, region->winy);
}

static void physarum_main_region_draw(const bContext *C, ARegion *region)
{
  physarum_draw_view(C, region);
}

/****************** Header region ******************/

static void physarum_header_region_init(wmWindowManager *UNUSED(wm), ARegion *region)
{
  ED_region_header_init(region);
}

static void physarum_header_region_draw(const bContext *C, ARegion *region)
{
  ED_region_header(C, region);
}

/****************** Properties region ******************/

/* add handlers, stuff you only do once or on area/region changes */
static void physarum_properties_region_init(wmWindowManager *wm, ARegion *region)
{
  ED_region_panels_init(wm, region);
}

static void physarum_properties_region_draw(const bContext *C, ARegion *region)
{
  ED_region_panels(C, region);
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
  st->free = physarum_free;
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

  /* regions: sidebar */
  art = MEM_callocN(sizeof(ARegionType), "spacetype physarum sidebar region");
  art->regionid = RGN_TYPE_UI;
  art->prefsizex = UI_SIDEBAR_PANEL_WIDTH;
  art->keymapflag = ED_KEYMAP_UI | ED_KEYMAP_FRAMES;
  art->init = physarum_properties_region_init;
  art->draw = physarum_properties_region_draw;

  BLI_addhead(&st->regiontypes, art);

  /* regions: hud */
  art = ED_area_type_hud(st->spaceid);

  BLI_addhead(&st->regiontypes, art);

  BKE_spacetype_register(st);
}
