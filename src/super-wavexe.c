/*******************************************************************************
*  FILENAME:     super-wavexe.c
*  DESCRIPTION:  Super Executable Wavetable Synthesizer
*
*  NOTES:        A little wavetable synthesizer optimized to make tiny
*                executable music files.
* 
*                An upgrade of:
*                https://github.com/protodomemusic/wavexe and
*                https://github.com/protodomemusic/mmml
*
*                You can find the new compiler (mega-mmml.py) in this repo under
*                '/compiler'. Look at the shell scripts in root for how to build
*                a song.
*
*                For information on how to write your own music using this
*                engine, check out README.md.
*
*                TO DO NEXT:
*                - Panning can be a bit weird. -100% has a lot of signal in the
*                  right channel (bad), whereas 100% has no info in the left
*                  channel (good).
*                - Looping on final channel has issues. If the sequencer reaches
*                  the end of material, the behavior can be a bit weird.
*                - Tempo commands only affect the channel they're in. I guess
*                  this isn't a bug per se, but it's weird and annoying in
*                  practice.
*                - Editing, and pointing to, a new instrument-config.h for every
*                  song is also annoying and weird, like above.
*                - Vibrato/sweeps would be dope. I know you can use portamento
*                  as a sweep (see 'garlic-bread.4ml') but it's hacky.
*
*  AUTHOR:       Blake 'PROTODOME' Troise
*  PLATFORM:     Command Line Application (MacOS / Linux)
*  DATE:         Initial:   1st April 2022
*                Update #1: 29th July 2023
*                Update #2: 27th March 2024
*******************************************************************************/

// libraries
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <string.h>

// comment out to change stuff
//#define LINUX // linux playback
#define DEBUG // debug messages

// currently only linux supports live playback via the ALSA library
// make sure to do: apt-get install libasound2-dev
#ifdef LINUX
	#include <alsa/asoundlib.h>
	#include "alsa-playback.c"
#endif

// the custom data files to read from, currently looking at the
// default song files
#include "../songs/garlic-bread/4ml-data.h"
#include "../songs/garlic-bread/instrument-config.h"

// note: definitions here required by wave-export.c
#define SAMPLE_RATE    44100  // cd quality audio
#define TOTAL_CHANNELS 2      // stereo file
#define TOTAL_SAMPLES  (PLAY_TIME * TOTAL_CHANNELS) * SAMPLE_RATE

// the voice we're using for the drums (behaves slightly differently)
#define DRUM_VOICE TOTAL_VOICES - 1

// important definitions
#define OSC_DIVISOR      100    // change this to alter master tuning
#define FX_UPDATE_RATE   100    // how many frames before we update the FX parameters
#define LPF_HIGH         0.3    // the maximum height of the filter
#define ZERO_CROSS_EVENT 0.006  // what we consider to be a zero crossing
#define DITHER_GAIN      5      // volume of dither noise

// audio effects
#include "fx/simple-limiter.c"
#include "fx/simple-filter.c"
#include "fx/simple-delay.c"
#include "fx/freeverb.c"

// externals
#include "handy-functions.h"
#include "wave-data.h"
#include "4ml.c"
#include "wave-export.c"

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

float    osc_portamento_rate;
uint8_t  portamento_enabled;

// drum variables
uint32_t drum_accumulator = DRUM_DATA_SIZE;
uint8_t  current_drum_sample;

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

	// add portamento rate and enable
	osc_portamento_rate = map(instrument_bank[instrument_id][12], 0, 255, 1.0, 0.0001);

	if (instrument_bank[instrument_id][12] == 0)
		portamento_enabled = 0;
	else
		portamento_enabled = 1;
}

int main()
{
	#ifdef DEBUG
		// get start time (for checking processing time)
		clock_t begin = clock();
	#endif

	// we need our lookups
	trig_tables_init();

	// where we'll put the final mix, individual voice, and wavecycle data
	float *buffer_float   = (float*)calloc(TOTAL_SAMPLES, sizeof(float));
	float *voice_buffer   = (float*)calloc(TOTAL_SAMPLES,  sizeof(float));
	float *wavecycle_data = (float*)calloc(WAVECYCLE_SIZE * (TOTAL_WAVECYCLES + 1), sizeof(float));

	#ifdef DEBUG
		printf("Building wavecycles...\n");
	#endif

	generate_wavecycles(wavecycle_data);

	// where we'll put drum data
	float *drum_data = (float*)calloc(TOTAL_DRUM_DATA_SIZE, sizeof(float));

	#ifdef DEBUG
		printf("Building drums...\n");
	#endif

	drum_generator(drum_data, wavecycle_data);

	// fx variables
	uint16_t fx_timer;
	uint8_t  fx_update_flag [TOTAL_VOICES] = { 0 };

	#ifdef DEBUG
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
	#endif

	/*~~~~~~~~~~~~~~~~~~~~~~~*/
	/*  generate audio file  */
	/*~~~~~~~~~~~~~~~~~~~~~~~*/

	#ifdef DEBUG
		printf("Generating audio...\n");
	#endif

	for (uint8_t v = 0; v < TOTAL_VOICES; ++v)
	{
		#ifdef DEBUG
			// get voice start time
			clock_t voice_begin = clock();
			printf("Voice #%i (Generation)", v);
		#endif

		// initialize MMML variables
		_4ml_setup(v);

		// oscillator variables
		double   osc_accumulator   = 0;
		float    osc_pitch         = 0;
		float    osc_volume        = 0;
		float    osc_target_volume = 0;
		uint8_t  osc_instrument    = 0;
		uint8_t  osc_note_on       = 0;
		uint8_t  osc_panning       = 0;
		float    osc_current_pitch = 0;
		float    osc_target_pitch  = 0;

		// wavetable sample mixes
		float    osc_mix_1 = 0;
		float    osc_mix_2 = 0;

		// output waveform smoothing mix
		float    osc_mix_3 = 1.0;

		// stereo offset variables
		float   tan_smoothing_l   = 0.0;
		float   tan_smoothing_r   = 0.0;
		uint8_t stereo_fx_counter = 0;

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
			float drum_output        = 0.0;

			if (v < DRUM_VOICE)
			{
				current_osc_sample = (osc_accumulator += osc_pitch) / OSC_DIVISOR;
				channel_output_l   = osc_wavetable_l[(uint32_t)current_osc_sample % WAVECYCLE_SIZE];
				channel_output_r   = osc_wavetable_r[(uint32_t)current_osc_sample % WAVECYCLE_SIZE];

				// apply low pass filter
				channel_output_l = apply_lpf(channel_output_l, osc_lpf_state, osc_lpf_q + osc_lpf_state, 0);
				channel_output_r = apply_lpf(channel_output_r, osc_lpf_state, osc_lpf_q + osc_lpf_state, 1);

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
					drum_output = drum_data[drum_accumulator + (DRUM_DATA_SIZE * current_drum_sample)] * cube_smooth(osc_target_volume);
					float_output_l += drum_output;
					float_output_r += drum_output;
				}
			}

			// update waveform if fx has changed...
			if (fx_update_flag[v] == 1)
			{
				// ...and if we're (basically) at a zero crossing (makes waveforms update smoother)
				// plus it also reduces how often we have to run this code.
				if ((channel_output_l > -ZERO_CROSS_EVENT && channel_output_l < ZERO_CROSS_EVENT) ||
				    (channel_output_r > -ZERO_CROSS_EVENT && channel_output_r < ZERO_CROSS_EVENT))
				{
					float output = 0.0;

					// let's calculate the panning first
					float panning_l;
					float panning_r;

					float pan_mapped = (((float)(osc_panning / 100.0) + 1) / 2.0) * (PI / 2.0);

					panning_r = sin_lookup(pan_mapped);
					panning_l = sin_lookup(pan_mapped + (PI/2)); // cosine

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
						// reset portamento on note on
						// weird decision maybe, but I think it sounds better
						// and it's my engine so I get to choose
						// compile and uncomment if you disagree coward
						osc_current_pitch = osc_target_pitch;

						// reset accumulator so we start at a zero crossing
						osc_accumulator = 0;

						// reset all the waveform mixes
						osc_mix_1 = 0.0;
						osc_mix_2 = 0.0;
						osc_mix_3 = 1.0;

						// do the rest of the stuff
						osc_volume  = osc_target_volume;
						osc_note_on = 0;
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
						// fix for drums and portamento
						osc_current_pitch   = osc_target_pitch;
						osc_pitch           = osc_current_pitch;

						osc_note_on         = 0;
						current_drum_sample = (uint8_t)osc_pitch - 1;
						drum_accumulator    = 0;
					}
				}

				fx_timer = FX_UPDATE_RATE;
			}

			// calculate the portamento
			if (portamento_enabled)
			{
				if (osc_current_pitch != osc_target_pitch)
				{
					if (fabs(osc_current_pitch - osc_target_pitch) < osc_portamento_rate)
						osc_current_pitch = osc_target_pitch;
					else if (osc_current_pitch < osc_target_pitch)
						osc_current_pitch += osc_portamento_rate;
					else if (osc_current_pitch > osc_target_pitch)
						osc_current_pitch -= osc_portamento_rate;
				}
				osc_pitch = osc_current_pitch;
			}
			else
				osc_pitch = osc_target_pitch;

			/*~~~~~~~~~~~~~~~~~~~~~~~~~*/
			/*  process sequence data  */
			/*~~~~~~~~~~~~~~~~~~~~~~~~~*/

			_4ml_update(&osc_target_pitch,&osc_target_volume,&osc_instrument,&osc_note_on,&osc_panning,v);
		}

		/*~~~~~~~~~~~~~~~~~~~~*/
		/*  apply channel FX  */
		/*~~~~~~~~~~~~~~~~~~~~*/

		#ifdef DEBUG
			printf("(Effects)");
		#endif

		if (voice_boost[v] > 0)
		{
			// process eq
			if (channel_fx[v][0] > 0)
			{
				apply_eq(
					voice_buffer,
					TOTAL_SAMPLES,
					map((float)eq_bank[channel_fx[v][0]][0], 1, 255, 0.001, 2.0),
					map((float)eq_bank[channel_fx[v][0]][1], 1, 255, 0.001, 2.0),
					(float)eq_bank[channel_fx[v][0]][2] / 100.0,
					(float)eq_bank[channel_fx[v][0]][3] / 100.0,
					eq_bank[channel_fx[v][0]][4],
					(float)eq_bank[channel_fx[v][0]][5] / 100.0);
			}

			// process limiter
			if (channel_fx[v][1])
			{
				apply_limiter(
					voice_buffer,
					TOTAL_SAMPLES,
					(uint8_t)limiter_bank[channel_fx[v][1]][0],
					(uint8_t)limiter_bank[channel_fx[v][1]][1],
					(float)  limiter_bank[channel_fx[v][1]][2] / 100.0);
			}

			// process delay
			if (channel_fx[v][2] > 0)
			{
				apply_delay(
					voice_buffer,
					TOTAL_SAMPLES,
					(uint32_t)delay_bank[channel_fx[v][2]][0] * 100,
					(float)   delay_bank[channel_fx[v][2]][1] / 100.0,
					(float)   delay_bank[channel_fx[v][2]][2] / 100.0,
					(uint8_t) delay_bank[channel_fx[v][2]][3],
					(float)   delay_bank[channel_fx[v][2]][4] / 100.0);
			}

			// process reverb
			if (channel_fx[v][3] > 0)
			{
				apply_reverb(
					voice_buffer,
					TOTAL_SAMPLES,
					(float)reverb_bank[channel_fx[v][3]][0] / 100.0,
					(float)reverb_bank[channel_fx[v][3]][1] / 100.0,
					(float)reverb_bank[channel_fx[v][3]][2] / 100.0,
					(float)reverb_bank[channel_fx[v][3]][3] / 100.0,
					(float)reverb_bank[channel_fx[v][3]][4] / 100.0);
			}
		}

		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		/*  export channel to master  */
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

		for (uint32_t i = 0; i < TOTAL_SAMPLES; i++)
			buffer_float[i] += voice_buffer[i] * map(voice_boost[v], 0, 255, 0.0, 2.0);

		// clean up the buffer
		memset(voice_buffer, 0, TOTAL_SAMPLES * sizeof(float));

		#ifdef DEBUG
			// let's just see how long it took
			clock_t voice_end = clock();
			double time_spent = (double)(voice_end - voice_begin) / CLOCKS_PER_SEC;
			printf(" - %.2f secs\n", time_spent);
		#endif
	}

	// we can free up some memory now
	free(wavecycle_data);
	free(drum_data);
	free(voice_buffer);

	/*~~~~~~~~~~~~~~~~~~~*/
	/*  apply master FX  */
	/*~~~~~~~~~~~~~~~~~~~*/

	#ifdef DEBUG
		// get fx start time
		clock_t clock_begin = clock();
		printf("Applying master FX...");
	#endif

	apply_normalizer(buffer_float, TOTAL_SAMPLES);
	apply_limiter(buffer_float, TOTAL_SAMPLES, MASTER_LIMITER_ATTACK_MS, MASTER_LIMITER_RELEASE_MS, MASTER_LIMITER_THRESHOLD);
	apply_normalizer(buffer_float, TOTAL_SAMPLES);

	#ifdef DEBUG
		// let's just see how long it took
		clock_t clock_end = clock();
		double time_spent = (double)(clock_end - clock_begin) / CLOCKS_PER_SEC;
		printf(" - %.2f secs\n", time_spent);
	#endif

	/*~~~~~~~~~~~~~~~~~~~~~~~*/
	/*  process output file  */
	/*~~~~~~~~~~~~~~~~~~~~~~~*/

	#ifdef DEBUG
		printf("Converting to 16-bit...\n");
	#endif

	// final output array
	int16_t *buffer_int = (int16_t*)calloc(TOTAL_SAMPLES, sizeof(int16_t));

	// convert float back to int16_t
	for (uint32_t i = 0; i < TOTAL_SAMPLES; i++)
		buffer_int[i] = (int16_t)(buffer_float[i] * 32767); // + (noise(0) * DITHER_GAIN);

	// we'll miss you buffer float
	free(buffer_float);

	#ifdef DEBUG
		printf("Building wave file...\n");
	#endif

	wave_export(buffer_int, TOTAL_SAMPLES);

	#ifdef DEBUG
		// let's just see how long it took
		clock_t end = clock();
		time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
		printf("Done! Took %.2f seconds to complete.\n\n", time_spent);
	#endif

	// time to listen to the results (on linux only)
	#ifdef LINUX
		#ifdef DEBUG
			printf("It's playback time!\n\n");
		#endif
		alsa_playback(buffer_int, TOTAL_SAMPLES);
	#endif

	// eh, don't need to
	free(buffer_int);

	return 0;
}
