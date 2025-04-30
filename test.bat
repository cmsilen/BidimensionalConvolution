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
gcc -g -O0 -fno-omit-frame-pointer -m64 -o main %FILE%

for /L %%i in (1,1,40) do (
    for %%j in (10,20,30) do (
        echo Esecuzione: main %%i threads %%j imgs
        main %%i %%j
        main %%i %%j
        main %%i %%j
        main %%i %%j
        main %%i %%j
    )
)

:fine
