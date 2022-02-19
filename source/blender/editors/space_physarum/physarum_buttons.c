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

static void physarum_panel_properties_header(const bContext *C, Panel *panel)
{
  bScreen *screen = CTX_wm_screen(C);
  SpacePhysarum *sphys = CTX_wm_space_physarum(C);
  Scene *scene = CTX_data_scene(C);
  PointerRNA spaceptr, sceneptr;
  uiLayout *col;

  /* get RNA pointers for use when creating the UI elements */
  RNA_id_pointer_create(&scene->id, &sceneptr);
  RNA_pointer_create(&screen->id, &RNA_SpaceGraphEditor, sphys, &spaceptr);

  /* 2D-Cursor */
  col = uiLayoutColumn(panel->layout, false);
  uiItemR(col, &spaceptr, "Physarum_Properties", 0, "", ICON_NONE);
}

static void physarum_panel_properties(const bContext *C, Panel *panel)
{
  bScreen *screen = CTX_wm_screen(C);
  SpacePhysarum *sphys = CTX_wm_space_physarum(C);
  Scene *scene = CTX_data_scene(C);
  PointerRNA spaceptr, sceneptr;
  uiLayout *layout = panel->layout;
  uiLayout *col, *sub;

  /* get RNA pointers for use when creating the UI elements */
  RNA_id_pointer_create(&scene->id, &sceneptr);
  RNA_pointer_create(&screen->id, &RNA_SpaceGraphEditor, sphys, &spaceptr);

  uiLayoutSetPropSep(layout, true);
  uiLayoutSetPropDecorate(layout, false);

  /* 2D-Cursor */
  col = uiLayoutColumn(layout, false);

  sub = uiLayoutColumn(col, true);

  uiItemR(sub, &sceneptr, "frame_current", 0, IFACE_("Propertie A"), ICON_NONE);

  uiItemR(sub, &sceneptr, "frame_current", 0, IFACE_("Propertie B"), ICON_NONE);

  uiItemR(sub, &sceneptr, "frame_current", 0, IFACE_("Propertie C"), ICON_NONE);

  sub = uiLayoutColumn(col, true);
  uiItemO(sub, IFACE_("Begin"), ICON_NONE, "GRAPH_OT_frame_jump");
}


void physarum_buttons_register(ARegionType *art)
{
  PanelType *pt;

  pt = MEM_callocN(sizeof(PanelType), "spacetype physarum panel properties");
  strcpy(pt->idname, "PHYSARUM_PT_view");
  strcpy(pt->category, "Properties");
  strcpy(pt->translation_context, BLT_I18NCONTEXT_DEFAULT_BPYRNA);
  pt->draw = physarum_panel_properties;
  pt->draw_header = physarum_panel_properties_header;
  BLI_addtail(&art->paneltypes, pt);
}

