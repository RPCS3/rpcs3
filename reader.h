#pragma once

#include <ios>
#include "document.h"

namespace YAML
{
	class Parser;

	class Reader
	{
	public:
		Reader(std::istream& in);
		~Reader();

		void GetNextDocument(Document& document);

	private:
		Parser *m_pParser;
	};
}
