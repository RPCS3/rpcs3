/*  ZeroGS KOSMOS
 *  Copyright (C) 2005-2006 zerofrog@gmail.com
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef RasterFont_Header
#define RasterFont_Header

class RasterFont
{

	protected:
		int	fontOffset;

	public:
		RasterFont();
		~RasterFont(void);
		static int debug;

		// some useful constants
		enum	{char_width = 10};
		enum	{char_height = 15};

		// and the happy helper functions
		void printString(const char *s, double x, double y, double z = 0.0);
		void printCenteredString(const char *s, double y, int screen_width, double z = 0.0);
};

#endif
