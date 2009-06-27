/* 
 *	Copyright (C) 2007-2009 Gabest
 *	http://www.gabest.org
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

#include "GSTextureCache.h"
#include "GSDevice11.h"

class GSTextureCache11 : public GSTextureCache
{
	class Source11 : public Source
	{
	public:
		explicit Source11(GSRenderer* r) : Source(r) {}

		bool Create();
		bool Create(Target* dst);
	};

	class Target11 : public Target
	{
	public:
		explicit Target11(GSRenderer* r) : Target(r) {}

		void Read(const GSVector4i& r);
	};

protected:
	Source* CreateSource() {return new Source11(m_renderer);}
	Target* CreateTarget() {return new Target11(m_renderer);}

public:
	GSTextureCache11(GSRenderer* r);
};
