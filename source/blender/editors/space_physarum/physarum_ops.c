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

#include "BKE_context.h"

#include "ED_screen.h"

#include "WM_api.h"

#include "physarum_intern.h"

/* poll function for the operators */
int test_context_for_button_operator(struct bContext *C)
{
  return 1;
}

/* RED */

int set_background_red_in_main_region(struct bContext *C, struct wmOperator *oper)
{
  SpacePhysarum *sphys = CTX_wm_space_physarum(C);
  ARegion *ar = CTX_wm_region(C);

  sphys->color = RED;

  ED_area_tag_redraw(CTX_wm_area(C));

  return OPERATOR_FINISHED;
}


void SPACE_PHYSARUM_OT_red_region(wmOperatorType *ot)
{
  ot->name = "Red region button";
  ot->description = "Turns the main region background to red";
  ot->idname = "SPACE_PHYSARUM_OT_red_region";

  /* api callbacks */
  ot->exec = set_background_red_in_main_region;
  ot->poll = test_context_for_button_operator;

  /* flags */
  ot->flag = "OPTYPE_REGISTER";
}

/* GREEN */

int set_background_green_in_main_region(struct bContext *C, struct wmOperator *oper)
{
  SpacePhysarum *sphys = CTX_wm_space_physarum(C);
  ARegion *ar = CTX_wm_region(C);

  sphys->color = GREEN;

  ED_area_tag_redraw(CTX_wm_area(C));

  return OPERATOR_FINISHED;
}

void SPACE_PHYSARUM_OT_green_region(wmOperatorType *ot)
{
  ot->name = "Green region button";
  ot->description = "Turns the main region background to green";
  ot->idname = "SPACE_PHYSARUM_OT_green_region";

  /* api callbacks */
  ot->exec = set_background_green_in_main_region;
  ot->poll = test_context_for_button_operator;

  /* flags */
  ot->flag = "OPTYPE_REGISTER";
}

/* BLUE */

int set_background_blue_in_main_region(struct bContext *C, struct wmOperator *oper)
{
  SpacePhysarum *sphys = CTX_wm_space_physarum(C);
  ARegion *ar = CTX_wm_region(C);

  sphys->color = BLUE;

  ED_area_tag_redraw(CTX_wm_area(C));

  return OPERATOR_FINISHED;
}

void SPACE_PHYSARUM_OT_blue_region(wmOperatorType *ot)
{
  ot->name = "Blue region button";
  ot->description = "Turns the main region background to blue";
  ot->idname = "SPACE_PHYSARUM_OT_blue_region";

  /* api callbacks */
  ot->exec = set_background_blue_in_main_region;
  ot->poll = test_context_for_button_operator;

  /* flags */
  ot->flag = "OPTYPE_REGISTER";
}
