/**************************************************************
*  FILENAME:     mmml.c
*  DESCRIPTION:  Self-contained μMML engine for powering the
*                sequencing of music software. For more
*                information on the μMML language, see the
*                following URL:
*                https://github.com/protodomemusic/mmml
*
*                Unlike the above, this one only processes one
*                voice at a time.
*
*  DATE:         29th December 2022
**************************************************************/

// the data file to read from
#include "wavexe-mmml-data.h"

#define MAXLOOPS         5        // total number of nested loops
#define TOTAL_NOTES      108      // how many notes we'll generate
#define NOTE_END         7902.13  // the highest frequency we'll generate
#define MASTER_TRANSPOSE 35       // master pitch offset
#define DEFAULT_TEMPO    640

// mmml variables
float    mmml_volume;
uint16_t mmml_octave;
uint16_t mmml_length;
uint16_t mmml_note;
int8_t   mmml_transpose;
int8_t   mmml_tie_flag;
uint8_t  loops_active;
uint16_t data_pointer;
uint16_t pointer_location;
uint16_t loop_duration    [MAXLOOPS];
uint16_t loop_point       [MAXLOOPS];
float    notes            [TOTAL_NOTES];

// main timer variables
uint16_t tick_counter;
uint16_t tick_speed;

// run once at startup to initialize μMML variables
void mmml_setup(uint8_t v)
{
	tick_counter = 0;
	tick_speed   = DEFAULT_TEMPO;

	// initial mmml variables
	mmml_volume    = 0.5;
	mmml_octave    = 2;
	mmml_length    = 4;
	mmml_note      = 24;
	mmml_transpose = 0;
	mmml_tie_flag  = 0;
	loops_active   = 0;

	for (int i = 0; i < MAXLOOPS; ++i)
	{
		loop_duration [i] = 0;
		loop_point    [i] = 0;
	}

	// generate frequencies for pitch lookup table
	for(uint8_t i = 0; i < TOTAL_NOTES; i++)
	{
		float frequency = NOTE_END / pow(1.059463094, TOTAL_NOTES - i);
		notes[i] = frequency;
	}

	// get locations of data from file
	data_pointer = (uint8_t)mmml_data[v * 2] << 8;
	data_pointer = data_pointer | (uint8_t)mmml_data[(v*2)+1];
}

// call every frame
void mmml_update(float *pitch_io, float *volume_io, uint8_t *instrument_io, uint8_t *note_on_io, uint8_t *panning_io, uint8_t v)
{
	// temporary data storage variables
	uint8_t  buffer1 = 0;
	uint8_t  buffer2 = 0;
	uint8_t  buffer3 = 0;
	uint16_t buffer4 = 0;

	if (tick_counter-- == 0)
	{
		// Variable tempo, sets the fastest / smallest possible clock event.
		tick_counter = tick_speed;

		// If the note ended, start processing the next byte of data.
		if (mmml_length == 0)
		{
			LOOP:

			// Temporary storage of data for quick processing.
			// first nibble of data
			buffer1 = ((uint8_t)mmml_data[data_pointer] >> 4);
			// second nibble of data
			buffer2 = (uint8_t)mmml_data[data_pointer] & 0x0F;

			// function command
			if (buffer1 == 15)
			{
				// Another buffer for commands that require an additional byte.
				buffer3 = (uint8_t)mmml_data[data_pointer + 1];

				// loop start
				if (buffer2 == 0)
				{
					loops_active++;
					loop_point[loops_active - 1] = data_pointer + 2;
					loop_duration[loops_active - 1] = buffer3 - 1;
					data_pointer += 2;
				}
				// loop end
				else if (buffer2 == 1)
				{
					/* If we reach the end of the loop and the duration isn't zero,
					 * decrement our counter and start again. */
					if(loop_duration[loops_active - 1] > 0)
					{
						data_pointer = loop_point[loops_active - 1 ];
						loop_duration[loops_active - 1]--;
					}
					// If we're done, move away from the loop.
					else
					{
						loops_active--;
						data_pointer++;
					}
				}
				// macro
				else if (buffer2 == 2)
				{
					pointer_location = data_pointer + 2;
					
					data_pointer = (uint8_t)mmml_data[(buffer3 + TOTAL_VOICES) * 2] << 8;
					data_pointer = data_pointer | (uint8_t)mmml_data[((buffer3 + TOTAL_VOICES) * 2) + 1];
				}
				// tempo
				else if (buffer2 == 3)
				{
					tick_speed = buffer3 * 14;
					data_pointer += 2;
				}
				// transpose
				else if (buffer2 == 4)
				{
					mmml_transpose  = buffer3;
					data_pointer += 2;
				}
				// instrument
				else if (buffer2 == 5)
				{
					*instrument_io = buffer3;
					data_pointer += 2;
				}
				// tie
				else if (buffer2 == 6)
				{
					mmml_tie_flag = 1;
					data_pointer++;
				}
				// panning
				else if (buffer2 == 7)
				{
					*panning_io = buffer3;
					data_pointer += 2;
				}
				// channel end
				else if (buffer2 == 15)
				{
					// If we've got a previous position saved, go to it...
					if (pointer_location != 0)
					{
						data_pointer = pointer_location;
						pointer_location = 0;
					}
					// ...If not, go back to the start.
					else
					{
						data_pointer = (uint8_t) mmml_data[(v*2)] << 8;
						data_pointer = data_pointer | (uint8_t) mmml_data[(v*2)+1];
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
					mmml_octave = buffer2;
				// volume
				if(buffer1 == 14)
					mmml_volume = buffer2 / 8.0;

				data_pointer++;
				goto LOOP; // see comment above previous GOTO
			}

			// note value
			if (buffer1 != 0 && buffer1 < 14)
			{
				// set the note and pitch
				mmml_note = buffer1;
				*pitch_io = notes[(mmml_note + (mmml_octave*12)) + mmml_transpose + MASTER_TRANSPOSE];

				// assuming the last channel is a drum
				// remove if you don't want this behavior
				if (v == TOTAL_VOICES - 1)
					*pitch_io = mmml_note;

				if (mmml_tie_flag == 0)
				{
					// reset the note properties by triggering a note on event
					*note_on_io = 1;

					// update the oscillator volume
					*volume_io = mmml_volume;
				}

				if (mmml_tie_flag == 1)
					mmml_tie_flag = 0;
			}
			// rest
			else
			{
				// avoid clicky note-offs by turning on a release envelope
				*note_on_io = 2;

				// clear tie flag here too
				mmml_tie_flag = 0;
			}

			// note duration value
			if (buffer2 < 8)
				// standard duration
				mmml_length = 127 >> buffer2;
			else if(buffer2 == 15)
			{
				// durationless note
				data_pointer++;
				goto LOOP;
			}
			else
				// dotted (1 + 1/2) duration
				mmml_length = 95 >> (buffer2 & 7);

			// next element in data
			data_pointer++;
		}
		// keep waiting until the note is over...
		else
			mmml_length--;
	}
}