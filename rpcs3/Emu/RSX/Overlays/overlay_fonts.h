#pragma once

#include "util/types.hpp"
#include "overlay_utils.h"

#include <memory>
#include <vector>

// STB_IMAGE_IMPLEMENTATION and STB_TRUETYPE_IMPLEMENTATION defined externally
#include <stb_image.h>
#include <stb_truetype.h>

namespace rsx
{
	namespace overlays
	{
		enum class language_class
		{
			default_ = 0,   // Typically latin-1, extended latin, hebrew, arabic and cyrillic
			cjk_base = 1,   // The thousands of CJK glyphs occupying pages 2E-9F
			hangul = 2      // Korean jamo
		};

		struct glyph_load_setup
		{
			std::vector<std::string> font_names;
			std::vector<std::string> lookup_font_dirs;
		};

		// Each 'page' holds an indexed block of 256 code points
		// The BMP (Basic Multilingual Plane) has 256 allocated pages but not all are necessary
		// While there are supplementary planes, the BMP is the most important thing to support
		struct codepage
		{
			static constexpr u32 bitmap_width = 1024;
			static constexpr u32 bitmap_height = 1024;
			static constexpr u32 char_count = 256; // 16x16 grid at max 48pt
			static constexpr u32 oversample = 2;

			std::vector<stbtt_packedchar> pack_info;
			std::vector<u8> glyph_data;
			char32_t glyph_base = 0;
			f32 sampler_z = 0.f;

			void initialize_glyphs(char32_t codepage_id, f32 font_size, const std::vector<u8>& ttf_data);
			stbtt_aligned_quad get_char(char32_t c, f32& x_advance, f32& y_advance);
		};

		class font
		{
		private:
			f32 size_pt = 12.f;
			f32 size_px = 16.f; // Default font 12pt size
			f32 em_size = 0.f;
			std::string font_name;

			std::vector<std::pair<char32_t, std::unique_ptr<codepage>>> m_glyph_map;
			bool initialized = false;

			struct
			{
				char32_t codepage_id = 0;
				codepage* page = nullptr;
			}
			codepage_cache;

			static char32_t get_page_id(char32_t c) { return c >> 8; }
			static language_class classify(char32_t codepage_id);
			glyph_load_setup get_glyph_files(language_class class_) const;
			codepage* initialize_codepage(char32_t c);
		public:

			font(std::string_view ttf_name, f32 size);

			stbtt_aligned_quad get_char(char32_t c, f32& x_advance, f32& y_advance);

			std::vector<vertex> render_text_ex(f32& x_advance, f32& y_advance, const char32_t* text, usz char_limit, u16 max_width, bool wrap);

			std::vector<vertex> render_text(const char32_t* text, u16 max_width = -1, bool wrap = false);

			std::pair<f32, f32> get_char_offset(const char32_t* text, usz max_length, u16 max_width = -1, bool wrap = false);

			bool matches(std::string_view name, int size) const { return static_cast<int>(size_pt) == size && font_name == name; }
			std::string_view get_name() const { return font_name; }
			f32 get_size_pt() const { return size_pt; }
			f32 get_size_px() const { return size_px; }
			f32 get_em_size() const { return em_size; }

			// Renderer info
			size3u get_glyph_data_dimensions() const { return { codepage::bitmap_width, codepage::bitmap_height, ::size32(m_glyph_map) }; }
			const std::vector<u8>& get_glyph_data() const;
		};

		// TODO: Singletons are cancer
		class fontmgr
		{
		private:
			std::vector<std::unique_ptr<font>> fonts;
			static fontmgr* m_instance;

			font* find(std::string_view name, int size)
			{
				for (const auto& f : fonts)
				{
					if (f->matches(name, size))
						return f.get();
				}

				fonts.push_back(std::make_unique<font>(name, static_cast<f32>(size)));
				return fonts.back().get();
			}

		public:

			fontmgr() = default;
			~fontmgr()
			{
				if (m_instance)
				{
					delete m_instance;
					m_instance = nullptr;
				}
			}

			static font* get(std::string_view name, int size)
			{
				if (m_instance == nullptr)
					m_instance = new fontmgr;

				return m_instance->find(name, size);
			}
		};
	}
}
