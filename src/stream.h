#pragma once

#include "noncopyable.h"
#include <deque>
#include <ios>
#include <string>
#include <iostream>
#include <set>

namespace YAML
{
	static const size_t MAX_PARSER_PUSHBACK = 8;

	class Stream: private noncopyable
	{
	public:
		friend class StreamCharSource;
		
		Stream(std::istream& input);
		~Stream();

		operator bool() const;
		bool operator !() const { return !static_cast <bool>(*this); }

		char peek() const;
		char get();
		std::string get(int n);
		void eat(int n = 1);

		static char eof() { return 0x04; }

		int pos, line, column;
	
	private:
		enum CharacterSet {utf8, utf16le, utf16be, utf32le, utf32be};

		std::istream& m_input;
		CharacterSet m_charSet;
		unsigned char m_bufPushback[MAX_PARSER_PUSHBACK];
		mutable size_t m_nPushedBack;
		mutable std::deque<char> m_readahead;
		unsigned char* const m_pPrefetched;
		mutable size_t m_nPrefetchedAvailable;
		mutable size_t m_nPrefetchedUsed;
		
		void AdvanceCurrent();
		char CharAt(size_t i) const;
		bool ReadAheadTo(size_t i) const;
		bool _ReadAheadTo(size_t i) const;
		void StreamInUtf8() const;
		void StreamInUtf16() const;
		void StreamInUtf32() const;
		unsigned char GetNextByte() const;
	};

	// CharAt
	// . Unchecked access
	inline char Stream::CharAt(size_t i) const {
		return m_readahead[i];
	}
	
	inline bool Stream::ReadAheadTo(size_t i) const {
		if(m_readahead.size() > i)
			return true;
		return _ReadAheadTo(i);
	}	
}
