/* SPU2-X, A plugin for Emulating the Sound Processing Unit of the Playstation 2
 * Developed and maintained by the Pcsx2 Development Team.
 *
 * SPU2-X is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Lesser General Public License as published by the Free Software Found-
 * ation, either version 3 of the License, or (at your option) any later version.
 *
 * SPU2-X is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with SPU2-X.  If not, see <http://www.gnu.org/licenses/>.
 */


#pragma once

struct StereoOut32
{
	static StereoOut32 Empty;

	s32 Left;
	s32 Right;

	StereoOut32() :
		Left( 0 ),
		Right( 0 )
	{
	}

	StereoOut32( s32 left, s32 right ) :
		Left( left ),
		Right( right )
	{
	}

	StereoOut32( const StereoOut16& src );
	explicit StereoOut32( const StereoOutFloat& src );

	StereoOut16 DownSample() const;

	StereoOut32 operator*( const int& factor ) const
	{
		return StereoOut32(
			Left * factor,
			Right * factor
		);
	}

	StereoOut32& operator*=( const int& factor )
	{
		Left *= factor;
		Right *= factor;
		return *this;
	}

	StereoOut32 operator+( const StereoOut32& right ) const
	{
		return StereoOut32(
			Left + right.Left,
			Right + right.Right
		);
	}

	StereoOut32 operator/( int src ) const
	{
		return StereoOut32( Left / src, Right / src );
	}

	void ResampleFrom( const StereoOut32& src )
	{
		this->Left = src.Left << 2;
		this->Right = src.Right << 2;
	}

};


extern void	Mix();
extern s32	clamp_mix( s32 x, u8 bitshift=0 );
extern s32	MulShr32( s32 srcval, s32 mulval );

extern StereoOut32 clamp_mix( const StereoOut32& sample, u8 bitshift=0 );
