#include <gtest/gtest.h>

#include "Input/blob_detector.h"

#include <cmath>
#include <numbers>
#include <random>

namespace blob
{
	struct test_image
	{
		u32 width = 0;
		u32 height = 0;
		std::vector<u8> pixels;

		test_image(u32 w, u32 h)
			: width(w), height(h), pixels(usz{w} * h, 0)
		{
		}

		void disk(f32 center_x, f32 center_y, f32 radius, u8 value = 255)
		{
			for (u32 y = 0; y < height; y++)
			{
				for (u32 x = 0; x < width; x++)
				{
					const f32 dx = x - center_x;
					const f32 dy = y - center_y;

					if (dx * dx + dy * dy <= radius * radius)
					{
						pixels[y * width + x] = value;
					}
				}
			}
		}

		void rect(u32 x0, u32 y0, u32 x1, u32 y1, u8 value = 255)
		{
			for (u32 y = y0; y <= y1; y++)
			{
				for (u32 x = x0; x <= x1; x++)
				{
					pixels[y * width + x] = value;
				}
			}
		}

		void detect(blob_detector& detector, bool fill_holes, u32 min_circle_area = 0) const
		{
			detector.detect(pixels, width, height, fill_holes, min_circle_area);
		}
	};

	// Returns the blob that contains the given pixel
	const blob_detector::blob* blob_at(const blob_detector& detector, u32 x, u32 y)
	{
		const u32 label = detector.labels().at(y * detector.width() + x);
		return label ? &detector.blobs().at(label - 1) : nullptr;
	}

	TEST(BlobDetector, EmptyImage)
	{
		blob_detector detector;
		test_image image(64, 48);

		image.detect(detector, true);
		EXPECT_TRUE(detector.blobs().empty());
		EXPECT_EQ(detector.labels().size(), 64u * 48u);
	}

	TEST(BlobDetector, InvalidInput)
	{
		blob_detector detector;

		detector.detect({}, 64, 48, true);
		EXPECT_TRUE(detector.blobs().empty());

		const std::vector<u8> pixel { 255 };
		detector.detect(pixel, 0, 0, true);
		EXPECT_TRUE(detector.blobs().empty());
		EXPECT_TRUE(detector.labels().empty());
	}

	TEST(BlobDetector, SinglePixel)
	{
		blob_detector detector;
		test_image image(10, 10);
		image.pixels[4 * 10 + 5] = 1;

		image.detect(detector, true);
		ASSERT_EQ(detector.blobs().size(), 1u);

		const blob_detector::blob& b = detector.blobs()[0];
		EXPECT_EQ(b.area, 1u);
		EXPECT_EQ(b.perimeter, 1u);
		EXPECT_EQ(b.min_x, 5u);
		EXPECT_EQ(b.max_x, 5u);
		EXPECT_EQ(b.min_y, 4u);
		EXPECT_EQ(b.max_y, 4u);
		EXPECT_FLOAT_EQ(b.center_x, 5.0f);
		EXPECT_FLOAT_EQ(b.center_y, 4.0f);
		EXPECT_FLOAT_EQ(b.radius, 0.0f);
	}

	TEST(BlobDetector, Disk)
	{
		blob_detector detector;
		test_image image(640, 480);
		image.disk(300.0f, 200.0f, 20.0f);

		image.detect(detector, true);
		ASSERT_EQ(detector.blobs().size(), 1u);

		const blob_detector::blob& b = detector.blobs()[0];
		EXPECT_NEAR(b.radius, 20.0f, 0.6f);
		EXPECT_NEAR(b.center_x, 300.0f, 0.6f);
		EXPECT_NEAR(b.center_y, 200.0f, 0.6f);
		EXPECT_NEAR(b.centroid_x, 300.0f, 0.1f);
		EXPECT_NEAR(b.centroid_y, 200.0f, 0.1f);
		EXPECT_GT(b.fill_ratio(), 0.9f);
		EXPECT_LT(b.fill_ratio(), 1.1f);
		EXPECT_EQ(b.min_x, 280u);
		EXPECT_EQ(b.max_x, 320u);
		EXPECT_EQ(b.min_y, 180u);
		EXPECT_EQ(b.max_y, 220u);
	}

	TEST(BlobDetector, RingWithAndWithoutHoleFilling)
	{
		blob_detector detector;
		test_image image(200, 200);
		image.disk(100.0f, 100.0f, 30.0f);
		image.disk(100.0f, 100.0f, 15.0f, 0);

		image.detect(detector, false);
		ASSERT_EQ(detector.blobs().size(), 1u);
		const u32 ring_area = detector.blobs()[0].area;

		image.detect(detector, true);
		ASSERT_EQ(detector.blobs().size(), 1u);
		const blob_detector::blob& b = detector.blobs()[0];

		// The hole is part of the blob now
		EXPECT_GT(b.area, ring_area);
		EXPECT_GT(b.fill_ratio(), 0.9f);
		EXPECT_NEAR(b.radius, 30.0f, 0.6f);

		// Only the outer boundary is left
		EXPECT_LT(b.perimeter, 2 * 3.1416f * 30.0f * 1.5f);
	}

	TEST(BlobDetector, NestedBlobStaysSeparateWhenFillingHoles)
	{
		blob_detector detector;
		test_image image(100, 100);
		image.disk(50.0f, 50.0f, 30.0f);
		image.disk(50.0f, 50.0f, 20.0f, 0);
		image.disk(50.0f, 50.0f, 5.0f);

		image.detect(detector, false);
		ASSERT_EQ(detector.blobs().size(), 2u);
		const u32 ring_area = blob_at(detector, 50, 20)->area;

		// The gap touches the ring and the inner disk, so it must not be filled
		image.detect(detector, true);
		ASSERT_EQ(detector.blobs().size(), 2u);
		EXPECT_EQ(detector.labels()[50 * 100 + 35], 0u);

		const blob_detector::blob* ring = blob_at(detector, 50, 20);
		const blob_detector::blob* inner = blob_at(detector, 50, 50);
		ASSERT_NE(ring, nullptr);
		ASSERT_NE(inner, nullptr);
		EXPECT_NE(ring, inner);
		EXPECT_EQ(ring->area, ring_area);
		EXPECT_NEAR(inner->radius, 5.0f, 0.6f);
	}

	TEST(BlobDetector, OnlyHolesOfSingleBlobsAreFilled)
	{
		blob_detector detector;
		test_image image(100, 100);

		// Outer ring
		image.disk(50.0f, 50.0f, 40.0f);
		image.disk(50.0f, 50.0f, 35.0f, 0);

		// Inner ring with a hole that should be filled
		image.disk(50.0f, 50.0f, 20.0f);
		image.disk(50.0f, 50.0f, 10.0f, 0);

		image.detect(detector, true);
		ASSERT_EQ(detector.blobs().size(), 2u);

		// The inner hole was filled, the gap between the rings was not
		const blob_detector::blob* inner = blob_at(detector, 50, 50);
		ASSERT_NE(inner, nullptr);
		EXPECT_EQ(detector.labels()[50 * 100 + 75], 0u);
		EXPECT_GT(inner->fill_ratio(), 0.9f);
		EXPECT_NEAR(inner->radius, 20.0f, 0.6f);
	}

	TEST(BlobDetector, LargeHoleIsNotFilled)
	{
		blob_detector detector;
		test_image image(100, 100);

		// A thin outline around a large area
		image.rect(10, 10, 89, 89);
		image.rect(13, 13, 86, 86, 0);

		image.detect(detector, true);
		ASSERT_EQ(detector.blobs().size(), 1u);
		EXPECT_EQ(detector.blobs()[0].area, 80u * 80u - 74u * 74u);
		EXPECT_EQ(detector.labels()[50 * 100 + 50], 0u);

		// A hole that is smaller than its blob is still filled
		image.rect(10, 10, 89, 89);
		image.rect(40, 40, 59, 59, 0);

		image.detect(detector, true);
		ASSERT_EQ(detector.blobs().size(), 1u);
		EXPECT_EQ(detector.blobs()[0].area, 80u * 80u);
	}

	TEST(BlobDetector, DilateAndFindEnclosed)
	{
		blob_detector detector;
		test_image image(100, 100);

		// Ring with a gap of one pixel
		image.disk(50.0f, 50.0f, 30.0f);
		image.disk(50.0f, 50.0f, 25.0f, 0);
		image.rect(50, 70, 50, 82, 0);

		// The center is connected to the outside through the gap
		std::vector<u8> enclosed = image.pixels;
		detector.find_enclosed(enclosed, image.width, image.height);
		EXPECT_EQ(enclosed[50 * 100 + 50], 0);

		// The dilation closes the gap
		enclosed = image.pixels;
		detector.dilate(enclosed, image.width, image.height);
		EXPECT_EQ(enclosed[75 * 100 + 50], 255);
		EXPECT_EQ(enclosed[50 * 100 + 50], 0);

		detector.find_enclosed(enclosed, image.width, image.height);
		EXPECT_EQ(enclosed[50 * 100 + 50], 255); // Center
		EXPECT_EQ(enclosed[50 * 100 + 78], 0);   // Barrier
		EXPECT_EQ(enclosed[5 * 100 + 5], 0);     // Outside

		// Invalid input is ignored
		std::vector<u8> invalid(10, 255);
		detector.dilate(invalid, 100, 100);
		detector.find_enclosed(invalid, 100, 100);
		EXPECT_EQ(invalid, std::vector<u8>(10, 255));
	}

	TEST(BlobDetector, MomentEllipse)
	{
		blob_detector detector;
		test_image image(300, 200);

		// Disk
		image.disk(50.0f, 50.0f, 20.0f);

		// Horizontal capsule with radius 20 and a straight part of 40 pixels
		image.disk(120.0f, 150.0f, 20.0f);
		image.disk(160.0f, 150.0f, 20.0f);
		image.rect(120, 130, 160, 170);

		// Vertical 2:1 rectangle
		image.rect(240, 20, 259, 59);

		image.detect(detector, true, 30);
		ASSERT_EQ(detector.blobs().size(), 3u);

		const blob_detector::blob* disk = blob_at(detector, 50, 50);
		const blob_detector::blob* capsule = blob_at(detector, 140, 150);
		const blob_detector::blob* rect = blob_at(detector, 250, 40);
		ASSERT_NE(disk, nullptr);
		ASSERT_NE(capsule, nullptr);
		ASSERT_NE(rect, nullptr);

		EXPECT_NEAR(disk->major_axis, 20.0f, 0.5f);
		EXPECT_NEAR(disk->minor_axis, 20.0f, 0.5f);
		EXPECT_NEAR(disk->ellipse_fill_ratio(), 1.0f, 0.02f);

		EXPECT_NEAR(capsule->major_axis / capsule->minor_axis, 1.94f, 0.05f);
		EXPECT_NEAR(capsule->minor_axis, 20.0f * 1.09f, 0.5f);
		EXPECT_NEAR(capsule->angle, 0.0f, 0.01f);
		EXPECT_GT(capsule->ellipse_fill_ratio(), 0.97f);
		EXPECT_NEAR(capsule->centroid_x, 140.0f, 0.1f);
		EXPECT_NEAR(capsule->centroid_y, 150.0f, 0.1f);

		// The capsule is as wide as the sphere
		EXPECT_NEAR(detector.find_inscribed_circle(*capsule).radius, 20.0f, 1.0f);

		EXPECT_NEAR(rect->major_axis / rect->minor_axis, 2.0f, 0.05f);
		EXPECT_NEAR(std::abs(rect->angle), std::numbers::pi_v<f32> / 2.0f, 0.01f);
		EXPECT_NEAR(rect->ellipse_fill_ratio(), 0.95f, 0.02f);

		// A single pixel has no extent
		image.pixels.assign(image.pixels.size(), 0);
		image.pixels[10 * 300 + 10] = 255;
		image.detect(detector, true);
		ASSERT_EQ(detector.blobs().size(), 1u);
		EXPECT_FLOAT_EQ(detector.blobs()[0].major_axis, 0.0f);
		EXPECT_FLOAT_EQ(detector.blobs()[0].ellipse_fill_ratio(), 0.0f);
	}

	TEST(BlobDetector, RoundBulgeAndCapsule)
	{
		blob_detector detector;
		test_image image(400, 300);

		// Disk
		image.disk(50.0f, 50.0f, 20.0f);

		// Disk that touches a thinner bar
		image.disk(200.0f, 50.0f, 25.0f);
		image.rect(220, 40, 399, 60);

		// Two disks of the same size that touch each other
		image.disk(50.0f, 150.0f, 20.0f);
		image.disk(85.0f, 150.0f, 20.0f);

		// Diagonal capsule with radius 15
		for (int i = 0; i <= 40; i++)
		{
			image.disk(200.0f + i, 150.0f + i, 15.0f);
		}

		// L shape
		image.rect(300, 200, 389, 219);
		image.rect(300, 200, 319, 289);

		image.detect(detector, true, 30);
		ASSERT_EQ(detector.blobs().size(), 5u);

		const blob_detector::blob* disk = blob_at(detector, 50, 50);
		const blob_detector::blob* bulge = blob_at(detector, 200, 50);
		const blob_detector::blob* pair = blob_at(detector, 50, 150);
		const blob_detector::blob* capsule = blob_at(detector, 220, 170);
		const blob_detector::blob* corner = blob_at(detector, 305, 205);
		ASSERT_NE(disk, nullptr);
		ASSERT_NE(bulge, nullptr);
		ASSERT_NE(pair, nullptr);
		ASSERT_NE(capsule, nullptr);
		ASSERT_NE(corner, nullptr);

		// Round bulge
		EXPECT_TRUE(detector.is_round_bulge(*disk, detector.find_inscribed_circle(*disk)));
		EXPECT_TRUE(detector.is_round_bulge(*bulge, detector.find_inscribed_circle(*bulge)));
		EXPECT_FALSE(detector.is_round_bulge(*pair, detector.find_inscribed_circle(*pair)));
		EXPECT_FALSE(detector.is_round_bulge(*disk, blob_detector::inscribed_circle{}));

		// The outline around a lone disk is free, the bar covers a small part of the bulge outline
		const blob_detector::inscribed_circle bulge_circle = detector.find_inscribed_circle(*bulge);
		EXPECT_FLOAT_EQ(detector.free_outline_ratio(*disk, 50.0f, 50.0f, 20.0f), 1.0f);
		EXPECT_GT(detector.free_outline_ratio(*bulge, bulge_circle.center_x, bulge_circle.center_y, bulge_circle.radius), 0.8f);
		EXPECT_LT(detector.free_outline_ratio(*bulge, bulge_circle.center_x, bulge_circle.center_y, bulge_circle.radius), 1.0f);

		// Capsule
		blob_detector::capsule result {};
		ASSERT_TRUE(blob_detector::fit_capsule(*capsule, detector.find_inscribed_circle(*capsule), result));
		EXPECT_NEAR(result.center_x, 220.0f, 0.5f);
		EXPECT_NEAR(result.center_y, 170.0f, 0.5f);
		EXPECT_NEAR(result.radius, 15.0f, 1.0f);
		EXPECT_NEAR(result.half_length, 20.0f * std::numbers::sqrt2_v<f32>, 1.5f);
		EXPECT_NEAR(std::abs(result.dir_x), std::numbers::sqrt2_v<f32> / 2.0f, 0.01f);
		EXPECT_NEAR(result.dir_x, result.dir_y, 0.01f);

		// Disks are not elongated and L shapes are not smooth
		EXPECT_FALSE(blob_detector::fit_capsule(*disk, detector.find_inscribed_circle(*disk), result));
		EXPECT_FALSE(blob_detector::fit_capsule(*corner, detector.find_inscribed_circle(*corner), result));
	}

	TEST(BlobDetector, InscribedCircle)
	{
		blob_detector detector;
		test_image image(200, 200);
		image.disk(100.0f, 100.0f, 20.0f);

		image.detect(detector, true);
		ASSERT_EQ(detector.blobs().size(), 1u);

		const blob_detector::blob& b = detector.blobs()[0];
		const blob_detector::inscribed_circle circle = detector.find_inscribed_circle(b);
		EXPECT_NEAR(circle.center_x, 100.0f, 1.0f);
		EXPECT_NEAR(circle.center_y, 100.0f, 1.0f);
		EXPECT_NEAR(circle.radius, b.radius, 1.0f);

		// There is nothing left outside of the inscribed circle
		EXPECT_LT(circle.remainder_radius, 2.0f);

		// Invalid blobs are ignored
		EXPECT_FLOAT_EQ(detector.find_inscribed_circle(blob_detector::blob{}).radius, 0.0f);
	}

	TEST(BlobDetector, InscribedCircleOfMergedBlob)
	{
		blob_detector detector;
		test_image image(300, 200);

		// A disk that touches a bar with the same color
		image.disk(100.0f, 100.0f, 25.0f);
		image.rect(120, 85, 299, 115);

		image.detect(detector, true, 30);
		ASSERT_EQ(detector.blobs().size(), 1u);

		// The merged blob is not round, but the inscribed circle still finds the disk
		const blob_detector::blob& b = detector.blobs()[0];
		EXPECT_LT(b.fill_ratio(), 0.5f);
		const blob_detector::inscribed_circle circle = detector.find_inscribed_circle(b);
		EXPECT_NEAR(circle.center_x, 100.0f, 1.5f);
		EXPECT_NEAR(circle.center_y, 100.0f, 1.5f);
		EXPECT_NEAR(circle.radius, 25.0f, 1.0f);

		// The bar is 31 pixels wide
		EXPECT_NEAR(circle.remainder_radius, 15.0f, 1.0f);
	}

	TEST(BlobDetector, InscribedCircleIgnoresOtherBlobs)
	{
		blob_detector detector;
		test_image image(200, 200);

		// A frame around a small disk. The disk is inside the bounding box of the frame, but it doesn't belong to it.
		image.rect(10, 10, 189, 189);
		image.rect(20, 20, 179, 179, 0);
		image.disk(100.0f, 100.0f, 10.0f);

		image.detect(detector, true, 30);
		ASSERT_EQ(detector.blobs().size(), 2u);

		const blob_detector::blob* frame = blob_at(detector, 10, 10);
		const blob_detector::blob* disk = blob_at(detector, 100, 100);
		ASSERT_NE(frame, nullptr);
		ASSERT_NE(disk, nullptr);

		// The frame is 10 pixels wide
		EXPECT_NEAR(detector.find_inscribed_circle(*frame).radius, 5.0f, 1.0f);
		EXPECT_NEAR(detector.find_inscribed_circle(*disk).radius, 10.0f, 1.0f);
	}

	TEST(BlobDetector, OpeningRemovesNoiseAndBridges)
	{
		blob_detector detector;
		test_image image(200, 100);

		// Two disks that are connected by a thin line
		image.disk(50.0f, 50.0f, 20.0f);
		image.disk(150.0f, 50.0f, 20.0f);
		image.rect(50, 50, 150, 51);

		// Noise
		image.pixels[10 * 200 + 10] = 255;
		image.pixels[90 * 200 + 100] = 255;

		// A blob at the image border
		image.rect(0, 80, 9, 99);

		image.detect(detector, true);
		EXPECT_EQ(detector.blobs().size(), 4u);

		std::vector<u8> pixels = image.pixels;
		detector.open(pixels, image.width, image.height);
		detector.detect(pixels, image.width, image.height, true);
		ASSERT_EQ(detector.blobs().size(), 3u);

		const blob_detector::blob* left = blob_at(detector, 50, 50);
		const blob_detector::blob* right = blob_at(detector, 150, 50);
		const blob_detector::blob* border = blob_at(detector, 0, 99);
		ASSERT_NE(left, nullptr);
		ASSERT_NE(right, nullptr);
		ASSERT_NE(border, nullptr);
		EXPECT_NE(left, right);

		// The disks keep their shape and the border blob doesn't shrink
		EXPECT_NEAR(left->radius, 20.0f, 0.6f);
		EXPECT_NEAR(right->radius, 20.0f, 0.6f);
		EXPECT_EQ(border->area, 10u * 20u);

		// Invalid input is ignored
		std::vector<u8> invalid(10, 255);
		detector.open(invalid, 100, 100);
		EXPECT_EQ(invalid, std::vector<u8>(10, 255));
	}

	TEST(BlobDetector, FillRatioRejectsNonCircularShapes)
	{
		blob_detector detector;
		test_image image(200, 200);
		image.rect(20, 20, 59, 59);   // Square
		image.rect(100, 20, 179, 59); // 2:1 rectangle

		image.detect(detector, true);
		ASSERT_EQ(detector.blobs().size(), 2u);

		EXPECT_NEAR(detector.blobs()[0].fill_ratio(), 0.64f, 0.05f);
		EXPECT_NEAR(detector.blobs()[1].fill_ratio(), 0.51f, 0.05f);
	}

	TEST(BlobDetector, DiagonalPixelsAreConnected)
	{
		blob_detector detector;
		test_image image(10, 10);
		image.pixels[1 * 10 + 1] = 255;
		image.pixels[2 * 10 + 2] = 255;
		image.pixels[3 * 10 + 3] = 255;

		image.detect(detector, true);
		ASSERT_EQ(detector.blobs().size(), 1u);
		EXPECT_EQ(detector.blobs()[0].area, 3u);
	}

	TEST(BlobDetector, SeparateBlobs)
	{
		blob_detector detector;
		test_image image(100, 50);
		image.disk(20.0f, 25.0f, 10.0f);
		image.disk(70.0f, 25.0f, 15.0f);

		image.detect(detector, true);
		ASSERT_EQ(detector.blobs().size(), 2u);

		// Labels are 1-based and assigned in raster order (the larger disk starts in an earlier row)
		EXPECT_EQ(detector.blobs()[0].label, 1u);
		EXPECT_EQ(detector.blobs()[1].label, 2u);
		EXPECT_EQ(detector.labels()[25 * 100 + 70], 1u);
		EXPECT_EQ(detector.labels()[25 * 100 + 20], 2u);
		EXPECT_EQ(detector.labels()[0], 0u);

		const blob_detector::blob* small = blob_at(detector, 20, 25);
		const blob_detector::blob* large = blob_at(detector, 70, 25);
		ASSERT_NE(small, nullptr);
		ASSERT_NE(large, nullptr);
		EXPECT_NEAR(small->radius, 10.0f, 0.6f);
		EXPECT_NEAR(large->radius, 15.0f, 0.6f);
	}

	TEST(BlobDetector, LateMergeAndBorderContact)
	{
		blob_detector detector;
		test_image image(20, 20);

		// U shape: the two arms are only connected at the bottom, and the shape touches the image border
		image.rect(0, 0, 2, 15);
		image.rect(10, 0, 12, 15);
		image.rect(0, 13, 12, 15);

		image.detect(detector, true);
		ASSERT_EQ(detector.blobs().size(), 1u);
		EXPECT_EQ(detector.blobs()[0].area, 3u * 16u + 3u * 16u + 7u * 3u);

		// The open U is not a hole, so nothing should have been filled
		EXPECT_EQ(detector.labels()[5 * 20 + 6], 0u);
	}

	TEST(BlobDetector, FillBlob)
	{
		blob_detector detector;
		test_image image(100, 50);
		image.disk(20.0f, 25.0f, 10.0f);
		image.disk(70.0f, 25.0f, 15.0f);

		image.detect(detector, true);
		ASSERT_EQ(detector.blobs().size(), 2u);

		const blob_detector::blob* small = blob_at(detector, 20, 25);
		const blob_detector::blob* large = blob_at(detector, 70, 25);
		ASSERT_NE(small, nullptr);
		ASSERT_NE(large, nullptr);

		std::vector<u8> pixels = image.pixels;
		detector.fill_blob(pixels, *small, 0);

		// The small blob is gone, the large one is untouched
		EXPECT_EQ(pixels[25 * 100 + 20], 0);
		EXPECT_EQ(pixels[25 * 100 + 70], 255);

		std::vector<u8> rgba(pixels.size() * 4, 0);
		detector.fill_blob_rgba(rgba, *large, { 1, 2, 3, 4 });
		EXPECT_EQ(rgba[(25 * 100 + 70) * 4 + 0], 1);
		EXPECT_EQ(rgba[(25 * 100 + 70) * 4 + 3], 4);
		EXPECT_EQ(rgba[(25 * 100 + 20) * 4 + 0], 0);
	}

	TEST(BlobDetector, MinCircleArea)
	{
		blob_detector detector;
		test_image image(100, 50);
		image.disk(20.0f, 25.0f, 2.0f);  // Small
		image.disk(70.0f, 25.0f, 15.0f); // Large

		image.detect(detector, true, 30);
		ASSERT_EQ(detector.blobs().size(), 2u);

		const blob_detector::blob* small = blob_at(detector, 20, 25);
		const blob_detector::blob* large = blob_at(detector, 70, 25);
		ASSERT_NE(small, nullptr);
		ASSERT_NE(large, nullptr);

		// The small blob is still reported, but without an enclosing circle
		EXPECT_LT(small->area, 30u);
		EXPECT_FLOAT_EQ(small->radius, 0.0f);
		EXPECT_FLOAT_EQ(small->center_x, small->centroid_x);
		EXPECT_NEAR(large->radius, 15.0f, 0.6f);
	}

	TEST(BlobDetector, MinEnclosingCircle)
	{
		std::mt19937 rng(42);

		for (int iteration = 0; iteration < 200; iteration++)
		{
			std::vector<blob_detector::point> points;
			const u32 count = 1 + rng() % 40;

			for (u32 i = 0; i < count; i++)
			{
				points.push_back({ static_cast<s32>(rng() % 100), static_cast<s32>(rng() % 100) });
			}

			std::vector<blob_detector::point> shuffled = points;
			f32 center_x, center_y, radius;
			blob_detector::min_enclosing_circle(shuffled, center_x, center_y, radius);

			f32 max_distance = 0.0f;
			f32 max_half_distance = 0.0f;

			for (const blob_detector::point& a : points)
			{
				max_distance = std::max(max_distance, std::hypot(a.x - center_x, a.y - center_y));

				for (const blob_detector::point& b : points)
				{
					max_half_distance = std::max(max_half_distance, std::hypot(static_cast<f32>(a.x - b.x), static_cast<f32>(a.y - b.y)) / 2.0f);
				}
			}

			// All points are inside
			EXPECT_LE(max_distance, radius + 1e-3f);

			// The circle is at least as large as half the largest distance, and at most diameter / sqrt(3)
			EXPECT_GE(radius, max_half_distance - 1e-3f);
			EXPECT_LE(radius, max_half_distance * 1.1548f + 1e-3f);
		}
	}

	TEST(BlobDetector, MinEnclosingCircleCollinear)
	{
		std::vector<blob_detector::point> points = { { 0, 0 }, { 5, 0 }, { 10, 0 } };
		f32 center_x, center_y, radius;
		blob_detector::min_enclosing_circle(points, center_x, center_y, radius);

		EXPECT_NEAR(center_x, 5.0f, 1e-4f);
		EXPECT_NEAR(center_y, 0.0f, 1e-4f);
		EXPECT_NEAR(radius, 5.0f, 1e-4f);
	}

	TEST(BlobDetector, DrawingStaysInBounds)
	{
		constexpr u32 width = 32;
		constexpr u32 height = 16;
		std::vector<u8> rgba(width * height * 4, 0);

		// Partially or completely outside of the image
		rgba_draw::circle(rgba, width, height, 30.0f, 2.0f, 10, { 1, 2, 3, 4 }, true);
		rgba_draw::circle(rgba, width, height, -5.0f, 8.0f, 3, { 1, 2, 3, 4 }, false);
		rgba_draw::circle(rgba, width, height, 1000.0f, 1000.0f, 3, { 1, 2, 3, 4 }, true);
		rgba_draw::line(rgba, width, height, -10, -10, 50, 40, { 1, 2, 3, 4 });

		EXPECT_EQ(rgba[(2 * width + 30) * 4], 1);
	}

	TEST(BlobDetector, DrawCircleOutline)
	{
		constexpr u32 width = 21;
		constexpr u32 height = 21;
		std::vector<u8> rgba(width * height * 4, 0);

		rgba_draw::circle(rgba, width, height, 10.0f, 10.0f, 5, { 255, 0, 0, 255 }, false);

		// On the outline
		EXPECT_EQ(rgba[(10 * width + 15) * 4], 255);
		EXPECT_EQ(rgba[(5 * width + 10) * 4], 255);

		// Inside and outside
		EXPECT_EQ(rgba[(10 * width + 10) * 4], 0);
		EXPECT_EQ(rgba[(10 * width + 18) * 4], 0);
	}
}
