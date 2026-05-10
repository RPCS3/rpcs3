#pragma once

#include "common.h"

namespace gl
{
	class pixel_pack_settings
	{
		bool m_swap_bytes = false;
		bool m_lsb_first = false;
		int m_row_length = 0;
		int m_image_height = 0;
		int m_skip_rows = 0;
		int m_skip_pixels = 0;
		int m_skip_images = 0;
		int m_alignment = 4;

	public:
		void apply() const
		{
			glPixelStorei(GL_PACK_SWAP_BYTES, m_swap_bytes ? GL_TRUE : GL_FALSE);
			glPixelStorei(GL_PACK_LSB_FIRST, m_lsb_first ? GL_TRUE : GL_FALSE);
			glPixelStorei(GL_PACK_ROW_LENGTH, m_row_length);
			glPixelStorei(GL_PACK_IMAGE_HEIGHT, m_image_height);
			glPixelStorei(GL_PACK_SKIP_ROWS, m_skip_rows);
			glPixelStorei(GL_PACK_SKIP_PIXELS, m_skip_pixels);
			glPixelStorei(GL_PACK_SKIP_IMAGES, m_skip_images);
			glPixelStorei(GL_PACK_ALIGNMENT, m_alignment);
		}

		pixel_pack_settings& swap_bytes(bool value = true)
		{
			m_swap_bytes = value;
			return *this;
		}
		pixel_pack_settings& lsb_first(bool value = true)
		{
			m_lsb_first = value;
			return *this;
		}
		pixel_pack_settings& row_length(int value)
		{
			m_row_length = value;
			return *this;
		}
		pixel_pack_settings& image_height(int value)
		{
			m_image_height = value;
			return *this;
		}
		pixel_pack_settings& skip_rows(int value)
		{
			m_skip_rows = value;
			return *this;
		}
		pixel_pack_settings& skip_pixels(int value)
		{
			m_skip_pixels = value;
			return *this;
		}
		pixel_pack_settings& skip_images(int value)
		{
			m_skip_images = value;
			return *this;
		}
		pixel_pack_settings& alignment(int value)
		{
			m_alignment = value;
			return *this;
		}

		bool get_swap_bytes() const
		{
			return m_swap_bytes;
		}
		int get_row_length() const
		{
			return m_row_length;
		}
	};

	class pixel_unpack_settings
	{
		bool m_swap_bytes = false;
		bool m_lsb_first = false;
		int m_row_length = 0;
		int m_image_height = 0;
		int m_skip_rows = 0;
		int m_skip_pixels = 0;
		int m_skip_images = 0;
		int m_alignment = 4;

	public:
		void apply() const
		{
			glPixelStorei(GL_UNPACK_SWAP_BYTES, m_swap_bytes ? GL_TRUE : GL_FALSE);
			glPixelStorei(GL_UNPACK_LSB_FIRST, m_lsb_first ? GL_TRUE : GL_FALSE);
			glPixelStorei(GL_UNPACK_ROW_LENGTH, m_row_length);
			glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, m_image_height);
			glPixelStorei(GL_UNPACK_SKIP_ROWS, m_skip_rows);
			glPixelStorei(GL_UNPACK_SKIP_PIXELS, m_skip_pixels);
			glPixelStorei(GL_UNPACK_SKIP_IMAGES, m_skip_images);
			glPixelStorei(GL_UNPACK_ALIGNMENT, m_alignment);
		}

		pixel_unpack_settings& swap_bytes(bool value = true)
		{
			m_swap_bytes = value;
			return *this;
		}
		pixel_unpack_settings& lsb_first(bool value = true)
		{
			m_lsb_first = value;
			return *this;
		}
		pixel_unpack_settings& row_length(int value)
		{
			m_row_length = value;
			return *this;
		}
		pixel_unpack_settings& image_height(int value)
		{
			m_image_height = value;
			return *this;
		}
		pixel_unpack_settings& skip_rows(int value)
		{
			m_skip_rows = value;
			return *this;
		}
		pixel_unpack_settings& skip_pixels(int value)
		{
			m_skip_pixels = value;
			return *this;
		}
		pixel_unpack_settings& skip_images(int value)
		{
			m_skip_images = value;
			return *this;
		}
		pixel_unpack_settings& alignment(int value)
		{
			m_alignment = value;
			return *this;
		}

		bool get_swap_bytes() const
		{
			return m_swap_bytes;
		}

		int get_row_length() const
		{
			return m_row_length;
		}
	};
}
