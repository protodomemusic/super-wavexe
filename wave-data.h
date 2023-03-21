/**************************************************************
*  FILENAME:     wave-data.h
*  DESCRIPTION:  Where we generate the various waveforms to be
*                used by the wavetable synth.
*
*                Basically, just FM (to keep it conceptually
*                simple), but with cool hyperbolic functions
*                for saw-ish waves.
*
*                As of 19/03/23 it also generates drums using
*                a simple LFSR and an oscillator described
*                above.
*
*  DATE:         21st August 2022
**************************************************************/

#define TOTAL_ALGORITHMS  3
#define TOTAL_DIVISORS    7                                 // total divisors for synthesis
#define TOTAL_WAVECYCLES  TOTAL_DIVISORS * TOTAL_ALGORITHMS // reminder: up to 255 possible
#define WAVECYCLE_SIZE    1024                              // immutable (for now)
#define WAVECYCLE_HALF    (WAVECYCLE_SIZE / 2)              // saves dividing it in the code

// some noise for drums and dithering
float noise(float pitch)
{
	static uint16_t update;
	static uint16_t lfsr = 1;
	static uint16_t bit;
	static float    output;

	if (update-- == 0)
	{
		bit    = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5)) & 1;
		lfsr   = (lfsr >> 1) | (bit << 15);
		output = ((lfsr % 200) / 200.0) - 0.5;
		update = pitch + 1;
	}

	return output;
}

// definitions for the drum parameters
#define TOTAL_DRUM_SOUNDS             5
#define TOTAL_DRUM_SOUND_PARAMETERS   6

// how long (in milliseconds) each drum sample will last
#define DRUM_LENGTH     800
#define DRUM_PITCH      4
// the total size (in samples) of each drum sound (roughly 140KB)
#define DRUM_DATA_SIZE        ((SAMPLE_RATE/1000) * DRUM_LENGTH) / DRUM_PITCH
#define TOTAL_DRUM_DATA_SIZE  DRUM_DATA_SIZE * TOTAL_DRUM_SOUNDS

// store drum sound parameters as tiny 8-bit numbers 
uint8_t drum_sounds [TOTAL_DRUM_SOUNDS][TOTAL_DRUM_SOUND_PARAMETERS] = 
{
//---------------------------------------//
//     oscillator/wave      |    noise   //
//--------------| <------ decays ------> //
//    wav | frq | vol | ptc | vol |  ptc //
//---------------------------------------//
// 0: kick                  |
   {     1,   70,   25,  240,  230,  255 },
// 1: snare                 |
   {     4,   16,   15,   80,   18,   20 },
// 2: closed hi-hat         |
   {    10,  255,  120,    0,   30,    0 },
// 3: open hi-hat           |
   {    10,  255,  120,    0,    4,    0 },
// 4: tom                   |
   {     1,   70,    9,   24,  230,  255 },
//---------------------------------------//
};

// generate a bunch of drums and put in memory
void drum_generator(float *input_drum_data, float *input_wavecycle_array)
{
	float    wave_sample;
	float    wave_pitch;
	float    wave_pitch_decay;
	float    wave_accumulator;
	float    wave_volume_decay;
	float    wave_volume;
	float    wave_output;
	uint16_t wave_shape;

	float noise_pitch;
	float noise_pitch_decay;
	float noise_volume_decay;
	float noise_volume;
	float noise_output;

	float final_output;

	uint16_t fx_timer = FX_UPDATE_RATE;

	// save a new drum sound to memory
	for (int d = 0; d < TOTAL_DRUM_SOUNDS; ++d)
	{
		// reset drum variables
		wave_sample      = 0.0;
		wave_accumulator = 0.0;
		wave_volume      = 1.0;
		noise_volume     = 1.0;
		noise_pitch      = 0.0;

		// load new drum parameters and map to floats
		wave_shape         =     drum_sounds[d][0];
		wave_pitch         = map(drum_sounds[d][1], 0, 255,    1.0, 500.0);
		wave_volume_decay  = map(drum_sounds[d][2], 0, 255,  0.001,  0.04);
		wave_pitch_decay   = map(drum_sounds[d][3], 0, 255,  0.001,   1.0);
		noise_volume_decay = map(drum_sounds[d][4], 0, 255,  0.001,  0.04);
		noise_pitch_decay  = map(drum_sounds[d][5], 0, 255, 0.0001,   1.0);

		for (int s = 0; s < DRUM_DATA_SIZE * DRUM_PITCH; ++s)
		{
			// start with noise
			noise_output = noise(noise_pitch) * quart_smooth(noise_volume);

			// add an oscillator
			wave_sample = (wave_accumulator += wave_pitch) / OSC_DIVISOR;
			wave_output = input_wavecycle_array[(uint32_t)wave_sample % WAVECYCLE_SIZE * wave_shape];
			wave_output = (wave_output / 1.2) * quart_smooth(wave_volume);

			// process any modulations
			if (fx_timer-- == 0)
			{
				// wave volume decay
				if (wave_volume > 0.0)
					wave_volume -= wave_volume_decay;

				// wave pitch decay
				if (wave_pitch > 0.0)
					wave_pitch -= wave_pitch_decay;

				// noise volume decay
				if (noise_volume > 0.0)
					noise_volume -= noise_volume_decay;

				// noise pitch decay
				noise_pitch += noise_pitch_decay;

				fx_timer = FX_UPDATE_RATE;
			}

			// combine noise and oscillator
			final_output = (wave_output + noise_output) / 2;

			// add a little EQ notch to make snare a bit punchier
			final_output = final_output + (resonant_LPF(final_output, 0.003, 0.02, 0) / 2);

			// save sample to drum data
			if (s % DRUM_PITCH == 0)
				input_drum_data[(s / DRUM_PITCH) + (DRUM_DATA_SIZE * d)] = final_output;
		}
	}
}

static float divisors [TOTAL_DIVISORS * 2] =
{
	0.1, 0.1, // basic
	1,   1,   // soft
	4,   1,   // medium
	1.5, 3,   // firm
	3.5, 7,   // hard
	4,   4,   // glassy
	6,   6,   // brittle
};

float operator_function(input, function)
{
	float output;

	switch (function)
	{
		case 0: // sine wave operator
			output =  sin(PI * (input - WAVECYCLE_HALF) / WAVECYCLE_HALF);
			break;
		case 1: // hypertan wave operator
			output = tanh(PI * (input - WAVECYCLE_HALF) / WAVECYCLE_HALF);
			break;
		case 2: // hypersine wave operator
			output = sinh(PI * (input - WAVECYCLE_HALF) / WAVECYCLE_HALF);
			break;
		case 3: // silence
			output = 0;
			break;
	}
	return output;
}

void generate_wavecycles(float *input_wavecycle_array)
{
	float    output;
	uint16_t current_algorithm;

	// where we'll put our temporary wavecycle data
	float *wave_data = (float*)malloc(WAVECYCLE_SIZE * sizeof(float));

	for (uint16_t i = 0; i < TOTAL_WAVECYCLES; ++i)
	{
		/*=====================*/
		/* wavecycle generator */
		/*=====================*/

		current_algorithm = i / TOTAL_DIVISORS;

		for (uint16_t w = 0; w < WAVECYCLE_SIZE; w++)
			wave_data [w] = (operator_function(w, current_algorithm) * cos(PI * (w - WAVECYCLE_HALF) / (WAVECYCLE_HALF / divisors[(i % TOTAL_DIVISORS)*2]))) * cos(PI * (w - WAVECYCLE_HALF) / (WAVECYCLE_HALF / divisors[((i % TOTAL_DIVISORS)*2)+1]));

		/*=========================*/
		/* wavecycle normalization */
		/*=========================*/

		float peak_value    = 0;
		float current_value = 0;

		uint16_t zero_crossing_offset = 0;

		// get peak value
		for (uint16_t w = 0; w < WAVECYCLE_SIZE; w++)
		{
			current_value = wave_data[w];

			if (fabs(current_value) > peak_value)
				peak_value = current_value;
		}

		// scale values between -1.0 and 1.0 and look for first zero crossing
		for (uint16_t w = 0; w < WAVECYCLE_SIZE; w++)
		{
			wave_data[w] = map(wave_data[w], -peak_value, peak_value, -1.0, 1.0);

			if (zero_crossing_offset == 0)
				if (input_wavecycle_array[w + WAVECYCLE_SIZE * i] == 0.0)
					zero_crossing_offset = w;
		}

		/*========================*/
		/* start at zero crossing */
		/*========================*/

		// we want to start with zero to de-click all waveforms

		// get first zero crossing and write data to start of wavetable
		for (uint16_t w = zero_crossing_offset; w < WAVECYCLE_SIZE; w++)
			input_wavecycle_array[(w - zero_crossing_offset) + WAVECYCLE_SIZE * i] = wave_data[w];

		// get the chunk before the offset and append to the end
		for (uint16_t w = 0; w < zero_crossing_offset; w++)
			input_wavecycle_array[(w + zero_crossing_offset) + WAVECYCLE_SIZE * i] = wave_data[w];
	}
}