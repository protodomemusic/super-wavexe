/*******************************************************************************
*  FILENAME:     instrument-config.h
*  SONG:         Sine Time
*  DESCRIPTION:  Tables for instrument/effects configuration.
* 
*  NOTES:        This probably needs a little more TLC as it's converted from an
*                older version. Don't judge me too harshly on how this sounds.
*
*  DATE:         29th March 2023
*                25th May 2024 (small update for new format)
*******************************************************************************/

//-------------------------//

// duration of the song in seconds
#define PLAY_TIME  160

// master limiter parameters
#define MASTER_LIMITER_THRESHOLD  0.6
#define MASTER_LIMITER_ATTACK_MS  10
#define MASTER_LIMITER_RELEASE_MS 10

// master pitch offset for 4ml.c
#define MASTER_TRANSPOSE 38

//-------------------------//

#define TOTAL_INSTRUMENTS     9
#define INSTRUMENT_PARAMETERS 13

static uint8_t instrument_bank [TOTAL_INSTRUMENTS][INSTRUMENT_PARAMETERS] =
{
//------------------------------------------------------------------------------------------//
//   waveform  |   wave mix  | decay | tanh shape fx |   low pass   | fldbk | instr | glide |
//   1 | 2 | 3 | 1->2 | 2->3 | speed | speed | stereo| speed |   Q  | dstrn | boost | time  |
//-------------|-------------|-------|---------------|----------------------|-------|-------//
// 0: squishy saw            |       |               |              |       |       |       |
   {  7, 14,  1,   200,   200,    200,    200,      1,    240,     0,      0,     40,   100, },
// 1: wurlitzer              |       |               |              |       |       |       |
   {  1,  7,  0,     1,   200,    130,    100,     90,      0,     0,    100,     45,     0, },
// 2: music box              |       |               |              |       |       |       |
   {  6,  0,  0,   200,   220,      1,      1,     10,      0,     0,      0,     76,     0, },
// 3: modem lead             |       |               |              |       |       |       |
   {  7, 14,  0,   120,   150,    190,    210,     10,      0,     0,    200,    120,     0, },
// 4: gritty bass            |       |               |              |       |       |       |
   {  7,  0,  3,     1,   252,    190,    255,     10,      0,     0,    120,    190,   100, },
// 5: funky rhodes           |       |               |              |       |       |       |
   {  3,  1,  0,    20,    20,     20,    255,     90,    240,   200,    120,     70,     0, },
// 6: gritty bass (but quiet)|       |               |              |       |       |       |
   {  7,  0,  3,     1,   252,    190,    255,     10,      0,     0,    120,    100,   100, },
// 7: clicky bass            |       |               |              |       |       |       |
   { 10,  0,  0,     1,     1,    190,    255,     10,      0,     0,    120,    180,   100, },
// 8: soft rhodes            |       |               |              |       |       |       |
   {  8,  0,  0,     1,    20,    130,    255,     90,      0,     0,     40,     40,     0, },
//-----------------------------------------------------------------------------------------//
};

#define TOTAL_FX 4

static uint8_t channel_fx [TOTAL_VOICES][TOTAL_FX] =
{
//----------------------------------//
// eq    | limiter | delay | reverb |
//----------------------------------//
// voice #0        |       |        |
   {   1,        3,      1,       2  },
// voice #1        |       |        |
   {   1,        3,      2,       2  },
// voice #2        |       |        |
   {   1,        3,      1,       2  },
// voice #3        |       |        |
   {   1,        3,      3,       2  },
// voice #4        |       |        |
   {   0,        1,      4,       2  },
// voice #5        |       |        |
   {   0,        0,      3,       4  },
// voice #6        |       |        |
   {   4,        1,      0,       1  },
// voice #7        |       |        |
   {   0,        0,      0,       1  },
//----------------------------------//
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
   {     0,     210,       0,      40,       1,      30  },
// 2: low boost     |        |        |        |        |
   {     1,       0,      10,       0,       2,      30  },
// 3: radio         |        |        |        |        |
   {    10,      30,      20,      80,       2,     100  },
// 4: low cut       |        |        |        |        |
   {     0,     210,       0,      40,       2,      40  },
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
//------------------------------------------------//
//  <---------------- osc ----------------> | drm |
//  #0  | #1  | #2  | #3  | #4  | #5  | #6  | #7  |
//------|-----|-----|-----|-----|-----|-----|-----//
      60,   60,   60,   60,   75,   40,  200,   30
//------------------------------------------------//
};

// // silence stuff for mixing
// static uint8_t voice_boost [TOTAL_VOICES] =
// {
// //------------------------------------------------//
// //  <---------------- osc ----------------> | drm |
// //  #0  | #1  | #2  | #3  | #4  | #5  | #6  | #7  |
// //------|-----|-----|-----|-----|-----|-----|-----//
//        0,    0,    0,    0,   70,    0,    0,    0
// //------------------------------------------------//
// };