/*******************************************************************************
*  FILENAME:     instrument-config.h
*  DESCRIPTION:  Default tables for instrument/effects configuration. Duplicate
*                this file before you edit it - it has useful info in it!
*
*                Edit your instrument properties in the first table, then select
*                which effect you'd like applied to each voice in the second.
*
*                Unfortunately, you can't change the effects over time just yet,
*                though (as a note) you could have a new 'X0-255' 4ML command.
*                It could index the sample it was at when the command was last
*                seen and then, when effects are later applied, it would switch
*                between presets when the stored sample location was reached.
*
*                See diagram for effects chain.
*
*  DATE:         29th March 2023
********************************************************************************

Here's a quick run-down of effects and how they work. I picked
stuff that helped 'de-FM' and rough up the FM waveforms a bit.
I was aiming for an kinda rhodes/wurlitzer/crumar multiman
vibe.

The effects are listed in the order in which they are applied.
All effect parameters are 8-bit values from 0 to 255.

---------------------
| tanh wave shaping |
---------------------

Alters the shape of the waveform using the hyperbolic tan
function. The intensity of the shaping is altered in one
channel to create a subtle stereo effect. The 'speed' parameter
dictates how fast the effect decays leaving the original
waveform. Higher values slow the decay speed.

----------------------
| low pass filtering |
----------------------

Applied to all channels. This slows down the processing time,
but it sounds a lot better (especially as there's nothing else
to stop Nyquist foldback shenanigans).

At the moment, LPF only starts low and sweeps up. Lower values
slow the sweep speed. Q controls how audible/resonant this
effect is. Higher values are more resonant.

-----------------------
| foldback distortion |
-----------------------

Wave data above the foldback point is inverted. This produces
a really interesting kind of distortion - fancy clipping
really. I think the effect shines when enveloped; the foldback
point slowly raised, allowing the original waveform to sound.

Worth noting that this does not follow the volume set in the
MMML data, so if you're gently clipping at v5, v4 will
probably have no distortion and v6 will have loads. Might
change this in future.

Although there's no foldback distortion decay rate, because
there's a volume decay parameter, you get the same effect as
the waveform amplitude slowly drops below the foldback point.

---------------------------
| instrument volume boost |
---------------------------

A pretty straightforward boost for the instrument. Applied
after all the other instrument effects. Different from the
'voice_boost' array as this is applied to the instrument only
and is not fixed for the whole channel.

---------
| delay |
---------

Stereo delay effect. You can think of the 'offset' parameter as
BPM (if you like, I do). Higher values slow the delay rate
down. The 'feedback' parameter is really 'feedback reduction',
or: by how much the volume should be reduced per delay. Higher
values reduce the feedback volume more, preventing screeching.

This is distinct from mix, which could have low feedback
values, but just mixed quieter overall. 'Direction' selects
either L or R as the first direction for the stereo 'ping-pong'
effect (bouncing the delay between L and R).

----------
| reverb |
----------

It's just Freeverb and 100% stolen from rxi and Jezar at
Dreampoint. Parameters are standard reverb parameters; all
values go from off (0) to very intense (255). More info here:
https://ccrma.stanford.edu/~jos/pasp/Freeverb.html

---------------
| voice boost |
---------------

A final boost. This boosts everything, including the effects.
I'm using it to just balance the drums at the moment, but it's
nice to have volume controls at multiple levels (there's three
at the moment, a pre-effect MMML 'v' command, a post-instrument
effect volume, and a this is a 'post-everything' gain.)

*******************************************************************************/

#define TOTAL_INSTRUMENTS     9
#define INSTRUMENT_PARAMETERS 12

static uint8_t instrument_bank [TOTAL_INSTRUMENTS][INSTRUMENT_PARAMETERS] =
{
//----------------------------------------------------------------------------------//
//   waveform  |   wave mix  | decay | tanh shape fx |   low pass   | fldbk | instr |
//   1 | 2 | 3 | 1->2 | 2->3 | speed | speed | stereo| speed |   Q  | dstrn | boost |
//-------------|-------------|-------|---------------|----------------------|-------|
// 0: squishy saw            |       |               |              |       |       |
   {  7, 14,  1,   200,   200,    200,    200,      1,    200,     0,      0,     40,},
// 1: wurlitzer              |       |               |              |       |       |
   {  1,  7,  0,     1,   200,    130,    100,     90,      0,     0,    100,     45,},
// 2: music box              |       |               |              |       |       |
   {  6,  0,  0,   200,   220,      1,      1,     10,      0,     0,      0,     76,},
// 3: modem lead             |       |               |              |       |       |
   {  7, 14,  0,   120,   150,    190,    210,     10,      0,     0,    200,    120,},
// 4: gritty bass            |       |               |              |       |       |
   {  0,  0,  3,     1,   252,    190,    255,     10,      0,     0,    120,    190,},
// 5: funky rhodes           |       |               |              |       |       |
   {  3,  1,  0,    20,    20,     20,    255,     90,    240,   200,    120,     70,},
// 6: clicky bass            |       |               |              |       |       |
   { 10,  0,  0,     1,     1,    190,    255,     10,      0,     0,    120,    190,},
// 7: soft rhodes            |       |               |              |       |       |
   {  0,  0,  0,   110,    20,    130,    255,     90,      0,     0,    100,     45,},
//----------------------------------------------------------------------------------//
};

#define TOTAL_FX 2

static uint8_t channel_fx [TOTAL_VOICES][TOTAL_FX] =
{
//----------------//
// reverb | delay //
//----------------//
// voice #0
   {   0,    1  },
// voice #1
   {   1,    4  },
// voice #2
   {   1,    4  },
// voice #3
   {   1,    4  },
// voice #4
   {   2,    4  },
// voice #5
   {   2,    4  },
// voice #6
   {   0,    1  },
//----------------//
};

#define TOTAL_DELAY_PRESETS     5
#define TOTAL_DELAY_PARAMETERS  5

static uint8_t delay_bank [TOTAL_DELAY_PRESETS][TOTAL_DELAY_PARAMETERS] =
{
//----------------------------------------------//
//  offset | feedbk | spread | dirctn |   mix  |
//---------|--------|--------|--------|--------|//
// 0: no delay      |        |        |        |
   {     0,       0,       0,       0,       0  },
// 1: soft delay L  |        |        |        |
   {   160,      99,      20,       1,      12  },
// 2: soft delay R  |        |        |        |
   {   161,      99,      20,       0,      12  },
// 3: classic stereo echo    |        |        |
   {   161,      20,      80,       0,      40  },
// 4: big delay     |        |        |        |
   {   161,      45,      20,       1,      30  },
//----------------------------------------------//
};

#define TOTAL_REVERB_PRESETS     5
#define TOTAL_REVERB_PARAMETERS  5

static uint8_t reverb_bank [TOTAL_REVERB_PRESETS][TOTAL_REVERB_PARAMETERS] =
{
//--------------------------------------//
//   width |  dry |  wet | damp | room |
//----------------|------|------|------|//
// 0: no reverb   |      |      |      |
   {     0,   100,     0,     0,     0 },
// 1: a dusting   |      |      |      |
   {   150,   100,    15,    10,    10 },
// 2: a little space     |      |      |
   {   150,   100,    15,    50,    30 },
// 3: roomy       |      |      |      |
   {   200,   100,    20,    60,    80 },
// 4: spacious    |      |      |      |
   {   200,   100,    20,   100,    80 },
//--------------------------------------//
};

static uint8_t voice_boost [TOTAL_VOICES] =
{
//------------------------------------------//
//  <-------------------------------> | drm |
//  #0  | #1  | #2  | #3  | #4  | #5  | #6  |
//------|-----|-----|-----|-----|-----|-----//
     180,  127,  127,  127,  127,  127, 110
//------------------------------------------//
};