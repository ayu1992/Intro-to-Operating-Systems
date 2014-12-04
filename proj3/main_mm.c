#include "mm.h"

/* Main driver that tests the
 * clock time for our dynamic
 * memory allocator */
int main (int argc, char **argv)
{
	int i, j;
	struct timeval times, timee;
	mm_t MM;
	void *chunk = NULL;

	
	
	if (mm_init(&MM,NUM_CHUNKS, CHUNK_SIZE) < 0)
		fprintf(stderr, "Error in mm_init\n");
	j = gettimeofday (&times, (void *)NULL);
	for (i=0; i< ITERS; i++) { 
		chunk = mm_get(&MM);
		mm_put(&MM,chunk);
	}
	
	
	
	j = gettimeofday (&timee, (void *)NULL);
	mm_release(&MM);			/* release stuff on heap*/
	fprintf (stderr, "MM time took %f msec\n",comp_time (times, timee)/1000.0);
	return 0;
}
