#include "tests.h"

int main(int argc, char **argv)
{
#ifdef WINDOWS
	_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF|_CRTDBG_ALLOC_MEM_DF);
#endif // WINDOWS
	Test::RunAll();

	return 0;
}
