float wire_depth_alpha(float depth, vec2 info)
{
	if (info.x > 0) {
		float view_z = -get_view_z_from_depth(depth);

		/* Clamp bias by the near clip plane. */
		float bias_clamp = max(-get_view_z_from_depth(0), info.y);

		/* Subtract bias from the depth and compute the fade alpha. */
		float alpha = pow(0.5, max(0.0, view_z - bias_clamp) * info.x);
		return clamp(alpha, 0, 1);
	}
	else {
		return 1.0;
	}
}
