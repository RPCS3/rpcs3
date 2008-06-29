#include "scanscalar.h"
#include "scanner.h"
#include "exp.h"
#include "exceptions.h"
#include "token.h"

namespace YAML
{
	//////////////////////////////////////////////////////////
	// WhitespaceInfo

	WhitespaceInfo::WhitespaceInfo(): leadingBlanks(false), fold(true), chomp(0), increment(0)
	{
	}

	void WhitespaceInfo::SetChompers(char ch)
	{
		if(ch == '+')
			chomp = 1;
		else if(ch == '-')
			chomp = -1;
		else if(Exp::Digit.Matches(ch)) {
			increment = ch - '0';
			if(increment == 0)
				throw ZeroIndentationInBlockScalar();
		}
	}

	void WhitespaceInfo::AddBlank(char ch)
	{
		if(!leadingBlanks)
			whitespace += ch;
	}

	void WhitespaceInfo::AddBreak(const std::string& line)
	{
		// where to store this character?
		if(!leadingBlanks) {
			leadingBlanks = true;
			whitespace = "";
			leadingBreaks += line;
		} else
			trailingBreaks += line;
	}

	std::string WhitespaceInfo::Join(bool lastLine)
	{
		std::string ret;

		if(leadingBlanks) {
			// fold line break?
			if(fold && Exp::Break.Matches(leadingBreaks) && trailingBreaks.empty() && !lastLine)
				ret = " ";
			else if(!lastLine || chomp != -1)
				ret = leadingBreaks;

			if(!lastLine || chomp == 1)
				ret += trailingBreaks;

			leadingBlanks = false;
			leadingBreaks = "";
			trailingBreaks = "";
		} else if(!whitespace.empty()) {
			ret = whitespace;
			whitespace = "";
		}

		return ret;
	}

	// PlainScalarToken
	// . We scan these in passes of two steps each: First, grab all non-whitespace
	//   characters we can, and then grab all whitespace characters we can.
	// . This has the benefit of letting us handle leading whitespace (which is chomped)
	//   and in-line whitespace (which is kept) separately.
	template <> PlainScalarToken *Scanner::ScanToken(PlainScalarToken *pToken)
	{
		//// now eat and store the scalar
		//std::string scalar;
		//WhitespaceInfo info;

		//while(INPUT) {
		//	// doc start/end tokens
		//	if(IsDocumentStart() || IsDocumentEnd())
		//		break;

		//	// comment
		//	if(Exp::Comment.Matches(INPUT))
		//		break;

		//	// first eat non-blanks
		//	while(INPUT && !Exp::BlankOrBreak.Matches(INPUT)) {
		//		// illegal colon in flow context
		//		if(m_flowLevel > 0 && Exp::IllegalColonInScalar.Matches(INPUT))
		//			throw IllegalScalar();

		//		// characters that might end the scalar
		//		if(m_flowLevel > 0 && Exp::EndScalarInFlow.Matches(INPUT))
		//			break;
		//		if(m_flowLevel == 0 && Exp::EndScalar.Matches(INPUT))
		//			break;

		//		// finally, read the character!
		//		scalar += GetChar();
		//	}

		//	// did we hit a non-blank character that ended us?
		//	if(!Exp::BlankOrBreak.Matches(INPUT))
		//		break;

		//	// now eat blanks
		//	while(INPUT && Exp::BlankOrBreak.Matches(INPUT)) {
		//		if(Exp::Blank.Matches(INPUT)) {
		//			// can't use tabs as indentation! only spaces!
		//			if(INPUT.peek() == '\t' && info.leadingBlanks && m_column <= m_indents.top())
		//				throw IllegalTabInScalar();

		//			info.AddBlank(GetChar());
		//		} else	{
		//			// we know it's a line break; see how many characters to read
		//			int n = Exp::Break.Match(INPUT);
		//			std::string line = GetChar(n);
		//			info.AddBreak(line);

		//			// and we can't continue a simple key to the next line
		//			ValidateSimpleKey();
		//		}
		//	}

		//	// break if we're below the indentation level
		//	if(m_flowLevel == 0 && m_column <= m_indents.top())
		//		break;

		//	// finally join whitespace
		//	scalar += info.Join();
		//}

		ScanScalarInfo info;
		info.end = (m_flowLevel > 0 ? Exp::EndScalarInFlow : Exp::EndScalar) || (RegEx(' ') + Exp::Comment);
		info.eatEnd = false;
		info.indent = (m_flowLevel > 0 ? 0 : m_indents.top() + 1);
		info.fold = true;
		info.eatLeadingWhitespace = true;
		info.trimTrailingSpaces = true;
		info.chomp = CLIP;

		// insert a potential simple key
		if(m_simpleKeyAllowed)
			InsertSimpleKey();

		pToken->value = ScanScalar(INPUT, info);

		m_simpleKeyAllowed = false;
		if(true/*info.leadingBlanks*/)
			m_simpleKeyAllowed = true;

		return pToken;
	}

	// QuotedScalarToken
	template <> QuotedScalarToken *Scanner::ScanToken(QuotedScalarToken *pToken)
	{
		//// now eat and store the scalar
		//std::string scalar;
		//WhitespaceInfo info;

		//while(INPUT) {
		//	if(IsDocumentStart() || IsDocumentEnd())
		//		throw DocIndicatorInQuote();

		//	if(INPUT.peek() == EOF)
		//		throw EOFInQuote();

		//	// first eat non-blanks
		//	while(INPUT && !Exp::BlankOrBreak.Matches(INPUT)) {
		//		// escaped single quote?
		//		if(pToken->single && Exp::EscSingleQuote.Matches(INPUT)) {
		//			int n = Exp::EscSingleQuote.Match(INPUT);
		//			scalar += GetChar(n);
		//			continue;
		//		}

		//		// is the quote ending?
		//		if(INPUT.peek() == quote)
		//			break;

		//		// escaped newline?
		//		if(Exp::EscBreak.Matches(INPUT))
		//			break;

		//		// other escape sequence
		//		if(INPUT.peek() == '\\') {
		//			int length = 0;
		//			scalar += Exp::Escape(INPUT, length);
		//			m_column += length;
		//			continue;
		//		}

		//		// and finally, just add the damn character
		//		scalar += GetChar();
		//	}

		//	// is the quote ending?
		//	if(INPUT.peek() == quote) {
		//		// eat and go
		//		GetChar();
		//		break;
		//	}

		//	// now we eat blanks
		//	while(Exp::BlankOrBreak.Matches(INPUT)) {
		//		if(Exp::Blank.Matches(INPUT)) {
		//			info.AddBlank(GetChar());
		//		} else {
		//			// we know it's a line break; see how many characters to read
		//			int n = Exp::Break.Match(INPUT);
		//			std::string line = GetChar(n);
		//			info.AddBreak(line);

		//			// and we can't continue a simple key to the next line
		//			ValidateSimpleKey();
		//		}
		//	}

		//	// and finally join the whitespace
		//	scalar += info.Join();
		//}

		// eat single or double quote
		char quote = INPUT.GetChar();
		pToken->single = (quote == '\'');

		ScanScalarInfo info;
		info.end = (pToken->single ? RegEx(quote) && !Exp::EscSingleQuote : RegEx(quote));
		info.eatEnd = true;
		info.escape = (pToken->single ? '\'' : '\\');
		info.indent = 0;
		info.fold = true;
		info.eatLeadingWhitespace = true;
		info.trimTrailingSpaces = false;
		info.chomp = CLIP;

		// insert a potential simple key
		if(m_simpleKeyAllowed)
			InsertSimpleKey();

		pToken->value = ScanScalar(INPUT, info);
		m_simpleKeyAllowed = false;

		return pToken;
	}

	// BlockScalarToken
	template <> BlockScalarToken *Scanner::ScanToken(BlockScalarToken *pToken)
	{
		WhitespaceInfo info;

		// eat block indicator ('|' or '>')
		char indicator = INPUT.GetChar();
		info.fold = (indicator == Keys::FoldedScalar);

		// eat chomping/indentation indicators
		int n = Exp::Chomp.Match(INPUT);
		for(int i=0;i<n;i++)
			info.SetChompers(INPUT.GetChar());

		// first eat whitespace
		while(Exp::Blank.Matches(INPUT))
			INPUT.Eat(1);

		// and comments to the end of the line
		if(Exp::Comment.Matches(INPUT))
			while(INPUT && !Exp::Break.Matches(INPUT))
				INPUT.Eat(1);

		// if it's not a line break, then we ran into a bad character inline
		if(INPUT && !Exp::Break.Matches(INPUT))
			throw UnexpectedCharacterInBlockScalar();

		// and eat that baby
		INPUT.EatLineBreak();

		// set the initial indentation
		int indent = info.increment;
		if(info.increment && m_indents.top() >= 0)
			indent += m_indents.top();

		GetBlockIndentation(INPUT, indent, info.trailingBreaks, m_indents.top());

		ScanScalarInfo sinfo;
		sinfo.indent = indent;
		sinfo.fold = info.fold;
		sinfo.eatLeadingWhitespace = false;
		sinfo.trimTrailingSpaces = false;
		sinfo.chomp = (CHOMP) info.chomp;

		pToken->value = ScanScalar(INPUT, sinfo);

		// simple keys always ok after block scalars (since we're gonna start a new line anyways)
		m_simpleKeyAllowed = true;
		return pToken;
	}

	// GetBlockIndentation
	// . Helper to scanning a block scalar.
	// . Eats leading *indentation* zeros (i.e., those that come before 'indent'),
	//   and updates 'indent' (if it hasn't been set yet).
	void GetBlockIndentation(Stream& INPUT, int& indent, std::string& breaks, int topIndent)
	{
		int maxIndent = 0;

		while(1) {
			// eat as many indentation spaces as we can
			while((indent == 0 || INPUT.column < indent) && INPUT.peek() == ' ')
				INPUT.Eat(1);

			if(INPUT.column > maxIndent)
				maxIndent = INPUT.column;

			// do we need more indentation, but we've got a tab?
			if((indent == 0 || INPUT.column < indent) && INPUT.peek() == '\t')
				throw IllegalTabInScalar();   // TODO: are literal scalar lines allowed to have tabs here?

			// is this a non-empty line?
			if(!Exp::Break.Matches(INPUT))
				break;

			// otherwise, eat the line break and move on
			int n = Exp::Break.Match(INPUT);
			breaks += INPUT.GetChar(n);
		}

		// finally, set the indentation
		if(indent == 0) {
			indent = maxIndent;
			if(indent < topIndent + 1)
				indent = topIndent + 1;
			if(indent < 1)
				indent = 1;
		}
	}

	// ScanScalar
	std::string ScanScalar(Stream& INPUT, ScanScalarInfo info)
	{
		bool emptyLine = false, moreIndented = false;
		std::string scalar;

		while(INPUT) {
			// ********************************
			// Phase #1: scan until line ending
			while(!info.end.Matches(INPUT) && !Exp::Break.Matches(INPUT)) {
				if(INPUT.peek() == EOF)
					break;

				// escaped newline? (only if we're escaping on slash)
				if(info.escape == '\\' && Exp::EscBreak.Matches(INPUT)) {
					int n = Exp::EscBreak.Match(INPUT);
					INPUT.Eat(n);
					continue;
				}

				// escape this?
				if(INPUT.peek() == info.escape) {
					scalar += Exp::Escape(INPUT);
					continue;
				}

				// otherwise, just add the damn character
				scalar += INPUT.GetChar();
			}

			// eof? if we're looking to eat something, then we throw
			if(INPUT.peek() == EOF) {
				if(info.eatEnd)
					throw EOFInQuote();
				break;
			}

			// are we done via character match?
			int n = info.end.Match(INPUT);
			if(n >= 0) {
				if(info.eatEnd)
					INPUT.Eat(n);
				break;
			}

			// ********************************
			// Phase #2: eat line ending
			n = Exp::Break.Match(INPUT);
			INPUT.Eat(n);

			// ********************************
			// Phase #3: scan initial spaces

			// first the required indentation
			while(INPUT.peek() == ' ' && INPUT.column < info.indent)
				INPUT.Eat(1);

			// and then the rest of the whitespace
			if(info.eatLeadingWhitespace) {
				while(Exp::Blank.Matches(INPUT))
					INPUT.Eat(1);
			}

			// was this an empty line?
			bool nextEmptyLine = Exp::Break.Matches(INPUT);
			bool nextMoreIndented = (INPUT.peek() == ' ');

			if(info.fold && !emptyLine && !nextEmptyLine && !moreIndented && !nextMoreIndented)
				scalar += " ";
			else
				scalar += "\n"; 

			emptyLine = nextEmptyLine;
			moreIndented = nextMoreIndented;

			// are we done via indentation?
			if(!emptyLine && INPUT.column < info.indent)
				break;
		}

		// post-processing
		if(info.trimTrailingSpaces) {
			unsigned pos = scalar.find_last_not_of(' ');
			if(pos < scalar.size())
				scalar.erase(pos + 1);
		}

		if(info.chomp <= 0) {
			unsigned pos = scalar.find_last_not_of('\n');
			if(info.chomp == 0 && pos + 1 < scalar.size())
				scalar.erase(pos + 2);
			else if(info.chomp == -1 && pos < scalar.size())
				scalar.erase(pos + 1);
		}

		return scalar;
	}
}
