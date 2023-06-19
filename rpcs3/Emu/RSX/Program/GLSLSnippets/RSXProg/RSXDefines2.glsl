R"(
// Small structures that should be defined before any backend logic
struct sampler_info
{
	vec4 scale_bias;
	uint remap;
	uint flags;
};

)"
