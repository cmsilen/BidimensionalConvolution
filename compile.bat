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

IF "%~1"=="5" (
    echo compilazione con ottimizzazioni matematiche
    gcc -g3 -O3 -ffast-math -march=native -fno-omit-frame-pointer -m64 -o main %FILE%
    goto fine
)

IF "%~1"=="6" (
    echo compilazione con ottimizzazioni matematiche
    gcc -g3 -O3 -ffast-math -march=native -fno-omit-frame-pointer -m64 -o main %FILE%
    goto fine
)

IF "%~1"=="7" (
    echo compilazione con ottimizzazioni matematiche
    gcc -g3 -O3 -ffast-math -march=native -funroll-loops -fno-tree-pre -ftree-vectorize -fopt-info-vec -fno-omit-frame-pointer -m64 -o main %FILE%
    goto fine
)

gcc -g3 -Og -fno-omit-frame-pointer -m64 -o main %FILE%

:fine
