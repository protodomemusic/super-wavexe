#!/bin/bash
# This should really be ignored on commit but,
# as there's no documentation for wavexe just
# yet, it's helpful to see how this should be
# used in conjunction with the mmml compiler.

# get rid of old stuff

rm src/super-wavexe
rm output.wav

# note this will fail if you've just downloaded from the repository
# you can ignore it though (unless you want to write your own music)
python3 compiler/mega-mmml.py songs/default/default.4ml
mv 4ml-data.h songs/default/

gcc src/super-wavexe.c -O3 -o src/super-wavexe -lm -lasound

# if you want tiny executables, uncomment
# below, comment out the above (and install upx)

# gcc src/super-wavexe.c -O3 -o src/super-wavexe-big -lm -lasound
# upx -9 -o src/super-wavexe src/super-wavexe-big
# rm src/super-wavexe-big

./src/super-wavexe