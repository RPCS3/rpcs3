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
		PopIndentTo(-1);
		VerifyAllSimpleKeys();

		m_simpleKeyAllowed = false;

		// store pos and eat indicator
		int line = INPUT.line, column = INPUT.column;
		INPUT.eat(1);

		// read name
		while(INPUT.peek() != EOF && !Exp::BlankOrBreak.Matches(INPUT))
			name += INPUT.get();

		// read parameters
		while(1) {
			// first get rid of whitespace
			while(Exp::Blank.Matches(INPUT))
				INPUT.eat(1);

			// break on newline or comment
			if(INPUT.peek() == EOF || Exp::Break.Matches(INPUT) || Exp::Comment.Matches(INPUT))
				break;

			// now read parameter
			std::string param;
			while(INPUT.peek() != EOF && !Exp::BlankOrBreak.Matches(INPUT))
				param += INPUT.get();

			params.push_back(param);
		}
		
		Token *pToken = new Token(TT_DIRECTIVE, line, column);
		pToken->value = name;
		pToken->params = params;
		m_tokens.push(pToken);
	}

	// DocStart
	void Scanner::ScanDocStart()
	{
		PopIndentTo(INPUT.column);
		VerifyAllSimpleKeys();
		m_simpleKeyAllowed = false;

		// eat
		int line = INPUT.line, column = INPUT.column;
		INPUT.eat(3);
		m_tokens.push(new Token(TT_DOC_START, line, column));
	}

	// DocEnd
	void Scanner::ScanDocEnd()
	{
		PopIndentTo(-1);
		VerifyAllSimpleKeys();
		m_simpleKeyAllowed = false;

		// eat
		int line = INPUT.line, column = INPUT.column;
		INPUT.eat(3);
		m_tokens.push(new Token(TT_DOC_END, line, column));
	}

	// FlowStart
	void Scanner::ScanFlowStart()
	{
		// flows can be simple keys
		InsertSimpleKey();
		m_flowLevel++;
		m_simpleKeyAllowed = true;

		// eat
		int line = INPUT.line, column = INPUT.column;
		char ch = INPUT.get();
		TOKEN_TYPE type = (ch == Keys::FlowSeqStart ? TT_FLOW_SEQ_START : TT_FLOW_MAP_START);
		m_tokens.push(new Token(type, line, column));
	}

	// FlowEnd
	void Scanner::ScanFlowEnd()
	{
		if(m_flowLevel == 0)
			throw ParserException(INPUT.line, INPUT.column, "illegal flow end");

		m_flowLevel--;
		m_simpleKeyAllowed = false;

		// eat
		int line = INPUT.line, column = INPUT.column;
		char ch = INPUT.get();
		TOKEN_TYPE type = (ch == Keys::FlowSeqEnd ? TT_FLOW_SEQ_END : TT_FLOW_MAP_END);
		m_tokens.push(new Token(type, line, column));
	}

	// FlowEntry
	void Scanner::ScanFlowEntry()
	{
		m_simpleKeyAllowed = true;

		// eat
		int line = INPUT.line, column = INPUT.column;
		INPUT.eat(1);
		m_tokens.push(new Token(TT_FLOW_ENTRY, line, column));
	}

	// BlockEntry
	void Scanner::ScanBlockEntry()
	{
		// we better be in the block context!
		if(m_flowLevel > 0)
			throw ParserException(INPUT.line, INPUT.column, "illegal block entry");

		// can we put it here?
		if(!m_simpleKeyAllowed)
			throw ParserException(INPUT.line, INPUT.column, "illegal block entry");

		PushIndentTo(INPUT.column, true);
		m_simpleKeyAllowed = true;

		// eat
		int line = INPUT.line, column = INPUT.column;
		INPUT.eat(1);
		m_tokens.push(new Token(TT_BLOCK_ENTRY, line, column));
	}

	// Key
	void Scanner::ScanKey()
	{
		// handle keys diffently in the block context (and manage indents)
		if(m_flowLevel == 0) {
			if(!m_simpleKeyAllowed)
				throw ParserException(INPUT.line, INPUT.column, "illegal map key");

			PushIndentTo(INPUT.column, false);
		}

		// can only put a simple key here if we're in block context
		if(m_flowLevel == 0)
			m_simpleKeyAllowed = true;
		else
			m_simpleKeyAllowed = false;

		// eat
		int line = INPUT.line, column = INPUT.column;
		INPUT.eat(1);
		m_tokens.push(new Token(TT_KEY, line, column));
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
					throw ParserException(INPUT.line, INPUT.column, "illegal map value");

				PushIndentTo(INPUT.column, false);
			}

			// can only put a simple key here if we're in block context
			if(m_flowLevel == 0)
				m_simpleKeyAllowed = true;
			else
				m_simpleKeyAllowed = false;
		}

		// eat
		int line = INPUT.line, column = INPUT.column;
		INPUT.eat(1);
		m_tokens.push(new Token(TT_VALUE, line, column));
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
		int line = INPUT.line, column = INPUT.column;
		char indicator = INPUT.get();
		alias = (indicator == Keys::Alias);

		// now eat the content
		while(Exp::AlphaNumeric.Matches(INPUT))
			name += INPUT.get();

		// we need to have read SOMETHING!
		if(name.empty()) {
			std::stringstream msg;
			msg << (alias ? "alias" : "anchor");
			msg << " not found after ";
			msg << (alias ? "*" : "&");
			throw ParserException(INPUT.line, INPUT.column, msg.str());
		}

		// and needs to end correctly
		if(INPUT.peek() != EOF && !Exp::AnchorEnd.Matches(INPUT)) {
			std::stringstream msg;
			msg << "illegal character found while scanning ";
			msg << (alias ? "alias" : "anchor");
			throw ParserException(INPUT.line, INPUT.column, msg.str());
		}

		// and we're done
		Token *pToken = new Token(alias ? TT_ALIAS : TT_ANCHOR, line, column);
		pToken->value = name;
		m_tokens.push(pToken);
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
		int line = INPUT.line, column = INPUT.column;
		handle += INPUT.get();

		// read the handle
		while(INPUT.peek() != EOF && INPUT.peek() != Keys::Tag && !Exp::BlankOrBreak.Matches(INPUT))
			handle += INPUT.get();

		// is there a suffix?
		if(INPUT.peek() == Keys::Tag) {
			// eat the indicator
			handle += INPUT.get();

			// then read it
			while(INPUT.peek() != EOF && !Exp::BlankOrBreak.Matches(INPUT))
				suffix += INPUT.get();
		} else {
			// this is a bit weird: we keep just the '!' as the handle and move the rest to the suffix
			suffix = handle.substr(1);
			handle = "!";
		}

		Token *pToken = new Token(TT_TAG, line, column);
		pToken->value = handle;
		pToken->params.push_back(suffix);
		m_tokens.push(pToken);
	}

	// PlainScalar
	void Scanner::ScanPlainScalar()
	{
		std::string scalar;

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

		int line = INPUT.line, column = INPUT.column;
		scalar = ScanScalar(INPUT, params);

		// can have a simple key only if we ended the scalar by starting a new line
		m_simpleKeyAllowed = params.leadingSpaces;

		// finally, we can't have any colons in a scalar, so if we ended on a colon, there
		// had better be a break after it
		if(Exp::IllegalColonInScalar.Matches(INPUT))
			throw ParserException(INPUT.line, INPUT.column, "illegal character in scalar");

		Token *pToken = new Token(TT_SCALAR, line, column);
		pToken->value = scalar;
		m_tokens.push(pToken);
	}

	// QuotedScalar
	void Scanner::ScanQuotedScalar()
	{
		std::string scalar;

		// eat single or double quote
		char quote = INPUT.get();
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

		int line = INPUT.line, column = INPUT.column;
		scalar = ScanScalar(INPUT, params);
		m_simpleKeyAllowed = false;

		Token *pToken = new Token(TT_SCALAR, line, column);
		pToken->value = scalar;
		m_tokens.push(pToken);
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
		int line = INPUT.line, column = INPUT.column;
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
					throw ParserException(INPUT.line, INPUT.column, "cannot set zero indentation for a block scalar");

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
			throw ParserException(INPUT.line, INPUT.column, "unexpected character in block scalar");

		// set the initial indentation
		if(m_indents.top() >= 0)
			params.indent += m_indents.top();

		params.eatLeadingWhitespace = false;
		params.trimTrailingSpaces = false;
		params.onTabInIndentation = THROW;

		scalar = ScanScalar(INPUT, params);

		// simple keys always ok after block scalars (since we're gonna start a new line anyways)
		m_simpleKeyAllowed = true;

		Token *pToken = new Token(TT_SCALAR, line, column);
		pToken->value = scalar;
		m_tokens.push(pToken);
	}
}
