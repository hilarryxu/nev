#!/bin/sh

BUILD_TYPE=${BUILD_TYPE:-release}

premake5 gmake
make CC=gcc config=$BUILD_TYPE
