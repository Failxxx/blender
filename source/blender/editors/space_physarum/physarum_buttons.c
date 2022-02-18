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

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "DNA_armature_types.h"
#include "DNA_curve_types.h"
#include "DNA_lattice_types.h"
#include "DNA_mesh_types.h"
#include "DNA_meshdata_types.h"
#include "DNA_meta_types.h"
#include "DNA_object_types.h"
#include "DNA_scene_types.h"

#include "MEM_guardedalloc.h"

#include "BLT_translation.h"

#include "BLI_array_utils.h"
#include "BLI_bitmap.h"
#include "BLI_blenlib.h"
#include "BLI_math.h"
#include "BLI_utildefines.h"

#include "BKE_action.h"
#include "BKE_armature.h"
#include "BKE_context.h"
#include "BKE_curve.h"
#include "BKE_customdata.h"
#include "BKE_deform.h"
#include "BKE_editmesh.h"
#include "BKE_object.h"
#include "BKE_object_deform.h"
#include "BKE_report.h"
#include "BKE_screen.h"

#include "DEG_depsgraph.h"

#include "WM_api.h"
#include "WM_types.h"

#include "RNA_access.h"

#include "ED_mesh.h"
#include "ED_object.h"
#include "ED_screen.h"

#include "UI_interface.h"
#include "UI_resources.h"

#include "physarum_intern.h"

/* ******************* physarum space & buttons ************** */

enum {
  B_REDR = 2,
  B_TRANSFORM_PANEL_MEDIAN = 1008,
  B_TRANSFORM_PANEL_DIMS = 1009,
};

static void physarum_editparams_buts(uiLayout *layout, Object *ob)
{
  PointerRNA mbptr, ptr;
  uiLayout *col;

  col = uiLayoutColumn(layout, false);
  uiItemR(col, &ptr, "co", 0, NULL, ICON_NONE);

  uiItemR(col, &ptr, "radius", 0, NULL, ICON_NONE);
  uiItemR(col, &ptr, "stiffness", 0, NULL, ICON_NONE);

  uiItemR(col, &ptr, "type", 0, NULL, ICON_NONE);

  col = uiLayoutColumn(layout, true);
  uiItemL(col, IFACE_("Parameters:"), ICON_NONE);
  uiItemR(col, &ptr, "Params_A", 0, "A", ICON_NONE);
  uiItemR(col, &ptr, "Params_A", 0, "B", ICON_NONE);
  uiItemR(col, &ptr, "Params_A", 0, "C", ICON_NONE);
}

/* SUREMENT A REVOIR */

static void do_physarum_region_buttons(bContext *C, void *UNUSED(index), int event)
{
  ViewLayer *view_layer = CTX_data_view_layer(C);
  Object *ob = OBACT(view_layer);

  switch (event) {

    case B_REDR:
      ED_area_tag_redraw(CTX_wm_area(C));
      return;

    case B_TRANSFORM_PANEL_MEDIAN:
      if (ob) {
        DEG_id_tag_update(ob->data, ID_RECALC_GEOMETRY);
      }
      break;
    case B_TRANSFORM_PANEL_DIMS:
      if (ob) {
      }
      break;
  }
}

static void physarum_panel_parameters(const bContext *C, Panel *panel)
{
  uiBlock *block;
  ViewLayer *view_layer = CTX_data_view_layer(C);
  Object *ob = view_layer->basact->object;
  Object *obedit = OBEDIT_FROM_OBACT(ob);
  uiLayout *col;

  block = uiLayoutGetBlock(panel->layout);
  UI_block_func_handle_set(block, do_physarum_region_buttons, NULL);

  col = uiLayoutColumn(panel->layout, false);

  physarum_editparams_buts(col, ob);
}

static bool physarum_panel_parameters_poll(const bContext *C, PanelType *UNUSED(pt))
{
  ViewLayer *view_layer = CTX_data_view_layer(C);
  return (view_layer->basact != NULL);
}

void physarum_buttons_register(ARegionType *art)
{
  PanelType *pt;

  pt = MEM_callocN(sizeof(PanelType), "spacetype physarum panel object");
  strcpy(pt->idname, "Physarum_PT_Parameters");
  strcpy(pt->label, N_("Parameters")); 
  strcpy(pt->category, "Params");
  strcpy(pt->translation_context, BLT_I18NCONTEXT_DEFAULT_BPYRNA);
  pt->draw = physarum_panel_parameters;
  pt->poll = physarum_panel_parameters_poll;
  BLI_addtail(&art->paneltypes, pt);
}
