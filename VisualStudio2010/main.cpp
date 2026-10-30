#include <xtl.h>
#include <stdio.h>
#include "CPU/PPUInterpreter.h"
int main() { ExperimentalPPU cpu; cpu.Reset(); bool ok = cpu.Execute(0x38600007u) && cpu.GetRegister(3) == 7; printf("RPCS3 Xbox 360 test: %s\n", ok ? "PASS" : "FAIL"); for (;;) Sleep(1000); return 0; }
