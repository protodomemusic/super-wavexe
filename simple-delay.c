/**************************************************************
*  FILENAME:     simple-delay.c
*  DESCRIPTION:  Basic little stereo delay. Does the job.
*
*  DATE:         13th March 2022
**************************************************************/

void delay_process(float *input_buffer, uint32_t input_length, uint32_t delay_time, float delay_feedback, float delay_spread, uint8_t delay_direction, float delay_volume)
{
	float *delay_buffer     = (float*)malloc(delay_time * 2 * sizeof(float)); // x2 because stereo
	float *input_buffer_wet = (float*)malloc(input_length   * sizeof(float));

	uint32_t delay_playhead = 1;

	float right_volume    = 0.0;
	float left_volume     = 0.0;
	float declicker_right = 0.0;
	float declicker_left  = 0.0;

	uint8_t declicker_l_flag = 0;
	uint8_t declicker_r_flag = 0;

	uint8_t  pingpong_flag    = delay_direction;
	uint32_t pingpong_counter = 0;

	for (uint32_t i = 0; i < input_length; ++i)
		input_buffer_wet[i] = input_buffer[i];

	for (uint32_t i = 0; i < input_length; i += 2) // stereo file, jump 2 samples
	{
		// once we've got a whole delays worth of material, start
		// writing previous sample data to the delay buffer
		if(i > delay_time)
		{
			delay_buffer[delay_playhead]   = input_buffer_wet[i   - delay_time] * declicker_left;
			delay_buffer[delay_playhead+1] = input_buffer_wet[i+1 - delay_time] * declicker_right;

			delay_playhead += 2;
			delay_playhead = delay_playhead % delay_time;
		}

		// change volume of toggle slowly, to avoid clicking
		if (declicker_left < 1.0 && declicker_l_flag == 0)
			declicker_left += 0.001;
		if (declicker_right < 1.0 && declicker_r_flag == 0)
			declicker_right += 0.001;

		// decay to the stereo spread level
		if (declicker_left > delay_spread && declicker_l_flag == 1)
			declicker_left -= 0.001;
		if (declicker_right > delay_spread && declicker_r_flag == 1)
			declicker_right -= 0.001;		

		// toggle delay between L and R
		if(pingpong_counter++ > delay_time)
		{
			if (pingpong_flag == 0)
			{
				pingpong_flag = 1;

				declicker_l_flag = 0;
				declicker_r_flag = 1;
			}
			else if (pingpong_flag == 1)
			{
				pingpong_flag = 0;

				declicker_l_flag = 1;
				declicker_r_flag = 0;
			}
			pingpong_counter  = 0;
		}

		// save the delay data to the current buffer. also, lower the volume to prevent feedback
		input_buffer_wet[i]   += (delay_buffer[delay_playhead]   * delay_feedback);
		input_buffer_wet[i+1] += (delay_buffer[delay_playhead+1] * delay_feedback);
	}

	for (uint32_t i = 0; i < input_length; ++i)
		input_buffer[i] = (input_buffer[i] * (1.0 - delay_volume)) + (input_buffer_wet[i] * delay_volume);

	free(delay_buffer);
	free(input_buffer_wet);
}