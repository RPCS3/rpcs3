R"(
#ifdef _FORCE_POSITION_INVARIANCE
// PS3 has shader invariance, but we don't really care about most attributes outside ATTR0
invariant gl_Position;
#endif

#ifdef _EMULATE_ZCLIP_XFORM_STANDARD
// Technically the depth value here is the 'final' depth that should be stored in the Z buffer.
// Forward mapping eqn is d' = d * (f - n) + n, where d' is the stored Z value (this) and d is the normalized API value.
vec4 apply_zclip_xform(
	const in vec4 pos,
	const in float near_plane,
	const in float far_plane)
{
	if (pos.w != 0.0)
	{
		const float real_n = min(far_plane, near_plane);
		const float real_f = max(far_plane, near_plane);
		const double depth_range = double(real_f - real_n);
		const double inv_range = (depth_range > 0.000001) ? (1.0 / (depth_range * pos.w)) : 0.0;
		const double actual_d = (double(pos.z) - double(real_n * pos.w)) * inv_range;
		const double nearest_d = floor(actual_d + 0.5);
		const double epsilon = (inv_range * pos.w) / 16777215.;     // Epsilon value is the minimum discernable change in Z that should affect the stored Z
		const double d = _select(actual_d, nearest_d, abs(actual_d - nearest_d) < epsilon);
		return vec4(pos.xy, float(d * pos.w), pos.w);
	}
	else
	{
		return pos; // Only values where Z=0 can ever pass this clip
	}
}
#elif defined(_EMULATE_ZCLIP_XFORM_FALLBACK)
vec4 apply_zclip_xform(
	const in vec4 pos,
	const in float near_plane,
	const in float far_plane)
{
	float d = float(pos.z / pos.w);
	if (d < 0.f && d >= near_plane)
	{
		// Clamp
		d = 0.f;
	}
	else if (d > 1.f && d <= far_plane)
	{
		// Compress Z and store towards highest end of the range
		d = min(1., 0.99 + (0.01 * (pos.z - near_plane) / (far_plane - near_plane)));
	}
	else // This catch-call also handles w=0 since d=inf
	{
		return pos;
	}

	return vec4(pos.x, pos.y, d * pos.w, pos.w);
}
#endif

)"
