#pragma once

namespace YAML
{
	class Token {};
	class StreamStartToken: public Token {};
	class StreamEndToken: public Token {};
	class DocumentStartToken: public Token {};
	class DocumentEndToken: public Token {};
}
