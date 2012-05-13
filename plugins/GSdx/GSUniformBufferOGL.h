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
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

class GSUniformBufferOGL {
	GLuint buffer;		// data object
	GLuint index;		// GLSL slot
	uint   size;	    // size of the data
	const GLenum target;

public:
	GSUniformBufferOGL(GLuint index, uint size) : index(index)
												  , size(size)
												  ,target(GL_UNIFORM_BUFFER)
	{
		glGenBuffers(1, &buffer);
		bind();
		allocate();
		attach();
	}

	void bind()
	{
		glBindBuffer(target, buffer);
	}

	void allocate()
	{
		glBufferData(target, size, NULL, GL_STREAM_DRAW);
	}

	void attach()
	{
		glBindBufferBase(target, index, buffer);
	}

	void upload(const void* src)
	{
		uint32 flags = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
		uint8* dst = (uint8*) glMapBufferRange(target, 0, size, flags);
		memcpy(dst, src, size);
		glUnmapBuffer(target);
	}

	~GSUniformBufferOGL() {
		glDeleteBuffers(1, &buffer);
	}
};
