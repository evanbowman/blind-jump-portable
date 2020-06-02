#!/bin/bash

rm CMakeCache.txt
rm -r CMakeFiles/

cmake . -DGAMEBOY_ADVANCE=OFF
