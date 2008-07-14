#include "node.h"
#include "exceptions.h"

namespace YAML
{
	Node::Iterator::Iterator(): type(IT_NONE)
	{
	}

	Node::Iterator::Iterator(std::vector <Node *>::const_iterator it): seqIter(it), type(IT_SEQ)
	{
	}

	Node::Iterator::Iterator(std::map <Node *, Node *, ltnode>::const_iterator it): mapIter(it), type(IT_MAP)
	{
	}

	Node::Iterator::~Iterator()
	{
	}

	Node::Iterator& Node::Iterator::operator ++ ()
	{
		if(type == IT_SEQ)
			++seqIter;
		else if(type == IT_MAP)
			++mapIter;

		return *this;
	}

	Node::Iterator Node::Iterator::operator ++ (int)
	{
		Iterator temp = *this;

		if(type == IT_SEQ)
			++seqIter;
		else if(type == IT_MAP)
			++mapIter;

		return temp;
	}

	const Node& Node::Iterator::operator * ()
	{
		if(type == IT_SEQ)
			return **seqIter;

		throw BadDereference();
	}

	const Node *Node::Iterator::operator -> ()
	{
		if(type == IT_SEQ)
			return &**seqIter;

		throw BadDereference();
	}

	const Node& Node::Iterator::first()
	{
		if(type == IT_MAP)
			return *mapIter->first;

		throw BadDereference();
	}

	const Node& Node::Iterator::second()
	{
		if(type == IT_MAP)
			return *mapIter->second;

		throw BadDereference();
	}

	bool operator == (const Node::Iterator& it, const Node::Iterator& jt)
	{
		if(it.type != jt.type)
			return false;

		if(it.type == Node::Iterator::IT_SEQ)
			return it.seqIter == jt.seqIter;
		else if(it.type == Node::Iterator::IT_MAP)
			return it.mapIter == jt.mapIter;

		return true;
	}

	bool operator != (const Node::Iterator& it, const Node::Iterator& jt)
	{
		return !(it == jt);
	}
}
