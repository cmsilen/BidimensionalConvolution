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
call compile %~1


for %%j in (3,5,11,19,29) do (
    echo Esecuzione: main %%j filter 30 imgs
    main 16 30 %%j 1 1
    main 16 30 %%j 1 1
    main 16 30 %%j 1 1
    main 16 30 %%j 1 1
    main 16 30 %%j 1 1
)

:fine
