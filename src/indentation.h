#pragma once

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

