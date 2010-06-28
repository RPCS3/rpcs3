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

#include "FixedPointTypes.h"
#include <cmath>		// for pow!

template< int Precision >
FixedInt<Precision>::FixedInt()
{
	Raw = 0;
}

template< int Precision >
FixedInt<Precision>::FixedInt( int signedval )
{
	Raw = signedval * Precision;
}

template< int Precision >
FixedInt<Precision>::FixedInt( double doubval )
{
	Raw = (int)(doubval * (double)Precision);
}

template< int Precision >
FixedInt<Precision>::FixedInt( float floval )
{
	Raw = (int)(floval * (float)Precision);
}

template< int Precision >
FixedInt<Precision> FixedInt<Precision>::operator+( const FixedInt<Precision>& right ) const
{
	return FixedInt<Precision>().SetRaw( Raw + right.Raw );
}

template< int Precision >
FixedInt<Precision> FixedInt<Precision>::operator-( const FixedInt<Precision>& right ) const
{
	return FixedInt<Precision>().SetRaw( Raw + right.Raw );
}

template< int Precision >
FixedInt<Precision>& FixedInt<Precision>::operator+=( const FixedInt<Precision>& right )
{
	return SetRaw( Raw + right.Raw );
}

template< int Precision >
FixedInt<Precision>& FixedInt<Precision>::operator-=( const FixedInt<Precision>& right )
{
	return SetRaw( Raw + right.Raw );
}

template< int Precision >
FixedInt<Precision>& FixedInt<Precision>::ConfineTo( const FixedInt<Precision>& low, const FixedInt<Precision>& high )
{
	return SetRaw( std::min( std::max( Raw, low.Raw ), high.Raw ) );
}

// Uses 64 bit internally to avoid overflows.  For more precise/optimized 32 bit math
// you'll need to use the Raw values directly.
template< int Precision >
FixedInt<Precision> FixedInt<Precision>::operator*( const FixedInt<Precision>& right ) const
{
	s64 mulres = (s64)Raw * right.Raw;
	return FixedInt<Precision>().SetRaw( (s32)(mulres / Precision) );
}

// Uses 64 bit internally to avoid overflows.  For more precise/optimized 32 bit math
// you'll need to use the Raw values directly.
template< int Precision >
FixedInt<Precision> FixedInt<Precision>::operator/( const FixedInt<Precision>& right ) const
{
	s64 divres = Raw * Precision;
	return FixedInt<Precision>().SetRaw( (s32)(divres / right.Raw) );
}

// Uses 64 bit internally to avoid overflows.  For more precise/optimized 32 bit math
// you'll need to use the Raw values directly.
template< int Precision >
FixedInt<Precision>& FixedInt<Precision>::operator*=( const FixedInt<Precision>& right )
{
	s64 mulres = (s64)Raw * right.Raw;
	return SetRaw( (s32)(mulres / Precision) );
}

// Uses 64 bit internally to avoid overflows.  For more precise/optimized 32 bit math
// you'll need to use the Raw values directly.
template< int Precision >
FixedInt<Precision>& FixedInt<Precision>::operator/=( const FixedInt<Precision>& right )
{
	s64 divres = Raw * Precision;
	return SetRaw( (s32)(divres / right.Raw) );
}

// returns TRUE if the value overflows the legal integer range of this container.
template< int Precision >
bool FixedInt<Precision>::OverflowCheck( int signedval )
{
	return ( signedval >= (INT_MAX / Precision) );
}

// returns TRUE if the value overflows the legal integer range of this container.
template< int Precision >
bool FixedInt<Precision>::OverflowCheck( double signedval )
{
	return ( signedval >= (INT_MAX / Precision) );
}

template< int Precision > int FixedInt<Precision>::GetWhole() const		{ return Raw / Precision; }
template< int Precision > int FixedInt<Precision>::GetFraction() const	{ return Raw % Precision; }

template< int Precision >
FixedInt<Precision>& FixedInt<Precision>::SetRaw( s32 rawsrc )
{
	Raw = rawsrc;
	return *this;
}

template< int Precision >
FixedInt<Precision>& FixedInt<Precision>::Round()
{
	Raw = ToIntRounded();
	return *this;
}

template< int Precision >
FixedInt<Precision>& FixedInt<Precision>::SetWhole( s32 wholepart )
{
	pxAssert( wholepart < (INT_MAX / Precision) );
	Raw = GetFraction() + (wholepart * Precision);
	return *this;
}

template< int Precision >
FixedInt<Precision>& FixedInt<Precision>::SetFraction( u32 fracpart )
{
	Raw = (GetWhole() * Precision) + fracpart;
	return *this;
}

template< int Precision >
wxString FixedInt<Precision>::ToString() const
{
	return wxsFormat( L"%d.%d", GetWhole(), (GetFraction() * 100) / Precision );
}

template< int Precision >
wxString FixedInt<Precision>::ToString( int fracDigits ) const
{
	if( fracDigits == 0 ) return wxsFormat( L"%d", GetWhole() );

	pxAssert( fracDigits <= 7 );		// higher numbers would just cause overflows and bad mojo.
	int mulby = (int)pow( 10.0, fracDigits );
	return wxsFormat( L"%d.%d", GetWhole(), (GetFraction() * mulby) / Precision );
}

template< int Precision >
double FixedInt<Precision>::ToDouble() const
{
	return ((double)Raw / (double)Precision);
}

template< int Precision >
float FixedInt<Precision>::ToFloat() const
{
	return ((float)Raw / (float)Precision);
}

template< int Precision >
int FixedInt<Precision>::ToIntTruncated() const
{
	return Raw / Precision;
}

template< int Precision >
int FixedInt<Precision>::ToIntRounded() const
{
	return (Raw + (Precision/2)) / Precision;
}

template< int Precision >
bool FixedInt<Precision>::TryFromString( FixedInt<Precision>& dest, const wxString& parseFrom )
{
	long whole=0, frac=0;
	const wxString beforeFirst( parseFrom.BeforeFirst( L'.' ) );
	const wxString afterFirst( parseFrom.AfterFirst( L'.' ).Mid(0, 5) );
	bool success = true;

	if( !beforeFirst.IsEmpty() )
		success = success && beforeFirst.ToLong( &whole );

	if( !afterFirst.IsEmpty() )
		success = success && afterFirst.ToLong( &frac );

	if( !success ) return false;

	dest.SetWhole( whole );

	if( afterFirst.Length() != 0 && frac != 0 )
	{
		int fracPower = (int)pow( 10.0, (int)afterFirst.Length() );
		dest.SetFraction( (frac * Precision) / fracPower );
	}
	return true;
}

template< int Precision >
FixedInt<Precision> FixedInt<Precision>::FromString( const wxString& parseFrom, const FixedInt<Precision>& defval )
{
	FixedInt<Precision> dest;
	if( !TryFromString( dest, parseFrom ) ) return defval;
	return dest;
}

// This version of FromString throws a ParseError exception if the conversion fails.
template< int Precision >
FixedInt<Precision> FixedInt<Precision>::FromString( const wxString parseFrom )
{
	FixedInt<Precision> dest;
	if( !TryFromString( dest, parseFrom ) ) throw Exception::ParseError()
		.SetDiagMsg(wxsFormat(L"Parse error on FixedInt<%d>::FromString", Precision));

	return dest;
}
