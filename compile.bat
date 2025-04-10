@echo off
IF "%1" == "single" (
	gcc -g -O0 -fno-omit-frame-pointer -o main mainSingleThread.c
	GOTO End
)

IF "%1" == "multi" (
	gcc -g -O0 -fno-omit-frame-pointer -o main mainMultiThread.c
	GOTO End
)

echo "usage: compile [single | multi]"

:End