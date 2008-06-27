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
		// TODO: increase flow level

		m_simpleKeyAllowed = true;

		// eat
		Eat(1);
		return pToken;
	}

	// FlowMapStartToken
	template <> FlowMapStartToken *Scanner::ScanToken(FlowMapStartToken *pToken)
	{
		// TODO: "save simple key"
		// TODO: increase flow level

		m_simpleKeyAllowed = true;

		// eat
		Eat(1);
		return pToken;
	}

	// FlowSeqEndToken
	template <> FlowSeqEndToken *Scanner::ScanToken(FlowSeqEndToken *pToken)
	{
		// TODO: "remove simple key"
		// TODO: decrease flow level

		m_simpleKeyAllowed = false;

		// eat
		Eat(1);
		return pToken;
	}

	// FlowMapEndToken
	template <> FlowMapEndToken *Scanner::ScanToken(FlowMapEndToken *pToken)
	{
		// TODO: "remove simple key"
		// TODO: decrease flow level

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
	template <> PlainScalarToken *Scanner::ScanToken(PlainScalarToken *pToken)
	{
		// TODO: "save simple key"

		m_simpleKeyAllowed = false;

		// now eat and store the scalar
		std::string scalar, whitespace, leadingBreaks, trailingBreaks;
		bool leadingBlanks = false;

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

				if(leadingBlanks) {
					if(!leadingBreaks.empty() && leadingBreaks[0] == '\n') {
						// fold line break?
						if(trailingBreaks.empty())
							scalar += ' ';
						else {
							scalar += trailingBreaks;
							trailingBreaks = "";
						}
					} else {
						scalar += leadingBreaks + trailingBreaks;
						leadingBreaks = "";
						trailingBreaks = "";
					}
				} else if(!whitespace.empty()) {
					scalar += whitespace;
					whitespace = "";
				}

				// finally, read the character!
				scalar += GetChar();
			}

			// did we hit a non-blank character that ended us?
			if(!Exp::BlankOrBreak.Matches(INPUT))
				break;

			// now eat blanks
			while(INPUT && Exp::BlankOrBreak.Matches(INPUT)) {
				if(Exp::Blank.Matches(INPUT)) {
					if(leadingBlanks && m_column <= m_indents.top())
						throw IllegalTabInScalar();

					// maybe store this character
					if(!leadingBlanks)
						whitespace += GetChar();
					else
						Eat(1);
				} else {
					// where to store this character?
					if(!leadingBlanks) {
						leadingBlanks = true;
						whitespace = "";
						leadingBreaks += GetChar();
					} else
						trailingBreaks += GetChar();
				}
			}

			// and finally break if we're below the indentation level
			if(m_flowLevel == 0 && m_column <= m_indents.top())
				break;
		}

		// now modify our token
		pToken->SetValue(scalar);
		if(leadingBlanks)
			m_simpleKeyAllowed = true;

		return pToken;
	}
}
