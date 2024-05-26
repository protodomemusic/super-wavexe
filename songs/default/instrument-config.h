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

Yo! I've left this in for now, but it's going to be superseded
by the fx.md document in the 'docs' directory.

VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV

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
channel to create a subtle stereo effect.

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

//-------------------------//

// duration of the song in seconds
#define PLAY_TIME  10

// master limiter parameters
#define MASTER_LIMITER_THRESHOLD  0.7
#define MASTER_LIMITER_ATTACK_MS  10
#define MASTER_LIMITER_RELEASE_MS 10

// master pitch offset for 4ml.c
#define MASTER_TRANSPOSE 29

//-------------------------//

#define TOTAL_INSTRUMENTS     15
#define INSTRUMENT_PARAMETERS 13

static uint8_t instrument_bank [TOTAL_INSTRUMENTS][INSTRUMENT_PARAMETERS] =
{
//------------------------------------------------------------------------------------------//
//   waveform  |   wave mix  | decay | tanh shape fx |   low pass   | fldbk | instr | glide |
//   1 | 2 | 3 | 1->2 | 2->3 | speed | speed | stereo| speed |   Q  | dstrn | boost | time  |
//-------------|-------------|-------|---------------|----------------------|-------|-------//
// 0: plucked guitar         |       |               |              |       |       |       |
   { 11, 10,  1,    110,  200,    180,    200,    100,    180,   255,    140,    100,   150, },
// 1: space bass             |       |               |              |       |       |       |
   { 15,  3,  0,     20,  200,    190,    200,    100,      0,     0,    120,    100,   100, },
// 2: soft fm bass           |       |               |              |       |       |       |
   {  3,  2,  1,    220,  200,    170,     10,     10,     40,    90,      0,    120,   150, },
// 3: soft rhodes            |       |               |              |       |       |       |
   {  0,  0,  0,   110,    20,    130,    255,     90,      0,     0,    100,     95,     0, },
// 4: moog accordion         |       |               |              |       |       |       |
   { 21,  7,  0,     0,   180,    200,    100,     90,     10,    10,    140,     90,   100, },
// 5: brass pad              |       |               |              |       |       |       |
   { 21, 21,  0,     7,   195,    210,    255,    110,      1,   200,    100,     65,   240, },
// 6: electric piano         |       |               |              |       |       |       |
   {  1,  7,  0,   120,   180,    180,    100,     90,      2,    10,    140,     90,     0, },
// 7: funky xylophone        |       |               |              |       |       |       |
   {  3,  1,  0,    20,    20,     20,    255,     90,    240,   200,    110,     90,     0, },
// 8: squishy saw            |       |               |              |       |       |       |
   {  7, 15,  0,    210,  200,    200,    100,      1,     30,   255,     20,     60,   150, },
// 9: wurlitzer              |       |               |              |       |       |       |
   {  1,  7,  0,     1,   200,    130,    100,     90,      0,     0,    100,     65,     0, },
//10: steel drum/music box   |       |               |              |       |       |       |
   {  12, 6,  0,     1,   220,     30,      1,     10,     10,     0,     80,    106,     0, },
//11: modem lead             |       |               |              |       |       |       |
   {  7, 14,  0,   120,   150,    190,    210,     10,      0,     0,    200,    140,     0, },
//12: saw bass               |       |               |              |       |       |       |
   {  7, 15,  3,   120,   200,    255,      0,      0,      1,   100,    100,    100,   220, },
//13: saw pad                |       |               |              |       |       |       |
   { 21, 15, 16,   107,   200,    190,    255,    110,      1,   200,    100,     40,   200, },
//14: soft arp               |       |               |              |       |       |       |
   {  7, 15,  0,   210,   200,    200,    100,      1,     30,   255,     20,     60,     0, },
//------------------------------------------------------------------------------------------//
};

#define TOTAL_FX 4

static uint8_t channel_fx [TOTAL_VOICES][TOTAL_FX] =
{
//----------------------------------//
// eq    | limiter | delay | reverb |
//----------------------------------//
// voice #0        |       |        |
   {   4,        4,      0,       4  },
// voice #1        |       |        |
   {   1,        4,      5,       4  },
// voice #2        |       |        |
   {   1,        4,      5,       4  },
// voice #3        |       |        |
   {   1,        4,      5,       4  },
// voice #4        |       |        |
   {   1,        4,      5,       4  },
// voice #5        |       |        |
   {   0,        4,      5,       4  },
// voice #6        |       |        |
   {   0,        4,      5,       4  },
// voice #7        |       |        |
   {   0,        4,      5,       4  },
//-----------------------------------//
};

#define TOTAL_EQ_PRESETS     5
#define TOTAL_EQ_PARAMETERS  6

static uint8_t eq_bank [TOTAL_EQ_PRESETS][TOTAL_EQ_PARAMETERS] =
{
//------------------------------------------------------//
//  lopass | hipass | low q  | hi q   | intens |   mix  |
//---------|--------|--------|--------|--------|--------//
// 0: no eq         |        |        |        |        |
   {     0,       0,       0,       0,       0,       0  },
// 1: high boost    |        |        |        |        |
   {     0,     210,       0,      40,       1,      40  },
// 2: low boost     |        |        |        |        |
   {     1,       0,      10,       0,       2,      30  },
// 3: radio         |        |        |        |        |
   {    10,      30,      20,      80,       2,     100  },
// 4: low cut       |        |        |        |        |
   {     0,     210,       0,      40,       2,      30  },
//-------------------------------------------------------//
};

#define TOTAL_LIMITER_PRESETS     5
#define TOTAL_LIMITER_PARAMETERS  3

static uint8_t limiter_bank [TOTAL_LIMITER_PRESETS][TOTAL_LIMITER_PARAMETERS] =
{
//---------------------------//
//  attack | release | limit | 
//---------|---------|-------//
// 0: transparent limiter    | 
   {    10,       10,    100, },
// 1: super hard squash      | 
   {    10,       10,     10, },
// 2: hard squash    |       |
   {    10,       10,     30, },
// 3: somewhat compress      |
   {    10,       10,     50, },
// 4: gently limit   |       |
   {    10,       10,     70, },
//----------------------------//
};

#define TOTAL_DELAY_PRESETS     6
#define TOTAL_DELAY_PARAMETERS  5

static uint8_t delay_bank [TOTAL_DELAY_PRESETS][TOTAL_DELAY_PARAMETERS] =
{
//---------------------------------------------//
//  offset | feedbk | spread | dirctn |   mix  |
//---------|--------|--------|--------|--------//
// 0: no delay      |        |        |        |
   {     0,       0,       0,       0,       0  },
// 1: soft delay L  |        |        |        |
   {   161,      99,      20,       1,       8  },
// 2: soft delay R  |        |        |        |
   {   162,      99,      20,       0,       8  },
// 3: classic stereo echo    |        |        |
   {   162,      20,      80,       0,      50  },
// 4: big delay     |        |        |        |
   {   162,      45,      20,       1,      50  },
// 5: vast delay    |        |        |        |
   {   162,      45,      20,       1,      70  },
//----------------------------------------------//
};

#define TOTAL_REVERB_PRESETS     5
#define TOTAL_REVERB_PARAMETERS  5

static uint8_t reverb_bank [TOTAL_REVERB_PRESETS][TOTAL_REVERB_PARAMETERS] =
{
//-------------------------------------//
//   width |  dry |  wet | damp | room |
//----------------|------|------|------//
// 0: no reverb   |      |      |      |
   {     0,   100,     0,     0,     0  },
// 1: a dusting   |      |      |      |
   {   150,   100,    15,    10,    10  },
// 2: a little space     |      |      |
   {   150,   100,    15,    50,    30  },
// 3: roomy       |      |      |      |
   {   200,   100,    20,    60,    80  },
// 4: dreamy      |      |      |      |
   {   200,   100,    60,   100,    90  },
//--------------------------------------//
};

static uint8_t voice_boost [TOTAL_VOICES] =
{
//------------------------------------------------//
//  <---------------- osc ----------------> | drm |
//  #0  | #1  | #2  | #3  | #4  | #5  | #6  | #7  |
//------|-----|-----|-----|-----|-----|-----|-----//
     200,   80,   80,   80,   80,   80,   80,    80
//------------------------------------------------//
};

// // silence stuff for mixing
// static uint8_t voice_boost [TOTAL_VOICES] =
// {
// //------------------------------------------------//
// //  <---------------- osc ----------------> | drm |
// //  #0  | #1  | #2  | #3  | #4  | #5  | #6  | #7  |
// //------|-----|-----|-----|-----|-----|-----|-----//
//        0,    0,    0,    0,    0,  100,    0,    0
// //------------------------------------------------//
// };