#!/bin/bash

set -e

./make.sh "i686"
./make.sh "sh4"
./make.sh "mipsel"
./make.sh "mipsel_softfpu"
./make.sh "armv7"
./make.sh "armv5t"

