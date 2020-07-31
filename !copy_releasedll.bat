echo off

REM Copy the .dll to all game directories
REM This is easier way than using the MSVC custom build option for
REM all commands

set CODEDIR="."
set WONDIR=C:\Games\Half-Life
set STEAMDIR="C:\Program Files (x86)\Steam\steamapps\common\Half-Life"


echo on

copy %CODEDIR%\Release\marine_bot.dll     %WONDIR%\firearms\marine_bot
copy %CODEDIR%\Release\marine_bot.dll     %STEAMDIR%\firearms\marine_bot