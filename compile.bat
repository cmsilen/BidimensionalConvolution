@echo off

REM Controllo se è stato passato un parametro
IF "%~1"=="" (
    echo Uso: compila.bat ^<numero^>
    goto fine
)

REM Prende il numero in input
set NUM=%1

REM Costruisce il nome del file
set FILE=mainMultiThreadV%NUM%.c

REM Compila con gcc

IF "%~1"=="4" (
    echo compilazione con priorità massima
    gcc -g -O3 -fprefetch-loop-arrays -flto -funroll-loops -fno-omit-frame-pointer -m64 -o main %FILE%
    goto fine
)

gcc -g -Og -fno-omit-frame-pointer -m64 -o main %FILE%

:fine
