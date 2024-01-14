/**************************************************************
*  FILENAME:     wave-export.h
*  DESCRIPTION:  Normalizes float audio buffer to 16-bit int,
*                then exports as 16-bit wave file.
*
*                The wave building code was adapted from:
*                https://stackoverflow.com/questions/23030980/
*                creating-a-mono-wav-file-using-c - thanks
*                Safayet Ahmed!
*
*  DATE:         29th March 2023
**************************************************************/

// definitions required for a 16-bit wave file
#define BITS_PER_SAMPLE  16  // 16-bit wave files require 16 bits per sample
#define AUDIO_FORMAT     1   // for PCM data
#define SUBCHUNK_1_SIZE  16  // dunno, what's this?
#define BYTE_RATE        SAMPLE_RATE * TOTAL_CHANNELS * BITS_PER_SAMPLE / 8
#define BLOCK_ALIGN      TOTAL_CHANNELS * BITS_PER_SAMPLE / 8
#define TOTAL_SAMPLES    (PLAY_TIME * TOTAL_CHANNELS) * SAMPLE_RATE
#define SUBCHUNK_2_SIZE  TOTAL_SAMPLES * BITS_PER_SAMPLE / 8
#define CHUNK_SIZE       4 + (8 + SUBCHUNK_1_SIZE) + (8 + SUBCHUNK_2_SIZE)

// holds the 44 byte header information
typedef struct wavfile_header_s
{
	int8_t  chunk_id[4];
	int32_t chunk_size;
	int8_t  format[4];
	int8_t  subchunk_1_id[4];
	int32_t subchunk_1_size;
	int16_t audio_format;
	int16_t total_channels;
	int32_t sample_rate;
	int32_t byte_rate;
	int16_t block_align;
	int16_t bits_per_sample;
	int8_t  subchunk_2_id[4];
	int32_t subchunk_2_size;
}
wavfile_header_t;

void wave_export(int16_t *input_buffer, uint32_t input_length)
{
	//===== WAVE HEADER CODE =====//

	// define & open the output file
	FILE* output_wave_file;
	output_wave_file = fopen("./output.wav", "w");

	// populate the header struct
	wavfile_header_t wav_header;

	wav_header.chunk_id[0] = 'R';
	wav_header.chunk_id[1] = 'I';
	wav_header.chunk_id[2] = 'F';
	wav_header.chunk_id[3] = 'F';

	wav_header.chunk_size = CHUNK_SIZE;

	wav_header.format[0] = 'W';
	wav_header.format[1] = 'A';
	wav_header.format[2] = 'V';
	wav_header.format[3] = 'E';

	wav_header.subchunk_1_id[0] = 'f';
	wav_header.subchunk_1_id[1] = 'm';
	wav_header.subchunk_1_id[2] = 't';
	wav_header.subchunk_1_id[3] = ' ';

	wav_header.subchunk_1_size  = SUBCHUNK_1_SIZE;
	wav_header.audio_format     = AUDIO_FORMAT;
	wav_header.total_channels   = TOTAL_CHANNELS;
	wav_header.sample_rate      = SAMPLE_RATE;
	wav_header.byte_rate        = BYTE_RATE;
	wav_header.block_align      = BLOCK_ALIGN;
	wav_header.bits_per_sample  = BITS_PER_SAMPLE;

	wav_header.subchunk_2_id[0] = 'd';
	wav_header.subchunk_2_id[1] = 'a';
	wav_header.subchunk_2_id[2] = 't';
	wav_header.subchunk_2_id[3] = 'a';
	wav_header.subchunk_2_size  = SUBCHUNK_2_SIZE;

	// write header to file
	fwrite(&wav_header, sizeof(wavfile_header_t), 1, output_wave_file);

	#ifdef DEBUG
		printf("Writing to output file...\n");
	#endif

	// write sample data to file
	fwrite(input_buffer, sizeof(uint16_t), input_length-1, output_wave_file);

	#ifdef DEBUG
		printf("Successfully written! (Maybe, there's no error checking.)\n");
	#endif

	fclose(output_wave_file);
}

#define DITHER_GAIN  5  // volume of dither noise

void float_to_sixteen_bit(float *input_buffer_float, int16_t *input_buffer_int, uint32_t input_length)
{
	float loudest_sample = 0.0;
	float current_sample = 0.0;
	float multiplier     = 0.0;

	for (uint32_t i = 0; i < TOTAL_SAMPLES; i++)
	{
		current_sample = fabs(input_buffer_float[i]);
		if (current_sample > loudest_sample)
			loudest_sample = current_sample;
	}

	#ifdef DEBUG
		printf("Loudest sample: %f\n",loudest_sample);
	#endif

	multiplier = 32767.0 / loudest_sample;

	for (uint32_t i = 0; i < TOTAL_SAMPLES; i++)
		input_buffer_float[i] = input_buffer_float[i] * multiplier;


	// squash down to 16-bit, add dither
	for (uint32_t i = 0; i < TOTAL_SAMPLES; i++)
	{
		input_buffer_float [i] = input_buffer_float[i] + (noise(0) * DITHER_GAIN);
		input_buffer_int   [i] = (int16_t)input_buffer_float[i];
	}
}
