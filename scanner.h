#pragma once

#include <ios>
#include <string>
#include <queue>
#include <stack>
#include <set>

namespace YAML
{
	struct Token;

	class Scanner
	{
	public:
		Scanner(std::istream& in);
		~Scanner();

		Token *GetNextToken();

		void ScanNextToken();
		void ScanToNextToken();
		Token *PushIndentTo(int column, bool sequence);
		void PopIndentTo(int column);
		void IncreaseFlowLevel();
		void DecreaseFlowLevel();

		void InsertSimpleKey();
		bool ValidateSimpleKey();
		void ValidateAllSimpleKeys();

		void Scan();

	private:
		char GetChar();
		std::string GetChar(int n);
		void Eat(int n = 1);
		void EatLineBreak();

		bool IsWhitespaceToBeEaten(char ch);
		bool IsDocumentStart();
		bool IsDocumentEnd();
		bool IsBlockEntry();
		bool IsKey();
		bool IsValue();
		bool IsPlainScalar();

		struct WhitespaceInfo {
			WhitespaceInfo();

			void AddBlank(char ch);
			void AddBreak(const std::string& line);
			std::string Join();

			bool leadingBlanks;
			std::string whitespace, leadingBreaks, trailingBreaks;
		};

		struct SimpleKey {
			SimpleKey(int pos_, int line_, int column_, int flowLevel_);

			void Validate();
			void Invalidate();

			int pos, line, column, flowLevel;
			bool required;
			Token *pMapStart, *pKey;
		};

		template <typename T> void ScanAndEnqueue(T *pToken);
		template <typename T> T *ScanToken(T *pToken);

	private:
		// the stream
		std::istream& INPUT;
		int m_line, m_column;

		// the output (tokens)
		std::queue <Token *> m_tokens;
		std::set <Token *> m_limboTokens;

		// state info
		bool m_startedStream, m_endedStream;
		bool m_simpleKeyAllowed;
		int m_flowLevel;                // number of unclosed '[' and '{' indicators
		bool m_isLastKeyValid;
		std::stack <SimpleKey> m_simpleKeys;
		std::stack <int> m_indents;
	};
}
