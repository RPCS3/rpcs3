#include "aliascontent.h"

namespace YAML
{
	AliasContent::AliasContent(Content* pNodeContent)
		: m_pRef(pNodeContent)
	{
	}

	Content *AliasContent::Clone() const
	{
		return 0; // TODO: how to clone an alias?
	}

	void AliasContent::Parse(Scanner * /*pScanner*/, ParserState& /*state*/)
	{
	}

	void AliasContent::Write(Emitter&) const
	{
		// no content (just an alias)
	}

	bool AliasContent::GetBegin(std::vector <Node *>::const_iterator& i) const
	{
		return m_pRef->GetBegin(i);
	}

	bool AliasContent::GetBegin(std::map <Node *, Node *, ltnode>::const_iterator& i) const
	{
		return m_pRef->GetBegin(i);
	}

	bool AliasContent::GetEnd(std::vector <Node *>::const_iterator& i) const
	{
		return m_pRef->GetEnd(i);
	}

	bool AliasContent::GetEnd(std::map <Node *, Node *, ltnode>::const_iterator& i) const
	{
		return m_pRef->GetEnd(i);
	}

	Node* AliasContent::GetNode(std::size_t n) const
	{
		return m_pRef->GetNode(n);
	}

	std::size_t AliasContent::GetSize() const
	{
		return m_pRef->GetSize();
	}

	bool AliasContent::IsScalar() const
	{
		return m_pRef->IsScalar();
	}

	bool AliasContent::IsMap() const
	{
		return m_pRef->IsMap();
	}

	bool AliasContent::IsSequence() const
	{
		return m_pRef->IsSequence();
	}

	bool AliasContent::GetScalar(std::string& scalar) const
	{
		return m_pRef->GetScalar(scalar);
	}

	int AliasContent::Compare(Content *pContent)
	{
		return m_pRef->Compare(pContent);
	}

	int AliasContent::Compare(Scalar *pScalar)
	{
		return m_pRef->Compare(pScalar);
	}

	int AliasContent::Compare(Sequence *pSequence)
	{
		return m_pRef->Compare(pSequence);
	}

	int AliasContent::Compare(Map *pMap)
	{
		return m_pRef->Compare(pMap);
	}
}
