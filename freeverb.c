/*******************************************************************************
 * 
 * Public domain C implementation of the original freeverb, with the addition of
 * support for sample rates other than 44.1khz. Written by rxi, found at:
 * https://gist.github.com/rxi/e5488c6660154329ddfc4a7a7d2997f8
 *
 * Classic freeverb C++ version written by Jezar at Dreampoint, June 2000
 *
 * Edits here are alterations to rxi's C version to make it as portable as
 * possible (some languages don't allow passing of structs into functions, nor
 * arrays in structs).
 * 
 *******************************************************************************/

#define NUMCOMBS        3
#define NUMALLPASSES    2
#define MUTED           0.0
#define FIXEDGAIN       0.05
#define SCALEWET        1
#define SCALEDRY        1
#define SCALEDAMP       0.4
#define SCALEROOM       0.28
#define STEREOSPREAD    23
#define OFFSETROOM      0.7

#define RV_SAMPLE_RATE  44100.0
#define FREEZEMODE      0.5
#define TOTAL_CHANNELS  2

float comb_feedback    [TOTAL_CHANNELS][NUMCOMBS];
float comb_filterstore [TOTAL_CHANNELS][NUMCOMBS];
float comb_damp1       [TOTAL_CHANNELS][NUMCOMBS];
float comb_damp2       [TOTAL_CHANNELS][NUMCOMBS];
float comb_buf         [TOTAL_CHANNELS][NUMCOMBS][2048];
int   comb_bufsize     [TOTAL_CHANNELS][NUMCOMBS];
int   comb_bufidx      [TOTAL_CHANNELS][NUMCOMBS];

float allpass_feedback [TOTAL_CHANNELS][NUMALLPASSES];
float allpass_buf      [TOTAL_CHANNELS][NUMALLPASSES][1024];
int   allpass_bufsize  [TOTAL_CHANNELS][NUMALLPASSES];
int   allpass_bufidx   [TOTAL_CHANNELS][NUMALLPASSES];

// reverb context variables
float verb_mode      = 0;
float verb_gain      = 0;
float verb_roomsize  = 0;
float verb_roomsize1 = 0;
float verb_damp      = 0;
float verb_damp1     = 0;
float verb_wet       = 0;
float verb_wet1      = 0;
float verb_wet2      = 0;
float verb_dry       = 0;
float verb_width     = 0;

static inline float allpass_process(int chn, int obj, float input)
{
	float bufout = allpass_buf[chn][obj][allpass_bufidx[chn][obj]];

	float output = -input + bufout;
	allpass_buf[chn][obj][allpass_bufidx[chn][obj]] = input + bufout * allpass_feedback[chn][obj];

	if (++allpass_bufidx[chn][obj] >= allpass_bufsize[chn][obj])
		allpass_bufidx[chn][obj] = 0;

	return output;
}

static inline float comb_process(int chn, int obj, float input)
{
	float output = comb_buf[chn][obj][comb_bufidx[chn][obj]];

	comb_filterstore[chn][obj] = output * comb_damp2[chn][obj] + comb_filterstore[chn][obj] * comb_damp1[chn][obj];

	comb_buf[chn][obj][comb_bufidx[chn][obj]] = input + comb_filterstore[chn][obj] * comb_feedback[chn][obj];

	if (++comb_bufidx[chn][obj] >= comb_bufsize[chn][obj])
		comb_bufidx[chn][obj] = 0;

	return output;
}

static inline void comb_set_damp(int chn, int obj, float n)
{
	comb_damp1[chn][obj] = n;
	comb_damp2[chn][obj] = 1.0 - n;
}

static void reverb_update()
{
	verb_wet1 = verb_wet * (verb_width * 0.5 + 0.5);
	verb_wet2 = verb_wet * ((1 - verb_width) * 0.5);

	verb_roomsize1 = verb_roomsize;
	verb_damp1     = verb_damp;
	verb_gain      = FIXEDGAIN;

	for (int i = 0; i < NUMCOMBS; i++)
	{
		comb_feedback[0][i] = verb_roomsize1;
		comb_feedback[1][i] = verb_roomsize1;
		comb_set_damp(0, i, verb_damp1);
		comb_set_damp(1, i, verb_damp1);
	}
}

void reverb_process(float *input_buffer, int input_length, float v_width, float v_dry, float v_wet, float v_damp, float v_room)
{
	/*===== INITIALISE PARAMETERS =====*/

	for (int i = 0; i < NUMALLPASSES; i++)
	{
		allpass_feedback[0][i] = 0.5;
		allpass_feedback[1][i] = 0.5;
	}

	const int combs[]     = { 1116, 1188, 1277, };
	const int allpasses[] = { 556, 441, };

	// init comb buffers
	for (int i = 0; i < NUMCOMBS; i++)
	{
		comb_bufsize[0][i] = combs[i];
		comb_bufsize[1][i] = (combs[i] + STEREOSPREAD);
	}

	// init allpass buffers
	for (int i = 0; i < NUMALLPASSES; i++)
	{
		allpass_bufsize[0][i] = allpasses[i];
		allpass_bufsize[1][i] = (allpasses[i] + STEREOSPREAD);
	}

	//------- initialise parameters -------//

	verb_wet      = v_wet  * SCALEWET;
	verb_roomsize = v_room * SCALEROOM + OFFSETROOM;
	verb_dry      = v_dry  * SCALEDRY;
	verb_damp     = v_damp * SCALEDAMP;
	verb_width    = v_width;

	reverb_update();

	/*===== PROCESS REVERB =====*/

	for (int i = 0; i < input_length; i += 2)
	{
		float outl  = 0;
		float outr  = 0;
		float input = (input_buffer[i] + input_buffer[i+1]) * verb_gain;

		/* accumulate comb filters in parallel */
		for (int i = 0; i < NUMCOMBS; i++)
		{
			outl += comb_process(0, i, input);
			outr += comb_process(1, i, input);
		}

		/* feed through allpasses in series */
		for (int i = 0; i < NUMALLPASSES; i++)
		{
			outl = allpass_process(0, i, outl);
			outr = allpass_process(1, i, outr);
		}

		/* replace buffer with output */
		input_buffer[i  ] = outl * verb_wet1 + outr * verb_wet2 + input_buffer[i  ] * verb_dry;
		input_buffer[i+1] = outr * verb_wet1 + outl * verb_wet2 + input_buffer[i+1] * verb_dry;
	}

	memset(comb_buf, 0, sizeof comb_buf);
	memset(allpass_buf, 0, sizeof allpass_buf);
}