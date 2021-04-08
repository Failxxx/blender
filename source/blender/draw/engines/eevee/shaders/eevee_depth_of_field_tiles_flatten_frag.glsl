
/**
 * Tile flatten pass: Takes the halfres CoC buffer and converts it to 8x8 tiles.
 *
 * Output min and max values for each tile and for both foreground & background.
 * Also outputs min intersectable CoC for the background, which is the minimum CoC
 * that comes from the background pixels.
 *
 * Input:
 * - Half-resolution Circle of confusion. Out of setup pass.
 * Output:
 * - Separated foreground and background CoC. 1/8th of half-res resolution. So 1/16th of full-res.
 **/

#pragma BLENDER_REQUIRE(eevee_depth_of_field_lib.glsl)

uniform sampler2D coc_tx;

layout(location = 0) out vec4 out_tile_fg;
layout(location = 1) out vec3 out_tile_bg;

const int halfres_tile_divisor = DOF_TILE_DIVISOR / 2;

void main()
{
  ivec2 halfres_bounds = textureSize(coc_tx, 0).xy - 1;
  ivec2 tile_co = ivec2(gl_FragCoord.xy);

  CocTile tile = dof_coc_tile_init();

  for (int x = 0; x < halfres_tile_divisor; x++) {
    /* OPTI: Could be done in separate passes. */
    for (int y = 0; y < halfres_tile_divisor; y++) {
      ivec2 sample_texel = tile_co * halfres_tile_divisor + ivec2(x, y);
      vec2 sample_data = texelFetch(coc_tx, min(sample_texel, halfres_bounds), 0).rg;
      float sample_coc = sample_data.x;
      float sample_slight_focus_coc = sample_data.y;

      float fg_coc = min(sample_coc, 0.0);
      tile.fg_min_coc = min(tile.fg_min_coc, fg_coc);
      tile.fg_max_coc = max(tile.fg_max_coc, fg_coc);

      float bg_coc = max(sample_coc, 0.0);
      tile.bg_min_coc = min(tile.bg_min_coc, bg_coc);
      tile.bg_max_coc = max(tile.bg_max_coc, bg_coc);

      if (sample_coc > 0.0) {
        tile.bg_min_intersectable_coc = min(tile.bg_min_intersectable_coc, bg_coc);
      }
      if (sample_coc < 0.0) {
        tile.fg_max_intersectable_coc = max(tile.fg_max_intersectable_coc, fg_coc);
      }

      tile.fg_slight_focus_max_coc = dof_coc_max_slight_focus(tile.fg_slight_focus_max_coc,
                                                              sample_slight_focus_coc);
    }
  }

  dof_coc_tile_store(tile, out_tile_fg, out_tile_bg);
}
