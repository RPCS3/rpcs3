#pragma once

#include <unordered_map>
#include <queue>
#include <vector>
#include <optional>

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
		std::optional<u32> get_redir(const std::string& hostname);

	private:
		shared_mutex mutex;
		std::unordered_map<s32, std::queue<std::vector<u8>>> m_dns_spylist;
		std::vector<std::pair<std::string, u32>> m_redirs;
	};
} // namespace np
