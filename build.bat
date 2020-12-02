cd /d %~dp0
@echo off
set path=D:\MinGW\bin;%path%
premake5 gmake
mingw32-make config=release
PAUSE
