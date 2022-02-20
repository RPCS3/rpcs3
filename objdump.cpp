// objdump injection utility for Linux perf tools.
// Profiling JIT generated code is always problematic.
// On Linux, perf annotation tools do not automatically
// disassemble runtime-generated code.
// However, it's possible to override objdump utility
// which is used to disassemeble executables.
// This tool intercepts objdump commands, and if they
// correspond to JIT generated objects in RPCS3,
// it should be able to correctly disassemble them.
// Usage:
// 1. Make sure ~/.cache/rpcs3/ASMJIT directory exists.
// 2. Build this utility, for example:
//    g++-11 objdump.cpp -o objdump
// 3. Run perf, for example:
//    perf record -b -p `pgrep rpcs3`
// 4. Specify --objdump override, for example:
//    perf report --objdump=./objdump --gtk

#include <cstring>
#include <cstdio>
#include <cstdint>
#include <unistd.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <string>
#include <vector>
#include <charconv>

std::string to_hex(std::uint64_t value, bool prfx = true)
{
	char buf[20]{}, *ptr = buf + 19;
	do *--ptr = "0123456789abcdef"[value % 16], value /= 16; while (value);
	if (!prfx) return ptr;
	*--ptr = 'x';
	*--ptr = '0';
	return ptr;
}

int main(int argc, char* argv[])
{
	std::string home;

	if (const char* d = ::getenv("XDG_CACHE_HOME"))
		home = d;
	else if (const char* d = ::getenv("XDG_CONFIG_HOME"))
		home = d;
	else if (const char* d = ::getenv("HOME"))
		home = d, home += "/.cache";

	// Get cache path
	home += "/rpcs3/ASMJIT/";

	// Get objects
	int fd = open((home + ".objects").c_str(), O_RDONLY);

	if (fd < 0)
		return 1;

	// Map 4GiB (full size)
	const auto data = mmap(nullptr, 0x10000'0000, PROT_READ, MAP_SHARED, fd, 0);

	struct entry
	{
		std::uint64_t addr;
		std::uint32_t size;
		std::uint32_t off;
	};

	// Index part (precedes actual data)
	const auto index = static_cast<const entry*>(data);

	const entry* found = nullptr;

	std::string out_file;

	std::vector<std::string> args;

	for (int i = 0; i < argc; i++)
	{
		// Replace args
		std::string arg = argv[i];

		if (std::uintptr_t(data) != -1 && arg.find("--start-address=0x") == 0)
		{
			// Decode address and try to find the object
			std::uint64_t addr = -1;

			std::from_chars(arg.data() + strlen("--start-address=0x"), arg.data() + arg.size(), addr, 16);

			for (int j = 0; j < 0x100'0000; j++)
			{
				if (index[j].addr == 0)
				{
					break;
				}

				if (index[j].addr == addr)
				{
					found = index + j;
					break;
				}
			}

			if (found)
			{
				// Extract object into a new file (read file name from the mapped memory)
				const char* name = static_cast<char*>(data) + found->off + found->size;

				if (name[0])
				{
					out_file = home + name;
				}
				else
				{
					out_file = "/tmp/rpcs3.objdump." + std::to_string(getpid());
					unlink(out_file.c_str());
				}

				const int fd2 = open(out_file.c_str(), O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

				if (fd2 > 0)
				{
					// Don't overwrite if exists
					write(fd2, static_cast<char*>(data) + found->off, found->size);
					close(fd2);
				}

				args.emplace_back("--adjust-vma=" + to_hex(addr));
				continue;
			}
		}

		if (found && arg.find("--stop-address=0x") == 0)
		{
			continue;
		}

		if (found && arg == "-d")
		{
			arg = "-D";
		}

		if (arg == "-l")
		{
			arg = "-Mintel,x86-64";
		}

		args.emplace_back(std::move(arg));
	}

	if (found)
	{
		args.pop_back();
		args.emplace_back("-b");
		args.emplace_back("binary");
		args.emplace_back("-m");
		args.emplace_back("i386:x86-64");
		args.emplace_back(std::move(out_file));
	}

	args[0] = "/usr/bin/objdump";

	std::vector<char*> new_argv;

	for (auto& arg : args)
	{
		new_argv.push_back(arg.data());
	}

	new_argv.push_back(nullptr);

	if (found)
	{
		int fds[2];
		pipe(fds);

		if (fork() > 0)
		{
			close(fds[1]);
			char c = 0;
			std::string buf;

			while (read(fds[0], &c, 1) != 0)
			{
				if (c)
				{
					buf += c;

					if (c == '\n')
					{
						write(STDOUT_FILENO, buf.data(), buf.size());
						buf.clear();
					}
				}

				c = 0;
			}

			return 0;
		}
		else
		{
			while ((dup2(fds[1], STDOUT_FILENO) == -1) && (errno == EINTR)) {}
			close(fds[1]);
			close(fds[0]);
			// Fallthrough
		}
	}

	return execv(new_argv[0], new_argv.data());
}
