#pragma once

#include <ios>
#include <string>
#include <queue>
#include <stack>

namespace YAML
{
	class Token;

	namespace Keys
	{
		const char Comment = '#';
	}

	class Scanner
	{
	public:
		Scanner(std::istream& in);
		~Scanner();

		Token *ScanNextToken();
		void ScanToNextToken();

		void Scan();

	private:
		char GetChar();
		void EatLineBreak();
		void EatDocumentStart();
		void EatDocumentEnd();

		bool IsWhitespaceToBeEaten();
		bool IsLineBreak();
		bool IsBlank();
		bool IsDocumentStart();
		bool IsDocumentEnd();
		template <typename T> T *ScanToken(T *pToken);

	private:
		// the stream
		std::istream& INPUT;
		int m_column;

		// the output (tokens)
		std::queue <Token *> m_tokens;

		// state info
		bool m_startedStream;
		bool m_simpleKeyAllowed;
		int m_flowLevel;                // number of unclosed '[' and '{' indicators
		std::stack <int> m_indents;
	};
}
