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

class GSUniformBufferOGL {
	GLuint buffer;		// data object
	GLuint index;		// GLSL slot
	uint32   size;	    // size of the data

public:
	GSUniformBufferOGL(GLuint index, uint32 size) : index(index)
												  , size(size)
	{
		gl_GenBuffers(1, &buffer);
		bind();
		allocate();
		attach();
	}

	void bind()
	{
		gl_BindBuffer(GL_UNIFORM_BUFFER, buffer);
	}

	void allocate()
	{
		gl_BufferData(GL_UNIFORM_BUFFER, size, NULL, GL_STREAM_DRAW);
	}

	void attach()
	{
		gl_BindBufferBase(GL_UNIFORM_BUFFER, index, buffer);
	}

	void upload(const void* src)
	{
		// glMapBufferRange allow to set various parameter but the call is
		// synchronous whereas glBufferSubData could be asynchronous.
		// TODO: investigate the extension ARB_invalidate_subdata
		gl_BufferSubData(GL_UNIFORM_BUFFER, 0, size, src);
	}

	~GSUniformBufferOGL() {
		gl_DeleteBuffers(1, &buffer);
	}
};
