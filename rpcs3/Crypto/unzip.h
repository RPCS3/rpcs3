#pragma once

std::vector<u8> unzip(const void* src, usz size);

template <typename T>
inline std::vector<u8> unzip(const T& src)
{
	return unzip(src.data(), src.size());
}

bool unzip(const void* src, usz size, fs::file& out);

template <typename T>
inline bool unzip(const std::vector<u8>& src, fs::file& out)
{
	return unzip(src.data(), src.size(), out);
}

bool zip(const void* src, usz size, fs::file& out, bool multi_thread_it = false);

template <typename T>
inline bool zip(const T& src, fs::file& out)
{
	return zip(src.data(), src.size(), out);
}
