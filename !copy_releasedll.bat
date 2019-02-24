echo off

REM Copy the .dll to all game directories
REM This is easier way than using the MSVC custom build option for
REM all commands

set CODEDIR="C:\HLSdk\Multiplayer Source\marine_bot"
set WONDIR=C:\Sierra\Half-Life
set STEAMDIR="C:\Program Files (x86)\Steam\steamapps\mailtomapper@centrum.cz\half-life"


echo on

copy %CODEDIR%\Release\marine_bot.dll     %WONDIR%\firearms\marine_bot
REM copy %CODEDIR%\Release\marine_bot.dll     %STEAMDIR%\firearms\marine_bot