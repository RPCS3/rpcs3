#pragma once

#include "Loader/ISO.h"
#include "util/atomic.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>

extern atomic_t<bool> g_headless;

class iso_test : public testing::Test
{
protected:
	std::filesystem::path m_directory;
	std::string m_path;
	std::vector<u8> m_image;
	bool m_was_headless = false;

	void SetUp() override
	{
		m_was_headless = g_headless.exchange(true);
		const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
		for (u32 attempt = 0;; attempt++)
		{
			m_directory = std::filesystem::temp_directory_path() / ("rpcs3-iso-test-" + std::to_string(stamp) + "-" + std::to_string(attempt));
			if (std::filesystem::create_directory(m_directory))
			{
				break;
			}
		}

		m_path = (m_directory / "test.iso").string();
		m_image.resize(24 * ISO_SECTOR_SIZE);
		m_image[3] = 1; // One unencrypted PS3 region.
		m_image[15] = 23; // Last sector in the region.

		auto both_endian = [&](usz offset, u32 value, usz width)
		{
			for (usz i = 0; i < width; i++)
			{
				m_image[offset + i] = static_cast<u8>(value >> (i * 8));
				m_image[offset + width + i] = static_cast<u8>(value >> ((width - i - 1) * 8));
			}
		};

		auto record = [&](usz offset, std::string_view name, u32 sector, u32 size, u8 flags)
		{
			const usz length = 33 + name.size() + (name.size() % 2 == 0);
			m_image[offset] = static_cast<u8>(length);
			both_endian(offset + 2, sector, 4);
			both_endian(offset + 10, size, 4);
			m_image[offset + 18] = 126;
			m_image[offset + 19] = 9;
			m_image[offset + 20] = 4;
			m_image[offset + 25] = flags;
			both_endian(offset + 28, 1, 2);
			m_image[offset + 32] = static_cast<u8>(name.size());
			std::copy(name.begin(), name.end(), m_image.begin() + offset + 33);
			return length;
		};

		const usz pvd = 16 * ISO_SECTOR_SIZE;
		m_image[pvd] = 1;
		std::memcpy(m_image.data() + pvd + 1, "CD001", 5);
		m_image[pvd + 6] = 1;
		both_endian(pvd + 80, 24, 4);
		both_endian(pvd + 120, 1, 2);
		both_endian(pvd + 124, 1, 2);
		both_endian(pvd + 128, ISO_SECTOR_SIZE, 2);
		record(pvd + 156, std::string_view("\0", 1), 18, ISO_SECTOR_SIZE, 2);
		m_image[17 * ISO_SECTOR_SIZE] = 255;
		std::memcpy(m_image.data() + 17 * ISO_SECTOR_SIZE + 1, "CD001", 5);
		m_image[17 * ISO_SECTOR_SIZE + 6] = 1;
		usz entry = 18 * ISO_SECTOR_SIZE;
		entry += record(entry, std::string_view("\0", 1), 18, ISO_SECTOR_SIZE, 2);
		entry += record(entry, std::string_view("\1", 1), 18, ISO_SECTOR_SIZE, 2);
		record(entry, "TEST.BIN;1", 20, 3, 0);
		std::memcpy(m_image.data() + 20 * ISO_SECTOR_SIZE, "abcBAD", 6);
		std::memcpy(m_image.data() + 22 * ISO_SECTOR_SIZE, "xyzBAD", 6);

		std::ofstream out(m_path, std::ios::binary);
		out.write(reinterpret_cast<const char*>(m_image.data()), m_image.size());
		ASSERT_TRUE(out.good());
	}

	void TearDown() override
	{
		std::filesystem::remove_all(m_directory);
		g_headless = m_was_headless;
	}
};
