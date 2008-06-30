#pragma once

#include "content.h"
#include <string>

namespace YAML
{
	class Scalar: public Content
	{
	public:
		Scalar();
		virtual ~Scalar();

		virtual void Parse(Scanner *pScanner);
		virtual void Write(std::ostream& out, int indent);

	protected:
		std::string m_data;
	};
}
