

// Boiler plate exception handlers used at boot time.
// EXCEPMAN replaces these with something more useful.

__asm__(".org 0x0000");

__asm__(".set noreorder");
void CpuException0() {
}

__asm__(".org 0x0080");

__asm__(".set noreorder");
void CpuException() {
}

