#!/bin/bash
# This should really be ignored on commit but,
# as there's no documentation for wavexe just
# yet, it's helpful to see how this should be
# used in conjunction with the 4ml compiler.

# get rid of old stuff

rm src/super-wavexe
rm output.wav

python3 compiler/4ml-compiler.py songs/garlic-bread/input.4ml
mv 4ml-data.h songs/garlic-bread/

gcc src/super-wavexe.c -O3 -o src/super-wavexe -lm

# if you want tiny executables, uncomment
# below, comment out the above (and install upx)

# gcc src/super-wavexe.c -O3 -o src/super-wavexe-big -lm
# upx -9 -o src/super-wavexe src/super-wavexe-big
# rm src/super-wavexe-big

./src/super-wavexe
play output.wav