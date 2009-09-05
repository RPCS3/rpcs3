#include "crt.h"
#include "scanner.h"
#include "token.h"
#include "exceptions.h"
#include "exp.h"

namespace YAML
{
	Scanner::SimpleKey::SimpleKey(const Mark& mark_, int flowLevel_)
		: mark(mark_), flowLevel(flowLevel_), pMapStart(0), pKey(0)
	{
	}

	void Scanner::SimpleKey::Validate()
	{
		if(pMapStart)
			pMapStart->status = Token::VALID;
		if(pKey)
			pKey->status = Token::VALID;
	}

	void Scanner::SimpleKey::Invalidate()
	{
		if(pMapStart)
			pMapStart->status = Token::INVALID;
		if(pKey)
			pKey->status = Token::INVALID;
	}

	// InsertSimpleKey
	// . Adds a potential simple key to the queue,
	//   and saves it on a stack.
	void Scanner::InsertSimpleKey()
	{
		SimpleKey key(INPUT.mark(), m_flowLevel);

		// first add a map start, if necessary
		key.pMapStart = PushIndentTo(INPUT.column(), IndentMarker::MAP);
		if(key.pMapStart)
			key.pMapStart->status = Token::UNVERIFIED;

		// then add the (now unverified) key
		m_tokens.push(Token(Token::KEY, INPUT.mark()));
		key.pKey = &m_tokens.back();
		key.pKey->status = Token::UNVERIFIED;

		m_simpleKeys.push(key);
	}

	// VerifySimpleKey
	// . Determines whether the latest simple key to be added is valid,
	//   and if so, makes it valid.
	// . If 'force' is true, then we'll pop no matter what (whether we can verify it or not).
	bool Scanner::VerifySimpleKey(bool force)
	{
		m_isLastKeyValid = false;
		if(m_simpleKeys.empty())
			return m_isLastKeyValid;

		// grab top key
		SimpleKey key = m_simpleKeys.top();

		// only validate if we're in the correct flow level
		if(key.flowLevel != m_flowLevel) {
			if(force)
				m_simpleKeys.pop();
			return false;
		}

		m_simpleKeys.pop();

		bool isValid = true;

		// needs to be followed immediately by a value
		if(m_flowLevel > 0 && !Exp::ValueInFlow.Matches(INPUT))
			isValid = false;
		if(m_flowLevel == 0 && !Exp::Value.Matches(INPUT))
			isValid = false;

		// also needs to be less than 1024 characters and inline
		if(INPUT.line() != key.mark.line || INPUT.pos() - key.mark.pos > 1024)
			isValid = false;

		// invalidate key
		if(isValid)
			key.Validate();
		else
			key.Invalidate();

		// In block style, remember that we've pushed an indent for this potential simple key (if it was starting).
		// If it was invalid, then we need to pop it off.
		// Note: we're guaranteed to be popping the right one (i.e., there couldn't have been anything in
		//       between) since keys have to be inline, and will be invalidated immediately on a newline.
		if(!isValid && m_flowLevel == 0)
			m_indents.pop();

		m_isLastKeyValid = isValid;
		return isValid;
	}

	// VerifyAllSimplyKeys
	// . Pops all simple keys (with checking, but if we can't verify one, then pop it anyways).
	void Scanner::VerifyAllSimpleKeys()
	{
		while(!m_simpleKeys.empty())
			VerifySimpleKey(true);
	}
}
