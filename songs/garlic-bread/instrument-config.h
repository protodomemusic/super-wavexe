/*******************************************************************************
*  FILENAME:     instrument-config.h
*  SONG:         I Want Garlic Bread
*  DESCRIPTION:  Tables for instrument/effects configuration.
*
*  DATE:         27th March 2024
*******************************************************************************/

//-------------------------//

// duration of the song in seconds
#define PLAY_TIME  151

// master limiter parameters
#define MASTER_LIMITER_THRESHOLD  0.7
#define MASTER_LIMITER_ATTACK_MS  10
#define MASTER_LIMITER_RELEASE_MS 10

// master pitch offset for 4ml.c
#define MASTER_TRANSPOSE 29

//-------------------------//

#define TOTAL_INSTRUMENTS     11
#define INSTRUMENT_PARAMETERS 13

static uint8_t instrument_bank [TOTAL_INSTRUMENTS][INSTRUMENT_PARAMETERS] =
{
//------------------------------------------------------------------------------------------//
//   waveform  |   wave mix  | decay | tanh shape fx |   low pass   | fldbk | instr | glide |
//   1 | 2 | 3 | 1->2 | 2->3 | speed | speed | stereo| speed |   Q  | dstrn | boost | time  |
//-------------|-------------|-------|---------------|----------------------|-------|-------//
// 0: squishy saw            |       |               |              |       |       |       |
   {  7, 15,  0,    210,  200,    200,    100,      1,     30,   255,     20,    100,   150, },
// 1: wurlitzer              |       |               |              |       |       |       |
   {  1,  7,  0,     1,   200,    130,    100,     90,      0,     0,    100,     45,     0, },
// 2: steel drum             |       |               |              |       |       |       |
   {  12, 6,  0,     1,   220,     30,      1,     10,     10,     0,     80,     86,     0, },
// 3: modem lead             |       |               |              |       |       |       |
   {  7, 14,  0,   120,   150,    190,    210,     10,      0,     0,    200,    120,     0, },
// 4: saw bass               |       |               |              |       |       |       |
   {  7, 15,  3,   120,   200,    255,      0,      0,      1,   100,    100,    170,   220, },
// 5: funky rhodes           |       |               |              |       |       |       |
   {  3,  1,  0,    20,    20,     20,    255,     90,    240,   200,    110,     70,     0, },
// 6: electric piano         |       |               |              |       |       |       |
   {  1,  7,  0,   120,   180,    180,    100,     90,      2,    10,    140,     70,     0, },
// 7: moog accordion         |       |               |              |       |       |       |
   { 21,  7,  0,     0,   180,    200,    100,     90,     10,    10,    140,     70,     0, },
// 8: soft rhodes            |       |               |              |       |       |       |
   {  0,  0,  0,   110,    20,    130,    255,     90,      0,     0,    100,     45,     0, },
// 9: saw bass slide         |       |               |              |       |       |       |
   {  7, 15,  3,   120,   200,    255,      0,      0,      1,   100,    100,    170,   254, },
// 10: squishy saw slide     |       |               |              |       |       |       |
   {  7, 15,  0,    210,  200,    200,    100,      1,     30,   255,     20,    100,   240, },
//------------------------------------------------------------------------------------------//
};

#define TOTAL_FX 4

static uint8_t channel_fx [TOTAL_VOICES][TOTAL_FX] =
{
//----------------------------------//
// eq    | limiter | delay | reverb |
//----------------------------------//
// voice #0        |       |        |
   {   1,        4,      1,       3  },
// voice #1        |       |        |
   {   1,        4,      2,       3  },
// voice #2        |       |        |
   {   1,        4,      1,       3  },
// voice #3        |       |        |
   {   1,        4,      2,       3  },
// voice #4        |       |        |
   {   4,        3,      0,       0  },
// voice #5        |       |        |
   {   2,        2,      4,       0  },
// voice #6        |       |        |
   {   0,        1,      3,       4  },
// voice #7        |       |        |
   {   0,        0,      0,       2  },
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
   {     0,     210,       0,      40,       1,      30  },
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

#define TOTAL_DELAY_PRESETS     5
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
// 4: spacious    |      |      |      |
   {   200,   100,    20,   100,    80  },
//--------------------------------------//
};

static uint8_t voice_boost [TOTAL_VOICES] =
{
//------------------------------------------------//
//  <---------------- osc ----------------> | drm |
//  #0  | #1  | #2  | #3  | #4  | #5  | #6  | #7  |
//------|-----|-----|-----|-----|-----|-----|-----//
      80,   80,   80,   80,  100,  130,  240,  100
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