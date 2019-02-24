# marine_bot
Devised by Frank McNeil - Bots for HL: FireArms

Genaral info
Marine Bot is a bot designed for Firearms a Half-life 1 multiplayer mod. Actually for the latest Firearms versions 2.9 and 3.0, but also works with versions 2.4-2.8. Marine Bot works with both Steam and WON (also WON 2) systems. There are no special system requirements. If your PC is capable of running Half-Life then you will be able to run Marine Bot. However the better PC you have the smoother the game will be. You know the bot needs some CPU time in order to work the way we want. So expect a slight FPS drop with every bot you add into the game.

The Marine Bot team
Founder, Lead Programmer, Waypointing and Testing: Frank McNeil (contact)
Team Coordinator, Emailing, Dedicated server testing, Waypointing, Throwing Rocks From Afar: Drek
Linux conversion, Dedicated server testing and overall help and support: Sargeant.Rock
Documentation, Waypointing, Installation package and stuff: Modest Genius
Artwork: ThChosenOne

Ex Team Members
_shadow_fa: Programming, Testing, Linux
Buv: Linux Testing
Cervezas: Waypointing
Cpl. Shrike: Programming
Creslin: Testing, Linux
Elektra: Forums Administration
glendale2x: Linux
gregor: Waypointing, Testing
Loule: Waypointing
Mav: Programming
oldmanbob: Lead Waypointer, Testing
Pastorn: Waypointing, Testing
Recce: Waypointing
Rogacz: Testing
Seeker: Waypointing, Testing
Spicoli: Testing
Wyx: Waypointing

External contributors and other credits
Some parts of this program are based on Botman's HPB bot template. Thank you Botman! You can download Botman's templates and get help on using them or any aspect of programming your own bot at http://hpb-bot.bots-united.com/.

Distribution permissions and disclaimer
This program may be distributed in any way provided that you keep the entire package complete and credit the authors. You may NOT charge in any way for ownership of, or access to, this program without the prior consent of the authors, either in writing or via email. We will almost certainly agree if it is to be included on a magazine CD or other similar medium, but Games Fusion and the like will never get our consent.

This software is provided 'as is'. There is no warranty or technical support, although we will endeavour to answer all queries. The authors are not responsible for any loss of data or any other damage resulting from the use or misuse of this program. Likewise the authors cannot be held responsible for the use or misuse of this software by third parties. Use of this software is AT YOUR OWN RISK!

This software is BETA. This means it has not undergone full and rigorous testing on mulitple system setups, and may cause your computer to crash or become unstable. If you experience any problems please let us know.

The source code for this software is released under the terms of the GNU Public License and is available as a standalone .zip package along with full details of the License.

Install instructions
Follow these steps exactly the way they are written here in order to install the bot correctly.

Using the .exe installer program
run the installation utility and follow the instructions (be sure to read the information on the second page)
that's all, run the game and a few bots will join you
Using the .zip or .rar archive
unpack the archive into your ...\half-life\firearms folder
run the ...\half-life\firearms\liblist_install_mb.exe
that's all, run the game and a few bots will join you
Additional steps if you are running Adminmod/Metamod
open the file 'liblist.gam' in your ...\half-life\firearms folder using notepad
comment out the line under 'MarineBot LAN' using '//' at the start of the line
uncomment the line under 'MarineBot AdminMod' by deleting the '//'
add '+localinfo mm_gamedll marine_bot/marine_bot.dll' in the properties of the shortcut you use to start your game or server
Uninstall instructions
Follow these steps exactly the way they are written here in order to uninstall the bot correctly.

If you used the .exe installer program to install Marine Bot
run the ...\half-life\Uninstall_MB091.exe (or use the Add/Remove Programs from your Control Panel in Windows OS)
erase the whole marine_bot folder inside your ...\half-life\firearms folder
run the ...\half-life\firearms\liblist_uninstall_mb.exe
erase both ...\half-life\firearms\liblist_install_mb.exe and ...\half-life\firearms\liblist_uninstall_mb.exe files
that's all
If you used the .zip or .rar archive to install Marine Bot
erase the whole marine_bot folder inside your ...\half-life\firearms folder
run the ...\half-life\firearms\liblist_uninstall_mb.exe
erase both ...\half-life\firearms\liblist_install_mb.exe and ...\half-life\firearms\liblist_uninstall_mb.exe files
that's all
Supported maps
Well the bot can be used on any map, but with supported I mean maps that have waypoints ie. maps the bot can play with some sort of wisdom. You know where the bot won't just blindly roam, but where he will try to visit important locations like the map pushpoints, objective item locations, places where you usually meet others and so on. Anyways here's the default waypoint list.

obj_armory
obj_bocage
obj_paradise
obj_sweden
obj_thanatos
obj_vengeance
obj_willow
ps_bridge
ps_coldwar
ps_crash
ps_inlands
ps_island
ps_marie
ps_outlands
ps_river
ps_splinter
ps_storm_r1
ps_upham
psobj_thanatos
sd_durandal
sd_force
sd_oberland
sdtc_balcome
tc_basrah
tc_battlefield_r2
tc_caen
tc_golan
tc_iwojima
tc_okinawa_b3
tc_okinawa_b4
tc_rubble
tc_thebes
Waypointing instructions (ie. how you can make a unsupported map a supported one)
What are the waypoints for?
The bot is using waypoints to navigate through the map. Waypoint is a location in the map (in the 3d space), which the bot will use when navigating. The bot has no eyes so the waypoint will tell him that this location is a walkable space and not a solid wall. The most simple description of a waypoint is that it is a 'hint', which guides the bot.
When editing waypoints, you can see the waypoints drawn with an orange beam. Some special waypoints are displayed in other colors. Waypoints are being stored as a 'mapname.wpt' file in your Marine Bot waypoints folder (ie. marine_bot\defaultwpts). As a addition to the waypoints there are waypoint paths. The path is a connection between two or more waypoints and it tells the bot that there's a clear way between these waypoints. Waypoint paths are being stored in 'mapname.pth'. Unlike the waypoints the paths aren't necessary. However paths improve the navigation and open a lot of possibilities. Therefore we recommend using them.
A few basic waypointing console commands:
wpt showshows the waypoints (displays them using a colored beams). It toggles between show and hide waypoint.
wpt addadds a waypoint to your location
wpt deletedeletes a waypoint you're standing by
wpt movemoves/repositions close waypoint to your location (you have to be quite close to a waypoint)
wpt infodisplays number and properties (if available) of the waypoint you're standing by

pathwpt showshows/hides the waypoint paths (displays vertical lines between waypoints)
pathwpt startstarts a new path on the waypoint you're standing by
pathwpt stopends the path (you can use it anywhere, because it only stops adding any new waypoint in the path)
pathwpt addadds a waypoint you are standing by to the path
pathwpt autoaddtoggles between automatic pathwaypointing (automatically adds any waypoint you touch to the path) and manual path building by using the command from above
pathwpt deletedeletes the path on the waypoint you're standing by

wpt savewrites the waypoints and paths into mapname.wpt and mapname.pth files
wpt loadreads the waypoints and paths from the files (it can also be used as one step undo feature when you're working on the waypoints)
Special waypoints:
You can assign a special tag/flag/type to any waypoint. All you need to do is get close to the waypoint you want to make special and use 'wpt change argument' command. Where the argument is the new type of the waypoint. You can use one of the following options:
aim
ammobox
claymore
cross
crouch
door
duckjump
goback
jump
ladder
normal
parachute
prone
pushpoint
shoot
sniper
sprint
trigger
use
usedoor
Aimthe bot will use this waypoint as a target to aim his weapon at, useful when you set up a camping spot
Ammoboxtells the bot there's an ammobox close
Claymorethe bot may place a claymore mine at this location (although he can also save the claymore for later use)
Crossthis waypoint serves as a crossroad marker, allows you to give the bot more than one route
Crouchthe bot will crouch/duck
Doorthe bot will slow down to pass the doors that open automatically
Duckjumpthe bot will duck-jump forward here
Gobacktells the bot to return back at the beginning of this path, useful on paths leading to camping spots
Jumpthe bot will jump forward here
Laddertells the bot there's a ladder, there should be one ladder waypoint at the bottom and one at the top of the ladder
Normalthe bot will just pass through
Parachutethe bot will check if he has a parachute before moving forward
Pronethe bot will go prone here
Pushpointallows the bot to see which flag needs to be captured on push maps
Shootthe bot will fire his weapon to break a breakable obstacle (e.g. window) or sd object around this waypoint
Sniperthe bot won't move towards the enemy during combat, useful to create a camper spot behind a sandbag or in window and like
Sprintthe bot will start or stop sprinting at this waypoint, useful to make the bot pass through open area quickly
Triggerallows the bot to follow the game situation based on linked messages, useful on obj maps to open or close certain routes
Usethe bot will activate/use a close object (e.g. button or turret)
Usedoorthe bot will slow down to pass the doors that needs to be open manually
For more info check the waypointing documentation where you can get detailed explanation.
Okay I know the commands now. What do I have to do to start the waypointing?
The only thing you should do is kick all the bots off the game. The bots may behave strange and/or even crash the game. So type this command 'kickbot all' into the console. Then you can use the 'pathwpt show' command to display waypoints and paths (if there are any).
So how do I add a new waypoint?
Just type the 'wpt add' command and a new waypoint will appear at your position. You can use argument to specify the waypoint type if you wish. I mean you can add a cross waypoint by typing 'wpt add cross' into the console. This is a faster way than adding standard waypoints and then changing them to special types using the 'wpt change argument' command. You do it all using just one command.
I see blue, yellow, green and other color beams instead of the orange beam waypoints. What they are for?
These are the special waypoint types mentioned above. They tell the bot to do some kind of a action there. Or they inform the bot that there's something he would like to have/do there. See the 'special waypoints' paragraph for more info.
I used the 'pathwpt show' command, but I don't see any paths. What's wrong?
Sometimes you have to be close to a waypoint in order to see the path. Once you're standing close enough the paths on the waypoint will appear. There can also be a problem with the other waypoint in the path. I mean the other waypoint is simply too far to draw it and draw the path beam as well.
I see some of the paths are different in color or appearance. What does this mean?
There are more types of paths. The fact that some of the paths are red for example means that they are assigned for the red team only. On the other hand green path beam tells you the path is assigned for a sniper (light green) or machine gunner (dark green).
I don't like certain path. Can I get rid of it?
Yes of course. You can use the 'pathwpt delete' command to erase any path you don't like. Get close to one of its waypoints and type the command. Or if there are more paths on the waypoint you can use the argument ('pathwpt delete argument') to specify the exact path number (e.g. pathwpt delete 12). The path matching this number will be erased.
Okay so how do I find out the path number then?
Pretty simple. You can use the 'pathwpt info' command which will tell you all about the path including its unique number. Get close to one of its waypoints and type the command.
Tip: If you want to see all the paths on the waypoint you can use 'pathwpt printall argument' command where the argument is the unique number of your waypoint.
And what have to I do to add a new path?
Use the 'pathwpt start' command while standing by the 1st waypoint. Then get to the 2nd waypoint and use the 'pathwpt add' command there. You've just connected the two waypoints with the shortest possible path. If you want to continue adding more waypoints you'll have to get close to each of them and use the 'pathwpt add' command there. Whenever you wish to end the path you can just type the 'pathwpt stop' command into the console.
Tip: You can also make your life a bit easier by using 'pathwpt autoadd' command. Then all you need to do to add another waypoint in a path is getting close enough to it to literally touch it. It will be added automatically. Just be careful when there are many waypoints close, because you may then add even those waypoints you didn't want in this path.
Is there any waypointing tutorial or any guidelines I should follow?
Of course. Reading this step-by-step manual should help you undestand what lies behind the waypoint navigation.
FAQ (Frequently Asked Questions)
What does FAQ mean?
It's an acronym for Frequently Asked Questions.
Is there any in-game help?
Yes, access it by opening the console and entering 'help' or '?' (no quotation marks).
So I will have to use the console commands to control Marine Bot?
Yes full control can only be done using the console commands. However there is a version of the Firearms command menu included with Marine Bot that gives you control over your bots in Firearms 2.8 and above. You can access this menu by pressing the command menu key (by default 'p') ingame. You can change this key in the Options screen for Firearms. Note that this is for a listenserver only, see the 'hlds_cmds.txt' file for a full listing of all commands available on a dedicated server. If you are running Firearms 2.7 or below, you can control Marine Bot using the menu bound to your END key.
When I start a game with Marine Bot I get the message 'Host_Error: Couldn't get DLL API from...'. What’s wrong?
This error occurs when Half-Life can’t find the correct Marine Bot files, usually due to problems with your 'liblist.gam' file. Make sure you have Marine Bot properly installed. Scroll at the top of this page or see the ‘readme.txt’ file for details on how to do this.
Why does Half-Life crash when I use the 'wpt save' command to save my waypoints?
Sorry that you lost your work, but you don't have Marine Bot properly installed. This error will result if there is no marine_bot/defaultwpts folder, which should be created during a correct installation.
While the map is loading, I get an error along the lines of 'NULL entity ...', but the game still loads, and certain gameplay features don’t work. How can I fix this?
If you are using Firearms in version 2.4 up to latest 3.0 this shouldn't happen. If this does happen please let us know using the contact information below. If you are using any other version then you can do the same, but we can't promise that we can do anything about it, because our programmers tend to use the latest full version of Firearms (currently 3.0).
How do I add bots into my game?
If you are starting with a new installation, bots should join automatically.
Open the console and type 'recruit' (without quotes) to add randomly generated bot to the game.
If the 'recruit' command for adding bots doesn't work, then see 3rd question. You probably don't have a modified 'liblist.gam' file in your ...\half-life\firearms folder. See the top of this page or 'readme.txt' file for details.
Can I arrange it so that bots automatically join the game instead of having to add them one by one?
If you are starting from a fresh installation, 15 bots should join automatically.
If you only want a certain number of bots to join the game, you can use the console command 'fillserver #' (doesn't work on DS) where # is the number of bots you wish to add (e.g. fillserver 15).
If you don’t want to fill the server, or want more control, you can add bots using your 'marine.cfg' file. See that file for details.
If you want different numbers or configurations of bots for each map, you can create a separate file for this. Simply copy the 'marine.cfg' file into your marine_bot\mapcfgs folder, and rename it to the same name as the map plus '_marine.cfg' e.g. for ps_marie map you would rename it to 'ps_marie_marine.cfg' or for obj_willow map it would be 'obj_willow_marine.cfg' and so on. Then edit it in the same way.
Bots appear to be stuck, moving blindly between corners. They don’t seem to be able to find their way around the map at all, why is this?
For Marine Bot to work correctly, you need to have specially created waypoints for each map you want to use bots on. Marine Bot should tell you with a message on the screen if it is missing waypoints. Look in your marine_bot/defaultwpts folder to see if there is a .wpt file with the same name as the map, such as 'ps_marie.wpt'. Current Marine Bot does come with many waypoints. You can scroll up on this page to see the default waypoint list.
If you can’t find waypoints for a specific map, you could always make them yourself and email them to us. If they are sufficiently good we may even include them in the next version of Marine Bot and/or put them in the 'files' section for others to download.
Can I change the names the bots use ingame?
Yes, in the 'marine_dog-tags.txt' file. Be sure to read and follow the instuctions there otherwise things may not work.
Can I change the gear the bots use ingame?
Yes, in the ‘marine.cfg’ file. See that file for details.
If you want bots to have different setups for certain maps, then create a mapname_marine.cfg file for that map in the same way as described above and then edit the configs there. See that file for details.
I have waypoints for a map, but they don't work. Why?
Marine Bot automatically converts waypoints from the previous version into its current version. Try typing ‘wpt load’ in the console (no quotation marks), or if that doesn’t work try ‘wpt loadunsupported’, or if that doesn’t work try ‘wpt loadunsupportedversion5’. If either of these works, use ‘wpt save’ to save them. If all return an error, then your waypoint file is probably corrupt, try downloading it again. Alternatively, make sure your waypoint file is of the same name as the map, note that some official maps change names between versions. Finally, MB can only convert from last two most recent waypoint formats, anything older than MB in version 0.8b will not be supported.
Tip: You can type 'get_wpt_system' command to see which waypoint system are the waypoints created in. Marine Bot 0.8b used waypoint system version 5. Marine Bot 0.9b used system version 6. And Marine Bot 0.91b is using waypoint system version 7.
Why do I get the message 'waypoints subscription FAILED ...' when I try to change the authors name for a set of waypoints?
The author's signature can only be set once by the author. Once the author's signature is set you can only set/change the 'modified by' signature.
If you are creating new waypoints from scratch just save them first and then you will be able to set your signature.
Who made Marine Bot?
See the 'info' section or the ‘readme.txt’ file for a full list of contributors and credits.
How can I help develop Marine Bot?
Play it and test it. We are always happy to receive any comments, criticism or suggestions you may have, to contact us see the question below.
Create waypoints for maps (see the documentation for details), especially those which have not been waypointed before. Then email them to us. If the waypoints are good enough we will include them in the next release.
If you are running a dedicated server, please let us know. We are especially interested in any logs of server crashes, as these enable us to fix these problems quickly and easily for the next version.
