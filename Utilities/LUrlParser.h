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

#pragma once

#include <string>

namespace LUrlParser
{
	enum LUrlParserError
	{
		LUrlParserError_Ok = 0,
		LUrlParserError_Uninitialized = 1,
		LUrlParserError_NoUrlCharacter = 2,
		LUrlParserError_InvalidSchemeName = 3,
		LUrlParserError_NoDoubleSlash = 4,
		LUrlParserError_NoAtSign = 5,
		LUrlParserError_UnexpectedEndOfLine = 6,
		LUrlParserError_NoSlash = 7,
	};

	class clParseURL
	{
	public:
		LUrlParserError m_ErrorCode;
		std::string m_Scheme;
		std::string m_Host;
		std::string m_Port;
		std::string m_Path;
		std::string m_Query;
		std::string m_Fragment;
		std::string m_UserName;
		std::string m_Password;

		clParseURL()
			: m_ErrorCode( LUrlParserError_Uninitialized )
		{}

		/// return 'true' if the parsing was successful
		bool IsValid() const { return m_ErrorCode == LUrlParserError_Ok; }

		/// helper to convert the port number to int, return 'true' if the port is valid (within the 0..65535 range)
		bool GetPort( int* OutPort ) const;

		/// parse the URL
		static clParseURL ParseURL( const std::string& URL );

	private:
		explicit clParseURL( LUrlParserError ErrorCode )
			: m_ErrorCode( ErrorCode )
		{}
	};

} // namespace LUrlParser
