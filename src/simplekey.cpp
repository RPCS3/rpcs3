#include "crt.h"
#include "scanner.h"
#include "token.h"
#include "exceptions.h"
#include "exp.h"

namespace YAML
{
	Scanner::SimpleKey::SimpleKey(const Mark& mark_, int flowLevel_)
		: mark(mark_), flowLevel(flowLevel_), pIndent(0), pMapStart(0), pKey(0)
	{
	}

	void Scanner::SimpleKey::Validate()
	{
		// Note: pIndent will *not* be garbage here; see below
		if(pIndent)
			pIndent->isValid = true;
		if(pMapStart)
			pMapStart->status = Token::VALID;
		if(pKey)
			pKey->status = Token::VALID;
	}

	void Scanner::SimpleKey::Invalidate()
	{
		// Note: pIndent might be a garbage pointer here, but that's ok
		//       An indent will only be popped if the simple key is invalid
		if(pMapStart)
			pMapStart->status = Token::INVALID;
		if(pKey)
			pKey->status = Token::INVALID;
	}

	// ExistsActiveSimpleKey
	// . Returns true if there's a potential simple key at our flow level
	//   (there's allowed at most one per flow level, i.e., at the start of the flow start token)
	bool Scanner::ExistsActiveSimpleKey() const
	{
		if(m_simpleKeys.empty())
			return false;
		
		const SimpleKey& key = m_simpleKeys.top();
		return key.flowLevel == m_flowLevel;
	}

	// InsertPotentialSimpleKey
	// . If we can, add a potential simple key to the queue,
	//   and save it on a stack.
	void Scanner::InsertPotentialSimpleKey()
	{
		if(ExistsActiveSimpleKey())
			return;
		
		SimpleKey key(INPUT.mark(), m_flowLevel);

		// first add a map start, if necessary
		key.pIndent = PushIndentTo(INPUT.column(), IndentMarker::MAP);
		if(key.pIndent) {
			key.pIndent->isValid = false;
			key.pMapStart = key.pIndent->pStartToken;
			key.pMapStart->status = Token::UNVERIFIED;
		}

		// then add the (now unverified) key
		m_tokens.push(Token(Token::KEY, INPUT.mark()));
		key.pKey = &m_tokens.back();
		key.pKey->status = Token::UNVERIFIED;

		m_simpleKeys.push(key);
	}

	// InvalidateSimpleKey
	// . Automatically invalidate the simple key in our flow level
	void Scanner::InvalidateSimpleKey()
	{
		if(m_simpleKeys.empty())
			return;
		
		// grab top key
		SimpleKey& key = m_simpleKeys.top();
		if(key.flowLevel != m_flowLevel)
			return;
		
		key.Invalidate();
		m_simpleKeys.pop();
	}

	// VerifySimpleKey
	// . Determines whether the latest simple key to be added is valid,
	//   and if so, makes it valid.
	bool Scanner::VerifySimpleKey()
	{
		m_isLastKeyValid = false;
		if(m_simpleKeys.empty())
			return m_isLastKeyValid;

		// grab top key
		SimpleKey key = m_simpleKeys.top();

		// only validate if we're in the correct flow level
		if(key.flowLevel != m_flowLevel)
			return false;

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

		m_isLastKeyValid = isValid;
		return isValid;
	}

	void Scanner::PopAllSimpleKeys()
	{
		while(!m_simpleKeys.empty())
			m_simpleKeys.pop();
	}
}
