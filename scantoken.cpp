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
		if(INPUT.column > 0)
			INPUT.column = 0;

		PopIndentTo(-1);
		VerifyAllSimpleKeys();

		m_simpleKeyAllowed = false;
		m_endedStream = true;

		return pToken;
	}

	// DocumentStartToken
	template <> DocumentStartToken *Scanner::ScanToken(DocumentStartToken *pToken)
	{
		PopIndentTo(INPUT.column);
		VerifyAllSimpleKeys();
		m_simpleKeyAllowed = false;

		// eat
		INPUT.Eat(3);
		return pToken;
	}

	// DocumentEndToken
	template <> DocumentEndToken *Scanner::ScanToken(DocumentEndToken *pToken)
	{
		PopIndentTo(-1);
		VerifyAllSimpleKeys();
		m_simpleKeyAllowed = false;

		// eat
		INPUT.Eat(3);
		return pToken;
	}

	// FlowSeqStartToken
	template <> FlowSeqStartToken *Scanner::ScanToken(FlowSeqStartToken *pToken)
	{
		// flow sequences can be simple keys
		InsertSimpleKey();
		m_flowLevel++;
		m_simpleKeyAllowed = true;

		// eat
		INPUT.Eat(1);
		return pToken;
	}

	// FlowMapStartToken
	template <> FlowMapStartToken *Scanner::ScanToken(FlowMapStartToken *pToken)
	{
		// flow maps can be simple keys
		InsertSimpleKey();
		m_flowLevel++;
		m_simpleKeyAllowed = true;

		// eat
		INPUT.Eat(1);
		return pToken;
	}

	// FlowSeqEndToken
	template <> FlowSeqEndToken *Scanner::ScanToken(FlowSeqEndToken *pToken)
	{
		if(m_flowLevel == 0)
			throw IllegalFlowEnd();

		m_flowLevel--;
		m_simpleKeyAllowed = false;

		// eat
		INPUT.Eat(1);
		return pToken;
	}

	// FlowMapEndToken
	template <> FlowMapEndToken *Scanner::ScanToken(FlowMapEndToken *pToken)
	{
		if(m_flowLevel == 0)
			throw IllegalFlowEnd();

		m_flowLevel--;
		m_simpleKeyAllowed = false;

		// eat
		INPUT.Eat(1);
		return pToken;
	}

	// FlowEntryToken
	template <> FlowEntryToken *Scanner::ScanToken(FlowEntryToken *pToken)
	{
		m_simpleKeyAllowed = true;

		// eat
		INPUT.Eat(1);
		return pToken;
	}

	// BlockEntryToken
	template <> BlockEntryToken *Scanner::ScanToken(BlockEntryToken *pToken)
	{
		// we better be in the block context!
		if(m_flowLevel > 0)
			throw IllegalBlockEntry();

		// can we put it here?
		if(!m_simpleKeyAllowed)
			throw IllegalBlockEntry();

		PushIndentTo(INPUT.column, true);
		m_simpleKeyAllowed = true;

		// eat
		INPUT.Eat(1);
		return pToken;
	}

	// KeyToken
	template <> KeyToken *Scanner::ScanToken(KeyToken *pToken)
	{
		// handle keys diffently in the block context (and manage indents)
		if(m_flowLevel == 0) {
			if(!m_simpleKeyAllowed)
				throw IllegalMapKey();

			PushIndentTo(INPUT.column, false);
		}

		// can only put a simple key here if we're in block context
		if(m_flowLevel == 0)
			m_simpleKeyAllowed = true;
		else
			m_simpleKeyAllowed = false;

		// eat
		INPUT.Eat(1);
		return pToken;
	}

	// ValueToken
	template <> ValueToken *Scanner::ScanToken(ValueToken *pToken)
	{
		// does this follow a simple key?
		if(m_isLastKeyValid) {
			// can't follow a simple key with another simple key (dunno why, though - it seems fine)
			m_simpleKeyAllowed = false;
		} else {
			// handle values diffently in the block context (and manage indents)
			if(m_flowLevel == 0) {
				if(!m_simpleKeyAllowed)
					throw IllegalMapValue();

				PushIndentTo(INPUT.column, false);
			}

			// can only put a simple key here if we're in block context
			if(m_flowLevel == 0)
				m_simpleKeyAllowed = true;
			else
				m_simpleKeyAllowed = false;
		}

		// eat
		INPUT.Eat(1);
		return pToken;
	}

	// AnchorToken
	template <> AnchorToken *Scanner::ScanToken(AnchorToken *pToken)
	{
		// insert a potential simple key
		if(m_simpleKeyAllowed)
			InsertSimpleKey();
		m_simpleKeyAllowed = false;

		// eat the indicator
		char indicator = INPUT.GetChar();
		pToken->alias = (indicator == Keys::Alias);

		// now eat the content
		std::string tag;
		while(Exp::AlphaNumeric.Matches(INPUT))
			tag += INPUT.GetChar();

		// we need to have read SOMETHING!
		if(tag.empty())
			throw AnchorNotFound();

		// and needs to end correctly
		if(INPUT.peek() != EOF && !Exp::AnchorEnd.Matches(INPUT))
			throw IllegalCharacterInAnchor();

		// and we're done
		pToken->value = tag;
		return pToken;
	}
}
