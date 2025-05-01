#include <gtest/gtest.h>
#include "Emu/RSX/Common/simple_array.hpp"

using namespace rsx;

TEST(SimpleArray, DefaultConstructor)
{
    simple_array<int> arr;
    EXPECT_TRUE(arr.empty());
    EXPECT_EQ(arr.size(), 0);
    EXPECT_GE(arr.capacity(), 1);
}

TEST(SimpleArray, InitialSizeConstructor)
{
    simple_array<int> arr(5);
    EXPECT_FALSE(arr.empty());
    EXPECT_EQ(arr.size(), 5);
    EXPECT_GE(arr.capacity(), 5);
}

TEST(SimpleArray, InitialSizeValueConstructor)
{
    simple_array<int> arr(3, 42);
    EXPECT_EQ(arr.size(), 3);
    for (int i = 0; i < 3; ++i) {
        EXPECT_EQ(arr[i], 42);
    }
}

TEST(SimpleArray, InitializerListConstructor)
{
    simple_array<int> arr{1, 2, 3, 4, 5};
    EXPECT_EQ(arr.size(), 5);
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(arr[i], i + 1);
    }
}

TEST(SimpleArray, CopyConstructor)
{
    simple_array<int> arr1{1, 2, 3};
    simple_array<int> arr2(arr1);
    EXPECT_EQ(arr1.size(), arr2.size());
    for (u32 i = 0; i < arr1.size(); ++i) {
        EXPECT_EQ(arr1[i], arr2[i]);
    }
}

TEST(SimpleArray, MoveConstructor)
{
    simple_array<int> arr1{1, 2, 3};
    u32 original_size = arr1.size();
    simple_array<int> arr2(std::move(arr1));
    EXPECT_EQ(arr2.size(), original_size);
    EXPECT_TRUE(arr1.empty());
}

TEST(SimpleArray, PushBackAndAccess)
{
    simple_array<int> arr;
    arr.push_back(1);
    arr.push_back(2);
    arr.push_back(3);
    
    EXPECT_EQ(arr.size(), 3);
    EXPECT_EQ(arr[0], 1);
    EXPECT_EQ(arr[1], 2);
    EXPECT_EQ(arr[2], 3);
    EXPECT_EQ(arr.front(), 1);
    EXPECT_EQ(arr.back(), 3);
}

TEST(SimpleArray, PopBack)
{
    simple_array<int> arr{1, 2, 3};
    EXPECT_EQ(arr.pop_back(), 3);
    EXPECT_EQ(arr.size(), 2);
    EXPECT_EQ(arr.back(), 2);
}

TEST(SimpleArray, Insert)
{
    simple_array<int> arr{1, 3, 4};
    auto it = arr.insert(arr.begin() + 1, 2);
    EXPECT_EQ(*it, 2);
    EXPECT_EQ(arr.size(), 4);
    for (int i = 0; i < 4; ++i) {
        EXPECT_EQ(arr[i], i + 1);
    }
}

TEST(SimpleArray, Clear)
{
    simple_array<int> arr{1, 2, 3};
    arr.clear();
    EXPECT_TRUE(arr.empty());
    EXPECT_EQ(arr.size(), 0);
}

TEST(SimpleArray, SmallBufferOptimization)
{
    // Test with a small type that should use stack storage
    simple_array<char> small_arr(3, 'a');
    EXPECT_TRUE(small_arr.is_local_storage());
    
    // Test with a larger type or more elements that should use heap storage
    struct LargeType { char data[128]; };
    simple_array<LargeType> large_arr(10);
    EXPECT_FALSE(large_arr.is_local_storage());
}

TEST(SimpleArray, Iterator)
{
    simple_array<int> arr{1, 2, 3, 4, 5};
    int sum = 0;
    for (const auto& val : arr) {
        sum += val;
    }
    EXPECT_EQ(sum, 15);
}

TEST(SimpleArray, EraseIf)
{
    simple_array<int> arr{1, 2, 3, 4, 5};
    bool modified = arr.erase_if([](const int& val) { return val % 2 == 0; });
    EXPECT_TRUE(modified);
    EXPECT_EQ(arr.size(), 3);
    EXPECT_EQ(arr[0], 1);
    EXPECT_EQ(arr[1], 3);
    EXPECT_EQ(arr[2], 5);
}

TEST(SimpleArray, Map)
{
    simple_array<int> arr{1, 2, 3};
    auto result = arr.map([](const int& val) { return val * 2; });
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], 2);
    EXPECT_EQ(result[1], 4);
    EXPECT_EQ(result[2], 6);
}

TEST(SimpleArray, Reduce)
{
    simple_array<int> arr{1, 2, 3, 4, 5};
    int sum = arr.reduce(0, [](const int& acc, const int& val) { return acc + val; });
    EXPECT_EQ(sum, 15);
}

TEST(SimpleArray, Any)
{
    simple_array<int> arr{1, 2, 3, 4, 5};
    EXPECT_TRUE(arr.any([](const int& val) { return val > 3; }));
    EXPECT_FALSE(arr.any([](const int& val) { return val > 5; }));
}

TEST(SimpleArray, Sort)
{
    simple_array<int> arr{5, 3, 1, 4, 2};
    arr.sort([](const int& a, const int& b) { return a < b; });
    for (u32 i = 0; i < arr.size(); ++i) {
        EXPECT_EQ(arr[i], i + 1);
    }
}
