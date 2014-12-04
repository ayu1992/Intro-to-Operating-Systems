#ifndef __MM_H
#define __MM_H

#include <sys/time.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define INTERVAL 0

#define CHUNK_SIZE 64
#define NUM_CHUNKS 1000000
#define ITERS  1000000

typedef struct {
	void* memory;				// a preallocated continuous block of memory
	bool* chunkAvailable; 		// a boolean array that indicate if the chunks are available
	int chunkSize;				// size of a chunk
	int numChunks;				// number of chunks
	int* stackOfUnused;			// a stack of indexes that are unused
	int stacksize;				// size of the stack
} mm_t;

double comp_time(struct timeval time_s, struct timeval time_e);
int mm_init(mm_t *mm, int num_chunks, int chunk_size);
void *mm_get(mm_t *mm);
void mm_put(mm_t *mm, void *chunk);
void mm_release(mm_t *mm);

#endif
