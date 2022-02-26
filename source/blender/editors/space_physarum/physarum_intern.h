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
float randf(const float a, const float b);

/* Physarum custom shaders */
// Debug
extern char datatoc_gpu_shader_3D_debug_physarum_vs_glsl[];
extern char datatoc_gpu_shader_3D_debug_physarum_fs_glsl[];
// Physarum 2D
extern char datatoc_gpu_shader_3D_physarum_2d_quad_vs_glsl[];
extern char datatoc_gpu_shader_3D_physarum_2d_post_process_fs_glsl[];

/* Operators */

/* Physarum draw functions */
void initialize_physarum_rendering_settings(PRenderingSettings *prs);
void initialize_physarum_gpu_data(PhysarumGPUData *pgd);
void adapt_projection_matrix_window_rescale(PRenderingSettings *prs);
void physarum_draw_view(const bContext *C, ARegion *region);
void free_gpu_data(SpacePhysarum *sphys);

/* Physarum 2D algorithms functions */
void physarum_2d_compute_matrix(PhysarumData2D *pdata_2d, float projectionMatrix[4][4]);
void physarum_2d_draw_view(PhysarumData2D *pdata_2d, float projectionMatrix[4][4]);
void initialize_physarum_data_2d(PhysarumData2D *pdata_2d);
void free_physarum_data_2d(PhysarumData2D *pdata_2d);
