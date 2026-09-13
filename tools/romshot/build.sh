#!/bin/sh
# Build the headless screenshotter. Expects sources/mgba-src + sources/mgba-build
# (see tools/romshot/README.md for how to produce them).
set -e
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)
cc -O2 -std=gnu11 -DUSE_PNG -o "$here/romshot" "$here/romshot.c" \
	-I"$root/sources/mgba-src/include" -I"$root/sources/mgba-build/include" \
	-L"$root/sources/mgba-build" -lmgba.0.10.5 \
	$(libpng-config --cflags --ldflags) -lz \
	-Wl,-rpath,"$root/sources/mgba-build"
echo "built $here/romshot"
