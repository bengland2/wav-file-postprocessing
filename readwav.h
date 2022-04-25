#ifndef _readwav_h_
# define _readwav_h_ 1

#include <stdint.h>

/* this function only supports 16-bit PCM samples at this time */
typedef int16_t wav_sample_t;

/*
 * input:
 *   wav_filename_p - pathname of .wav file to read and return samples from
 * output:
 *   sample_buf_out - returns pointer to sample buffer (uint16_t *)
 *   sample_count_out - returns number of samples in buffer
 * returns:
 *   0 - if samples were read and returned
 * caller must free sample_buf_out after using
 * subroutine will exit with non-zero status if any error is encountered
 */
int readwav(char * wav_filename_p, wav_sample_t **sample_buf_out, int *sample_count_out);

/*
 * input:
 *  sample_buf - array of sample values
 *  sample_count - number of sample values in array
 */
void print_samples(wav_sample_t * sample_buf, int sample_count);

#endif
