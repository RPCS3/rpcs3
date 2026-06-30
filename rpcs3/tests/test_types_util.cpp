#include "stdafx.h"
#include <gtest/gtest.h>

extern atomic_t<bool> g_headless;

#define CHECK_COMPILATION_ERRORS 0
#define CHECK_DEATH 0

namespace utils
{
	TEST(Utils, read_write_to_ptr_array)
	{
		g_headless = true; // Disable exception popups

		// write_to_ptr without pos
		std::array<u8, sizeof(u64)> arr{};
		write_to_ptr(arr, u8(umax));
		EXPECT_EQ(arr.at(0), u8(umax));
		write_to_ptr(arr, u16(umax));
		EXPECT_EQ(*utils::bless<u16>(arr.data()), u16(umax));
		write_to_ptr(arr, u32(umax));
		EXPECT_EQ(*utils::bless<u32>(arr.data()), u32(umax));
		write_to_ptr(arr, u64(umax));
		EXPECT_EQ(*utils::bless<u64>(arr.data()), u64(umax));

		// write_to_ptr and read_from_ptr with pos
		std::array<u8, sizeof(u128)> arr2{};

		for (u8 i = 0; i < arr2.size(); i++)
		{
			write_to_ptr(arr2, i, i);
			EXPECT_EQ(arr2.at(i), i);
			EXPECT_EQ(read_from_ptr<u8>(arr2, i), i);
		}

		arr2 = {};
		for (u16 i = 0; i < arr2.size(); i += sizeof(u16))
		{
			write_to_ptr(arr2, i, i);
			EXPECT_EQ(*utils::bless<u16>(arr2.data() + i), i);
			EXPECT_EQ(read_from_ptr<u16>(arr2, i), i);
		}

		arr2 = {};
		for (u32 i = 0; i < arr2.size(); i += sizeof(u32))
		{
			write_to_ptr(arr2, i, i);
			EXPECT_EQ(*utils::bless<u32>(arr2.data() + i), i);
			EXPECT_EQ(read_from_ptr<u32>(arr2, i), i);
		}

		arr2 = {};
		for (u64 i = 0; i < arr2.size(); i += sizeof(u64))
		{
			write_to_ptr(arr2, i, i);
			EXPECT_EQ(*utils::bless<u64>(arr2.data() + i), i);
			EXPECT_EQ(read_from_ptr<u64>(arr2, i), i);
		}

		arr2 = {};
		for (usz i = 0; i < arr2.size(); i += sizeof(u128))
		{
			write_to_ptr(arr2, i, u128(i));
			const u128 exp_u128 = i;
			EXPECT_EQ(std::memcmp(arr2.data(), &exp_u128, sizeof(u128)), 0);
			const u128 val_u128 = read_from_ptr<u128>(arr2, i);
			EXPECT_EQ(std::memcmp(&val_u128, &exp_u128, sizeof(u128)), 0);
		}

		// write_to_ptr without pos
		std::array<u32, sizeof(u64) / sizeof(u32)> arr32{};
		write_to_ptr(arr32, u32(umax));
		EXPECT_EQ(*utils::bless<u32>(arr32.data()), u32(umax));
		write_to_ptr(arr32, u64(umax));
		EXPECT_EQ(*utils::bless<u64>(arr32.data()), u64(umax));

		// write_to_ptr and read_from_ptr with pos
		std::array<u32, sizeof(u128) / sizeof(u32)> arr32_2{};

		for (u32 i = 0; i < arr32_2.size() * sizeof(u32); i += sizeof(u32))
		{
			const usz index = i / sizeof(u32);
			write_to_ptr(arr32_2, index, i);
			EXPECT_EQ(*utils::bless<u32>(arr32_2.data() + index), i);
			EXPECT_EQ(read_from_ptr<u32>(arr32_2, index), i);
		}

		arr32_2 = {};
		for (u64 i = 0; i < arr32_2.size() * sizeof(u32); i += sizeof(u64))
		{
			const usz index = i / sizeof(u64);
			write_to_ptr(arr32_2, index, i);
			EXPECT_EQ(*utils::bless<u64>(arr32_2.data() + index), i);
			EXPECT_EQ(read_from_ptr<u64>(arr32_2, index), i);
		}

		arr32_2 = {};
		for (usz i = 0; i < arr32_2.size() * sizeof(u32); i += sizeof(u128))
		{
			const usz index = i / sizeof(u128);
			write_to_ptr(arr32_2, index, u128(i));
			const u128 exp_u128 = i;
			EXPECT_EQ(std::memcmp(arr32_2.data(), &exp_u128, sizeof(u128)), 0);
			const u128 val_u128 = read_from_ptr<u128>(arr32_2, index);
			EXPECT_EQ(std::memcmp(&val_u128, &exp_u128, sizeof(u128)), 0);
		}

		// These tests can take a long time and produce warnings, so let's only run them in local builds
#if CHECK_DEATH
		EXPECT_DEATH(write_to_ptr(arr, u128()), ".*");
		EXPECT_DEATH(write_to_ptr(arr2, arr2.size(), u8()), ".*");
		EXPECT_DEATH(write_to_ptr(arr2, arr2.size(), u16()), ".*");
		EXPECT_DEATH(write_to_ptr(arr2, arr2.size(), u32()), ".*");
		EXPECT_DEATH(write_to_ptr(arr2, arr2.size(), u64()), ".*");
		EXPECT_DEATH(write_to_ptr(arr2, arr2.size(), u128()), ".*");
		EXPECT_DEATH(read_from_ptr<u8>(arr2, arr2.size()), ".*");
		EXPECT_DEATH(read_from_ptr<u16>(arr2, arr2.size()), ".*");
		EXPECT_DEATH(read_from_ptr<u32>(arr2, arr2.size()), ".*");
		EXPECT_DEATH(read_from_ptr<u64>(arr2, arr2.size()), ".*");
		EXPECT_DEATH(read_from_ptr<u128>(arr2, arr2.size()), ".*");

		EXPECT_DEATH(write_to_ptr(arr32, u128()), ".*");
		EXPECT_DEATH(write_to_ptr(arr32_2, arr32_2.size(), u32()), ".*");
		EXPECT_DEATH(write_to_ptr(arr32_2, arr32_2.size(), u64()), ".*");
		EXPECT_DEATH(write_to_ptr(arr32_2, arr32_2.size(), u128()), ".*");
		EXPECT_DEATH(read_from_ptr<u32>(arr32_2, arr32_2.size()), ".*");
		EXPECT_DEATH(read_from_ptr<u64>(arr32_2, arr32_2.size()), ".*");
		EXPECT_DEATH(read_from_ptr<u128>(arr32_2, arr32_2.size()), ".*");
#endif
	}

	TEST(Utils, read_write_to_ptr_c_array)
	{
		g_headless = true; // Disable exception popups

		// write_to_ptr without pos
		u8 arr[sizeof(u64)]{};
		write_to_ptr(arr, u8(umax));
		EXPECT_EQ(*arr, u8(umax));
		write_to_ptr(arr, u16(umax));
		EXPECT_EQ(*utils::bless<u16>(arr + 0), u16(umax));
		write_to_ptr(arr, u32(umax));
		EXPECT_EQ(*utils::bless<u32>(arr + 0), u32(umax));
		write_to_ptr(arr, u64(umax));
		EXPECT_EQ(*utils::bless<u64>(arr + 0), u64(umax));

		// write_to_ptr and read_from_ptr with pos
		u8 arr2[sizeof(u128)]{};

		for (u8 i = 0; i < sizeof(arr2); i++)
		{
			write_to_ptr(arr2, i, i);
			EXPECT_EQ(arr2[i], i);
			EXPECT_EQ(read_from_ptr<u8>(arr2, i), i);
		}

		std::memset(arr2, 0, sizeof(arr2));
		for (u16 i = 0; i < sizeof(arr2); i += sizeof(u16))
		{
			write_to_ptr(arr2, i, i);
			EXPECT_EQ(*utils::bless<u16>(arr2 + i), i);
			EXPECT_EQ(read_from_ptr<u16>(arr2, i), i);
		}

		std::memset(arr2, 0, sizeof(arr2));
		for (u32 i = 0; i < sizeof(arr2); i += sizeof(u32))
		{
			write_to_ptr(arr2, i, i);
			EXPECT_EQ(*utils::bless<u32>(arr2 + i), i);
			EXPECT_EQ(read_from_ptr<u32>(arr2, i), i);
		}

		std::memset(arr2, 0, sizeof(arr2));
		for (u64 i = 0; i < sizeof(arr2); i += sizeof(u64))
		{
			write_to_ptr(arr2, i, i);
			EXPECT_EQ(*utils::bless<u64>(arr2 + i), i);
			EXPECT_EQ(read_from_ptr<u64>(arr2, i), i);
		}

		std::memset(arr2, 0, sizeof(arr2));
		for (usz i = 0; i < sizeof(arr2); i += sizeof(u128))
		{
			write_to_ptr(arr2, i, u128(i));
			const u128 exp_u128 = i;
			EXPECT_EQ(std::memcmp(arr2, &exp_u128, sizeof(u128)), 0);
			const u128 val_u128 = read_from_ptr<u128>(arr2, i);
			EXPECT_EQ(std::memcmp(&val_u128, &exp_u128, sizeof(u128)), 0);
		}

		// write_to_ptr without pos
		u32 arr32[sizeof(u64) / sizeof(u32)]{};
		write_to_ptr(arr32, u32(umax));
		EXPECT_EQ(*utils::bless<u32>(arr32 + 0), u32(umax));
		write_to_ptr(arr32, u64(umax));
		EXPECT_EQ(*utils::bless<u64>(arr32 + 0), u64(umax));

		// write_to_ptr and read_from_ptr with pos
		u32 arr32_2[sizeof(u128) / sizeof(u32)]{};

		std::memset(arr32_2, 0, sizeof(arr32_2));
		for (u32 i = 0; i < sizeof(arr32_2); i += sizeof(u32))
		{
			const usz index = i / sizeof(u32);
			write_to_ptr(arr32_2, index, i);
			EXPECT_EQ(*utils::bless<u32>(arr32_2 + index), i);
			EXPECT_EQ(read_from_ptr<u32>(arr32_2, index), i);
		}

		std::memset(arr32_2, 0, sizeof(arr32_2));
		for (u64 i = 0; i < sizeof(arr32_2); i += sizeof(u64))
		{
			const usz index = i / sizeof(u64);
			write_to_ptr(arr32_2, index, i);
			EXPECT_EQ(*utils::bless<u64>(arr32_2 + index), i);
			EXPECT_EQ(read_from_ptr<u64>(arr32_2, index), i);
		}

		std::memset(arr32_2, 0, sizeof(arr32_2));
		for (usz i = 0; i < sizeof(arr32_2); i += sizeof(u128))
		{
			const usz index = i / sizeof(u128);
			write_to_ptr(arr32_2, index, u128(i));
			const u128 exp_u128 = i;
			EXPECT_EQ(std::memcmp(arr32_2, &exp_u128, sizeof(u128)), 0);
			const u128 val_u128 = read_from_ptr<u128>(arr32_2, index);
			EXPECT_EQ(std::memcmp(&val_u128, &exp_u128, sizeof(u128)), 0);
		}

		// These tests can take a long time and produce warnings, so let's only run them in local builds
#if CHECK_DEATH
		EXPECT_DEATH(write_to_ptr(arr, u128()), ".*");
		EXPECT_DEATH(write_to_ptr(arr2, std::size(arr2), u8()), ".*");
		EXPECT_DEATH(write_to_ptr(arr2, std::size(arr2), u16()), ".*");
		EXPECT_DEATH(write_to_ptr(arr2, std::size(arr2), u32()), ".*");
		EXPECT_DEATH(write_to_ptr(arr2, std::size(arr2), u64()), ".*");
		EXPECT_DEATH(write_to_ptr(arr2, std::size(arr2), u128()), ".*");
		EXPECT_DEATH(read_from_ptr<u8>(arr2, std::size(arr2)), ".*");
		EXPECT_DEATH(read_from_ptr<u16>(arr2, std::size(arr2)), ".*");
		EXPECT_DEATH(read_from_ptr<u32>(arr2, std::size(arr2)), ".*");
		EXPECT_DEATH(read_from_ptr<u64>(arr2, std::size(arr2)), ".*");
		EXPECT_DEATH(read_from_ptr<u128>(arr2, std::size(arr2)), ".*");

		EXPECT_DEATH(write_to_ptr(arr32, u128()), ".*");
		EXPECT_DEATH(write_to_ptr(arr32_2, std::size(arr32_2), u32()), ".*");
		EXPECT_DEATH(write_to_ptr(arr32_2, std::size(arr32_2), u64()), ".*");
		EXPECT_DEATH(write_to_ptr(arr32_2, std::size(arr32_2), u128()), ".*");
		EXPECT_DEATH(read_from_ptr<u32>(arr32_2, std::size(arr32_2)), ".*");
		EXPECT_DEATH(read_from_ptr<u64>(arr32_2, std::size(arr32_2)), ".*");
		EXPECT_DEATH(read_from_ptr<u128>(arr32_2, std::size(arr32_2)), ".*");
#endif
	}

	TEST(Utils, read_write_to_ptr_unsafe)
	{
		g_headless = true; // Disable exception popups

		// write_to_ptr without pos
		constexpr usz arr_size = sizeof(u64);
		u8* arr = new u8[arr_size];
		std::memset(arr, 0, arr_size);

		write_to_ptr_unsafe(arr, u8(umax));
		EXPECT_EQ(*arr, u8(umax));
		write_to_ptr_unsafe(arr, u16(umax));
		EXPECT_EQ(*utils::bless<u16>(arr), u16(umax));
		write_to_ptr_unsafe(arr, u32(umax));
		EXPECT_EQ(*utils::bless<u32>(arr), u32(umax));
		write_to_ptr_unsafe(arr, u64(umax));
		EXPECT_EQ(*utils::bless<u64>(arr), u64(umax));

		// write_to_ptr and read_from_ptr with pos
		constexpr usz arr2_size = sizeof(u128);
		u8* arr2 = new u8[arr2_size];

		std::memset(arr2, 0, arr2_size);
		for (u8 i = 0; i < arr2_size; i++)
		{
			write_to_ptr_unsafe(arr2, i, i);
			EXPECT_EQ(arr2[i], i);
			EXPECT_EQ(read_from_ptr_unsafe<u8>(arr2, i), i);
		}

		std::memset(arr2, 0, arr2_size);
		for (u16 i = 0; i < arr2_size; i += sizeof(u16))
		{
			write_to_ptr_unsafe(arr2, i, i);
			EXPECT_EQ(*utils::bless<u16>(arr2 + i), i);
			EXPECT_EQ(read_from_ptr_unsafe<u16>(arr2, i), i);
		}

		std::memset(arr2, 0, arr2_size);
		for (u32 i = 0; i < arr2_size; i += sizeof(u32))
		{
			write_to_ptr_unsafe(arr2, i, i);
			EXPECT_EQ(*utils::bless<u32>(arr2 + i), i);
			EXPECT_EQ(read_from_ptr_unsafe<u32>(arr2, i), i);
		}

		std::memset(arr2, 0, arr2_size);
		for (usz i = 0; i < arr2_size; i += sizeof(u64))
		{
			write_to_ptr_unsafe(arr2, i, u64(i));
			EXPECT_EQ(*utils::bless<u64>(arr2 + i), i);
			EXPECT_EQ(read_from_ptr_unsafe<u64>(arr2, i), i);
		}

		std::memset(arr2, 0, arr2_size);
		for (usz i = 0; i < arr2_size; i += sizeof(u128))
		{
			write_to_ptr_unsafe(arr2, i, u128(i));
			const u128 exp_u128 = i;
			EXPECT_EQ(std::memcmp(arr2, &exp_u128, sizeof(u128)), 0);
			const u128 val_u128 = read_from_ptr_unsafe<u128>(arr2, i);
			EXPECT_EQ(std::memcmp(&val_u128, &exp_u128, sizeof(u128)), 0);
		}

		// write_to_ptr without pos
		constexpr usz arr32_count = sizeof(u64) / sizeof(u32);
		constexpr usz arr32_size = arr32_count * sizeof(u32);
		u32* arr32 = new u32[arr32_count];
		std::memset(arr32, 0, arr32_size);

		write_to_ptr_unsafe(arr32, u32(umax));
		EXPECT_EQ(*utils::bless<u32>(arr32), u32(umax));
		write_to_ptr_unsafe(arr32, u64(umax));
		EXPECT_EQ(*utils::bless<u64>(arr32), u64(umax));

		// write_to_ptr and read_from_ptr with pos
		constexpr usz arr32_2_count = sizeof(u128) / sizeof(u32);
		constexpr usz arr32_2_size = arr32_2_count * sizeof(u32);
		u32* arr32_2 = new u32[arr32_2_count];

		std::memset(arr32_2, 0, arr32_2_size);
		for (u32 i = 0; i < arr32_2_size; i += sizeof(u32))
		{
			const usz index = i / sizeof(u32);
			write_to_ptr_unsafe(arr32_2, index, i);
			EXPECT_EQ(*utils::bless<u32>(arr32_2 + index), i);
			EXPECT_EQ(read_from_ptr_unsafe<u32>(arr32_2, index), i);
		}

		std::memset(arr32_2, 0, arr32_2_size);
		for (u64 i = 0; i < arr32_2_size; i += sizeof(u64))
		{
			const usz index = i / sizeof(u64);
			write_to_ptr_unsafe(arr32_2, index, i);
			EXPECT_EQ(*utils::bless<u64>(arr32_2 + index), i);
			EXPECT_EQ(read_from_ptr_unsafe<u64>(arr32_2, index), i);
		}

		std::memset(arr32_2, 0, arr32_2_size);
		for (usz i = 0; i < arr32_2_size; i += sizeof(u128))
		{
			const usz index = i / sizeof(u128);
			write_to_ptr_unsafe(arr32_2, index, u128(i));
			const u128 exp_u128 = i;
			EXPECT_EQ(std::memcmp(arr32_2, &exp_u128, sizeof(u128)), 0);
			const u128 val_u128 = read_from_ptr_unsafe<u128>(arr32_2, index);
			EXPECT_EQ(std::memcmp(&val_u128, &exp_u128, sizeof(u128)), 0);
		}

		delete[] arr;
		delete[] arr2;
		delete[] arr32;
		delete[] arr32_2;
	}

	TEST(Utils, read_from_ptr_const_eval_array)
	{
		g_headless = true; // Disable exception popups

		constexpr std::array<u8, sizeof(u64)> arr { 1, 2, 3, 4, 5, 6, 7, 8 };

		static_assert(read_from_ptr<u8>(arr, 0) == u8(1));
		static_assert(read_from_ptr<u8>(arr, 1) == u8(2));
		static_assert(read_from_ptr<u8>(arr, 2) == u8(3));
		static_assert(read_from_ptr<u8>(arr, 3) == u8(4));
		static_assert(read_from_ptr<u8>(arr, 4) == u8(5));
		static_assert(read_from_ptr<u8>(arr, 5) == u8(6));
		static_assert(read_from_ptr<u8>(arr, 6) == u8(7));
		static_assert(read_from_ptr<u8>(arr, 7) == u8(8));

		static_assert(read_from_ptr<le_t<u16>>(arr, 0) == u16(0x0201));
		static_assert(read_from_ptr<le_t<u16>>(arr, 2) == u16(0x0403));
		static_assert(read_from_ptr<le_t<u16>>(arr, 4) == u16(0x0605));
		static_assert(read_from_ptr<le_t<u16>>(arr, 6) == u16(0x0807));

		static_assert(read_from_ptr<le_t<u32>>(arr, 0) == u32(0x04030201));
		static_assert(read_from_ptr<le_t<u32>>(arr, 4) == u32(0x08070605));

		static_assert(read_from_ptr<le_t<u64>>(arr, 0) == u64(0x0807060504030201));

		constexpr std::array<u32, sizeof(u64) / sizeof(u32)> arr32 { 0x00000000, 0xFFFFFFFF };

		static_assert(read_from_ptr<u32>(arr32, 0) == u32(0x00000000));
		static_assert(read_from_ptr<u32>(arr32, 1) == u32(0xFFFFFFFF));

		static_assert(read_from_ptr<le_t<u64>>(arr32, 0) == u64(0xFFFFFFFF00000000));

		// This should not compile
#if CHECK_COMPILATION_ERRORS
		constexpr auto v8 = read_from_ptr<u8>(arr, arr.size());
		constexpr auto v16 = read_from_ptr<u16>(arr, arr.size() + 1 - sizeof(u16));
		constexpr auto v32 = read_from_ptr<u32>(arr, arr.size() + 1 - sizeof(u32));
		constexpr auto v64 = read_from_ptr<u64>(arr, arr.size() + 1 - sizeof(u64));
		constexpr auto v128 = read_from_ptr<u128>(arr, 0);

		constexpr auto v32_32 = read_from_ptr<u32>(arr32, arr32.size());
		constexpr auto v64_32 = read_from_ptr<u64>(arr32, arr32.size() + 1 - sizeof(u64) / sizeof(u32));
		constexpr auto v128_32 = read_from_ptr<u128>(arr32, 0);
#endif
	}

	TEST(Utils, read_from_ptr_const_eval_c_array)
	{
		g_headless = true; // Disable exception popups

		constexpr u8 arr[sizeof(u64)] { 1, 2, 3, 4, 5, 6, 7, 8 };

		static_assert(read_from_ptr<u8>(arr, 0) == u8(1));
		static_assert(read_from_ptr<u8>(arr, 1) == u8(2));
		static_assert(read_from_ptr<u8>(arr, 2) == u8(3));
		static_assert(read_from_ptr<u8>(arr, 3) == u8(4));
		static_assert(read_from_ptr<u8>(arr, 4) == u8(5));
		static_assert(read_from_ptr<u8>(arr, 5) == u8(6));
		static_assert(read_from_ptr<u8>(arr, 6) == u8(7));
		static_assert(read_from_ptr<u8>(arr, 7) == u8(8));

		static_assert(read_from_ptr<le_t<u16>>(arr, 0) == u16(0x0201));
		static_assert(read_from_ptr<le_t<u16>>(arr, 2) == u16(0x0403));
		static_assert(read_from_ptr<le_t<u16>>(arr, 4) == u16(0x0605));
		static_assert(read_from_ptr<le_t<u16>>(arr, 6) == u16(0x0807));

		static_assert(read_from_ptr<le_t<u32>>(arr, 0) == u32(0x04030201));
		static_assert(read_from_ptr<le_t<u32>>(arr, 4) == u32(0x08070605));

		static_assert(read_from_ptr<le_t<u64>>(arr, 0) == u64(0x0807060504030201));

		constexpr u32 arr32[sizeof(u64) / sizeof(u32)] { 0x00000000, 0xFFFFFFFF };

		static_assert(read_from_ptr<u32>(arr32, 0) == u32(0x00000000));
		static_assert(read_from_ptr<u32>(arr32, 1) == u32(0xFFFFFFFF));

		static_assert(read_from_ptr<le_t<u64>>(arr32, 0) == u64(0xFFFFFFFF00000000));

		// This should not compile
#if CHECK_COMPILATION_ERRORS
		constexpr auto v8 = read_from_ptr<u8>(arr, std::size(arr));
		constexpr auto v16 = read_from_ptr<u16>(arr, std::size(arr) + 1 - sizeof(u16));
		constexpr auto v32 = read_from_ptr<u32>(arr, std::size(arr) + 1 - sizeof(u32));
		constexpr auto v64 = read_from_ptr<u64>(arr, std::size(arr) + 1 - sizeof(u64));
		constexpr auto v128 = read_from_ptr<u128>(arr, 0);

		constexpr auto v32_32 = read_from_ptr<u32>(arr32, std::size(arr32));
		constexpr auto v64_32 = read_from_ptr<u64>(arr32, std::size(arr32) + 1 - sizeof(u64) / sizeof(u32));
		constexpr auto v128_32 = read_from_ptr<u128>(arr32, 0);
#endif
	}
}
