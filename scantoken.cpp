#include "scanner.h"
#include "token.h"
#include "exceptions.h"
#include "exp.h"

namespace YAML
{
	///////////////////////////////////////////////////////////////////////
	// Specialization for scanning specific tokens

	// StreamStartToken
	template <> StreamStartToken *Scanner::ScanToken(StreamStartToken *pToken)
	{
		m_startedStream = true;
		m_simpleKeyAllowed = true;
		m_indents.push(-1);

		return pToken;
	}

	// StreamEndToken
	template <> StreamEndToken *Scanner::ScanToken(StreamEndToken *pToken)
	{
		// force newline
		if(m_column > 0)
			m_column = 0;

		PopIndentTo(-1);
		// TODO: "reset simple keys"

		m_simpleKeyAllowed = false;
		m_endedStream = true;

		return pToken;
	}

	// DocumentStartToken
	template <> DocumentStartToken *Scanner::ScanToken(DocumentStartToken *pToken)
	{
		PopIndentTo(m_column);
		// TODO: "reset simple keys"

		m_simpleKeyAllowed = false;

		// eat
		Eat(3);
		return pToken;
	}

	// DocumentEndToken
	template <> DocumentEndToken *Scanner::ScanToken(DocumentEndToken *pToken)
	{
		PopIndentTo(-1);
		// TODO: "reset simple keys"

		m_simpleKeyAllowed = false;

		// eat
		Eat(3);
		return pToken;
	}

	// FlowSeqStartToken
	template <> FlowSeqStartToken *Scanner::ScanToken(FlowSeqStartToken *pToken)
	{
		// TODO: "save simple key"

		IncreaseFlowLevel();
		m_simpleKeyAllowed = true;

		// eat
		Eat(1);
		return pToken;
	}

	// FlowMapStartToken
	template <> FlowMapStartToken *Scanner::ScanToken(FlowMapStartToken *pToken)
	{
		// TODO: "save simple key"

		IncreaseFlowLevel();
		m_simpleKeyAllowed = true;

		// eat
		Eat(1);
		return pToken;
	}

	// FlowSeqEndToken
	template <> FlowSeqEndToken *Scanner::ScanToken(FlowSeqEndToken *pToken)
	{
		// TODO: "remove simple key"

		DecreaseFlowLevel();
		m_simpleKeyAllowed = false;

		// eat
		Eat(1);
		return pToken;
	}

	// FlowMapEndToken
	template <> FlowMapEndToken *Scanner::ScanToken(FlowMapEndToken *pToken)
	{
		// TODO: "remove simple key"

		DecreaseFlowLevel();
		m_simpleKeyAllowed = false;

		// eat
		Eat(1);
		return pToken;
	}

	// FlowEntryToken
	template <> FlowEntryToken *Scanner::ScanToken(FlowEntryToken *pToken)
	{
		// TODO: "remove simple key"

		m_simpleKeyAllowed = true;

		// eat
		Eat(1);
		return pToken;
	}

	// BlockEntryToken
	template <> BlockEntryToken *Scanner::ScanToken(BlockEntryToken *pToken)
	{
		// we better be in the block context!
		if(m_flowLevel == 0) {
			// can we put it here?
			if(!m_simpleKeyAllowed)
				throw IllegalBlockEntry();

			PushIndentTo(m_column, true);	// , -1
		} else {
			// TODO: throw?
		}

		// TODO: "remove simple key"

		m_simpleKeyAllowed = true;

		// eat
		Eat(1);
		return pToken;
	}

	// KeyToken
	template <> KeyToken *Scanner::ScanToken(KeyToken *pToken)
	{
		// are we in block context?
		if(m_flowLevel == 0) {
			if(!m_simpleKeyAllowed)
				throw IllegalMapKey();

			PushIndentTo(m_column, false);
		}

		// TODO: "remove simple key"

		// can only put a simple key here if we're in block context
		if(m_flowLevel == 0)
			m_simpleKeyAllowed = true;
		else
			m_simpleKeyAllowed = false;

		// eat
		Eat(1);
		return pToken;
	}

	// ValueToken
	template <> ValueToken *Scanner::ScanToken(ValueToken *pToken)
	{
		// TODO: Is it a simple key?
		if(false) {
		} else {
			// If not, ...
			// are we in block context?
			if(m_flowLevel == 0) {
				if(!m_simpleKeyAllowed)
					throw IllegalMapValue();

				PushIndentTo(m_column, false);
			}
		}

		// can only put a simple key here if we're in block context
		if(m_flowLevel == 0)
			m_simpleKeyAllowed = true;
		else
			m_simpleKeyAllowed = false;

		// eat
		Eat(1);
		return pToken;
	}

	// PlainScalarToken
	// . We scan these in passes of two steps each: First, grab all non-whitespace
	//   characters we can, and then grab all whitespace characters we can.
	// . This has the benefit of letting us handle leading whitespace (which is chomped)
	//   and in-line whitespace (which is kept) separately.
	template <> PlainScalarToken *Scanner::ScanToken(PlainScalarToken *pToken)
	{
		// TODO: "save simple key"

		m_simpleKeyAllowed = false;

		// now eat and store the scalar
		std::string scalar;
		WhitespaceInfo info;

		while(INPUT) {
			// doc start/end tokens
			if(IsDocumentStart() || IsDocumentEnd())
				break;

			// comment
			if(Exp::Comment.Matches(INPUT))
				break;

			// first eat non-blanks
			while(INPUT && !Exp::BlankOrBreak.Matches(INPUT)) {
				// illegal colon in flow context
				if(m_flowLevel > 0 && Exp::IllegalColonInScalar.Matches(INPUT))
					throw IllegalScalar();

				// characters that might end the scalar
				if(m_flowLevel > 0 && Exp::EndScalarInFlow.Matches(INPUT))
					break;
				if(m_flowLevel == 0 && Exp::EndScalar.Matches(INPUT))
					break;

				// finally, read the character!
				scalar += GetChar();
			}

			// did we hit a non-blank character that ended us?
			if(!Exp::BlankOrBreak.Matches(INPUT))
				break;

			// now eat blanks
			while(INPUT && Exp::BlankOrBreak.Matches(INPUT)) {
				if(Exp::Blank.Matches(INPUT)) {
					// can't use tabs as indentation! only spaces!
					if(INPUT.peek() == '\t' && info.leadingBlanks && m_column <= m_indents.top())
						throw IllegalTabInScalar();

					info.AddBlank(GetChar());
				} else	{
					// we know it's a line break; see how many characters to read
					int n = Exp::Break.Match(INPUT);
					std::string line = GetChar(n);
					info.AddBreak(line);
				}
			}

			// break if we're below the indentation level
			if(m_flowLevel == 0 && m_column <= m_indents.top())
				break;

			// finally join whitespace
			scalar += info.Join();
		}

		// now modify our token
		pToken->value = scalar;
		if(info.leadingBlanks)
			m_simpleKeyAllowed = true;

		return pToken;
	}

	// QuotedScalarToken
	template <> QuotedScalarToken *Scanner::ScanToken(QuotedScalarToken *pToken)
	{
		// TODO: "save simple key"

		m_simpleKeyAllowed = false;

		// eat single or double quote
		char quote = GetChar();
		bool single = (quote == '\'');

		// now eat and store the scalar
		std::string scalar;
		WhitespaceInfo info;

		while(INPUT) {
			if(IsDocumentStart() || IsDocumentEnd())
				throw DocIndicatorInQuote();

			if(INPUT.peek() == EOF)
				throw EOFInQuote();

			// first eat non-blanks
			while(INPUT && !Exp::BlankOrBreak.Matches(INPUT)) {
				// escaped single quote?
				if(single && Exp::EscSingleQuote.Matches(INPUT)) {
					int n = Exp::EscSingleQuote.Match(INPUT);
					scalar += GetChar(n);
					continue;
				}

				// is the quote ending?
				if(INPUT.peek() == (single ? '\'' : '\"'))
					break;

				// escaped newline?
				if(Exp::EscBreak.Matches(INPUT))
					break;

				// other escape sequence
				if(INPUT.peek() == '\\') {
					int length = 0;
					scalar += Exp::Escape(INPUT, length);
					m_column += length;
					continue;
				}

				// and finally, just add the damn character
				scalar += GetChar();
			}

			// is the quote ending?
			if(INPUT.peek() == (single ? '\'' : '\"')) {
				// eat and go
				GetChar();
				break;
			}

			// now we eat blanks
			while(Exp::BlankOrBreak.Matches(INPUT)) {
				if(Exp::Blank.Matches(INPUT)) {
					info.AddBlank(GetChar());
				} else {
					// we know it's a line break; see how many characters to read
					int n = Exp::Break.Match(INPUT);
					std::string line = GetChar(n);
					info.AddBreak(line);
				}
			}

			// and finally join the whitespace
			scalar += info.Join();
		}

		pToken->value = scalar;
		return pToken;
	}

	//////////////////////////////////////////////////////////
	// WhitespaceInfo stuff

	Scanner::WhitespaceInfo::WhitespaceInfo(): leadingBlanks(false)
	{
	}

	void Scanner::WhitespaceInfo::AddBlank(char ch)
	{
		if(!leadingBlanks)
			whitespace += ch;
	}

	void Scanner::WhitespaceInfo::AddBreak(const std::string& line)
	{
		// where to store this character?
		if(!leadingBlanks) {
			leadingBlanks = true;
			whitespace = "";
			leadingBreaks += line;
		} else
			trailingBreaks += line;
	}

	std::string Scanner::WhitespaceInfo::Join()
	{
		std::string ret;

		if(leadingBlanks) {
			if(Exp::Break.Matches(leadingBreaks)) {
				// fold line break?
				if(trailingBreaks.empty())
					ret = " ";
				else
					ret = trailingBreaks;
			} else {
				ret = leadingBreaks + trailingBreaks;
			}

			leadingBlanks = false;
			leadingBreaks = "";
			trailingBreaks = "";
		} else if(!whitespace.empty()) {
			ret = whitespace;
			whitespace = "";
		}

		return ret;
	}
}
