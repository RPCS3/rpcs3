/*
 *	Copyright (C) 2011-2011 Gregory hainaut
 *	Copyright (C) 2007-2009 Gabest
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "GSTexture.h"

namespace PboPool {
	extern GLuint pool[8];
	extern uint32 current_pbo;

	void BindPbo();
	void UnbindPbo();

	void Init();
	void Destroy();
}

class GSTextureOGL : public GSTexture
{
	private:
		GLuint m_texture_id;	 // the texture id
		uint32 m_pbo_id;
		int m_pbo_size;
		GLuint m_fbo_read;

		// internal opengl format/type/alignment
		GLenum m_int_format;
		GLenum m_int_type;
		uint32 m_int_alignment;
		uint32 m_int_shift;

		GLuint64 m_handles[12];

	public:
		explicit GSTextureOGL(int type, int w, int h, int format, GLuint fbo_read);
		virtual ~GSTextureOGL();

		bool Update(const GSVector4i& r, const void* data, int pitch);
		bool Map(GSMap& m, const GSVector4i* r = NULL);
		void Unmap();
		bool Save(const string& fn, bool dds = false);
		void Save(const string& fn, const void* image, uint32 pitch);
		void SaveRaw(const string& fn, const void* image, uint32 pitch);

		void Clear(const void *data);

		void EnableUnit();

		bool IsBackbuffer() { return (m_type == GSTexture::Backbuffer); }
		bool IsDss() { return (m_type == GSTexture::DepthStencil); }

		GLuint GetID() { return m_texture_id; }
		GLuint64 GetHandle(GLuint sampler_id);
};
