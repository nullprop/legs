#!/bin/bash

set -euo pipefail

# TODO: gcc busted
export CXX=clang++

# TODO: ubsan doesn't link with jolt
meson setup --reconfigure \
    -D examples=true \
    -D buildtype=debug \
    -D b_sanitize=address \
    -D b_lundef=false \
    build

