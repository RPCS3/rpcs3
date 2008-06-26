#pragma once

#include "content.h"
#include <string>

namespace YAML
{
	class Scalar: public Content
	{
	public:
		Scalar(const std::string& data);
		virtual ~Scalar();

	protected:
		std::string m_data;
	};
}
