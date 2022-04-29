/* parse canonical WAVE format */
/* to compile: cc -Wall -g -o readwav readwav.c */
/* see Canon.html for wav format details */
/* originally from http://www.lightlink.com/tjweber/StripWav/Canon.html */
/* also see http://www.mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html */
/* also see https://stackoverflow.com/questions/63929283/what-is-a-list-chunk-in-a-riff-wav-header */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "readwav.h"

#define OK 	0
#define NOTOK 	1

#define WAV_HEADER_BUFFER_SIZE (1<<10)
#define WAV_SAMPLES_PER_SEC 44100
#define STRUCT_ID_LEN 4
#define BYTES_PER_SAMPLE 2
#define MAX_PATHNAME_LEN 1024
#define BLOCK_ALIGNMENT 4

static const int cols_per_row = 16;

struct wav_header {
	char		wh_riffstr[STRUCT_ID_LEN]; /* should be what's in riffstr below */
	uint32_t	wh_file_length;
	char		wh_wavestr[STRUCT_ID_LEN]; /* should be what's in wavestr below */
};
static char * riffstr = "RIFF";
static char * wavestr = "WAVE";


struct wav_fmt {
	char		wf_fmtstr[STRUCT_ID_LEN]; /* should be what's in fmtstr below */
	uint32_t	wf_len;
	uint16_t	wf_fmt;
	uint16_t	wf_channels;
	uint32_t	wf_samples_per_sec;
	uint32_t	wf_bytes_per_sec;
	uint16_t	wf_block_align;
	uint16_t	wf_bits_per_sample;
};
static char * fmtstr = "fmt ";

#if 0
/* in some .wav formats, this appears after the wav_fmt struct
 * The wf_len field tells you whether this will happen or not
 */

struct wav_fmt_extension {
	uint16_t	wfe_extension_size;
	uint16_t	wfe_number_valid_bits;
	uint32_t	wfe_speaker_position_mask;
	uint16_t	wfe_wave_format_code;
#define                  WAVE_FORMAT_PCM 		0x0001
#define                  WAVE_FORMAT_IEEE_FLOAT 	0x0003
#define                  WAVE_FORMAT_ALAW		0x0006
#define                  WAVE_FORMAT_MULAW		0x0007
#define                  WAVE_FORMAT_EXTENSIBLE		0xFFFE
	uint8_t         wfe_guid[14];
};
#endif


/* this structure follows the wav_fmt(_extension) structure */

struct wav_fact {
	char		wfct_factstr[STRUCT_ID_LEN]; /* should be what's in factstr below */
	uint32_t	wfct_chunk_size;
	uint32_t	wfct_number_samples;
};
static char * factstr = "fact";

struct wav_data {
	char		wd_datastr[STRUCT_ID_LEN]; /* should be what's in datastr below */
	uint32_t	wd_chunk_size;
};
static char * datastr = "data";

struct wav_list {
	char		wl_liststr[STRUCT_ID_LEN]; /* should be what's in liststr below */
	uint32_t 	wl_chunk_size;		   /* how to skip over it */
	char		wl_list_type_id[STRUCT_ID_LEN];  /* often this is INFO */
	/* what follows is dependent on the contents of list_type_id */
};
static char * liststr = "LIST";
static char * infostr = "INFO";

void usage(char * msg) {
	printf("ERROR: %s\n", msg);
	printf("usage: readwav myfile.wav\n");
	exit(NOTOK);
}

void syscall_error(char * msg) {
	printf("ERROR: %s: %s\n", msg, strerror(errno));
	exit(NOTOK);
}


static int not_match_str(char * actual_4byte_str, char * expected_4byte_str) {
	return strncmp(actual_4byte_str, expected_4byte_str, STRUCT_ID_LEN);
}

/* caller is responsible for freeing the buffer allocated and returned in sample_buf_out */

int readwav(char * wav_filename_p, wav_sample_t **sample_buf_out, int *sample_count_out)
{
	int fd;
	int count;
	/* int chunk = 0; */
	/* uint64_t offset = 0; */
	int file_len;
	int extension_size;
	struct wav_header *wh_p;
	struct wav_fmt *wf_p;
	struct wav_list *wl_p;
	struct wav_data *wd_p;
	/* struct wav_fmt_extension *wfe_p; */
	struct wav_fact *wfct_p;
	unsigned char * buf;
	struct stat st;
	uint32_t sample_buf_size;
	off_t file_offset, off;
	uint32_t number_samples;

	/* initialize outputs */

	*sample_buf_out = (wav_sample_t * )NULL;
	*sample_count_out = 0;

	/* allocate buffer to read headers */

	buf = (unsigned char * )malloc(WAV_HEADER_BUFFER_SIZE);
	if (!buf)
		syscall_error("malloc");

	/* open file and read header */

	fd = open(wav_filename_p, O_RDONLY);
	if (fd < 0)
		syscall_error(wav_filename_p);
	count = read(fd, buf, WAV_HEADER_BUFFER_SIZE);
	printf("read %d bytes from file %s\n", count, wav_filename_p);
	if (count < 0)
		syscall_error(".wav header");
	if (count < (sizeof(struct wav_header) + sizeof(struct wav_fmt) + sizeof(struct wav_fact)))
		usage(".wav file too short");

	/* parse RIFF header */

	wh_p = (struct wav_header * )buf;
	if (not_match_str(riffstr, wh_p->wh_riffstr) || not_match_str(wavestr, wh_p->wh_wavestr))
		usage("invalid WAV file");
	file_len = wh_p->wh_file_length;
	if (fstat(fd, &st))
		syscall_error("stat");
	if (st.st_size - 8 != file_len)
		usage("file length error");
	printf("file length = %u bytes\n", file_len);

	/* parse format structure */

	wf_p = (struct wav_fmt * )((char * )wh_p + sizeof(struct wav_header));
	if (not_match_str(fmtstr, wf_p->wf_fmtstr))
		usage("invalid wav_fmt chunk");
	printf(
	 "wf_len=%u wf_fmt=%u wf_channels=%u wf_samples_per_sec=%u wf_bytes_per_sec=%u wf_block_align=%u wf_bits_per_sample=%u\n",
	 wf_p->wf_len, wf_p->wf_fmt, wf_p->wf_channels, 
	 wf_p->wf_samples_per_sec, wf_p->wf_bytes_per_sec, wf_p->wf_block_align, wf_p->wf_bits_per_sample);
	if (wf_p->wf_bits_per_sample != 16)
		usage("only support 16 bits per sample at this time");
	if (wf_p->wf_channels < 1 || wf_p->wf_channels > 2) usage("only 1 or 2 channels supported ");
	if (wf_p->wf_samples_per_sec != WAV_SAMPLES_PER_SEC) usage("only fixed samples per sec supported ");
	if (wf_p->wf_bits_per_sample != 16) usage("only 16 bits per sample supported");
	if (wf_p->wf_bytes_per_sec != WAV_SAMPLES_PER_SEC * BYTES_PER_SAMPLE * wf_p->wf_channels)
		usage("inconsistent bytes per second");
	if (wf_p->wf_block_align != BLOCK_ALIGNMENT) usage("expect block alignment of 4 bytes");

	extension_size = sizeof(wf_p->wf_fmtstr) + sizeof(wf_p->wf_len) + wf_p->wf_len;
	if (wf_p->wf_len == 16) {
		;
	} else if (wf_p->wf_len == 18) {
		;
	} else if (wf_p->wf_len == 40) {
		usage("unsupported format extension block");
#if 0
		extension_size = 22;
		if (wfe_p->wfe_extension_size != extension_size)
			usage("extension size not 22 bytes");
		printf("extension number_valid_bits=%u speaker_position_mask=%x wave_format_code=%x ", 
			wfe_p->wfe_number_valid_bits, wfe_p->wfe_speaker_position_mask, wfe_p->wfe_wave_format_code);
		printf("guid: ");
		for (int k = 2; k < 16; k++) {
			printf("%02x", wfe_p->wfe_guid[k]);
		printf("\n");
		}
#endif 
	} else {
		usage("invalid wf_len");
	}
	printf("extension size %d\n", extension_size);

	/* parse LIST chunk if there is one */

	wl_p = (struct wav_list * )((char * )wf_p + extension_size);
	if (!not_match_str(liststr, wl_p->wl_liststr)) {
		char extract_4byte_id[STRUCT_ID_LEN+1] = {0};
		strncpy(extract_4byte_id, wl_p->wl_list_type_id, STRUCT_ID_LEN);
		printf("LIST structure found with length %u header %4s\n", wl_p->wl_chunk_size, extract_4byte_id);
		if (not_match_str(infostr, wl_p->wl_list_type_id)) {
			printf("unrecognized LIST type found\n");
		}
		wfct_p = (struct wav_fact * )((char * )wl_p + sizeof(wl_p->wl_chunk_size) + sizeof(wl_p->wl_list_type_id) + wl_p->wl_chunk_size);
	} else {
		wfct_p = (struct wav_fact * )((char * )wf_p + extension_size);
	}

	/* parse optional fact chunk */

	if (!not_match_str(factstr, wfct_p->wfct_factstr)) {
		if (wfct_p->wfct_chunk_size != 4)
			usage("fact structure length is not 4");
		printf("fact samples/chan = %u\n", wfct_p->wfct_number_samples);
		wd_p = (struct wav_data * )((char * )wfct_p + sizeof(wfct_p->wfct_factstr) + sizeof(wfct_p->wfct_chunk_size) + wfct_p->wfct_chunk_size);
	} else {
		wd_p = (struct wav_data * )wfct_p;
		wfct_p = NULL;
	}

	/* parse data chunk */

	if (not_match_str(datastr, wd_p->wd_datastr))
		usage("invalid wav_data structure");
	printf("data chunk size=%u\n", wd_p->wd_chunk_size);

	/* read samples */

	if (wfct_p) {
		number_samples = wfct_p->wfct_number_samples;
	} else {
		number_samples = wd_p->wd_chunk_size / BYTES_PER_SAMPLE;
	}
	sample_buf_size = sizeof(wav_sample_t) * number_samples;
	printf("allocating sample buf of %u bytes\n", sample_buf_size);
	wav_sample_t * sample_buf = (wav_sample_t * )malloc(sample_buf_size);
	if (!sample_buf)
		usage("could not allocate sample buf");
	memset(sample_buf, 0, sample_buf_size);
	file_offset = (off_t )((unsigned char * )wd_p + sizeof(struct wav_data) - buf);
	printf("file offset = %lu\n", file_offset);
	off = lseek(fd, file_offset, SEEK_SET);
	if (off != file_offset)
		syscall_error("could not set file offset for sample read");
	count = read(fd, (void * )sample_buf, sample_buf_size);
	printf("read %d bytes of sample data\n", count);
	if (count < 0)
		syscall_error("could not read samples");
	if (count < sample_buf_size)
		usage("could not read all sample data");

	/* return sample buf and sample count */

	*sample_count_out = number_samples;
	*sample_buf_out = sample_buf;

	return OK;
}

/* print out samples in a column-aligned decimal format */

void print_samples(wav_sample_t * sample_buf, int sample_count)
{
	for (int sample = 0; sample < sample_count; sample++) {
		printf("%6d ", sample_buf[sample]);
		if ((sample+1) % cols_per_row == 0)
			printf("\n");
	}
}

/** NOT DEBUGGED, DO NOT USE YET **/

int writewav(char * wav_filename_p, wav_sample_t *sample_buf_in, int sample_count)
{
	char temp_file_path[MAX_PATHNAME_LEN] = { 0 };
	struct wav_header wh;
	struct wav_fmt wf;
	struct wav_data wd;
	/* unused at present : struct wav_fmt_extension wfe; */
	struct wav_fact wfct;
	int fd;
	int rc;

	/* construct temp file name in same directory, write to that, rename at end */

	strcpy(temp_file_path, wav_filename_p);
	strcat(temp_file_path, ".tmp");
	fd = open(temp_file_path, O_WRONLY|O_EXCL);

	/* initialize .wav header and write it */

	strncpy(wh.wh_riffstr, riffstr, STRUCT_ID_LEN);
	strncpy(wh.wh_wavestr, wavestr, STRUCT_ID_LEN);
	wh.wh_file_length = sizeof(wh) + sizeof(wf) + sizeof(wfct) + sizeof(wd) + (sample_count * BYTES_PER_SAMPLE);
	rc = write(fd, (uint8_t * )&wh, sizeof(wh));
	if (rc < 0) syscall_error("could not write wav header");
	if (rc != sizeof(wh)) usage("could not write complete header");

	/* initialize format header and write it */

	strncpy(wf.wf_fmtstr, fmtstr, STRUCT_ID_LEN);
	wf.wf_len = 16;
	wf.wf_fmt = 1;  /* FIXME */
	wf.wf_channels = 2;
	wf.wf_samples_per_sec = WAV_SAMPLES_PER_SEC;
	wf.wf_bytes_per_sec = BYTES_PER_SAMPLE * wf.wf_samples_per_sec * wf.wf_channels;
	wf.wf_block_align = 4;
	wf.wf_bits_per_sample = 16;
	rc = write(fd ,(uint8_t * )&wf, sizeof(wf));
	if (rc < 0) syscall_error("could not write wav format");
	if (rc != sizeof(wf)) usage("could not write complete format");

	/* initialize fact struct and write it */

	strncpy(wfct.wfct_factstr, factstr, STRUCT_ID_LEN);
	wfct.wfct_chunk_size = 4;
	wfct.wfct_number_samples = sample_count;
	rc = write(fd ,(uint8_t * )&wfct, sizeof(wfct));
	if (rc < 0) syscall_error("could not write fact struct");
	if (rc != sizeof(wfct)) usage("could not write complete fact struct");

	/* initialize data struct and write it */

	strncpy(wd.wd_datastr, datastr, STRUCT_ID_LEN);
	wd.wd_chunk_size = sample_count * wf.wf_channels * BYTES_PER_SAMPLE;
	rc = write(fd ,(uint8_t * )&wd, sizeof(wd));
	if (rc < 0) syscall_error("could not write data header struct");
	if (rc != sizeof(wd)) usage("could not write complete data header struct");

	/* write the samples */

	rc = write(fd ,(uint8_t * )sample_buf_in, wd.wd_chunk_size);
	if (rc < 0) syscall_error("could not write sample data");
	if (rc < wd.wd_chunk_size) usage("could not write complete sample data");

	/* close and rename file */

	rc = close(fd);
	if (rc < 0) syscall_error("close written file");
	rc = rename(temp_file_path, wav_filename_p);
	if (rc < 0) syscall_error("rename to final filename");

	return OK;
}

