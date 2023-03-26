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
*                - Portamento
*                - Vibrato/sweeps
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
#include <string.h>

// note: definitions here required by wave-export.c
#define PLAY_TIME      150    // duration of recording in seconds
#define SAMPLE_RATE    44100  // cd quality audio
#define TOTAL_CHANNELS 2      // stereo file
#define TOTAL_SAMPLES  (PLAY_TIME * TOTAL_CHANNELS) * SAMPLE_RATE

// the voice we're using for the drums (behaves slightly differently)
#define DRUM_VOICE TOTAL_VOICES - 1

// math stuff
#define PI 3.1415 // an accurate enough pi

// stealing the arduino map function - it's very handy indeed!
float map(float x, float in_min, float in_max, float out_min, float out_max)
{
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// foldback distortion by ed.bpu@eriflleh
// borrowed from https://www.musicdsp.org/en/latest/
float foldback(float input, float threshold)
{
	if (input > threshold || input <- threshold)
		input = fabs(fabs(fmod(input - threshold, threshold * 4)) - threshold * 2) - threshold;
	return input;
}

// nice little curve generator, makes a linear input (0.0 - 1.0) non-linear
// works on one side of the waveform for timbral interest
// I've found that having some DC wobble makes the waveforms sound a little
// more natural.
// Adjust the amount of variation with the "strength" parameter.
float tan_smooth(float input, float strength)
{
	// requires cleanup at 0.0 and 1.0 as the algorithm isn't quite perfect
	if (input > 0.0 && input < 1.0)
	{
		float x = (input - 0.5) / 0.5;
		float y = tanh(PI * x) / 2 + 0.5;
		float variation = sin(PI * input) * strength;
		return y + variation;
	}
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
#define MASTER_GAIN    5700    // convert -1.0 - 1.0 float to 16-bit
#define DITHER_GAIN    5       // volume of dither noise
#define OSC_DIVISOR    100     // change this to alter master tuning
#define TOTAL_VOICES   8       // how many oscillators (plus drum channel) we're calculating
#define FX_UPDATE_RATE 100     // how many frames before we update the FX parameters
#define LPF_HIGH       0.5     // the maximum height of the filter
#define ZERO_CROSS_EVENT 0.005 // what we consider to be a zero crossing

// externals
#include "simple-filter.c"
#include "wave-export.c"
#include "wave-data.h"
#include "mmml.c"
#include "freeverb.c"
#include "simple-delay.c"

// oscillator variables
float    osc_decay;

// wavetable variables
uint8_t  osc_sample_1;
uint8_t  osc_sample_2;
uint8_t  osc_sample_3;
float    osc_wavetable_l [WAVECYCLE_SIZE];
float    osc_wavetable_r [WAVECYCLE_SIZE];

// wavetable sample mixes
float    osc_mix_1_rate;
float    osc_mix_2_rate;
float    osc_mix_3_rate;

// effect variables
float    stereo_effect;
float    osc_lpf_state;
float    osc_lpf_speed;
float    osc_lpf_q;
float    osc_distortion;
float    osc_instrument_boost;

// drum variables
uint32_t drum_accumulator = DRUM_DATA_SIZE;
uint8_t  current_drum_sample;

#define TOTAL_INSTRUMENTS     9
#define INSTRUMENT_PARAMETERS 12

static uint8_t instrument_bank [TOTAL_INSTRUMENTS][INSTRUMENT_PARAMETERS] =
{
//----------------------------------------------------------------------------------//
//   waveform  |   wave mix  | decay | tan smooth fx |   low pass   | fldbk | instr |
//   1 | 2 | 3 | 1->2 | 2->3 | speed | speed | stereo| speed |   Q  | dstrn | boost |
//-------------|-------------|-------|---------------|----------------------|-------|
// 0: squishy saw            |       |               |              |       |       |
   {  7,  8,  9,   155,   225,    200,      1,    100,     80,   255,      0,     40,},
// 1: wurlitzer              |       |               |              |       |       |
   {  1,  7,  0,     1,   200,    130,    100,     90,      0,     0,    100,     60,},
// 2: music box              |       |               |              |       |       |
   {  6,  0,  0,   200,   220,      1,      1,     10,      0,     0,      0,     76,},
// 3: modem lead             |       |               |              |       |       |
   {  7, 14,  0,   200,   150,    190,    210,     10,      0,     0,    200,    120,},
// 4: gritty bass            |       |               |              |       |       |
   {  0,  0,  3,     1,   252,    190,    255,     10,      0,     0,    120,    255,},
// 5: funky rhodes           |       |               |              |       |       |
   {  3,  1,  0,   110,    20,    130,    255,     90,    240,   200,    120,     74,},
// 6: xylophonic organ       |       |               |              |       |       |
   { 11,  0,  0,    40,    20,    120,    220,    120,      0,     0,    120,     64,},
// 7: clicky bass            |       |               |              |       |       |
   {  7,  0,  3,     1,   252,    190,    255,     10,     20,    10,    120,    235,},
// 8: soft rhodes            |       |               |              |       |       |
   {  0,  0,  0,   110,    20,    130,    255,     90,      0,     0,    100,     54,},
//----------------------------------------------------------------------------------//
};

// allows for balancing of voices
static float voice_boost [TOTAL_VOICES] =
{
//------------------------------------------------//
//  <---------------- osc ----------------> | drm //
//  #0  | #1  | #2  | #3  | #4  | #5  | #6  | #7  //
//------|-----|-----|-----|-----|-----|-----|-----//
     1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  1.0,  0.9
//------------------------------------------------//
};

#define TOTAL_REVERB_PRESETS     5
#define TOTAL_REVERB_PARAMETERS  5

static uint8_t reverb_bank [TOTAL_REVERB_PRESETS][TOTAL_REVERB_PARAMETERS] =
{
//---------------------------------------//
//   width |  dry |  wet | damp | room  |//
//----------------|------|------|-------|//
// 0: no reverb   |      |      |       |
	{     0,   100,     0,     0,     0  },
// 1: a dusting   |      |      |       |
	{   150,   100,    15,    10,    10  },
// 2: a little space     |      |       |
	{   150,   100,    15,    50,    30  },
// 3: roomy       |      |      |       |
	{   200,   100,    20,    60,    80  },
// 4: spacious    |      |      |       |
	{   200,   100,    40,   100,    80  },
//----------------------------------------//
};

#define TOTAL_DELAY_PRESETS     5
#define TOTAL_DELAY_PARAMETERS  5

static uint8_t delay_bank [TOTAL_DELAY_PRESETS][TOTAL_DELAY_PARAMETERS] =
{
//-----------------------------------------------//
//  offset | feedbk | spread | dirctn |   mix  | //
//---------|--------|--------|--------|--------|-//
// 0: no delay      |        |        |        |
	{     0,       0,       0,       0,       0  },
// 1: super subtle delay left         |        |
	{   140,      99,      20,       1,       8  },
// 2: super subtle delay right        |        |
	{   140,      99,      20,       0,       8  },
// 3: classic stereo echo    |        |        |
	{   255,      40,      80,       0,      40  },
// 4: huge delay    |        |        |        |
	{   255,      99,      20,       1,      50  },
//-----------------------------------------------//
};

#define TOTAL_FX 2

static uint8_t channel_fx [TOTAL_VOICES][TOTAL_FX] =
{
//----------------//
// reverb | delay //
//----------------//
// voice #0
	{   3,    1  },
// voice #1
	{   3,    2  },
// voice #2
	{   3,    1  },
// voice #3
	{   3,    2  },
// voice #4
	{   3,    1  },
// voice #5
	{   4,    3  },
// voice #6
	{   1,    0  },
// voice #7
	{   2,    0  },
//----------------//
};

void configure_instrument(uint8_t instrument_id)
{
	// waveform
	osc_sample_1   = instrument_bank[instrument_id][0];
	osc_sample_2   = instrument_bank[instrument_id][1];
	osc_sample_3   = instrument_bank[instrument_id][2];

	// waveform transition speed
	osc_mix_1_rate = cube_smooth(map(instrument_bank[instrument_id][3], 0, 255, 0.1,  0.0001));
	osc_mix_2_rate = cube_smooth(map(instrument_bank[instrument_id][4], 0, 255, 0.1,  0.0001));

	// decay rate
	osc_decay      = cube_smooth(map(instrument_bank[instrument_id][5], 0, 255, 0.05, 0.0001));

	// tan smoothing
	osc_mix_3_rate = cube_smooth(map(instrument_bank[instrument_id][6], 0, 255, 0.1,  0.0001));
	stereo_effect  = cube_smooth(map(instrument_bank[instrument_id][7], 0, 255, 0.0,  1.0));

	// low pass filter
	osc_lpf_state  = 0.01;
	osc_lpf_speed  = map(instrument_bank[instrument_id][8], 0, 255, 0.00001, 0.01);
	osc_lpf_q      = map(instrument_bank[instrument_id][9], 0, 255, 0.1, 0.008);

	// distortion
	osc_distortion = map(instrument_bank[instrument_id][10], 0, 255, 1.0, 0.05);

	// instrument volume
	osc_instrument_boost = map(instrument_bank[instrument_id][11], 0, 255, 0.0, 2.0);
}

int main()
{
	// get start time (for checking processing time)
	clock_t begin = clock();

	// where we'll put the final float data
	float *buffer_float = (float*)malloc(TOTAL_SAMPLES * sizeof(float));

	// where we'll put the individual voice data
	float *voice_buffer = (float*)malloc(TOTAL_SAMPLES * sizeof(float));

	// where we'll put wavecycle data
	float *wavecycle_data = (float*)malloc(WAVECYCLE_SIZE * (TOTAL_WAVECYCLES + 1) * sizeof(float));

	printf("Building wavecycles...\n");
	generate_wavecycles(wavecycle_data);

	// where we'll put drum data
	float *drum_data = (float*)malloc(TOTAL_DRUM_DATA_SIZE * sizeof(float));

	printf("Building drums...\n");
	drum_generator(drum_data, wavecycle_data);

	// fx variables
	uint16_t fx_timer;
	uint8_t  fx_update_flag [TOTAL_VOICES];

	//-------------//
	// debug stuff //
	//-------------//
	uint32_t debug_song_buffer_size    = TOTAL_SAMPLES  * sizeof(float);
	uint32_t debug_voice_buffer_size   = TOTAL_SAMPLES  * sizeof(float);
	uint32_t debug_wavecycle_data_size = WAVECYCLE_SIZE * (TOTAL_WAVECYCLES + 1) * sizeof(float);
	uint32_t debug_drum_data_size      = TOTAL_DRUM_DATA_SIZE * sizeof(float);
	uint32_t debug_total_memory_usage  = debug_drum_data_size + debug_voice_buffer_size + debug_wavecycle_data_size + debug_drum_data_size;

	printf("-------\n");
	printf("song buffer      = %u bytes\n", debug_song_buffer_size);
	printf("voice buffer     = %u bytes\n", debug_voice_buffer_size);
	printf("wavecycle data   = %u bytes\n", debug_wavecycle_data_size);
	printf("drum sample data = %u bytes\n", debug_drum_data_size);
	printf("total mem usage  = %u bytes\n", debug_total_memory_usage);
	printf("-------\n");
	//-------------//

	/*~~~~~~~~~~~~~~~~~~~~~~~*/
	/*  generate audio file  */
	/*~~~~~~~~~~~~~~~~~~~~~~~*/

	printf("Generating audio...\n");

	for (uint8_t v = 0; v < TOTAL_VOICES; ++v)
	{
		printf("Voice #%i (Generation)", v);

		// initialize MMML variables
		mmml_setup(v);

		// oscillator variables
		double   osc_accumulator   = 0;
		float    osc_pitch         = 0;
		float    osc_volume        = 0;
		float    osc_target_volume = 0;
		uint8_t  osc_instrument    = 0;
		uint8_t  osc_note_on       = 0;
		uint8_t  osc_panning       = 0;

		// wavetable sample mixes
		float    osc_mix_1 = 0;
		float    osc_mix_2 = 0;

		// output waveform smoothing mix
		float    osc_mix_3 = 1.0;

		// stereo offset variables
		float tan_smoothing_l = 0.0;
		float tan_smoothing_r = 0.0;
		uint8_t stereo_fx_counter;

		// initial parameters
		configure_instrument(0);
		fx_timer = FX_UPDATE_RATE;

		for(uint32_t t = 0; t < TOTAL_SAMPLES - 1; t += TOTAL_CHANNELS)
		{
			/*~~~~~~~~~~~~~~~~~~~~~~*/
			/*  process wavecycles  */
			/*~~~~~~~~~~~~~~~~~~~~~~*/

			// calculate the samples
			float current_osc_sample = 0.0;
			float channel_output_l   = 0.0;
			float channel_output_r   = 0.0;
			float float_output_l     = 0.0;
			float float_output_r     = 0.0;

			if (v < DRUM_VOICE)
			{
				current_osc_sample = (osc_accumulator += osc_pitch) / OSC_DIVISOR;
				channel_output_l   = osc_wavetable_l[(uint32_t)current_osc_sample % WAVECYCLE_SIZE];
				channel_output_r   = osc_wavetable_r[(uint32_t)current_osc_sample % WAVECYCLE_SIZE];

				// apply low pass filter
				if (osc_lpf_q > 0)
				{
					channel_output_l = resonant_LPF(channel_output_l, osc_lpf_state, osc_lpf_q + osc_lpf_state, 0);
					channel_output_r = resonant_LPF(channel_output_r, osc_lpf_state, osc_lpf_q + osc_lpf_state, 1);
				}

				// apply foldback distortion
				if (osc_distortion < 1.0)
				{
					channel_output_l = foldback(channel_output_l, osc_distortion);
					channel_output_r = foldback(channel_output_r, osc_distortion);
				}

				// write the final sample to the output
				float_output_l += channel_output_l * osc_instrument_boost;
				float_output_r += channel_output_r * osc_instrument_boost;
			}
			else
			{
				if (drum_accumulator < DRUM_DATA_SIZE)
				{
					drum_accumulator++;
					float_output_l += drum_data[drum_accumulator + (DRUM_DATA_SIZE * current_drum_sample)] * cube_smooth(osc_target_volume);
					float_output_r += drum_data[drum_accumulator + (DRUM_DATA_SIZE * current_drum_sample)] * cube_smooth(osc_target_volume);
				}
			}

			// update waveform if fx has changed...
			if (fx_update_flag[v] == 1)
			{
				// ...and if we're (basically) at a zero crossing
				if ((channel_output_l > -0.005 && channel_output_l < 0.005) || (channel_output_r > -0.005 && channel_output_r < ZERO_CROSS_EVENT))
				{
					float output = 0.0;

					// let's calculate the panning first
					float panning_l;
					float panning_r;

					float pan_mapped = (((float)(osc_panning / 100.0) + 1) / 2.0) * (PI / 2.0);

					panning_r = sin(pan_mapped);
					panning_l = cos(pan_mapped);

					for (int16_t i = 0; i < WAVECYCLE_SIZE; i++)
					{
						// read sample 1
						output = wavecycle_data[(i + (osc_sample_1 * WAVECYCLE_SIZE))];

						// merge with sample 2
						output = map(osc_mix_1, 0.0, 1.0, output, wavecycle_data[(i + (osc_sample_2 * WAVECYCLE_SIZE))]);

						// merge with sample 3
						output = map(osc_mix_2, 0.0, 1.0, output, wavecycle_data[(i + (osc_sample_3 * WAVECYCLE_SIZE))]);

						// apply volume
						if (osc_volume > 0.0)
							output = output * cube_smooth(osc_volume);
						else
							output = 0.0;

						// write to table and apply smoothing algorithm to output for a really neat timbral effect
						osc_wavetable_l [i] = map(cube_smooth(osc_mix_3), 0.0, 1.0, output, tan_smooth(output, tan_smoothing_l)) * panning_l;
						osc_wavetable_r [i] = map(cube_smooth(osc_mix_3), 0.0, 1.0, output, tan_smooth(output, tan_smoothing_r)) * panning_r;
					};

					fx_update_flag[v]  = 0;
				}
			}

			// save to output
			voice_buffer[t]   = float_output_l;
			voice_buffer[t+1] = float_output_r;

			/*~~~~~~~~~~~~~~~~~~~~~~~~*/
			/*  update FX parameters  */
			/*~~~~~~~~~~~~~~~~~~~~~~~~*/

			if (fx_timer-- == 0)
			{
				if (v < DRUM_VOICE)
				{
					//==================// 
					// update envelopes //
					//==================//

					// if we're at volume, trigger decay
					if (osc_note_on == 0)
						osc_note_on = 3;
					
					// de-click release envelope
					if (osc_note_on  == 2 && osc_volume > 0.0)
						osc_volume -= 0.1;

					// decay envelope
					if (osc_note_on  == 3 && osc_volume > 0.0)
						osc_volume -= osc_decay * osc_target_volume; // compensate for target volume

					// prevent volume from exceeding boundaries
					if (osc_volume > 1.0)
						osc_volume = 1.0;
					else if (osc_volume < 0.0)
						osc_volume = 0.0;

					/*-~-~-~-~-~-~-~-~-~-*/

					// wavetable envelopes
					if (osc_mix_1 < 1.0)
						osc_mix_1 += osc_mix_1_rate;

					if (osc_mix_2 < 1.0)
						osc_mix_2 += osc_mix_2_rate;

					// weird tanh output mixer
					if (osc_mix_3 > 0.0)
						osc_mix_3 -= osc_mix_3_rate;

					/*-~-~-~-~-~-~-~-~-~-*/

					if (osc_lpf_state < LPF_HIGH)
						osc_lpf_state = osc_lpf_state + osc_lpf_speed;

					// check for instrument reset
					if (osc_note_on == 1)
					{
						// reset accumulator so we start at a zero crossing
						osc_accumulator = 0;

						// reset all the waveform mixes
						osc_mix_1 = 0.0;
						osc_mix_2 = 0.0;
						osc_mix_3 = 1.0;

						// do the rest of the stuff
						osc_volume          = osc_target_volume;
						osc_note_on         = 0;
						configure_instrument(osc_instrument);

						stereo_fx_counter++;
						tan_smoothing_l = 0;
						tan_smoothing_r = 0;

						if ((stereo_fx_counter & 1) == 0)
							tan_smoothing_l = stereo_effect;
						else
							tan_smoothing_r = stereo_effect;
					}

					fx_update_flag[v] = 1;
				}
				else
				{
					// update the drum voice
					if (osc_note_on == 1)
					{
						osc_note_on         = 0;
						current_drum_sample = (uint8_t)osc_pitch - 1;
						drum_accumulator    = 0;
					}
				}

				fx_timer = FX_UPDATE_RATE;
			}

			/*~~~~~~~~~~~~~~~~~~~~~~~~~*/
			/*  process sequence data  */
			/*~~~~~~~~~~~~~~~~~~~~~~~~~*/

			mmml_update(&osc_pitch,&osc_target_volume,&osc_instrument,&osc_note_on,&osc_panning,v);
		}

		/*~~~~~~~~~~~~~~~~~~~~*/
		/*  apply channel FX  */
		/*~~~~~~~~~~~~~~~~~~~~*/

		printf("(Effects)\n");

		// process delay
		if (channel_fx[v][1] > 0)
		{
			delay_process(
				voice_buffer,
				TOTAL_SAMPLES,
				(uint32_t)delay_bank[channel_fx[v][1]][0] * 100,
				(float)   delay_bank[channel_fx[v][1]][1] / 100.0,
				(float)   delay_bank[channel_fx[v][1]][2] / 100.0,
				(uint8_t) delay_bank[channel_fx[v][1]][3],
				(float)   delay_bank[channel_fx[v][1]][4] / 100.0);
		}

		// process reverb
		if (channel_fx[v][0] > 0)
		{
			reverb_process(
				voice_buffer,
				TOTAL_SAMPLES,
				(float)reverb_bank[channel_fx[v][0]][0] / 100.0,
				(float)reverb_bank[channel_fx[v][0]][1] / 100.0,
				(float)reverb_bank[channel_fx[v][0]][2] / 100.0,
				(float)reverb_bank[channel_fx[v][0]][3] / 100.0,
				(float)reverb_bank[channel_fx[v][0]][4] / 100.0);
		}

		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		/*  export channel to master  */
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

		for (uint32_t i = 0; i < TOTAL_SAMPLES; i++)
			buffer_float[i] += voice_buffer[i] * voice_boost[v];

		// clean up the buffer
		memset(voice_buffer, 0, TOTAL_SAMPLES * sizeof(float));
	}

	// we can free up some memory now
	free(wavecycle_data);
	free(drum_data);
	free(voice_buffer);

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