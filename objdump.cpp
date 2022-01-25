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
#include <sys/wait.h>
#include <sys/sendfile.h>
#include <spawn.h>
#include <unordered_map>
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

	// Get object names
	int fd = open((home + ".objects").c_str(), O_RDONLY);

	if (fd < 0)
		return 1;

	// Addr -> offset;size in .objects
	std::unordered_map<std::uint64_t, std::pair<std::uint64_t, std::uint64_t>> objects;

	while (true)
	{
		// Size is name size, not object size
		std::uint64_t ptr, size;
		if (read(fd, &ptr, 8) != 8 || read(fd, &size, 8) != 8)
			break;
		std::uint64_t off = lseek(fd, 0, SEEK_CUR);
		objects.emplace(ptr, std::make_pair(off, size));
		lseek(fd, size, SEEK_CUR);
	}

	std::vector<std::string> args;

	std::uint64_t addr = 0;

	for (int i = 0; i < argc; i++)
	{
		// Replace args
		std::string arg = argv[i];

		if (arg.find("--start-address=0x") == 0)
		{
			std::from_chars(arg.data() + strlen("--start-address=0x"), arg.data() + arg.size(), addr, 16);

			if (objects.count(addr))
			{
				// Extract object into a tmp file
				lseek(fd, objects[addr].first, SEEK_SET);
				const int fd2 = open("/tmp/rpcs3.objdump.tmp", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
				sendfile(fd2, fd, nullptr, objects[addr].second);
				close(fd2);

				args.emplace_back("--adjust-vma=" + to_hex(addr));
				continue;
			}
		}

		if (objects.count(addr) && arg.find("--stop-address=0x") == 0)
		{
			continue;
		}

		if (objects.count(addr) && arg == "-d")
		{
			arg = "-D";
		}

		if (arg == "-l")
		{
			arg = "-Mintel,x86-64";
		}

		args.emplace_back(std::move(arg));
	}

	if (objects.count(addr))
	{
		args.pop_back();
		args.emplace_back("-b");
		args.emplace_back("binary");
		args.emplace_back("-m");
		args.emplace_back("i386");
		args.emplace_back("/tmp/rpcs3.objdump.tmp");
	}

	args[0] = "/usr/bin/objdump";

	std::vector<char*> new_argv;

	for (auto& arg : args)
	{
		new_argv.push_back(arg.data());
	}

	new_argv.push_back(nullptr);

	if (objects.count(addr))
	{
		int fds[2];
		pipe(fds);

		// objdump is broken; fix address truncation
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
						// Replace broken address
						if ((buf[0] >= '0' && buf[0] <= '9') || (buf[0] >= 'a' && buf[0] <= 'f'))
						{
							std::uint64_t ptr = -1;
							auto cvt = std::from_chars(buf.data(), buf.data() + buf.size(), ptr, 16);

							if (cvt.ec == std::errc() && ptr < addr)
							{
								auto fix = to_hex((ptr - std::uint32_t(addr)) + addr, false);
								write(STDOUT_FILENO, fix.data(), fix.size());
								buf = std::string(cvt.ptr);
							}
						}

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
