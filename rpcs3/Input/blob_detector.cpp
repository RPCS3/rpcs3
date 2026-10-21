#include "stdafx.h"
#include "blob_detector.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <random>

f32 blob_detector::blob::fill_ratio() const
{
	if (!area)
	{
		return 0.0f;
	}

	// The radius goes through the centers of the boundary pixels, so we add half a pixel to cover the whole pixels.
	const f32 r = radius + 0.5f;
	if (r <= 0.0f)
	{
		return 0.0f;
	}

	return area / (std::numbers::pi_v<f32> * r * r);
}

f32 blob_detector::blob::ellipse_fill_ratio() const
{
	if (!area || major_axis <= 0.0f || minor_axis <= 0.0f)
	{
		return 0.0f;
	}

	return area / (std::numbers::pi_v<f32> * major_axis * minor_axis);
}

u32 blob_detector::find_root(u32 label)
{
	while (m_parents[label] != label)
	{
		// Path halving
		m_parents[label] = m_parents[m_parents[label]];
		label = m_parents[label];
	}

	return label;
}

void blob_detector::unite(u32 a, u32 b)
{
	const u32 root_a = find_root(a);
	const u32 root_b = find_root(b);

	if (root_a == root_b)
	{
		return;
	}

	// Keep the smaller label as root
	if (root_a < root_b)
	{
		m_parents[root_b] = root_a;
	}
	else
	{
		m_parents[root_a] = root_b;
	}
}

void blob_detector::fill_holes()
{
	// Background regions are 4-connected (the complement of the 8-connected foreground).
	constexpr u8 outside = 2; // Background that is connected to the image border
	constexpr u8 visited = 3; // Background that is part of a hole

	const u32 width = m_width;
	const u32 height = m_height;
	const u32 size = width * height;

	// Flood fill the background from the image border
	m_stack.clear();

	const auto push_outside = [this](u32 index)
	{
		if (m_mask[index] == 0)
		{
			m_mask[index] = outside;
			m_stack.push_back(index);
		}
	};

	for (u32 x = 0; x < width; x++)
	{
		push_outside(x);
		push_outside((height - 1) * width + x);
	}

	for (u32 y = 0; y < height; y++)
	{
		push_outside(y * width);
		push_outside(y * width + width - 1);
	}

	while (!m_stack.empty())
	{
		const u32 index = m_stack.back();
		m_stack.pop_back();

		const u32 x = index % width;
		const u32 y = index / width;

		if (x > 0) push_outside(index - 1);
		if (x + 1 < width) push_outside(index + 1);
		if (y > 0) push_outside(index - width);
		if (y + 1 < height) push_outside(index + width);
	}

	// All remaining background pixels are part of a hole.
	// Only fill holes that are enclosed by a single blob. A hole that touches multiple blobs is the gap between an outer blob and the blobs inside of it.
	// Also skip holes that are larger than their blob. Those are usually large areas that are enclosed by a thin outline and not a ring.
	for (u32 start = 0; start < size; start++)
	{
		if (m_mask[start] != 0)
		{
			continue;
		}

		u32 hole_label = 0;
		bool single_blob = true;

		m_region.clear();
		m_stack.clear();
		m_mask[start] = visited;
		m_stack.push_back(start);

		while (!m_stack.empty())
		{
			const u32 index = m_stack.back();
			m_stack.pop_back();
			m_region.push_back(index);

			const auto visit = [&](u32 neighbor)
			{
				const u8 value = m_mask[neighbor];

				if (value == 0)
				{
					m_mask[neighbor] = visited;
					m_stack.push_back(neighbor);
				}
				else if (value == 1)
				{
					const u32 label = m_labels[neighbor];

					if (!hole_label)
					{
						hole_label = label;
					}
					else if (label != hole_label)
					{
						single_blob = false;
					}
				}
			};

			// Holes can't touch the image border, so all neighbors exist
			visit(index - 1);
			visit(index + 1);
			visit(index - width);
			visit(index + width);
		}

		if (single_blob && hole_label && m_region.size() <= m_areas[hole_label - 1])
		{
			for (u32 index : m_region)
			{
				m_labels[index] = hole_label;
			}
		}
	}

	// Filled holes are foreground now
	for (u32 i = 0; i < size; i++)
	{
		m_mask[i] = m_labels[i] ? 1 : 0;
	}
}

// Calculates the squared euclidean distance transform of one row or column in place (Felzenszwalb and Huttenlocher).
// v needs n entries, z and d need n + 1 entries.
static void distance_transform_1d(f32* f, u32 n, s32* v, f32* z, f32* d)
{
	u32 k = 0;
	v[0] = 0;
	z[0] = -std::numeric_limits<f32>::infinity();
	z[1] = std::numeric_limits<f32>::infinity();

	// Intersection of the parabolas at q and p
	const auto intersection = [f](s32 q, s32 p)
	{
		return ((f[q] + static_cast<f32>(q * q)) - (f[p] + static_cast<f32>(p * p))) / static_cast<f32>(2 * (q - p));
	};

	for (s32 q = 1; q < static_cast<s32>(n); q++)
	{
		f32 s = intersection(q, v[k]);

		while (s <= z[k])
		{
			k--;
			s = intersection(q, v[k]);
		}

		k++;
		v[k] = q;
		z[k] = s;
		z[k + 1] = std::numeric_limits<f32>::infinity();
	}

	k = 0;

	for (s32 q = 0; q < static_cast<s32>(n); q++)
	{
		while (z[k + 1] < q)
		{
			k++;
		}

		const f32 dq = static_cast<f32>(q - v[k]);
		d[q] = dq * dq + f[v[k]];
	}

	std::copy(d, d + n, f);
}

blob_detector::inscribed_circle blob_detector::find_inscribed_circle(const blob& b)
{
	inscribed_circle result {};

	if (!b.label || b.label > m_blobs.size() || b.min_x > b.max_x || b.min_y > b.max_y || b.max_x >= m_width || b.max_y >= m_height)
	{
		return result;
	}

	// The center of the largest inscribed circle is the pixel with the largest distance to the background.
	// Calculate the exact distance transform of the bounding box with a border of 1 pixel.
	// Every pixel that doesn't belong to the blob counts as background, so other blobs in the bounding box are ignored.
	const u32 w = b.max_x - b.min_x + 3;
	const u32 h = b.max_y - b.min_y + 3;

	// Larger than any real squared distance, but small enough to keep the calculations precise
	const f32 far = static_cast<f32>(u64{w + h} * (w + h));

	m_distances.resize(usz{w} * h);

	for (u32 y = 0; y < h; y++)
	{
		for (u32 x = 0; x < w; x++)
		{
			const bool inside = x > 0 && y > 0 && x + 1 < w && y + 1 < h &&
				m_labels[(b.min_y + y - 1) * m_width + (b.min_x + x - 1)] == b.label;

			m_distances[y * w + x] = inside ? far : 0.0f;
		}
	}

	const u32 n = std::max(w, h);
	m_line.resize(n * 2 + 1);
	m_parabolas.resize(n);
	m_bounds.resize(n + 1);

	f32* line = m_line.data();
	f32* temp = line + n;

	// Columns
	for (u32 x = 0; x < w; x++)
	{
		for (u32 y = 0; y < h; y++) line[y] = m_distances[y * w + x];
		distance_transform_1d(line, h, m_parabolas.data(), m_bounds.data(), temp);
		for (u32 y = 0; y < h; y++) m_distances[y * w + x] = line[y];
	}

	// Rows
	f32 max_distance = 0.0f;
	u32 max_x = 0;
	u32 max_y = 0;

	for (u32 y = 0; y < h; y++)
	{
		f32* row = &m_distances[y * w];
		distance_transform_1d(row, w, m_parabolas.data(), m_bounds.data(), temp);

		for (u32 x = 0; x < w; x++)
		{
			if (row[x] > max_distance)
			{
				max_distance = row[x];
				max_x = x;
				max_y = y;
			}
		}
	}

	if (max_distance <= 0.0f)
	{
		return result;
	}

	result.center_x = static_cast<f32>(b.min_x + max_x - 1);
	result.center_y = static_cast<f32>(b.min_y + max_y - 1);

	// The distance goes to the center of the closest background pixel. Subtract half a pixel to get a radius through the boundary pixel centers.
	result.radius = std::max(0.0f, std::sqrt(max_distance) - 0.5f);

	// Find the largest distance outside of the inscribed circle. This is small if the rest of the blob is thin compared to the circle.
	const f32 outer_radius = std::sqrt(max_distance) + 1.0f;
	const f32 outer_radius_squared = outer_radius * outer_radius;
	f32 max_remainder_distance = 0.0f;

	for (u32 y = 0; y < h; y++)
	{
		const f32* row = &m_distances[y * w];
		const f32 dy = static_cast<f32>(y) - static_cast<f32>(max_y);

		for (u32 x = 0; x < w; x++)
		{
			const f32 dx = static_cast<f32>(x) - static_cast<f32>(max_x);

			if (row[x] > max_remainder_distance && dx * dx + dy * dy > outer_radius_squared)
			{
				max_remainder_distance = row[x];
			}
		}
	}

	result.remainder_radius = max_remainder_distance > 0.0f ? std::max(0.0f, std::sqrt(max_remainder_distance) - 0.5f) : 0.0f;

	return result;
}

f32 blob_detector::free_outline_ratio(const blob& b, f32 center_x, f32 center_y, f32 radius) const
{
	constexpr u32 sample_count = 64;
	const f32 r = radius + 2.0f;
	const s32 width = static_cast<s32>(m_width);
	const s32 height = static_cast<s32>(m_height);

	u32 free_count = 0;

	for (u32 i = 0; i < sample_count; i++)
	{
		const f32 angle = i * (2.0f * std::numbers::pi_v<f32> / sample_count);
		const s32 x = static_cast<s32>(std::lround(center_x + r * std::cos(angle)));
		const s32 y = static_cast<s32>(std::lround(center_y + r * std::sin(angle)));

		if (x < 0 || y < 0 || x >= width || y >= height || m_labels[y * width + x] != b.label)
		{
			free_count++;
		}
	}

	return free_count / static_cast<f32>(sample_count);
}

bool blob_detector::is_round_bulge(const blob& b, const inscribed_circle& circle) const
{
	if (circle.radius <= 0.0f)
	{
		return false;
	}

	return circle.remainder_radius < circle.radius * 0.6f &&
	       free_outline_ratio(b, circle.center_x, circle.center_y, circle.radius) >= 0.6f;
}

bool blob_detector::fit_capsule(const blob& b, const inscribed_circle& circle, capsule& result)
{
	const f32 radius = circle.radius;

	if (b.minor_axis <= 0.0f || radius <= 0.0f)
	{
		return false;
	}

	// Clearly elongated, but not a line
	const f32 elongation = b.major_axis / b.minor_axis;
	if (elongation < 1.2f || elongation > 5.0f)
	{
		return false;
	}

	// Capsules have about the same area as their moment ellipse (0.97 to 1.0). Irregular shapes have less.
	if (b.ellipse_fill_ratio() < 0.85f)
	{
		return false;
	}

	// The minor axis of a capsule is 1.0 to 1.15 times its half width
	if (b.minor_axis < radius * 0.9f || b.minor_axis > radius * 1.25f)
	{
		return false;
	}

	result.center_x = b.centroid_x;
	result.center_y = b.centroid_y;
	result.radius = radius;
	result.half_length = std::max(0.0f, b.radius - radius); // The enclosing radius of a capsule is half its length plus its radius
	result.dir_x = std::cos(b.angle);
	result.dir_y = std::sin(b.angle);
	return true;
}

// 3x3 erosion (min) or dilation (max) in two separable passes.
// Pixels outside of the image are ignored, so blobs that touch the image border don't shrink.
template <bool Erode>
static void morphology_3x3(std::vector<u8>& binary, std::vector<u8>& temp, u32 width, u32 height)
{
	const usz size = usz{width} * height;

	if (binary.size() != size || binary.size() != temp.size() || !size || size > u32{umax})
	{
		return;
	}

	const auto op = [](u8 a, u8 b) -> u8
	{
		if constexpr (Erode) return a < b ? a : b;
		else return a > b ? a : b;
	};

	// Horizontal pass
	const u8* src = binary.data();
	u8* dst = temp.data();
	if (width == 1)
	{
		std::memcpy(dst, src, size);
	}
	else
	{
		for (u32 y = 0; y < height; y++, src += width, dst += width)
		{
			dst[0] = op(src[0], src[1]);

			for (u32 x = 1; x + 1 < width; x++)
			{
				dst[x] = op(op(src[x - 1], src[x]), src[x + 1]);
			}

			dst[width - 1] = op(src[width - 2], src[width - 1]);
		}
	}

	// Vertical pass. Missing rows at the border are replaced by the current row.
	src = temp.data();
	dst = binary.data();
	for (u32 y = 0; y < height; y++, dst += width)
	{
		const u8* above = src + (y > 0 ? y - 1 : y) * width;
		const u8* center = src + y * width;
		const u8* below = src + (y + 1 < height ? y + 1 : y) * width;

		for (u32 x = 0; x < width; x++)
		{
			dst[x] = op(op(above[x], center[x]), below[x]);
		}
	}
}

void blob_detector::open(std::vector<u8>& binary, u32 width, u32 height)
{
	const usz size = usz{width} * height;

	if (binary.size() != size || !size || size > u32{umax})
	{
		return;
	}

	m_temp.resize(size);

	// Normalize the foreground to 255, so we can use min and max
	for (u8& value : binary)
	{
		value = value ? 255 : 0;
	}

	morphology_3x3<true>(binary, m_temp, width, height);
	morphology_3x3<false>(binary, m_temp, width, height);
}

void blob_detector::dilate(std::vector<u8>& binary, u32 width, u32 height)
{
	const usz size = usz{width} * height;

	if (binary.size() != size || !size || size > u32{umax})
	{
		return;
	}

	m_temp.resize(size);

	// Normalize the foreground to 255, so we can use max
	for (u8& value : binary)
	{
		value = value ? 255 : 0;
	}

	morphology_3x3<false>(binary, m_temp, width, height);
}

void blob_detector::find_enclosed(std::vector<u8>& mask, u32 width, u32 height)
{
	const usz size = usz{width} * height;

	if (mask.size() != size || !size || size > u32{umax})
	{
		return;
	}

	constexpr u8 unreached = 1; // Not reached from the border yet
	constexpr u8 barrier = 2;  
	constexpr u8 outside = 0;    // Reached from the border

	for (u8& value : mask)
	{
		value = value ? barrier : unreached;
	}

	// Flood fill from the image border
	m_stack.clear();

	const auto push = [&](u32 index)
	{
		if (mask[index] == unreached)
		{
			mask[index] = outside;
			m_stack.push_back(index);
		}
	};

	for (u32 x = 0; x < width; x++)
	{
		push(x);
		push((height - 1) * width + x);
	}

	for (u32 y = 0; y < height; y++)
	{
		push(y * width);
		push(y * width + width - 1);
	}

	while (!m_stack.empty())
	{
		const u32 index = m_stack.back();
		m_stack.pop_back();

		const u32 x = index % width;
		const u32 y = index / width;

		if (x > 0) push(index - 1);
		if (x + 1 < width) push(index + 1);
		if (y > 0) push(index - width);
		if (y + 1 < height) push(index + width);
	}

	// Everything that wasn't reached is enclosed
	for (u8& value : mask)
	{
		value = (value == unreached) ? 255 : 0;
	}
}

void blob_detector::detect(const std::vector<u8>& binary, u32 width, u32 height, bool fill_holes, u32 min_circle_area)
{
	m_blobs.clear();
	m_width = 0;
	m_height = 0;

	const usz size = usz{width} * height;

	if (binary.size() != size || !size || size > u32{umax})
	{
		m_mask.clear();
		m_labels.clear();
		return;
	}

	m_width = width;
	m_height = height;

	m_mask.resize(size);
	m_labels.assign(size, 0);

	for (usz i = 0; i < size; i++)
	{
		m_mask[i] = binary[i] ? 1 : 0;
	}

	// First pass: assign provisional labels and record which ones are connected (8-connectivity)
	m_parents.clear();
	m_parents.push_back(0); // Label 0 is the background

	for (u32 y = 0; y < height; y++)
	{
		for (u32 x = 0; x < width; x++)
		{
			const u32 index = y * width + x;

			if (!m_mask[index])
			{
				continue;
			}

			u32 label = 0;

			const auto check_neighbor = [&](u32 neighbor)
			{
				const u32 neighbor_label = m_labels[neighbor];

				if (!neighbor_label)
				{
					return;
				}

				if (!label)
				{
					label = neighbor_label;
				}
				else if (neighbor_label != label)
				{
					unite(label, neighbor_label);
				}
			};

			// Check the neighbors that were already visited
			if (x > 0)
			{
				check_neighbor(index - 1);
			}

			if (y > 0)
			{
				const u32 above = index - width;

				if (x > 0) check_neighbor(above - 1);
				check_neighbor(above);
				if (x + 1 < width) check_neighbor(above + 1);
			}

			if (!label)
			{
				label = ::size32(m_parents);
				m_parents.push_back(label);
			}

			m_labels[index] = label;
		}
	}

	// Second pass: resolve the final labels in raster order
	m_compact_labels.assign(m_parents.size(), 0);
	u32 blob_count = 0;
	m_areas.clear();

	for (u32& label : m_labels)
	{
		if (!label)
		{
			continue;
		}

		u32& final_label = m_compact_labels[find_root(label)];

		if (!final_label)
		{
			final_label = ++blob_count;
			m_areas.push_back(0);
		}

		label = final_label;
		m_areas[final_label - 1]++;
	}

	if (fill_holes)
	{
		this->fill_holes();
	}

	// Third pass: collect the blob data
	struct sums
	{
		u64 x = 0;
		u64 y = 0;
		u64 xx = 0;
		u64 yy = 0;
		u64 xy = 0;
	};
	std::vector<sums> blob_sums(blob_count);

	m_blobs.resize(blob_count);

	if (m_boundaries.size() < blob_count)
	{
		m_boundaries.resize(blob_count);
	}

	for (u32 i = 0; i < blob_count; i++)
	{
		m_blobs[i].label = i + 1;
		m_boundaries[i].clear();
	}

	for (u32 y = 0; y < height; y++)
	{
		for (u32 x = 0; x < width; x++)
		{
			const u32 index = y * width + x;
			const u32 label = m_labels[index];

			if (!label)
			{
				continue;
			}

			blob& b = m_blobs[label - 1];

			if (!b.area)
			{
				b.min_x = x;
				b.min_y = y;
				b.max_x = x;
			}

			b.area++;
			b.min_x = std::min(b.min_x, x);
			b.max_x = std::max(b.max_x, x);
			b.max_y = y;

			sums& s = blob_sums[label - 1];
			s.x += x;
			s.y += y;
			s.xx += u64{x} * x;
			s.yy += u64{y} * y;
			s.xy += u64{x} * y;

			// A pixel is part of the boundary if it touches the image border or the background (4-connectivity)
			const bool is_boundary =
				x == 0 || y == 0 || x + 1 == width || y + 1 == height ||
				!m_mask[index - 1] || !m_mask[index + 1] ||
				!m_mask[index - width] || !m_mask[index + width];

			if (is_boundary)
			{
				m_boundaries[label - 1].push_back(point{ static_cast<s32>(x), static_cast<s32>(y) });
			}
		}
	}

	for (usz i = 0; i < m_blobs.size(); i++)
	{
		blob& b = m_blobs[i];
		const sums& s = blob_sums[i];
		std::vector<point>& boundary = m_boundaries[i];

		const f64 area = b.area;
		const f64 centroid_x = s.x / area;
		const f64 centroid_y = s.y / area;

		b.centroid_x = static_cast<f32>(centroid_x);
		b.centroid_y = static_cast<f32>(centroid_y);
		b.perimeter = ::size32(boundary);

		// Fit an ellipse with the same second order central moments. The variance along an axis of a filled ellipse is (semi-axis)^2 / 4.
		const f64 mu20 = s.xx / area - centroid_x * centroid_x;
		const f64 mu02 = s.yy / area - centroid_y * centroid_y;
		const f64 mu11 = s.xy / area - centroid_x * centroid_y;
		const f64 mean = (mu20 + mu02) / 2.0;
		const f64 half_diff = (mu20 - mu02) / 2.0;
		const f64 delta = std::sqrt(half_diff * half_diff + mu11 * mu11);

		b.major_axis = static_cast<f32>(2.0 * std::sqrt(std::max(0.0, mean + delta)));
		b.minor_axis = static_cast<f32>(2.0 * std::sqrt(std::max(0.0, mean - delta)));
		b.angle = static_cast<f32>(0.5 * std::atan2(2.0 * mu11, mu20 - mu02));

		if (b.area < min_circle_area)
		{
			// Skip the expensive part for small blobs that will be ignored anyway
			b.center_x = b.centroid_x;
			b.center_y = b.centroid_y;
			b.radius = 0.0f;
			continue;
		}

		min_enclosing_circle(boundary, b.center_x, b.center_y, b.radius);
	}
}

void blob_detector::fill_blob(std::vector<u8>& binary, const blob& b, u8 value) const
{
	const usz size = usz{m_width} * m_height;

	if (binary.size() != size || !b.label || b.max_x >= m_width || b.max_y >= m_height)
	{
		return;
	}

	for (u32 y = b.min_y; y <= b.max_y; y++)
	{
		for (u32 x = b.min_x; x <= b.max_x; x++)
		{
			const u32 index = y * m_width + x;

			if (m_labels[index] == b.label)
			{
				binary[index] = value;
			}
		}
	}
}

void blob_detector::fill_blob_rgba(std::vector<u8>& rgba, const blob& b, const std::array<u8, 4>& color) const
{
	const usz size = usz{m_width} * m_height * 4u;

	if (rgba.size() != size || !b.label || b.max_x >= m_width || b.max_y >= m_height)
	{
		return;
	}

	for (u32 y = b.min_y; y <= b.max_y; y++)
	{
		for (u32 x = b.min_x; x <= b.max_x; x++)
		{
			const u32 index = y * m_width + x;

			if (m_labels[index] == b.label)
			{
				std::copy(color.begin(), color.end(), &rgba[index * 4]);
			}
		}
	}
}

void blob_detector::min_enclosing_circle(std::vector<point>& points, f32& center_x, f32& center_y, f32& radius)
{
	center_x = 0.0f;
	center_y = 0.0f;
	radius = 0.0f;

	if (points.empty())
	{
		return;
	}

	struct circle
	{
		f64 x = 0.0;
		f64 y = 0.0;
		f64 r2 = 0.0; // Squared radius
	};

	const auto contains = [](const circle& c, const point& p)
	{
		const f64 dx = p.x - c.x;
		const f64 dy = p.y - c.y;
		return dx * dx + dy * dy <= c.r2 * (1.0 + 1e-10) + 1e-7;
	};

	const auto circle_from_2 = [](const point& a, const point& b)
	{
		const f64 x = (a.x + b.x) / 2.0;
		const f64 y = (a.y + b.y) / 2.0;
		const f64 dx = a.x - x;
		const f64 dy = a.y - y;
		return circle{ x, y, dx * dx + dy * dy };
	};

	const auto circle_from_3 = [&](const point& a, const point& b, const point& c)
	{
		const f64 ax = a.x, ay = a.y;
		const f64 bx = b.x, by = b.y;
		const f64 cx = c.x, cy = c.y;
		const f64 d = 2.0 * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by));

		if (std::abs(d) < 1e-12)
		{
			// Collinear points: use the two points that are furthest apart
			const circle ab = circle_from_2(a, b);
			const circle ac = circle_from_2(a, c);
			const circle bc = circle_from_2(b, c);
			return std::max({ ab, ac, bc }, [](const circle& l, const circle& r) { return l.r2 < r.r2; });
		}

		const f64 a2 = ax * ax + ay * ay;
		const f64 b2 = bx * bx + by * by;
		const f64 c2 = cx * cx + cy * cy;
		const f64 x = (a2 * (by - cy) + b2 * (cy - ay) + c2 * (ay - by)) / d;
		const f64 y = (a2 * (cx - bx) + b2 * (ax - cx) + c2 * (bx - ax)) / d;
		const f64 dx = ax - x;
		const f64 dy = ay - y;
		return circle{ x, y, dx * dx + dy * dy };
	};

	// Randomize the order to get the expected linear runtime. Use a fixed seed to get reproducible results.
	std::mt19937 rng(static_cast<u32>(points.size()));
	std::shuffle(points.begin(), points.end(), rng);

	circle c{ static_cast<f64>(points[0].x), static_cast<f64>(points[0].y), 0.0 };

	for (usz i = 1; i < points.size(); i++)
	{
		if (contains(c, points[i]))
		{
			continue;
		}

		// points[i] has to be on the boundary
		c = circle{ static_cast<f64>(points[i].x), static_cast<f64>(points[i].y), 0.0 };

		for (usz j = 0; j < i; j++)
		{
			if (contains(c, points[j]))
			{
				continue;
			}

			// points[i] and points[j] have to be on the boundary
			c = circle_from_2(points[i], points[j]);

			for (usz k = 0; k < j; k++)
			{
				if (!contains(c, points[k]))
				{
					c = circle_from_3(points[i], points[j], points[k]);
				}
			}
		}
	}

	center_x = static_cast<f32>(c.x);
	center_y = static_cast<f32>(c.y);
	radius = static_cast<f32>(std::sqrt(c.r2));
}

namespace rgba_draw
{
	static inline void set_pixel(std::vector<u8>& rgba, u32 width, u32 height, s32 x, s32 y, const std::array<u8, 4>& color)
	{
		if (x < 0 || y < 0 || static_cast<u32>(x) >= width || static_cast<u32>(y) >= height)
		{
			return;
		}

		std::copy(color.begin(), color.end(), &rgba[(static_cast<usz>(y) * width + x) * 4]);
	}

	void circle(std::vector<u8>& rgba, u32 width, u32 height, f32 center_x, f32 center_y, s32 radius, const std::array<u8, 4>& color, bool filled)
	{
		const usz size = usz{width} * height * 4u;

		if (rgba.empty() || rgba.size() != size || radius < 0)
		{
			return;
		}

		const s32 cx = static_cast<s32>(std::lround(center_x));
		const s32 cy = static_cast<s32>(std::lround(center_y));

		// Outline: all pixels within half a pixel of the radius
		const f32 outer = radius + 0.5f;
		const f32 inner = std::max(0.0f, radius - 0.5f);
		const f32 outer2 = outer * outer;
		const f32 inner2 = inner * inner;
		const s32 r2 = radius * radius;

		const s32 y_begin = std::max(0, cy - radius - 1);
		const s32 y_end = std::min(static_cast<s32>(height) - 1, cy + radius + 1);
		const s32 x_begin = std::max(0, cx - radius - 1);
		const s32 x_end = std::min(static_cast<s32>(width) - 1, cx + radius + 1);

		for (s32 y = y_begin; y <= y_end; y++)
		{
			for (s32 x = x_begin; x <= x_end; x++)
			{
				const s32 dx = x - cx;
				const s32 dy = y - cy;
				const s32 d2 = dx * dx + dy * dy;

				if (filled ? (d2 <= r2) : (d2 >= inner2 && d2 < outer2))
				{
					set_pixel(rgba, width, height, x, y, color);
				}
			}
		}
	}

	void line(std::vector<u8>& rgba, u32 width, u32 height, s32 x0, s32 y0, s32 x1, s32 y1, const std::array<u8, 4>& color)
	{
		const usz size = usz{width} * height * 4u;

		if (rgba.empty() || rgba.size() != size)
		{
			return;
		}

		// Bresenham
		const s32 dx = std::abs(x1 - x0);
		const s32 dy = -std::abs(y1 - y0);
		const s32 sx = x0 < x1 ? 1 : -1;
		const s32 sy = y0 < y1 ? 1 : -1;
		s32 err = dx + dy;

		while (true)
		{
			set_pixel(rgba, width, height, x0, y0, color);

			if (x0 == x1 && y0 == y1)
			{
				break;
			}

			const s32 e2 = 2 * err;

			if (e2 >= dy)
			{
				err += dy;
				x0 += sx;
			}

			if (e2 <= dx)
			{
				err += dx;
				y0 += sy;
			}
		}
	}
}
