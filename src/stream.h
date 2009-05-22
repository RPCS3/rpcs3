#pragma once

#include <ios>
#include <string>

namespace YAML
{
	// a simple buffer wrapper that knows how big it is
	struct Buffer {
		Buffer(const char *b, int s): buffer(b), size(s) {}

		operator bool() const { return size > 0; }
		bool operator !() const { return !static_cast <bool> (*this); }
		char operator [] (int i) const { return buffer[i]; }
		const Buffer operator + (int offset) const { return Buffer(buffer + offset, size - offset); }
		Buffer& operator ++ () { ++buffer; --size; return *this; }

		const char *buffer;
		int size;
	};

	class Stream
	{
	public:
		Stream(std::istream& input);
		~Stream();

		operator bool() const;
		bool operator !() const { return !static_cast <bool>(*this); }

		const Buffer current() const { return Buffer(buffer + pos, size - pos); }
		char peek();
		char get();
		std::string get(int n);
		void eat(int n = 1);

		int pos, line, column, size;
	
	private:
		char *buffer;
	};
}
