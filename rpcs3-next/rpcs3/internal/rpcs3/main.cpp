namespace rpcs3
{
	void test_func();
}

#include <cstdio>

[[noreturn]] void main()
{
	freopen("out.txt", "w+", stdout);
	rpcs3::test_func();
}