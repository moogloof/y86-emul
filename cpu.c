#include <cpu.h>
#include <ram.h>

extern char* ram_buffer;

// CPU State
struct {
	uint64_t registers[8];
	uint64_t pc; // Program counter
	state fetch;
	state decode;
	state execute;
	state memory;
	state writeback;
	uint8_t halted; // Just a flag to see if halted or not
	uint8_t stalling;
} cpu_state;

// Initialize CPU
void init_cpu(void) {
	cpu_state.pc = 0;
	cpu_state.halted = 0;
	cpu_state.stalling = 0;
}

// Cycle through stages
int cycle(void) {
	// Run pipeline
	if (writeback() < 0)
		return -5;
	if (memory() < 0)
		return -4;
	if (execute() < 0)
		return -3;
	if (decode() < 0)
		return -2;
	if (fetch() < 0)
		return -1;

	// Move pipeline
	cpu_state.writeback = cpu_state.memory;
	cpu_state.memory = cpu_state.execute;
	cpu_state.execute = cpu_state.decode;
	cpu_state.decode = cpu_state.fetch;

	return 0;
}

// Fetch
int fetch(void) {
	cpu_state.fetch.instruction_data.icode = ram_buffer[cpu_state.pc] >> 4;
	cpu_state.fetch.instruction_data.ifun = ram_buffer[cpu_state.pc] & 0xf;

	cpu_state.fetch.instruction_data.regA = 0xf;
	cpu_state.fetch.instruction_data.regB = 0xf;

	cpu_state.fetch.instruction_data.valP = cpu_state.pc;

	switch (cpu_state.fetch.instruction_data.icode) {
		case 0: case 1:
			cpu_state.fetch.instruction_data.valP += 1;
			break;
		case 2:
			cpu_state.fetch.instruction_data.regA = ram_buffer[cpu_state.pc + 1] >> 4;
			cpu_state.fetch.instruction_data.regB = ram_buffer[cpu_state.pc + 1] & 0xf;
			cpu_state.fetch.instruction_data.valP += 2;
			break;
		case 3: case 4: case 5:
			cpu_state.fetch.instruction_data.regA = ram_buffer[cpu_state.pc + 1] >> 4;
			cpu_state.fetch.instruction_data.regB = ram_buffer[cpu_state.pc + 1] & 0xf;
			cpu_state.fetch.instruction_data.valC = read_ram(cpu_state.pc + 2);
			cpu_state.fetch.instruction_data.valP += 10;
			break;
		case 6:
			cpu_state.fetch.instruction_data.regA = ram_buffer[cpu_state.pc + 1] >> 4;
			cpu_state.fetch.instruction_data.regB = ram_buffer[cpu_state.pc + 1] & 0xf;
			cpu_state.fetch.instruction_data.valP += 2;
			if (cpu_state.fetch.instruction_data.ifun > 3)
				return -1;
			break;
		case 7:
			cpu_state.fetch.instruction_data.valC = read_ram(cpu_state.pc + 1);
			cpu_state.fetch.instruction_data.valP = cpu_state.fetch.instruction_data.valC;
			break;
		case 8:
			cpu_state.fetch.instruction_data.valC = read_ram(cpu_state.pc + 1);
			cpu_state.fetch.instruction_data.valP = cpu_state.fetch.instruction_data.valC;
			cpu_state.fetch.instruction_data.regB = 4;
			break;
		case 9:
			cpu_state.fetch.instruction_data.regA = 4;
			cpu_state.fetch.instruction_data.regB = 4;
			cpu_state.fetch.instruction_data.valP += 1;
			break;
		case 0xa: case 0xb:
			cpu_state.fetch.instruction_data.regA = ram_buffer[cpu_state.pc + 1] >> 4;
			cpu_state.fetch.instruction_data.regB = 4;
			cpu_state.fetch.instruction_data.valP += 2;
			break;
		default:
			return -1;
	}

	return 0;
}

// Decode
int decode(void) {
	if (cpu_state.decode.instruction_data.regA != 0xf) {
		if (cpu_state.decode.instruction_data.regA < 8) {
			cpu_state.decode.valA = cpu_state.registers[cpu_state.decode.instruction_data.regA];
		} else {
			return -1;
		}
	}

	if (cpu_state.decode.instruction_data.regB != 0xf) {
		if (cpu_state.decode.instruction_data.regA < 8) {
			cpu_state.decode.valB = cpu_state.registers[cpu_state.decode.instruction_data.regB];
		} else {
			return -1;
		}
	}

	return 0;
}

// Helper for the flags
static int check_flag(uint8_t flag, uint8_t cpu_zf, uint8_t cpu_sf, uint8_t cpu_of) {
	// Check for flags
	switch (cpu_state.execute.instruction_data.ifun) {
		case 0:
			return 1;
		case 1:
			return cpu_zf || cpu_sf != cpu_of;
		case 2:
			return !cpu_zf && cpu_sf != cpu_of;
		case 3:
			return cpu_zf;
		case 4:
			return !cpu_zf;
		case 5:
			return cpu_zf || cpu_sf == cpu_of;
		case 6:
			return !cpu_zf && cpu_sf == cpu_of;
		default:
			return -1;
	}
}

// Execute
int execute(void) {
	int do_write;

	switch (cpu_state.execute.instruction_data.icode) {
		case 0: case 1:
			break;
		case 2:
			do_write = check_flag(cpu_state.execute.instruction_data.ifun, cpu_state.execute.eflags.zf, cpu_state.execute.eflags.sf, cpu_state.execute.eflags.of);

			if (do_write < 0)
				return -1;
			else if (do_write)
				cpu_state.execute.valE = cpu_state.execute.valA;

			break;
		case 3:
			cpu_state.execute.valE = cpu_state.execute.instruction_data.valC;
			break;
		case 4: case 5:
			cpu_state.execute.valE = cpu_state.execute.valB + cpu_state.execute.instruction_data.valC;
			break;
		case 6:
			switch (cpu_state.execute.instruction_data.ifun) {
				case 0:
					cpu_state.execute.valE = cpu_state.execute.valB + cpu_state.execute.valA;
					break;
				case 1:
					cpu_state.execute.valE = cpu_state.execute.valB - cpu_state.execute.valA;
					break;
				case 2:
					cpu_state.execute.valE = cpu_state.execute.valB & cpu_state.execute.valA;
					break;
				case 3:
					cpu_state.execute.valE = cpu_state.execute.valB ^ cpu_state.execute.valA;
					break;
			}

			cpu_state.execute.eflags.zf = cpu_state.execute.valE == 0;
			cpu_state.execute.eflags.sf = cpu_state.execute.valE >> 63;
			cpu_state.execute.eflags.of = cpu_state.execute.eflags.sf != (cpu_state.execute.valB >> 63);
			break;
		case 7:
			cpu_state.execute.cnd = check_flag(cpu_state.execute.instruction_data.ifun, cpu_state.execute.eflags.zf, cpu_state.execute.eflags.sf, cpu_state.execute.eflags.of);

			if (cpu_state.execute.cnd < 0)
				return -1;

			break;
		case 8:
			cpu_state.execute.valE = cpu_state.execute.valB - 4;
			break;
		case 9:
			cpu_state.execute.valE = cpu_state.execute.valB + 4;
			break;
		case 0xa:
			cpu_state.execute.valE = cpu_state.execute.valB - 4;
			break;
		case 0xb:
			cpu_state.execute.valE = cpu_state.execute.valB + 4;
			break;
	}

	return 0;
}

int memory(void) {
	switch (cpu_state.memory.instruction_data.icode) {
		case 0: case 1: case 2: case 3:
			break;
		case 4:
			write_ram(cpu_state.memory.valE, cpu_state.memory.valA);
			break;
		case 5:
			cpu_state.memory.valM = read_ram(cpu_state.memory.valE);
			break;
		case 6: case 7:
			break;
		case 8:
			write_ram(cpu_state.memory.valE, cpu_state.memory.instruction_data.valP);
			break;
		case 9:
			cpu_state.memory.valM = read_ram(cpu_state.memory.valA);
			break;
		case 0xa:
			write_ram(cpu_state.memory.valE, cpu_state.memory.valA);
			break;
		case 0xb:
			cpu_state.memory.valM = read_ram(cpu_state.memory.valB);
			break;
	}

	return 0;
}

int writeback(void) {
	switch (cpu_state.writeback.instruction_data.icode) {
		case 0:
			cpu_state.halted = 1;
			break;
		case 1:
			break;
		case 2: case 3:
			cpu_state.registers[cpu_state.writeback.instruction_data.regB] = cpu_state.writeback.valE;
			break;
		case 4:
			break;
		case 5:
			cpu_state.registers[cpu_state.writeback.instruction_data.regA] = cpu_state.writeback.valM;
			break;
		case 6:
			cpu_state.registers[cpu_state.writeback.instruction_data.regB] = cpu_state.writeback.valE;
			break;
		case 7:
			break;
		case 8: case 9: case 0xa:
			cpu_state.registers[4] = cpu_state.writeback.valE;
			break;
		case 0xb:
			cpu_state.registers[4] = cpu_state.writeback.valE;
			cpu_state.registers[cpu_state.writeback.instruction_data.regA] = cpu_state.writeback.valM;
			break;
	}

	return 0;
}
