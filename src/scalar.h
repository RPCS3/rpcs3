#ifndef SCALAR_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define SCALAR_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if !defined(__GNUC__) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4) // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif


#include "content.h"
#include <string>

namespace YAML
{
	class Scalar: public Content
	{
	public:
		Scalar();
		virtual ~Scalar();

		virtual void SetData(const std::string& data) { m_data = data; }

		virtual bool IsScalar() const { return true; }
		virtual void EmitEvents(AliasManager& am, EventHandler& eventHandler, const Mark& mark, const std::string& tag, anchor_t anchor) const;

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

