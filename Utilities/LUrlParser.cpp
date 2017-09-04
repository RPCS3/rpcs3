/*
 * Lightweight URL & URI parser (RFC 1738, RFC 3986)
 * https://github.com/corporateshark/LUrlParser
 * 
 * The MIT License (MIT)
 * 
 * Copyright (C) 2015 Sergey Kosarevsky (sk@linderdaum.com)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "LUrlParser.h"

#include <algorithm>
#include <cstring>
#include <stdlib.h>

// check if the scheme name is valid
static bool IsSchemeValid( const std::string& SchemeName )
{
	for ( auto c : SchemeName  )
	{
		if ( !isalpha( c ) && c != '+' && c != '-' && c != '.' ) return false;
	}

	return true;
}

bool LUrlParser::clParseURL::GetPort( int* OutPort ) const
{
	if ( !IsValid() ) { return false; }

	int Port = atoi( m_Port.c_str() );

	if ( Port <= 0 || Port > 65535 ) { return false; }

	if ( OutPort ) { *OutPort = Port; }

	return true;
}

// based on RFC 1738 and RFC 3986
LUrlParser::clParseURL LUrlParser::clParseURL::ParseURL( const std::string& URL )
{
	LUrlParser::clParseURL Result;

	const char* CurrentString = URL.c_str();

	/*
	 *	<scheme>:<scheme-specific-part>
	 *	<scheme> := [a-z\+\-\.]+
	 *	For resiliency, programs interpreting URLs should treat upper case letters as equivalent to lower case in scheme names
	 */

	// try to read scheme
	{
		const char* LocalString = strchr( CurrentString, ':' );

		if ( !LocalString )
		{
			return clParseURL( LUrlParserError_NoUrlCharacter );
		}

		// save the scheme name
		Result.m_Scheme = std::string( CurrentString, LocalString - CurrentString );

		if ( !IsSchemeValid( Result.m_Scheme ) )
		{
			return clParseURL( LUrlParserError_InvalidSchemeName );
		}

		// scheme should be lowercase
		std::transform( Result.m_Scheme.begin(), Result.m_Scheme.end(), Result.m_Scheme.begin(), ::tolower );

		// skip ':'
		CurrentString = LocalString+1;
	}

	/*
	 *	//<user>:<password>@<host>:<port>/<url-path>
	 *	any ":", "@" and "/" must be normalized
	 */

	// skip "//"
	if ( *CurrentString++ != '/' ) return clParseURL( LUrlParserError_NoDoubleSlash );
	if ( *CurrentString++ != '/' ) return clParseURL( LUrlParserError_NoDoubleSlash );

	// check if the user name and password are specified
	bool bHasUserName = false;

	const char* LocalString = CurrentString;

	while ( *LocalString )
	{
		if ( *LocalString == '@' )
		{
			// user name and password are specified
			bHasUserName = true;
			break;
		}
		else if ( *LocalString == '/' )
		{
			// end of <host>:<port> specification
			bHasUserName = false;
			break;
		}

		LocalString++;
	}

	// user name and password
	LocalString = CurrentString;

	if ( bHasUserName )
	{
		// read user name
		while ( *LocalString && *LocalString != ':' && *LocalString != '@' ) LocalString++;

		Result.m_UserName = std::string( CurrentString, LocalString - CurrentString );

		// proceed with the current pointer
		CurrentString = LocalString;

		if ( *CurrentString == ':' )
		{
			// skip ':'
			CurrentString++;

			// read password
			LocalString = CurrentString;

			while ( *LocalString && *LocalString != '@' ) LocalString++;

			Result.m_Password = std::string( CurrentString, LocalString - CurrentString );

			CurrentString = LocalString;
		}

		// skip '@'
		if ( *CurrentString != '@' )
		{
			return clParseURL( LUrlParserError_NoAtSign );
		}

		CurrentString++;
	}

	bool bHasBracket = ( *CurrentString == '[' );

	// go ahead, read the host name
	LocalString = CurrentString;

	while ( *LocalString )
	{
		if ( bHasBracket && *LocalString == ']' )
		{
			// end of IPv6 address
			LocalString++;
			break;
		}
		else if ( !bHasBracket && ( *LocalString == ':' || *LocalString == '/' ) )
		{
			// port number is specified
			break;
		}

		LocalString++;
	}

	Result.m_Host = std::string( CurrentString, LocalString - CurrentString );

	CurrentString = LocalString;

	// is port number specified?
	if ( *CurrentString == ':' )
	{
		CurrentString++;

		// read port number
		LocalString = CurrentString;

		while ( *LocalString && *LocalString != '/' ) LocalString++;

		Result.m_Port = std::string( CurrentString, LocalString - CurrentString );

		CurrentString = LocalString;
	}

	// end of string
	if ( !*CurrentString )
	{
		Result.m_ErrorCode = LUrlParserError_Ok;

		return Result;
	}

	// skip '/'
	if ( *CurrentString != '/' )
	{
		return clParseURL( LUrlParserError_NoSlash );
	}

	CurrentString++;

	// parse the path
	LocalString = CurrentString;

	while ( *LocalString && *LocalString != '#' && *LocalString != '?' ) LocalString++;

	Result.m_Path = std::string( CurrentString, LocalString - CurrentString );

	CurrentString = LocalString;

	// check for query
	if ( *CurrentString == '?' )
	{
		// skip '?'
		CurrentString++;

		// read query
		LocalString = CurrentString;

		while ( *LocalString && *LocalString != '#' ) LocalString++;

		Result.m_Query = std::string( CurrentString, LocalString - CurrentString );

		CurrentString = LocalString;
	}

	// check for fragment
	if ( *CurrentString == '#' )
	{
		// skip '#'
		CurrentString++;

		// read fragment
		LocalString = CurrentString;

		while ( *LocalString ) LocalString++;

		Result.m_Fragment = std::string( CurrentString, LocalString - CurrentString );

		CurrentString = LocalString;
	}

	Result.m_ErrorCode = LUrlParserError_Ok;

	return Result;
}
