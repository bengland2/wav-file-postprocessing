/* library of functions to apply effects to music */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include "wav_file_access.h"

static void usage(const char * msg)
{
	printf("ERROR: %s\n", msg);
	printf("usage: wav_transform.c -f freq -m modulating-freq -l left-right -a fractional-amplitude\n");
	printf("left-right must be from -1 to 1 (default 0 means both channels)\n");
	printf("fractional amplitude is from 0 to 1.0\n\n");
	exit(NOTOK);
}

void check_range(const char * range_name, float val, float lowbound, float upbound)
{
	if (val < lowbound || val > upbound) {
		printf("ERROR: var %s val %f not in [ %f, %f ]\n",
				range_name, val, lowbound, upbound);
		exit(NOTOK);
	}
}

/* insert sinusoid ripple of given frequency with modulating frequency */

void wav_xform_sine_ripple( 
		wav_sample_t * sample_data_in, 
		int sample_count, 
		int channels, 
		float left_right, 
		float fractional_amplitude, 
		float freq, 
		float modulating_freq )
{
	const double PIover2 = PI / 2.0;
	const double twoPI = PI * 2.0;
	double single_chan_amplitude = 1.0;
	double two_channel_amplitudes[2] = { 1.0, 0.0 }; /* default is all left channel */

	double freq_radians = freq / twoPI;
	double modulating_freq_radians = modulating_freq / twoPI;

	check_range("left_right", left_right, -1., 1.);
	check_range("fractional_amplitude", fractional_amplitude, 0., 1.);
	check_range("freq", freq, 40., 15000.);
	check_range("modulating_freq", modulating_freq, 0.1, 10000.);

	/* pre-compute effect of left-right parameter */

	double * channel_amplitudes;
	if (channels == 1)
		channel_amplitudes = &single_chan_amplitude;
	else if (channels == 2) {
		channel_amplitudes = &two_channel_amplitudes[0];
		double left_right_radians = ((left_right + 1.0) / 2.0) * PIover2;
		channel_amplitudes[0] = cos(left_right_radians);
		channel_amplitudes[1] = sin(left_right_radians);
		printf("left amplitude = %f, right amplitude = %f\n",
				channel_amplitudes[0], channel_amplitudes[1]);
	} else
		usage("only 1 or 2 channels allowed");

	for (int k = 0; k < sample_count ; k++) {
	  int sample_chan = k % channels;
	  /* convert array index into time */
	  double sample_time = k / FLOAT_SAMPLES_PER_SEC;
	  double old_sample = sample_data_in[k];
	  /* insert weird sinusoidal thingy */
	  double new_signal = MAX_VOLUME * fractional_amplitude * 
			     cos(sample_time * freq_radians) * 
			     cos(sample_time * modulating_freq_radians) *
			     channel_amplitudes[sample_chan];
	  /* make room for additional signal */
	  double attenuated_old_sample = old_sample * (1.0 - fractional_amplitude);
	  double generated_sample = attenuated_old_sample + new_signal;
	  int generated_int_sample = (generated_sample * 0.9999);
	  if (abs(generated_int_sample) > MAX_VOLUME) {
		  printf("ERROR: volume maximum exceeded at sample %d with old vol %lf new vol %lf\n",
				  k, old_sample, generated_sample);
		  exit(NOTOK);
	  }
	  sample_data_in[k] = generated_int_sample;
	}
}

int main(int argc, char **argv)
{
	int rc;
	char * input_wav_filename = NULL;
	char * output_wav_filename = NULL;
	wav_sample_t * sample_buf = NULL;
	int sample_count = 0;
        int chans = 0;
	float freq = 440.;
	float modulating_freq = 1. ;
	float fractional_amplitude = 0.2;
	float left_right = 0.0;
	int opt;

	opterr = 0;
	while ((opt = getopt (argc, argv, "f:m:l:a:")) != -1)
	{
	  switch (opt)
	  {
	    case 'f':
		freq = atof(optarg);
		break;
	    case 'm':
		modulating_freq = atof(optarg);
		break;
	    case 'l':
		left_right = atof(optarg);
		break;
	    case 'a':
		fractional_amplitude = atof(optarg);
		break;
	    case '?':
        	if (optopt == 'c')
          		printf("Option -%c requires an argument.\n", optopt);
        	else if (isprint (optopt))
          		printf("Unknown option `-%c'.\n", optopt);
        	else
          		printf("Unknown option character `\\x%x'.\n", optopt);
		usage("option parse error");
	  };
	}
	if (optind != argc - 2) 
		usage("input and output .wav filename must be supplied");

	input_wav_filename = argv[optind];
	printf("%s is .wav file to transform\n", input_wav_filename);
	output_wav_filename = argv[optind+1];
	printf("%s is output .wav file \n", output_wav_filename);

	printf("%9.2f = frequency\n%9.2f = modulating frequency\n%9.2f = fractional amplitude\n%9.2f = left-right direction\n",
		freq, modulating_freq, fractional_amplitude, left_right);
	if (chans == 1 && left_right != 0.0)
		usage("cannot specify left-right direction with only 1 channel");

	/* read the wav file */

	rc = wav_read(input_wav_filename, &sample_buf, &sample_count, &chans);
	if (rc) return rc;
	printf("sample count %d, channels %d\n", sample_count, chans);
	/* print_samples(sample_buf, sample_count); */

	/* transform the wav file */

	wav_xform_sine_ripple( sample_buf, sample_count, chans, 
		left_right, fractional_amplitude, freq, modulating_freq );
	
	/* write out the resultig wav file */

	rc = wav_write(output_wav_filename, sample_buf, sample_count, chans);
	return rc;
}
