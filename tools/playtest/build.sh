#!/bin/sh
# Builds the headless play-test harness against the local libmgba 0.10.5.
set -e
root=$(cd "$(dirname "$0")/../.." && pwd)
src=$root/sources/mgba-src
build=$root/sources/mgba-build
cc -O2 -std=gnu11 -o "$root/tools/playtest/playtest" \
	"$root/tools/playtest/playtest.c" \
	-I"$src/include" -I"$src/src" -I"$build/include" \
	-L"$build" -lmgba -Wl,-rpath,"$build"
echo "built $root/tools/playtest/playtest"
