#include "stdafx.h"
#include "Utilities/rXml.h"

rXmlNode::rXmlNode()
{
}

rXmlNode::rXmlNode(const pugi::xml_node& node)
{
	handle = node;
}

std::shared_ptr<rXmlNode> rXmlNode::GetChildren()
{
	if (handle)
	{
		if (const pugi::xml_node child = handle.first_child())
		{
			return std::make_shared<rXmlNode>(child);
		}
	}

	return nullptr;
}

std::shared_ptr<rXmlNode> rXmlNode::GetNext()
{
	if (handle)
	{
		if (const pugi::xml_node result = handle.next_sibling())
		{
			return std::make_shared<rXmlNode>(result);
		}
	}

	return nullptr;
}

std::string rXmlNode::GetName()
{
	if (handle)
	{
		if (const pugi::char_t* name = handle.name())
		{
			return name;
		}
	}

	return {};
}

std::string rXmlNode::GetAttribute(const std::string& name)
{
	if (handle)
	{
		const auto pred = [&name](const pugi::xml_attribute& attr) { return (name == attr.name()); };
		if (const pugi::xml_attribute attr = handle.find_attribute(pred))
		{
			if (const pugi::char_t* value = attr.value())
			{
				return value;
			}
		}
	}

	return {};
}

std::string rXmlNode::GetNodeContent()
{
	if (handle)
	{
		if (const pugi::xml_text text = handle.text())
		{
			if (const pugi::char_t* value = text.get())
			{
				return value;
			}
		}
	}

	return {};
}

rXmlDocument::rXmlDocument()
{
}

pugi::xml_parse_result rXmlDocument::Read(const std::string& data)
{
	if (handle)
	{
		return handle.load_buffer(data.data(), data.size());
	}

	return {};
}

std::shared_ptr<rXmlNode> rXmlDocument::GetRoot()
{
	if (const pugi::xml_node root = handle.root())
	{
		return std::make_shared<rXmlNode>(root);
	}

	return nullptr;
}
