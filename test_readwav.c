#include <stdio.h>
#include <assert.h>
#include "readwav.h"

int main(int argc, char **argv) {
	int rc;
	wav_sample_t * sample_buf;
	int sample_count;
       
	rc = readwav(argv[1], &sample_buf, &sample_count);
	assert(rc == 0);
	printf("sample count %d\n", sample_count);
	print_samples(sample_buf, sample_count);
}
