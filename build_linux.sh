#!/bin/bash
# This should really be ignored on commit but,
# as there's no documentation for wavexe just
# yet, it's helpful to see how this should be
# used in conjunction with the mmml compiler.

# get rid of old stuff

rm super-wavexe
rm output.wav

# note this will fail if you've just
# downloaded from the repository you can
# ignore it though (unless you want to write
# your own music)

./mmml-compiler -f input.mmml -t wavexe -c 8

gcc super-wavexe.c -O3 -o super-wavexe -lm -lasound

# if you want tiny executables, uncomment
# below, comment out the above (and install
# upx)

#gcc super-wavexe.c -O3 -o super-wavexe-big -lm -lasound
#upx -9 -o super-wavexe super-wavexe-big
#rm super-wavexe-big

./super-wavexe