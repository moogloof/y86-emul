#ifndef RAM_H
#define RAM_H

#include <stdint.h>

// Define ram size
#define RAM_SIZE 0x100000

// Read quad from memory
uint64_t read_ram(uint64_t);

// Write quad to memory
void write_ram(uint64_t, uint64_t);

#endif
