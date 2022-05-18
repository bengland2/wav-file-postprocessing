#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "wav_file_access.h"

static char out_path[1024];

int main(int argc, char **argv) {
	int rc;
	wav_sample_t * sample_buf;
	int sample_count;
        int chans;
	char *dot;

	rc = wav_read(argv[1], &sample_buf, &sample_count, &chans);
	assert(rc == 0);
	printf("sample count %d, channels %d\n", sample_count, chans);
	/* print_samples(sample_buf, sample_count); */

	strcpy(out_path, argv[1]);
	dot = rindex(out_path, '.');
	if (strcmp(dot, ".wav")) {
		printf("input file must end in .wav\n");
		exit(1);
	}
	strcpy(dot, "_test_write.wav");
	rc = wav_write(out_path, sample_buf, sample_count, chans);
	assert(rc == 0);
}
