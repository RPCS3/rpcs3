#pragma once

#ifndef SCALAR_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define SCALAR_H_62B23520_7C8E_11DE_8A39_0800200C9A66


#include "content.h"
#include <string>

namespace YAML
{
	class Scalar: public Content
	{
	public:
		Scalar();
		Scalar(const std::string& data);
		virtual ~Scalar();

		virtual Content *Clone() const;

		virtual void Parse(Scanner *pScanner, ParserState& state);
		virtual void Write(Emitter& out) const;

		virtual bool IsScalar() const { return true; }

		// extraction
		virtual bool GetScalar(std::string& scalar) const {
			scalar = m_data;
			return true;
		}

		// ordering
		virtual int Compare(Content *pContent);
		virtual int Compare(Scalar *pScalar);
		virtual int Compare(Sequence *) { return -1; }
		virtual int Compare(Map *) { return -1; }

	protected:
		std::string m_data;
	};
}

#endif // SCALAR_H_62B23520_7C8E_11DE_8A39_0800200C9A66

