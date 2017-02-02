#include "stdafx.h"
#include "Emu\RSX\Common\BufferUtils.h"


TEST_CLASS(rsx_common)
{
	// Check UB256 size is correctly set in write_vertex_array_data_to_buffer
	TEST_METHOD(ub256_upload)
	{
		std::vector<gsl::byte> dest_buffer(2200);
		std::vector<gsl::byte> src_buffer(100000); // Big enough

		write_vertex_array_data_to_buffer(gsl::multi_span<gsl::byte>(dest_buffer), src_buffer.data(), 0, 550, rsx::vertex_base_type::ub256, 4, 20, 4);
	}
};
