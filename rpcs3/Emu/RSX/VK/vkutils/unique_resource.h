#pragma once

#include <util/types.hpp>

namespace vk
{
	u64 gen_uid();

	class unique_resource
	{
	public:
		unique_resource()
			: m_uid(gen_uid())
		{}

		u64 uid() const { return m_uid; }
		bool operator == (const unique_resource& other) const { return m_uid == other.m_uid; }

	private:
		u64 m_uid = 0ull;
	};
}
