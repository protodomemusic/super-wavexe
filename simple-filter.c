/**************************************************************
*  FILENAME:     simple-filter.h
*  DESCRIPTION:  Simple resonant high/low-pass filters, adapted
*                from Beam Myself Into The Future's code:
*                https://beammyselfintothefuture.wordpress.com/
*                I spent ages looking for simple, but decent,
*                filter code and theirs was both the simplest,
*                and best, I came across. Thanks BMITF!
* DATE:          3rd May 2022
**************************************************************/

#define LPF 0
#define HPF 1

float last_input  [TOTAL_CHANNELS];
float last_output [TOTAL_CHANNELS];
float momentum    [TOTAL_CHANNELS];
 
float resonant_LPF (float input, float cutoff, float resonance, uint8_t channel)
{
	// Lower resonance values increases resonance
	// Lower cutoff values lowers the cutoff frequency

	float distance_to_go = input - last_output[channel];
	momentum[channel] += distance_to_go * cutoff;

	return last_output[channel] += momentum[channel] + distance_to_go * resonance;
}
 
float resonant_HPF (float input, float cutoff, float resonance, uint8_t channel)
{
	last_output[channel] += momentum[channel] - last_input[channel] + input;
	last_input[channel] = input;
	momentum[channel] = momentum[channel] * resonance - last_output[channel] * cutoff;

	return last_output[channel];
}