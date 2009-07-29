#pragma once

#ifndef INDENTATION_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define INDENTATION_H_62B23520_7C8E_11DE_8A39_0800200C9A66


#include "ostream.h"
#include <iostream>

namespace YAML
{
	struct Indentation {
		Indentation(unsigned n_): n(n_) {}
		unsigned n;
	};
	
	inline ostream& operator << (ostream& out, const Indentation& indent) {
		for(unsigned i=0;i<indent.n;i++)
			out << ' ';
		return out;
	}

	struct IndentTo {
		IndentTo(unsigned n_): n(n_) {}
		unsigned n;
	};
	
	inline ostream& operator << (ostream& out, const IndentTo& indent) {
		while(out.col() < indent.n)
			out << ' ';
		return out;
	}
}


#endif // INDENTATION_H_62B23520_7C8E_11DE_8A39_0800200C9A66
