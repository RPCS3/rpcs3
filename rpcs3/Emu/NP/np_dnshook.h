#pragma once

#include <map>
#include <queue>
#include <vector>

#include "util/types.hpp"
#include "Utilities/mutex.h"

namespace np
{
	class dnshook
	{
	public:
		dnshook();

		void add_dns_spy(u32 sock);
		void remove_dns_spy(u32 sock);

		bool is_dns(u32 sock);
		bool is_dns_queue(u32 sock);

		std::vector<u8> get_dns_packet(u32 sock);
		s32 analyze_dns_packet(s32 s, const u8* buf, u32 len);

	private:
		shared_mutex mutex;
		std::map<s32, std::queue<std::vector<u8>>> dns_spylist{};
		std::map<std::string, u32> switch_map{};
	};
} // namespace np
