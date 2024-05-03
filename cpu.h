#ifndef CPU_H
#define CPU_H

#include <stdint.h>

// Instruction data
typedef struct {
	uint8_t icode;
	uint8_t ifun;
	int rega;
	int regb;
	int valc;
} instruction_t;

// Initialize CPU
void init_cpu();

// Cycle through stages
// Return a negative number if there is an exception
// Return 0 if succeeded
int cycle(void);

// Fetch
int fetch(instruction_t*);

// Decode
int decode(instruction_t*);

// Execute
int execute(instruction_t*);

// Memory
int memory(instruction_t*);

// Write back
int writeback(instruction_t*);

#endif
