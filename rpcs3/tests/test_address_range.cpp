#include <gtest/gtest.h>

#define private public
#include "Utilities/address_range.h"
#undef private

using namespace utils;

namespace utils
{
	TEST(AddressRange, Constructors)
	{
		// Default constructor
		address_range32 empty;
		EXPECT_FALSE(empty.valid());
		EXPECT_EQ(empty.start, umax);
		EXPECT_EQ(empty.end, 0);

		// Static factory methods
		address_range32 r1 = address_range32::start_length(0x1000, 0x1000);
		EXPECT_EQ(r1.start, 0x1000);
		EXPECT_EQ(r1.end, 0x1FFF);
		EXPECT_EQ(r1.length(), 0x1000);
		EXPECT_TRUE(r1.valid());

		address_range32 r2 = address_range32::start_end(0x2000, 0x2FFF);
		EXPECT_EQ(r2.start, 0x2000);
		EXPECT_EQ(r2.end, 0x2FFF);
		EXPECT_EQ(r2.length(), 0x1000);
		EXPECT_TRUE(r2.valid());

		// Edge cases
		address_range32 zero_length = address_range32::start_length(0x1000, 0);
		EXPECT_FALSE(zero_length.valid());

		address_range32 single_byte = address_range32::start_length(0x1000, 1);
		EXPECT_TRUE(single_byte.valid());
		EXPECT_EQ(single_byte.start, 0x1000);
		EXPECT_EQ(single_byte.end, 0x1000);
		EXPECT_EQ(single_byte.length(), 1);
	}

	TEST(AddressRange, LengthAndBoundaries)
	{
		address_range32 r = address_range32::start_length(0x1000, 0x1000);

		// Test length
		EXPECT_EQ(r.length(), 0x1000);

		// Test set_length
		r.set_length(0x2000);
		EXPECT_EQ(r.start, 0x1000);
		EXPECT_EQ(r.end, 0x2FFF);
		EXPECT_EQ(r.length(), 0x2000);

		// Test next_address and prev_address
		EXPECT_EQ(r.next_address(), 0x3000);
		EXPECT_EQ(r.prev_address(), 0xFFF);
	}

	TEST(AddressRange, Overlapping)
	{
		address_range32 r1 = address_range32::start_length(0x1000, 0x1000); // 0x1000-0x1FFF

		// Complete overlap
		address_range32 r2 = address_range32::start_length(0x1000, 0x1000); // 0x1000-0x1FFF
		EXPECT_TRUE(r1.overlaps(r2));
		EXPECT_TRUE(r2.overlaps(r1));

		// Partial overlap at start
		address_range32 r3 = address_range32::start_length(0x800, 0x1000); // 0x800-0x17FF
		EXPECT_TRUE(r1.overlaps(r3));
		EXPECT_TRUE(r3.overlaps(r1));

		// Partial overlap at end
		address_range32 r4 = address_range32::start_length(0x1800, 0x1000); // 0x1800-0x27FF
		EXPECT_TRUE(r1.overlaps(r4));
		EXPECT_TRUE(r4.overlaps(r1));

		// No overlap, before
		address_range32 r5 = address_range32::start_length(0x0, 0x1000); // 0x0-0xFFF
		EXPECT_FALSE(r1.overlaps(r5));
		EXPECT_FALSE(r5.overlaps(r1));

		// No overlap, after
		address_range32 r6 = address_range32::start_length(0x2000, 0x1000); // 0x2000-0x2FFF
		EXPECT_FALSE(r1.overlaps(r6));
		EXPECT_FALSE(r6.overlaps(r1));

		// Single address overlap at start
		address_range32 r7 = address_range32::start_length(0x800, 0x801); // 0x800-0x1000
		EXPECT_TRUE(r1.overlaps(r7));
		EXPECT_TRUE(r7.overlaps(r1));

		// Single address overlap at end
		address_range32 r8 = address_range32::start_length(0x1FFF, 0x1000); // 0x1FFF-0x2FFE
		EXPECT_TRUE(r1.overlaps(r8));
		EXPECT_TRUE(r8.overlaps(r1));

		// Address overlap test
		EXPECT_TRUE(r1.overlaps(0x1000));  // Start boundary
		EXPECT_TRUE(r1.overlaps(0x1FFF));  // End boundary
		EXPECT_TRUE(r1.overlaps(0x1800));  // Middle
		EXPECT_FALSE(r1.overlaps(0xFFF));  // Just before
		EXPECT_FALSE(r1.overlaps(0x2000)); // Just after
	}

	TEST(AddressRange, Inside)
	{
		address_range32 r1 = address_range32::start_length(0x1000, 0x1000); // 0x1000-0x1FFF

		// Same range
		address_range32 r2 = address_range32::start_length(0x1000, 0x1000); // 0x1000-0x1FFF
		EXPECT_TRUE(r1.inside(r2));
		EXPECT_TRUE(r2.inside(r1));

		// Smaller range inside
		address_range32 r3 = address_range32::start_length(0x1200, 0x800); // 0x1200-0x19FF
		EXPECT_TRUE(r3.inside(r1));
		EXPECT_FALSE(r1.inside(r3));

		// Larger range outside
		address_range32 r4 = address_range32::start_length(0x800, 0x2000); // 0x800-0x27FF
		EXPECT_TRUE(r1.inside(r4));
		EXPECT_FALSE(r4.inside(r1));

		// Partially overlapping
		address_range32 r5 = address_range32::start_length(0x1800, 0x1000); // 0x1800-0x27FF
		EXPECT_FALSE(r1.inside(r5));
		EXPECT_FALSE(r5.inside(r1));

		// No overlap
		address_range32 r6 = address_range32::start_length(0x3000, 0x1000); // 0x3000-0x3FFF
		EXPECT_FALSE(r1.inside(r6));
		EXPECT_FALSE(r6.inside(r1));
	}

	TEST(AddressRange, Touches)
	{
		address_range32 r1 = address_range32::start_length(0x1000, 0x1000); // 0x1000-0x1FFF

		// Same range (overlaps)
		address_range32 r2 = address_range32::start_length(0x1000, 0x1000); // 0x1000-0x1FFF
		EXPECT_TRUE(r1.touches(r2));

		// Overlapping ranges
		address_range32 r3 = address_range32::start_length(0x1800, 0x1000); // 0x1800-0x27FF
		EXPECT_TRUE(r1.touches(r3));

		// Adjacent at end of r1
		address_range32 r4 = address_range32::start_length(0x2000, 0x1000); // 0x2000-0x2FFF
		EXPECT_TRUE(r1.touches(r4));
		EXPECT_TRUE(r4.touches(r1));

		// Adjacent at start of r1
		address_range32 r5 = address_range32::start_length(0x0, 0x1000); // 0x0-0xFFF
		EXPECT_TRUE(r1.touches(r5));
		EXPECT_TRUE(r5.touches(r1));

		// Not touching
		address_range32 r6 = address_range32::start_length(0x3000, 0x1000); // 0x3000-0x3FFF
		EXPECT_FALSE(r1.touches(r6));
		EXPECT_FALSE(r6.touches(r1));
	}

	TEST(AddressRange, Distance)
	{
		address_range32 r1 = address_range32::start_length(0x1000, 0x1000); // 0x1000-0x1FFF

		// Touching ranges
		address_range32 r2 = address_range32::start_length(0x2000, 0x1000); // 0x2000-0x2FFF
		EXPECT_EQ(r1.distance(r2), 0);
		EXPECT_EQ(r2.distance(r1), 0);
		EXPECT_EQ(r1.signed_distance(r2), 0);
		EXPECT_EQ(r2.signed_distance(r1), 0);

		// Gap of 0x1000 (r3 after r1)
		address_range32 r3 = address_range32::start_length(0x3000, 0x1000); // 0x3000-0x3FFF
		EXPECT_EQ(r1.distance(r3), 0x1000);
		EXPECT_EQ(r3.distance(r1), 0x1000);
		EXPECT_EQ(r1.signed_distance(r3), 0x1000);
		EXPECT_EQ(r3.signed_distance(r1), -0x1000);

		// Gap of 0x1000 (r4 before r1)
		address_range32 r4 = address_range32::start_end(0, 0xEFF); // 0x0-0xEFF
		EXPECT_EQ(r1.distance(r4), 0x100);
		EXPECT_EQ(r4.distance(r1), 0x100);
		EXPECT_EQ(r1.signed_distance(r4), -0x100);
		EXPECT_EQ(r4.signed_distance(r1), 0x100);

		// Overlapping ranges
		address_range32 r5 = address_range32::start_length(0x1800, 0x1000); // 0x1800-0x27FF
		EXPECT_EQ(r1.distance(r5), 0);
		EXPECT_EQ(r5.distance(r1), 0);
		EXPECT_EQ(r1.signed_distance(r5), 0);
		EXPECT_EQ(r5.signed_distance(r1), 0);
	}

	TEST(AddressRange, MinMax)
	{
		address_range32 r1 = address_range32::start_length(0x1000, 0x1000); // 0x1000-0x1FFF
		address_range32 r2 = address_range32::start_length(0x1800, 0x1000); // 0x1800-0x27FF

		// Get min-max
		address_range32 min_max = r1.get_min_max(r2);
		EXPECT_EQ(min_max.start, 0x1000);
		EXPECT_EQ(min_max.end, 0x27FF);

		// Set min-max
		address_range32 r3 = address_range32::start_length(0x2000, 0x1000); // 0x2000-0x2FFF
		r3.set_min_max(r1);
		EXPECT_EQ(r3.start, 0x1000);
		EXPECT_EQ(r3.end, 0x2FFF);

		// Test with invalid ranges
		address_range32 empty;
		address_range32 min_max2 = r1.get_min_max(empty);
		EXPECT_EQ(min_max2.start, r1.start);
		EXPECT_EQ(min_max2.end, r1.end);

		address_range32 min_max3 = empty.get_min_max(r1);
		EXPECT_EQ(min_max3.start, r1.start);
		EXPECT_EQ(min_max3.end, r1.end);

		address_range32 min_max4 = empty.get_min_max(empty);
		EXPECT_EQ(min_max4.start, umax);
		EXPECT_EQ(min_max4.end, 0);
	}

	TEST(AddressRange, Intersect)
	{
		address_range32 r1 = address_range32::start_length(0x1000, 0x2000); // 0x1000-0x2FFF

		// Complete overlap
		address_range32 r2 = address_range32::start_length(0x0, 0x4000); // 0x0-0x3FFF
		address_range32 i1 = r1.get_intersect(r2);
		EXPECT_EQ(i1.start, 0x1000);
		EXPECT_EQ(i1.end, 0x2FFF);

		// Partial overlap at start
		address_range32 r3 = address_range32::start_length(0x0, 0x2000); // 0x0-0x1FFF
		address_range32 i2 = r1.get_intersect(r3);
		EXPECT_EQ(i2.start, 0x1000);
		EXPECT_EQ(i2.end, 0x1FFF);

		// Partial overlap at end
		address_range32 r4 = address_range32::start_length(0x2000, 0x2000); // 0x2000-0x3FFF
		address_range32 i3 = r1.get_intersect(r4);
		EXPECT_EQ(i3.start, 0x2000);
		EXPECT_EQ(i3.end, 0x2FFF);

		// No overlap
		address_range32 r5 = address_range32::start_length(0x4000, 0x1000); // 0x4000-0x4FFF
		address_range32 i4 = r1.get_intersect(r5);
		EXPECT_FALSE(i4.valid());

		// Test intersect method
		address_range32 r6 = address_range32::start_length(0x1000, 0x2000); // 0x1000-0x2FFF
		r6.intersect(r3);
		EXPECT_EQ(r6.start, 0x1000);
		EXPECT_EQ(r6.end, 0x1FFF);
	}

	TEST(AddressRange, Validity)
	{
		// Valid range
		address_range32 r1 = address_range32::start_length(0x1000, 0x1000);
		EXPECT_TRUE(r1.valid());

		// Invalid range (default constructor)
		address_range32 r2;
		EXPECT_FALSE(r2.valid());

		// Invalid range (start > end)
		address_range32 r3 = address_range32::start_end(0x2000, 0x1000);
		EXPECT_FALSE(r3.valid());

		// Invalidate
		r1.invalidate();
		EXPECT_FALSE(r1.valid());
		EXPECT_EQ(r1.start, umax);
		EXPECT_EQ(r1.end, 0);
	}

	TEST(AddressRange, Comparison)
	{
		address_range32 r1 = address_range32::start_length(0x1000, 0x1000);
		address_range32 r2 = address_range32::start_length(0x1000, 0x1000);
		address_range32 r3 = address_range32::start_length(0x2000, 0x1000);

		EXPECT_TRUE(r1 == r2);
		EXPECT_FALSE(r1 == r3);
	}

	TEST(AddressRange, StringRepresentation)
	{
		address_range32 r1 = address_range32::start_length(0x1000, 0x1000);
		std::string str = r1.str();

		// The exact format may vary, but it should contain the start and end addresses
		EXPECT_NE(str.find("1000"), std::string::npos);
		EXPECT_NE(str.find("1fff"), std::string::npos);
	}

	// Tests for address_range_vector32
	TEST(AddressRangeVector, BasicOperations)
	{
		address_range_vector32 vec;
		EXPECT_TRUE(vec.empty());
		EXPECT_EQ(vec.size(), 0);

		// Add a range
		vec.merge(address_range32::start_length(0x1000, 0x1000));
		EXPECT_FALSE(vec.empty());
		EXPECT_EQ(vec.size(), 1);

		// Clear
		vec.clear();
		EXPECT_TRUE(vec.empty());
		EXPECT_EQ(vec.size(), 0);
	}

	TEST(AddressRangeVector, MergeOperations)
	{
		address_range_vector32 vec;

		// Add non-touching ranges
		vec.merge(address_range32::start_length(0x1000, 0x1000)); // 0x1000-0x1FFF
		vec.merge(address_range32::start_length(0x3000, 0x1000)); // 0x3000-0x3FFF
		EXPECT_EQ(vec.valid_count(), 2);

		// Add a range that touches the first range
		vec.merge(address_range32::start_length(0x2000, 0x1000)); // 0x2000-0x2FFF
		// Should merge all three ranges
		EXPECT_EQ(vec.valid_count(), 1);
		EXPECT_TRUE(vec.contains(address_range32::start_end(0x1000, 0x3FFF)));

		// Add a non-touching range
		vec.merge(address_range32::start_length(0x5000, 0x1000)); // 0x5000-0x5FFF
		EXPECT_EQ(vec.valid_count(), 2);

		// Add an overlapping range
		vec.merge(address_range32::start_length(0x4000, 0x2000)); // 0x4000-0x5FFF
		EXPECT_EQ(vec.valid_count(), 1);
		EXPECT_TRUE(vec.contains(address_range32::start_end(0x1000, 0x5FFF)));
	}

	TEST(AddressRangeVector, ExcludeOperations)
	{
		address_range_vector32 vec;
		vec.merge(address_range32::start_length(0x1000, 0x4000)); // 0x1000-0x4FFF

		// Exclude from the middle
		vec.exclude(address_range32::start_length(0x2000, 0x1000)); // 0x2000-0x2FFF
		EXPECT_EQ(vec.valid_count(), 2);

		auto it = vec.begin();
		EXPECT_EQ(it->start, 0x1000);
		EXPECT_EQ(it->end, 0x1FFF);
		++it;
		EXPECT_EQ(it->start, 0x3000);
		EXPECT_EQ(it->end, 0x4FFF);

		// Exclude from the start
		vec.exclude(address_range32::start_length(0x1000, 0x1000)); // 0x1000-0x1FFF
		EXPECT_EQ(vec.valid_count(), 1);
		EXPECT_TRUE(vec.contains(address_range32::start_end(0x3000, 0x4FFF)));

		// Exclude from the end
		vec.exclude(address_range32::start_length(0x4000, 0x1000)); // 0x4000-0x4FFF
		EXPECT_EQ(vec.valid_count(), 1);
		EXPECT_TRUE(vec.contains(address_range32::start_end(0x3000, 0x3FFF)));

		// Exclude entire range
		vec.exclude(address_range32::start_length(0x3000, 0x1000)); // 0x3000-0x3FFF
		EXPECT_EQ(vec.valid_count(), 0);

		// Test excluding with another vector
		vec.merge(address_range32::start_length(0x1000, 0x4000)); // 0x1000-0x4FFF

		address_range_vector32 vec2;
		vec2.merge(address_range32::start_length(0x2000, 0x1000)); // 0x2000-0x2FFF
		vec2.merge(address_range32::start_length(0x4000, 0x1000)); // 0x4000-0x4FFF

		vec.exclude(vec2);
		EXPECT_EQ(vec.valid_count(), 2);

		EXPECT_TRUE(vec.contains(address_range32::start_end(0x1000, 0x1FFF)));
		EXPECT_TRUE(vec.contains(address_range32::start_end(0x3000, 0x3FFF)));
	}

	TEST(AddressRangeVector, ConsistencyCheck)
	{
		address_range_vector32 vec;
		vec.merge(address_range32::start_length(0x1000, 0x1000)); // 0x1000-0x1FFF
		vec.merge(address_range32::start_length(0x3000, 0x1000)); // 0x3000-0x3FFF

		EXPECT_TRUE(vec.check_consistency());

		// This would cause inconsistency, but merge should handle it
		vec.merge(address_range32::start_length(0x2000, 0x1000)); // 0x2000-0x2FFF
		EXPECT_TRUE(vec.check_consistency());
		EXPECT_EQ(vec.valid_count(), 1);
	}

	TEST(AddressRangeVector, OverlapsAndContains)
	{
		address_range_vector32 vec;
		vec.merge(address_range32::start_length(0x1000, 0x1000)); // 0x1000-0x1FFF
		vec.merge(address_range32::start_length(0x3000, 0x1000)); // 0x3000-0x3FFF

		// Test overlaps with range
		EXPECT_TRUE(vec.overlaps(address_range32::start_length(0x1500, 0x1000))); // 0x1500-0x24FF
		EXPECT_TRUE(vec.overlaps(address_range32::start_length(0x3500, 0x1000))); // 0x3500-0x44FF
		EXPECT_FALSE(vec.overlaps(address_range32::start_length(0x2000, 0x1000))); // 0x2000-0x2FFF

		// Test contains
		EXPECT_TRUE(vec.contains(address_range32::start_length(0x1000, 0x1000))); // 0x1000-0x1FFF
		EXPECT_TRUE(vec.contains(address_range32::start_length(0x3000, 0x1000))); // 0x3000-0x3FFF
		EXPECT_FALSE(vec.contains(address_range32::start_length(0x1500, 0x1000))); // 0x1500-0x24FF

		// Test overlaps with another vector
		address_range_vector32 vec2;
		vec2.merge(address_range32::start_length(0x1500, 0x1000)); // 0x1500-0x24FF
		EXPECT_TRUE(vec.overlaps(vec2));

		address_range_vector32 vec3;
		vec3.merge(address_range32::start_length(0x2000, 0x1000)); // 0x2000-0x2FFF
		EXPECT_FALSE(vec.overlaps(vec3));

		// Test inside
		address_range32 big_range = address_range32::start_length(0x0, 0x5000); // 0x0-0x4FFF
		EXPECT_TRUE(vec.inside(big_range));

		address_range32 small_range = address_range32::start_length(0x1000, 0x1000); // 0x1000-0x1FFF
		EXPECT_FALSE(vec.inside(small_range));
	}

	// Test the std::hash implementation for address_range32
	TEST(AddressRange, Hash)
	{
		address_range32 r1 = address_range32::start_length(0x1000, 0x1000);
		address_range32 r2 = address_range32::start_length(0x1000, 0x1000);
		address_range32 r3 = address_range32::start_length(0x2000, 0x1000);

		std::hash<address_range32> hasher;
		EXPECT_EQ(hasher(r1), hasher(r2));
		EXPECT_NE(hasher(r1), hasher(r3));
	}

	// Test invalidation rules around umax
	TEST(AddressRange, Invalidate32)
	{
		address_range32 r1 = address_range32::start_length(0x0, 0x1000);
		r1.invalidate();

		EXPECT_FALSE(r1.valid());
		EXPECT_EQ(r1.start, 0xffff'ffffu);
		EXPECT_EQ(r1.end, 0u);
	}

	TEST(AddressRange, Invalidate64)
	{
		address_range64 r1 = address_range64::start_length(0x0, 0x1000);
		r1.invalidate();

		EXPECT_FALSE(r1.valid());
		EXPECT_EQ(r1.start, 0xffff'ffff'ffff'ffffull);
		EXPECT_EQ(r1.end, 0ull);
	}

	TEST(AddressRange, Invalidate16)
	{
		const u16 start = 0x1000, length = 0x1000;
		address_range16 r1 = address_range16::start_length(start, length);
		r1.invalidate();

		EXPECT_FALSE(r1.valid());
		EXPECT_EQ(r1.start, 0xffff);
		EXPECT_EQ(r1.end, 0);
	}

	TEST(AddressRange, InvalidConstruction)
	{
		address_range32 r1 = address_range32::start_length(umax, u32{umax} / 2);
		EXPECT_FALSE(r1.valid());
	}

	TEST(AddressRange, LargeValues64)
	{
		const u32 start = umax, length = u32{umax} / 2;
		address_range64 r1 = address_range64::start_length(start, length);

		EXPECT_EQ(r1.start, 0xffff'ffffull);
		EXPECT_EQ(r1.end, 0x1'7fff'fffd);
	}
}
