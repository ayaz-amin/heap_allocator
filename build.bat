@echo off

mkdir debug
pushd debug

set SRC=../src/*.c
set SDLINC=../inc/
set SDLLIB=../lib/x64/SDL3.lib
set CLLIBS=kernel32.lib user32.lib

cl /I%SDLINC% %SRC% %CLLIBS% %SDLLIB% /link /SUBSYSTEM:CONSOLE /OUT:main.exe

popd
