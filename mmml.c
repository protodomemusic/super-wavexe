/**************************************************************
*  FILENAME:     mmml.c
*  DESCRIPTION:  Self-contained μMML engine for powering the
*                sequencing of music software. For more
*                information on the μMML language, see the
*                following URL:
*                https://github.com/protodomemusic/mmml
*
*  DATE:         29th December 2022
**************************************************************/

// the data file to read from
#include "wavexe-mmml-data.h"

#define MAXLOOPS         5        // total number of nested loops
#define TOTAL_NOTES      108      // how many notes we'll generate
#define NOTE_END         7902.13  // the highest frequency we'll generate
#define MASTER_TRANSPOSE 31       // master pitch offset

// mmml variables
float    mmml_volume       [TOTAL_VOICES];
uint16_t mmml_octave       [TOTAL_VOICES];
uint16_t mmml_length       [TOTAL_VOICES];
uint16_t mmml_note         [TOTAL_VOICES];
int8_t   mmml_transpose    [TOTAL_VOICES];
int8_t   mmml_panning      [TOTAL_VOICES];
int8_t   mmml_tie_flag     [TOTAL_VOICES];
uint8_t  loops_active      [TOTAL_VOICES];
uint8_t  current_length    [TOTAL_VOICES];
uint16_t data_pointer      [TOTAL_VOICES];
uint16_t loop_duration     [MAXLOOPS][TOTAL_VOICES];
uint16_t loop_point        [MAXLOOPS][TOTAL_VOICES];
uint16_t pointer_location  [TOTAL_VOICES];
float    notes             [TOTAL_NOTES];

// run once at startup to initialize μMML variables
void mmml_setup()
{
	// initial mmml variables
	for (uint8_t v = 0; v < TOTAL_VOICES; v++)
	{
		mmml_volume[v] = 0.5;
		mmml_octave[v] = 2;
	}

	// generate frequencies for pitch lookup table
	for(uint8_t i = 0; i < TOTAL_NOTES; i++)
	{
		float frequency = NOTE_END / pow(1.059463094, TOTAL_NOTES - i);
		notes[i] = frequency;
	}

	// get locations of data from file
	for (uint8_t v = 0; v < TOTAL_VOICES; v++)
	{
		data_pointer [v] = (uint8_t)mmml_data[v * 2] << 8;
		data_pointer [v] = data_pointer[v] | (uint8_t)mmml_data[(v*2)+1];
	}
}

// call every frame
void mmml(float pitch_io[], float volume_io[], uint8_t instrument_io[], uint8_t note_on_io[])
{
	// main timer variables
	static uint16_t tick_counter = 0;
	static uint16_t tick_speed   = 700; // default tempo

	// temporary data storage variables
	uint8_t  buffer1 = 0;
	uint8_t  buffer2 = 0;
	uint8_t  buffer3 = 0;
	uint16_t buffer4 = 0;

	if (tick_counter-- == 0)
	{
		// Variable tempo, sets the fastest / smallest possible clock event.
		tick_counter = tick_speed;

		for (uint8_t v = 0; v < TOTAL_VOICES; v++)
		{
			// If the note ended, start processing the next byte of data.
			if (mmml_length[v] == 0)
			{
				LOOP:

				// Temporary storage of data for quick processing.
				// first nibble of data
				buffer1 = ((uint8_t)mmml_data[(data_pointer[v])] >> 4);
				// second nibble of data
				buffer2 = (uint8_t)mmml_data[data_pointer[v]] & 0x0F;

				// function command
				if (buffer1 == 15)
				{
					// Another buffer for commands that require an additional byte.
					buffer3 = (uint8_t)mmml_data[data_pointer[v] + 1];

					// loop start
					if (buffer2 == 0)
					{
						loops_active[v]++;
						loop_point[loops_active[v] - 1][v] = data_pointer[v] + 2;
						loop_duration[loops_active[v] - 1][v] = buffer3 - 1;
						data_pointer[v]+= 2;
					}
					// loop end
					else if (buffer2 == 1)
					{
						/* If we reach the end of the loop and the duration isn't zero,
						 * decrement our counter and start again. */
						if(loop_duration[loops_active[v] - 1][v] > 0)
						{
							data_pointer[v] = loop_point[loops_active[v] - 1 ][v];
							loop_duration[loops_active[v] - 1][v]--;
						}
						// If we're done, move away from the loop.
						else
						{
							loops_active[v]--;
							data_pointer[v]++;
						}
					}
					// macro
					else if (buffer2 == 2)
					{
						pointer_location[v] = data_pointer[v] + 2;
						
						data_pointer[v] = (uint8_t)mmml_data[(buffer3 + TOTAL_VOICES) * 2] << 8;
						data_pointer[v] = data_pointer[v] | (uint8_t)mmml_data[((buffer3 + TOTAL_VOICES) * 2) + 1];
					}
					// tempo
					else if (buffer2 == 3)
					{
						tick_speed = buffer3 * 14;
						data_pointer[v] += 2;
					}
					// transpose
					else if (buffer2 == 4)
					{
						mmml_transpose[v] = buffer3;
						data_pointer[v] += 2;
					}
					// instrument
					else if (buffer2 == 5)
					{
						instrument_io[v] = buffer3;
						data_pointer[v] += 2;
					}
					// tie
					else if (buffer2 == 6)
					{
						mmml_tie_flag[v] = 1;
						data_pointer[v]++;
					}
					// panning
					else if (buffer2 == 7)
					{
						mmml_panning[v]  = buffer3;
						data_pointer[v] += 2;
					}
					// channel end
					else if (buffer2 == 15)
					{
						// If we've got a previous position saved, go to it...
						if (pointer_location[v] != 0)
						{
							data_pointer[v] = pointer_location[v];
							pointer_location[v] = 0;
						}
						// ...If not, go back to the start.
						else
						{
							data_pointer[v] = (uint8_t) mmml_data[(v*2)] << 8;
							data_pointer[v] = data_pointer[v] | (uint8_t) mmml_data[(v*2)+1];
						}
					}

					/* For any command that should happen 'instantaneously' (e.g. anything
					 * that isn't a note or rest - in mmml notes can't be altered once
					 * they've started playing), we need to keep checking this loop until we
					 * find an event that requires waiting. */

					goto LOOP;
				}

				if (buffer1 == 13 || buffer1 == 14)
				{
					// octave
					if(buffer1 == 13)
						mmml_octave[v] = buffer2;
					// volume
					if(buffer1 == 14)
						mmml_volume[v] = buffer2 / 8.0;

					data_pointer[v]++;
					goto LOOP; // see comment above previous GOTO
				}

				// note value
				if (buffer1 != 0 && buffer1 < 14)
				{
					// NOTE! I quickly changed the behaviour of ties so that it will update the note pitch
					// it probably needs some looking at, so don't forget.

					// trigger the oscillators
					if (v < TOTAL_VOICES)
					{
						mmml_note[v] = buffer1;

						if (mmml_tie_flag[v] == 0)
						{
							// reset the lfo

							// update the instrument bank originally here
						}

						pitch_io[v] = notes[(mmml_note[v]+(mmml_octave[v]*12)) + mmml_transpose[v] + MASTER_TRANSPOSE];

						if (mmml_tie_flag[v] == 0)
						{							
							// reset the note properties by triggering a note on event
							note_on_io[v] = 1;

							// update the oscillator volume
							volume_io[v] = mmml_volume[v];

							// if (env_vol_type[v] == 0)
							// 	volume_io[v] = mmml_volume[v];
							// else
							// 	volume_io[v] = 0.0;

							// reset the volume decay envelope speed

							// update wavetable originally here
						}
					}

					if (mmml_tie_flag[v] == 1)
						mmml_tie_flag[v] = 0;
				}
				// rest
				else
				{
					// trigger the oscillators
					if (v < TOTAL_VOICES)
					{
						// update the instrument bank originally here
						
						// avoid clicky note-offs by turning on the decay envelope
						note_on_io[v] = 2;
						
					}

					// clear tie flag here too
					mmml_tie_flag[v] = 0;
				}

				// note duration value
				if (buffer2 < 8)
					// standard duration
					mmml_length[v] = 127 >> buffer2;
				else if(buffer2 == 15)
				{
					// durationless note
					data_pointer[v]++;
					goto LOOP;
				}
				else
					// dotted (1 + 1/2) duration
					mmml_length[v] = 95 >> (buffer2 & 7);

				// next element in data
				data_pointer[v]++;
			}
			// keep waiting until the note is over...
			else
				mmml_length[v]--;
		}
	}
}