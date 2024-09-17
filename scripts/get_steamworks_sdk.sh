#!/bin/bash

set -euo pipefail

SDK_VER=160

rm -rf include/steamworks
curl "https://partner.steamgames.com/downloads/steamworks_sdk_$SDK_VER.zip" -o /tmp/steamworks.zip
unzip /tmp/steamworks.zip -d include/steamworks
head -n 32 include/steamworks/sdk/Readme.txt

