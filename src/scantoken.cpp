#include "crt.h"
#include "scanner.h"
#include "token.h"
#include "exceptions.h"
#include "exp.h"
#include "scanscalar.h"
#include <sstream>

namespace YAML
{
	///////////////////////////////////////////////////////////////////////
	// Specialization for scanning specific tokens

	// Directive
	// . Note: no semantic checking is done here (that's for the parser to do)
	void Scanner::ScanDirective()
	{
		std::string name;
		std::vector <std::string> params;

		// pop indents and simple keys
		PopAllIndents();
		VerifyAllSimpleKeys();

		m_simpleKeyAllowed = false;

		// store pos and eat indicator
		Mark mark = INPUT.mark();
		INPUT.eat(1);

		// read name
		while(INPUT && !Exp::BlankOrBreak.Matches(INPUT))
			name += INPUT.get();

		// read parameters
		while(1) {
			// first get rid of whitespace
			while(Exp::Blank.Matches(INPUT))
				INPUT.eat(1);

			// break on newline or comment
			if(!INPUT || Exp::Break.Matches(INPUT) || Exp::Comment.Matches(INPUT))
				break;

			// now read parameter
			std::string param;
			while(INPUT && !Exp::BlankOrBreak.Matches(INPUT))
				param += INPUT.get();

			params.push_back(param);
		}
		
		Token token(TT_DIRECTIVE, mark);
		token.value = name;
		token.params = params;
		m_tokens.push(token);
	}

	// DocStart
	void Scanner::ScanDocStart()
	{
		PopAllIndents();
		VerifyAllSimpleKeys();
		m_simpleKeyAllowed = false;

		// eat
		Mark mark = INPUT.mark();
		INPUT.eat(3);
		m_tokens.push(Token(TT_DOC_START, mark));
	}

	// DocEnd
	void Scanner::ScanDocEnd()
	{
		PopAllIndents();
		VerifyAllSimpleKeys();
		m_simpleKeyAllowed = false;

		// eat
		Mark mark = INPUT.mark();
		INPUT.eat(3);
		m_tokens.push(Token(TT_DOC_END, mark));
	}

	// FlowStart
	void Scanner::ScanFlowStart()
	{
		// flows can be simple keys
		InsertSimpleKey();
		m_flowLevel++;
		m_simpleKeyAllowed = true;

		// eat
		Mark mark = INPUT.mark();
		char ch = INPUT.get();
		TOKEN_TYPE type = (ch == Keys::FlowSeqStart ? TT_FLOW_SEQ_START : TT_FLOW_MAP_START);
		m_tokens.push(Token(type, mark));
	}

	// FlowEnd
	void Scanner::ScanFlowEnd()
	{
		if(m_flowLevel == 0)
			throw ParserException(INPUT.mark(), ErrorMsg::FLOW_END);

		m_flowLevel--;
		m_simpleKeyAllowed = false;

		// eat
		Mark mark = INPUT.mark();
		char ch = INPUT.get();
		TOKEN_TYPE type = (ch == Keys::FlowSeqEnd ? TT_FLOW_SEQ_END : TT_FLOW_MAP_END);
		m_tokens.push(Token(type, mark));
	}

	// FlowEntry
	void Scanner::ScanFlowEntry()
	{
		m_simpleKeyAllowed = true;

		// eat
		Mark mark = INPUT.mark();
		INPUT.eat(1);
		m_tokens.push(Token(TT_FLOW_ENTRY, mark));
	}

	// BlockEntry
	void Scanner::ScanBlockEntry()
	{
		// we better be in the block context!
		if(m_flowLevel > 0)
			throw ParserException(INPUT.mark(), ErrorMsg::BLOCK_ENTRY);

		// can we put it here?
		if(!m_simpleKeyAllowed)
			throw ParserException(INPUT.mark(), ErrorMsg::BLOCK_ENTRY);

		PushIndentTo(INPUT.column(), IndentMarker::SEQ);
		m_simpleKeyAllowed = true;

		// eat
		Mark mark = INPUT.mark();
		INPUT.eat(1);
		m_tokens.push(Token(TT_BLOCK_ENTRY, mark));
	}

	// Key
	void Scanner::ScanKey()
	{
		// handle keys diffently in the block context (and manage indents)
		if(m_flowLevel == 0) {
			if(!m_simpleKeyAllowed)
				throw ParserException(INPUT.mark(), ErrorMsg::MAP_KEY);

			PushIndentTo(INPUT.column(), IndentMarker::MAP);
		}

		// can only put a simple key here if we're in block context
		if(m_flowLevel == 0)
			m_simpleKeyAllowed = true;
		else
			m_simpleKeyAllowed = false;

		// eat
		Mark mark = INPUT.mark();
		INPUT.eat(1);
		m_tokens.push(Token(TT_KEY, mark));
	}

	// Value
	void Scanner::ScanValue()
	{
		// does this follow a simple key?
		if(m_isLastKeyValid) {
			// can't follow a simple key with another simple key (dunno why, though - it seems fine)
			m_simpleKeyAllowed = false;
		} else {
			// handle values diffently in the block context (and manage indents)
			if(m_flowLevel == 0) {
				if(!m_simpleKeyAllowed)
					throw ParserException(INPUT.mark(), ErrorMsg::MAP_VALUE);

				PushIndentTo(INPUT.column(), IndentMarker::MAP);
			}

			// can only put a simple key here if we're in block context
			if(m_flowLevel == 0)
				m_simpleKeyAllowed = true;
			else
				m_simpleKeyAllowed = false;
		}

		// eat
		Mark mark = INPUT.mark();
		INPUT.eat(1);
		m_tokens.push(Token(TT_VALUE, mark));
	}

	// AnchorOrAlias
	void Scanner::ScanAnchorOrAlias()
	{
		bool alias;
		std::string name;

		// insert a potential simple key
		if(m_simpleKeyAllowed)
			InsertSimpleKey();
		m_simpleKeyAllowed = false;

		// eat the indicator
		Mark mark = INPUT.mark();
		char indicator = INPUT.get();
		alias = (indicator == Keys::Alias);

		// now eat the content
		while(Exp::AlphaNumeric.Matches(INPUT))
			name += INPUT.get();

		// we need to have read SOMETHING!
		if(name.empty())
			throw ParserException(INPUT.mark(), alias ? ErrorMsg::ALIAS_NOT_FOUND : ErrorMsg::ANCHOR_NOT_FOUND);

		// and needs to end correctly
		if(INPUT && !Exp::AnchorEnd.Matches(INPUT))
			throw ParserException(INPUT.mark(), alias ? ErrorMsg::CHAR_IN_ALIAS : ErrorMsg::CHAR_IN_ANCHOR);

		// and we're done
		Token token(alias ? TT_ALIAS : TT_ANCHOR, mark);
		token.value = name;
		m_tokens.push(token);
	}

	// Tag
	void Scanner::ScanTag()
	{
		std::string handle, suffix;

		// insert a potential simple key
		if(m_simpleKeyAllowed)
			InsertSimpleKey();
		m_simpleKeyAllowed = false;

		// eat the indicator
		Mark mark = INPUT.mark();
		handle += INPUT.get();

		// read the handle
		while(INPUT && INPUT.peek() != Keys::Tag && !Exp::BlankOrBreak.Matches(INPUT))
			handle += INPUT.get();

		// is there a suffix?
		if(INPUT.peek() == Keys::Tag) {
			// eat the indicator
			handle += INPUT.get();

			// then read it
			while(INPUT && !Exp::BlankOrBreak.Matches(INPUT))
				suffix += INPUT.get();
		} else {
			// this is a bit weird: we keep just the '!' as the handle and move the rest to the suffix
			suffix = handle.substr(1);
			handle = "!";
		}

		Token token(TT_TAG, mark);
		token.value = handle;
		token.params.push_back(suffix);
		m_tokens.push(token);
	}

	// PlainScalar
	void Scanner::ScanPlainScalar()
	{
		std::string scalar;

		// set up the scanning parameters
		ScanScalarParams params;
		params.end = (m_flowLevel > 0 ? Exp::EndScalarInFlow : Exp::EndScalar) || (Exp::BlankOrBreak + Exp::Comment);
		params.eatEnd = false;
		params.indent = (m_flowLevel > 0 ? 0 : GetTopIndent() + 1);
		params.fold = true;
		params.eatLeadingWhitespace = true;
		params.trimTrailingSpaces = true;
		params.chomp = CLIP;
		params.onDocIndicator = BREAK;
		params.onTabInIndentation = THROW;

		// insert a potential simple key
		if(m_simpleKeyAllowed)
			InsertSimpleKey();

		Mark mark = INPUT.mark();
		scalar = ScanScalar(INPUT, params);

		// can have a simple key only if we ended the scalar by starting a new line
		m_simpleKeyAllowed = params.leadingSpaces;

		// finally, check and see if we ended on an illegal character
		//if(Exp::IllegalCharInScalar.Matches(INPUT))
		//	throw ParserException(INPUT.mark(), ErrorMsg::CHAR_IN_SCALAR);

		Token token(TT_SCALAR, mark);
		token.value = scalar;
		m_tokens.push(token);
	}

	// QuotedScalar
	void Scanner::ScanQuotedScalar()
	{
		std::string scalar;

		// peek at single or double quote (don't eat because we need to preserve (for the time being) the input position)
		char quote = INPUT.peek();
		bool single = (quote == '\'');

		// setup the scanning parameters
		ScanScalarParams params;
		params.end = (single ? RegEx(quote) && !Exp::EscSingleQuote : RegEx(quote));
		params.eatEnd = true;
		params.escape = (single ? '\'' : '\\');
		params.indent = 0;
		params.fold = true;
		params.eatLeadingWhitespace = true;
		params.trimTrailingSpaces = false;
		params.chomp = CLIP;
		params.onDocIndicator = THROW;

		// insert a potential simple key
		if(m_simpleKeyAllowed)
			InsertSimpleKey();

		Mark mark = INPUT.mark();

		// now eat that opening quote
		INPUT.get();
		
		// and scan
		scalar = ScanScalar(INPUT, params);
		m_simpleKeyAllowed = false;

		Token token(TT_SCALAR, mark);
		token.value = scalar;
		m_tokens.push(token);
	}

	// BlockScalarToken
	// . These need a little extra processing beforehand.
	// . We need to scan the line where the indicator is (this doesn't count as part of the scalar),
	//   and then we need to figure out what level of indentation we'll be using.
	void Scanner::ScanBlockScalar()
	{
		std::string scalar;

		ScanScalarParams params;
		params.indent = 1;
		params.detectIndent = true;

		// eat block indicator ('|' or '>')
		Mark mark = INPUT.mark();
		char indicator = INPUT.get();
		params.fold = (indicator == Keys::FoldedScalar);

		// eat chomping/indentation indicators
		int n = Exp::Chomp.Match(INPUT);
		for(int i=0;i<n;i++) {
			char ch = INPUT.get();
			if(ch == '+')
				params.chomp = KEEP;
			else if(ch == '-')
				params.chomp = STRIP;
			else if(Exp::Digit.Matches(ch)) {
				if(ch == '0')
					throw ParserException(INPUT.mark(), ErrorMsg::ZERO_INDENT_IN_BLOCK);

				params.indent = ch - '0';
				params.detectIndent = false;
			}
		}

		// now eat whitespace
		while(Exp::Blank.Matches(INPUT))
			INPUT.eat(1);

		// and comments to the end of the line
		if(Exp::Comment.Matches(INPUT))
			while(INPUT && !Exp::Break.Matches(INPUT))
				INPUT.eat(1);

		// if it's not a line break, then we ran into a bad character inline
		if(INPUT && !Exp::Break.Matches(INPUT))
			throw ParserException(INPUT.mark(), ErrorMsg::CHAR_IN_BLOCK);

		// set the initial indentation
		if(GetTopIndent() >= 0)
			params.indent += GetTopIndent();

		params.eatLeadingWhitespace = false;
		params.trimTrailingSpaces = false;
		params.onTabInIndentation = THROW;

		scalar = ScanScalar(INPUT, params);

		// simple keys always ok after block scalars (since we're gonna start a new line anyways)
		m_simpleKeyAllowed = true;

		Token token(TT_SCALAR, mark);
		token.value = scalar;
		m_tokens.push(token);
	}
}
