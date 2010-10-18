#pragma once

#ifndef EVENTHANDLER_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define EVENTHANDLER_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#include "yaml-cpp/anchor.h"
#include <string>

namespace YAML
{
	struct Mark;
	
	class EventHandler
	{
	public:
		virtual ~EventHandler() {}

		virtual void OnDocumentStart(const Mark& mark) = 0;
		virtual void OnDocumentEnd() = 0;
		
		virtual void OnNull(const std::string& tag, anchor_t anchor) = 0;
		virtual void OnAlias(const Mark& mark, anchor_t anchor) = 0;
		virtual void OnScalar(const Mark& mark, const std::string& tag, anchor_t anchor, const std::string& value) = 0;

		virtual void OnSequenceStart(const Mark& mark, const std::string& tag, anchor_t anchor) = 0;
		virtual void OnSequenceEnd() = 0;

		virtual void OnMapStart(const Mark& mark, const std::string& tag, anchor_t anchor) = 0;
		virtual void OnMapEnd() = 0;
	};
}

#endif // EVENTHANDLER_H_62B23520_7C8E_11DE_8A39_0800200C9A66

