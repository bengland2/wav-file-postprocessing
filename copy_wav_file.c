/* copy .wav file from specified source to destination */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "wav_file_access.h"


int main(int argc, char **argv) {
	int rc;
	wav_sample_t * sample_buf;
	int sample_count;
        int chans;

	if (argc < 3) {
		printf("usage: copy_wav_file file1.wav file2.wav\n");
		exit(NOTOK);
	}
	rc = wav_read(argv[1], &sample_buf, &sample_count, &chans);
	if (rc) return rc;
	printf("sample count %d, channels %d\n", sample_count, chans);
	/* print_samples(sample_buf, sample_count); */

	rc = wav_write(argv[2], sample_buf, sample_count, chans);
	return rc;
}
