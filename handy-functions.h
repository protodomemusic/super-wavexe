/**************************************************************
*  FILENAME:     handy-functions.h
*  DESCRIPTION:  Just a buncha useful math-y stuff. 
*
*  DATE:         28th March 2023
**************************************************************/

// math stuff
#define PI 3.1415              // an accurate enough pi
#define LOOKUP_TABLE_SIZE 1000 // size for sin/tanh lookup tables

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

// Right! This is a weird one. I think the M1/M2 has functions to make sin() tanh() cos() etc much
// faster than using the lookup solution below. But the lookup is demonstrably faster on Raspberry
// Pi, so I guess it depends on the architecture. ¯\_(._.)_/¯

float sin_table  [LOOKUP_TABLE_SIZE + 1];
float tanh_table [LOOKUP_TABLE_SIZE + 1];

void trig_tables_init()
{
	float z;

	for (int i = 0; i <= LOOKUP_TABLE_SIZE; i++)
	{
		// sine
		z = -M_PI + (2.0 * M_PI * i) / LOOKUP_TABLE_SIZE;
		sin_table[i] = sinf(z);

		// hypertan
		z = -5.0 + (10.0 * i) / LOOKUP_TABLE_SIZE;
		tanh_table [i] = tanhf(z);
	}
}

float sin_lookup(float x)
{
	// compute the index of the lookup table
	double index = (LOOKUP_TABLE_SIZE / (2.0 * PI)) * (x + PI);

	// interpolate between the two nearest entries in the lookup table
	int   i    = (int) index;
	float frac = index - i;
	float y0   = sin_table [i];
	float y1   = sin_table [i + 1];
	float y    = y0 + frac * (y1 - y0);

	return y;
}

float tanh_lookup(float x) 
{
	// compute the index of the lookup table
	double index = (LOOKUP_TABLE_SIZE / 10.0) * (x + 5.0);

	// interpolate between the two nearest entries in the lookup table
	int   i    = (int) index;
	float frac = index - i;
	float y0   = tanh_table [i];
	float y1   = tanh_table [i + 1];
	float y    = y0 + frac * (y1 - y0);

	return y;
}

// Nice little curve generator, makes a linear input (0.0 - 1.0) non-linear.
// Works on one side of the waveform for timbral interest. I've found that
// having some DC wobble makes the waveforms sound a little more natural.
// Adjust the amount of variation with the "strength" parameter (mainly
// just for stereo difference.
float tan_smooth(float input, float strength)
{
	// requires cleanup at 0.0 and 1.0 as the algorithm isn't quite perfect
	if (input > 0.0 && input < 1.0)
	{
		float x = (input - 0.5) / 0.5;
		float y = tanh_lookup(PI * x) / 2 + 0.5;
		float variation = sin_lookup(PI * input) * strength;
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

// another smoothing, different shape
float quart_smooth(float input)
{
	if (input <= 0.0) return 0.0;
	else if (input >= 1.0)
		return 1.0;
	else
		return input * input * input * input * (10 + input * (-15 + 6 * input));
}