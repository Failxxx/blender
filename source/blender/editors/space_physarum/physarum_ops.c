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
static const int DEFAULT_BYTES_STIRNG = 1024;

typedef struct Path {
  char folder[1024];
  char file_name[1024];
  char full_path[2048];
  char extension[128];
} Path;

/* -------------------------------------------------------------------- */
/** \Render Single Frame
 * \{ */

unsigned char *create_bitmap_file_header(int height, int stride)
{
  int fileSize = FILE_HEADER_SIZE + INFO_HEADER_SIZE + (stride * height);

  static unsigned char fileHeader[] = {
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

  fileHeader[0] = (unsigned char)('B');
  fileHeader[1] = (unsigned char)('M');
  fileHeader[2] = (unsigned char)(fileSize);
  fileHeader[3] = (unsigned char)(fileSize >> 8);
  fileHeader[4] = (unsigned char)(fileSize >> 16);
  fileHeader[5] = (unsigned char)(fileSize >> 24);
  fileHeader[10] = (unsigned char)(FILE_HEADER_SIZE + INFO_HEADER_SIZE);

  return fileHeader;
}

unsigned char *create_bitmap_info_header(int height, int width)
{
  static unsigned char infoHeader[] = {
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

  infoHeader[0] = (unsigned char)(INFO_HEADER_SIZE);
  infoHeader[4] = (unsigned char)(width);
  infoHeader[5] = (unsigned char)(width >> 8);
  infoHeader[6] = (unsigned char)(width >> 16);
  infoHeader[7] = (unsigned char)(width >> 24);
  infoHeader[8] = (unsigned char)(height);
  infoHeader[9] = (unsigned char)(height >> 8);
  infoHeader[10] = (unsigned char)(height >> 16);
  infoHeader[11] = (unsigned char)(height >> 24);
  infoHeader[12] = (unsigned char)(1);
  infoHeader[14] = (unsigned char)(BYTES_PER_PIXEL * 8);

  return infoHeader;
}

void get_file_name_from_raw_path(char *dest, const char *raw_path)
{
  char path[1024] = "";
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
  char path[1024] = "";
  strcpy(path, raw_path);

  if (path[strlen(path) - 1] != '\\') {
    // Get complete file name
    const char delimiter_slash[2] = "\\";
    char complete_file_name[1024] = "";
    char *token = strtok(path, delimiter_slash);
    while (token != NULL) {
      strcpy(complete_file_name, token);
      token = strtok(NULL, delimiter_slash);
    }

    // Get folder
    char folder[1024] = "";
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
  char path[1024] = "";
  get_folder_from_raw_path(path, raw_path);
  printf("Path = %s\n", path);
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

void export_bitmap_image(unsigned char *image, const int height, const int width, const char *output_path)
{
  int width_in_bytes = width * BYTES_PER_PIXEL;
  unsigned char padding[3] = {0, 0, 0};
  int padding_size = (4 - (width_in_bytes) % 4) % 4;
  int stride = (width_in_bytes) + padding_size;
  
  FILE *output_file = fopen(output_path, "wb");
  if (output_file == NULL) {
    printf("ERROR::Physarum::export_bitmap_image, error when creating the output_file.\n");
    printf("Path = %s\n", output_path);
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
}

static int physarum_single_render_exec(bContext *C, wmOperator *op)
{
  SpacePhysarum *sphys = CTX_wm_space_physarum(C);

  if (path_may_be_ok(sphys->output_path)) {
    Path path = get_path_from_raw_path(sphys->output_path);
    export_bitmap_image(
        sphys->output_image_data, sphys->screen_height, sphys->screen_width, path.full_path);
  }

  return OPERATOR_FINISHED;
}

void PHYSARUM_OT_single_render(wmOperatorType *ot)
{
  /* identifiers */
  ot->name = "Render one frame";
  ot->idname = "PHYSARUM_OT_single_render";
  ot->description = "Render a unique frame";

  /* api callbacks */
  ot->exec = physarum_single_render_exec;
}

/* -------------------------------------------------------------------- */
/** \Render Animation
 * \{ */

static int physarum_animation_render_exec(bContext *C, wmOperator *op)
{
  SpacePhysarum *sphys = CTX_wm_space_physarum(C);
  PhysarumRenderingSettings *prs = sphys->prs;
  sphys->render_frames_counter = sphys->nb_frames_to_render;


  for (int i = 0; i < sphys->nb_frames_to_render; ++i) {
    char *output_path = (char *) malloc(strlen(sphys->output_path) + 200);
    strcpy(output_path, sphys->output_path);
    char *imageCount = (char *) malloc(256);
    snprintf(imageCount, 256, "_%d.bmp", i);
    strcat(output_path, imageCount);
    export_bitmap_image(sphys->output_image_data, prs->screen_height, prs->screen_width, output_path);
    printf("Image generated!\n");
    free(imageCount);
    printf("Free 1 ok\n");
    free(output_path);
    printf("Free 2 ok\n");
  }

  return OPERATOR_FINISHED;
}

void PHYSARUM_animation_frame_render(const bContext *C)
{
  SpacePhysarum *sphys = CTX_wm_space_physarum(C);
  PhysarumRenderingSettings *prs = sphys->prs;

  char *imageFileName = (char *)malloc(256);
  snprintf(imageFileName, 256, "Physarum_Animation_Render_%d.bmp", sphys->nb_frames_to_render - sphys->render_frames_counter);
  export_bitmap_image(sphys->output_image_data, prs->screen_height, prs->screen_width, imageFileName);
  printf("Image generated!");
  free(imageFileName);
}

void PHYSARUM_OT_animation_render(wmOperatorType *ot)
{
  /* identifiers */
  ot->name = "Animation Render";
  ot->idname = "PHYSARUM_OT_animation_render";
  ot->description = "Render an animation of physarum";

  /* api callbacks */
  ot->exec = physarum_animation_render_exec;
}

/* -------------------------------------------------------------------- */
/** \Choice of Physarum Mode
 * \{ */

static int physarum_2D_drawing_exec(bContext *C, wmOperator *op)
{
  SpacePhysarum *sphys = CTX_wm_space_physarum(C);
  sphys->mode = SP_PHYSARUM_2D;
  return OPERATOR_FINISHED;
}

void PHYSARUM_OT_draw_2D(wmOperatorType *ot)
{
  /* identifiers */
  ot->name = "Physarum 2D";
  ot->idname = "PHYSARUM_OT_draw_2d";
  ot->description = "Choose physarum 2D rendering mode";

  /* api callbacks */
  ot->exec = physarum_2D_drawing_exec;
}

static int physarum_3D_drawing_exec(bContext *C, wmOperator *op)
{
  SpacePhysarum *sphys = CTX_wm_space_physarum(C);
  sphys->mode = SP_PHYSARUM_3D;
  return OPERATOR_FINISHED;
}

void PHYSARUM_OT_draw_3D(wmOperatorType *ot)
{
  /* identifiers */
  ot->name = "Physarum 3D";
  ot->idname = "PHYSARUM_OT_draw_3d";
  ot->description = "Choose physarum 3D rendering mode";

  /* api callbacks */
  ot->exec = physarum_3D_drawing_exec;
}
