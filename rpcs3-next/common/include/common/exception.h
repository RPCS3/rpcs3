#pragma once
#include "platform.h"
#include <memory>

namespace rpcs3
{
	inline namespace core
	{
		struct exception : public std::exception
		{
			std::unique_ptr<char[]> message;

			template<typename... Args> never_inline safe_buffers exception(const char* file, int line, const char* func, const char* text, Args... args) noexcept
			{
				const std::string data = format(text, args...) + format("\n(in file %s:%d, in function %s)", file, line, func);

				message.reset(new char[data.size() + 1]);

				std::memcpy(message.get(), data.c_str(), data.size() + 1);
			}

			exception(const exception& other) noexcept
			{
				const std::size_t size = std::strlen(other.message.get());

				message.reset(new char[size + 1]);

				std::memcpy(message.get(), other.message.get(), size + 1);
			}

			virtual const char* what() const noexcept override
			{
				return message.get();
			}
		};
	}
}

#define EXCEPTION(text, ...) rpcs3::core::exception(__FILE__, __LINE__, __FUNCTION__, text, ##__VA_ARGS__)