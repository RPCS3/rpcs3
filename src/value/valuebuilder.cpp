#include "valuebuilder.h"
#include "yaml-cpp/mark.h"
#include "yaml-cpp/value.h"
#include <cassert>

namespace YAML
{
	ValueBuilder::ValueBuilder(): m_pMemory(new detail::memory_holder), m_pRoot(0), m_mapDepth(0)
	{
		m_anchors.push_back(0); // since the anchors start at 1
	}
	
	ValueBuilder::~ValueBuilder()
	{
	}
	
	Value ValueBuilder::Root()
	{
		if(!m_pRoot)
			return Value();
		
		return Value(*m_pRoot, m_pMemory);
	}

	void ValueBuilder::OnDocumentStart(const Mark&)
	{
	}
	
	void ValueBuilder::OnDocumentEnd()
	{
	}
	
	void ValueBuilder::OnNull(const Mark& mark, anchor_t anchor)
	{
		detail::node& node = Push(anchor);
		node.set_null();
		Pop();
	}
	
	void ValueBuilder::OnAlias(const Mark& /*mark*/, anchor_t anchor)
	{
		detail::node& node = *m_anchors[anchor];
		m_stack.push_back(&node);
		Pop();
	}
	
	void ValueBuilder::OnScalar(const Mark& mark, const std::string& tag, anchor_t anchor, const std::string& value)
	{
		detail::node& node = Push(anchor);
		node.set_scalar(value);
		Pop();
	}
	
	void ValueBuilder::OnSequenceStart(const Mark& mark, const std::string& tag, anchor_t anchor)
	{
		detail::node& node = Push(anchor);
		node.set_type(ValueType::Sequence);
	}
	
	void ValueBuilder::OnSequenceEnd()
	{
		Pop();
	}
	
	void ValueBuilder::OnMapStart(const Mark& mark, const std::string& tag, anchor_t anchor)
	{
		detail::node& node = Push(anchor);
		node.set_type(ValueType::Map);
		m_mapDepth++;
	}
	
	void ValueBuilder::OnMapEnd()
	{
		assert(m_mapDepth > 0);
		m_mapDepth--;
		Pop();
	}

	detail::node& ValueBuilder::Push(anchor_t anchor)
	{
		const bool needsKey = (!m_stack.empty() && m_stack.back()->type() == ValueType::Map && m_keys.size() < m_mapDepth);
		
		detail::node& node = m_pMemory->create_node();
		m_stack.push_back(&node);
		RegisterAnchor(anchor, node);
		
		if(needsKey)
			m_keys.push_back(&node);
		
		return node;
	}
	
	void ValueBuilder::Pop()
	{
		assert(!m_stack.empty());
		if(m_stack.size() == 1) {
			m_pRoot = m_stack[0];
			m_stack.pop_back();
			return;
		}
		
		detail::node& node = *m_stack.back();
		m_stack.pop_back();

		detail::node& collection = *m_stack.back();
		
		if(collection.type() == ValueType::Sequence) {
			collection.append(node, m_pMemory);
		} else if(collection.type() == ValueType::Map) {
			detail::node& key = *m_keys.back();
			if(&key != &node) {
				m_keys.pop_back();
				collection.insert(key, node, m_pMemory);
			}
		} else {
			assert(false);
			m_stack.clear();
		}
	}

	void ValueBuilder::RegisterAnchor(anchor_t anchor, detail::node& node)
	{
		if(anchor) {
			assert(anchor == m_anchors.size());
			m_anchors.push_back(&node);
		}
	}
}
