################################################################################
#  FILENAME:     midi-to-4ml.py
#  DESCRIPTION:  MIDI to Mega Micro Music Macro Language (4ML) Converter
#
#  NOTES:        Real bare-bones converter to convert MIDI files into something
#                that 4ML players will be able to interpret. It's in such early
#                stages that it's packed full of bugs and inaccuracies, plus the
#                4ML it generates is pretty garbage.
#
#                That said, it's been a really useful tool and works well enough
#                if you're willing to cleanup the MIDI file before import and
#                the converted code on export.
#
#                Make sure your channels are free of overlapping notes and
#                quantized to 128th notes minimum. The script should throw a
#                warning if you mess this up somewhere, plus it'll go ahead and
#                convert the rest anyway, but you might get timing issues.
#
#                TO-DO:
#                - Go though and comment stuff, because I rushed this for a
#                  project I'll forget how it works if I leave it too long.
#                - Export the 4ML to a file, rather than just print to
#                  terminal.
#                - The 4ML is ugly when printed too. Maybe add a space next to
#                  rests? Basically decide on a pretty format for the language.
#                - FIX THE BUGZ
#
#                BUGS:
#                - Sometimes it will incorrectly interpret a 128th note as
#                  being shorter than it really is. Generally this is resolved
#                  automatically with no impact on MML timing, but you'll still
#                  get a warning. I think this is down to tick divisions not
#                  being 100% accurate over time, so I don't know how this is
#                  avoided in other conversion software.
#                - Issue with removing repeated note lengths for tied durations.
#                  For example, g4&g32g4&g32 becomes g4&g32gg
#                - Leading rests are ignored when processing any additional
#                  channels other than the first. For example if the MIDI went:
#                  C1 r4c8 C2 r4c8, it would processed as C1 r4c8 C2 c8
#                - Newlines are incorrectly inserted one character after a new
#                  bar starts. So you get stuff like:
#                  C1 v4 o2 c4c4c4c4 v4 o2 c4
#                  c4c4c4
#                - Bar numbers are wrong when reporting errors.
#
#  AUTHOR:       Blake 'PROTODOME' Troise
#  PLATFORM:     Command Line Application
#  DATE:         8th January 2024
################################################################################

#############
# libraries #
#############

import mido
import sys

# set this to 1 to enable printing, or to 0 to disable
debug = 0

# set this to 1 to enable velocity conversion
velocity_print = 1

# set this to 1 to hide durations on repeat
duration_repeat = 0

# the number of 128th notes in a bar (mainly for printing bars cleanly and error messages)
# 4/4 = 128, 3/4 = 96
time_signature = 128

###################
# terminal colors #
###################

# Thanks to Peter Mortensen for the code:
# https://stackoverflow.com/questions/287871/how-do-i-print-colored-text-to-the-terminal

class bcolors:
    HEADER    = '\033[95m'
    OKBLUE    = '\033[94m'
    OKCYAN    = '\033[96m'
    OKGREEN   = '\033[92m'
    WARNING   = '\033[93m'
    FAIL      = '\033[91m'
    ENDC      = '\033[0m'
    BOLD      = '\033[1m'
    UNDERLINE = '\033[4m'

##################
# oh no an error #
##################

def throw_error(error_number,additional_info):
    print(f"{bcolors.FAIL}ERROR: {bcolors.ENDC}", end = '')
    match error_number:
        case 0:
            print("File not found.")
        case _:
            print("Miscellaneous error! The worst kind!")

##################
# hmmm a warning #
##################

def throw_warning(warning_number,additional_info_1,additional_info_2):
    print(f"{bcolors.WARNING}WARNING: {bcolors.ENDC}", end = '')
    match warning_number:
        case 0:
            print(f"Found a non 128th note duration in channel: {additional_info_2}, at bar: {additional_info_1}. This may not be your mistake, but be aware of timing issues.")
        case 1:
            print(f"Found an overlapping note in channel: {additional_info_2}, at bar: {additional_info_1}.")
        case _:
            print("Miscellaneous warning! The worst kind!")

def midi_note_to_name(note_number):
    notes = ['c', 'c+', 'd', 'd+', 'e', 'f', 'f+', 'g', 'g+', 'a', 'a+', 'b']
    note_name = notes[note_number % 12]
    octave = note_number // 12 - 1
    return note_name, octave

def duration_to_128th_notes(ticks, ticks_per_beat):
    return (ticks * 32) / ticks_per_beat

def decompose_notes(total_128th_notes):
    note_values = {
        "1": 128,
        "2":  64,
        "4":  32,
        "8":  16,
        "16":  8,
        "32":  4,
        "64":  2,
        "128": 1
    }
    result = []
    for note, value in note_values.items():
        while total_128th_notes >= value:
            total_128th_notes -= value
            result.append(note)
    return result

def scale_volume(velocity):
    # Scale the volume from 0-127 to 1-16
    return int(velocity / 8) + 1

def process_midi_file(midi_file_path):
    global debug

    midi_file = mido.MidiFile(midi_file_path)

    # default tempo (120 BPM)
    tempo = 500000

    # track position
    total_128th_notes = 0
    total_ticks = 0
    current_bar = 0

    # array to store the MIDI data
    midi_data = []

    # dictionary to track the active note in each channel
    active_notes = {}
    last_note_off_time = {}

    for track in midi_file.tracks:
        current_time = 0

        for msg in track:
            if not msg.is_meta:
                current_time += msg.time
                total_ticks  += msg.time

            if msg.type == 'set_tempo':
                tempo = msg.tempo

            elif msg.type == 'note_on' and msg.velocity > 0:
                # check for overlapping notes
                if msg.channel in active_notes:

                    # overlapping note detected
                    throw_warning(1, round(total_128th_notes / time_signature, 2), track.name)

                    # process the previous note
                    prev_note, prev_start_time = active_notes[msg.channel]
                    note_duration = current_time - prev_start_time
                    note_128th_notes = duration_to_128th_notes(note_duration, midi_file.ticks_per_beat)
                    note_notes = decompose_notes(round(note_128th_notes))
                    note_name, octave = midi_note_to_name(prev_note)

                    midi_data.append([track.name, note_name, octave, note_notes, volume, f'{note_128th_notes:.2f}', f'{note_duration:.2f}', f'{current_bar:.2f}'])

                    total_128th_notes += note_128th_notes
                    del active_notes[msg.channel]

                # check for rest
                last_off = last_note_off_time.get(msg.channel, 0)
                if current_time > last_off:
                    rest_duration = current_time - last_off
                    rest_128th_notes = duration_to_128th_notes(rest_duration, midi_file.ticks_per_beat)
                    rest_notes = decompose_notes(round(rest_128th_notes))

                    midi_data.append([track.name, 'r', '-', rest_notes, '-', f'{rest_128th_notes:.2f}', f'{rest_duration:.2f}', f'{current_bar:.2f}'])

                    total_128th_notes += rest_128th_notes

                # start the new note
                note_start_time = current_time
                volume = scale_volume(msg.velocity)
                active_notes[msg.channel] = (msg.note, note_start_time)

            elif msg.type == 'note_off' or (msg.type == 'note_on' and msg.velocity == 0):
                if msg.channel in active_notes and active_notes[msg.channel][0] == msg.note:
                    note_duration = current_time - active_notes[msg.channel][1]
                    note_128th_notes = duration_to_128th_notes(note_duration, midi_file.ticks_per_beat)

                    # check for note smaller than a 128th note
                    if not note_128th_notes.is_integer():
                        throw_warning(0, round(total_128th_notes / time_signature, 2), track.name)

                    note_notes = decompose_notes(round(note_128th_notes))
                    note_name, octave = midi_note_to_name(msg.note)

                    midi_data.append([track.name, note_name, octave, note_notes, volume, f'{note_128th_notes:.2f}', f'{note_duration:.2f}', f'{current_bar:.2f}'])

                    total_128th_notes += note_128th_notes
                    del active_notes[msg.channel]

                    last_note_off_time[msg.channel] = current_time

    if debug:
        # determine column widths
        column_widths = [max(len(str(item[i])) for item in midi_data) for i in range(8)]

        # print the data with formatting
        for item in midi_data:
            formatted_line = ' '.join(str(field).ljust(column_widths[i]) for i, field in enumerate(item))
            print(formatted_line)

    # create the mml output
    else:
        previous_channel  = ''
        previous_note_name     = ''
        previous_duration = 0
        previous_octave   = 0
        previous_volume   = 0
        total_128th_notes = 0
        total_channels    = 1

        for i, item in enumerate(midi_data):
            mml_channel       = item[0]
            mml_note_name     = item[1]
            mml_octave        = item[2]
            mml_note_duration = f'&{mml_note_name}'.join(item[3])
            mml_volume        = item[4]
            mml_128ths        = float(item[5])

            if previous_channel != mml_channel:
                print(f'\n\n{bcolors.WARNING}C{total_channels} {bcolors.ENDC}', end="")

                # reset everything for new channel
                previous_duration = 0
                previous_octave   = 0
                previous_volume   = 0
                total_128th_notes = 0
                total_channels   += 1

            if previous_octave != mml_octave and mml_octave != '-':
                print(f'{bcolors.FAIL}o{mml_octave}{bcolors.ENDC}', end="")

            if velocity_print != 0:
                if previous_volume != mml_volume and mml_volume != '-':
                    print(f'{bcolors.OKBLUE}v{mml_volume}{bcolors.ENDC}', end="")
            
            print(f'{mml_note_name}', end="")

            if duration_repeat == 1:
                if previous_duration != mml_note_duration:
                    print(f'{mml_note_duration}', end="")
                else:
                    print('', end="")
            else:
                print(f'{mml_note_duration}', end="")

            # newline for every bar
            if (int(total_128th_notes) % time_signature) == 0:
                print('')

            # store values for recall next loop
            if mml_volume != '-':
                previous_volume = mml_volume

            if mml_octave != '-':
                previous_octave = mml_octave

            previous_duration  = mml_note_duration
            previous_channel   = mml_channel
            previous_note_name = mml_note_name
            total_128th_notes += mml_128ths

# main()
midi_file_path = 'input.mid'
process_midi_file(midi_file_path)
print("")