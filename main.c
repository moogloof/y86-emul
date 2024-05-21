#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ram.h>
#include <cpu.h>
#define SIZE 256

void hexDump(size_t, void *, int);

extern char* ram_buffer;

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

	// Cycle the CPU pipeline
	int cycle_result = 0;
	while (cycle_result <= 0) {
		cycle_result = cycle();
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
		}
	}

	// Close the file
	fclose(myfile);

	return 0;
}
