#include "mm.h"
/* Main driver that tests the
 * clock time for the built-in
 * malloc and free functions */
int main (int argc, char **argv)
{
	int i, j;
	struct timeval times, timee;
	void* b;
	j = gettimeofday (&times, (void *)NULL);
	for (i = 0; i < ITERS; i++) {
		b = (void*)malloc (CHUNK_SIZE);
		free (b);
	}
	j = gettimeofday (&timee, (void *)NULL);
	fprintf (stderr, "MALLOC/FREE time took %f msec\n",
		comp_time (times, timee)/1000.0);
	return 0;
}
