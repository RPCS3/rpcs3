R"(
// Small structures that should be defined before any backend logic
// Avoid arrays and sub-vec4 members because of std140 padding constraints
struct sampler_info
{
	float scale_x, scale_y, scale_z;  // 12
	float bias_x, bias_y, bias_z;     // 24
	float clamp_min_x, clamp_min_y;   // 32
	float clamp_max_x, clamp_max_y;   // 40
	uint remap;                       // 44
	uint flags;                       // 48
};

struct vertex_context_t
{
	mat4 scale_offset_mat;
	uint user_clip_configuration_bits;
	uint transform_branch_bits;
	float point_size;
	float z_near;
	float z_far;
	float reserved[3];
};

struct draw_parameters_t
{
	uint vertex_base_index;
	uint vertex_index_offset;
	uint draw_id;
	uint xform_constants_offset;
	uint vs_context_offset;
	uint fs_constants_offset;
	uint fs_context_offset;
	uint fs_texture_base_index;
	uint fs_stipple_pattern_offset;
	uint reserved;
	uvec2 attrib_data[16];
};

struct fragment_context_t
{
	float fog_param0;
	float fog_param1;
	uint rop_control;
	float alpha_ref;
	uint fog_mode;
	float wpos_scale;
	vec2 wpos_bias;
};

)"
