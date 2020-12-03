#!/bin/sh

BUILD_TYPE=${BUILD_TYPE:-debug}

premake5 gmake
make config=$BUILD_TYPE
