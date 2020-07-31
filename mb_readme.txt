To compile Marine Bot you'll need to have Marine Bot source code and the Half-Life SDK. The Half-Life SDK (HLSDK) is available
from multiple locations on the internet. There are three major versions, beside the original standard HLSDK v2.3, I have been able
to download, Metamod patched HLSDK 2.3-p4, then Valve Software Half-Life 1 SDK released in year 2013 and a version
available from the Allied Modders GitHub pages. The Allied Modders version is based on the HLSDK released in 2013 and
is the most recent version available. Marine Bot can be compiled successfully with all three of those versions.


Table of contents:
-----------------
1. Installing Marine Bot source code into Half-Life Standard Development Kit
2. Setting Marine Bot source code compatibility mode
3. Microsoft Visual Studio custom build
4. Optional step (to step no. 2) for Dedicated Server admins


----------------------------------------------------------------
1. INSTALLING MARINE BOT SOURCE CODE INTO HALF-LIFE SDK
----------------------------------------------------------------

You will first need to extract the HLSDK somewhere. Once you have done that you will need to extract
the Marine Bot source code package into a location inside that HLSDK folder. Where you extract it
will depend on the version of the HLSDK you are using.

For original standard HLSDK v2.3 you will need to extract the contents of the Marine Bot source code package into
the "Multiplayer Source" folder located in the root folder of the HLSDK source that you extracted.

For Metamod patched HLSDK 2.3-p4 you will need to extract the contents of the Marine Bot source code package into
the "multiplayer" folder located in the root folder of the HLSDK source that you extracted.

If you are using the newer 2013 version of the HLSDK from Valve GitHub you will need to extract the contents of your
Marine Bot source code package into the root "halflife-master" folder.

And for Allied Modders HLSDK you will need to extract the contents of your Marine Bot source code package
into the root "hlsdk-master" folder.

If you are using standard HLSDK 2.3 executable version for Windows then
the path to the Marine Bot "bot.cpp", for example, should look like this:

C:\HLSdk\Multiplayer Source\marine_bot\bot.cpp

If you are using the newer Valve Software HLSDK then it'll look like this:

C:\halflife-master\marine_bot\bot.cpp

You can of course rename the "marine_bot" folder to anything you like. It can be "mb_fa" for example. 
So the path to "bot.cpp" would then be:

C:\HlSdk\Multiplayer Source\mb_fa\bot.cpp


----------------------------------------------------------------
2. SETTING MARINE BOT SOURCE CODE COMPATIBILITY MODE
----------------------------------------------------------------

Once you have Marine Bot source code in place you will need to make it compatible with HLSDK version you are using. 

If you have the original standard HLSDK v2.3 then you can skip this step, because the source code is compatible by default.

But for the other three major versions of HLSDK you will need to open the "defines.h" file inside the "marine_bot" folder.
And find the line that matches the version of the HLSDK you are using.

For Metamod patched HLSDK v2.3-p4 find the line that reads as follows:

//#define NEWSDKMM		1

And uncomment that line by deleting the "//" at the start of the line.

For Valve Software Half-Life 1 SDK released in year 2013 find the line that reads as follows:

//#define NEWSDKVALVE		1

And uncomment that line by deleting the "//" at the start of the line.

And for Allied Modders HLSDK find the line that reads as follows:

//#define NEWSDKAM		1

And uncomment that line by deleting the "//" at the start of the line.


----------------------------------------------------------------
3. MICROSOFT VISUAL STUDIO CUSTOM BUILD
----------------------------------------------------------------

Finally if you're using MS Visual Studio then open the "!copy_releasedll.bat" and edit it based on what Half-Life version
you are using (Won or Steam) and where is it located. Visual Studio will return an error if the commands in this file fail. 
This script copies the compiled .dll to all the necessary locations. There are only two now, but in the past there were many locations.
Marine Bot supports mod directories with non-default names. If you don't want to use this script then edit the properties of Marine Bot
project in Visual Studio to remove the custom build link to this file.


----------------------------------------------------------------
4. (OPTIONAL) FOR DEDICATED SERVER ADMINS ON LINUX
----------------------------------------------------------------

If you are server administrator and you aren't sure about editing the source code you have the option to ignore the step 2 and
use the "makefile" to set the compatibility. It's basically the same thing. You will need to locate that file inside
your Marine Bot source code folder and open it for editing. Then find the compile option matching your version of HLSDK and
uncomment that line by removing the hash tag "#" at the start of the line. For the original standard HLSDK v2.3 there is no option,
because the source code is compatible with it by default.

Beside that you can also use the linker option to statically link the "libstdc++.so.6" into your "marine_bot.so". Doing so will kill
the ugly SteamCMD bug that results in "libstdc++.so.6: version CXXABI_1.3.9 not found" error. Your "marine_bot.so" will grow to
almost 3 megabytes in size though. Therefore this option is disabled by default.
