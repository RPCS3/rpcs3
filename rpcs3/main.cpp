#include "stdafx.h"
#include "rpcs3.h"

LOG_CHANNEL(sys_log, "SYS");

int main(int argc, char** argv)
{
	const int exit_code = run_rpcs3(argc, argv);
	sys_log.notice("RPCS3 terminated with exit code %d", exit_code);
	return exit_code;
}
