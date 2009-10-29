#pragma once

#ifndef SCANTAG_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define SCANTAG_H_62B23520_7C8E_11DE_8A39_0800200C9A66


#include <string>
#include "stream.h"

namespace YAML
{
	const std::string ScanVerbatimTag(Stream& INPUT);
	const std::string ScanTagHandle(Stream& INPUT, bool& canBeHandle);
	const std::string ScanTagSuffix(Stream& INPUT);
}

#endif // SCANTAG_H_62B23520_7C8E_11DE_8A39_0800200C9A66

