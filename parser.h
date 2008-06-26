#pragma once

#include <ios>
#include <string>
#include <stack>

namespace YAML
{
	class Node;

	const std::string DocStart = "---";
	const std::string DocEnd = "...";

	const char SeqToken = '-';

	enum CONTEXT { C_BLOCK, C_FLOW };

	class Parser
	{
	public:
		struct State
		{
			State(CONTEXT context_, int indent_, bool startingNewLine_)
			: context(context_), indent(indent_), startingNewLine(startingNewLine_) {}

			CONTEXT context;
			int indent;
			bool startingNewLine;
		};

	public:
		Parser(std::istream& in);
		~Parser();

		operator bool () const;
		bool operator !() const;

		// parse helpers
		static bool IsWhitespace(char ch);
		void Putback(const std::string& str);
		void StripWhitespace(int n = -1);
		int GetNumOfSpaces();
		bool SeqContinues();

		// readers
		Node *ReadNextNode();
		std::string ReadNextToken();

	private:
		bool m_ok;

		std::istream& INPUT;
		std::stack <State> m_state;
	};
}
