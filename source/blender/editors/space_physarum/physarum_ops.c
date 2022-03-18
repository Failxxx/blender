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
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "BKE_context.h"

#include "ED_screen.h"
#include "BKE_main.h"

#include "WM_api.h"

#include "physarum_intern.h"

static const char default_output_file_name[64] = "physarum_output";
static const int BYTES_PER_PIXEL = 3;
static const int FILE_HEADER_SIZE = 14;
static const int INFO_HEADER_SIZE = 40;
#define DEFAULT_BYTES_STIRNG 1024

typedef struct Path {
  char folder[DEFAULT_BYTES_STIRNG];
  char file_name[DEFAULT_BYTES_STIRNG];
  char full_path[DEFAULT_BYTES_STIRNG * 2];
  char extension[DEFAULT_BYTES_STIRNG];
} Path;

// Utils functions

unsigned char *create_bitmap_file_header(const int height, const int stride)
{
  int file_size = FILE_HEADER_SIZE + INFO_HEADER_SIZE + (stride * height);

  static unsigned char file_header[] = {
      0,
      0,  /// signature
      0,
      0,
      0,
      0,  /// image file size in bytes
      0,
      0,
      0,
      0,  /// reserved
      0,
      0,
      0,
      0,  /// start of pixel array
  };

  file_header[0] = (unsigned char)('B');
  file_header[1] = (unsigned char)('M');
  file_header[2] = (unsigned char)(file_size);
  file_header[3] = (unsigned char)(file_size >> 8);
  file_header[4] = (unsigned char)(file_size >> 16);
  file_header[5] = (unsigned char)(file_size >> 24);
  file_header[10] = (unsigned char)(FILE_HEADER_SIZE + INFO_HEADER_SIZE);

  return file_header;
}

unsigned char *create_bitmap_info_header(const int height, const int width)
{
  static unsigned char info_header[] = {
      0, 0, 0, 0,  /// header size
      0, 0, 0, 0,  /// image width
      0, 0, 0, 0,  /// image height
      0, 0,        /// number of color planes
      0, 0,        /// bits per pixel
      0, 0, 0, 0,  /// compression
      0, 0, 0, 0,  /// image size
      0, 0, 0, 0,  /// horizontal resolution
      0, 0, 0, 0,  /// vertical resolution
      0, 0, 0, 0,  /// colors in color table
      0, 0, 0, 0,  /// important color count
  };

  info_header[0] = (unsigned char)(INFO_HEADER_SIZE);
  info_header[4] = (unsigned char)(width);
  info_header[5] = (unsigned char)(width >> 8);
  info_header[6] = (unsigned char)(width >> 16);
  info_header[7] = (unsigned char)(width >> 24);
  info_header[8] = (unsigned char)(height);
  info_header[9] = (unsigned char)(height >> 8);
  info_header[10] = (unsigned char)(height >> 16);
  info_header[11] = (unsigned char)(height >> 24);
  info_header[12] = (unsigned char)(1);
  info_header[14] = (unsigned char)(BYTES_PER_PIXEL * 8);

  return info_header;
}

void get_file_name_from_raw_path(char *dest, const char *raw_path)
{
  char path[DEFAULT_BYTES_STIRNG] = "";
  strcpy(path, raw_path);

  if (path[strlen(path) - 1] != '\\') {
    // Remove extension of file
    char *last_dot_pos = strrchr(path, '.');
    if (last_dot_pos != NULL)
      *last_dot_pos = '\0';

    const char delimiter[2] = "\\";
    char *token = strtok(path, delimiter);
    while (token != NULL) {
      strcpy(dest, token);
      token = strtok(NULL, delimiter);
    }
  }
  else {
    strcpy(dest, default_output_file_name);
  }
}

void get_folder_from_raw_path(char *dest, const char *raw_path)
{
  char path[DEFAULT_BYTES_STIRNG] = "";
  strcpy(path, raw_path);

  if (path[strlen(path) - 1] != '\\') {
    // Get complete file name
    const char delimiter_slash[2] = "\\";
    char complete_file_name[DEFAULT_BYTES_STIRNG] = "";
    char *token = strtok(path, delimiter_slash);
    while (token != NULL) {
      strcpy(complete_file_name, token);
      token = strtok(NULL, delimiter_slash);
    }

    // Get folder
    char folder[DEFAULT_BYTES_STIRNG] = "";
    strcpy(path, raw_path);
    token = strtok(path, delimiter_slash);
    while (token != NULL) {
      if (strcmp(token, complete_file_name)) {
        strcat(folder, token);
        strcat(folder, "\\");
      }
      token = strtok(NULL, "\\");
    }

    strcpy(dest, folder);
  }
  else {
    strcpy(dest, path);
  }
}

void get_full_path_for_animation_rendering(Path *path, const int nb_frame)
{
  char nb_frame_str[DEFAULT_BYTES_STIRNG] = "";
  strcpy(path->full_path, path->folder);
  strcat(path->full_path, path->file_name);
  snprintf(nb_frame_str, DEFAULT_BYTES_STIRNG, "_%d", nb_frame);
  strcat(path->full_path, nb_frame_str);
  strcat(path->full_path, path->extension);
}

Path get_path_from_raw_path(const char *raw_path)
{
  Path path;
  strcpy(path.extension, ".bmp");
  get_file_name_from_raw_path(path.file_name, raw_path);
  get_folder_from_raw_path(path.folder, raw_path);
  // Construct full path
  strcpy(path.full_path, path.folder);
  strcat(path.full_path, path.file_name);
  strcat(path.full_path, path.extension);
  return path;
}

int path_may_be_ok(const char *raw_path) {
  // Check if given path is empty
  if (raw_path == NULL || strlen(raw_path) <= 0) {
    printf("ERROR::Physarum, the given path is empty!\n");
    return 0;
  }

  // Check if folder exists
  char path[DEFAULT_BYTES_STIRNG] = "";
  get_folder_from_raw_path(path, raw_path);
  struct stat info;

  if (stat(path, &info) != 0) {
    printf("ERROR::Physarum, the given path does not exists!\n");
    printf("Path = %s\n", raw_path);
    return 0;
  }
  else if ((info.st_mode & S_IFMT) == S_IFDIR) {
    return 1;
  }

  printf("ERROR::Physarum, the given path does not seem correct...\n");
  printf("Path = %s\n", raw_path);
  return 0;
}

int export_bitmap_image(unsigned char *image, const int height, const int width, const char *output_path)
{
  int error = 0;
  int width_in_bytes = width * BYTES_PER_PIXEL;
  unsigned char padding[3] = {0, 0, 0};
  int padding_size = (4 - (width_in_bytes) % 4) % 4;
  int stride = (width_in_bytes) + padding_size;

  if (image == NULL) {
    printf("ERROR::Physarum::export_bitmap_image, the given image is null, can't write it to a file.\n");
    error = 1;
    return error;
  }
  
  FILE *output_file = fopen(output_path, "wb");
  if (output_file == NULL) {
    printf("ERROR::Physarum::export_bitmap_image, error when creating the output_file.\n");
    printf("Path = %s\n", output_path);
    error = 1;
  }
  else {
    unsigned char *file_header = create_bitmap_file_header(height, stride);
    size_t written_blocks = fwrite(file_header, 1, FILE_HEADER_SIZE, output_file);
    if (written_blocks != FILE_HEADER_SIZE) {
      printf("ERROR::Physarum::export_bitmap_image, error when writting the file header.");
    }

    unsigned char *info_header = create_bitmap_info_header(height, width);
    written_blocks = fwrite(info_header, 1, INFO_HEADER_SIZE, output_file);
    if (written_blocks != INFO_HEADER_SIZE) {
      printf("ERROR::Physarum::export_bitmap_image, error when writting the header information.");
    }

    for (int i = 0; i < height; i++) {
      fwrite(image + (i * width_in_bytes), BYTES_PER_PIXEL, width, output_file);
      fwrite(padding, 1, padding_size, output_file);
    }

    printf("Image saved at: %s\n", output_path);
    fclose(output_file);
  }

  return error;
}

// Render a single frame

int physarum_render_single_frame(SpacePhysarum *sphys)
{
  int error = 0;
  if (path_may_be_ok(sphys->output_path)) {
    Path path = get_path_from_raw_path(sphys->output_path);
    if (sphys->rendering_mode == SP_PHYSARUM_RENDER_ANIMATION) {
      get_full_path_for_animation_rendering(&path, sphys->render_frames_counter);
    }
    error = export_bitmap_image(
        sphys->output_image_data, sphys->screen_height, sphys->screen_width, path.full_path);
  }
  else {
    error = 1;  // Path is not ok, there is an error
  }

  return error;
}

static int physarum_single_render_exec(bContext *C, wmOperator *op)
{
  SpacePhysarum *sphys = CTX_wm_space_physarum(C);
  physarum_render_single_frame(sphys);

  return OPERATOR_FINISHED;
}

static bool physarum_single_render_poll(bContext *C)
{
  SpacePhysarum *sphys = CTX_wm_space_physarum(C);
  return sphys->rendering_mode != SP_PHYSARUM_RENDER_ANIMATION;
}

void PHYSARUM_OT_single_render(wmOperatorType *ot)
{
  /* identifiers */
  ot->name = "Render one frame";
  ot->idname = "PHYSARUM_OT_single_render";
  ot->description = "Render a unique frame";

  /* api callbacks */
  ot->exec = physarum_single_render_exec;
  ot->poll = physarum_single_render_poll;
}

// Render an animation

static int physarum_animation_render_exec(bContext *C, wmOperator *op)
{
  SpacePhysarum *sphys = CTX_wm_space_physarum(C);
  sphys->render_frames_counter = 0;
  sphys->rendering_mode = SP_PHYSARUM_RENDER_ANIMATION;

  return OPERATOR_FINISHED;
}

static bool physarum_animation_render_poll(bContext *C)
{
  SpacePhysarum *sphys = CTX_wm_space_physarum(C);
  return sphys->rendering_mode != SP_PHYSARUM_RENDER_ANIMATION;
}

void PHYSARUM_OT_animation_render(wmOperatorType *ot)
{
  /* identifiers */
  ot->name = "Animation Render";
  ot->idname = "PHYSARUM_OT_animation_render";
  ot->description = "Render an animation of physarum";

  /* api callbacks */
  ot->exec = physarum_animation_render_exec;
  ot->poll = physarum_animation_render_poll;
}

void physarum_render_animation(SpacePhysarum *sphys)
{
  if (sphys->render_frames_counter < sphys->nb_frames_to_render) {
    int error = physarum_render_single_frame(sphys);
    if (error == 1) { // If there is an error, stop the rendering
      sphys->rendering_mode = SP_PHYSARUM_VIEWPORT;
      sphys->render_frames_counter = sphys->nb_frames_to_render;
      printf("ERROR:Physarum::render_animation, error detected rendering the animation, stop.\n");
      return;
    }
  }
  else { // End of the rendering, stop it
    sphys->rendering_mode = SP_PHYSARUM_VIEWPORT;
    sphys->render_frames_counter = sphys->nb_frames_to_render;
    return;
  }

  sphys->render_frames_counter++;
}

// Switch between physarum 2D and physarum 3D

static int physarum_2D_drawing_exec(bContext *C, wmOperator *op)
{
  SpacePhysarum *sphys = CTX_wm_space_physarum(C);
  sphys->mode = SP_PHYSARUM_2D;
  return OPERATOR_FINISHED;
}

static bool physarum_2D_drawing_poll(bContext *C)
{
  SpacePhysarum *sphys = CTX_wm_space_physarum(C);
  return sphys->rendering_mode != SP_PHYSARUM_RENDER_ANIMATION;
}

void PHYSARUM_OT_draw_2D(wmOperatorType *ot)
{
  /* identifiers */
  ot->name = "Physarum 2D";
  ot->idname = "PHYSARUM_OT_draw_2d";
  ot->description = "Choose physarum 2D rendering mode";

  /* api callbacks */
  ot->exec = physarum_2D_drawing_exec;
  ot->poll = physarum_2D_drawing_poll;
}

static int physarum_3D_drawing_exec(bContext *C, wmOperator *op)
{
  SpacePhysarum *sphys = CTX_wm_space_physarum(C);
  sphys->mode = SP_PHYSARUM_3D;
  return OPERATOR_FINISHED;
}

static bool physarum_3D_drawing_poll(bContext *C)
{
  SpacePhysarum *sphys = CTX_wm_space_physarum(C);
  return sphys->rendering_mode != SP_PHYSARUM_RENDER_ANIMATION;
}

void PHYSARUM_OT_draw_3D(wmOperatorType *ot)
{
  /* identifiers */
  ot->name = "Physarum 3D";
  ot->idname = "PHYSARUM_OT_draw_3d";
  ot->description = "Choose physarum 3D rendering mode";

  /* api callbacks */
  ot->exec = physarum_3D_drawing_exec;
  ot->poll = physarum_3D_drawing_poll;
}

/* DRAW FUNCTION */

static int physarum_drawing_exec(bContext *C, wmOperator *op)
{
  SpacePhysarum *sphys = CTX_wm_space_physarum(C);
  ARegion *ar = sphys->region;
  bContext *context = sphys->context;

  physarum_draw_view(context, ar);
  return OPERATOR_FINISHED;
}

void PHYSARUM_OT_draw(struct wmOperatorType *ot)
{
  /* identifiers */
  ot->name = "Physarum 3D";
  ot->idname = "PHYSARUM_OT_draw";
  ot->description = "Draw physarum";

  /* api callbacks */
  ot->exec = physarum_drawing_exec;
}
