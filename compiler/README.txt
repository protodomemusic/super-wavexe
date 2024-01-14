#=================================#
# Mega Micro Music Macro Language #
#=================================#

#================================================#
# What is Mega Micro Music Macro Language (4ML)? #
#================================================#

4ML is a lightweight musical sequencing language for use in music software projects where you may want a minimal musical payload. It's an expansion of a simple language (Micro Music Macro Language) I defined for use in microcontroller programs and other small executable music projects. The language ended up being a quick and efficient way of storing music data in a format that suited the kind of music I like to write: melodically driven, reasonably complex, with a high note event density. Based on Music Macro Language (MML), it is both relatively easy to read in a text editor and close enough to the compiled bytecode to make writing and programming efficient.

With the 'mega' variation of the language I wanted to: 1. make it even easier to create MML documents 2. syntactically bring the language closer in line with the (various) pre-existing MML implementations, and 3. move the compiler to a language that's easier to work with (Python).

#========================#
# 4ML Command Dictionary #
#========================#

In defining this language, I have looked at many of the most popular existing MML syntaxes and attempted to use the most popular symbols for each command. MML engines can generally be quite extensive and packed full of platform specific commands. With 4ML I want to be more flexible, and keep things much simpler (it does come with the trade-off of removing features though).

Note, where other MML engines have been listed in the 'Also defined in' column, this does indicate compatibility, just that the commands share the same (or very similar) behavior between implementations. Where commands differ, I have put the respective symbol in brackets (for fun and reference I guess).

;
Single Line Comment
Applies from the semicolon to the end of the line.
--
Also in: PPMCK, MGSC, XPMCK

/* */
Multi-Line Comment
Applies from the first instance of /* through to the corresponding */.
--
Also in: PPMCK, XPMCK, MML@

C<0-255>
Channel Definitions
This is a core change to the MML paradigm that is not reflected in most MML implementations. Typically, channels are defined by the letters A-Z and, although it is very unlikely that you will be writing for more than 26 voices, it feels arbitrary to limit the language in that way. Additionally, where a language like PPMCK may have distinct sonic identities attached to each channel letter, 4ML is to be applied generally.
--
Also in: PPMCK (A-Z), MGSC (A-Z), XPMCK (A-Z), MML@ (,)

P<0-255>
Pattern Definitions
Patterns are essentially subroutines.
--
Also in: XPMCK (\)

p<0-255>
Pattern Invocations
--
Also in: XPMCK

+ and #
Sharpen Note
At the moment there's no way to flatten a note. Also worth noting that, whilst you'd expect b+ to be higher than b, as it's equivalent to c and there's no octave change, it's the c an octave below.
--
Also in: PPMCK, MGSC, XPMCK, MML@

o<0-16>
Octave
--
Also in: PPMCK, MGSC, XPMCK, MML@

< and >
Octave Lower and Octave Raise
--
Also in: PPMCK, MGSC, XPMCK, MML@

v<0-16>
Volume
--
Also in: PPMCK, MGSC, XPMCK, MML@

t<0-255>
Tempo
--
Also in: PPMCK, MGSC, XPMCK, MML@

& or ^
Tie / Slur
--
Also in: PPMCK, MGSC, XPMCK, MML@

@<0-255>
Instrument
--
Also in: PPMCK, MGSC, XPMCK

K<0-255>
Transpose
--
Also in: PPMCK, MGSC, XPMCK

F<0-255>
Miscellaneous Expanded Functions
For some platforms there are musical or system requirements that make more sense than others. This kind of stuff I don't really want to permanently embed into core 4ML syntax, so there's the generic 'F' command. At this moment in time (2024/01/10), it only reads the next byte, but it could theoretically read another byte beyond that for further adaptability. If you (likely me a few years from now) are thinking about supporting these functions for some new future player, it's kinda not required, so don't sweat it.

#===================#
# command reference #
#===================#

--------------------------------------  -------------------------------------------
| 4ML  | BYTECODE | HEX | COMMAND    |  | 4ML       | BYTECODE | HEX | COMMAND    |
--------------------------------------  -------------------------------------------
| r    | 0000     | 0   | rest       |  | g         | 1000     | 8   | note - g   |
| c    | 0001     | 1   | note - c   |  | g+        | 1001     | 9   | note - g#  |
| c+   | 0010     | 2   | note - c#  |  | a         | 1010     | A   | note - a   |
| d    | 0011     | 3   | note - d   |  | a+        | 1011     | B   | note - a#  |
| d+   | 0100     | 4   | note - d#  |  | b         | 1100     | C   | note - b   |
| e    | 0101     | 5   | note - e   |  | o,<,>     | 1101     | D   | octave     |
| f    | 0110     | 6   | note - f   |  | v         | 1110     | E   | volume     |
| f+   | 0111     | 7   | note - f#  |  | [,],m,t.. | 1111     | F   | function   | ----
--------------------------------------  -------------------------------------------     |
----------------------------------------------------------                              |
|      |        |     | READS NEXT |                     |                              |
| 4ML  | BINARY | HEX | BYTE?      | COMMAND             | <----------------------------
----------------------------------------------------------
| [    | 0000   | 0   | yes        | loop start          |
| ]    | 0001   | 1   | no         | loop end            |
| p    | 0010   | 2   | yes        | macro               |
| t    | 0011   | 3   | yes        | tempo               |
| K    | 0100   | 4   | yes        | transpose           |
| @    | 0101   | 5   | yes        | instrument          |
| &    | 0110   | 6   | no         | tie                 |
| s    | 0111   | 7   | yes        | stereo (panning)    |
|      | 1000   | 8   |            |                     |
|      | 1001   | 9   |            |                     |
|      | 1010   | A   |            |                     |
|      | 1011   | B   |            |                     |
|      | 1100   | C   |            |                     |
|      | 1101   | D   |            |                     |
| F    | 1110   | E   | yes        | misc functions      | ---
| N/A  | 1111   | F   | no         | channel/macro end   |    |
----------------------------------------------------------    |
--------------------------------------------                  |
| 4ML  | BYTECODE | HEX | COMMAND          | <----------------
--------------------------------------------
| 1    | 0000     | 0   | volume increment |
| 2    | 0001     | 1   | volume decrement |
--------------------------------------------

#=========================#
# note duration reference #
#=========================#

-------------------------  ------------------------------
| 4ML  | BYTECODE | HEX |  | 4ML       | BYTECODE | HEX |
-------------------------  ------------------------------
| 1    | 0000     | 0   |  | 2.        | 1000     | 8   |
| 2    | 0001     | 1   |  | 4.        | 1001     | 9   |
| 4    | 0010     | 2   |  | 8.        | 1010     | A   |
| 8    | 0011     | 3   |  | 16.       | 1011     | B   |
| 16   | 0100     | 4   |  | 32.       | 1100     | C   |
| 32   | 0101     | 5   |  | 64.       | 1101     | D   |
| 64   | 0110     | 6   |  |           | 1110     | E   |
| 128  | 0111     | 7   |  |           | 1111     | F   |
-------------------------  ------------------------------