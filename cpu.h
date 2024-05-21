#ifndef CPU_H
#define CPU_H

#include <stdint.h>

typedef struct {
	uint64_t newPC;
	uint64_t valM;
	uint64_t valE;
	uint64_t valA;
	uint64_t valB;
	uint64_t cnd;
	struct {
		uint64_t valC;
		uint64_t valP;
		uint8_t icode;
		uint8_t ifun;
		uint8_t regA;
		uint8_t regB;
	} instruction_data;
	struct {
		uint8_t zf;
		uint8_t sf;
		uint8_t of;
	} eflags;
} state;

// Initialize CPU
void init_cpu(void);

// Cycle through stages
// Return a negative number if there is an exception
// Return 0 if succeeded
int cycle(void);

// Fetch
int fetch(void);

// Decode
int decode(void);

// Execute
int execute(void);

// Memory
int memory(void);

// Write back
int writeback(void);

#endif
