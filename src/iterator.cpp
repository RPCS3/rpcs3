#include "node.h"
#include "exceptions.h"
#include "iterpriv.h"

namespace YAML
{
	Iterator::Iterator(): m_pData(0)
	{
		m_pData = new IterPriv;
	}

	Iterator::Iterator(IterPriv *pData): m_pData(pData)
	{
	}

	Iterator::Iterator(const Iterator& rhs): m_pData(0)
	{
		m_pData = new IterPriv(*rhs.m_pData);
	}

	Iterator& Iterator::operator = (const Iterator& rhs)
	{
		if(this == &rhs)
			return *this;

		delete m_pData;
		m_pData = new IterPriv(*rhs.m_pData);
		return *this;
	}

	Iterator::~Iterator()
	{
		delete m_pData;
	}

	Iterator& Iterator::operator ++ ()
	{
		if(m_pData->type == IterPriv::IT_SEQ)
			++m_pData->seqIter;
		else if(m_pData->type == IterPriv::IT_MAP)
			++m_pData->mapIter;

		return *this;
	}

	Iterator Iterator::operator ++ (int)
	{
		Iterator temp = *this;

		if(m_pData->type == IterPriv::IT_SEQ)
			++m_pData->seqIter;
		else if(m_pData->type == IterPriv::IT_MAP)
			++m_pData->mapIter;

		return temp;
	}

	const Node& Iterator::operator * () const
	{
		if(m_pData->type == IterPriv::IT_SEQ)
			return **m_pData->seqIter;

		throw BadDereference();
	}

	const Node *Iterator::operator -> () const
	{
		if(m_pData->type == IterPriv::IT_SEQ)
			return *m_pData->seqIter;

		throw BadDereference();
	}

	const Node& Iterator::first() const
	{
		if(m_pData->type == IterPriv::IT_MAP)
			return *m_pData->mapIter->first;

		throw BadDereference();
	}

	const Node& Iterator::second() const
	{
		if(m_pData->type == IterPriv::IT_MAP)
			return *m_pData->mapIter->second;

		throw BadDereference();
	}

	bool operator == (const Iterator& it, const Iterator& jt)
	{
		if(it.m_pData->type != jt.m_pData->type)
			return false;

		if(it.m_pData->type == IterPriv::IT_SEQ)
			return it.m_pData->seqIter == jt.m_pData->seqIter;
		else if(it.m_pData->type == IterPriv::IT_MAP)
			return it.m_pData->mapIter == jt.m_pData->mapIter;

		return true;
	}

	bool operator != (const Iterator& it, const Iterator& jt)
	{
		return !(it == jt);
	}
}
