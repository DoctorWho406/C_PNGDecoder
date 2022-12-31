@echo off

set BAT_BASE_PATH=%~p0

if exist "%BAT_BASE_PATH%bin" (
    RD /S /Q "%BAT_BASE_PATH%bin"
)

MD "%BAT_BASE_PATH%bin"

clang -o "%BAT_BASE_PATH%bin\main.exe" "%BAT_BASE_PATH%src\main.c" -I "%BAT_BASE_PATH%include" -I "%BAT_BASE_PATH%include\SDL" -l zlibwapi -l linkedlist -l SDL2 -L "%BAT_BASE_PATH%library"

COPY "%BAT_BASE_PATH%library\zlibwapi.dll" "%BAT_BASE_PATH%bin\zlibwapi.dll"
COPY "%BAT_BASE_PATH%library\linkedlist.dll" "%BAT_BASE_PATH%bin\linkedlist.dll"
COPY "%BAT_BASE_PATH%library\SDL2.dll" "%BAT_BASE_PATH%bin\SDL2.dll"