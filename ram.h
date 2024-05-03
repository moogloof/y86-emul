#ifndef RAM_H
#define RAM_H

#include <stdint.h>

// Read quad from memory
uint64_t read(uint64_t);

// Write quad to memory
uint64_t write(uint64_t, uint64_t);

#endif
