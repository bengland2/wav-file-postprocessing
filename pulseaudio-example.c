/* to compile:
 *   cc -Wall -g -D_REENTRANT -lpulse -pthread -lm -o pulseaudio-example pulseaudio-example.c
 * where part of this command came from:
 *   pkg-config --cflags --libs libpulse
 * to run:
 *   env var NICE_CHANGE lets you adjust priority, requires CAP_SYS_NICE capability
 *   to get this capability: sudo setcap "CAP_SYS_NICE+ep" pulseaudio-example
 *   then to run: NICE_CHANGE=-10 ./pulseaudio-example
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pulse/pulseaudio.h>
#include <math.h>
#include <sys/time.h>
#include <stdint.h>
#include "readwav.h"

/* process exit status */
#define OK 	0
#define NOTOK 	1

#define MICROSEC_PER_SEC 1000000
#define LATENCY_BUFFER_ELEMENTS 10000
#define SAMPLES_PER_SEC 44100
#define FLOAT_SAMPLES_PER_SEC 44100.0
#define MAX_VOLUME 9/10

static int usecs_per_report = 20000;
static int latency = 10000; // start latency in micro seconds
static wav_sample_t * sampledata;
static int sample_count;
static int channels;
static pa_buffer_attr bufattr;
static int underflows = 0;
static pa_sample_spec ss;
static pa_mainloop *pa_ml;

static pa_usec_t latencies[LATENCY_BUFFER_ELEMENTS] = {0};
static int latency_count = 0;
static int latency_count_multiple = 10;
static uint64_t last_time_latencies_reported = 0;

static const char *debug_env_var = "PA_DEBUG";
static int pa_debug = -1;

uint64_t get_time_usec(void) {
	struct timeval now;
	uint64_t time_usec;

	if (gettimeofday(&now, NULL)) {
		perror("gettimeofday");
		exit(NOTOK);
	}
	time_usec = (uint64_t )now.tv_sec * MICROSEC_PER_SEC + now.tv_usec;
	return time_usec;
}

void process_latencies(pa_usec_t next_latency) {
#ifdef DEBUG_LATENCIES
	float min_l, max_l, sum, sum_squares, avg, variance, stdev;
#endif
	uint64_t now_usec;

	if (latency_count >= LATENCY_BUFFER_ELEMENTS) {
		printf("ERROR: latency buffer overflow at %d elements\n", latency_count);
		exit(NOTOK);
	}
	if (next_latency > 1000000) {
		printf("latency sample %ld count %d > 1 sec, discarding\n", 
				next_latency, latency_count);
		return;
	}
	latencies[latency_count++] = next_latency;
	if (latency_count % latency_count_multiple != 0) return;
	now_usec = get_time_usec();
	if (now_usec - last_time_latencies_reported < usecs_per_report) return;

#ifdef DEBUG_LATENCIES
	min_l = 1e20; /* greater than any possible latency */
	max_l = -1.0; /* below any possible latency */
	sum = 0.0;
	sum_squares = 0.0;
	for (int k = 0; k < latency_count; k++) {
		pa_usec_t l = latencies[k];
		sum += l;
		sum_squares += (l*l);
		if (l < min_l) min_l = l;
		if (l > max_l) max_l = l;
	}
	avg = sum / latency_count;
	variance = sum_squares / latency_count;
	stdev = sqrt(variance);
	printf("time %lu latcount %d avg %f stdev %f min %f max %f\n", 
			now_usec, latency_count, avg, stdev, min_l, max_l);
#endif
	last_time_latencies_reported = now_usec;
	latency_count = 0;
}

// This callback gets called when our context changes state.  We really only
// care about when it's ready or if it has failed
void pa_state_cb(pa_context *c, void *userdata) {
  pa_context_state_t state;
  int *pa_ready = userdata;
  state = pa_context_get_state(c);
  switch  (state) {
    // These are just here for reference
  case PA_CONTEXT_UNCONNECTED:
  case PA_CONTEXT_CONNECTING:
  case PA_CONTEXT_AUTHORIZING:
  case PA_CONTEXT_SETTING_NAME:
  default:
    break;
  case PA_CONTEXT_FAILED:
  case PA_CONTEXT_TERMINATED:
    *pa_ready = 2;
    break;
  case PA_CONTEXT_READY:
    *pa_ready = 1;
    break;
  }
}

static void stream_request_cb(pa_stream *s, size_t length, void *userdata) {
  pa_usec_t usec;
  int neg;
  static int samples_consumed = 0;
  int samples_remaining = sample_count - samples_consumed;
  if (samples_remaining < length)
	  length = samples_remaining;
  pa_stream_get_latency(s,&usec,&neg);
  if (pa_debug) {
    printf("  latency %8d us\n",(int)usec);
    printf("samples_consumed %d samples_remaining %d length %lu\n", 
	   samples_consumed, samples_remaining, length);
  }
  pa_stream_write(s, &sampledata[samples_consumed], length*channels, NULL, 0LL, PA_SEEK_RELATIVE);
  samples_consumed += length;
  samples_remaining = sample_count - samples_consumed;
  if (samples_remaining == 0) {
	if (pa_debug) printf("no samples remaining\n");
	pa_mainloop_quit(pa_ml, 0);
  }
  process_latencies(usec);
}

static void stream_underflow_cb(pa_stream *s, void *userdata) {
  // We increase the latency by 50% if we get 6 underflows and latency is under 2s
  // This is very useful for over the network playback that can't handle low latencies
  printf("underflow\n");
  underflows++;
  if (underflows >= 6 && latency < 2000000) {
    latency = (latency*3)/2;
    bufattr.maxlength = pa_usec_to_bytes(latency,&ss);
    bufattr.tlength = pa_usec_to_bytes(latency,&ss);  
    pa_stream_set_buffer_attr(s, &bufattr, NULL, NULL);
    underflows = 0;
    printf("latency increased to %d\n", latency);
  }
}

int main(int argc, char *argv[]) {
  pa_mainloop_api *pa_mlapi;
  pa_context *pa_ctx;
  pa_stream *playstream;
  pa_context_state_t context_state;
  int r;
  int pa_ready = 0;
  int retval = 0;
  /* 
  unsigned int a;
  double amp; 
  */
  int nice_change;
  int rc;
  char * nice_change_str;
  float coeff = 5000;
  char * coeff_str;

  pa_debug = getenv(debug_env_var) != NULL;

  nice_change_str = getenv("NICE_CHANGE");
  if (nice_change_str) {
	  nice_change = atoi(nice_change_str);
	  printf("changing priority by %d\n", nice_change);
	  rc = nice(nice_change);
	  if (rc != OK) {
		  perror("nice");
		  exit(NOTOK);
	  }
  }

  coeff_str = getenv("COEFF");
  if (coeff_str) {
	  coeff = atof(coeff_str);
  }
  if (pa_debug) printf("frequency coefficient = %f\n", coeff);

  /* read in wave file into sample buffer */

  rc = wav_read(argv[1], &sampledata, &sample_count, &channels);

  /* post process it */

#if 0
  /* speed it up by factor of 2 */
  for (int k = 0; (k<<1) < sample_count ; k++) {
	  sampledata[k] = sampledata[k<<1];
  }
  sample_count /= 2;
#endif

#if 0
  /* slow it down by factor of 2 */
  wav_sample_t * old_sampledata = sampledata;
  sampledata = malloc(sizeof(wav_sample_t) * sample_count * 2 + 4);
  for (int k = 0; k < sample_count; k++) {
	sampledata[2*k] = old_sampledata[k];
  }
  if (sample_count > 0) {
  	sampledata[2*sample_count] = sampledata[2*sample_count - 2];
  	for (int j = 0; j < sample_count; j++) {
		/* interpolate linearly between the two values */
		sampledata[2*j+1] = (sampledata[2*j] + sampledata[2*j + 2]) / 2 ;
	}
  }
#endif

#if 0
  /* this used to insert a sinusoidal signal into the recording */
  for (int k = 0; k < sample_count ; k++) {
	  /* make room for additional signal */
	  sampledata[k] = (sampledata[k]*0.9);
	  /* insert weird sinusoidal thingy */
	  sampledata[k] += ((1<<8) * 
			    (cos((k/2)/FLOAT_SAMPLES_PER_SEC) * 
			     sin((k/7))));
	  assert(sampledata[k] < 1<<15);
	  //sampledata[k] *= MAX_VOLUME;
  }
#endif

  // Create a mainloop API and connection to the default server
  pa_ml = pa_mainloop_new();
  pa_mlapi = pa_mainloop_get_api(pa_ml);
  pa_ctx = pa_context_new(pa_mlapi, "Simple PA test application");
  pa_context_connect(pa_ctx, NULL, 0, NULL);

  // This function defines a callback so the server will tell us it's state.
  // Our callback will wait for the state to be ready.  The callback will
  // modify the variable to 1 so we know when we have a connection and it's
  // ready.
  // If there's an error, the callback will set pa_ready to 2
  pa_context_set_state_callback(pa_ctx, pa_state_cb, &pa_ready);

  context_state = pa_context_get_state(pa_ctx);
  if (pa_debug) printf("context state = %x\n", (unsigned )context_state);

  // We can't do anything until PA is ready, so just iterate the mainloop
  // and continue
  while (pa_ready == 0) {
    pa_mainloop_iterate(pa_ml, 1, NULL);
  }
  if (pa_ready == 2) {
    retval = -1;
    goto exit;
  }

  ss.rate = 44100;
  ss.channels = channels;
  ss.format = PA_SAMPLE_S16LE;
  playstream = pa_stream_new(pa_ctx, "Playback", &ss, NULL);
  if (!playstream) {
    printf("pa_stream_new failed\n");
    retval = -15;
    goto exit;
  }
  pa_stream_set_write_callback(playstream, stream_request_cb, NULL);
  pa_stream_set_underflow_callback(playstream, stream_underflow_cb, NULL);
  bufattr.fragsize = (uint32_t)-1;
  bufattr.maxlength = pa_usec_to_bytes(latency,&ss);
  bufattr.minreq = pa_usec_to_bytes(0,&ss);
  bufattr.prebuf = (uint32_t)-1;
  bufattr.tlength = pa_usec_to_bytes(latency,&ss);
  r = pa_stream_connect_playback(playstream, NULL, &bufattr,
                                 PA_STREAM_INTERPOLATE_TIMING
                                 |PA_STREAM_ADJUST_LATENCY
                                 |PA_STREAM_AUTO_TIMING_UPDATE, NULL, NULL);
  if (r < 0) {
    // Old pulse audio servers don't like the ADJUST_LATENCY flag, so retry without that
    r = pa_stream_connect_playback(playstream, NULL, &bufattr,
                                   PA_STREAM_INTERPOLATE_TIMING|
                                   PA_STREAM_AUTO_TIMING_UPDATE, NULL, NULL);
  }
  if (r < 0) {
    printf("pa_stream_connect_playback failed\n");
    retval = -1;
    goto exit;
  }

  // Run the mainloop until pa_mainloop_quit() is called
  // (this example never calls it, so the mainloop runs forever).
  pa_mainloop_run(pa_ml, NULL);

exit:
  // clean up and disconnect
  pa_context_disconnect(pa_ctx);
  pa_context_unref(pa_ctx);
  pa_mainloop_free(pa_ml);
  return retval;
}
