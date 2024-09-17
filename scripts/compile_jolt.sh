#!/bin/bash -x

set -euo pipefail

mode=${1:-"Debug"}

CWD="$(pwd)"

mkdir -p lib
cd include/JoltPhysics/Build

rm -rf Linux_Debug
rm -rf Linux_Release
rm -rf Linux_Distribution

./cmake_linux_clang_gcc.sh "$mode" clang++ \
    -DBUILD_SHARED_LIBS=ON \
    -DCPP_RTTI_ENABLED=ON

cd "Linux_$mode"

make -j "$(nproc)"

./UnitTests

cp libJolt.so "$CWD/lib/libJolt.so"

