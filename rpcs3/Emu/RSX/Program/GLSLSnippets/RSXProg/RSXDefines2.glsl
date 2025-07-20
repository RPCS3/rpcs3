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

struct vertex_layout_t
{
	uint vertex_base_index;
	uint vertex_index_offset;
	uint draw_id;
	uint reserved;
	uvec2 attrib_data[16];
};

)"
