#pragma once

#include "Utilities/types.h"
#include "overlay_utils.h"

#include <vector>
#include <unordered_map>

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

			language_class classify(char32_t page);
			glyph_load_setup get_glyph_files(language_class class_);
			codepage* initialize_codepage(char32_t page);
		public:

			font(const char* ttf_name, f32 size);

			stbtt_aligned_quad get_char(char32_t c, f32& x_advance, f32& y_advance);

			void render_text_ex(std::vector<vertex>& result, f32& x_advance, f32& y_advance, const char32_t* text, u32 char_limit, u16 max_width, bool wrap);

			std::vector<vertex> render_text(const char32_t* text, u16 max_width = UINT16_MAX, bool wrap = false);

			std::pair<f32, f32> get_char_offset(const char32_t* text, u16 max_length, u16 max_width = UINT16_MAX, bool wrap = false);

			bool matches(const char* name, int size) const { return font_name == name && static_cast<int>(size_pt) == size; }
			std::string_view get_name() const { return font_name; };
			f32 get_size_pt() const { return size_pt; }
			f32 get_size_px() const { return size_px; }
			f32 get_em_size() const { return em_size; }

			// Renderer info
			size3u get_glyph_data_dimensions() const { return { codepage::bitmap_width, codepage::bitmap_height, ::size32(m_glyph_map) }; }
			void get_glyph_data(std::vector<u8>& bytes) const;
		};

		// TODO: Singletons are cancer
		class fontmgr
		{
		private:
			std::vector<std::unique_ptr<font>> fonts;
			static fontmgr* m_instance;

			font* find(const char* name, int size)
			{
				for (auto& f : fonts)
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

			static font* get(const char* name, int size)
			{
				if (m_instance == nullptr)
					m_instance = new fontmgr;

				return m_instance->find(name, size);
			}
		};
	}
}
