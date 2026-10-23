#pragma once

#include "util/types.hpp"

#include <array>
#include <vector>

// Finds connected blobs in binary images.
class blob_detector
{
public:
	struct point
	{
		s32 x = 0;
		s32 y = 0;
	};

	struct blob
	{
		u32 label = 0;      // Label of the blob in labels() (1-based)
		u32 area = 0;       // Number of pixels (including filled holes)
		u32 perimeter = 0;  // Number of boundary pixels
		u32 min_x = 0;      // Bounding box
		u32 min_y = 0;
		u32 max_x = 0;
		u32 max_y = 0;
		f32 centroid_x = 0.0f; // Center of mass
		f32 centroid_y = 0.0f;
		f32 center_x = 0.0f;   // Center of the minimum enclosing circle
		f32 center_y = 0.0f;
		f32 radius = 0.0f;     // Radius of the minimum enclosing circle (through the boundary pixel centers)
		f32 major_axis = 0.0f; // Semi-axes of the ellipse with the same second order moments as the blob
		f32 minor_axis = 0.0f;
		f32 angle = 0.0f;      // Angle of the major axis in radians (-pi/2 to pi/2, y points down)

		// Ratio between the area of the blob and the area of its enclosing circle.
		// About 1.0 for a disk, 0.64 for a square and 0.5 for a half disk or a 2:1 ellipse.
		f32 fill_ratio() const;

		// Ratio between the area of the blob and the area of its moment ellipse.
		// About 1.0 for ellipses and capsules, 0.95 for rectangles and lower for irregular shapes.
		f32 ellipse_fill_ratio() const;
	};

	struct inscribed_circle
	{
		f32 center_x = 0.0f;         // Center of the largest circle that fits inside the blob
		f32 center_y = 0.0f;
		f32 radius = 0.0f;           // Radius of the largest inscribed circle (through the boundary pixel centers)
		f32 remainder_radius = 0.0f; // Radius of the largest inscribed circle whose center lies outside of the largest inscribed circle
	};

	struct capsule
	{
		f32 center_x = 0.0f;    // Center of the capsule (centroid of the blob)
		f32 center_y = 0.0f;
		f32 radius = 0.0f;      // Half the width of the capsule
		f32 half_length = 0.0f; // Half the length of the straight part
		f32 dir_x = 1.0f;       // Direction of the long axis
		f32 dir_y = 0.0f;
	};

	// Finds all 8-connected blobs in a binary image (0 = background, anything else = foreground).
	// If fill_holes is true, background regions that are completely enclosed by a single blob are added to that blob,
	// unless the hole is larger than the blob itself (e.g. a large area that is only enclosed by a thin outline).
	// Holes that touch more than one blob (e.g. the gap between a frame and a blob inside of it) are not filled.
	// The enclosing circle is only calculated for blobs with at least min_circle_area pixels. Smaller blobs get radius 0.
	void detect(const std::vector<u8>& binary, u32 width, u32 height, bool fill_holes, u32 min_circle_area = 0);

	const std::vector<blob>& blobs() const { return m_blobs; }
	const std::vector<u32>& labels() const { return m_labels; } // One label per pixel (0 = background)
	u32 width() const { return m_width; }
	u32 height() const { return m_height; }

	// Sets all pixels of the blob to value. The image must have the same size as the one passed to detect().
	void fill_blob(std::vector<u8>& binary, const blob& b, u8 value) const;

	// Sets all pixels of the blob to color. The image must have the same size as the one passed to detect().
	void fill_blob_rgba(std::vector<u8>& rgba, const blob& b, const std::array<u8, 4>& color) const;

	// Calculates the largest circle that fits inside the blob. The blob has to be one of the blobs found by the last call to detect().
	// This is relatively expensive for large blobs, so it is only done on demand.
	inscribed_circle find_inscribed_circle(const blob& b);

	// Returns the fraction of a circle outline (slightly outside of the given circle) that doesn't belong to the blob.
	f32 free_outline_ratio(const blob& b, f32 center_x, f32 center_y, f32 radius) const;

	// Checks if the inscribed circle is a round bulge of the blob, e.g. a disk that touches a thinner object.
	// The rest of the blob has to be clearly thinner than the circle and most of the circle outline has to be free.
	bool is_round_bulge(const blob& b, const inscribed_circle& circle) const;

	// Checks if the blob looks like a capsule (an elongated, smooth shape), e.g. a disk that is blurred by movement.
	// circle has to be the inscribed circle of the blob. Rectangles can't be distinguished from capsules.
	static bool fit_capsule(const blob& b, const inscribed_circle& circle, capsule& result);

	// Morphological opening with a 3x3 square (erode, then dilate). Removes noise and thin lines and splits blobs that are connected by thin bridges.
	void open(std::vector<u8>& binary, u32 width, u32 height);

	// Morphological dilation with a 3x3 square. Grows the foreground by one pixel and closes small gaps.
	void dilate(std::vector<u8>& binary, u32 width, u32 height);

	// Replaces a mask of barrier pixels (anything but 0) with a mask of all pixels that are enclosed by the barrier (255).
	// A pixel is enclosed if it can't be reached from the image border without crossing a barrier pixel (4-connectivity).
	// The barrier pixels themselves are not part of the result.
	void find_enclosed(std::vector<u8>& mask, u32 width, u32 height);

	// Calculates the smallest circle that contains all points (Welzl's algorithm). The points will be reordered.
	static void min_enclosing_circle(std::vector<point>& points, f32& center_x, f32& center_y, f32& radius);

private:
	void fill_holes();
	u32 find_root(u32 label);
	void unite(u32 a, u32 b);

	u32 m_width = 0;
	u32 m_height = 0;

	std::vector<u8> m_mask;            // 1 = foreground
	std::vector<u32> m_labels;         // Label per pixel
	std::vector<u32> m_parents;        // Union-find parents of the provisional labels
	std::vector<u32> m_compact_labels; // Provisional root label -> final label
	std::vector<u32> m_stack;          // Pixel indices for the flood fill
	std::vector<u32> m_region;         // Pixel indices of the current hole
	std::vector<u32> m_areas;          // Area of each blob before the holes are filled
	std::vector<u8> m_temp;            // Temporary image for the morphology
	std::vector<f32> m_distances;      // Squared distances for the distance transform
	std::vector<f32> m_line;           // One row or column of the distance transform
	std::vector<s32> m_parabolas;      // Parabola positions for the distance transform
	std::vector<f32> m_bounds;         // Parabola boundaries for the distance transform
	std::vector<std::vector<point>> m_boundaries; // Boundary pixels of each blob
	std::vector<blob> m_blobs;
};

// Simple drawing functions for RGBA images (4 bytes per pixel, no row padding)
namespace rgba_draw
{
	void circle(std::vector<u8>& rgba, u32 width, u32 height, f32 center_x, f32 center_y, s32 radius, const std::array<u8, 4>& color, bool filled);
	void line(std::vector<u8>& rgba, u32 width, u32 height, s32 x0, s32 y0, s32 x1, s32 y1, const std::array<u8, 4>& color);
}
