/*******************************************************************************
*  FILENAME:     simple-filter.c
*  DESCRIPTION:  Simple resonant high/low-pass filters, adapted from Beam Myself
*                Into The Future's code:
*                https://beammyselfintothefuture.wordpress.com/
*                I spent ages looking for simple, but decent, filter code and
*                theirs was both the simplest, and best, I came across.
*                Thanks BMITF!
* 
*                There's also a little EQ in here too.
* 
*                Requires a TOTAL_CHANNELS definition of 2 (stereo). Assumes an
*                interleaved stereo PCM input in the input_buffer array.
* 
*  DATE:         3rd May 2022
*                4th April 2024
*******************************************************************************/

float apply_lpf(float input, float cutoff, float resonance, uint8_t channel)
{
	static float last_input  [TOTAL_CHANNELS];
	static float last_output [TOTAL_CHANNELS];
	static float momentum    [TOTAL_CHANNELS];

	float distance_to_go = input - last_output[channel];
	momentum[channel] += distance_to_go * cutoff;

	return last_output[channel] += momentum[channel] + distance_to_go * resonance;
}
 
float apply_hpf(float input, float cutoff, float resonance, uint8_t channel)
{
	static float last_input  [TOTAL_CHANNELS];
	static float last_output [TOTAL_CHANNELS];
	static float momentum    [TOTAL_CHANNELS];

	last_output[channel] += momentum[channel] - last_input[channel] + input;
	last_input[channel] = input;
	momentum[channel] = momentum[channel] * resonance - last_output[channel] * cutoff;

	return last_output[channel];
}

void apply_eq(float* input_buffer, size_t input_length, float   low_cutoff, float high_cutoff, 
              float  low_q,        float  hi_q,         uint8_t intensity,  float mix)
{
	// early exit if we're goin in dry
	if (mix <= 0.0f) return;
	if (mix > 1.0f)  mix = 1.0f;

	// process low pass first...
	if (low_cutoff > 0.0)
	{
		for (int j = 0; j < intensity; ++j)
		{
			for (uint32_t i = 0; i < input_length; i += 2)
			{
				for (uint8_t c = 0; c < TOTAL_CHANNELS; ++c)
				{
					// keep original signal for mixing
					float dry_signal = input_buffer[i+c];
					float wet_signal = apply_lpf(dry_signal, low_cutoff, low_q, c);
					input_buffer[i+c] = dry_signal * (1 - mix) + wet_signal * mix;
				}
			}
		}
	}

	// ...then add a high pass to create a band pass effect
	if (high_cutoff > 0.0)
	{
		for (int j = 0; j < intensity; ++j)
		{
			for (uint32_t i = 0; i < input_length; i += 2)
			{
				for (uint8_t c = 0; c < TOTAL_CHANNELS; ++c)
				{
					float dry_signal = input_buffer[i+c];
					float wet_signal = apply_hpf(dry_signal, high_cutoff, hi_q, c);
					input_buffer[i+c] = dry_signal * (1 - mix) + wet_signal * mix;
				}
			}
		}
	}
}
