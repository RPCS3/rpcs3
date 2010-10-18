#pragma once

#ifndef PTR_STACK_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define PTR_STACK_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#include "yaml-cpp/noncopyable.h"
#include <memory>
#include <vector>

template <typename T>
class ptr_stack: private YAML::noncopyable
{
public:
	ptr_stack() {}
	~ptr_stack() { clear(); }
	
	void clear() {
		for(unsigned i=0;i<m_data.size();i++)
			delete m_data[i];
		m_data.clear();
	}
	
	std::size_t size() const { return m_data.size(); }
	bool empty() const { return m_data.empty(); }
	
	void push(std::auto_ptr<T> t) {
		m_data.push_back(NULL);
		m_data.back() = t.release();
	}
	std::auto_ptr<T> pop() {
		std::auto_ptr<T> t(m_data.back());
		m_data.pop_back();
		return t;
	}
	T& top() { return *m_data.back(); }
	
private:
	std::vector<T*> m_data;
};

#endif // PTR_STACK_H_62B23520_7C8E_11DE_8A39_0800200C9A66
