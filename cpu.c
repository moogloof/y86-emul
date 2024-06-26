#include <cpu.h>
#include <ram.h>
#include <string.h>
#include <stdio.h>

extern uint8_t ram_buffer[RAM_SIZE];

// CPU State
cpu_state_t cpu_state;
cpu_state_t old_cpu_state;
state bubble_state;

// Initialize CPU
void init_cpu(void) {
	memset(&cpu_state, 0, sizeof(cpu_state_t));
	memset(&old_cpu_state, 0, sizeof(cpu_state_t));

	// all nops at first
	cpu_state.fetch.instruction_data.icode = 1;
	cpu_state.decode.instruction_data.icode = 1;
	cpu_state.execute.instruction_data.icode = 1;
	cpu_state.memory.instruction_data.icode = 1;
	cpu_state.writeback.instruction_data.icode = 1;

	// Define bubble
	// Bubbles are automatically overwritten each time in fetch unless the thing is stalled
	memset(&bubble_state, 0, sizeof(state));
	bubble_state.instruction_data.icode = 1;
	bubble_state.instruction_data.regA = 0xf;
	bubble_state.instruction_data.regB = 0xf;
}

// Cycle through stages
int cycle(void) {
	// Handle branch misprediction
	if (cpu_state.branch_mispredict) {
		printf("CPU LOG :: Handling branch mispredict, bubbling fetch and decode, updating PC\r\n");
		cpu_state.branch_mispredict = 0;

		// Before bubbling, release used registers from decode
		if (cpu_state.decode.instruction_data.regA < 8)
			cpu_state.register_locks[cpu_state.decode.instruction_data.regA] = 0;
		if (cpu_state.decode.instruction_data.regB < 8)
			cpu_state.register_locks[cpu_state.decode.instruction_data.regB] = 0;

		// Bubble states
		cpu_state.fetch = bubble_state;
		cpu_state.decode = bubble_state;

		cpu_state.pc = cpu_state.execute.instruction_data.valP;
	}

	memcpy(&old_cpu_state, &cpu_state, sizeof(cpu_state_t));

	// Move the pipeline
	if (cpu_state.decode.stalling) {
		// Handling stalling at decode stage
		cpu_state.writeback = cpu_state.memory;
		cpu_state.memory = cpu_state.execute;
		cpu_state.execute = bubble_state;

		if (writeback() < 0)
			return -5;
		if (memory() < 0)
			return -4;
		if (execute() < 0)
			return -3;
		if (decode() < 0)
			return -2;
	} else if (cpu_state.fetch.stalling) {
		// Handling stalling at fetch stage
		cpu_state.writeback = cpu_state.memory;
		cpu_state.memory = cpu_state.execute;
		cpu_state.execute = cpu_state.decode;
		cpu_state.decode = bubble_state;

		if (writeback() < 0)
			return -5;
		if (memory() < 0)
			return -4;
		if (execute() < 0)
			return -3;
		if (decode() < 0)
			return -2;
	} else {
		cpu_state.writeback = cpu_state.memory;
		cpu_state.memory = cpu_state.execute;
		cpu_state.execute = cpu_state.decode;
		cpu_state.decode = cpu_state.fetch;

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
	}

	if (cpu_state.halted)
		return 1;

	return 0;
}

// Fetch
int fetch(void) {
	cpu_state.fetch.instruction_data.icode = ram_buffer[old_cpu_state.pc] >> 4;
	cpu_state.fetch.instruction_data.ifun = ram_buffer[old_cpu_state.pc] & 0xf;

	cpu_state.fetch.instruction_data.regA = 0xf;
	cpu_state.fetch.instruction_data.regB = 0xf;

	cpu_state.fetch.instruction_data.valP = old_cpu_state.pc;
	cpu_state.fetch.instruction_data.valC = 0;

	switch (cpu_state.fetch.instruction_data.icode) {
		case 0: case 1:
			cpu_state.fetch.instruction_data.valP += 1;
			cpu_state.pc = cpu_state.fetch.instruction_data.valP;
			break;
		case 2:
			cpu_state.fetch.instruction_data.regA = ram_buffer[old_cpu_state.pc + 1] >> 4;
			cpu_state.fetch.instruction_data.regB = ram_buffer[old_cpu_state.pc + 1] & 0xf;
			cpu_state.fetch.instruction_data.valP += 2;
			cpu_state.pc = cpu_state.fetch.instruction_data.valP;
			break;
		case 3:
			cpu_state.fetch.instruction_data.regA = ram_buffer[old_cpu_state.pc + 1] >> 4;
			cpu_state.fetch.instruction_data.regB = ram_buffer[old_cpu_state.pc + 1] & 0xf;
			cpu_state.fetch.instruction_data.valC = read_ram(old_cpu_state.pc + 2);
			cpu_state.fetch.instruction_data.valP += 10;
			cpu_state.pc = cpu_state.fetch.instruction_data.valP;
			if (cpu_state.fetch.instruction_data.regA != 0xf)
				return -1;
			break;
		case 4:
			cpu_state.fetch.instruction_data.regA = ram_buffer[old_cpu_state.pc + 1] >> 4;
			cpu_state.fetch.instruction_data.regB = ram_buffer[old_cpu_state.pc + 1] & 0xf;
			cpu_state.fetch.instruction_data.valC = read_ram(old_cpu_state.pc + 2);
			cpu_state.fetch.instruction_data.valP += 10;
			cpu_state.pc = cpu_state.fetch.instruction_data.valP;
			break;
		case 5:
			cpu_state.fetch.instruction_data.regA = ram_buffer[old_cpu_state.pc + 1] >> 4;
			cpu_state.fetch.instruction_data.regB = ram_buffer[old_cpu_state.pc + 1] & 0xf;
			cpu_state.fetch.instruction_data.valC = read_ram(old_cpu_state.pc + 2);
			cpu_state.fetch.instruction_data.valP += 10;
			cpu_state.pc = cpu_state.fetch.instruction_data.valP;
			break;
		case 6:
			cpu_state.fetch.instruction_data.regA = ram_buffer[old_cpu_state.pc + 1] >> 4;
			cpu_state.fetch.instruction_data.regB = ram_buffer[old_cpu_state.pc + 1] & 0xf;
			cpu_state.fetch.instruction_data.valP += 2;
			cpu_state.pc = cpu_state.fetch.instruction_data.valP;
			if (cpu_state.fetch.instruction_data.ifun > 3)
				return -1;
			break;
		case 7:
			cpu_state.fetch.instruction_data.valC = read_ram(old_cpu_state.pc + 1);
			cpu_state.fetch.instruction_data.valP += 9;
			cpu_state.pc = cpu_state.fetch.instruction_data.valC;
			break;
		case 8:
			cpu_state.fetch.instruction_data.valC = read_ram(old_cpu_state.pc + 1);
			cpu_state.fetch.instruction_data.valP += 9;
			cpu_state.pc = cpu_state.fetch.instruction_data.valC;
			break;
		case 9:
			cpu_state.fetch.instruction_data.valP += 1;
			cpu_state.pc = cpu_state.fetch.instruction_data.valP;
			break;
		case 0xa: case 0xb:
			cpu_state.fetch.instruction_data.regA = ram_buffer[old_cpu_state.pc + 1] >> 4;
			cpu_state.fetch.instruction_data.regB = ram_buffer[old_cpu_state.pc + 1] & 0xf;
			cpu_state.fetch.instruction_data.valP += 2;
			cpu_state.pc = cpu_state.fetch.instruction_data.valP;
			if (cpu_state.fetch.instruction_data.regB != 0xf)
				return -1;
			break;
		default:
			return -1;
	}

	return 0;
}

// Decode
int decode(void) {
	switch (cpu_state.decode.instruction_data.icode) {
		case 8:
			cpu_state.decode.instruction_data.regB = 4;
			break;
		case 9:
			printf("CPU LOG :: Ret instruction detected, bubbling and stalling fetch\r\n");
			cpu_state.decode.instruction_data.regA = 4;
			cpu_state.decode.instruction_data.regB = 4;
			cpu_state.fetch = bubble_state;
			cpu_state.fetch.stalling = 1;
			break;
		case 0xa: case 0xb:
			cpu_state.decode.instruction_data.regB = 4;
			break;
	}

	if (cpu_state.decode.instruction_data.regA != 0xf) {
		if (cpu_state.decode.instruction_data.regA < 8) {
			if (old_cpu_state.register_locks[cpu_state.decode.instruction_data.regA]) {
				cpu_state.decode.stalling = 1;

				printf("CPU LOG :: Stalling at decode stage\r\n");

				return 1;
			} else {
				cpu_state.decode.valA = old_cpu_state.registers[cpu_state.decode.instruction_data.regA];

				if (cpu_state.decode.stalling) {
					cpu_state.decode.stalling = 0;
					printf("CPU LOG :: Unstalling decode stage\r\n");
				}
			}
		} else {
			return -1;
		}
	}

	if (cpu_state.decode.instruction_data.regB != 0xf) {
		if (cpu_state.decode.instruction_data.regB < 8) {
			if (old_cpu_state.register_locks[cpu_state.decode.instruction_data.regB]) {
				cpu_state.decode.stalling = 1;

				printf("CPU LOG :: Stalling at decode stage\r\n");

				return 1;
			} else {
				cpu_state.decode.valB = old_cpu_state.registers[cpu_state.decode.instruction_data.regB];

				if (cpu_state.decode.stalling) {
					cpu_state.decode.stalling = 0;
					printf("CPU LOG :: Unstalling decode stage\r\n");
				}
			}
		} else {
			return -1;
		}
	}

	switch (cpu_state.decode.instruction_data.icode) {
		case 2: case 3:
			cpu_state.register_locks[cpu_state.decode.instruction_data.regB] = 1;
			break;
		case 5:
			cpu_state.register_locks[cpu_state.decode.instruction_data.regA] = 1;
			break;
		case 6:
			cpu_state.register_locks[cpu_state.decode.instruction_data.regB] = 1;
			break;
		case 8: case 9: case 0xa: case 0xb:
			cpu_state.register_locks[4] = 1;
			break;
	}

	return 0;
}

// Helper for the flags
static int check_flag(uint8_t flag, uint8_t cpu_zf, uint8_t cpu_sf, uint8_t cpu_of) {
	// Check for flags
	switch (flag) {
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
			do_write = check_flag(cpu_state.execute.instruction_data.ifun, cpu_state.eflags.zf, cpu_state.eflags.sf, cpu_state.eflags.of);

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

			cpu_state.eflags.zf = cpu_state.execute.valE == 0;
			cpu_state.eflags.sf = cpu_state.execute.valE >> 63;
			cpu_state.eflags.of = cpu_state.eflags.sf != (cpu_state.execute.valB >> 63);
			break;
		case 7:
			cpu_state.execute.cnd = check_flag(cpu_state.execute.instruction_data.ifun, cpu_state.eflags.zf, cpu_state.eflags.sf, cpu_state.eflags.of);

			if (cpu_state.execute.cnd < 0)
				return -1;
			else if (!cpu_state.execute.cnd) {
				cpu_state.branch_mispredict = 1;
				printf("CPU LOG :: Branch mispredict detected, will be handled in next cycle\r\n");
			}

			break;
		case 8:
			cpu_state.execute.valE = cpu_state.execute.valB - 8;
			break;
		case 9:
			cpu_state.execute.valE = cpu_state.execute.valB + 8;
			break;
		case 0xa:
			cpu_state.execute.valE = cpu_state.execute.valB - 8;
			break;
		case 0xb:
			cpu_state.execute.valE = cpu_state.execute.valB + 8;
			break;
	}

	return 0;
}

int memory(void) {
	switch (cpu_state.memory.instruction_data.icode) {
		case 0: case 1: case 2: case 3:
			break;
		case 4:
			if (cpu_state.memory.valE > RAM_SIZE - 8)
				return -1;
			write_ram(cpu_state.memory.valE, cpu_state.memory.valA);
			break;
		case 5:
			if (cpu_state.memory.valE > RAM_SIZE - 8)
				return -1;
			cpu_state.memory.valM = read_ram(cpu_state.memory.valE);
			break;
		case 6: case 7:
			break;
		case 8:
			if (cpu_state.memory.valE > RAM_SIZE - 8)
				return -1;
			write_ram(cpu_state.memory.valE, cpu_state.memory.instruction_data.valP);
			break;
		case 9:
			if (cpu_state.memory.valA > RAM_SIZE - 8)
				return -1;
			printf("CPU LOG :: Successfully ran ret, updating PC, unstalling fetch\r\n");
			cpu_state.memory.valM = read_ram(cpu_state.memory.valA);
			cpu_state.pc = cpu_state.memory.valM;
			cpu_state.fetch.stalling = 0;
			break;
		case 0xa:
			if (cpu_state.memory.valE > RAM_SIZE - 8)
				return -1;
			write_ram(cpu_state.memory.valE, cpu_state.memory.valA);
			break;
		case 0xb:
			if (cpu_state.memory.valB > RAM_SIZE - 8)
				return -1;
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
			cpu_state.register_locks[cpu_state.writeback.instruction_data.regB] = 0;
			break;
		case 4:
			break;
		case 5:
			cpu_state.registers[cpu_state.writeback.instruction_data.regA] = cpu_state.writeback.valM;
			cpu_state.register_locks[cpu_state.writeback.instruction_data.regA] = 0;
			break;
		case 6:
			cpu_state.registers[cpu_state.writeback.instruction_data.regB] = cpu_state.writeback.valE;
			cpu_state.register_locks[cpu_state.writeback.instruction_data.regB] = 0;
			break;
		case 7:
			break;
		case 8: case 9:
			cpu_state.registers[4] = cpu_state.writeback.valE;
			cpu_state.register_locks[4] = 0;
			break;
		case 0xa:
			cpu_state.registers[4] = cpu_state.writeback.valE;
			cpu_state.register_locks[4] = 0;
			break;
		case 0xb:
			cpu_state.registers[4] = cpu_state.writeback.valE;
			cpu_state.registers[cpu_state.writeback.instruction_data.regA] = cpu_state.writeback.valM;
			cpu_state.register_locks[4] = 0;
			break;
	}

	return 0;
}
