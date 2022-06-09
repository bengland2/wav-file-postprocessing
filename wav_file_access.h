#ifndef _test_wav_access_h_
# define _test_wav_access_h_ 1

#include <stdint.h>

#ifndef PI
#define PI 3.14159
#endif

/* process exit status */
#define OK 	0
#define NOTOK 	1

#define MICROSEC_PER_SEC 1000000
#define SAMPLES_PER_SEC 44100
#define FLOAT_SAMPLES_PER_SEC 44100.0
//#define MAX_VOLUME 9/10
#define BYTES_PER_SAMPLE 2
#define MAX_VOLUME (1<<15)

/* this function only supports 16-bit PCM samples at this time */
typedef int16_t wav_sample_t;

/*
 * input:
 *   wav_filename_p - pathname of .wav file to read and return samples from
 * output:
 *   sample_buf_out - returns pointer to sample buffer (uint16_t *)
 *   sample_count_out - returns number of samples in buffer
 *   channels_out - number of sound channels (1 or 2)
 * returns:
 *   0 - if samples were read and returned
 *   non-0 otherwise
 *
 * caller must free sample_buf_out after using
 * subroutine will exit with non-zero status if any error is encountered
 */
int wav_read(char * wav_filename_p, wav_sample_t **sample_buf_out, int *sample_count_out, int *channels_out);

/*
 * input:
 *  sample_buf - array of sample values
 *  sample_count - number of sample values in array
 */
void print_samples(wav_sample_t * sample_buf, int sample_count);

/*
 * input:
 *   wav_filename_p - write .wav file to this path
 *   sample_buf_in - array of sample values
 *   sample_count - how many samples in each channel
 *   channels - how many channels 
 * returns 0 if successful, non-0 otherwise
 */
int wav_write(char * wav_filename_p, wav_sample_t *sample_buf_in, int sample_count, int channels);

#endif
