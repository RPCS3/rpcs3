#include <gtest/gtest.h>

#define private public
#include "Emu/Cell/lv2/sys_fs.h"
#undef private

using namespace utils;

namespace utils
{
	TEST(cellFs, PathRoot)
	{
		std::string path = "/.";
		auto [root, trail] = lv2_fs_object::get_path_root_and_trail(path);
		EXPECT_TRUE(root.empty());
		EXPECT_TRUE(trail.empty());

		path = "/./././dev_bdvd/./";
		std::tie(root, trail) = lv2_fs_object::get_path_root_and_trail(path);
		EXPECT_EQ(root, "dev_bdvd"sv);
		EXPECT_TRUE(trail.empty());

		path = "/../";
		std::tie(root, trail) = lv2_fs_object::get_path_root_and_trail(path);
		EXPECT_TRUE(root.empty());
		EXPECT_EQ(trail, "ENOENT"sv);
	}

	TEST(cellFs, PathSimplify)
	{
		std::string path = "/dev_hdd0/";
		auto [root, trail] = lv2_fs_object::get_path_root_and_trail(path);
		EXPECT_EQ(root, "dev_hdd0"sv);
		EXPECT_TRUE(trail.empty());

		path = "/dev_hdd0/game";
		std::tie(root, trail) = lv2_fs_object::get_path_root_and_trail(path);
		EXPECT_EQ(root, "dev_hdd0"sv);
		EXPECT_EQ(trail, "game"sv);

		path = "/dev_hdd0/game/NP1234567";
		std::tie(root, trail) = lv2_fs_object::get_path_root_and_trail(path);
		EXPECT_EQ(root, "dev_hdd0"sv);
		EXPECT_EQ(trail, "game/NP1234567"sv);

		path = "/dev_hdd0/game/NP1234567/../../NP1234568/.";
		std::tie(root, trail) = lv2_fs_object::get_path_root_and_trail(path);
		EXPECT_EQ(root, "dev_hdd0"sv);
		EXPECT_EQ(trail, "NP1234568"sv);
	}
}
