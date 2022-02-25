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

#pragma once

/* internal exports only */

/* Physarum custom shaders */
extern char datatoc_gpu_shader_3D_debug_physarum_vs_glsl[];
extern char datatoc_gpu_shader_3D_debug_physarum_fs_glsl[];

/* Operators */
void SPACE_PHYSARUM_OT_red_region(wmOperatorType *ot);
void SPACE_PHYSARUM_OT_green_region(wmOperatorType *ot);
void SPACE_PHYSARUM_OT_blue_region(wmOperatorType *ot);

/* Physarum draw functions */
void initialize_physarum_rendering_settings(PRenderingSettings *prs);
void initialize_physarum_gpu_data(PhysarumGPUData *pgd);
void adapt_projection_matrix_window_rescale(PRenderingSettings *prs);
void physarum_draw_view(const bContext *C, ARegion *region);
void free_gpu_data(SpacePhysarum *sphys);

void physarum_buttons_register(struct ARegionType *art);

