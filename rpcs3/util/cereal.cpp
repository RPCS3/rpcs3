#include "util/cereal.hpp"
#include <string>
#include "Utilities/StrFmt.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/RSX/Capture/rsx_capture.h"

#ifndef _MSC_VER
#pragma GCC diagnostic ignored "-Weffc++"
#endif

#include "cereal/archives/binary.hpp"
#include <cereal/types/vector.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/unordered_set.hpp>
#include <cereal/types/unordered_map.hpp>

#include <sstream>

namespace cereal
{
	[[noreturn]] void throw_exception(const std::string& err)
	{
		fmt::throw_exception("%s", err);
	}

	[[noreturn]] void throw_exception(const char* err)
	{
		fmt::throw_exception("%s", err);
	}
}

template <>
std::string cereal_serialize<rsx::frame_capture_data>(const rsx::frame_capture_data& data)
{
	std::ostringstream os;
	cereal::BinaryOutputArchive archive(os);
	archive(data);
	return os.str();
}

template <>
void cereal_deserialize<rsx::frame_capture_data>(rsx::frame_capture_data& data, const std::string& src)
{
	std::istringstream is(src);
	cereal::BinaryInputArchive archive(is);
	archive(data);
}
