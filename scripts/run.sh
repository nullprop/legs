#!/bin/bash

set -euo pipefail

cd build
meson compile
LSAN_OPTIONS="suppressions=../suppr.txt" ./examples/02_systems/02_systems

