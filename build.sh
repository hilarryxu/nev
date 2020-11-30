#!/bin/sh

BUILD_TYPE=${BUILD_TYPE:-release}

premake5 gmake
make config=$BUILD_TYPE
