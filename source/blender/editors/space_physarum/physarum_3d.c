/** \file
 * \ingroup spphysarum
 */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "MEM_guardedalloc.h"

#include "BLI_blenlib.h"
#include "BLI_rand.h"
#include "BLI_utildefines.h"

#include "BKE_context.h"
#include "BKE_screen.h"

#include "ED_screen.h"
#include "ED_space_api.h"

#include "GPU_capabilities.h"
#include "GPU_context.h"
#include "GPU_framebuffer.h"
#include "GPU_immediate.h"
#include "GPU_immediate_util.h"
#include "GPU_viewport.h"

#include "WM_api.h"

#include "physarum_intern.h"

void P3D_init(Physarum3D *p3d, int particles_amount, int texture_size)
{
  P3D_particles_generate(p3d);
}

void P3D_draw(Physarum3D *p3d, PhysarumRenderingSettings *prs)
{
  P3D_init(p3d, 1000, 1000);

}

void P3D_free(Physarum3D *p3d)
{
}


//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------

void P3D_particles_generate(Physarum3D *p3d)
{
  for (size_t i = 0; i < p3d->particules_amount; i++) {

  }
}
