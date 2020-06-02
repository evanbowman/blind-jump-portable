#!/bin/bash

rm CMakeCache.txt
rm -r CMakeFiles/

cmake -DGBA_AUTOBUILD_IMG=ON -DCMAKE_TOOLCHAIN_FILE=$(pwd)/devkitarm.cmake .
