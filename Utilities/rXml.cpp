#include "stdafx.h"
#include "Utilities/rXml.h"
#pragma warning(push)
#pragma warning(disable : 4996)
#include <pugixml.hpp>
#pragma warning(pop)

rXmlNode::rXmlNode()
{
	ownPtr = true;
	handle = new pugi::xml_node;
}

rXmlNode::rXmlNode(pugi::xml_node *ptr)
{
	ownPtr = false;
	handle = ptr;
}

rXmlNode::~rXmlNode()
{
	if (ownPtr)
	{
		delete handle;
	}
}

rXmlNode::rXmlNode(const rXmlNode& other)
{
	ownPtr = true;
	handle = new pugi::xml_node(*other.handle);
}

rXmlNode &rXmlNode::operator=(const rXmlNode& other)
{
	if (ownPtr)
	{
		delete handle;
	}
	handle = new pugi::xml_node(*other.handle);
	ownPtr = true;
	return *this;
}

std::shared_ptr<rXmlNode> rXmlNode::GetChildren()
{
	// it.begin() returns node_iterator*, *it.begin() return node*.
	pugi::xml_object_range<pugi::xml_node_iterator> it = handle->children();
	pugi::xml_node begin = *it.begin();

	if (begin)
	{
		return std::make_shared<rXmlNode>(&begin);
	}
	else
	{
		return nullptr;
	}
}

std::shared_ptr<rXmlNode> rXmlNode::GetNext()
{
	pugi::xml_node result = handle->next_sibling();
	if (result)
	{
		return std::make_shared<rXmlNode>(&result);
	}
	else
	{
		return nullptr;
	}
}

std::string rXmlNode::GetName()
{
	return handle->name();
}

std::string rXmlNode::GetAttribute(const std::string &name)
{
	auto pred = [&name](pugi::xml_attribute attr) { return (name == attr.name()); };
	return handle->find_attribute(pred).value();
}

std::string rXmlNode::GetNodeContent()
{
	return handle->text().get(); 
}

void *rXmlNode::AsVoidPtr()
{
	return static_cast<void*>(handle);
}

rXmlDocument::rXmlDocument()
{
	handle = new pugi::xml_document;
}

rXmlDocument::~rXmlDocument()
{
	delete handle;
}

void rXmlDocument::Load(const std::string & path)
{
	// TODO: Unsure of use of c_str.
	handle->load_string(path.c_str());
}

std::shared_ptr<rXmlNode> rXmlDocument::GetRoot()
{
	pugi::xml_node root = handle->root();
	return std::make_shared<rXmlNode>(&root);
}

void *rXmlDocument::AsVoidPtr()
{
	return static_cast<void*>(handle);
}
