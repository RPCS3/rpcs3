#pragma once

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
