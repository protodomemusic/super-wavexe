SUPER WAVEXE!
=============

A little wavetable synthesizer optimized to make tiny executable music files. Basically a tiny DAW you sequence using MML, and then mix/master in a text file.

The program kind of works like this:

```
---------------------
| MML Sequence Data |
---------------------
         |
         V
 -------------------
 | Voice synthesis | <--
 -------------------    |
         |              | Repeat for
         V              | each voice
    ------------        |
    | Voice FX | -------
    ------------
         |
         | combine everything
         |
         V
   -------------
   | Master FX |
   -------------
         |
         V
   --------------
   | WAV output |
   --------------
```

The synthesizer is mono in the sense that it processes one 'voice' at a time. Each voice has its own, individual effects bus which is all combined into a single audio stream for processing and export. So, when you're composing for this, try to think in a 'per-voice', classic chiptune approach.

Update 25/05/24
---------------

Came back to this again because the engine still rules. Sounds like a 'analogue-y' FM hybrid - like if you trained a parrot to make FM sounds.

I've added a simple limiter which makes a HUGE difference to the output quality, plus there's numerous tweaks across the board. Making songs is now slightly easier and most of the key parameters you'd want to tweak per song are in the `instrument-config.h` file in the respective song's folder.

There's also now a documentation section (the `docs` directory) which is *very much* in progress. Every time I come back to this I forget how it works (it's a bit quirky at times) so the aim is to get it all in writing to make it easier for me (and maybe you?) to decipher.

Anyway I'm always learning - and I'm making up for years of neglected math - so if you've got any suggestions on how to improve stuff I'd LOVE to hear them.

(Oh and there's an AVR and GB 4ML player in the 'bonus' directory. It's messy as this should really be its own, standalone upgrade of mmml (https://github.com/protodomemusic/mmml), but I'm actively developing the compiler here so 🤷)

Update 01/08/23
---------------

Revisited this because I honestly like how cute and dumb this engine sounds. Added a Python compiler for an updated dialect of MMML called **MEGA MICRO MUSIC MACRO LANGUAGE**, or 4ML. Changed around some stuff to make it a little more flexible and user friendly. Still a little clumsy generally, but it's getting there.

If you want to write your own music (or, future me, when you're writing music) this is the current workflow. You'll need Python 3 and GCC for this.

- Duplicate the `songs/default` folder as you'll need a custom `instrument-config.h` file. For the `voice_boost` and `channel_fx` arrays you'll want as many entries as you're planning on channels in your 4ML file.
- You can add as many instruments as you like (well, 256). If you want inspiration/reference in building your own instruments have a look at `songs/sine-time/instrument-config.h`.
- Create your 4ML file in the new folder you made.
- Open the shell script for your platform and edit the following lines so that they point to your source files: `python3 compiler/mega-mmml.py songs/default/default.4ml` `mv 4ml-data.h songs/default/`
- In `src/super-wavexe.c`, edit these lines to point to your source files: `#include "../songs/sine-time/4ml-data.h"` and `#include "../songs/sine-time/instrument-config.h"`
- In `src/super-wavexe.c`, change the number of seconds in the `PLAY_TIME` definition to the song length of your choosing.
- Run the shell script for your platform and either: 1. enjoy the tunes or 2. deal with the inevitable warnings and errors from the 4ML compiler and GCC.

There are more in-depth instructions for the 4ML compiler in `compiler/mega-mmml.py` and `songs/default/default.4ml`, and the instrument/effect format in `songs/default/instrument-config.h`.

Update 21/08/22
---------------

An upgrade of wavexe (https://github.com/protodomemusic/wavexe). Oh you better get ready, because this time wavecycles are algorithmically generated (more memory, but smaller executable) and waveforms are calculated as floats (which conceptually simplifies things). Also, currently, tables are populated using FM synthesis (wow), and oscillators get resonant low-pass filters for additional squelch (fantastic).

No sequencing or anything yet, will add build instructions when there's a semblance of usefulness to the code.