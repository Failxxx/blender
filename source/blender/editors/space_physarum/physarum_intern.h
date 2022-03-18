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
// Debug
extern char datatoc_gpu_shader_3D_debug_physarum_vs_glsl[];
extern char datatoc_gpu_shader_3D_debug_physarum_fs_glsl[];
// Physarum 2D
extern char datatoc_gpu_shader_3D_physarum_2d_quad_vs_glsl[];
extern char datatoc_gpu_shader_3D_physarum_2d_render_agents_vs_glsl[];
extern char datatoc_gpu_shader_3D_physarum_2d_diffuse_decay_fs_glsl[];
extern char datatoc_gpu_shader_3D_physarum_2d_update_agents_fs_glsl[];
extern char datatoc_gpu_shader_3D_physarum_2d_render_agents_fs_glsl[];
extern char datatoc_gpu_shader_3D_physarum_2d_post_process_fs_glsl[];

// Physarum 3D
extern char datatoc_gpu_shader_3D_physarum_3d_particle_cs_glsl[];
extern char datatoc_gpu_shader_3D_physarum_3d_decay_cs_glsl[];
extern char datatoc_gpu_shader_3D_physarum_3d_vertex_vs_glsl[];
extern char datatoc_gpu_shader_3D_physarum_3d_pixel_fs_glsl[];

/* Operators */

/* Physarum draw functions */
void initialize_physarum_rendering_settings(PRenderingSettings *prs);
void adapt_projection_matrix_window_rescale(PRenderingSettings *prs);
void physarum_draw_view(const bContext *C, ARegion *region);

/* Physarum 2D functions */
void initialize_physarum_2d(Physarum2D *p2d);
void free_physarum_2d(Physarum2D *p2d);
void physarum_2d_reset_simulation(Physarum2D *p2d, const float particles_population_factor);
void physarum_2d_draw_view(Physarum2D *p2d);
void physarum_2d_handle_events(Physarum2D *p2d,
                               SpacePhysarum *sphys,
                               const bContext *C,
                               ARegion *region);
struct GPUVertBuf *make_new_quad_mesh();

/* Render Function */
void PHYSARUM_OT_reset_physarum_2d(wmOperatorType *ot);
void PHYSARUM_OT_single_render(struct wmOperatorType *ot);
void PHYSARUM_OT_animation_render(struct wmOperatorType *ot);
void PHYSARUM_OT_draw_3D(struct wmOperatorType *ot);
void PHYSARUM_OT_draw_2D(struct wmOperatorType *ot);
void physarum_render_animation(SpacePhysarum *sphys);

/* Physarum 3D algorithms functions */
void P3D_init(Physarum3D *p3d, int particles_amount, int texture_size);
void P3D_draw(Physarum3D *p3d);
void P3D_free(Physarum3D *p3d);
void P3D_particles_generate(Physarum3D *p3d);
struct GPUVertBuf *P3D_get_display_VBO();
struct GPUVertBuf *P3D_get_data_VBO(Physarum3D *p3d);
void P3D_load_shaders(Physarum3D *p3d);
