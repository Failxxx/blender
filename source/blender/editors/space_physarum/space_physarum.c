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

#include "UI_interface.h"
#include "UI_resources.h"
#include "UI_view2d.h"
#include "../interface/interface_intern.h"


#include "physarum_intern.h"

extern char datatoc_gpu_shader_3D_debug_physarum_vs_glsl[];
extern char datatoc_gpu_shader_3D_debug_physarum_fs_glsl[];

static void physarum_init(struct wmWindowManager *wm, struct ScrArea *area)
{
  SpacePhysarum *sphys = (SpacePhysarum *)area->spacedata.first;
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

  /* properties region */
  ar = MEM_callocN(sizeof(ARegion), "properties region for physarum");

  BLI_addtail(&sphys->regionbase, ar);
  ar->regiontype = RGN_TYPE_UI;
  ar->alignment = RGN_ALIGN_RIGHT;

  /* main region */
  ar = MEM_callocN(sizeof(ARegion), "main region of physarum");
  BLI_addtail(&sphys->regionbase, ar);
  ar->regiontype = RGN_TYPE_WINDOW;

  return (SpaceLink *)sphys;
}

/* function which initializes the header region of the editor */
static void physarum_header_region_init(wmWindowManager *UNUSED(wm), ARegion *region)
{
  ED_region_header_init(region);
}

/* function which initializes the main region of the editor */
static void physarum_main_region_init(wmWindowManager *UNUSED(wm), ARegion *region)
{
  UI_view2d_region_reinit(&region->v2d, V2D_COMMONVIEW_CUSTOM, region->winx, region->winy);
}

/* function which initializes the ui region of the editor */
static void physarum_ui_region_init(wmWindowManager *wm, ARegion *region)
{
  ED_region_panels_init(wm, region);
}

static void physarum_ui_region_draw(const bContext *C, ARegion *region)
{
  ED_region_panels_draw(C, region);
}

/* draw function of the main region */
static void physarum_main_region_draw(const bContext *C, ARegion *region)
{
  SpacePhysarum *sphys = CTX_wm_space_physarum(C);
  View2D *v2d = &region->v2d;

  /* ----- Setup ----- */

  // Only for information
  float viewport[4];
  GPU_viewport_size_get_f(viewport);
  print_v4("Viewport:", viewport);

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
  uint pos = GPU_vertformat_attr_add(format, "v_in_f3Position", GPU_COMP_F32, pos_comp_len, GPU_FETCH_FLOAT);
  uint color = GPU_vertformat_attr_add(format, "v_in_f4Color", GPU_COMP_F32, col_comp_len, GPU_FETCH_FLOAT);

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
  //GPU_batch_program_set_builtin(batch, GPU_SHADER_3D_FLAT_COLOR);
  //const DRWContextState *draw_ctx = DRW_context_state_get();
  //const GPUShaderConfigData *sh_cfg = &GPU_shader_cfg_data[draw_ctx->sh_cfg];
  GPUShader *shader = GPU_shader_create_from_arrays({
    .vert = (const char *[]){datatoc_gpu_shader_3D_debug_physarum_vs_glsl, NULL},
    .frag = (const char *[]){datatoc_gpu_shader_3D_debug_physarum_fs_glsl, NULL}
  });
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

  perspective_m4(projectionMatrix, - 0.5f, 0.5f, -0.5f, 0.5f, 1.0f, 100.f);

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

static void draw_buttons(uiBlock *block, uiLayout *layout)
{
  static int value = 100;

  struct wmOperatorType *ot = WM_operatortype_find("SPACE_PHYSARUM_OT_red_region", true);
  struct uiBut *but = uiDefBut(
      block, UI_BTYPE_BUT_TOGGLE, 1, "RED", 100, 2, 30, 19, (void *)&value, 0., 0., 0., 0., "");
  but->optype = ot;

  ot = WM_operatortype_find("SPACE_PHYSARUM_OT_green_region", true);
  but = uiDefBut(
      block, UI_BTYPE_BUT_TOGGLE, 1, "GREEN", 200, 5, 75, 19, (void *)&value, 0., 0., 0., 0., "");
  but->optype = ot;

  ot = WM_operatortype_find("SPACE_PHYSARUM_OT_blue_region", true);
  but = uiDefBut(
      block, UI_BTYPE_BUT_TOGGLE, 1, "BLUE", 300, 5, 75, 19, (void *)&value, 0., 0., 0., 0., "");
  but->optype = ot;
}

static void physarum_header_region_draw(const bContext *C, ARegion *region)
{
  ED_region_header(C, region);
}

void physarum_operatortypes(void)
{
  WM_operatortype_append(SPACE_PHYSARUM_OT_red_region);
  WM_operatortype_append(SPACE_PHYSARUM_OT_green_region);
  WM_operatortype_append(SPACE_PHYSARUM_OT_blue_region);
}

/****************** properties region ******************/

/* add handlers, stuff you only do once or on area/region changes */
static void physarum_buttons_region_init(wmWindowManager *wm, ARegion *region)
{
  wmKeyMap *keymap;

  ED_region_panels_init(wm, region);

  keymap = WM_keymap_ensure(wm->defaultconf, "Graph Editor Generic", SPACE_GRAPH, 0);
  WM_event_add_keymap_handler_v2d_mask(&region->handlers, keymap);
}

static void physarum_buttons_region_draw(const bContext *C, ARegion *region)
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
  art->draw = physarum_ui_region_draw;
  art->prefsizex = UI_SIDEBAR_PANEL_WIDTH;
  art->keymapflag = ED_KEYMAP_UI | ED_KEYMAP_FRAMES;

  /* regions: UI buttons */
  art = MEM_callocN(sizeof(ARegionType), "spacetype graphedit region");
  art->regionid = RGN_TYPE_UI;
  art->prefsizex = UI_SIDEBAR_PANEL_WIDTH;
  art->keymapflag = ED_KEYMAP_UI | ED_KEYMAP_FRAMES;
  art->init = physarum_buttons_region_init;
  art->draw = physarum_buttons_region_draw;

  BLI_addhead(&st->regiontypes, art);

  physarum_buttons_register(art);

  art = ED_area_type_hud(st->spaceid);
  BLI_addhead(&st->regiontypes, art);

  BKE_spacetype_register(st);
}
