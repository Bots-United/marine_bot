echo off

REM Copy the .dll to all game directories
REM This is easier way than using the MSVC custom build option for
REM all commands

set CODEDIR="D:\Coding\HLSdk\Multiplayer Source\marine_bot"
set WONDIR=D:\WON2\Half-Life
set STEAMDIR="C:\Program Files (x86)\Steam\steamapps\mailtomapper@centrum.cz\half-life"

echo on

copy %CODEDIR%\Debug\marine_bot.dll     %WONDIR%\firearms\marine_bot
copy %CODEDIR%\Debug\marine_bot.dll     %STEAMDIR%\firearms\marine_bot
