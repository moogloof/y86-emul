#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ram.h>
#include <cpu.h>
#include <inttypes.h>
#define SIZE 256

void hexDump(size_t, void *, int);

extern char* ram_buffer;
extern cpu_state_t cpu_state;

char* register_name[16];

// Function to get the file size
size_t getFileSize(FILE *file) {
	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	fseek(file, 0, SEEK_SET);
	return size;
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s file\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	FILE *myfile = fopen(argv[1], "rb");
	if (myfile == 0) {
		fprintf(stderr, "%s: failed to open file '%s' for reading\n", argv[0], argv[1]);
		exit(EXIT_FAILURE);
	}

	// Get the size of the file
	size_t fileSize = getFileSize(myfile);

	// Read the file into the ram_buffer
	size_t bytesRead = fread(ram_buffer, 1, fileSize, myfile);
	if (bytesRead != fileSize) {
		fprintf(stderr, "Failed to read the entire file\n");
		fclose(myfile);
		exit(EXIT_FAILURE);
	}

	init_cpu();

	char* rax_name = "rax\0";
	char* rcx_name = "rcx\0";
	char* rdx_name = "rdx\0";
	char* rbx_name = "rbx\0";
	char* rsp_name = "rsp\0";
	char* rbp_name = "rbp\0";
	char* rsi_name = "rsi\0";
	char* rdi_name = "rdi\0";
	char* invalid_name = "invalid register\0";
	char* none_name = "no register\0";

	register_name[0] = rax_name;
	register_name[1] = rcx_name;
	register_name[2] = rdx_name;
	register_name[3] = rbx_name;
	register_name[4] = rsp_name;
	register_name[5] = rbp_name;
	register_name[6] = rsi_name;
	register_name[7] = rdi_name;
	for (int i = 8; i < 15; i++)
		register_name[i] = invalid_name;
	register_name[15] = none_name;

	// Cycle the CPU pipeline
	int cycle_result = 0;
	while (cycle_result <= 0) {
		cycle_result = cycle();
		printf("---------------------\r\n");
		printf("FETCH:\r\n");
		printf("    icode = %1x\r\n", cpu_state.fetch.instruction_data.icode);
		printf("    ifun  = %1x\r\n", cpu_state.fetch.instruction_data.ifun);
		printf("    rA    = %s\r\n", register_name[cpu_state.fetch.instruction_data.regA]);
		printf("    rB    = %s\r\n", register_name[cpu_state.fetch.instruction_data.regB]);
		printf("    valC  = %" PRIx64 "\r\n", cpu_state.fetch.instruction_data.valC);
		printf("    valP  = %" PRIx64 "\r\n", cpu_state.fetch.instruction_data.valP);
		printf("DECODE:\r\n");
		printf("    valA = %" PRIx64 "\r\n", cpu_state.decode.valA);
		printf("    valB = %" PRIx64 "\r\n", cpu_state.decode.valB);
		printf("    %%rsp = %" PRIx64 "\r\n", cpu_state.registers[4]);
		printf("EXECUTE:\r\n");
		printf("    valE = %" PRIx64 "\r\n", cpu_state.execute.valE);
		printf("    CC   = {ZF = %1x, SF = %1x, OF = %1x}\r\n", cpu_state.execute.eflags.zf, cpu_state.execute.eflags.sf, cpu_state.execute.eflags.of);
		printf("MEMORY:\r\n");
		printf("    valM = %" PRIx64 "\r\n", cpu_state.memory.valM);
		printf("WRITEBACK:\r\n");
		for (int i = 0; i < 8; i++)
			printf("    %s = %" PRIx64 "\r\n", register_name[i], cpu_state.registers[i]);
		printf("PC UPDATE:\r\n");
		printf("    newPC = %" PRIx64 "\r\n", cpu_state.pc);

		switch (cycle_result) {
			case -5:
				printf("EXCEPTION: Writeback error\r\n");
				break;
			case -4:
				printf("EXCEPTION: Memory access out of bounds (>0x%x)\r\n", RAM_SIZE);
				break;
			case -3:
				printf("EXCEPTION: Invalid instruction flag\r\n");
				break;
			case -2:
				printf("EXCEPTION: Invalid register\r\n");
				break;
			case -1:
				printf("EXCEPTION: Bad instruction code\r\n");
				break;
			case 1:
				printf("HALTED\r\n");
				break;
		}
	}

	// Close the file
	fclose(myfile);

	return 0;
}
