/*
 * Copyright 2011-2021 Blender Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "kernel/kernel_accumulate.h"
#include "kernel/kernel_emission.h"
#include "kernel/kernel_light.h"
#include "kernel/kernel_shader.h"

CCL_NAMESPACE_BEGIN

ccl_device_inline void integrate_light(INTEGRATOR_STATE_ARGS,
                                       ccl_global float *ccl_restrict render_buffer)
{
  /* Setup light sample. */
  Intersection isect ccl_optional_struct_init;
  integrator_state_read_isect(INTEGRATOR_STATE_PASS, &isect);

  const float3 ray_P = INTEGRATOR_STATE(ray, P);
  const float3 ray_D = INTEGRATOR_STATE(ray, D);
  const float ray_t = INTEGRATOR_STATE(ray, t);
  const float ray_time = INTEGRATOR_STATE(ray, time);

  LightSample ls ccl_optional_struct_init;
  const bool use_light_sample = light_sample_from_intersection(
      kg, &isect, ray_P, ray_D, ray_t, &ls);

  /* Advance ray beyond light. */
  const float3 new_ray_P = ray_offset(ray_P + ray_D * isect.t, ray_D);
  INTEGRATOR_STATE_WRITE(ray, P) = new_ray_P;
  INTEGRATOR_STATE_WRITE(ray, t) -= isect.t;

  if (!use_light_sample) {
    return;
  }

  /* Use visibility flag to skip lights. */
#ifdef __PASSES__
  const uint32_t path_flag = INTEGRATOR_STATE(path, flag);

  if (ls.shader & SHADER_EXCLUDE_ANY) {
    if (((ls.shader & SHADER_EXCLUDE_DIFFUSE) && (path_flag & PATH_RAY_DIFFUSE)) ||
        ((ls.shader & SHADER_EXCLUDE_GLOSSY) &&
         ((path_flag & (PATH_RAY_GLOSSY | PATH_RAY_REFLECT)) ==
          (PATH_RAY_GLOSSY | PATH_RAY_REFLECT))) ||
        ((ls.shader & SHADER_EXCLUDE_TRANSMIT) && (path_flag & PATH_RAY_TRANSMIT)) ||
        ((ls.shader & SHADER_EXCLUDE_SCATTER) && (path_flag & PATH_RAY_VOLUME_SCATTER)))
      return;
  }
#endif

  /* Evaluate light shader. */
  /* TODO: does aliasing like this break automatic SoA in CUDA? */
  ShaderDataTinyStorage emission_sd_storage;
  ShaderData *emission_sd = AS_SHADER_DATA(&emission_sd_storage);
  float3 light_eval = light_sample_shader_eval(INTEGRATOR_STATE_PASS, emission_sd, &ls, ray_time);
  if (is_zero(light_eval)) {
    return;
  }

  /* MIS weighting. */
  if (!(path_flag & PATH_RAY_MIS_SKIP)) {
    /* multiple importance sampling, get regular light pdf,
     * and compute weight with respect to BSDF pdf */
    const float ray_pdf = INTEGRATOR_STATE(path, ray_pdf);
    const float mis_weight = power_heuristic(ray_pdf, ls.pdf);
    light_eval *= mis_weight;
  }

  /* Write to render buffer. */
  kernel_accum_emission(INTEGRATOR_STATE_PASS, light_eval, render_buffer);
}

ccl_device void integrator_shade_light(INTEGRATOR_STATE_ARGS,
                                       ccl_global float *ccl_restrict render_buffer)
{
  integrate_light(INTEGRATOR_STATE_PASS, render_buffer);

  /* TODO: we could get stuck in an infinite loop if there are precision issues
   * and the same light is hit again.
   *
   * As a workaround count this as a transparent bounce. It makes some sense
   * to interpret lights as transparent surfaces (and support making them opaque),
   * but this needs to be revisited. */
  uint32_t transparent_bounce = INTEGRATOR_STATE(path, transparent_bounce) + 1;
  INTEGRATOR_STATE_WRITE(path, transparent_bounce) = transparent_bounce;

  if (transparent_bounce >= kernel_data.integrator.transparent_max_bounce) {
    INTEGRATOR_PATH_TERMINATE(SHADE_LIGHT);
    return;
  }
  else {
    INTEGRATOR_PATH_NEXT(SHADE_LIGHT, INTERSECT_CLOSEST);
    return;
  }

  /* TODO: in some cases we could continue directly to SHADE_BACKGROUND, but
   * probably that optimization is probably not practical if we add lights to
   * scene geometry. */
}

CCL_NAMESPACE_END
