/**************************************************************
*  FILENAME:     wave-data.h
*  DESCRIPTION:  Where we generate the various waveforms to be
*                used by the wavetable synth.
*
*                Basically, just FM (to keep it conceptually
*                simple), but with cool hyperbolic functions
*                for saw-ish waves.
*
*  DATE:         21st August 2022
**************************************************************/

#define TOTAL_ALGORITHMS  3
#define TOTAL_DIVISORS    7                                 // total divisors for synthesis
#define TOTAL_WAVECYCLES  TOTAL_DIVISORS * TOTAL_ALGORITHMS // reminder: up to 255 possible
#define WAVECYCLE_SIZE    1024                              // immutable (for now)
#define WAVECYCLE_HALF    (WAVECYCLE_SIZE / 2)              // saves dividing it in the code

float noise()
{
	#define NOISE_UPDATE_RATE 2000

	static uint16_t update = NOISE_UPDATE_RATE;
	static uint16_t lfsr   = 1;
	static uint16_t bit;
	static float    output;

	if (update-- == 0)
	{
		bit    = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5)) & 1;
		lfsr   = (lfsr >> 1) | (bit << 15);
		output = ((lfsr % 200) / 200.0) - 0.5;
		update = NOISE_UPDATE_RATE;
	}

	return output;
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
			output = sin(PI * (input - WAVECYCLE_HALF) / WAVECYCLE_HALF);
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

	for (uint16_t i = 0; i < TOTAL_WAVECYCLES; ++i)
	{
		/*=====================*/
		/* wavecycle generator */
		/*=====================*/

		current_algorithm = i / TOTAL_DIVISORS;

		for (uint16_t w = 0; w < WAVECYCLE_SIZE; w++)
			input_wavecycle_array [w + WAVECYCLE_SIZE * i] = (operator_function(w, current_algorithm) * cos(PI * (w - WAVECYCLE_HALF) / (WAVECYCLE_HALF / divisors[(i % TOTAL_DIVISORS)*2]))) * cos(PI * (w - WAVECYCLE_HALF) / (WAVECYCLE_HALF / divisors[((i % TOTAL_DIVISORS)*2)+1]));

		/*=========================*/
		/* wavecycle normalization */
		/*=========================*/

		float peak_value    = 0;
		float current_value = 0;

		// get peak value
		for (uint16_t w = 0; w < WAVECYCLE_SIZE; w++)
		{
			current_value = input_wavecycle_array[w + WAVECYCLE_SIZE * i];

			if (fabs(current_value) > peak_value)
				peak_value = current_value;
		}

		// scale values between -1.0 and 1.0
		for (uint16_t w = 0; w < WAVECYCLE_SIZE; w++)
			input_wavecycle_array[w + WAVECYCLE_SIZE * i] = map(input_wavecycle_array[w + WAVECYCLE_SIZE * i], -peak_value, peak_value, -1.0, 1.0);
	}
}