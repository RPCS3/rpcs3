#include "scanscalar.h"
#include "scanner.h"
#include "exp.h"
#include "exceptions.h"
#include "token.h"

namespace YAML
{
	// ScanScalar
	std::string ScanScalar(Stream& INPUT, ScanScalarInfo& info)
	{
		bool foundNonEmptyLine = false;
		bool emptyLine = false, moreIndented = false;
		std::string scalar;
		info.leadingSpaces = false;

		while(INPUT) {
			// ********************************
			// Phase #1: scan until line ending
			while(!info.end.Matches(INPUT) && !Exp::Break.Matches(INPUT)) {
				if(INPUT.peek() == EOF)
					break;

				// document indicator?
				if(INPUT.column == 0 && Exp::DocIndicator.Matches(INPUT)) {
					if(info.onDocIndicator == BREAK)
						break;
					else if(info.onDocIndicator == THROW)
						throw IllegalDocIndicator();
				}

				foundNonEmptyLine = true;

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
					throw IllegalEOF();
				break;
			}

			// doc indicator?
			if(info.onDocIndicator == BREAK && INPUT.column == 0 && Exp::DocIndicator.Matches(INPUT))
				break;

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
			while(INPUT.peek() == ' ' && (INPUT.column < info.indent || (info.detectIndent && !foundNonEmptyLine)))
				INPUT.Eat(1);

			// update indent if we're auto-detecting
			if(info.detectIndent && !foundNonEmptyLine)
				info.indent = std::max(info.indent, INPUT.column);

			// and then the rest of the whitespace
			while(Exp::Blank.Matches(INPUT)) {
				// we check for tabs that masquerade as indentation
				if(INPUT.peek() == '\t'&& INPUT.column < info.indent && info.onTabInIndentation == THROW)
					throw IllegalTabInIndentation();

				if(!info.eatLeadingWhitespace)
					break;

				INPUT.Eat(1);
			}

			// was this an empty line?
			bool nextEmptyLine = Exp::Break.Matches(INPUT);
			bool nextMoreIndented = (INPUT.peek() == ' ');

			// TODO: for block scalars, we always start with a newline, so we should fold OR keep that

			if(info.fold && !emptyLine && !nextEmptyLine && !moreIndented && !nextMoreIndented)
				scalar += " ";
			else
				scalar += "\n";

			emptyLine = nextEmptyLine;
			moreIndented = nextMoreIndented;

			// are we done via indentation?
			if(!emptyLine && INPUT.column < info.indent) {
				info.leadingSpaces = true;
				break;
			}
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

	// PlainScalarToken
	template <> PlainScalarToken *Scanner::ScanToken(PlainScalarToken *pToken)
	{
		// set up the scanning parameters
		ScanScalarInfo info;
		info.end = (m_flowLevel > 0 ? Exp::EndScalarInFlow : Exp::EndScalar) || (RegEx(' ') + Exp::Comment);
		info.eatEnd = false;
		info.indent = (m_flowLevel > 0 ? 0 : m_indents.top() + 1);
		info.fold = true;
		info.eatLeadingWhitespace = true;
		info.trimTrailingSpaces = true;
		info.chomp = CLIP;
		info.onDocIndicator = BREAK;
		info.onTabInIndentation = THROW;

		// insert a potential simple key
		if(m_simpleKeyAllowed)
			InsertSimpleKey();

		pToken->value = ScanScalar(INPUT, info);

		// can have a simple key only if we ended the scalar by starting a new line
		m_simpleKeyAllowed = info.leadingSpaces;

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
		ScanScalarInfo info;
		info.end = (pToken->single ? RegEx(quote) && !Exp::EscSingleQuote : RegEx(quote));
		info.eatEnd = true;
		info.escape = (pToken->single ? '\'' : '\\');
		info.indent = 0;
		info.fold = true;
		info.eatLeadingWhitespace = true;
		info.trimTrailingSpaces = false;
		info.chomp = CLIP;
		info.onDocIndicator = THROW;

		// insert a potential simple key
		if(m_simpleKeyAllowed)
			InsertSimpleKey();

		pToken->value = ScanScalar(INPUT, info);
		m_simpleKeyAllowed = false;

		return pToken;
	}

	// BlockScalarToken
	// . These need a little extra processing beforehand.
	// . We need to scan the line where the indicator is (this doesn't count as part of the scalar),
	//   and then we need to figure out what level of indentation we'll be using.
	template <> BlockScalarToken *Scanner::ScanToken(BlockScalarToken *pToken)
	{
		ScanScalarInfo info;
		info.indent = 1;
		info.detectIndent = true;

		// eat block indicator ('|' or '>')
		char indicator = INPUT.GetChar();
		info.fold = (indicator == Keys::FoldedScalar);

		// eat chomping/indentation indicators
		int n = Exp::Chomp.Match(INPUT);
		for(int i=0;i<n;i++) {
			char ch = INPUT.GetChar();
			if(ch == '+')
				info.chomp = KEEP;
			else if(ch == '-')
				info.chomp = STRIP;
			else if(Exp::Digit.Matches(ch)) {
				info.indent = ch - '0';
				info.detectIndent = false;
				if(info.indent == 0)
					throw ZeroIndentationInBlockScalar();
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
			info.indent += m_indents.top();

		info.eatLeadingWhitespace = false;
		info.trimTrailingSpaces = false;
		info.onTabInIndentation = THROW;

		pToken->value = ScanScalar(INPUT, info);

		// simple keys always ok after block scalars (since we're gonna start a new line anyways)
		m_simpleKeyAllowed = true;
		return pToken;
	}
}
