/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "Dependencies.h"

template< int Precision >
struct FixedInt
{
	s32 Raw;

	FixedInt();
	FixedInt( int signedval );
	FixedInt( double doubval );
	FixedInt( float floval );

	bool operator ==( const FixedInt<Precision>& right ) const { return Raw == right.Raw; }
	bool operator !=( const FixedInt<Precision>& right ) const { return Raw != right.Raw; }

	bool operator>( const FixedInt<Precision>& right ) const	{ return Raw > right.Raw; };
	bool operator>=( const FixedInt<Precision>& right ) const	{ return Raw >= right.Raw; };
	bool operator<( const FixedInt<Precision>& right ) const	{ return Raw < right.Raw; };
	bool operator<=( const FixedInt<Precision>& right ) const	{ return Raw <= right.Raw; };

	FixedInt<Precision> operator+( const FixedInt<Precision>& right ) const;
	FixedInt<Precision> operator-( const FixedInt<Precision>& right ) const;
	FixedInt<Precision>& operator+=( const FixedInt<Precision>& right );
	FixedInt<Precision>& operator-=( const FixedInt<Precision>& right );

	FixedInt<Precision>& ConfineTo( const FixedInt<Precision>& low, const FixedInt<Precision>& high );

	FixedInt<Precision> operator*( const FixedInt<Precision>& right ) const;
	FixedInt<Precision> operator/( const FixedInt<Precision>& right ) const;
	FixedInt<Precision>& operator*=( const FixedInt<Precision>& right );
	FixedInt<Precision>& operator/=( const FixedInt<Precision>& right );

	static bool OverflowCheck( int signedval );
	static bool OverflowCheck( double signedval );

	int GetWhole() const;
	int GetFraction() const;

	FixedInt<Precision>& SetRaw( s32 rawsrc );
	FixedInt<Precision>& Round();
	FixedInt<Precision>& SetWhole( s32 wholepart );
	FixedInt<Precision>& SetFraction( u32 fracpart );

	wxString ToString() const;
	wxString ToString( int fracDigits ) const;

	double ToDouble() const;
	float ToFloat() const;
	int ToIntTruncated() const;
	int ToIntRounded() const;

	static bool TryFromString( FixedInt<Precision>& dest, const wxString& parseFrom );
	static FixedInt<Precision> FromString( const wxString& parseFrom, const FixedInt<Precision>& defval );
	static FixedInt<Precision> FromString( const wxString parseFrom );
};

typedef FixedInt<256>		Fixed256;
typedef FixedInt<100>		Fixed100;
