/**************************************************************
*  FILENAME:     super-wavexe.c
*  DESCRIPTION:  Super Executable Wavetable Synthesizer
*
*  NOTES:        A little wavetable synthesizer optimized to
*                make tiny executable music files.
* 
*                An upgrade of:
*                https://github.com/protodomemusic/wavexe
* 
*                Sequencing powered by:
*                https://github.com/protodomemusic/mmml
*
*                TO DO NEXT:
*                - Instruments / channels
*                - Per-channel FX
*                - MMML sequencing
*                - Everything basically
*
*  AUTHOR:       Blake 'PROTODOME' Troise
*  PLATFORM:     Command Line Application (MacOS / Linux)
*  DATE:         1st April 2022
**************************************************************/

// libraries
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <math.h>

// note: definitions here required by wave-export.c
#define PLAY_TIME      10     // duration of recording in seconds
#define SAMPLE_RATE    44100  // cd quality audio
#define TOTAL_CHANNELS 2      // mono file
#define TOTAL_SAMPLES  (PLAY_TIME * TOTAL_CHANNELS) * SAMPLE_RATE

// stealing the arduino map function - it's very handy indeed!
float map(float x, float in_min, float in_max, float out_min, float out_max)
{
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// externals
#include "wave-export.c"
#include "wave-data.h"
#include "simple-filter.c"

// important definitions
#define MASTER_GAIN 5000     // convert 0-1 float to 16-bit
#define OSC_DIVISOR 100      // change this to alter master tuning
#define TOTAL_NOTES 108      // how many notes we'll generate
#define NOTE_END    7902.13  // the highest frequency we'll generate

// FX definitions
#define TOTAL_EQ_BANDS   2
#define TOTAL_EQ_PASSES  3
#define FILTERS_PER_BAND 2

// oscillator variables
double osc_accumulator;
float  osc_pitch;

// wavetable variables
float   osc_volume;
float   osc_mix_1;
float   osc_mix_2;
uint8_t osc_sample_1;
uint8_t osc_sample_2;
float   osc_wavetable [WAVECYCLE_SIZE];
float   mod_mix;

void update_wavetable( float *input_wavecycles, int8_t sample_1, int8_t sample_2, int8_t sample_3, float volume, float mix_1, float mix_2 )
{
	// let's update the voice's wavetable
	float output;

	for (int16_t i = 0; i < WAVECYCLE_SIZE; i++)
	{
		// read sample 1
		output = input_wavecycles[(i + (sample_1 * WAVECYCLE_SIZE))];

		// merge with sample 2
		output = map(mix_1, 0.0, 1.0, output, input_wavecycles[(i + (sample_2 * WAVECYCLE_SIZE))]);

		// merge with sample 3
		output = map(mix_2, 0.0, 1.0, output, input_wavecycles[(i + (sample_3 * WAVECYCLE_SIZE))]);

		// apply volume
		if (volume > 0)
			output = output * volume;
		else
			output = 0;

		// write to table
		osc_wavetable[i] = output;
	};
}

int main()
{
	// where we'll put the audio data
	float *buffer_float = (float*)malloc(TOTAL_SAMPLES * sizeof(float));

	// where we'll put the generated notes
	float *notes = (float*)malloc(TOTAL_NOTES * sizeof(float));

	// where we'll put wavecycle data
	float *wavecycle_data = (float*)malloc(WAVECYCLE_SIZE * TOTAL_WAVECYCLES * sizeof(float));

	printf("Building wavecycles...\n");
	generate_wavecycles(wavecycle_data);

	// debug stuff
	uint16_t debug_timer;
	float debug_counter   = 1;
	float debug_counter_2 = 0;
	uint8_t instrument_value = 0;

	// generate frequencies for pitch lookup table
	for(uint8_t i = 0; i < TOTAL_NOTES; i++)
	{
		float frequency = NOTE_END / pow(1.059463094, TOTAL_NOTES - i);
		notes[i] = frequency;
	}

	// initial parameters
	osc_pitch   = notes[70];
	osc_mix_1   = 0;
	osc_mix_2   = 0;
	debug_timer = 500;

	/*~~~~~~~~~~~~~~~~~~~~~~~*/
	/*  generate audio file  */
	/*~~~~~~~~~~~~~~~~~~~~~~~*/

	printf("Generating audio...\n");

	for(uint32_t t = 0; t < TOTAL_SAMPLES - 1; t += TOTAL_CHANNELS)
	{
		/*~~~~~~~~~~~~~~~~~~~~~~*/
		/*  process wavecycles  */
		/*~~~~~~~~~~~~~~~~~~~~~~*/

		// calculate the samples
		float current_osc_sample;
		float float_output;

		current_osc_sample = (osc_accumulator += osc_pitch) / OSC_DIVISOR;
		float_output       = osc_wavetable[(uint32_t)current_osc_sample % WAVECYCLE_SIZE];
		
		// stereo output
		for (int i = 0; i < TOTAL_CHANNELS; i++)
			buffer_float[t+i] = float_output;
			//buffer_float[t+i] = resonant_LPF(float_output, debug_counter, 0.1, i);

		//printf("%f\n",float_output);

		update_wavetable(wavecycle_data, 14, 6, 16, 1.0, osc_mix_1, osc_mix_2);

		//debug
		if (debug_timer-- == 0)
		{
			if (debug_counter > 0.008)
				debug_counter -= 0.0001;

			if (osc_mix_1 < 1.0)
				osc_mix_1 += 0.0009;

			if (osc_mix_2 < 1.0)
				osc_mix_2 += 0.0001;

			if (debug_counter_2 > 1.0)
			{
				osc_mix_1 = 0;
				osc_mix_2 = 0;
				debug_counter_2 = 0;
			}
			else{
				debug_counter_2 += 0.00001;
			}

			debug_timer = 10;
		}
	}

	/*~~~~~~~~~~~~~~~~~~~~~~~*/
	/*  process output file  */
	/*~~~~~~~~~~~~~~~~~~~~~~~*/

	// we can free up some memory here
	free(notes);
	free(wavecycle_data);

	printf("Converting to 16-bit...\n");

	// final output array
	int16_t *buffer_int = (int16_t*)malloc(TOTAL_SAMPLES * sizeof(int16_t));

	// squash down to 16-bit
	for (uint32_t i = 0; i < TOTAL_SAMPLES; i++)
	{
		buffer_float [i] = buffer_float[i] * MASTER_GAIN;
		buffer_int   [i] = (int16_t)buffer_float[i];
	}

	free(buffer_float);

	printf("Building wave file...\n");
	wave_export(buffer_int, TOTAL_SAMPLES);

	free(buffer_int);

	return 0;
}