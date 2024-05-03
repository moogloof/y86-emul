#include <cpu.h>

// CPU State
struct {
	uint64_t pc; // Program counter
	uint64_t rax; // Rest are general registers
	uint64_t rcx;
	uint64_t rdx;
	uint64_t rbx;
	uint64_t rsi;
	uint64_t rdi;
	uint64_t rsp; // Stack registers
	uint64_t rbp;
} cpu_state;

// Initialize CPU
void init_cpu() {
	cpu_state.pc = 0;
}
