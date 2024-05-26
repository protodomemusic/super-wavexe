/*******************************************************************************
*  FILENAME:     simple-limiter.c
*  DESCRIPTION:  Basic little stereo limiter. Does the job. Also, there's a tiny
*                normalize function in here too.
* 
*                Requires a SAMPLE_RATE definition (can be whatever you like)
*                and also a TOTAL_CHANNELS definition of 2 (stereo). Assumes an
*                interleaved stereo PCM input in the input_buffer array.
*
*  DATE:         4th April 2024
*******************************************************************************/

void apply_limiter(float *input_buffer, uint32_t input_length, uint8_t attack_ms, uint8_t release_ms, float limit)
{
	// number of samples corresponding to the attack and release time
	int lookahead_samples = (int)(attack_ms  * 0.001 * SAMPLE_RATE);
	int release_samples   = (int)(release_ms * 0.001 * SAMPLE_RATE);

	// loudness innit
	float gain = 1.0f;

	// keep track of the moving peak
	float moving_peak  = 0.0f;

	// double ended queue front and back indices
	int front = 0;
	int back  = 0;

	// double ended queue to store indices
	int  *double_ended_queue = (int *)malloc(input_length * sizeof(int));

	for (uint32_t i = 0; i < input_length; i++)
	{
		// remove elements outside the current window
		if (front < back && double_ended_queue[front] < (int)i - lookahead_samples)
			front++;

		// remove elements that are not greater than the current element
		while (front < back && fabs(input_buffer[double_ended_queue[back - 1]]) <= fabs(input_buffer[i]))
			back--;

		// add the current element index to the double ended queue
		double_ended_queue[back++] = i;

		// the element at the front of the double ended queue is the maximum in the current window
		moving_peak = fabs(input_buffer[double_ended_queue[front]]);

		// calculate gain
		float new_gain = moving_peak > limit ? (limit / moving_peak) : 1.0f;

		// attack: apply new gain immediately
		if (new_gain < gain)
			gain = new_gain;
		// release: apply gradually
		else
			gain += (new_gain - gain) / release_samples;

		input_buffer[i] *= gain;
	}

	free(double_ended_queue);
}

void apply_normalizer(float *input_buffer, uint32_t input_length)
{
	float loudest_sample = 0.0;
	float current_sample = 0.0;
	float multiplier     = 0.0;

	// find the loudest sample in terms of absolute value
	for (uint32_t i = 0; i < input_length; i++)
	{
		current_sample = fabs(input_buffer[i]);
		if (current_sample > loudest_sample)
			loudest_sample = current_sample;
	}

	multiplier = 1.0 / loudest_sample;

	// apply the calculated multiplier to each sample
	for (uint32_t i = 0; i < input_length; i++)
		input_buffer[i] = (input_buffer[i] * multiplier);
}