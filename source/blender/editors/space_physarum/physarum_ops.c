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

#include "BKE_context.h"

#include "ED_screen.h"

#include "WM_api.h"

#include "physarum_intern.h"


/* -------------------------------------------------------------------- */
/** \Render Single Frame
 * \{ */

const int BYTES_PER_PIXEL = 3;
const int FILE_HEADER_SIZE = 14;
const int INFO_HEADER_SIZE = 40;

unsigned char *createBitmapFileHeader(int height, int stride)
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

unsigned char *createBitmapInfoHeader(int height, int width)
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

void generateBitmapImage(unsigned char *image, int height, int width, char *imageFileName)
{
  int widthInBytes = width * BYTES_PER_PIXEL;

  unsigned char padding[3] = {0, 0, 0};
  int paddingSize = (4 - (widthInBytes) % 4) % 4;

  int stride = (widthInBytes) + paddingSize;

  FILE *imageFile = fopen(imageFileName, "wb");

  unsigned char *fileHeader = createBitmapFileHeader(height, stride);
  fwrite(fileHeader, 1, FILE_HEADER_SIZE, imageFile);

  unsigned char *infoHeader = createBitmapInfoHeader(height, width);
  fwrite(infoHeader, 1, INFO_HEADER_SIZE, imageFile);

  int i;
  for (i = 0; i < height; i++) {
    fwrite(image + (i * widthInBytes), BYTES_PER_PIXEL, width, imageFile);
    fwrite(padding, 1, paddingSize, imageFile);
  }

  fclose(imageFile);
}

static int physarum_single_render_exec(bContext *C, wmOperator *op)
{
  SpacePhysarum *sphys = CTX_wm_space_physarum(C);
  PhysarumRenderingSettings *prs = sphys->prs;
  char *imageFileName = (char *)"Physarum_Single_Frame.bmp";
  generateBitmapImage(sphys->image_data, prs->screen_height, prs->screen_width, imageFileName);
  printf("Image generated!");
}

void PHYSARUM_OT_single_render(wmOperatorType *ot)
{
  /* identifiers */
  ot->name = "Single Render";
  ot->idname = "PHYSARUM_OT_single_render";
  ot->description = "Render a single frame of physarum";

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
  sphys->counter_rendering_frame = sphys->number_frame;
  for (int i = 0; i < sphys->number_frame;i++) {
    char *imageFileName = (char *) malloc(256);
    snprintf(imageFileName, 256, "Physarum_Animation_Render_%d.bmp", i);
    generateBitmapImage(sphys->image_data, prs->screen_height, prs->screen_width, imageFileName);
    printf("Image generated!");
    free(imageFileName);
  }
}

void PHYSARUM_animation_frame_render(bContext *C)
{
  SpacePhysarum *sphys = CTX_wm_space_physarum(C);
  PhysarumRenderingSettings *prs = sphys->prs;

  char *imageFileName = (char *)malloc(256);
  snprintf(imageFileName, 256, "Physarum_Animation_Render_%d.bmp", sphys->number_frame - sphys->counter_rendering_frame);
  generateBitmapImage(sphys->image_data, prs->screen_height, prs->screen_width, imageFileName);
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
  PhysarumRenderingSettings *prs = sphys->prs;
  printf("Physarum 2D!");
}

void PHYSARUM_OT_draw_2D(wmOperatorType *ot)
{
  /* identifiers */
  ot->name = "Physarum 2D";
  ot->idname = "PHYSARUM_OT_draw_2d";
  ot->description = "Render a Physarum in 2 dimensions";

  /* api callbacks */
  ot->exec = physarum_2D_drawing_exec;
}

static int physarum_3D_drawing_exec(bContext *C, wmOperator *op)
{
  SpacePhysarum *sphys = CTX_wm_space_physarum(C);
  PhysarumRenderingSettings *prs = sphys->prs;
  printf("Physarum 3D!");
}

void PHYSARUM_OT_draw_3D(wmOperatorType *ot)
{
  /* identifiers */
  ot->name = "Physarum 3D";
  ot->idname = "PHYSARUM_OT_draw_3d";
  ot->description = "Render a Physarum in 3 dimensions";

  /* api callbacks */
  ot->exec = physarum_3D_drawing_exec;
}
