#!/bin/bash

# Absolute path to this script, e.g. /home/user/bin/foo.sh
SCRIPT=$(readlink -f "$0")
# Absolute path this script is in, thus /home/user/bin
SCRIPTPATH=$(dirname "$SCRIPT")

# We are run from the corresponding directory in build/ by CMake
GENERATOR=`pwd`/generator/netgen
OUTPUT=$SCRIPTPATH/generated
source $SCRIPTPATH/../generate_inc.sh
