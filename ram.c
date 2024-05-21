#include <ram.h>

// Memory buffer
// 1 MiB of memory
// 2^20 bytes of memory
char ram_buffer[RAM_SIZE];

// Read quad from memory
uint64_t read_ram(uint64_t addr) {
	uint64_t read_word = 0;

	for (int i = 0; i < sizeof(uint64_t); i++)
		read_word += ram_buffer[addr + i] << (i * 8);

	return read_word;
}

// Write quad to memory
void write_ram(uint64_t addr, uint64_t val) {
	for (int i = 0; i < sizeof(uint64_t); i++)
		ram_buffer[addr + i] = (char)(0xff & (val >> (i * 8)));
}
