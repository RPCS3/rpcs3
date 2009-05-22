#include "crt.h"
#include "alias.h"
#include <iostream>

namespace YAML
{
	Alias::Alias(Content* pNodeContent)
		: m_pRef(pNodeContent)
	{
	}

	void Alias::Parse(Scanner *pScanner, const ParserState& state)
	{
	}

	void Alias::Write(std::ostream& out, int indent, bool startedLine, bool onlyOneCharOnLine)
	{
		out << "\n";
	}

	bool Alias::GetBegin(std::vector <Node *>::const_iterator& i) const
	{
		return m_pRef->GetBegin(i);
	}

	bool Alias::GetBegin(std::map <Node *, Node *, ltnode>::const_iterator& i) const
	{
		return m_pRef->GetBegin(i);
	}

	bool Alias::GetEnd(std::vector <Node *>::const_iterator& i) const
	{
		return m_pRef->GetEnd(i);
	}

	bool Alias::GetEnd(std::map <Node *, Node *, ltnode>::const_iterator& i) const
	{
		return m_pRef->GetEnd(i);
	}

	Node* Alias::GetNode(unsigned n) const
	{
		return m_pRef->GetNode(n);
	}

	unsigned Alias::GetSize() const
	{
		return m_pRef->GetSize();
	}

	bool Alias::IsScalar() const
	{
		return m_pRef->IsScalar();
	}

	bool Alias::IsMap() const
	{
		return m_pRef->IsMap();
	}

	bool Alias::IsSequence() const
	{
		return m_pRef->IsSequence();
	}

	bool Alias::Read(std::string& v) const
	{
		return m_pRef->Read(v);
	}

	bool Alias::Read(int& v) const
	{
		return m_pRef->Read(v);
	}

	bool Alias::Read(unsigned& v) const
	{
		return m_pRef->Read(v);
	}

	bool Alias::Read(long& v) const
	{
		return m_pRef->Read(v);
	}

	bool Alias::Read(float& v) const
	{
		return m_pRef->Read(v);
	}

	bool Alias::Read(double& v) const
	{
		return m_pRef->Read(v);
	}

	bool Alias::Read(char& v) const
	{
		return m_pRef->Read(v);
	}

	bool Alias::Read(bool& v) const
	{
		return m_pRef->Read(v);
	}

	int Alias::Compare(Content *pContent)
	{
		return m_pRef->Compare(pContent);
	}

	int Alias::Compare(Scalar *pScalar)
	{
		return m_pRef->Compare(pScalar);
	}

	int Alias::Compare(Sequence *pSequence)
	{
		return m_pRef->Compare(pSequence);
	}

	int Alias::Compare(Map *pMap)
	{
		return m_pRef->Compare(pMap);
	}
}
