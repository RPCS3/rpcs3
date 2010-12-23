/* SPU2-X, A plugin for Emulating the Sound Processing Unit of the Playstation 2
 * Developed and maintained by the Pcsx2 Development Team.
 *
 * Original portions from SPU2ghz are (c) 2008 by David Quintana [gigaherz]
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

struct FrequencyResponseFilter
{
	static FrequencyResponseFilter Empty;

	StereoOut32 History_One_In;
	StereoOut32 History_One_Out;
	StereoOut32 History_Two_In;
	StereoOut32 History_Two_Out;

	s32 lx1;
	s32 lx2;
	s32 ly1;
	s32 ly2;

	float la0, la1, la2, lb1, lb2;
	float ha0, ha1, ha2, hb1, hb2;

	FrequencyResponseFilter() :
		History_One_In( 0, 0 ),
		History_One_Out( 0, 0 ),
		History_Two_In( 0, 0 ),
		History_Two_Out( 0, 0 ),
		lx1 ( 0 ),
		lx2 ( 0 ),
		ly1 ( 0 ),
		ly2 ( 0 ),
		
		la0 ( 1.00320890889339290000 ),
		la1 ( -1.97516434134506300000 ),
		la2 ( 0.97243484967313087000 ),
		lb1 ( -1.97525280404731810000 ),
		lb2 ( 0.97555529586426892000 ),

		ha0 ( 1.52690772687271160000 ),
		ha1 ( -1.62653918974914990000 ),
		ha2 ( 0.57997976029249387000 ),
		hb1 ( -0.80955590379048203000 ),
		hb2 ( 0.28990420120653748000 )
	{
	}

};

extern void	Mix();
extern s32	clamp_mix( s32 x, u8 bitshift=0 );

extern StereoOut32 clamp_mix( const StereoOut32& sample, u8 bitshift=0 );
