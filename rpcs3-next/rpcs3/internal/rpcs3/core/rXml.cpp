#include "stdafx.h"
#include "Utilities/rXml.h"
#pragma warning(push)
#pragma message("TODO: remove wx dependency: <wx/xml/xml.h>")
#pragma warning(disable : 4996)
#include <wx/xml/xml.h>
#pragma warning(pop)

rXmlNode::rXmlNode()
{
	ownPtr = true;
	handle = reinterpret_cast<void *>(new wxXmlNode());
}

rXmlNode::rXmlNode(void *ptr)
{
	ownPtr = false;
	handle = ptr;
}

rXmlNode::rXmlNode(const rXmlNode& other)
{
	ownPtr = true;
	handle = reinterpret_cast<void *>(new wxXmlNode(*reinterpret_cast<wxXmlNode*>(other.handle)));
}

rXmlNode &rXmlNode::operator=(const rXmlNode& other)
{
	if (ownPtr)
	{
		delete reinterpret_cast<wxXmlNode*>(handle);
	}
	handle = reinterpret_cast<void *>(new wxXmlNode(*reinterpret_cast<wxXmlNode*>(other.handle)));
	ownPtr = true;
	return *this;
}

rXmlNode::~rXmlNode()
{
	if (ownPtr)
	{
		delete reinterpret_cast<wxXmlNode*>(handle);
	}
}

std::shared_ptr<rXmlNode> rXmlNode::GetChildren()
{
	wxXmlNode* result = reinterpret_cast<wxXmlNode*>(handle)->GetChildren();
	if (result)
	{
		return std::make_shared<rXmlNode>(reinterpret_cast<void*>(result));
	}
	else
	{
		return std::shared_ptr<rXmlNode>(nullptr);
	}
}

std::shared_ptr<rXmlNode> rXmlNode::GetNext()
{
	wxXmlNode* result = reinterpret_cast<wxXmlNode*>(handle)->GetNext();
	if (result)
	{
		return std::make_shared<rXmlNode>(reinterpret_cast<void*>(result));
	}
	else
	{
		return std::shared_ptr<rXmlNode>(nullptr);
	}
}

std::string rXmlNode::GetName()
{
	return fmt::ToUTF8(reinterpret_cast<wxXmlNode*>(handle)->GetName());
}

std::string rXmlNode::GetAttribute(const std::string &name)
{
	return fmt::ToUTF8(reinterpret_cast<wxXmlNode*>(handle)->GetAttribute(fmt::FromUTF8(name)));
}

std::string rXmlNode::GetNodeContent()
{
	return fmt::ToUTF8(reinterpret_cast<wxXmlNode*>(handle)->GetNodeContent());
}

rXmlDocument::rXmlDocument()
{
	handle = reinterpret_cast<void *>(new wxXmlDocument());
}

rXmlDocument::rXmlDocument(const rXmlDocument& other)
{
	handle = reinterpret_cast<void *>(new wxXmlDocument(*reinterpret_cast<wxXmlDocument*>(other.handle)));
}

rXmlDocument &rXmlDocument::operator = (const rXmlDocument& other)
{
	delete reinterpret_cast<wxXmlDocument*>(handle);
	handle = reinterpret_cast<void *>(new wxXmlDocument(*reinterpret_cast<wxXmlDocument*>(other.handle)));
	return *this;
}

rXmlDocument::~rXmlDocument()
{
	delete reinterpret_cast<wxXmlDocument*>(handle);
}

void rXmlDocument::Load(const std::string & path)
{
	reinterpret_cast<wxXmlDocument*>(handle)->Load(fmt::FromUTF8(path));
}

std::shared_ptr<rXmlNode> rXmlDocument::GetRoot()
{
	return std::make_shared<rXmlNode>(reinterpret_cast<void*>(reinterpret_cast<wxXmlDocument*>(handle)->GetRoot()));
}
