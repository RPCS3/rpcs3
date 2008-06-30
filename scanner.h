#pragma once

#include <ios>
#include <string>
#include <queue>
#include <stack>
#include <set>
#include "stream.h"

namespace YAML
{
	struct Token;

	class Scanner
	{
	public:
		Scanner(std::istream& in);
		~Scanner();

		Token *GetNextToken();
		void EatNextToken();
		void PopNextToken();
		Token *PeekNextToken();

	private:
		// scanning
		void StartStream();
		void EndStream();
		void ScanNextToken();
		void ScanToNextToken();
		Token *PushIndentTo(int column, bool sequence);
		void PopIndentTo(int column);

		// checking input
		void InsertSimpleKey();
		bool VerifySimpleKey();
		void VerifyAllSimpleKeys();

		bool IsWhitespaceToBeEaten(char ch);

		struct SimpleKey {
			SimpleKey(int pos_, int line_, int column_, int flowLevel_);

			void Validate();
			void Invalidate();

			int pos, line, column, flowLevel;
			bool required;
			Token *pMapStart, *pKey;
		};

		// and the tokens
		void ScanDirective();
		void ScanDocStart();
		void ScanDocEnd();
		void ScanBlockSeqStart();
		void ScanBlockMapSTart();
		void ScanBlockEnd();
		void ScanBlockEntry();
		void ScanFlowStart();
		void ScanFlowEnd();
		void ScanFlowEntry();
		void ScanKey();
		void ScanValue();
		void ScanAnchorOrAlias();
		void ScanTag();
		void ScanPlainScalar();
		void ScanQuotedScalar();
		void ScanBlockScalar();

	private:
		// the stream
		Stream INPUT;

		// the output (tokens)
		std::queue <Token *> m_tokens;

		// state info
		bool m_startedStream, m_endedStream;
		bool m_simpleKeyAllowed;
		int m_flowLevel;                // number of unclosed '[' and '{' indicators
		bool m_isLastKeyValid;
		std::stack <SimpleKey> m_simpleKeys;
		std::stack <int> m_indents;
	};
}
