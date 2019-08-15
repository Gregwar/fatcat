#!/bin/sh
if [ ! -d build ]; then
	mkdir build
fi
cd build
git clean -fdx && cmake .. && make
cd ..