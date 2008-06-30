#include "scanner.h"
#include "token.h"
#include "exceptions.h"
#include "exp.h"
#include "scanscalar.h"

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

	// DirectiveToken
	// . Note: no semantic checking is done here (that's for the parser to do)
	template <> DirectiveToken *Scanner::ScanToken(DirectiveToken *pToken)
	{
		// pop indents and simple keys
		PopIndentTo(-1);
		VerifyAllSimpleKeys();

		m_simpleKeyAllowed = false;

		// eat indicator
		INPUT.Eat(1);

		// read name
		while(INPUT.peek() != EOF && !Exp::BlankOrBreak.Matches(INPUT))
			pToken->name += INPUT.GetChar();

		// read parameters
		while(1) {
			// first get rid of whitespace
			while(Exp::Blank.Matches(INPUT))
				INPUT.Eat(1);

			// break on newline or comment
			if(INPUT.peek() == EOF || Exp::Break.Matches(INPUT) || Exp::Comment.Matches(INPUT))
				break;

			// now read parameter
			std::string param;
			while(INPUT.peek() != EOF && !Exp::BlankOrBreak.Matches(INPUT))
				param += INPUT.GetChar();

			pToken->params.push_back(param);
		}
		
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

	// TagToken
	template <> TagToken *Scanner::ScanToken(TagToken *pToken)
	{
		// insert a potential simple key
		if(m_simpleKeyAllowed)
			InsertSimpleKey();
		m_simpleKeyAllowed = false;

		// eat the indicator
		INPUT.Eat(1);

		// read the handle
		while(INPUT.peek() != EOF && INPUT.peek() != Keys::Tag && !Exp::BlankOrBreak.Matches(INPUT))
			pToken->handle += INPUT.GetChar();

		// is there a suffix?
		if(INPUT.peek() == Keys::Tag) {
			// eat the indicator
			INPUT.Eat(1);

			// then read it
			while(INPUT.peek() != EOF && !Exp::BlankOrBreak.Matches(INPUT))
				pToken->suffix += INPUT.GetChar();
		}

		return pToken;
	}

	// PlainScalarToken
	template <> PlainScalarToken *Scanner::ScanToken(PlainScalarToken *pToken)
	{
		// set up the scanning parameters
		ScanScalarParams params;
		params.end = (m_flowLevel > 0 ? Exp::EndScalarInFlow : Exp::EndScalar) || (RegEx(' ') + Exp::Comment);
		params.eatEnd = false;
		params.indent = (m_flowLevel > 0 ? 0 : m_indents.top() + 1);
		params.fold = true;
		params.eatLeadingWhitespace = true;
		params.trimTrailingSpaces = true;
		params.chomp = CLIP;
		params.onDocIndicator = BREAK;
		params.onTabInIndentation = THROW;

		// insert a potential simple key
		if(m_simpleKeyAllowed)
			InsertSimpleKey();

		pToken->value = ScanScalar(INPUT, params);

		// can have a simple key only if we ended the scalar by starting a new line
		m_simpleKeyAllowed = params.leadingSpaces;

		// finally, we can't have any colons in a scalar, so if we ended on a colon, there
		// had better be a break after it
		if(Exp::IllegalColonInScalar.Matches(INPUT))
			throw IllegalScalar();

		return pToken;
	}

	// QuotedScalarToken
	template <> QuotedScalarToken *Scanner::ScanToken(QuotedScalarToken *pToken)
	{
		// eat single or double quote
		char quote = INPUT.GetChar();
		pToken->single = (quote == '\'');

		// setup the scanning parameters
		ScanScalarParams params;
		params.end = (pToken->single ? RegEx(quote) && !Exp::EscSingleQuote : RegEx(quote));
		params.eatEnd = true;
		params.escape = (pToken->single ? '\'' : '\\');
		params.indent = 0;
		params.fold = true;
		params.eatLeadingWhitespace = true;
		params.trimTrailingSpaces = false;
		params.chomp = CLIP;
		params.onDocIndicator = THROW;

		// insert a potential simple key
		if(m_simpleKeyAllowed)
			InsertSimpleKey();

		pToken->value = ScanScalar(INPUT, params);
		m_simpleKeyAllowed = false;

		return pToken;
	}

	// BlockScalarToken
	// . These need a little extra processing beforehand.
	// . We need to scan the line where the indicator is (this doesn't count as part of the scalar),
	//   and then we need to figure out what level of indentation we'll be using.
	template <> BlockScalarToken *Scanner::ScanToken(BlockScalarToken *pToken)
	{
		ScanScalarParams params;
		params.indent = 1;
		params.detectIndent = true;

		// eat block indicator ('|' or '>')
		char indicator = INPUT.GetChar();
		params.fold = (indicator == Keys::FoldedScalar);

		// eat chomping/indentation indicators
		int n = Exp::Chomp.Match(INPUT);
		for(int i=0;i<n;i++) {
			char ch = INPUT.GetChar();
			if(ch == '+')
				params.chomp = KEEP;
			else if(ch == '-')
				params.chomp = STRIP;
			else if(Exp::Digit.Matches(ch)) {
				if(ch == '0')
					throw ZeroIndentationInBlockScalar();

				params.indent = ch - '0';
				params.detectIndent = false;
			}
		}

		// now eat whitespace
		while(Exp::Blank.Matches(INPUT))
			INPUT.Eat(1);

		// and comments to the end of the line
		if(Exp::Comment.Matches(INPUT))
			while(INPUT && !Exp::Break.Matches(INPUT))
				INPUT.Eat(1);

		// if it's not a line break, then we ran into a bad character inline
		if(INPUT && !Exp::Break.Matches(INPUT))
			throw UnexpectedCharacterInBlockScalar();

		// set the initial indentation
		if(m_indents.top() >= 0)
			params.indent += m_indents.top();

		params.eatLeadingWhitespace = false;
		params.trimTrailingSpaces = false;
		params.onTabInIndentation = THROW;

		pToken->value = ScanScalar(INPUT, params);

		// simple keys always ok after block scalars (since we're gonna start a new line anyways)
		m_simpleKeyAllowed = true;
		return pToken;
	}
}
