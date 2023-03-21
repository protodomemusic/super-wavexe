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
*                - Per-channel FX
*                - Noise/percussion
*                - Vibrato/sweeps/portamento
*                - Actually use the filter
*                - Compile time is sllooowww, time to optimize
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
#include <time.h>

// note: definitions here required by wave-export.c
#define PLAY_TIME      10     // duration of recording in seconds
#define SAMPLE_RATE    44100  // cd quality audio
#define TOTAL_CHANNELS 2      // stereo file
#define TOTAL_SAMPLES  (PLAY_TIME * TOTAL_CHANNELS) * SAMPLE_RATE

// the voice we're using for the drums (behaves slightly differently)
#define DRUM_VOICE     TOTAL_VOICES - 1

// math stuff
#define PI 3.1415 // an accurate enough pi

// stealing the arduino map function - it's very handy indeed!
float map(float x, float in_min, float in_max, float out_min, float out_max)
{
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// nice little S-curve generator, makes a linear input (0.0 - 1.0) non-linear
// works on one side of the waveform for timbral interest
// I've found that having some DC wobble makes the waveforms sound a little
// more natural
float tan_smooth(float input)
{
	// requires cleanup at 0.0 and 1.0 as the algorithm isn't quite perfect
	if (input > 1.0 && input < 1.0)
		return (tanh(PI * (input - 0.5) / 0.5) / 2) + 0.5;
	else
		return input;
}

// cubic interpolation curve
// (does the same as the above, but for both sides of the waveform)
float cube_smooth(float input)
{
    if (input <= 0.0)
        return 0.0;
    else if (input >= 1.0)
        return 1.0;
    else
        return input * input * (3 - 2 * input);
}

float quart_smooth(float input)
{
    if (input <= 0.0)
        return 0.0;
    else if (input >= 1.0)
        return 1.0;
    else
        return input * input * input * input * (10 + input * (-15 + 6 * input));
}

// important definitions
#define MASTER_GAIN    1700  // convert -1.0 - 1.0 float to 16-bit
#define DITHER_GAIN    5     // volume of dither noise
#define OSC_DIVISOR    100   // change this to alter master tuning
#define TOTAL_VOICES   4     // how many oscillators (plus drum channel) we're calculating
#define FX_UPDATE_RATE 100   // how many frames before we update the FX parameters

// externals
#include "simple-filter.c"
#include "wave-export.c"
#include "wave-data.h"
#include "mmml.c"

// oscillator variables
double   osc_accumulator   [TOTAL_VOICES];
float    osc_pitch         [TOTAL_VOICES];
float    osc_volume        [TOTAL_VOICES];
float    osc_target_volume [TOTAL_VOICES];
uint8_t  osc_instrument    [TOTAL_VOICES];
uint8_t  osc_note_on       [TOTAL_VOICES];
uint8_t  osc_note_off      [TOTAL_VOICES];
uint8_t  osc_wave_mod      [TOTAL_VOICES];
float    osc_decay         [TOTAL_VOICES];

// wavetable variables
uint8_t  osc_sample_1      [TOTAL_VOICES];
uint8_t  osc_sample_2      [TOTAL_VOICES];
uint8_t  osc_sample_3      [TOTAL_VOICES];
float    osc_wavetable     [TOTAL_VOICES][WAVECYCLE_SIZE];

// wavetable sample mixes
float    osc_mix_1         [TOTAL_VOICES];
float    osc_mix_2         [TOTAL_VOICES];
float    osc_mix_1_rate    [TOTAL_VOICES];
float    osc_mix_2_rate    [TOTAL_VOICES];

// output waveform smoothing mix
float    osc_mix_3         [TOTAL_VOICES];

// filter variables
float    osc_lpf_position  [TOTAL_VOICES];
float    osc_lpf_target    [TOTAL_VOICES];

// lfo variables
double   lfo_accumulator   [TOTAL_VOICES];
float    lfo_pitch         [TOTAL_VOICES];
float    lfo_volume        [TOTAL_VOICES];

// drum variables
uint32_t drum_accumulator  = DRUM_DATA_SIZE;
uint8_t  current_drum_sample;

#define TOTAL_INSTRUMENTS     8
#define INSTRUMENT_PARAMETERS 7

static uint8_t instrument_bank [TOTAL_INSTRUMENTS][INSTRUMENT_PARAMETERS] =
{
//---------------------------------------------------------------------------------------//
//    waveform    /**/   wav mix   /**/  decay  /**/ 
//---------------------------------------------------------------------------------------//
// 0:
   {  0,  0,  0,  /**/   50,   80, /**/   255,  /**/  10 },
// 1:
   {  1,  1,  1,  /**/   50,   80, /**/   250,  /**/  10 },
// 2:
   {  2,  2,  2,  /**/   50,   80, /**/   250,  /**/  10 },
// 3:
   {  3,  3,  3,  /**/   50,   80, /**/   250,  /**/  10 },
// 4:
   {  4,  4,  4,  /**/   50,   80, /**/   250,  /**/  10 },
// 5:
   {  5,  5,  5,  /**/   50,   80, /**/   250,  /**/  10 },
// 6:
   {  6,  6,  6,  /**/   50,   80, /**/   250,  /**/  10 },
// 7:
   {  7,  7,  7,  /**/   50,   80, /**/   250,  /**/  10 },
//---------------------------------------------------------------------------------------//
};

// allows for balancing of voices
static float voice_boost [TOTAL_VOICES] =
{
//---------------------//
// <---- osc ----> drm //
//---------------------//
//  #1   #2   #3   #4  //
//---------------------//
   0.2, 0.2, 0.2, 1.0
//---------------------//
};

void configure_instrument(uint8_t voice, uint8_t instrument_id)
{
	// waveform
	osc_sample_1 [voice] = instrument_bank[instrument_id][0];
	osc_sample_2 [voice] = instrument_bank[instrument_id][1];
	osc_sample_3 [voice] = instrument_bank[instrument_id][2];

	// waveform transition speed
	osc_mix_1_rate [voice] = map(instrument_bank[instrument_id][3], 0, 255, 0.05, 0.0001);
	osc_mix_2_rate [voice] = map(instrument_bank[instrument_id][4], 0, 255, 0.01, 0.0001);

	// decay rate
	osc_decay [voice] = map(instrument_bank[instrument_id][5], 0, 255, 0.05, 0.0001);
}

void update_wavetable(float *input_wavecycles, uint8_t voice, int8_t sample_1, int8_t sample_2, int8_t sample_3, float volume, float mix_1, float mix_2, float mix_3)
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
		if (volume > 0.0)
			output = output * cube_smooth(volume);
		else
			output = 0.0;

		// write to table and apply smoothing algorithm to output for a really neat timbral effect
		osc_wavetable[voice][i] = map(tan_smooth(mix_3), 0.0, 1.0, output, tan_smooth(output));

		//osc_wavetable[voice][i] = output;
	};
}

int main()
{
	// get start time (for checking processing time)
	clock_t begin = clock();

	// where we'll put the final float data
	float *buffer_float = (float*)malloc(TOTAL_SAMPLES * sizeof(float));

	// where we'll put wavecycle data
	float *wavecycle_data = (float*)malloc(WAVECYCLE_SIZE * (TOTAL_WAVECYCLES + 1) * sizeof(float));

	printf("Building wavecycles...\n");
	generate_wavecycles(wavecycle_data);

	// where we'll put drum data
	float *drum_data = (float*)malloc(TOTAL_DRUM_DATA_SIZE * sizeof(float));

	printf("Building drums...\n");
	drum_generator(drum_data, wavecycle_data);

	// initialize MMML
	mmml_setup();

	// fx variables
	uint16_t fx_timer;
	uint8_t  fx_update_flag [TOTAL_VOICES];

	// debug data size
	printf("-------\n");
	printf("song buffer = %lu bytes\n",TOTAL_SAMPLES * sizeof(float));
	printf("wavecycle data = %lu bytes\n",WAVECYCLE_SIZE * (TOTAL_WAVECYCLES + 1) * sizeof(float));
	printf("drum sample data = %lu bytes\n",TOTAL_DRUM_DATA_SIZE * sizeof(float));
	printf("-------\n");

	// initial parameters
	for (uint8_t v = 0; v < TOTAL_VOICES; ++v)
	{
		configure_instrument(v, 0);

		osc_pitch[v]      = 440;
		osc_mix_3[v]      = 1.0;
		osc_lpf_target[v] = 0.001;
	}
	
	fx_timer = FX_UPDATE_RATE;

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
		float current_osc_sample = 0.0;
		float channel_output     = 0.0;
		float float_output       = 0.0;

		for (uint8_t v = 0; v < TOTAL_VOICES; ++v)
		{
			if (v < DRUM_VOICE)
			{
				current_osc_sample = (osc_accumulator[v] += osc_pitch[v]) / OSC_DIVISOR;
				channel_output     = osc_wavetable[v][(uint32_t)current_osc_sample % WAVECYCLE_SIZE] * voice_boost[v];
				float_output      += channel_output;
			}
			else
				if (drum_accumulator < DRUM_DATA_SIZE)
					float_output += drum_data[drum_accumulator++ + (DRUM_DATA_SIZE * current_drum_sample)] * cube_smooth(osc_target_volume[DRUM_VOICE]) * voice_boost[v];

			// update waveform if fx has changed...
			if (fx_update_flag[v] == 1)
			{
				// ...and if we're (basically) at a zero crossing
				if ((channel_output > -0.001 && channel_output < 0.001))
				{
					update_wavetable(wavecycle_data, v, osc_sample_1[v], osc_sample_2[v], osc_sample_3[v], osc_volume[v], osc_mix_1[v], osc_mix_2[v], osc_mix_3[v]);
					fx_update_flag[v]  = 0;
				}
			}

			//float_output += resonant_LPF(osc_wavetable[v][(uint32_t)current_osc_sample % WAVECYCLE_SIZE], osc_lpf_position[v], 0.2, 0);

			// stereo output
			for (uint8_t i = 0; i < TOTAL_CHANNELS; i++)
				buffer_float[t+i] = float_output;
		}

		/*~~~~~~~~~~~~~~~~~~~~~~~~*/
		/*  update FX parameters  */
		/*~~~~~~~~~~~~~~~~~~~~~~~~*/

		if (fx_timer-- == 0)
		{
			for (uint8_t v = 0; v < DRUM_VOICE; ++v)
			{
				//==================// 
				// update envelopes //
				//==================//

				// if we're at volume, trigger decay
				if (osc_note_on[v] == 0)
					osc_note_on[v] = 3;
				
				// de-click release envelope
				if (osc_note_on[v] == 2 && osc_volume[v] > 0.0)
					osc_volume[v] -= 0.1;

				// decay envelope
				if (osc_note_on[v] == 3 && osc_volume[v] > 0.0)
					osc_volume[v] -= osc_decay[v] * osc_target_volume[v]; // compensate for target volume

				// prevent volume from exceeding boundaries
				if (osc_volume[v] > 1.0)
					osc_volume[v] = 1.0;
				else if (osc_volume[v] < 0.0)
					osc_volume[v] = 0.0;

				/*-~-~-~-~-~-~-~-~-~-*/

				// wavetable envelopes
				if (osc_mix_1[v] < 1.0)
					osc_mix_1[v] += osc_mix_1_rate[v];

				if (osc_mix_2[v] < 1.0)
					osc_mix_2[v] += osc_mix_2_rate[v];

				if (osc_mix_3[v] > 0.0)
					osc_mix_3[v] -= 0.001;

				/*-~-~-~-~-~-~-~-~-~-*/

				// // update lfos
				// lfo_output[v] = wavetable_data[(uint8_t)((lfo_accumulator[v] += lfo_pitch[v]) >> 9) + (lfo_waveform[v] * 4)];
				// lfo_output[v] = (lfo_output[v] * lfo_amplitude[v]) >> 5;

				// // lfo swell / lfo delay
				// if (lfo_amplitude[v] < (lfo_intensity[v] + 1) && lfo_tick[v] == 0)
				// {
				// 	lfo_amplitude[v]++;
				// 	lfo_tick[v] = lfo_length[v] << 6;
				// }
				// else if (lfo_tick[v] > 0)
				// 	lfo_tick[v]--;

				/*-~-~-~-~-~-~-~-~-~-*/

				// filter envelope
				if (osc_lpf_position[v] > osc_lpf_target[v])
					osc_lpf_position[v] -= 0.0002;

				/*-~-~-~-~-~-~-~-~-~-*/

				// check for instrument reset
				if (osc_note_on[v] == 1)
				{
					// reset accumulator so we start at a zero crossing
					osc_accumulator[v] = 0;

					// reset all the waveform mixes
					osc_mix_1[v] = 0.0;
					osc_mix_2[v] = 0.0;
					osc_mix_3[v] = 1.0;

					// do the rest of the stuff
					osc_volume[v]       = osc_target_volume[v];
					osc_note_on[v]      = 0;
					osc_lpf_position[v] = 0.02;
					configure_instrument(v, osc_instrument[v]);
				}

				fx_update_flag[v] = 1;
			}

			// update the drum voice
			if (osc_note_on[DRUM_VOICE] == 1)
			{
				osc_note_on[DRUM_VOICE] = 0;
				current_drum_sample     = (uint8_t)osc_pitch[DRUM_VOICE] - 1;
				drum_accumulator        = 0;
			}

			fx_timer = FX_UPDATE_RATE;
		}

		/*~~~~~~~~~~~~~~~~~~~~~~~~~*/
		/*  process sequence data  */
		/*~~~~~~~~~~~~~~~~~~~~~~~~~*/

		mmml(osc_pitch,osc_target_volume,osc_instrument,osc_note_on);
	}

	/*~~~~~~~~~~~~~~~~~~~~*/
	/*  apply channel FX  */
	/*~~~~~~~~~~~~~~~~~~~~*/

	// we can free up some memory here
	free(wavecycle_data);
	free(drum_data);

	/*~~~~~~~~~~~~~~~~~~~~~~~*/
	/*  process output file  */
	/*~~~~~~~~~~~~~~~~~~~~~~~*/

	printf("Converting to 16-bit...\n");

	// final output array
	int16_t *buffer_int = (int16_t*)malloc(TOTAL_SAMPLES * sizeof(int16_t));

	// squash down to 16-bit, add dither
	for (uint32_t i = 0; i < TOTAL_SAMPLES; i++)
	{
		buffer_float [i] = (buffer_float[i] * MASTER_GAIN) + (noise(0) * DITHER_GAIN);
		buffer_int   [i] = (int16_t)buffer_float[i];
	}

	free(buffer_float);

	printf("Building wave file...\n");
	wave_export(buffer_int, TOTAL_SAMPLES);

	free(buffer_int);

	// let's just see how long it took
	clock_t end = clock();
	double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("Done! Took %.2f seconds to complete.\n\n", time_spent);

	return 0;
}