#!/bin/sh
# Native fixture checks; requires the desktop SDL2 development dependencies.
set -eu
cd "$(dirname "$0")/.."
fixture_dir=$(mktemp -d /tmp/nextui-library-tests.XXXXXX)
trap 'rm -rf "$fixture_dir"' EXIT HUP INT TERM
${CC:-cc} -std=gnu99 -g -O1 -ffunction-sections -fdata-sections \
    ${TEST_CFLAGS:-} -DPLATFORM='"desktop"' -DUSE_SDL2 \
    -DTEST_LIBRARY_ROOT="\"$fixture_dir\"" \
    -Iworkspace/all/common -Iworkspace/desktop/platform -Iworkspace/desktop/libmsettings \
    -include tests/library_paths.h $(pkg-config --cflags sdl2 SDL2_image SDL2_ttf) \
    tests/library_test.c workspace/all/common/utils.c workspace/all/common/config.c \
    -Wl,--gc-sections ${TEST_LDFLAGS:-} -lm \
    $(pkg-config --libs sdl2 SDL2_image SDL2_ttf) -o "$fixture_dir/library-test"
"$fixture_dir/library-test"
