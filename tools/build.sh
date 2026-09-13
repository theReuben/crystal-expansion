#!/bin/sh
# Crystal Expansion build wrapper.
# Requires GNU Make 4.0+ (see docs/port/decisions.md, D4): Apple's make 3.81
# hangs in implicit-rule search on this project's rule set.
set -e
export PATH="/opt/devkitpro/devkitARM/bin:$PATH"
export DEVKITARM=/opt/devkitpro/devkitARM DEVKITPRO=/opt/devkitpro
export CPATH=/opt/homebrew/include LIBRARY_PATH=/opt/homebrew/lib
exec gmake "$@"
