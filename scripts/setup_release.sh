#!/bin/bash

set -euo pipefail

meson setup --reconfigure \
    -D examples=true \
    -D buildtype=release \
    build

