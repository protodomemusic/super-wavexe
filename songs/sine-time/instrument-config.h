/**************************************************************
*  FILENAME:     instrument-config.h
*  SONG:         Sine Time
*  DESCRIPTION:  Tables for instrument/effects configuration.
*
*  DATE:         29th March 2023
**************************************************************/

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
// 6: gritty bass (but quiet)|       |               |              |       |       |
   {  0,  0,  3,     1,   252,    190,    255,     10,      0,     0,    120,    100,},
// 7: clicky bass            |       |               |              |       |       |
   { 10,  0,  0,     1,     1,    190,    255,     10,      0,     0,    120,    180,},
// 8: soft rhodes            |       |               |              |       |       |
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
   {   1,    3  },
// voice #1
   {   2,    3  },
// voice #2
   {   1,    3  },
// voice #3
   {   3,    3  },
// voice #4
   {   4,    3  },
// voice #5
   {   3,    4  },
// voice #6
   {   0,    1  },
// voice #7
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
//------------------------------------------------//
//  <---------------- osc ----------------> | drm |
//  #0  | #1  | #2  | #3  | #4  | #5  | #6  | #7  |
//------|-----|-----|-----|-----|-----|-----|-----//
     127,  127,  127,  127,  80,  127,  125,  80
//------------------------------------------------//
};

// // silence stuff for mixing
// static uint8_t voice_boost [TOTAL_VOICES] =
// {
// //------------------------------------------------//
// //  <---------------- osc ----------------> | drm |
// //  #0  | #1  | #2  | #3  | #4  | #5  | #6  | #7  |
// //------|-----|-----|-----|-----|-----|-----|-----//
//        0,    0,    0,    0,    0,    0,    0,  140
// //------------------------------------------------//
// };