#pragma once

#ifndef NONCOPYABLE_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define NONCOPYABLE_H_62B23520_7C8E_11DE_8A39_0800200C9A66


namespace YAML
{
	// this is basically boost::noncopyable
	class noncopyable
		{
		protected:
			noncopyable() {}
			~noncopyable() {}
			
		private:
			noncopyable(const noncopyable&);
			const noncopyable& operator = (const noncopyable&);
		};
}

#endif // NONCOPYABLE_H_62B23520_7C8E_11DE_8A39_0800200C9A66
