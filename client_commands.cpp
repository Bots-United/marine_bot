///////////////////////////////////////////////////////////////////////////////////////////////
//
//	-- GNU -- open source 
// Please read and agree to the mb_gnu_license.txt file
// (the file is located in the marine_bot source folder)
// before editing or distributing this source code.
// This source code is free for use under the rules of the GNU General Public License.
// For more information goto:: http://www.gnu.org/licenses/
//
// credits to - valve, botman.
//
// Marine Bot - code by Frank McNeil, Kota@, Mav, Shrike.
//
// (http://marinebot.xf.cz)
//
//
// client_commands.cpp
// 
////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(WIN32)
#pragma warning(disable: 4005 91)
#endif

#include "defines.h"

#include "extdll.h"
#include "util.h"
#include "cbase.h"

#include "bot.h"
#include "bot_func.h"
#include "bot_manager.h"
#include "waypoint.h"
#include "client_commands.h"

#ifdef DEBUG
#include "bot_weapons.h"
#endif // DEBUG


#define MENU_NONE		0
#define MENU_MAIN		1
// bot menu & submenus
#define MENU_1			2
#define MENU_1_9		3
#define MENU_1_9_1		4
#define MENU_1_9_6		5
// waypoint menu & submenus
#define MENU_2			6
#define MENU_2_1AND2_P1	7
#define MENU_2_1AND2_P2	8
#define MENU_2_1AND2_P3	9
#define MENU_2_3		10
#define MENU_2_4		11
#define MENU_2_5		12
#define MENU_2_6		13
#define MENU_2_6_3		14
#define MENU_2_7		15
#define MENU_2_7_9_P1	16
#define MENU_2_7_9_P2	17

#define MENU_2_9		24
#define MENU_2_9_2		25
#define MENU_2_9_2_4	26
#define MENU_2_9_2_9	27
#define MENU_2_9_8		28
// misc menu & submenus
#define MENU_3			66
// special case - team menu
#define MENU_99			99

int g_menu_state = 0;		// position in menu (which menu is curr shown)
int g_menu_team;			// for team based options
int g_menu_next_state;		// in cases that one step between curr and next is needed

// main menu
char* show_menu_main =
{ "MARINE_BOT main menu\n\n1. Bot Menu\n2. Waypoint Menu\n3. Misc\n4. CANCEL" };
// bot menu & all (possible) submenus
char* show_menu_1 =
{ "MARINE_BOT bot menu\n\n1. Add red marine\n2. Add blue marine\n3. Fill server\n4. Kick marine\n5. Kill marine\n6. Balance teams\n\n\n9. Settings\n0. MAIN" };
char* show_menu_1_9 =
{ "MARINE_BOT bot settings menu\n\n1. SpawnSkill\n2. BotSkill (bots in game)\n3. BotSkillUp\n4. BotSkillDown\n5. AimSkill\n6. Reactions\n7. Random SpawnSkill\n8. PassiveHealing\n9. BACK\n0. MAIN" };
char* show_menu_1_9_1 =
{ "MARINE_BOT skill levels menu\n\n1. level 1(best)\n2. level 2\n3. level 3(default)\n4. level 4\n5. level 5(worst)\n\n\n\n9. BACK\n0. MAIN" };
char* show_menu_1_9_6 =
{ "MARINE_BOT bot reactions menu\n\n1. 0.0 s(best)\n2. 0.1 s\n3. 0.2 s(default)\n4. 0.5 s\n5. 1.0 s\n6. 1.5 s\n7. 2.5 s\n8. 5.0 s(worst)\n9. BACK\n0. MAIN" };
// waypoint menu & all (possible) submenus
char* show_menu_2 =
{ "MARINE_BOT waypoint menu\n\n1. Placing\n2. Changing\n3. Priority\n4. Time\n5. Range\n6. Autowaypoint\n7. SettingPaths\n\n9. Service\n0. MAIN" };
char* show_menu_2a =
{ "MARINE_BOT waypoint menu\n\n1. Placing\n2. unavailable\n3. unavailable\n4. unavailable\n5. unavailable\n6. Autowaypoint\n7. SettingPaths\n\n9. Service\n0. MAIN" };
char* show_menu_2_1 =
{ "MARINE_BOT waypoint menu\n\n1. Normal/Std\n2. Delete\n3. Aim\n4. Ammobox\n5. Cross\n6. Crouch\n7. Door\n8. UseDoor\n9. NEXT\n0. MAIN" };
char* show_menu_2_2 =
{ "MARINE_BOT waypoint menu\n\n1. Normal/Std\n\n3. Aim\n4. Ammobox\n5. Cross\n6. Crouch\n7. Door\n8. UseDoor\n9. NEXT\n0. MAIN" };
char* show_menu_2_1and2_p2 =
{ "MARINE_BOT waypoint menu\n\n1. GoBack\n2. Jump\n3. DuckJump\n4. Ladder\n5. Parachute\n6. Prone\n7. Shoot\n8. Use\n9. NEXT\n0. MAIN" };
char* show_menu_2_1and2_p3 =
{ "MARINE_BOT waypoint menu\n\n1. Claymore\n2. Sniper\n3. Sprint \n4. PushPoint/Flag \n5. Trigger \n \n \n \n9. BACKTOTOP\n0. MAIN" };
char* show_menu_2_3 =
{ "MARINE_BOT priority menu\n\n1. level 1(highest)\n2. level 2\n3. level 3\n4. level 4\n5. level 5(lowest/default)\n6. level 0(no)\n\n\n9. BACK\n0. MAIN" };
char* show_menu_2_4 =
{ "MARINE_BOT time menu\n\n1. 1 second\n2. 2 seconds\n3. 3 seconds\n4. 4 seconds\n5. 5 seconds\n6. 10 seconds\n7. 20 seconds\n8. 30 seconds\n9. BACK\n0. MAIN" };
char* show_menu_2_5 =
{ "MARINE_BOT range menu\n\n1. 10 units\n2. 20 units\n3. 30 units\n4. 40 units\n5. 50 units\n6. 75 units\n7. 100 units\n8. 125 units\n9. 150 units\n0. MAIN" };
char* show_menu_2_6 =
{ "MARINE_BOT autowaypoint menu\n\n1. Start autowaypoint\n2. Stop autowaypoint\n3. Set distance\n\n\n\n\n\n\n0. MAIN" };
char* show_menu_2_6_3 =
{ "MARINE_BOT distance menu\n\n1. 80 units(minimum)\n2. 100 units\n3. 120 units\n4. 160 units\n5. 200 units(default)\n6. 280 units\n7. 340 units\n8. 400 units(maximum)\n9. BACK\n0. MAIN" };
char* show_menu_2_7 =
{ "MARINE_BOT path menu\n\n1. Show\\Hide paths\n2. Start path\n3. Stop path\n4. Continue path\n5. Add to\n6. Remove from\n7. Delete path\n8. AutoAdding on\\off\n9. Path flags\n0. MAIN" };
char* show_menu_2_7a =
{ "MARINE_BOT path menu\n\n1. Show\\Hide paths\n2. unavailable\n3. Stop path\n4. Continue path\n5. unavailable\n6. unavailable\n7. unavailable\n8. AutoAdding on\\off\n9. unavailable\n0. MAIN" };
char* show_menu_2_7_9_p1 =
{ "MARINE_BOT path flags menu\n\n1. One-way\n2. Two-way\n3. Patrol type\n4. Red team\n5. Blue team\n6. Both teams\n7. Snipers only\n8. MGunners only\n9. NEXT\n0. MAIN" };
char* show_menu_2_7_9_p2 =
{ "MARINE_BOT path flags menu\n\n1. All classes\n2. Avoid Enemy\n3. Ignore Enemy\n4. Carry Item\n\n\n\n\n9. BACK\n0. MAIN" };
char* show_menu_2_9 =
{ "MARINE_BOT service menu\n\n1. Show\\Hide wpts\n2. Adv. debugging\n3. Load wpts&paths\n4. Save wpts&paths\n5. Convert older wpts\n6. Clear wpts\n7. Destroy wpts\n8. Change wpt folder\n\n0. MAIN" };
char* show_menu_2_9_2 =
{ "MARINE_BOT debugging menu\n\n1. Check cross\n2. Check aim\n3. Check range\n4. Path highlight\n\n\n\n\n9. Draw distance\n0. MAIN" };
char* show_menu_2_9_2_4 =
{ "MARINE_BOT path highlight menu\n\n1. Red team\n2. Blue team\n3. One-way path\n4. Sniper path\n5. Machine gunner path\n\n\n\n9. Turn off\n0. MAIN" };
char* show_menu_2_9_2_9 =
{ "MARINE_BOT draw distance menu\n\n1. 400 units\n2. 500 units\n3. 600 units\n4. 700 units\n5. 800 units(default)\n6. 900 units\n7. 1000 units\n8. 1200 units\n9. 1400 units(maximum)\n0. MAIN" };
char* show_menu_2_9_8 =
{ "MARINE_BOT folder select menu\n\n1. Default\n2. Custom\n\n\n\n\n\n\n9. BACK\n0. MAIN" };
// misc menu & all (possible) submenus
char* show_menu_3 =
{ "MARINE_BOT misc menu\n\n1. Observer\n2. MrFreeze\n3. BotDontShoot\n4. BotIgnoreAll\n5. BotDontSpeak\n6. MBNoClip\n\n\n\n0. MAIN" };
// special case - team menu
char* show_menu_99 =
{ "MARINE_BOT team select menu\n\n1. Red team\n2. Blue team\n\n\n\n\n\n\n\n0. MAIN" };
char* show_menu_99a =
{ "MARINE_BOT team select menu\n\n1. Red team\n2. Blue team\n3. Both teams\n\n\n\n\n\n\n0. MAIN" };


extern bool is_dedicated_server;
extern char mb_version_info[32];

#ifdef _DEBUG
extern int debug_engine;
extern edict_t* pRecipient;

extern botname_t bot_names[MAX_BOT_NAMES];
extern edict_t* g_debug_bot;
extern bool g_debug_bot_on;

extern Section* conf_weapons;

inline bool CheckForSomeDebuggingCommands(edict_t* pEntity, const char* pcmd, const char* arg1, const char* arg2, const char* arg3, const char* arg4, const char* arg5);	// debugging commands
#endif

/*
* all Marine Bot listen server commands
* you must end each main 'if' branch with return
* otherwise you'll get 'unknown command' error in game
*/
bool CustomClientCommands(edict_t* pEntity, const char* pcmd, const char* arg1, const char* arg2, const char* arg3, const char* arg4, const char* arg5)
{
	char msg[126];

	if ((FStrEq(pcmd, "help")) || (FStrEq(pcmd, "?")))
	{
		if (FStrEq(arg1, "botsetup"))
		{
			// we don't need to show help in notify area (top left corner on the screen)
			// therefore we send them directly to console
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "\n***Additional bot settings***\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[kick <\"name\">] kicks bot with specified name (name must be in double quotes)\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[killbot <arg>] 'arg' could be <all> - kills all bots, <red/blue> - kills all red/blue team bots currently in the game\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[randomskill] toggles between using default bot skill and generating the skill randomly for each bot\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[spawnskill <number>] sets default bot skill on join (1-5 where 1=best, no effect to bots already in game)\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[setbotskill <number>] sets bot skill level for all bots already in game (1-5 where 1=best)\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[botskillup] increases bot skill level for all bots already in game (bots will be better)\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[botskilldown] decreases bot skill level for all bots already in game (bots will be worse)\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[setaimskill <number>] sets aim skill level for all bots already in game (1-5 where 1=best)\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[botskillview] prints botskill & aimskill for all bots\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[reactiontime <number>] sets bot reaction time (0-50 where 1 will be converted to 0.1s and 50 to 5.0s)\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[rangelimit <number>] sets the max distance of enemy the bot can see & attack (500-7500 units)\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[botdontspeak <off>] bots will not use Voice&Radio commands, off parameter returns to normal\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[botdontchat <off>] bots will not use say&say_team commands, off parameter returns to normal\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[passivehealing <off>] bots will not heal teammates automatically, off parameter returns to normal\n");
		}
		else if (FStrEq(arg1, "cheats"))
		{
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "\n***Should help in troubles***\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[observer <off>] bot ignores you, off parameter returns to normal\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[mrfreeze <off>] bot doesnt move, off parameter returns to normal\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[botdontshoot <off>] bot doesnt shoot, off parameter returns to normal\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[botignoreall <off>] bot ignores all enemies, off parameter returns to normal\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[mbnoclip <off>] you will be able to fly through all on the map, off parameter returns to normal\n");
		}
		else if (FStrEq(arg1, "misc"))
		{
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "\n***Miscellaneous commands***\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[check_aims] connects 'wait timed' waypoint with its valid aim waypoints\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[check_cross] connects cross waypoint with all waypoints in range\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[check_ranges] draws (highlight) waypoint range\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[get_wpt_system] prints the system the waypoint file is created in\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[get_texture_name] prints the name of the texture in front of you\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[mbwptmenu] shortcut to MB waypoint menu\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[mbwptmenuadd] shortcut to MB add waypoint menu\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[mbwptmenuchange] shortcut to MB change waypoint menu\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[mbwptmenupath] shortcut to MB path menu\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[mbwptmenupathflags] shortcut to MB change path flags menu\n");
		}
		else
		{
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "\n-------------------------------------------\nMarineBot console commands help\n-------------------------------------------\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[recruit <team> <class> <skin> <skill> <name>] adds bot with specified params, if no params all except skill is taken randomly\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[kickbot <arg>] 'arg' could be <all> - kicks all bots, <red/blue> - kicks one red/blue team bot out of the game\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[balanceteams <off>] tries balancing teams on server (moves bots to weaker team), 'off' disables autobalancing\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[fillserver <number>] fills the server with bots up to maxplayers limit or up to 'number' limit\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[marinebotmenu] shows MB command menu, unavailable since FA2.8\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[directory <name>] toggles through waypoint directories or directly sets one via 'name'\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[versioninfo] prints MB version\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "\n<><><><><><><><><><><><><><><><><><><<><><><><><><>\nfor additional help write one of following commands (without brackets)\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "<><><><><><><><><><><><><><><><><><><<><><><><><><>\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[help <botsetup>] botsetting help | [help <cheats>] few cheats | [help <misc>] various settings help\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[wpt <help>] waypointing help | [autowpt <help>] autowaypointing help | [pathwpt <help>] pathwaypointing help\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[debug <help>] debugging help | [test <help>] test help\n");
		}

		return true;
	}
	// add one bot
	else if ((FStrEq(pcmd, "recruit")) || (FStrEq(pcmd, "addbot")) || (FStrEq(pcmd, "addmarine")))
	{
		BotCreate(pEntity, arg1, arg2, arg3, arg4, arg5);
		botmanager.SetBotCheckTime(gpGlobals->time + 2.5);

		return true;
	}
	else if (FStrEq(pcmd, "fillserver"))	// automatically add bots up to maxplayers
	{
		int total_clients;

		total_clients = 0;
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (clients[i].pEntity != nullptr)
				total_clients++;
		}

		if (total_clients >= gpGlobals->maxClients)
		{
			botmanager.ResetListenServerFilling();

			PlaySoundConfirmation(pEntity, SND_FAILED);
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "error - server full\n");
		}
		else
		{
			botmanager.SetListeServerFilling(true);
			botmanager.SetBotCheckTime(gpGlobals->time + 0.5);

			// check if there is specified bot ammount to add (i.e. "arg filling")
			if ((arg1 != nullptr) && (*arg1 != 0))
			{
				int temp = atoi(arg1);

				if ((temp >= 1) && (temp < gpGlobals->maxClients - total_clients))
				{
					botmanager.SetBotsToBeAdded(temp);

					PlaySoundConfirmation(pEntity, SND_DONE);
					sprintf(msg, "%d bots will be added\n", botmanager.GetBotsToBeAdded());
					ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
				}
				else
				{
					botmanager.ResetListenServerFilling();

					PlaySoundConfirmation(pEntity, SND_FAILED);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "error - invalid argument or over maxplayers limit!\n");

					return true;
				}
			}

			ClientPrint(pEntity, HUD_PRINTNOTIFY, "filling the server...\n");
		}

		return true;
	}
	else if (FStrEq(pcmd, "kickbot"))
	{
		if (FStrEq(arg1, "all"))		// kick all bots
		{
			if (UTIL_KickBot(100))
			{
				PlaySoundConfirmation(pEntity, SND_DONE);
				sprintf(msg, "all bots were kicked\n");
			}
			else
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				sprintf(msg, "error - probably no bots\n");
			}
		}
		else if (FStrEq(arg1, "red"))	// kick random red team bot
		{
			if (UTIL_KickBot(100 + TEAM_RED))
			{
				PlaySoundConfirmation(pEntity, SND_DONE);
				sprintf(msg, "random red bot was kicked\n");
			}
			else
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				sprintf(msg, "error - probably no red bots\n");
			}
		}
		else if (FStrEq(arg1, "blue"))	// kick random blue team bot
		{
			if (UTIL_KickBot(100 + TEAM_BLUE))
			{
				PlaySoundConfirmation(pEntity, SND_DONE);
				sprintf(msg, "random blue bot was kicked\n");
			}
			else
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				sprintf(msg, "error - probably no blue bots\n");
			}
		}
		else if ((arg1 == nullptr) || (*arg1 == 0))	// no arg then kick random bot
		{
			if (UTIL_KickBot(-100))
			{
				PlaySoundConfirmation(pEntity, SND_DONE);
				sprintf(msg, "random bot was kicked\n");
			}
			else
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				sprintf(msg, "error - probably no bots\n");
			}
		}
		else
		{
			int index = UTIL_FindBotByName(arg1);

			if (index != -1)
			{
				if (UTIL_KickBot(index))
				{
					PlaySoundConfirmation(pEntity, SND_DONE);
					sprintf(msg, "bot was successfully kicked\n");
				}
				else
					sprintf(msg, "");	// null the output
			}
			else
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				sprintf(msg, "invalid argument!\n");
			}
		}

		ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

		return true;
	}
	else if (FStrEq(pcmd, "killbot"))
	{
		int temp = 0;

		if (FStrEq(arg1, "all"))		// kill all bots
		{
			if (UTIL_KillBot(100))
			{
				PlaySoundConfirmation(pEntity, SND_DONE);
				sprintf(msg, "all bots were killed\n");
			}
			else
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				sprintf(msg, "error - probably no bots\n");
			}
		}
		else if (FStrEq(arg1, "red"))	// kill all red team bots
		{
			if (UTIL_KillBot(100 + TEAM_RED))
			{
				PlaySoundConfirmation(pEntity, SND_DONE);
				sprintf(msg, "all red team bots were killed\n");
			}
			else
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				sprintf(msg, "error - probably no red team bots\n");
			}
		}
		else if (FStrEq(arg1, "blue"))	// kill all blue team bots
		{
			if (UTIL_KillBot(100 + TEAM_BLUE))
			{
				PlaySoundConfirmation(pEntity, SND_DONE);
				sprintf(msg, "all blue team bots were killed\n");
			}
			else
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				sprintf(msg, "error - probably no blue team bots\n");
			}
		}
		else
		{
			int index = UTIL_FindBotByName(arg1);

			if (index != -1)
			{
				if (UTIL_KillBot(index))
				{
					PlaySoundConfirmation(pEntity, SND_DONE);
					sprintf(msg, "bot was successfully killed\n");
				}
				else
					sprintf(msg, "");
			}
			else
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				sprintf(msg, "invalid or missing argument!\n");
			}
		}

		ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

		return true;
	}
	// try balancing teams
	else if ((FStrEq(pcmd, "balanceteams")) || (FStrEq(pcmd, "balance_teams")))
	{
		// turn off autobalance for this map, it will be turned back on after a map change
		if (FStrEq(arg1, "off"))
		{
			// disable it only once
			if (botmanager.IsTeamsBalanceNeeded() && (botmanager.IsOverrideTeamsBalance() == false))
			{
				botmanager.SetOverrideTeamsBalance(true);

				ClientPrint(pEntity, HUD_PRINTNOTIFY, "autobalance is temporary DISABLED\n");
				PlaySoundConfirmation(pEntity, SND_DONE);
			}
			else
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "autobalance is already DISABLED\n");

			return true;
		}

		botmanager.SetTeamsBalanceValue(UTIL_BalanceTeams());

		if (botmanager.GetTeamsBalanceValue() == -1)
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "teams balanced\n");
		else if (botmanager.GetTeamsBalanceValue() > 100)
		{
			sprintf(msg, "kick %d bots from RED team and add them to blue\n", botmanager.GetTeamsBalanceValue() - 100);
			ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
		}
		else if ((botmanager.GetTeamsBalanceValue() > 0) && (botmanager.GetTeamsBalanceValue() < 100))
		{
			sprintf(msg, "kick %d bots from BLUE team and add them to red\n", botmanager.GetTeamsBalanceValue());
			ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
		}
		else if (botmanager.GetTeamsBalanceValue() < -1)
		{
			PlaySoundConfirmation(pEntity, SND_FAILED);
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "ERROR\n");
		}

		return true;
	}
	// change if the bot will use default spawn skill or pick a skill randomly
	else if ((FStrEq(pcmd, "randomskill")) || (FStrEq(pcmd, "random_skill")))
	{
		RandomSkillCommand(pEntity, arg1);

		return true;
	}
	// set bot skill at spawn
	else if ((FStrEq(pcmd, "spawnskill")) || (FStrEq(pcmd, "spawn_skill")))
	{
		SpawnSkillCommand(pEntity, arg1);

		return true;
	}
	// change bot skill - apply to all bots
	else if ((FStrEq(pcmd, "setbotskill")) || (FStrEq(pcmd, "set_botskill")))
	{
		SetBotSkillCommand(pEntity, arg1);

		return true;
	}
	// increase bot skill - apply to all bots
	else if ((FStrEq(pcmd, "botskillup")) || (FStrEq(pcmd, "botskill_up")))
	{
		BotSkillUpCommand(pEntity, arg1);

		return true;
	}
	// decrease bot skill - apply to all bots
	else if ((FStrEq(pcmd, "botskilldown")) || (FStrEq(pcmd, "botskill_down")))
	{
		BotSkillDownCommand(pEntity, arg1);

		return true;
	}
	// change aim skill - apply to all bots
	else if ((FStrEq(pcmd, "setaimskill")) || (FStrEq(pcmd, "set_aimskill")))
	{
		SetAimSkillCommand(pEntity, arg1);

		return true;
	}
	// show botskill & aimskill for all bots
	else if (FStrEq(pcmd, "botskillview"))
	{
		int i, j;
		char client_name[32];

		ClientPrint(pEntity, HUD_PRINTCONSOLE, "\n-----------------------------\n");

		for (i = 0; i < MAX_CLIENTS; i++)
		{
			if (bots[i].is_used == FALSE)
				continue;

			for (j = 0; j < MAX_CLIENTS; j++)
			{
				if (bots[i].pEdict == clients[j].pEntity)
					break;
			}

			strcpy(client_name, STRING(clients[j].pEntity->v.netname));

			sprintf(msg, "%s's bot_skill:%d and aim_skill:%d\n", client_name,
				bots[i].bot_skill + 1, bots[i].aim_skill + 1);
			ClientPrint(pEntity, HUD_PRINTCONSOLE, msg);
		}
		ClientPrint(pEntity, HUD_PRINTCONSOLE, "-----------------------------\n\n");

		return true;
	}
	// set bot reaction_time
	else if ((FStrEq(pcmd, "reactiontime")) || (FStrEq(pcmd, "reaction_time")))
	{
		SetReactionTimeCommand(pEntity, arg1);

		return true;
	}
	// bot will not heal teammates automatically
	else if ((FStrEq(pcmd, "passivehealing")) || (FStrEq(pcmd, "passive_healing")))
	{
		PassiveHealingCommand(pEntity, arg1);

		return true;
	}
	// set max distance of enemy the bot can see and thus can engage
	else if ((FStrEq(pcmd, "rangelimit")) || (FStrEq(pcmd, "range_limit")))
	{
		RangeLimitCommand(pEntity, arg1);

		return true;
	}
	else if (FStrEq(pcmd, "directory"))	// change waypoint folder
	{
		SetWaypointDirectoryCommand(pEntity, arg1);

		return true;
	}
	else if (FStrEq(pcmd, "marinebotmenu"))		// MarineBot command menu
	{
		g_menu_state = MENU_MAIN;
		UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_main);

		return true;
	}
	else if (FStrEq(pcmd, "mbwptmenu"))			// shortcut to waypoint menu
	{
		g_menu_state = MENU_2;
		int is_wpt_near = -1;

		if (num_waypoints < 1)
			is_wpt_near = -1;
		else
			is_wpt_near = WaypointFindNearest(pEntity, 50.0, -1);

		if (is_wpt_near != -1)
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2);
		else
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2a);

		return true;
	}
	else if (FStrEq(pcmd, "mbwptmenuadd"))		// shortcut to waypoint menu (add wpt)
	{
		g_menu_state = MENU_2_1AND2_P1;
		g_menu_next_state = 128;		// to know that's adding

		UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_1);

		return true;
	}
	else if (FStrEq(pcmd, "mbwptmenuchange"))	// shortcut to waypoint menu (change wpt)
	{
		g_menu_state = MENU_2_1AND2_P1;

		int is_wpt_near = -1;

		if (num_waypoints < 1)
			is_wpt_near = -1;
		else
			is_wpt_near = WaypointFindNearest(pEntity, 50.0, -1);

		if (is_wpt_near != -1)
		{
			g_menu_next_state = 129;		// to know that's changing

			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_2);
		}

		return true;
	}
	else if (FStrEq(pcmd, "mbwptmenupath"))		// shortcut to waypoint menu (setting path)
	{
		g_menu_state = MENU_2_7;

		int is_wpt_near = -1;

		if (num_waypoints < 1)
			is_wpt_near = -1;
		else
			is_wpt_near = WaypointFindNearest(pEntity, 50.0, -1);

		if (is_wpt_near != -1)
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_7);
		else
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_7a);

		return true;
	}
	else if (FStrEq(pcmd, "mbwptmenupathflags"))	// shortcut to waypoint menu (changing path flags)
	{
		g_menu_state = MENU_2_7_9_P1;

		int is_wpt_near = -1;

		if (num_waypoints < 1)
			is_wpt_near = -1;
		else
			is_wpt_near = WaypointFindNearest(pEntity, 50.0, -1);

		if (is_wpt_near != -1)
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_7_9_p1);

		return true;
	}
	else if ((FStrEq(pcmd, "versioninfo")) || (FStrEq(pcmd, "version_info")))	// version info
	{
		sprintf(msg, "You are running MarineBot in version %s\n", mb_version_info);

		// display it also on HUD using same style as MB intro messages
		Vector color1 = Vector(200, 50, 0);
		Vector color2 = Vector(0, 250, 0);
		CustHudMessage(pEntity, msg, color1, color2, 2, 8);

		ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

		return true;
	}
	else if (FStrEq(pcmd, "observer"))		// bots ignore player
	{
		if (botdebugger.IsObserverMode() || FStrEq(arg1, "off"))
		{
			botdebugger.ResetObserverMode();
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "observer mode DISABLED\n");
		}
		else
		{
			botdebugger.SetObserverMode(true);
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "observer mode ENABLED\n");
		}

		return true;
	}
	else if (FStrEq(pcmd, "mrfreeze"))		// bot is stood/don't move
	{
		if (botdebugger.IsFreezeMode() || FStrEq(arg1, "off"))
		{
			botdebugger.ResetFreezeMode();
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "freeze mode DISABLED\n");
		}
		else
		{
			botdebugger.SetFreezeMode(true);
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "freeze mode ENABLED\n");
		}

		return true;
	}
	else if (FStrEq(pcmd, "botdontshoot"))	// bot will NOT press trigger
	{
		if (botdebugger.IsDontShoot() || FStrEq(arg1, "off"))
		{
			botdebugger.ResetDontShoot();
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "botdontshoot mode DISABLED\n");
		}
		else
		{
			botdebugger.SetDontShoot(true);
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "botdontshoot mode ENABLED\n");
		}

		return true;
	}
	else if (FStrEq(pcmd, "botignoreall"))	// bot ignore all opponents
	{
		if (botdebugger.IsIgnoreAll() || FStrEq(arg1, "off"))
		{
			botdebugger.ResetIgnoreAll();
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "botignoreall mode DISABLED\n");
		}
		else
		{
			botdebugger.SetIgnoreAll(true);
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "botignoreall mode ENABLED\n");
		}

		return true;
	}
	// bot will not use Voice&Radio cmds
	else if ((FStrEq(pcmd, "botdontspeak")) || (FStrEq(pcmd, "dont_speak")))
	{
		DontSpeakCommand(pEntity, arg1);

		return true;
	}
	// bot will/won't use say and say_team commands
	else if ((FStrEq(pcmd, "botdontchat")) || (FStrEq(pcmd, "dont_chat")))
	{
		DontChatCommand(pEntity, arg1);

		return true;
	}
	// client who use this command will switch to no clipping mode
	else if (FStrEq(pcmd, "mbnoclip"))
	{
		if ((pEntity->v.movetype == MOVETYPE_NOCLIP) || (FStrEq(arg1, "off")))
		{
			pEntity->v.movetype = MOVETYPE_WALK;
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "noclipping mode DISABLED\n");
		}
		else
		{
			pEntity->v.movetype = MOVETYPE_NOCLIP;
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "noclipping mode ENABLED\n");
		}

		return true;
	}
	// show valid connections
	else if (FStrEq(pcmd, "check_aims") || FStrEq(pcmd, "checkaims"))
	{
		// turn off this first to prevent overload on netchannel
		wptser.ResetCheckRanges();

		if (wptser.IsCheckAims() || FStrEq(arg1, "off"))
		{
			wptser.ResetCheckAims();
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "check_aims DISABLED!\n");
		}
		else
		{
			wptser.SetCheckAims(true);
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "check_aims ENABLED!\n");
		}

		return true;
	}
	// show valid connections
	else if (FStrEq(pcmd, "check_cross") || FStrEq(pcmd, "checkcross"))
	{
		// turn off this first to prevent overload on netchannel
		wptser.ResetCheckRanges();

		if (wptser.IsCheckCross() || FStrEq(arg1, "off"))
		{
			wptser.ResetCheckCross();
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "check_cross DISABLED!\n");
		}
		else
		{
			wptser.SetCheckCross(true);
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "check_cross ENABLED!\n");
		}

		return true;
	}
	// highlight waypoint ranges
	else if (FStrEq(pcmd, "check_ranges") || FStrEq(pcmd, "checkranges"))
	{
		// turn off these two first to prevent overload on netchannel
		wptser.ResetCheckAims();
		wptser.ResetCheckCross();

		if (wptser.IsCheckRanges() || FStrEq(arg1, "off"))
		{
			wptser.ResetCheckRanges();
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "check_ranges DISABLED!\n");
		}
		else
		{
			wptser.SetCheckRanges(true);
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "check_ranges ENABLED!\n");
		}

		return true;
	}
	// print the waypoint system version
	else if ((FStrEq(pcmd, "getwptsystem")) || (FStrEq(pcmd, "get_wpt_system")))
	{
		int version = WaypointGetSystemVersion();

		if (version == -1)
			sprintf(msg, "file doesn't exist or not a MB waypoint file!\n");
		else
			sprintf(msg, "this waypoint file uses waypoint system version %d.0\ncurrent waypoint system is in version %d.0\n",
				version, WAYPOINT_VERSION);

		ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

		return true;
	}
	else if ((FStrEq(pcmd, "gettexturename")) || (FStrEq(pcmd, "get_texture_name")))
	{
		UTIL_MakeVectors(pEntity->v.v_angle);

		Vector vecStart = pEntity->v.origin + pEntity->v.view_ofs;
		Vector vecEnd = vecStart + gpGlobals->v_forward * 100;

		TraceResult tr;
		UTIL_TraceLine(vecStart, vecEnd, dont_ignore_monsters, dont_ignore_glass, pEntity, &tr);

		const char* texture = g_engfuncs.pfnTraceTexture(tr.pHit, vecStart, vecEnd);

		if (texture == nullptr)
			sprintf(msg, "unable to get the texture name or you are too far!\n");
		else
			sprintf(msg, "the name of the texture in front is \"%s\"\n", texture);
		ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

		return true;
	}
	else if (FStrEq(pcmd, "debug"))
	{
		if ((FStrEq(arg1, "help")) || (FStrEq(arg1, "?")))
		{
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "\nAll valid debugging commands you can use\n***TIP: best results are when used with just one bot in game!***\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "--------------------------------\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[debug_actions] prints info about actions the bot does\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[debug_aim_targets] prints valid aim indexes\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[debug_cross] prints details how is bot deciding at cross waypoint\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[debug_paths] prints info about path the bot is following\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[debug_waypoints] prints info about waypoints the bot is heading towards (when not following a path)\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[debug_weapons] prints all weapon indexes the bot spawned with\n");
			//ClientPrint(pEntity, HUD_PRINTCONSOLE, "[] \n");

#ifdef _DEBUG
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[debug_menu] prints info about current state of the botmenu\n");
			//ClientPrint(pEntity, HUD_PRINTCONSOLE, "[debug_engine] \n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[debug_stuck] prints stuff if stuck on something\n");
#endif
		}

		return true;
	}
	else if (FStrEq(pcmd, "debugaimtargets") || FStrEq(pcmd, "debug_aim_targets"))
	{
		if (botdebugger.IsDebugAims() || FStrEq(arg1, "off"))
		{
			botdebugger.ResetDebugAims();
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_aim_targets DISABLED!\n");
		}
		else
		{
			botdebugger.SetDebugAims(true);
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_aim_targets ENABLED!\n");
		}

		return true;
	}
	else if (FStrEq(pcmd, "debugactions") || FStrEq(pcmd, "debug_actions"))
	{
		if (botdebugger.IsDebugActions() || FStrEq(arg1, "off"))
		{
			botdebugger.ResetDebugActions();
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_actions DISABLED!\n");
		}
		else
		{
			botdebugger.SetDebugActions(true);
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_actions ENABLED!\n");
		}

		return true;
	}
	else if (FStrEq(pcmd, "debugcross") || FStrEq(pcmd, "debug_cross"))
	{
		if (botdebugger.IsDebugCross() || FStrEq(arg1, "off"))
		{
			botdebugger.ResetDebugCross();
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_cross DISABLED!\n");
		}
		else
		{
			botdebugger.SetDebugCross(true);
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_cross ENABLED!\n");
		}

		return true;
	}
	else if (FStrEq(pcmd, "debugpaths") || FStrEq(pcmd, "debug_paths"))
	{
		if (botdebugger.IsDebugPaths() || FStrEq(arg1, "off"))
		{
			botdebugger.ResetDebugPaths();
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_paths DISABLED!\n");
		}
		else
		{
			botdebugger.SetDebugPaths(true);
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_paths ENABLED!\n");
		}

		return true;
	}
	else if (FStrEq(pcmd, "debugwaypoints") || FStrEq(pcmd, "debug_waypoints"))
	{
		if (botdebugger.IsDebugWaypoints() || FStrEq(arg1, "off"))
		{
			botdebugger.ResetDebugWaypoints();
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_waypoints DISABLED!\n");
		}
		else
		{
			botdebugger.SetDebugWaypoints(true);
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_waypoints ENABLED!\n");
		}

		return true;
	}
	else if (FStrEq(pcmd, "debugweapons") || FStrEq(pcmd, "debug_weapons"))
	{
		if (botdebugger.IsDebugWeapons() || FStrEq(arg1, "off"))
		{
			botdebugger.ResetDebugWeapons();
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_weapons DISABLED!\n");
		}
		else
		{
			botdebugger.SetDebugWeapons(true);
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_weapons ENABLED!\n");
		}

		return true;
	}
	else if (FStrEq(pcmd, "test"))
	{
		if ((FStrEq(arg1, "help")) || (FStrEq(arg1, "?")))
		{
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "\nAll valid test commands\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[aimcode] toggles standard/new aim code; the new aim code isn't fully finished\n");
		}
		else if ((FStrEq(arg1, "aimcode")) || (FStrEq(arg1, "aim_code")))
		{
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "This command isn't available in current Marine Bot version!\n");

			/*/
			extern bool g_test_aim_code;

			if (g_test_aim_code)
			{
				g_test_aim_code = false;
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "experimental aim code DISABLED!\n");
			}
			else
			{
				g_test_aim_code = true;
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "experimental aim code ENABLED!\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "This new aim code isn't finished yet so you may experience a lot of misses and buggy behaviour!\n");
			}
			/**/
		}
		else
		{
			PlaySoundConfirmation(pEntity, SND_FAILED);
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid or missing argument!\n");
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "***TIP: use 'test help' for help***\n");
		}

		return true;
	}
	else if (FStrEq(pcmd, "wpt"))
	{
		if ((FStrEq(arg1, "help")) || (FStrEq(arg1, "?")))
		{
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "\nAll waypoint commands\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[types] for help with the different types of waypoint\t\t[commands] for a list of all the commands you can use on a waypoint\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "--------------------------------\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[on] shows waypoints\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[off] hides waypoints\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[show] toggles show/hide waypoints\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[autosave] toggles auto saving waypoints & paths to special files\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[save] saves waypoints & paths to appropriate files\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[load] loads waypoints & paths from HDD\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[load <name>] loads waypoints & paths from specified 'name' files\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[loadunsupported] loads older waypoints & paths from HDD (it's usually done automatically at map start)\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[loadunsupportedversion5] loads old waypoints & paths made for MB0.8b (i.e. waypoint system version 5)\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[info <arg1> <arg2>] info about close or 'arg' waypoint; 'more' additional info; 'full' all info\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[count] info about total amounts of waypoints\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[compass <arg>] shows rapidly blinking beam that'll lead you to 'arg' waypoint; 'off' will turn it off\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[printall] prints all used waypoints and their flags; repeat this command to list through them\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[triggerevent <arg1> <arg2> <arg3>] allows dynamic priority gating (use 'wpt triggerevent help' for more info)\n");
		}
		else if (FStrEq(arg1, "types"))
		{
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "\nAll valid waypoint types\n--------------------------------\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[normal] default wpt.................[aim] aiming vector\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[ammobox] ammobox use.........[claymore] plant mine\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[cross] crossroad.....................[crouch] duck moves\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[door] autoopen doors..............[dooruse] manuallyopen doors\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[flag] pushpoint position............[goback] turnback&continue\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[jump] jump forward................[duckjump] jump&crouch\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[ladder] ladder down&top.........[parachute] check&jump out\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[prone] go prone.....................[shoot] fire weapon\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[sniper] don't move&snipe........[sprint] sprinting\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[trigger] dynamic gate..............[use] use something\n");
		}
		else if (FStrEq(arg1, "commands"))
		{
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "\nList of all the commands you can use on a waypoint\n--------------------------------\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[add <type>] adds 'type' waypoint\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[move <num>] moves closest or specified waypoint to current player position\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[delete] deletes close waypoint\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[change <type>] changes close waypoint to specified type\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[setpriority <num> <team>] changes 'team' (not specified=both) wpt priority to 'num' (0-5) where 0=no, 1=highest, 5=lowest\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[settime <num> <team>] changes 'team' (not specified=both) wpt time to 'num' (0-500)\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[setrange <num>] sets waypoint range for both teams (0-400)\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[reset <arg1> <arg2> <arg3> <arg4>] resets values specified in args back to default (use 'wpt reset help' for more info)\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[clear] deletes all waypoints (can be still loaded back)\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[destroy] deletes all waypoints & prepares for new creation (can NOT be loaded back)\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[author] shows waypoints creator signature\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[subscribe <name>] adds name (up to 32 chars - no spaces) signature to waypoints\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[modified <name>] adds name (up to 32 chars - no spaces) signature to waypoints\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[drawdistance <num>] sets the max distance the waypoint can be to show on screen\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[position <index>] prints the position of 'index' waypoint and yours too\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[detect <index>] prints all entities like buttons, bandages etc. that are around 'index' waypoint\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[repair <arg1> <arg2>] automatic waypoint repair tools (use 'wpt repair help' for more info)\n");
		}
		else if (FStrEq(arg1, "on"))
		{
			wptser.SetShowWaypoints(true);
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are ON\n");
		}
		else if (FStrEq(arg1, "off"))
		{
			wptser.ResetShowWaypoints();
			// turn paths off too (we don't want to see path beams when waypoints are hidden)
			wptser.ResetShowPaths();

			ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are OFF\n");
		}
		else if (FStrEq(arg1, "show"))
		{
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "***toggle mode used***\n");

			if (wptser.IsShowWaypoints())
			{
				wptser.ResetShowWaypoints();
				wptser.ResetShowPaths();
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are OFF\n");
			}
			else
			{
				wptser.SetShowWaypoints(true);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are ON\n");
			}
		}
		else if (FStrEq(arg1, "add"))	// place a new waypoint
		{
			char* result;

			// if this method doesn't return any error and the waypoints aren't visible yet
			// then they will automatically show
			result = WptAdd(pEntity, arg2);

			if ((strcmp(result, "too many wpts") == 0))
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "ERROR - too many waypoints\n");
			}
			else if ((strcmp(result, "no") == 0))
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "ERROR - there isn't waypoint to toggle\n");
			}
			else if ((strcmp(result, "unknown") == 0))
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid argument or unknown error!\n");
			}
			else if ((strcmp(result, "already_is") == 0))
			{
				// this is the only error where we need to display the waypoints
				// if they are still hidden
				wptser.SetShowWaypoints(true);

				PlaySoundConfirmation(pEntity, SND_FAILED);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "there already is a waypoint at this place!\nplace your new waypoint a few steps away\n");
			}
			else
			{
				sprintf(msg, "'%s' waypoint added SUCCESSFULLY\n", result);

				ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
			}
		}
		else if (FStrEq(arg1, "move"))	// move/reposition waypoint to new position
		{
			int wpt_index;

			if ((arg2 != nullptr) && (*arg2 != 0))
			{
				wpt_index = atoi(arg2);
				// due to array based style
				wpt_index -= 1;
			}
			else
				wpt_index = -1;

			if (WaypointReposition(pEntity, wpt_index))
			{
				PlaySoundConfirmation(pEntity, SND_DONE);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoint successfully moved to new position\n");
			}
			else
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "no waypoint close enough or you can't move it\n");
			}
		}
		else if (FStrEq(arg1, "delete"))	// remove waypoint
		{
			if (wptser.IsShowWaypoints() == false)
				wptser.SetShowWaypoints(true);

			WaypointDelete(pEntity);
		}
		else if (FStrEq(arg1, "autosave"))		// store waypoint data to the file
		{
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "***toggle mode used***\n");
			
			if (internals.IsWaypointsAutoSave())
			{
				internals.ResetWaypointsAutoSave();
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "auto saving waypoints DISABLED\n");

			}
			else
			{
				internals.SetWaypoitsAutoSave(true);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "auto saving waypoints ENABLED\n");
			}
		}
		else if (FStrEq(arg1, "save"))		// store waypoint data to the file
		{
			if (WaypointSave(nullptr))
			{
				if (WaypointPathSave(nullptr))
				{
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints and paths were successfully saved\n");
					PlaySoundConfirmation(pEntity, SND_DONE);
				}
				// there was some error - paths weren't saved
				else
				{
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "some error occurred - only waypoints were saved successfully!\n");
					PlaySoundConfirmation(pEntity, SND_FAILED);
				}
			}
			// there was some error - waypoints as well as paths weren't saved
			else
			{
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "some error occurred - waypoints weren't saved!\n");
				PlaySoundConfirmation(pEntity, SND_FAILED);
			}
		}
		else if (FStrEq(arg1, "load"))		// load waypoints
		{
			if ((arg2 != nullptr) && (*arg2 != 0))
			{
				char temp[64];

				strcpy(temp, arg2);

				if (WaypointLoad(pEntity, temp) > 0)
				{
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints loaded\n");

					if (WaypointPathLoad(pEntity, temp) > 0)
					{
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "paths loaded\n");
						PlaySoundConfirmation(pEntity, SND_DONE);
					}
					else
					{
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "no paths or invalid path file\n");
						PlaySoundConfirmation(pEntity, SND_FAILED);
					}
				}
			}
			else
			{
				if (WaypointLoad(pEntity, nullptr) > 0)
				{
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints loaded\n");

					if (WaypointPathLoad(pEntity, nullptr) > 0)
					{
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "paths loaded\n");
						PlaySoundConfirmation(pEntity, SND_DONE);
					}
					else
					{
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "no paths or invalid path file\n");
						PlaySoundConfirmation(pEntity, SND_FAILED);
					}
				}
			}
		}
		else if (FStrEq(arg1, "loadunsupported"))	// convert old waypoint file to new standards
		{
			if (WaypointLoadUnsupported(pEntity))
			{
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "older waypoints data loaded\n");

				if (WaypointPathLoadUnsupported(pEntity))
				{
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "older paths data loaded\n");
					PlaySoundConfirmation(pEntity, SND_DONE);
				}
				else
				{
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "older paths weren't loaded\n");
					PlaySoundConfirmation(pEntity, SND_FAILED);
				}
			}
			else
			{
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "older waypoints weren't loaded\n");
				PlaySoundConfirmation(pEntity, SND_FAILED);
			}
		}
		// convert waypoints made in version 5 ... waypoint system in MB0.8b
		else if (FStrEq(arg1, "loadunsupportedver5") || FStrEq(arg1, "loadunsupportedversion5"))
		{
			if (WaypointLoadVersion5(pEntity))
			{
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "old waypoints data loaded\n");

				if (WaypointPathLoadVersion5(pEntity))
				{
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "old paths data loaded\n");
					PlaySoundConfirmation(pEntity, SND_DONE);
				}
				else
				{
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "old paths weren't loaded\n");
					PlaySoundConfirmation(pEntity, SND_FAILED);
				}
			}
			else
			{
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "old waypoints weren't loaded\n");
				PlaySoundConfirmation(pEntity, SND_FAILED);
			}
		}
		else if (FStrEq(arg1, "clear"))		// delete all waypoints
		{
			WaypointInit();

			ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints cleared\n");
		}
		else if (FStrEq(arg1, "destroy"))	// delete all waypoints & free the file
		{
			WaypointDestroy();

			ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints destroyed\n");
		}
		else if (FStrEq(arg1, "change"))	// change waypoint flag (or type if you want)
		{
			char* result;

			result = WptFlagChange(pEntity, arg2);

			if (result == "no")
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				sprintf(msg, "NO waypoint close enough\n");
			}
			else if (result == "false")
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				sprintf(msg, "invalid argument!\n");
			}
			else if (strcmp((const char*)result, "removed") == 0)
			{
				PlaySoundConfirmation(pEntity, SND_DONE);
				sprintf(msg, "'%s' flag was successfully removed\n", arg2);
			}
			else
			{
				PlaySoundConfirmation(pEntity, SND_DONE);
				sprintf(msg, "changed to '%s' waypoint\n", result);
			}

			ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
		}
		else if (FStrEq(arg1, "setpriority"))	// change waypoint priority
		{
			int result;

			result = WptPriorityChange(pEntity, arg2, arg3);

			if (result == -4)
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				sprintf(msg, "NO waypoint close enough\n");
			}
			else if (result == -3)
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				sprintf(msg, "invalid priority value (0-5)!\n");
			}
			else if (result == -2)
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				sprintf(msg, "invalid team value (red/blue)!\n");
			}
			else if (result == -1)
			{
				PlaySoundConfirmation(pEntity, SND_DONE);
				sprintf(msg, "priority/priorities already is/are on the waypoint\n");
			}
			else if (result == 0)
			{
				PlaySoundConfirmation(pEntity, SND_DONE);

				if ((arg3 != nullptr) && (*arg3 != 0))
					sprintf(msg, "priority changed to NO priority!\n");
				else
					sprintf(msg, "both priorities changed to NO priority!\nWARNING - this waypoint is ignored by all bots!\n");
			}
			else if (result > 10)
			{
				PlaySoundConfirmation(pEntity, SND_DONE);
				sprintf(msg, "both priorities set to %d\n", result - 10);
			}
			else if (result == -100)
				sprintf(msg, "unknown event in 'setpriority'\n");
			else
			{
				PlaySoundConfirmation(pEntity, SND_DONE);

				if ((arg3 != nullptr) && (*arg3 != 0))
					sprintf(msg, "priority changed to %d\n", result);
				else
					sprintf(msg, "both priorities changed to %d\n", result);
			}

			ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
		}
		else if (FStrEq(arg1, "settime"))	// change waypoint time
		{
			float result;

			result = WptTimeChange(pEntity, arg2, arg3);

			if (result == -4.0)
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				sprintf(msg, "NO waypoint close enough\n");
			}
			else if (result == -3.0)
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				sprintf(msg, "invalid time value (0-500)!\n");
			}
			else if (result == -2.0)
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				sprintf(msg, "invalid team value (red/blue)!\n");
			}
			else if (result == -1.0)
			{
				PlaySoundConfirmation(pEntity, SND_DONE);
				sprintf(msg, "time/times already is/are on the waypoint\n");
			}
			else if (result > 1000.0)
			{
				PlaySoundConfirmation(pEntity, SND_DONE);
				result -= (float)1000.0;
				sprintf(msg, "both times set to %.1f\n", result);
			}
			else if (result == -100.0)
				sprintf(msg, "unknown event in 'settime'\n");
			else
			{
				PlaySoundConfirmation(pEntity, SND_DONE);

				if ((arg3 != nullptr) && (*arg3 != 0))
					sprintf(msg, "time changed to %.1f\n", result);
				else
					sprintf(msg, "both times changed to %.1f\n", result);
			}

			ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
		}
		else if (FStrEq(arg1, "setrange"))	// change waypoint range
		{
			float result;

			result = WptRangeChange(pEntity, arg2);

			if (result == -3.0)
			{
				PlaySoundConfirmation(pEntity, SND_DONE);
				sprintf(msg, "this range already is on the waypoint\n");
			}
			else if (result == -2.0)
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				sprintf(msg, "NO waypoint close enough\n");
			}
			else if (result == -1.0)
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				sprintf(msg, "invalid or missing argument!\n");
			}
			else
			{
				PlaySoundConfirmation(pEntity, SND_DONE);
				sprintf(msg, "waypoint range changed to %.1f\n", result);
			}

			ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
		}
		else if (FStrEq(arg1, "reset"))	// reset all waypoint additional info (priority, time etc.)
		{
			if ((FStrEq(arg2, "help")) || (FStrEq(arg2, "?")))
			{
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "All valid reset options\n---------------------------\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "<all> resets all values of close waypoint\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "<type> or <flag> or <tag> resets close waypoint type back to normal waypoint\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "<priority> resets the priority value of close waypoint for both teams\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "<time> resets the wait time value of close waypoint for both teams\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "<range> resets the range value of close waypoint\n");
			}
			else if (WptResetAdditional(pEntity, arg2, arg3, arg4, arg5) == TRUE)
			{
				PlaySoundConfirmation(pEntity, SND_DONE);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "all or some of waypoint values have been reset back to default\n");
			}
			else
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "NO waypoint close enough\n");
			}
		}
		else if (FStrEq(arg1, "info"))
		{
			WaypointPrintInfo(pEntity, arg2, arg3);
		}
		else if (FStrEq(arg1, "count"))			// print number of available wpts
		{
			sprintf(msg, "waypoints already used= %d still could use= %d out of %d\n",
				num_waypoints, MAX_WAYPOINTS - num_waypoints, MAX_WAYPOINTS);
			ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
		}
		else if (FStrEq(arg1, "compass"))
		{
			if (WaypointCompass(pEntity, arg2))
				PlaySoundConfirmation(pEntity, SND_DONE);
			else
			{
				// something went wrong so turn it off
				wptser.ResetCompassIndex();

				PlaySoundConfirmation(pEntity, SND_FAILED);
			}
		}
		else if (FStrEq(arg1, "printall"))
		{
			WaypointPrintAllWaypoints(pEntity);
		}
		else if ((FStrEq(arg1, "triggerevent")) || (FStrEq(arg1, "trigger_event")))	// handle the triggers
		{
			if ((FStrEq(arg2, "help")) || (FStrEq(arg2, "?")))
			{
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "\nAll valid trigger event options\n--------------------------------\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "<add> <trigger_name> <message> adds a message to given trigger slot\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "<delete> <trigger_name> erases given trigger slot\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "<setpriority> <prior> <team> sets the second (ie. trigger) priority, works same as standard priority\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "<settrigger> <trigger_name> <state> connects appropriate trigger msg to close waypoint, state means the 'on' or 'off' event\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "<removetrigger> <state> removes the trigger message from close waypoint based on the state value\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "<info> prints info about the trigger waypoint\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "<showall> prints all trigger event messages\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "<help> shows this help\n");
			}
			else if (FStrEq(arg2, "add"))
			{
				int result = WaypointAddTriggerEvent(arg3, arg4);

				if (result == 1)
				{
					PlaySoundConfirmation(pEntity, SND_DONE);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "trigger event set correctly\n");
				}
				else
				{
					PlaySoundConfirmation(pEntity, SND_FAILED);

					if (result == -1)
					{
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "missing argument!\n");
					}
					else if (result == -2)
					{
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid trigger name!\n");
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "valid trigger names are: trigger1, trigger2 ... trigger8\n");
					}
					else if (result == -3)
					{
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "this trigger is already in use!\n");
					}
					else if (result == -4)
					{
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "the trigger message is too long, shorten the message!\n");
					}
				}
			}
			else if (FStrEq(arg2, "delete"))
			{
				int result = WaypointDeleteTriggerEvent(arg3);

				if (result == 1)
				{
					PlaySoundConfirmation(pEntity, SND_DONE);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "trigger event successfully removed\n");
				}
				else
				{
					PlaySoundConfirmation(pEntity, SND_FAILED);

					if (result == -1)
					{
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "missing argument!\n");
					}
					else if (result == -2)
					{
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid trigger name!\n");
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "valid trigger names are: trigger1, trigger2 ... trigger8\n");
					}
					else if (result == -3)
					{
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "this trigger is already free!\n");
					}
				}
			}
			else if (FStrEq(arg2, "setpriority"))
			{
				int result = WptTriggerPriorityChange(pEntity, arg3, arg4);

				if (result == -4)
				{
					PlaySoundConfirmation(pEntity, SND_FAILED);
					sprintf(msg, "NO trigger waypoint close enough\n");
				}
				else if (result == -3)
				{
					PlaySoundConfirmation(pEntity, SND_FAILED);
					sprintf(msg, "invalid priority value (0-5)!\n");
				}
				else if (result == -2)
				{
					PlaySoundConfirmation(pEntity, SND_FAILED);
					sprintf(msg, "invalid team value (red/blue)!\n");
				}
				else if (result == -1)
				{
					PlaySoundConfirmation(pEntity, SND_DONE);
					sprintf(msg, "priority/priorities already is/are on the waypoint\n");
				}
				else if (result == 0)
				{
					PlaySoundConfirmation(pEntity, SND_DONE);

					if ((arg4 != nullptr) && (*arg4 != 0))
						sprintf(msg, "priority changed to NO priority!\n");
					else
						sprintf(msg, "both priorities changed to NO priority!\nWARNING - this waypoint is ignored by all bots!\n");

				}
				else if (result > 10)
				{
					PlaySoundConfirmation(pEntity, SND_DONE);
					sprintf(msg, "both priorities set to %d\n", result - 10);
				}
				else
				{
					PlaySoundConfirmation(pEntity, SND_DONE);

					if ((arg4 != nullptr) && (*arg4 != 0))
						sprintf(msg, "priority changed to %d\n", result);
					else
						sprintf(msg, "both priorities changed to %d\n", result);
				}

				ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
			}
			else if (FStrEq(arg2, "settrigger"))
			{
				int result = WaypointConnectTriggerEvent(pEntity, arg3, arg4);

				if (result == 1)
				{
					PlaySoundConfirmation(pEntity, SND_DONE);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "trigger set successfully\n");
				}
				else
				{
					PlaySoundConfirmation(pEntity, SND_FAILED);

					if (result == -1)
					{
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "missing argument!\n");
					}
					else if (result == -2)
					{
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid trigger name!\n");
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "valid trigger names are: trigger1, trigger2 ... trigger8\n");
					}
					else if (result == -3)
					{
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid state value!\n");
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "valid state values are 'on' or 'off'\n");
					}
					else if (result == -4)
					{
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "no trigger waypoint around!\n");
					}
				}
			}
			else if (FStrEq(arg2, "removetrigger"))
			{
				int result = WaypointRemoveTriggerEvent(pEntity, arg3);

				if (result == 1)
				{
					PlaySoundConfirmation(pEntity, SND_DONE);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "trigger successfully removed\n");
				}
				else
				{
					PlaySoundConfirmation(pEntity, SND_FAILED);

					if (result == -1)
					{
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "missing argument!\n");
					}
					else if (result == -2)
					{
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid state value!\n");
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "valid state values are 'on' or 'off'\n");
					}
					else if (result == -3)
					{
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "no trigger waypoint around!\n");
					}
				}
			}
			else if (FStrEq(arg2, "info"))
			{
				WaypointTriggerPrintInfo(pEntity);
			}
			else if (FStrEq(arg2, "showall"))
			{
				char msg[512];

				for (int index = 0; index < MAX_TRIGGERS; index++)
				{
					if (trigger_gamestate[index].GetUsed())
					{
						sprintf(msg, "trigger%d holds \"%s\"\n", index + 1, trigger_events[index].message);
					}
					else
						sprintf(msg, "trigger%d is free\n", index + 1);

					ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
				}
			}
			else
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid or missing argument!\n");
			}
		}
		else if (FStrEq(arg1, "subscribe"))		// subscribe waypoint file
		{
			if ((arg2) && (*arg2))
			{
				char temp[32];

				strncpy(temp, arg2, 31);
				temp[31] = 0;

				if (WaypointSubscribe(temp, TRUE))
				{
					WaypointSave(nullptr);		// save them

					// are there any paths then save them too
					if (num_w_paths > 0)
						WaypointPathSave(nullptr);

					PlaySoundConfirmation(pEntity, SND_DONE);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints subscription SUCCESSFUL\n");
				}
				else
				{
					PlaySoundConfirmation(pEntity, SND_FAILED);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints subscription FAILED (probably already subsribed or waypoint file doesn't exist yet)\n");
				}
			}
			else
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "missing argument!\n");
			}
		}
		else if (FStrEq(arg1, "modified"))	// subscribe modified waypoint file
		{
			if ((arg2) && (*arg2))
			{
				if ((FStrEq(arg2, "help")) || (FStrEq(arg2, "?")))
				{
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "\nAll valid options\n---------------------------\n");
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<clear> removes/erases current 'modified by' tag from waypoint file\n");
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<fill_your_name_here> assigns given name to 'modified by' tag in waypoint file\n");
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<help> shows this help\n");
				}
				else
				{
					char temp[32];

					strncpy(temp, arg2, 31);
					temp[31] = 0;

					if (WaypointSubscribe(temp, FALSE))
					{
						WaypointSave(nullptr);		// save them

						if (num_w_paths > 0)
							WaypointPathSave(nullptr);

						PlaySoundConfirmation(pEntity, SND_DONE);
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints subscription SUCCESSFUL\n");
					}
					else
					{
						PlaySoundConfirmation(pEntity, SND_FAILED);
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints subscription FAILED\n");
					}
				}
			}
			else
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "missing argument!\n");
			}
		}
		else if (FStrEq(arg1, "author"))	// print waypoints author
		{
			char author[32];
			char modified_by[32];

			WaypointAuthors(author, modified_by);

			if (FStrEq(author, "nofile"))
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "no waypoint file!\n");
			}
			else
			{
				if (FStrEq(author, "noauthor") && FStrEq(modified_by, "nosig"))
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints aren't subscribed!\n");
				else if (FStrEq(author, "noauthor") && !FStrEq(modified_by, "nosig"))
				{
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints author is unknown!\n");

					sprintf(msg, "waypoints were modified by %s\n", modified_by);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
				}
				else if (FStrEq(modified_by, "nosig"))
				{
					if (IsOfficialWpt(author))
						sprintf(msg, "official MarineBot waypoints by %s\n", author);
					else
						sprintf(msg, "waypoints were created by %s\n", author);

					// display it also on HUD using same style as MB intro messages
					Vector color1 = Vector(200, 50, 0);
					Vector color2 = Vector(0, 250, 0);
					CustHudMessage(pEntity, msg, color1, color2, 2, 8);

					ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
				}
				else
				{
					if (IsOfficialWpt(author) || IsOfficialWpt(modified_by))
						sprintf(msg, "official MarineBot waypoints by %s\nmodified by %s\n", author, modified_by);
					else
						sprintf(msg, "waypoints by %s\nmodified by %s\n", author, modified_by);

					Vector color1 = Vector(200, 50, 0);
					Vector color2 = Vector(0, 250, 0);
					CustHudMessage(pEntity, msg, color1, color2, 2, 8);

					ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
				}
			}
		}
		else if (FStrEq(arg1, "drawdistance"))	// change draw distance
		{
			if ((arg2 != nullptr) && (*arg2 != 0))
			{
				int temp = atoi(arg2);

				if ((temp >= 100) && (temp <= 1400))
				{
					wptser.SetWaypointsDrawDistance(temp);

					PlaySoundConfirmation(pEntity, SND_DONE);
					sprintf(msg, "waypoints draw distance set to %d\n", temp);
				}
				else
				{
					PlaySoundConfirmation(pEntity, SND_FAILED);
					sprintf(msg, "invalid draw distance value!\n");
				}

				ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
			}
		}
		else if ((FStrEq(arg1, "position")) || (FStrEq(arg1, "pos")))	// print waypoint origin
		{
			if ((arg2 != nullptr) && (*arg2 != 0))
			{
				int wpt_index = atoi(arg2) - 1;

				if ((wpt_index >= 0) && (wpt_index < num_waypoints))
				{
					if (IsWaypoint(wpt_index, WptT::wpt_no))
					{
						PlaySoundConfirmation(pEntity, SND_FAILED);
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoint doesn't exist (was deleted)!\n");
					}
					else
					{
						PlaySoundConfirmation(pEntity, SND_DONE);
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "Waypoint position is:\n");

						sprintf(msg, "X-coord: %.2f \t\t Y-coord: %.2f \t\t Z-coord (height): %.2f \n",
							waypoints[wpt_index].origin.x, waypoints[wpt_index].origin.y, waypoints[wpt_index].origin.z);
						ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

						ClientPrint(pEntity, HUD_PRINTNOTIFY, "Your position is:\n");

						sprintf(msg, "X-coord: %.2f \t\t Y-coord: %.2f \t\t Z-coord (height): %.2f \n",
							pEntity->v.origin.x, pEntity->v.origin.y, pEntity->v.origin.z);
						ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
					}
				}
				else
				{
					PlaySoundConfirmation(pEntity, SND_FAILED);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid waypoint index!\n");
				}
			}
			else
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid or missing argument!\n");
			}
		}
		else if (FStrEq(arg1, "detect"))	// print all entities around waypoint
		{
			if ((arg2 != nullptr) && (*arg2 != 0))
			{
				int wpt_index = atoi(arg2) - 1;

				if ((wpt_index >= 0) && (wpt_index < num_waypoints))
				{
					if (IsWaypoint(wpt_index, WptT::wpt_no))
					{
						PlaySoundConfirmation(pEntity, SND_FAILED);
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoint doesn't exist (was deleted)!\n");
					}
					else
					{
						edict_t* pent = nullptr;
						bool no_entities = true;

						PlaySoundConfirmation(pEntity, SND_DONE);
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "Checking entities around ...\n");

						while ((pent = UTIL_FindEntityInSphere(pent, waypoints[wpt_index].origin,
							PLAYER_SEARCH_RADIUS)) != nullptr)
						{
							no_entities = false;

							sprintf(msg, "found: %s\n", STRING(pent->v.classname));
							ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
						}

						if (no_entities)
						{
							ClientPrint(pEntity, HUD_PRINTNOTIFY, "found nothing!\n");
						}
					}
				}
			}
		}
		else if (FStrEq(arg1, "repair"))	// handle waypoint repair functions
		{
			if ((FStrEq(arg2, "help")) || (FStrEq(arg2, "?")))
			{
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "\nAll waypoint repair commands\n---------------------------\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[masterrepair] applies all available fixes except for range and position\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[range <index>] will fix range on all waypoints or on the 'index' waypoint\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[range_and_position <index>] will fix range and position on all waypoints or on the 'index' waypoint\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[pathend <index>] will fix missing goback on 'index' path end or all paths if no index specified\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[pathmerge <index>] will fix invalid merge of paths on 'index' path or all paths if no index specified\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[sniperspot <index>] will fix wrongly set sniper spot on 'index' path or all paths if no index specified\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[crosswpt <index>] will fix range on all cross waypoints or on the 'index' cross waypoint\n");
			}
			else if (FStrEq(arg2, "masterrepair"))
			{
				WaypointRepairInvalidPathMerge();
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "all paths were checked for invalid merge and found problems were repaired ...\n");

				WaypointRepairInvalidPathEnd();
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "all paths were checked for missing goback and all available fixes were applied ...\n");

				WaypointRepairSniperSpot();
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "any wrongly set sniper spot that was found was repaired! ...\n");

				WaypointValidatePath();																			// NEW CODE 094
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "all paths were checked for validity and all available fixes were applied ...\n");

				PlaySoundConfirmation(pEntity, SND_DONE);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoint master repair DONE!\n");
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "***TIP: use 'wpt save' command if you want to keep all these changes***\n");
			}
			else if (FStrEq(arg2, "range"))
			{
				if ((arg3 != nullptr) && (*arg3 != 0))
				{
					int wpt_index = atoi(arg3) - 1;
					if (UTIL_RepairWaypointRangeandPosition(wpt_index, pEntity, true))
					{
						PlaySoundConfirmation(pEntity, SND_DONE);
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoint range repair done\n");
					}
					else
					{
						PlaySoundConfirmation(pEntity, SND_FAILED);
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "that waypoint doesn't exist or you aren't allowed to work with this one!\n");
					}
				}
				else
				{
					UTIL_RepairWaypointRangeandPosition(pEntity, true);

					PlaySoundConfirmation(pEntity, SND_DONE);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "range was checked on all waypoints and the bad waypoints were repaired\n");
				}
			}
			else if (FStrEq(arg2, "range_and_position") || FStrEq(arg2, "rangeandposition") ||
				FStrEq(arg2, "rangeandpos"))
			{
				if ((arg3 != nullptr) && (*arg3 != 0))
				{
					int wpt_index = atoi(arg3) - 1;
					if (UTIL_RepairWaypointRangeandPosition(wpt_index, pEntity))
					{
						PlaySoundConfirmation(pEntity, SND_DONE);
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoint range and position repair done\n");
					}
					else
					{
						PlaySoundConfirmation(pEntity, SND_FAILED);
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "that waypoint doesn't exist or you aren't allowed to work with this one!\n");
					}
				}
				else
				{
					UTIL_RepairWaypointRangeandPosition(pEntity);

					PlaySoundConfirmation(pEntity, SND_DONE);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "all waypoints were checked and the bad ones were repaired\n");
				}
			}
			else if (FStrEq(arg2, "pathend"))
			{
				if ((arg3 != nullptr) && (*arg3 != 0))
				{
					int path_index = atoi(arg3) - 1;
					int result = WaypointRepairInvalidPathEnd(path_index);

					switch (result)
					{
					case -1:
						PlaySoundConfirmation(pEntity, SND_FAILED);
						sprintf(msg, "that path doesn't exist or unable to read it\n");
						break;
					case 0:
						PlaySoundConfirmation(pEntity, SND_DONE);
						sprintf(msg, "there is nothing to be fixed on this path\n");
						break;
					case 1:
						PlaySoundConfirmation(pEntity, SND_DONE);
						sprintf(msg, "path #%d was repaired\n", path_index + 1);
						break;
					case 10:
						PlaySoundConfirmation(pEntity, SND_FAILED);
						sprintf(msg, "unable to repair path #%d\n", path_index + 1);
						break;
					}

					ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
				}
				else
				{
					WaypointRepairInvalidPathEnd();

					PlaySoundConfirmation(pEntity, SND_DONE);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "all paths were checked for missing goback and all available fixes were applied\n");
				}
			}
			else if (FStrEq(arg2, "pathmerge"))
			{
				if ((arg3 != nullptr) && (*arg3 != 0))
				{
					int path_index = atoi(arg3) - 1;
					int result = WaypointRepairInvalidPathMerge(path_index);

					switch (result)
					{
					case -1:
						PlaySoundConfirmation(pEntity, SND_FAILED);
						sprintf(msg, "that path doesn't exist or unable to read it\n");
						break;
					case 0:
						PlaySoundConfirmation(pEntity, SND_DONE);
						sprintf(msg, "there is nothing to be fixed on this path\n");
						break;
					case 1:
						PlaySoundConfirmation(pEntity, SND_DONE);
						sprintf(msg, "path #%d was repaired\n", path_index + 1);
						break;
					case 2:
						PlaySoundConfirmation(pEntity, SND_FAILED);
						sprintf(msg, "found suspicious path connection on path #%d\n", path_index + 1);
						break;
					case 3:
						PlaySoundConfirmation(pEntity, SND_DONE);
						sprintf(msg, "path #%d was repaired, but there is also some suspicious path connection on this path\n", path_index + 1);
						break;
					}

					ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
				}
				else
				{
					WaypointRepairInvalidPathMerge();

					PlaySoundConfirmation(pEntity, SND_DONE);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "all paths were checked for invalid merge and found problems were repaired\n");
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "***TIP: all findings are logged to MB error log file***\n");
				}
			}
			else if (FStrEq(arg2, "sniperspot") || FStrEq(arg2, "camperspot"))
			{
				if ((arg3 != nullptr) && (*arg3 != 0))
				{
					int path_index = atoi(arg3) - 1;

					int result = WaypointRepairSniperSpot(path_index);

					switch (result)
					{
					case -1:
						PlaySoundConfirmation(pEntity, SND_FAILED);
						sprintf(msg, "that path doesn't exist or unable to read it\n");
						break;
					case 0:
						PlaySoundConfirmation(pEntity, SND_DONE);
						sprintf(msg, "there is nothing to be fixed on this path\n");
						break;
					case 1:
						PlaySoundConfirmation(pEntity, SND_DONE);
						sprintf(msg, "sniper spot on path #%d was repaired\n", path_index + 1);
						break;
					}

					ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
				}
				else
				{
					WaypointRepairSniperSpot();

					PlaySoundConfirmation(pEntity, SND_DONE);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "any wrongly set sniper spot that was found was repaired!\n");
				}
			}
			else if (FStrEq(arg2, "crosswpt") || FStrEq(arg2, "crossrange"))
			{
				if ((arg3 != nullptr) && (*arg3 != 0))
				{
					int cross_index = atoi(arg3) - 1;
					float original_range = 0.0;

					if ((cross_index != -1) && (cross_index < num_waypoints))
					{
						original_range = waypoints[cross_index].range;
					}

					float result = WaypointRepairCrossRange(cross_index);

					if (result == -1.0)
					{
						PlaySoundConfirmation(pEntity, SND_FAILED);
						sprintf(msg, "that cross waypoint doesn't exist\n");
					}
					else if (original_range == result)
					{
						PlaySoundConfirmation(pEntity, SND_DONE);
						sprintf(msg, "there is nothing to be fixed on this cross waypoint\n");
					}
					else
					{
						PlaySoundConfirmation(pEntity, SND_DONE);
						sprintf(msg, "range on cross waypoint #%d was repaired to connect to all free path ends nearby\n", cross_index + 1);
					}

					ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
				}
				else
				{
					WaypointRepairCrossRange();

					PlaySoundConfirmation(pEntity, SND_DONE);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "any wrongly set range on any cross waypoint that was found was repaired!\n");
				}
			}
			else
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid or missing argument!\n");
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "***TIP: use 'wpt repair help' for help***\n");
			}
		}
		else
		{
			PlaySoundConfirmation(pEntity, SND_FAILED);
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid or missing argument!\n");
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "***TIP: use 'wpt help' for help***\n");

			if (wptser.IsShowWaypoints())
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are ON\n");
			else
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are OFF\n");
		}

		return true;
	}
	else if (FStrEq(pcmd, "autowpt"))
	{
		if ((FStrEq(arg1, "help")) || (FStrEq(arg1, "?")))
		{
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "\nAll autowaypointing commands\n------------------------------\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[on] starts autowaypointing\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[off] stops autowaypointing\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[start] toggles start/stop autowaypointing\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[distance <number>] sets distance (80-400) between two waypoints while autowaypointing\n");
		}
		else if (FStrEq(arg1, "on"))
		{
			StartAutoWaypointg(TRUE);
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "autowaypointing is ON\n");
		}
		else if (FStrEq(arg1, "off"))
		{
			StartAutoWaypointg(FALSE);

			ClientPrint(pEntity, HUD_PRINTNOTIFY, "autowaypointing is OFF\n");
		}
		else if (FStrEq(arg1, "start"))
		{
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "***toggle mode used***\n");

			if (wptser.IsAutoWaypointing())
			{
				// turn autowaypointing off
				StartAutoWaypointg(FALSE);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "autowaypointing is OFF\n");
			}
			else
			{
				StartAutoWaypointg(TRUE);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "autowaypointing is ON\n");
			}
		}
		else if (FStrEq(arg1, "distance"))	// set distance between two wpts for autowpt
		{
			if ((arg2 != nullptr) && (*arg2 != 0))
			{
				int temp = atoi(arg2);

				if ((temp >= MIN_WPT_DIST) && (temp <= MAX_WPT_DIST))
				{
					wptser.SetAutoWaypointingDistance(temp);

					PlaySoundConfirmation(pEntity, SND_DONE);
					sprintf(msg, "autowaypointing distance set to %d\n", temp);
				}
				else
				{
					PlaySoundConfirmation(pEntity, SND_FAILED);
					sprintf(msg, "invalid autowaypointing distance!\n");
				}

				//ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
			}
			else
			{
				sprintf(msg, "current autowaypointing distance value is %.2f\n",
					wptser.GetAutoWaypointingDistance());
			}

			ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
		}
		else
		{
			PlaySoundConfirmation(pEntity, SND_FAILED);
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid or missing argument!\n");
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "***TIP: use 'autowpt help' for help***\n");

			if (wptser.IsAutoWaypointing())
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "autowaypointing is ON\n");
			else
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "autowaypointing is OFF\n");
		}

		return true;
	}
	else if (FStrEq(pcmd, "pathwpt"))
	{
		if ((FStrEq(arg1, "help")) || (FStrEq(arg1, "?")))
		{
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "\nAll pathwaypointing commands\n----------------------------------\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[commands] path creation commands help\n--------------------------------\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[on] shows paths\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[off] hides paths\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[show] toggles show/hide paths\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[save] saves all paths to the file\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[load] loads all paths from file\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[load <mapname>] loads all paths from 'mapname' file\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[count] prints info about total amounts of paths\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[info <index>] prints info about 'index' path or path on close waypoint\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[printall <arg>] prints info about all paths or paths on 'arg' waypoint or close wpt if arg is 'nearby'\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[printpath <index>] prints all waypoints and their types from 'index' path\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[checkinvalid <info>] will fix or remove all invalid paths eg. with length=1; if 'info' it will print more info\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[checkproblems <save>] prints problems with paths; if 'save' it will write all in MB error log file\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[compass <arg>] shows rapidly blinking beam that'll direct you to the beginning of 'arg' path\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[highlight <arg>] shows only path or paths matching the 'arg' filter; used without 'arg' will turn it off\n");
#ifdef _DEBUG
			//!!!!!!!!!!!!!!!!!!!!!!!!!
			//TEMP
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "\n!!!Following cmds are for dev purposes!!!\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[flags <index>] prints few info about actual or 'index' path\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[pap] prints basic info about all paths\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[pll <index>] prints all path wpts (whole llist)\n");

			//ClientPrint(pEntity, HUD_PRINTNOTIFY, "NOT DONE YET\n");
			//END TEMP
#endif
		}
		else if (FStrEq(arg1, "commands"))
		{
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "\nAll edit commands\n--------------------------------\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[start] starts path from close waypoint\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[stop] finishes the path that is currently worked on\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[continue <index>] marks 'index' path or path on close waypoint if no 'index' as the currently worked on path\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[add] adds close waypoint to currently worked on path\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[autoadd <on/off>] auto adds every waypoint that the player \"touches\"\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[insert <wpt#> <path#> <between1> <between2>] inserts waypoint into path between its two waypoints\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[remove <index>] removes close waypoint from currently worked on path or 'index' path\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[split <path#> <wpt#>] splits path on two parts on nearby or 'wpt' waypoint\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[delete <index>] deletes whole 'index' path or path on close waypoint\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[reset <index>] resets 'index' path or path on close waypoint back to default\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[setteam <team> <index>] sets team for 'index' path or path on close waypoint\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[setclass <class> <index>] sets class for 'index' path or path on close waypoint\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[setdirection <dir> <index>] sets direction for 'index' path or path on close waypoint\n");
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "[setmisc <misc> <index>] sets additional flags for 'index' path or path on close waypoint\n");
		}
		else if (FStrEq(arg1, "on"))
		{
			wptser.SetShowPaths(true);		// turn paths on
			wptser.SetShowWaypoints(true);	// we must turn waypoints on too

			ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoint paths are ON\n");
		}
		else if (FStrEq(arg1, "off"))
		{
			wptser.ResetShowPaths();

			ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoint paths are OFF\n");
		}
		else if (FStrEq(arg1, "show"))
		{
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "***toggle mode used***\n");

			if (wptser.IsShowPaths())
			{
				wptser.ResetShowPaths();	// turn paths off
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoint paths are OFF\n");
			}
			else
			{
				wptser.SetShowPaths(true);
				wptser.SetShowWaypoints(true);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoint paths are ON\n");
			}
		}
		else if (FStrEq(arg1, "save"))
		{
			if (WaypointPathSave(nullptr))
			{
				PlaySoundConfirmation(pEntity, SND_DONE);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "paths successfully saved\n");
			}
		}
		else if (FStrEq(arg1, "load"))
		{
			if ((arg2 != nullptr) && (*arg2 != 0))
			{
				char temp[64];

				strcpy(temp, arg2);

				if (WaypointPathLoad(pEntity, temp))
				{
					PlaySoundConfirmation(pEntity, SND_DONE);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "paths successfully loaded\n");
				}
			}
			else
			{
				if (WaypointPathLoad(pEntity, nullptr))
				{
					PlaySoundConfirmation(pEntity, SND_DONE);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "paths successfully loaded\n");
				}
			}
		}
		else if (FStrEq(arg1, "count"))
		{
			sprintf(msg, "paths already used= %d still could use= %d out of %d\n",
				num_w_paths, MAX_W_PATHS - num_w_paths, MAX_W_PATHS);
			ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
		}
		else if (FStrEq(arg1, "info"))
		{
			if ((arg2 != nullptr) && (*arg2 != 0))
			{
				int path_index = atoi(arg2);
				bool result = FALSE;

				// is it valid path index (+1/-1 due to array style)
				if ((path_index >= 1) && (path_index < num_w_paths + 1))
					result = WaypointPathInfo(pEntity, path_index - 1);

				// if that path doesn't exist print warning
				if (result == FALSE)
				{
					PlaySoundConfirmation(pEntity, SND_FAILED);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "that path doesn't exist!\n");
				}
			}
			else
				WaypointPathInfo(pEntity, -1);
		}
		else if (FStrEq(arg1, "printall"))
		{
			if ((arg2 != nullptr) && (*arg2 != 0))
			{
				bool result = FALSE;

				if (FStrEq(arg2, "nearby"))
				{
					if (!WaypointPrintAllPaths(pEntity, -10))
					{
						// print warning if there was an error
						PlaySoundConfirmation(pEntity, SND_FAILED);
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "not close enough to valid path waypoint!\n");
					}
				}
				else
				{
					int wpt_index = atoi(arg2);

					// is it valid waypoint index (+1/-1 due to array style)
					if ((wpt_index >= 1) && (wpt_index < num_waypoints + 1))
					{
						if (!WaypointPrintAllPaths(pEntity, wpt_index - 1))
						{
							PlaySoundConfirmation(pEntity, SND_FAILED);
							ClientPrint(pEntity, HUD_PRINTNOTIFY, "that waypoint doesn't exist or not a valid path waypoint!\n");
						}
					}
				}
			}
			else
				WaypointPrintAllPaths(pEntity, -1);
		}
		else if (FStrEq(arg1, "printpath"))
		{
			if ((arg2 != nullptr) && (*arg2 != 0))
			{
				int path_index = atoi(arg2) - 1;

				if ((path_index >= 0) && (path_index < num_w_paths) && WaypointPrintWholePath(pEntity, path_index))
				{
					PlaySoundConfirmation(pEntity, SND_DONE);
				}
				else
				{
					PlaySoundConfirmation(pEntity, SND_FAILED);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "that path doesn't exist!\n");
				}
			}
			else
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "missing argument!\n");
			}
		}
		else if (FStrEq(arg1, "checkinvalid"))
		{
			int result;

			if (FStrEq(arg2, "info"))
				result = UTIL_RemoveFalsePaths(TRUE);
			else
				result = UTIL_RemoveFalsePaths(FALSE);

			if (result == 0)
			{
				PlaySoundConfirmation(pEntity, SND_DONE);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "all paths are valid\n");
			}
			else if (result > 0)
			{
				int unfixed_paths = 0;													// NEW CODE 094

				while (result >= 600)													// NEW CODE 094
				{
					result -= 600;
					unfixed_paths++;
				}

				if (unfixed_paths == 0)
				{
					PlaySoundConfirmation(pEntity, SND_DONE);

					sprintf(msg, "total of %d invalid paths has been successfully fixed or completely removed\n",
						result);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
				}
				else
				{
					PlaySoundConfirmation(pEntity, SND_FAILED);

					sprintf(msg, "total of %d invalid paths has been successfully fixed or completely removed\n",
						result);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

					sprintf(msg, "there is total of %d invalid paths left in the waypoints that cannot be automatically fixed or removed\n",
						unfixed_paths);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
				}
			}
		}
		else if ((FStrEq(arg1, "checkproblems")) || (FStrEq(arg1, "checkforproblems")))
		{
			int result;

			if (FStrEq(arg2, "save"))
				result = UTIL_CheckPathsForProblems(TRUE);
			else
				result = UTIL_CheckPathsForProblems(FALSE);

			if (result == 0)
			{
				PlaySoundConfirmation(pEntity, SND_DONE);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "all paths successfully passed\n");
			}
			else
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);

				// The counter isn't accurate. It counts the paths, NOT the number of problems.
				// Until there's a correct counter it's better to drop this output.
				// Also we are checking even the solitary waypoints now so the output would have to be rephrased anyway.

				//sprintf(msg, "total of %d bugs/possible problems with paths have been found\n", result);
				//ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

				if (result >= 10)
				{
					sprintf(msg, "There may be more errors in the waypoints. Fix the listed ones and repeat the command to check for more.\n");
					ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
				}

				sprintf(msg, "***TIP: use 'pathwpt highlight' or 'pathwpt compass' or 'wpt compass' to locate the problem paths or waypoints***\n");
				ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
			}
		}
		else if (FStrEq(arg1, "compass"))
		{
			if ((arg2 != nullptr) && (*arg2 != 0))
			{
				if (FStrEq(arg2, "off"))
				{
					wptser.ResetCompassIndex();

					PlaySoundConfirmation(pEntity, SND_DONE);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "compass was turned off\n");
				}
				else
				{
					// convert it to array based index
					int path_index = atoi(arg2) - 1;

					if ((path_index >= 0) && (path_index < num_w_paths))
					{
						if (w_paths[path_index] != nullptr)
						{
							char index[5];

							// convert path first waypoint index to string
							sprintf(index, "%d", w_paths[path_index]->wpt_index + 1);

							if (WaypointCompass(pEntity, (const char*)index))
								PlaySoundConfirmation(pEntity, SND_DONE);
							else
								wptser.ResetCompassIndex();
						}
						else
						{
							// turn the compass off
							wptser.ResetCompassIndex();

							PlaySoundConfirmation(pEntity, SND_FAILED);
							ClientPrint(pEntity, HUD_PRINTNOTIFY, "that path doesn't exist (probably was deleted)!\n");
						}

					}
					else
					{
						wptser.ResetCompassIndex();

						PlaySoundConfirmation(pEntity, SND_FAILED);
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "that path doesn't exist!\n");
					}
				}
			}
			else
			{
				wptser.ResetCompassIndex();

				PlaySoundConfirmation(pEntity, SND_FAILED);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid or missing argument!\n");
			}
		}
		else if (FStrEq(arg1, "highlight"))
		{
			if ((arg2 != nullptr) && (*arg2 != 0))
			{
				int path;

				if (FStrEq(arg2, "red"))
					path = HIGHLIGHT_RED;
				else if (FStrEq(arg2, "blue"))
					path = HIGHLIGHT_BLUE;
				else if ((FStrEq(arg2, "oneway")) || (FStrEq(arg2, "one-way")) || (FStrEq(arg2, "one_way")))
					path = HIGHLIGHT_ONEWAY;
				else if ((FStrEq(arg2, "sniper")) || (FStrEq(arg2, "sniperonly")) || (FStrEq(arg2, "sniper-only")))
					path = HIGHLIGHT_SNIPER;
				else if ((FStrEq(arg2, "mgunner")) || (FStrEq(arg2, "mgunneronly")) || (FStrEq(arg2, "mgunner-only")))
					path = HIGHLIGHT_MGUNNER;
				else
					path = atoi(arg2) - 1;

				if ((path == HIGHLIGHT_RED) || (path == HIGHLIGHT_BLUE) || (path == HIGHLIGHT_ONEWAY) ||
					(path == HIGHLIGHT_SNIPER) || (path == HIGHLIGHT_MGUNNER) || ((path >= 0) && (path < MAX_W_PATHS)))
				{
					if (path == HIGHLIGHT_RED)
					{
						wptser.SetPathToHighlight(path);
						wptser.SetShowWaypoints(true);			// force displaying waypoints
						wptser.SetShowPaths(true);				// and paths too

						PlaySoundConfirmation(pEntity, SND_DONE);
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "path highlighting is ON!\nall red team possible paths will be drawn\n");
					}
					else if (path == HIGHLIGHT_BLUE)
					{
						wptser.SetPathToHighlight(path);
						wptser.SetShowWaypoints(true);
						wptser.SetShowPaths(true);

						PlaySoundConfirmation(pEntity, SND_DONE);
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "path highlighting is ON!\nall blue team possible paths will be drawn\n");
					}
					else if (path == HIGHLIGHT_ONEWAY)
					{
						wptser.SetPathToHighlight(path);
						wptser.SetShowWaypoints(true);
						wptser.SetShowPaths(true);

						PlaySoundConfirmation(pEntity, SND_DONE);
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "path highlighting is ON!\nall one-way paths will be drawn\n");
					}
					else if (path == HIGHLIGHT_SNIPER)
					{
						wptser.SetPathToHighlight(path);
						wptser.SetShowWaypoints(true);
						wptser.SetShowPaths(true);

						PlaySoundConfirmation(pEntity, SND_DONE);
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "path highlighting is ON!\nall sniper paths will be drawn\n");
					}
					else if (path == HIGHLIGHT_MGUNNER)
					{
						wptser.SetPathToHighlight(path);
						wptser.SetShowWaypoints(true);
						wptser.SetShowPaths(true);

						PlaySoundConfirmation(pEntity, SND_DONE);
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "path highlighting is ON!\nall machine gunner paths will be drawn\n");
					}
					else if (w_paths[path] != nullptr)
					{
						wptser.SetPathToHighlight(path);
						wptser.SetShowWaypoints(true);
						wptser.SetShowPaths(true);

						PlaySoundConfirmation(pEntity, SND_DONE);
						sprintf(msg, "path highlighting is ON!\npath no. %d is now the only drawn path\n", path + 1);
						ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
					}
					else
					{
						PlaySoundConfirmation(pEntity, SND_FAILED);
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "that path doesn't exist (probably was deleted)!\n");
					}
				}
				else
				{
					PlaySoundConfirmation(pEntity, SND_FAILED);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid argument!\n");
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "valid arguments are: red, blue, oneway, sniper, mgunner and index (where index is path number ie. a number between 1 and 512)\n");
				}
			}
			else
			{
				// turn it off
				wptser.ResetPathToHighlight();

				PlaySoundConfirmation(pEntity, SND_DONE);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "path highlighting is OFF!\n");
			}
		}
		else if (FStrEq(arg1, "start"))
		{
			// turn paths on
			wptser.SetShowPaths(true);
			wptser.SetShowWaypoints(true);  // and waypoints must be displayed too

			if (WaypointCreatePath(pEntity))
			{
				PlaySoundConfirmation(pEntity, SND_DONE);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "new path successfully started\n");
			}
			else
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "path cannot be started or not close enough to any waypoint!\n");
			}
		}
		else if (FStrEq(arg1, "stop"))
		{
			if (WaypointFinishPath(pEntity))
			{
				PlaySoundConfirmation(pEntity, SND_DONE);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "current path was finished\n");
			}
			else
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "can't finish current path!\n");
			}
		}
		else if (FStrEq(arg1, "continue"))
		{
			bool result = FALSE;

			if ((arg2 != nullptr) && (*arg2 != 0))
			{
				int path_index = atoi(arg2);

				// is valid path_index (+1/-1 due to array style)
				if ((path_index >= 1) && (path_index < num_w_paths + 1))
					result = WaypointPathContinue(pEntity, path_index - 1);
			}
			else
				result = WaypointPathContinue(pEntity, -1);

			if (result)
			{
				PlaySoundConfirmation(pEntity, SND_DONE);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "path is actual and ready to continue in\n");
			}
			else
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "that path doesn't exist or not close enough to any waypoint!\n");
			}
		}
		else if (FStrEq(arg1, "add"))
		{
			if (WaypointAddIntoPath(pEntity))
			{
				PlaySoundConfirmation(pEntity, SND_DONE);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoint added to actual path\n");
			}
			else
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "that path doesn't exist or not close enough to allowed waypoint or it already is in this path!\n");
			}
		}
		else if (FStrEq(arg1, "autoadd"))
		{
			// check if exist arg2 and is valid
			if (FStrEq(arg2, "on"))
				wptser.SetAutoAddToPath(true);
			else if (FStrEq(arg2, "off"))
				wptser.ResetAutoAddToPath();
			else
			{
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "***toggle mode used***\n");

				if (wptser.IsAutoAddToPath())
					wptser.ResetAutoAddToPath();	// turn auto adding off
				else
					wptser.SetAutoAddToPath(true);
			}

			// print proper msg
			if (wptser.IsAutoAddToPath())
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "autoadd to path is ON\n");
			else
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "autoadd to path is OFF\n");
		}
		else if (FStrEq(arg1, "insert"))
		{
			if ((arg2 == nullptr) || (*arg2 == 0))
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "missing arguments check for MB help!\n");
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "***TIP: use 'pathwpt insert help' for help!***\n");

				return true;
			}
			else if ((FStrEq(arg2, "help")) || (FStrEq(arg2, "?")))
			{
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "Insert command usage:\n---------------------------\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "'pathwpt insert <wpt_index> <path_index> <arg3> <arg4>\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "---------------------------\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "<wpt_index> is the unique no. of the waypoint you want to insert into existing path\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "<path_index> is the unique no. of existing path\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "<arg3> can be unique no. of a waypoint that already is in the path\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "<arg3> can be a word 'start' when the waypoint is going to be inserted to the beginning of the path\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "<arg3> can be a word 'end' when the waypoint is going to be inserted to the end of the path\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "<arg4> is unique no. of a waypoint that already is in the path and is a direct neighbour of arg3 waypoint\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "\nFew examples:\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "'pathwpt insert 123 50 start' will insert new waypoint no. 123 to the beginning of path no. 50\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "'pathwpt insert 123 50 end' will insert new waypoint no. 123 to the end of path no. 50\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "'pathwpt insert 123 50 400 660' will insert new waypoint no. 123 into path no. 50 between its wpts no. 400 and 660\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "'pathwpt insert 123 50 660 400' will do exactly the same thing as the command from above\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "path wpts no. 400 & 660 must be direct neighbours in the path (eg. pathwpt <-> pathwpt <-> 400 <-> 660 <-> pathwpt)\n");

				return true;
			}

			int result = WaypointInsertIntoPath(arg2, arg3, arg4, arg5);

			if (result == -6)
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				sprintf(msg, "can't insert a waypoint that already is in this path!\n");
			}
			else if (result == -5)
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				sprintf(msg, "both path waypoints are same, unknown position for insertion!\n");
			}
			else if (result == -4)
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				sprintf(msg, "invalid or missing waypoint for insertion!\n");
			}
			else if (result == -3)
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				sprintf(msg, "invalid or missing path!\n");
			}
			else if (result == -2)
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				sprintf(msg, "invalid or missing path waypoint1 index or this waypoint isn't in this path!\n");
			}
			else if (result == -1)
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				sprintf(msg, "invalid or missing path waypoint2 index or this waypoint isn't in this path!\n");
			}
			else if (result == 0)
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				sprintf(msg, "waypoint can't be inserted pathwaypoints aren't neighbours!\n");
			}
			else if (result == 1)
			{
				PlaySoundConfirmation(pEntity, SND_DONE);
				sprintf(msg, "waypoint successfully inserted\n");
			}
			else
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				sprintf(msg, "unknown error!\n");
			}

			ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

			return true;
		}
		else if (FStrEq(arg1, "remove"))
		{
			bool result = FALSE;

			if ((arg2 != nullptr) && (*arg2 != 0))
			{
				int path_index = atoi(arg2);

				// is it valid path_index (+1/-1 due to array style)
				if ((path_index >= 1) && (path_index < num_w_paths + 1))
					result = WaypointRemoveFromPath(pEntity, path_index - 1);
			}
			else
				result = WaypointRemoveFromPath(pEntity, -1);

			if (result)
			{
				PlaySoundConfirmation(pEntity, SND_DONE);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoint successfully removed from path\n");
			}
			else
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "that path doesn't exist or not close enough to any waypoint!\n");
			}
		}
		else if (FStrEq(arg1, "split"))
		{
			if ((FStrEq(arg2, "help")) || (FStrEq(arg2, "?")))
			{
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "Split command usage:\n---------------------------\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "'pathwpt split <path_index> <wpt_index>'\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "---------------------------\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "<path_index> is unique number of path you want to divide into two parts\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "<wpt_index> is the waypoint at which will the path be divided (ie. the future end point of this path)\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "Note: You can't split path which length is lesser than 5 (ie. path with less than 5 waypoints).\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "It's due to the fact that each valid path must be at least 2 waypoints long.\n");
			}
			else
			{
				int result = WaypointSplitPath(pEntity, arg2, arg3);

				if (result == -1)
				{
					PlaySoundConfirmation(pEntity, SND_FAILED);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "not close enough to any waypoint or there's no path on that waypoint!\n");
				}
				else if (result == -2)
				{
					PlaySoundConfirmation(pEntity, SND_FAILED);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "the path is too short for splitting (ie. the path has less than 5 waypoints)!\n");
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "There must stay at least 2 waypoints on either part of the path after the split.\n");
				}
				else if (result == -3)
				{
					PlaySoundConfirmation(pEntity, SND_FAILED);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "that waypoint isn't in this path!\n");
				}
				else if (result == -4)
				{
					PlaySoundConfirmation(pEntity, SND_FAILED);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "can't split the path at this position!\n");
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "There must stay at least 2 waypoints on either part of the path after the split.\n");
				}
				else if (result == -5)
				{
					PlaySoundConfirmation(pEntity, SND_FAILED);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "unable to split the path, because max. amount of paths has been reached!\n");
				}
				else if (result == -6)
				{
					PlaySoundConfirmation(pEntity, SND_FAILED);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "one or both parts of the path isn't valid\n");
				}
				else if (result == -7)
				{
					PlaySoundConfirmation(pEntity, SND_FAILED);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "such path doesn't exist or was deleted!\n");
				}
				else if (result == -8)
				{
					PlaySoundConfirmation(pEntity, SND_FAILED);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "such waypoint doesn't exist or was deleted!\n");
				}
				else if ((result >= 0) && (result < MAX_W_PATHS))
				{
					PlaySoundConfirmation(pEntity, SND_DONE);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "path successfully splitted\n");

					char details[128];
					sprintf(details, "the other part of the path is now a path no.%d\n", result + 1);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, details);
				}
			}
		}
		else if (FStrEq(arg1, "delete"))
		{
			bool result = FALSE;

			if ((arg2 != nullptr) && (*arg2 != 0))
			{
				int path_index = atoi(arg2);

				// is it valid path_index (+1/-1 due to array style)
				if ((path_index >= 1) && (path_index < num_w_paths + 1))
					result = WaypointDeletePath(pEntity, path_index - 1);
			}
			else
				result = WaypointDeletePath(pEntity, -1);

			if (result)
			{
				PlaySoundConfirmation(pEntity, SND_DONE);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "path successfully deleted\n");
			}
			else
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "that path doesn't exist or not close enough to its waypoint!\n");
			}
		}
		else if (FStrEq(arg1, "reset"))
		{
			bool result = FALSE;

			if ((arg2 != nullptr) && (*arg2 != 0))
			{
				int path_index = atoi(arg2);

				// is it valid path index (+1/-1 due to array style)
				if ((path_index >= 1) && (path_index < num_w_paths + 1))
					result = WaypointResetPath(pEntity, path_index - 1);
			}
			else
				result = WaypointResetPath(pEntity, -1);

			// print msg based on result value
			if (result)
			{
				PlaySoundConfirmation(pEntity, SND_DONE);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "path successfully reseted\n");
			}
			else
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "that path doesn't exist or not close enough!\n");
			}
		}
		else if (FStrEq(arg1, "setteam"))
		{
			// set the initial value to something that cannot happen
			int result = -128;

			if ((arg3 != nullptr) && (*arg3 != 0))
			{
				int path_index = atoi(arg3);

				// is it valid path index (+1 due to array style)
				if ((path_index >= 1) && (path_index < num_w_paths + 1))
					result = WptPathChangeTeam(pEntity, arg2, path_index - 1);
			}
			else if ((FStrEq(arg2, "help")) || (FStrEq(arg2, "?")))
			{
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "All valid team options\n---------------------------\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "<both> both teams will use it - DEFAULT\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "<red> only red team bots will use it\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "<blue> only blue team bots will use it\n");
			}
			else
				result = WptPathChangeTeam(pEntity, arg2, -1);

			// print msg based on result value
			if (result == -2)
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "that path doesn't exist or not close enough!\n");
			}
			else if (result == -1)
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid or missing argument!\n");
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "***TIP: use 'setteam help' for help!***\n");
			}
			else if (result == 1)
			{
				PlaySoundConfirmation(pEntity, SND_DONE);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "path team flag successfully changed\n");
			}
		}
		else if (FStrEq(arg1, "setclass"))
		{
			int result = -128;

			if ((arg3 != nullptr) && (*arg3 != 0))
			{
				int path_index = atoi(arg3);

				// is it valid path index (+1 due to array style)
				if ((path_index >= 1) && (path_index < num_w_paths + 1))
					result = WptPathChangeClass(pEntity, arg2, path_index - 1);
			}
			else if ((FStrEq(arg2, "help")) || (FStrEq(arg2, "?")))
			{
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "All valid class options\n-----------------------------\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "<all> all bots will use it - DEFAULT\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "<sniper> only snipers will use it\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "<mgunner> only mgunners will use it\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "Note: 'sniper' and 'mgunner' tags can be together on one specific path. Both classes will then be able to use such path.\n");
			}
			else
				result = WptPathChangeClass(pEntity, arg2, -1);

			// print msg based on result value
			if (result == -2)
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "that path doesn't exist or not close enough!\n");
			}
			else if (result == -1)
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid or missing argument!\n");
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "***TIP: use 'setclass help' for help!***\n");
			}
			else if (result == 1)
			{
				PlaySoundConfirmation(pEntity, SND_DONE);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "path class flag successfully changed\n");
			}
		}
		else if (FStrEq(arg1, "setdirection"))
		{
			int result = -128;

			if ((arg3 != nullptr) && (*arg3 != 0))
			{
				int path_index = atoi(arg3);

				// is it valid path index (+1 due to array style)
				if ((path_index >= 1) && (path_index < num_w_paths + 1))
					result = WptPathChangeWay(pEntity, arg2, path_index - 1);
			}
			else if ((FStrEq(arg2, "help")) || (FStrEq(arg2, "?")))
			{
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "All valid way/direction options\n----------------------------------------\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "<one> only one way path (start-end)\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "<two> two way path (start-end as well as end-start) - DEFAULT\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "<patrol> patrol type path (cycle start-end-start)\n");
			}
			else
				result = WptPathChangeWay(pEntity, arg2, -1);

			// print msg based on result value
			if (result == -2)
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "that path doesn't exist or not close enough!\n");
			}
			else if (result == -1)
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid or missing argument!\n");
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "***TIP: use 'setdirection help' for help!***\n");
			}
			else if (result == 1)
			{
				PlaySoundConfirmation(pEntity, SND_DONE);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "path direction flag successfully changed\n");
			}
		}
		else if (FStrEq(arg1, "setmisc"))
		{
			int result = -128;

			if ((arg3 != nullptr) && (*arg3 != 0))
			{
				int path_index = atoi(arg3);

				// is it valid path index (+1 due to array style)
				if ((path_index >= 1) && (path_index < num_w_paths + 1))
					result = WptPathChangeMisc(pEntity, arg2, path_index - 1);
			}
			else if ((FStrEq(arg2, "help")) || (FStrEq(arg2, "?")))
			{
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "All valid miscellaneous/additional options\n-----------------------------\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "<avoid_enemy> bot won't attack distant enemies\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "<ignore_enemy> bot won't attack enemies unless one is right next to him\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "<carry_item> bot will search for these paths when carrying a goal item\n");
			}
			else
				result = WptPathChangeMisc(pEntity, arg2, -1);

			// print msg based on result value
			if (result == -2)
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "that path doesn't exist or not close enough!\n");
			}
			else if (result == -1)
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid or missing argument!\n");
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "***TIP: use 'setmisc help' for help!***\n");
			}
			else if (result == 1)
			{
				PlaySoundConfirmation(pEntity, SND_DONE);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "miscellaneous/additional path flag successfully changed\n");
			}
		}


#ifdef _DEBUG
		// ONLY FOR TESTS
		else if (FStrEq(arg1, "movepathz"))
		{
			if ((arg2 != NULL) && (*arg2 != 0))
			{
				int path_index = atoi(arg2) - 1;

				if ((path_index >= 0) && (path_index < num_w_paths - 1))
				{
					float value = 0.0;

					if ((arg3 != NULL) && (*arg3 != 0))
					{
						value = atof(arg3);

						if (WaypointMoveWholePath(path_index, value, 3))
						{
							PlaySoundConfirmation(pEntity, SND_DONE);
							ClientPrint(pEntity, HUD_PRINTNOTIFY, "whole path has been repositioned\n");
						}
						else
						{
							PlaySoundConfirmation(pEntity, SND_FAILED);
							ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid argument!\n");
						}
					}
				}
			}
		}

		else if (FStrEq(arg1, "flags"))
		{
			extern int WaypointGetPathLength(int path_index);
			W_PATH* p;
			char* team_fl, * class_fl, * way_fl;
			char misc[128];
			int path_index = -1;

			if ((arg2 != NULL) && (*arg2 != 0))
			{
				int this_path_index = atoi(arg2);

				if ((this_path_index >= 1) && (this_path_index < num_w_paths + 1))
				{
					this_path_index--;//due array style
					path_index = this_path_index;
				}
			}
			else
				path_index = internals.GetPathToContinue();

			if (w_paths[path_index] == NULL)
				sprintf(msg, "path no. %d | doesn't exist (deleted)!\n", path_index + 1);
			else
			{
				p = w_paths[path_index];

				if (p->flags & P_FL_TEAM_NO)
					team_fl = "both";
				else if (p->flags & P_FL_TEAM_RED)
					team_fl = "red";
				else if (p->flags & P_FL_TEAM_BLUE)
					team_fl = "blue";
				else
					team_fl = "BUG";

				if (p->flags & P_FL_CLASS_ALL)
					class_fl = "all";
				else if (p->flags & P_FL_CLASS_SNIPER)
					class_fl = "sniper";
				else if (p->flags & P_FL_CLASS_MGUNNER)
					class_fl = "mgunner";
				else
					class_fl = "BUG";

				if (p->flags & P_FL_WAY_ONE)
					way_fl = "one";
				else if (p->flags & P_FL_WAY_TWO)
					way_fl = "two";
				else if (p->flags & P_FL_WAY_PATROL)
					way_fl = "patrol";
				else
					way_fl = "BUG";

				strcpy(misc, "NOTHING");

				if (p->flags & P_FL_MISC_AMMO)
				{
					strcpy(misc, " ammo ");
				}
				if (p->flags & P_FL_MISC_GOAL_RED)
				{
					if (strcmp(misc, "NOTHING") == 0)
						strcpy(misc, " RedGoal ");
					else
						strcat(misc, "& RedGoal ");
				}
				if (p->flags & P_FL_MISC_GOAL_BLUE)
				{
					if (strcmp(misc, "NOTHING") == 0)
						strcpy(misc, " BlueGoal ");
					else
						strcat(misc, "& BlueGoal ");
				}
				if (p->flags & P_FL_MISC_AVOID)
				{
					if (strcmp(misc, "NOTHING") == 0)
						strcpy(misc, " AvoidEnemy ");
					else
						strcat(misc, "& AvoidEnemy ");
				}
				if (p->flags & P_FL_MISC_IGNORE)
				{
					if (strcmp(misc, "NOTHING") == 0)
						strcpy(misc, " IgnoreEnemy ");
					else
						strcat(misc, "& IgnoreEnemy ");
				}
				if (p->flags & P_FL_MISC_GITEM)
				{
					if (strcmp(misc, "NOTHING") == 0)
						strcpy(misc, " CarryItem ");
					else
						strcat(misc, "& CarryItem ");
				}

				sprintf(msg, "path no. %d | length:%d | start_wpt:%d | team:%s | class:%s | dir:%s | misc:%s || flags (%d)\n",
					path_index + 1, WaypointGetPathLength(path_index), p->wpt_index + 1,
					team_fl, class_fl, way_fl, misc, p->flags);
			}

			ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
		}
		else if (FStrEq(arg1, "pll"))
		{
			int path_index = -1;

			if ((arg2 != NULL) && (*arg2 != 0))
			{
				int this_path_index = atoi(arg2);

				if ((this_path_index >= 1) && (this_path_index < num_w_paths + 1))
				{
					this_path_index--;//due array style
					path_index = this_path_index;
				}
			}
			else
				path_index = internals.GetPathToContinue();

			if (path_index == -1 || w_paths[path_index] == NULL)
				return true;

			sprintf(msg, "processing path no. %d\n", path_index + 1);
			ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

			int safety = 0;

			W_PATH* p = w_paths[path_index];//head node

			while (p)
			{
				int cur = -2;
				int prev = -2;
				int next = -2;

				if (safety > 1000)
				{
					ALERT(at_error, "\"ppl\" | LLError\n");

					break;
				}

				cur = p->wpt_index;

				if (p->prev)//is prev node
					prev = p->prev->wpt_index;//so get its wpt index
				if (p->next)//is next node
					next = p->next->wpt_index;//so get its wpt index

				sprintf(msg, "cur_wpt_i=%d | prev_wpt_i=%d | next_wpt_i=%d\n",
					cur + 1, prev + 1, next + 1);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

				p = p->next;// go to next node

				safety++;
			}
		}
#endif //_DEBUG

		else
		{
			PlaySoundConfirmation(pEntity, SND_FAILED);
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid or missing argument!\n");
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "***TIP: use 'pathwpt help' for help***\n");

			if (wptser.IsShowPaths())
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoint paths are ON\n");
			else
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoint paths are OFF\n");
		}

		return true;
	}
	else if (FStrEq(pcmd, "vguimenuoption") && (g_menu_state != MENU_NONE))
	{
		// whole marinebot menu (the hud menu, not supported in FA2.8 and above)
		MBMenuSystem(pEntity, arg1);
	}
	else
	{
#ifdef _DEBUG
		// nearly all debugging commands
		if (CheckForSomeDebuggingCommands(pEntity, pcmd, arg1, arg2, arg3, arg4, arg5))
			return true;
#endif
	}

	return false;
}

#ifdef _DEBUG
inline bool CheckForSomeDebuggingCommands(edict_t* pEntity, const char* pcmd, const char* arg1, const char* arg2, const char* arg3, const char* arg4, const char* arg5)
{
	char msg[256];

	if (FStrEq(pcmd, "toggledev"))
	{
		float dev_state = CVAR_GET_FLOAT("developer");

		if (dev_state == 0)
			CVAR_SET_FLOAT("developer", 1);
		else
			CVAR_SET_FLOAT("developer", 0);

		ClientPrint(pEntity, HUD_PRINTNOTIFY, "*** developer mode changed ***\n");

		return TRUE;
	}
	else if (FStrEq(pcmd, "cleardf") || FStrEq(pcmd, "destroydf") || FStrEq(pcmd, "erasedebugfile"))
	{
		char filename[256];

		UTIL_MarineBotFileName(filename, PUBLIC_DEBUG_FILE, NULL);
		remove(filename);

		return TRUE;
	}
	else if (FStrEq(pcmd, "settest") || FStrEq(pcmd, "testsetting") || FStrEq(pcmd, "testsettings"))
	{
		CVAR_SET_FLOAT("developer", 1);
		botmanager.SetOverrideTeamsBalance(true);
		internals.SetIsCustomWaypoints(true);
		botdebugger.SetDebugActions(true);
		botdebugger.SetDebugStuck(true);
		externals.SetSpawnSkill(1);
		//botdebugger.SetDebugPaths(true);
		//botdebugger.SetDebugWaypoints(true);

		ClientPrint(pEntity, HUD_PRINTCONSOLE, "test setting ENABLED!\n");
		return TRUE;
	}
	else if (FStrEq(pcmd, "modif"))
	{
		extern float modif;

		if ((arg1 != NULL) && (*arg1 != 0))
		{
			int temp = atoi(arg1);

			modif = (float)temp;
			modif /= 10.0;
		}

		return TRUE;
	}
	else if (FStrEq(pcmd, "nmodif"))
	{
		extern float modif;

		if ((arg1 != NULL) && (*arg1 != 0))
		{
			int temp = atoi(arg1);

			modif = (float)temp;
			modif /= -10.0;
		}

		return TRUE;
	}




	// allows us to move with all waypoints to desired direction

	else if (FStrEq(pcmd, "movewptsbyx"))
	{
		if ((arg1 != NULL) && (*arg1 != 0))
		{
			extern void ShiftWpts(int x, int y, int z);

			int temp = atoi(arg1);
			ShiftWpts(temp, 0, 0);
		}
		return TRUE;
	}
	// negative shift
	else if (FStrEq(pcmd, "movewptsbynx"))
	{
		if ((arg1 != NULL) && (*arg1 != 0))
		{
			extern void ShiftWpts(int x, int y, int z);

			int temp = atoi(arg1);
			ShiftWpts(-temp, 0, 0);
		}
		return TRUE;
	}
	else if (FStrEq(pcmd, "movewptsbyy"))
	{
		if ((arg1 != NULL) && (*arg1 != 0))
		{
			extern void ShiftWpts(int x, int y, int z);

			int temp = atoi(arg1);
			ShiftWpts(0, temp, 0);
		}
		return TRUE;
	}
	else if (FStrEq(pcmd, "movewptsbyny"))
	{
		if ((arg1 != NULL) && (*arg1 != 0))
		{
			extern void ShiftWpts(int x, int y, int z);

			int temp = atoi(arg1);
			ShiftWpts(0, -temp, 0);
		}
		return TRUE;
	}
	else if (FStrEq(pcmd, "movewptsbyz"))
	{
		if ((arg1 != NULL) && (*arg1 != 0))
		{
			extern void ShiftWpts(int x, int y, int z);

			int temp = atoi(arg1);
			ShiftWpts(0, 0, temp);
		}
		return TRUE;
	}
	else if (FStrEq(pcmd, "movewptsbynz"))
	{
		if ((arg1 != NULL) && (*arg1 != 0))
		{
			extern void ShiftWpts(int x, int y, int z);

			int temp = atoi(arg1);
			ShiftWpts(0, 0, -temp);
		}
		return TRUE;
	}

	else if ((strcmp(pcmd, "debugbot") == 0) || (strcmp(pcmd, "debug_bot") == 0))
	{
		if ((arg1 == NULL) || (*arg1 == 0))
		{
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "missing or invalid argument!\n");

			return TRUE;
		}
		else if (FStrEq(arg1, "off"))
		{
			g_debug_bot = NULL;
			g_debug_bot_on = false;
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "debugging of certain bot has been DISABLED!\n");

			return TRUE;
		}
		int i = UTIL_FindBotByName(arg1);

		if (i != -1)
		{
			g_debug_bot = bots[i].pEdict;
			g_debug_bot_on = true;
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "debugging of this bot is ENABLED!\n");
		}
		else
		{
			g_debug_bot = NULL;
			g_debug_bot_on = false;
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "debugging of certain bot has been DISABLED!\n");
		}

		return TRUE;
	}

	else if ((strcmp(pcmd, "debugengine") == 0) || (strcmp(pcmd, "debug_engine") == 0))
	{
		if (debug_engine)
		{
			debug_engine = 0;
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_engine DISABLED!\n");
		}
		else
		{
			debug_engine = 1;
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_engine ENBLED!\n");
		}

		return TRUE;
	}

	else if ((strcmp(pcmd, "testbalance") == 0) || (strcmp(pcmd, "testteams") == 0))
	{
		int i, actual_pl, bot_count, reds, blues, diff;
		char msg[512];

		actual_pl = bot_count = reds = blues = 0;
		UTIL_DebugInFile("Testing team balance --- on request\n");

		for (i = 0; i < MAX_CLIENTS; i++)
		{
			if (clients[i].pEntity == NULL)
				continue;
			else
			{
				++actual_pl;

				if (clients[i].pEntity->v.flags & FL_FAKECLIENT)
					++bot_count;

				if (UTIL_GetTeam(clients[i].pEntity) == TEAM_RED)
					reds++;
				if (UTIL_GetTeam(clients[i].pEntity) == TEAM_BLUE)
					blues++;

				sprintf(msg, "%s is in team: %d\n", STRING(clients[i].pEntity->v.netname),
					UTIL_GetTeam(clients[i].pEntity));
				UTIL_DebugInFile(msg);
			}
		}

		diff = reds - blues;


		sprintf(msg, "<dll.cpp | Testing team balance> - clients %d | bots %d | reds %d | blues %d | diff %d\n",
			actual_pl, bot_count, reds, blues, diff);
		UTIL_DebugInFile(msg);

	}
	else if ((strcmp(pcmd, "test_dumpedict") == 0) || (strcmp(pcmd, "testdumpedict") == 0) ||
		(strcmp(pcmd, "test_dumpclient") == 0) || (strcmp(pcmd, "testdumpclient") == 0))
	{
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (clients[i].pEntity)
				UTIL_DumpEdictToFile(clients[i].pEntity);
		}
	}

	// @@@@@@@@@@@ just for test things
	else if ((strcmp(pcmd, "test_clientdetection") == 0) || (strcmp(pcmd, "testclientdetection") == 0))
	{
		int cl_count, bot_count, human_count;
		cl_count = bot_count = human_count = 0;

		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (clients[i].pEntity != NULL)
			{
				cl_count++;

				if (clients[i].pEntity->v.flags & FL_FAKECLIENT)
					bot_count++;
				else
					human_count++;
			}
		}

		sprintf(msg, "total clients on server - (by client array: %d) (counted now: %d)\n",
			clients[0].ClientCount(), cl_count);
		PrintOutput(NULL, msg, MType::msg_null);

		sprintf(msg, "number of bots - (by cl array: %d) (counted now: %d)\n",
			clients[0].BotCount(), bot_count);
		PrintOutput(NULL, msg, MType::msg_null);

		sprintf(msg, "number of humans: (by cl array: %d) (counted now: %d)\n",
			clients[0].HumanCount(), human_count);
		PrintOutput(NULL, msg, MType::msg_null);
	}




	//TEMP: need cmd to change the default max effective range modifier for all weapons
	else if (FStrEq(pcmd, "trng"))
	{
		extern float rg_modif;

		if ((arg1 != NULL) && (*arg1 != 0))
		{
			int temp = atoi(arg1);

			rg_modif = (float)temp;
			rg_modif /= 100;

			ALERT(at_console, "range modifier changed - it's %.2f of original range now, reinit all weapons...\n", rg_modif);

			BotWeaponArraysInit(conf_weapons);
		}

		return TRUE;
	}
	//TEMP: need cmd to test new name validation util
	else if (FStrEq(pcmd, "debug_name"))
	{
		int used = 0;
		int free = 0;

		for (int i = 0; i < MAX_BOT_NAMES; i++)
		{
			if (bot_names[i].is_used)
				used++;
			else
				free++;
		}

		ALERT(at_console, "USED=%d | FREE=%d | TOTAL=%d\n", used, free, used + free);

		return TRUE;
	}

	

	//TEMP: need cmd to test in combat weapon manipulation
	else if (FStrEq(pcmd, "tbco1"))
	{
		extern bool in_bot_dev_level1;

		if (in_bot_dev_level1)
		{
			in_bot_dev_level1 = FALSE;
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "in bot dev level 1 turned off\n");
		}
		else
		{
			in_bot_dev_level1 = TRUE;
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "in bot dev level 1 turned on\n");
		}

		return TRUE;
	}

	//TEMP: need cmd to test in combat weapon manipulation
	else if (FStrEq(pcmd, "tbco2"))
	{
		extern bool in_bot_dev_level2;

		if (in_bot_dev_level2)
		{
			in_bot_dev_level2 = FALSE;
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "in bot dev level 2 turned off\n");
		}
		else
		{
			in_bot_dev_level2 = TRUE;
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "in bot dev level 2 turned on\n");
		}

		return TRUE;
	}

	//TEMP: need cmd to test getting effective range of bot's weapon
	else if (FStrEq(pcmd, "tbger"))
	{
		if ((arg1 == NULL) || (*arg1 == 0))
		{
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "missing or invalid argument!\n");
			return TRUE;
		}

		int index = UTIL_FindBotByName(arg1);

		if (index != -1)
		{
			sprintf(msg, "The effective range of bot's main weapon is %.2f\n", bots[index].GetEffectiveRange());
			ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
		}
		else
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "no bot with that name!\n");

		return TRUE;
	}

	//TEMP: need cmd to test combat movements
	else if (FStrEq(pcmd, "tbc"))
	{
		if ((arg1 == NULL) || (*arg1 == 0))
		{
			for (int i = 0; i < MAX_CLIENTS; i++)
			{
				if (bots[i].is_used)
				{
					sprintf(msg, "<%s>advT %.1f | OverrideAdvT %.1f | ChckStanceT %.1f | reloadT %.1f | globtime %.1f\n",
						bots[i].name, bots[i].f_combat_advance_time, bots[i].f_overide_advance_time,
						bots[i].f_check_stance_time, bots[i].f_reload_time, gpGlobals->time);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

					sprintf(msg, "snpT %.1f | waitT %.1f | dntmove %d | wait_f_enem %.1f\n",
						bots[i].sniping_time, bots[i].wpt_wait_time,
						bots[i].IsTask(TASK_DONTMOVEINCOMBAT), bots[i].f_bot_wait_for_enemy_time);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

					if (bots[i].bot_behaviour & BOT_PRONED)
						sprintf(msg, "Proned\n");
					if (bots[i].bot_behaviour & BOT_CROUCHED)
						sprintf(msg, "Crouched\n");
					if (bots[i].bot_behaviour & BOT_STANDING)
						sprintf(msg, "Standing\n");
					ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

					if (bots[i].pEdict->v.flags & FL_PRONED)
					{
						sprintf(msg, "<%d> bot is proned\n", i);
						ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
					}
				}
			}
		}
		else
		{
			int i = UTIL_FindBotByName(arg1);

			if (i != -1)
			{
				sprintf(msg, "<%s>advT %.1f | OverrideAdvT %.1f | ChckStanceT %.1f | reloadT %.1f | globtime %.1f\n",
					bots[i].name, bots[i].f_combat_advance_time, bots[i].f_overide_advance_time,
					bots[i].f_check_stance_time, bots[i].f_reload_time, gpGlobals->time);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

				sprintf(msg, "snpT %.1f | waitT %.1f | dntmove %d | wait_f_enem %.1f\n",
					bots[i].sniping_time, bots[i].wpt_wait_time,
					bots[i].IsTask(TASK_DONTMOVEINCOMBAT), bots[i].f_bot_wait_for_enemy_time);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

				if (bots[i].bot_behaviour & BOT_PRONED)
					sprintf(msg, "Proned\n");
				if (bots[i].bot_behaviour & BOT_CROUCHED)
					sprintf(msg, "Crouched\n");
				if (bots[i].bot_behaviour & BOT_STANDING)
					sprintf(msg, "Standing\n");
				ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

				if (bots[i].pEdict->v.flags & FL_PRONED)
				{
					sprintf(msg, "<%d> bot is proned\n", i);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
				}
			}
		}

		return TRUE;
	}

	//TEMP: need cmd to test grenades in fa28
	else if (FStrEq(pcmd, "tgr"))
	{
		extern bot_weapon_t weapon_defs[MAX_WEAPONS];

		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (bots[i].is_used)
			{
				int gren = bots[i].grenade_slot;

				ALERT(at_console, "forced usage %d (if 4 it's grenade) | gren_used(have grens<6==used>) %d | grenade slot ID %d\n",
					bots[i].forced_usage, bots[i].grenade_action, gren);

				ALERT(at_console, "f_shootT %.2f | grenT %.2f | currentT %.2f\n",
					bots[i].f_shoot_time, bots[i].grenade_time, gpGlobals->time);

				if (gren != -1)
				{
					ALERT(at_console, "Showing records for current grenade ...\n");
					ALERT(at_console, "Current Ammo1 %d | Ammo1Max %d | Current Ammo2 %d | Ammo2Max %d | Weapon Flags %d\n",
						bots[i].curr_rgAmmo[weapon_defs[gren].iAmmo1], weapon_defs[gren].iAmmo1Max,
						bots[i].curr_rgAmmo[weapon_defs[gren].iAmmo2], weapon_defs[gren].iAmmo2Max,
						weapon_defs[gren].iFlags);
				}
				else
				{
					ALERT(at_console, "Showing records for standard frag grenade (current ammo will prolly be zeros) ...\n");
					ALERT(at_console, "Current Ammo1 %d | Ammo1Max %d | Current Ammo2 %d | Ammo2Max %d | Weapon Flags %d\n",
						bots[i].curr_rgAmmo[weapon_defs[fa_weapon_frag].iAmmo1], weapon_defs[fa_weapon_frag].iAmmo1Max,
						bots[i].curr_rgAmmo[weapon_defs[fa_weapon_frag].iAmmo2], weapon_defs[fa_weapon_frag].iAmmo2Max,
						weapon_defs[fa_weapon_frag].iFlags);
				}

				if (!(bots[i].bot_weapons & (1 << gren)))
					ALERT(at_console, "Both grenades already used\n");
				else
					ALERT(at_console, "Still has at least one grenade\n");
			}
		}

		return TRUE;
	}

	//TEMP: need cmd to test bot ammo array
	else if (FStrEq(pcmd, "tbaa"))
	{
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (bots[i].is_used)
			{
				ALERT(at_console, "Printing curr rgAmmo for %s ...\n", bots[i].name);
				for (int j = 0; j < MAX_AMMO_SLOTS; j++)
				{
					sprintf(msg, "slot #%d holds %d | ", j, bots[i].curr_rgAmmo[j]);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
				}
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "\n");
			}
		}

		return TRUE;
	}

	//TEMP: need cmd to test weaponarrays
	else if (FStrEq(pcmd, "checkw"))
	{
		bot_weapon_select_t* pSelect = NULL;
		bot_fire_delay_t* pDelay = NULL;

		pSelect = &bot_weapon_select[0];
		pDelay = &bot_fire_delay[0];

		if ((arg1 != NULL) && (*arg1 != 0))
		{
			int temp = atoi(arg1);

			ALERT(at_console, "pSelect min_safe_distance for it is %.0f\n", pSelect[temp].min_safe_distance);
			ALERT(at_console, "pSelect max_effective_distance for it is %.0f\n", pSelect[temp].max_effective_distance);
			ALERT(at_console, "pDelay ID for it is %d\n", pDelay[temp].iId);
			ALERT(at_console, "pDelay primary_base_delay for it is %.2f\n", pDelay[temp].primary_base_delay);
			ALERT(at_console, "pDelay primary_min_delay[4] for it is %.2f\n", pDelay[temp].primary_min_delay[4]);
			ALERT(at_console, "pDelay primary_max_delay[4] for it is %.2f\n", pDelay[temp].primary_max_delay[4]);
		}

		return TRUE;
	}

	//TEMP: just to mark point in public debugging file
	else if (FStrEq(pcmd, "dumphi") || FStrEq(pcmd, "dumphello") || FStrEq(pcmd, "dumphere"))
	{
		UTIL_DebugInFile("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ Start here! @@@@@@@@@@@@@@@@@@@@@@\n");

		return TRUE;
	}


	//TEMP: need cmd to test weaponarrays
	else if (FStrEq(pcmd, "dumpw"))
	{
		bot_weapon_select_t* pSelect = NULL;
		bot_fire_delay_t* pDelay = NULL;

		pSelect = &bot_weapon_select[0];
		pDelay = &bot_fire_delay[0];

		FILE* fp;

		UTIL_DebugDev("***New Dump***", -100, -100);

		fp = fopen(debug_fname, "a");

		if (fp)
		{
			for (int i = 0; i < MAX_WEAPONS; i++)
			{
				fprintf(fp, "Weapons ID %d\n", pSelect[i].iId);
				fprintf(fp, "pSelect min_safe_distance for it is %.2f\n", pSelect[i].min_safe_distance);
				fprintf(fp, "pSelect max_effective_distance for it is %.2f\n", pSelect[i].max_effective_distance);
				fprintf(fp, "pDelay ID for it is %d\n", pDelay[i].iId);
				fprintf(fp, "pDelay primary_base_delay for it is %.2f\n", pDelay[i].primary_base_delay);
				fprintf(fp, "pDelay primary_min_delay[4] for it is %.2f\n", pDelay[i].primary_min_delay[4]);
				fprintf(fp, "pDelay primary_max_delay[4] for it is %.2f\n", pDelay[i].primary_max_delay[4]);
			}

			fclose(fp);
		}

		return TRUE;
	}

	//TEMP: need cmd to test the usage of silencers
	else if (FStrEq(pcmd, "tws"))
	{
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (bots[i].is_used)
			{
				bool IsAllowSil = FALSE;
				bool IsMountSil = FALSE;
				bool IsSilUsed = FALSE;
				bool IsWeapReady = FALSE;
				bool Akimbo = FALSE;

				if (bots[i].IsWeaponStatus(WS_ALLOWSILENCER))
					IsAllowSil = TRUE;
				if (bots[i].IsWeaponStatus(WS_MOUNTSILENCER))
					IsMountSil = TRUE;
				if (bots[i].IsWeaponStatus(WS_SILENCERUSED))
					IsSilUsed = TRUE;
				if (bots[i].weapon_action == W_READY)
					IsWeapReady = TRUE;
				if (bots[i].IsWeaponStatus(WS_USEAKIMBO))
					Akimbo = TRUE;

				ALERT(at_console, "weapon status value: %d | AllowSil %d | MountSil %d | SilUsed %d || Weap.iAttach %d || IsWeaponReady %d || iusr3 %d || akimbo %d (diff que. %d)\n",
					bots[i].weapon_status, IsAllowSil, IsMountSil, IsSilUsed, bots[i].current_weapon.iAttachment,
					IsWeapReady, bots[i].pEdict->v.iuser3,
					UTIL_IsBitSet(WS_USEAKIMBO, bots[i].weapon_status), Akimbo);
			}
		}

		return TRUE;
	}

	//TEMP: need cmd to test following ability
	else if (FStrEq(pcmd, "tfl"))
	{
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (bots[i].is_used)
			{
				char user_name[32];
				char target_name[32];

				if (bots[i].pBotUser != NULL)
					strcpy(user_name, STRING(bots[i].pBotUser->v.netname));
				else
					strcpy(user_name, "no master");

				if (bots[i].pBotEnemy != NULL)
					strcpy(target_name, STRING(bots[i].pBotEnemy->v.netname));
				else
					strcpy(target_name, "no enemy");

				ALERT(at_console, "f_botuseT %.2f | user/master %s | enemy %s\n",
					bots[i].f_shoot_time, user_name, target_name);
			}
		}

		return TRUE;
	}

	//TEMP: need cmd to test wpts
	else if (FStrEq(pcmd, "checkwpt"))
	{
		extern void Wpt_Check(void);

		Wpt_Check();

		return TRUE;
	}

	//TEMP: need cmd to test targeted aim wpts
	else if (FStrEq(pcmd, "tba"))
	{
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (bots[i].is_used)
			{
				ALERT(at_console, "CurrA %d | Aim1 %d | Aim2 %d | Aim3 %d | Aim4 %d \n",
					bots[i].curr_aim_index + 1, bots[i].aim_index[0] + 1, bots[i].aim_index[1] + 1,
					bots[i].aim_index[2] + 1, bots[i].aim_index[3] + 1);
			}
		}

		return TRUE;
	}


	//TEMP: need cmd to test target distance
	else if (FStrEq(pcmd, "tbd"))
	{
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (bots[i].is_used)
			{
				if (bots[i].pBotEnemy)
				{
					ALERT(at_console, "3D Distance to my enemy %.0f || yaw_speed %.2f\n",
						(bots[i].pEdict->v.origin - bots[i].pBotEnemy->v.origin).Length(),
						bots[i].pEdict->v.yaw_speed);
				}
			}
		}

		return TRUE;
	}


	//TEMP: need cmd to test weapon usage
	else if ((FStrEq(pcmd, "tclay")) || (FStrEq(pcmd, "tbclay")) || (FStrEq(pcmd, "tbcm")))
	{
		sprintf(msg, "f_waypoint time %.1f | wpt_wait %.1f | wpt_action %.1f | globtime %.1f\n",
			bots[0].f_waypoint_time, bots[0].wpt_wait_time, bots[0].wpt_action_time, gpGlobals->time);
		ClientPrint(pEntity, HUD_PRINTCONSOLE, msg);

		bool used_as_goal = FALSE;
		if (bots[0].clay_action == ALTW_GOALPLACED)
			used_as_goal = TRUE;

		sprintf(msg, "clayslot %d | clay_action %d | goalplaced %d | clay_time %.2f\n",
			bots[0].claymore_slot, bots[0].clay_action, used_as_goal, bots[0].f_use_clay_time);
		ClientPrint(pEntity, HUD_PRINTCONSOLE, msg);

		return TRUE;
	}


	//TEMP: need cmd to test weapon usage
	else if (FStrEq(pcmd, "tbw") || FStrEq(pcmd, "checkbotweapon") || FStrEq(pcmd, "checkbotweapons"))
	{
		if ((arg1 != NULL) && (*arg1 != 0))
		{
			int i = UTIL_FindBotByName(arg1);

			if (i != -1 && bots[i].is_used)
			{
				char firemode[16];
				char w_using[16];
				char w_action[16];

				if (bots[i].current_weapon.iFireMode == FM_SEMI)
					strcpy(firemode, "SEMI");
				else if (bots[i].current_weapon.iFireMode == FM_BURST)
					strcpy(firemode, "BURST");
				else if (bots[i].current_weapon.iFireMode == FM_AUTO)
					strcpy(firemode, "AUTO");
				else
					strcpy(firemode, "NONE");

				if (bots[i].forced_usage == USE_MAIN)
					strcpy(w_using, "MAIN");
				else if (bots[i].forced_usage == USE_BACKUP)
					strcpy(w_using, "BACKUP");
				else if (bots[i].forced_usage == USE_KNIFE)
					strcpy(w_using, "KNIFE");
				else if (bots[i].forced_usage == USE_GRENADE)
					strcpy(w_using, "GRENADE");
				else
					strcpy(firemode, "NONE-ERROR");

				if (bots[i].weapon_action == W_READY)
					strcpy(w_action, "READY");
				else if (bots[i].weapon_action == W_TAKEOTHER)
					strcpy(w_action, "TAKEOTHER");
				else if (bots[i].weapon_action == W_INCHANGE)
					strcpy(w_action, "INCHANGE");
				else if (bots[i].weapon_action == W_INHANDS)
					strcpy(w_action, "INHANDS");
				else if (bots[i].weapon_action == W_INRELOAD)
					strcpy(w_action, "INRELOAD");
				else if (bots[i].weapon_action == W_INMERGEMAGS)
					strcpy(w_action, "INMERGEMAGS");
				else
					strcpy(firemode, "NONE-ERROR");

				sprintf(msg, "<%d>WeaponID %d | WeapFMode %s | iAttach %d | secFireActive %d | ZOOM(iuser3) %d | BotFOV %1.f\n",
					i, bots[i].current_weapon.iId, firemode, bots[i].current_weapon.iAttachment,
					bots[i].secondary_active, bots[i].pEdict->v.iuser3,
					bots[i].pEdict->v.fov);
				ClientPrint(pEntity, HUD_PRINTCONSOLE, msg);

				sprintf(msg, "<%d>CurrWeapon: Ammo in chamber/clip %d | Primary reserve ammo/clips %d | Secondary reserve ammo/clips %d\n",
					i, bots[i].current_weapon.iClip, bots[i].current_weapon.iAmmo1,
					bots[i].current_weapon.iAmmo2);
				ClientPrint(pEntity, HUD_PRINTCONSOLE, msg);

				sprintf(msg, "<%d>Using %s | WeaponAction %s | MainNoAMMO %d | BackupNoAMMO %d | EnemyDist %.1f\n",
					i, w_using, w_action, bots[i].main_no_ammo, bots[i].backup_no_ammo,
					bots[i].f_prev_enemy_dist);
				ClientPrint(pEntity, HUD_PRINTCONSOLE, msg);

				sprintf(msg, "<%d>MainW ammo/clips needed to be taken %d | BackupW ammo/clips needed to be taken %d\n",
					i, bots[i].take_main_mags, bots[i].take_backup_mags);
				ClientPrint(pEntity, HUD_PRINTCONSOLE, msg);

				sprintf(msg, "<%d>Is Bipod %d | MainW ID %d | BackupW ID %d\n",
					i, bots[i].IsTask(TASK_BIPOD), bots[i].main_weapon, bots[i].backup_weapon);
				ClientPrint(pEntity, HUD_PRINTCONSOLE, msg);

				int mainW_ammoID = -1;
				int backupW_ammoID = -1;
				int nade_ammoID = -1;
				extern bot_weapon_t weapon_defs[MAX_WEAPONS];

				if (bots[i].main_weapon != -1)
					mainW_ammoID = weapon_defs[bots[i].main_weapon].iAmmo1;
				if (bots[i].backup_weapon != -1)
					backupW_ammoID = weapon_defs[bots[i].backup_weapon].iAmmo1;
				if (bots[i].grenade_slot != -1)
					nade_ammoID = weapon_defs[bots[i].grenade_slot].iAmmo1;

				sprintf(msg, "<%d>MainW ID %d | MainW AmmoID %d | BackupW ID %d | BackupW AmmoID %d | Gren ID %d | Gren AmmoID %d\n",
					i, bots[i].main_weapon, mainW_ammoID, bots[i].backup_weapon, backupW_ammoID,
					bots[i].grenade_slot, nade_ammoID);
				ClientPrint(pEntity, HUD_PRINTCONSOLE, msg);
			}
		}
		else
		{
			for (int i = 0; i < MAX_CLIENTS; i++)
			{
				if ((bots[i].is_used) &&
					((g_debug_bot_on == false) || (g_debug_bot_on && (g_debug_bot == bots[i].pEdict))))
				{
					char firemode[16];
					char w_using[16];
					char w_action[16];

					if (bots[i].current_weapon.iFireMode == FM_SEMI)
						strcpy(firemode, "SEMI");
					else if (bots[i].current_weapon.iFireMode == FM_BURST)
						strcpy(firemode, "BURST");
					else if (bots[i].current_weapon.iFireMode == FM_AUTO)
						strcpy(firemode, "AUTO");
					else
						strcpy(firemode, "NONE");

					if (bots[i].forced_usage == USE_MAIN)
						strcpy(w_using, "MAIN");
					else if (bots[i].forced_usage == USE_BACKUP)
						strcpy(w_using, "BACKUP");
					else if (bots[i].forced_usage == USE_KNIFE)
						strcpy(w_using, "KNIFE");
					else if (bots[i].forced_usage == USE_GRENADE)
						strcpy(w_using, "GRENADE");
					else
						strcpy(firemode, "NONE-ERROR");

					if (bots[i].weapon_action == W_READY)
						strcpy(w_action, "READY");
					else if (bots[i].weapon_action == W_TAKEOTHER)
						strcpy(w_action, "TAKEOTHER");
					else if (bots[i].weapon_action == W_INCHANGE)
						strcpy(w_action, "INCHANGE");
					else if (bots[i].weapon_action == W_INHANDS)
						strcpy(w_action, "INHANDS");
					else if (bots[i].weapon_action == W_INRELOAD)
						strcpy(w_action, "INRELOAD");
					else if (bots[i].weapon_action == W_INMERGEMAGS)
						strcpy(w_action, "INMERGEMAGS");
					else
						strcpy(firemode, "NONE-ERROR");

					sprintf(msg, "<%d>WeaponID %d | WeapFMode %s | iAttach %d | secFireActive %d | ZOOM(iuser3) %d | BotFOV %1.f\n",
						i, bots[i].current_weapon.iId, firemode, bots[i].current_weapon.iAttachment,
						bots[i].secondary_active, bots[i].pEdict->v.iuser3,
						bots[i].pEdict->v.fov);
					ClientPrint(pEntity, HUD_PRINTCONSOLE, msg);

					sprintf(msg, "<%d>CurrWeapon: Ammo in chamber/clip %d | Primary reserve ammo/clips %d | Secondary reserve ammo/clips %d\n",
						i, bots[i].current_weapon.iClip, bots[i].current_weapon.iAmmo1,
						bots[i].current_weapon.iAmmo2);
					ClientPrint(pEntity, HUD_PRINTCONSOLE, msg);

					sprintf(msg, "<%d>Using %s | WeaponAction %s | MainNoAMMO %d | BackupNoAMMO %d | EnemyDist %.1f\n",
						i, w_using, w_action, bots[i].main_no_ammo, bots[i].backup_no_ammo,
						bots[i].f_prev_enemy_dist);
					ClientPrint(pEntity, HUD_PRINTCONSOLE, msg);

					sprintf(msg, "<%d>MainW ammo/clips needed to be taken %d | BackupW ammo/clips needed to be taken %d\n",
						i, bots[i].take_main_mags, bots[i].take_backup_mags);
					ClientPrint(pEntity, HUD_PRINTCONSOLE, msg);

					sprintf(msg, "<%d>Is Bipod %d | MainW ID %d | BackupW ID %d\n",
						i, bots[i].IsTask(TASK_BIPOD), bots[i].main_weapon, bots[i].backup_weapon);
					ClientPrint(pEntity, HUD_PRINTCONSOLE, msg);

					int mainW_ammoID = -1;
					int backupW_ammoID = -1;
					int nade_ammoID = -1;
					extern bot_weapon_t weapon_defs[MAX_WEAPONS];

					if (bots[i].main_weapon != -1)
						mainW_ammoID = weapon_defs[bots[i].main_weapon].iAmmo1;
					if (bots[i].backup_weapon != -1)
						backupW_ammoID = weapon_defs[bots[i].backup_weapon].iAmmo1;
					if (bots[i].grenade_slot != -1)
						nade_ammoID = weapon_defs[bots[i].grenade_slot].iAmmo1;

					sprintf(msg, "<%d>MainW ID %d | MainW AmmoID %d | BackupW ID %d | BackupW AmmoID %d | Gren ID %d | Gren AmmoID %d\n",
						i, bots[i].main_weapon, mainW_ammoID, bots[i].backup_weapon, backupW_ammoID,
						bots[i].grenade_slot, nade_ammoID);
					ClientPrint(pEntity, HUD_PRINTCONSOLE, msg);
				}
			}
		}

		return TRUE;
	}

	//TEMP: need cmd to set weapon usage to main weapon
	else if (FStrEq(pcmd, "sbwm") || FStrEq(pcmd, "forcedweaponselectmain") ||
		FStrEq(pcmd, "forcedselectweaponmain") || FStrEq(pcmd, "forcedweaponusemain"))
	{
		if ((arg1 != NULL) && (*arg1 != 0))
		{
			int i = UTIL_FindBotByName(arg1);

			if (i != -1 && bots[i].is_used)
			{
				bots[i].UseWeapon(USE_MAIN);
				bots[i].main_no_ammo = FALSE;

				ClientPrint(pRecipient, HUD_PRINTCONSOLE, "Weapon usage set to main weapon\n");
			}
		}
		else
		{
			if (bots[0].is_used)
			{
				bots[0].UseWeapon(USE_MAIN);
				bots[0].main_no_ammo = FALSE;

				ClientPrint(pRecipient, HUD_PRINTCONSOLE, "Weapon usage set to main weapon\n");
			}
		}

		return TRUE;
	}
	//TEMP: need cmd to set weapon usage to backup weapon
	else if (FStrEq(pcmd, "sbwb") || FStrEq(pcmd, "forcedweaponselectbackup") ||
	FStrEq(pcmd, "forcedselectweaponbackup") || FStrEq(pcmd, "forcedweaponusebackup"))
	{
		if ((arg1 != NULL) && (*arg1 != 0))
		{
			int i = UTIL_FindBotByName(arg1);

			if (i != -1 && bots[i].is_used)
			{
				bots[i].UseWeapon(USE_BACKUP);
				bots[i].main_no_ammo = TRUE;

				ClientPrint(pRecipient, HUD_PRINTCONSOLE, "Weapon usage set to backup weapon (main no ammo set too)\n");
			}
		}
		else
		{
			if (bots[0].is_used)
			{
				bots[0].UseWeapon(USE_BACKUP);
				bots[0].main_no_ammo = TRUE;

				ClientPrint(pRecipient, HUD_PRINTCONSOLE, "Weapon usage set to backup weapon (main no ammo set too)\n");
			}
		}

		return TRUE;
	}
	//TEMP: need cmd to set weapon usage to knife
	else if (FStrEq(pcmd, "buwk") || FStrEq(pcmd, "forcedweaponselectknife") ||
	FStrEq(pcmd, "forcedselectweaponknife") || FStrEq(pcmd, "forcedweaponuseknife"))
	{
		if ((arg1 != NULL) && (*arg1 != 0))
		{
			int i = UTIL_FindBotByName(arg1);

			if (i != -1 && bots[i].is_used)
			{
				bots[i].UseWeapon(USE_KNIFE);
				bots[i].main_no_ammo = TRUE;
				bots[i].backup_no_ammo = TRUE;
				bots[i].weapon_action = W_TAKEOTHER;

				ClientPrint(pRecipient, HUD_PRINTCONSOLE, "Weapon usage set to knife (main+backup no ammo set too)\n");
			}
		}
		else
		{
			if (bots[0].is_used)
			{
				bots[0].UseWeapon(USE_KNIFE);
				bots[0].main_no_ammo = TRUE;
				bots[0].backup_no_ammo = TRUE;
				bots[0].weapon_action = W_TAKEOTHER;

				ClientPrint(pRecipient, HUD_PRINTCONSOLE, "Weapon usage set to knife (main+backup no ammo set too)\n");
			}
		}

		return TRUE;
	}

	//TEMP: need cmd to force weapon reload
	else if (FStrEq(pcmd, "fr") || FStrEq(pcmd, "forcedreload"))
	{
		if ((arg1 != NULL) && (*arg1 != 0))
		{
			int i = UTIL_FindBotByName(arg1);

			if (i != -1 && bots[i].is_used)
			{
				bots[i].weapon_action = W_INRELOAD;
				bots[i].SetWeaponStatus(WS_NOTEMPTYMAG);

				ClientPrint(pRecipient, HUD_PRINTCONSOLE, "forced reload initialized (partly used magazine set too)\n");
			}
		}
		else
		{
			if (bots[0].is_used)
			{
				bots[0].weapon_action = W_INRELOAD;
				bots[0].SetWeaponStatus(WS_NOTEMPTYMAG);

				ClientPrint(pRecipient, HUD_PRINTCONSOLE, "forced reload initialized (partly used magazine set too)\n");
			}
		}

		return TRUE;
	}

	//TEMP: need cmd to force magazine merging
	else if (FStrEq(pcmd, "fmm") || FStrEq(pcmd, "forcedmagazinemerge") ||
	FStrEq(pcmd, "forcedmergemags") || FStrEq(pcmd, "forcedmergemagazine") || FStrEq(pcmd, "forcedmergeclips"))
	{
		if ((arg1 != NULL) && (*arg1 != 0))
		{
			int i = UTIL_FindBotByName(arg1);

			if (i != -1 && bots[i].is_used)
			{
				bots[i].SetWeaponStatus(WS_MERGEMAGS1);
				bots[i].SetWeaponStatus(WS_MERGEMAGS2);

				ClientPrint(pRecipient, HUD_PRINTCONSOLE, "Going to call merge magazines command (both flags set)\n");
			}
		}
		else
		{
			if (bots[0].is_used)
			{
				bots[0].SetWeaponStatus(WS_MERGEMAGS1);
				bots[0].SetWeaponStatus(WS_MERGEMAGS2);

				ClientPrint(pRecipient, HUD_PRINTCONSOLE, "Going to call merge magazines command (both flags set)\n");
			}
		}
		
		return TRUE;
	}

	else if (FStrEq(pcmd, "gbe"))
	{
		if ((arg1 != NULL) && (*arg1 != 0))
		{
			int index = UTIL_FindBotByName(arg1);

			if (index != -1)
			{
				if (bots[index].pBotEnemy)
				{
					ALERT(at_console, "(%s) My enemy is name %s || My target ent is %s || My target globname is %s || Distance to enemy is %.1f\n",
						bots[index].name, STRING(bots[index].pBotEnemy->v.netname),
						STRING(bots[index].pBotEnemy->v.classname),
						STRING(bots[index].pBotEnemy->v.globalname),
						bots[index].f_prev_enemy_dist);
				}
				else
					ALERT(at_console, "Have no enemy at the moment\n");
			}
		}
		else
		{
			for (int i = 0; i < MAX_CLIENTS; i++)
			{
				if (bots[i].is_used)
				{
					if (bots[i].pBotEnemy)
					{
						ALERT(at_console, "(%s) My enemy is name %s || My target ent is %s || My target globname is %s || Distance to enemy is %.1f\n",
							bots[i].name, STRING(bots[i].pBotEnemy->v.netname),
							STRING(bots[i].pBotEnemy->v.classname),
							STRING(bots[i].pBotEnemy->v.globalname),
							bots[i].f_prev_enemy_dist);
					}
					else
						ALERT(at_console, "Have no enemy at the moment\n");
				}
			}
		}

		return TRUE;
	}

	// TEMP: need cmd to write wpt and origins history!!!!!!!!!!
	else if (FStrEq(pcmd, "tbot"))
	{
		if ((arg1 != NULL) && (*arg1 != 0))
		{
			int i = UTIL_FindBotByName(arg1);

			if (i != -1 && bots[i].is_used)
			{
				sprintf(msg, "***<%s>\n", bots[i].name);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

				ALERT(at_console, "curwpt = %d | pwpt0 = %d | pwpt1 = %d | pwpt2 = %d | pwpt3 = %d | pwpt4 = %d |\ncuwpt.x = %d | cuwpt.y = %d | cuwpt.z = %d| pwpt.x = %d | pwpt.y = %d | pwpt.z = %d | cupath = %d | prevpath = %d\n",
					bots[i].curr_wpt_index + 1, bots[i].prev_wpt_index.get() + 1,
					bots[i].prev_wpt_index.get(1) + 1, bots[i].prev_wpt_index.get(2) + 1,
					bots[i].prev_wpt_index.get(3) + 1, bots[i].prev_wpt_index.get(4) + 1,
					(int)bots[i].wpt_origin.x, (int)bots[i].wpt_origin.y, (int)bots[i].wpt_origin.z,
					(int)bots[i].prev_wpt_origin.x, (int)bots[i].prev_wpt_origin.y, (int)bots[i].prev_wpt_origin.z,
					bots[i].curr_path_index + 1, bots[i].prev_path_index + 1);

				sprintf(msg, "curr wpt %d | ladder end wpt %d | op_path_dir %d || stuck_time %.2f | changed_dirs %d\n",
					bots[i].curr_wpt_index + 1, bots[i].end_wpt_index + 1,
					bots[i].opposite_path_dir, bots[i].f_stuck_time, bots[i].changed_direction);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

				sprintf(msg, "f_waypoint time %.1f | wpt_wait %.1f | wpt_action %.1f | globtime %.1f\n",
					bots[i].f_waypoint_time, bots[i].wpt_wait_time,
					bots[i].wpt_action_time, gpGlobals->time);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

				bool used_as_goal = FALSE;
				if (bots[i].clay_action == ALTW_GOALPLACED)
					used_as_goal = TRUE;

				ALERT(at_console, "clayslot %d | clay_action %d | goalplaced %d | clay_time %.2f\n",
					bots[i].claymore_slot, bots[i].clay_action, used_as_goal,
					bots[i].f_use_clay_time);

				ALERT(at_console, "bot team %d (model says %s)| bot class %d | bot buttons %d\n",
					bots[i].bot_team, STRING(bots[i].pEdict->v.model),
					bots[i].bot_class, bots[i].pEdict->v.button);

				ALERT(at_console, "bot start action %d | not_started %d | bot promoted %d | bot velocity %.1f\n",
					bots[i].start_action, bots[i].not_started, bots[i].IsTask(TASK_PROMOTED),
					bots[i].pEdict->v.velocity.Length2D());

				ALERT(at_console, "Acti_t %.1f | TANK %d | Bandages %d\n",
					bots[i].wpt_action_time, bots[i].IsTask(TASK_USETANK),
					bots[i].bot_bandages);

				sprintf(msg, "bot chute %d | chute used %d | chute time %.1f | flags %d\n",
					bots[i].IsTask(TASK_PARACHUTE), bots[i].IsTask(TASK_PARACHUTE_USED),
					bots[i].chute_action_time, bots[i].pEdict->v.flags);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
			}
		}
		else
		{
			for (int i = 0; i < MAX_CLIENTS; i++)
			{
				if (bots[i].is_used)
				{
					// do we need to debug just one bot and this bot isn't the one then skip him
					if ((g_debug_bot_on) && (bots[i].pEdict != g_debug_bot))
						continue;

					sprintf(msg, "***<%s>\n", bots[i].name);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

					//ALERT(at_console, "curwpt = %d | pwpt0 = %d | pwpt1 = %d | pwpt2 = %d | pwpt3 = %d | pwpt4 = %d |\ncuwpt.x = %d | cuwpt.y = %d | cuwpt.z = %d| pwpt.x = %d | pwpt.y = %d | pwpt.z = %d | cupath = %d | pevpath = %d\n",
					//	bots[i].curr_wpt_index + 1, bots[i].prev_wpt_index[0] + 1,
					//	bots[i].prev_wpt_index[1] + 1, bots[i].prev_wpt_index[2] + 1,
					//	bots[i].prev_wpt_index[3] + 1, bots[i].prev_wpt_index[4] + 1,
					//	(int)bots[i].wpt_origin.x, (int)bots[i].wpt_origin.y, (int)bots[i].wpt_origin.z,
					//	(int)bots[i].prev_wpt_origin.x,	(int)bots[i].prev_wpt_origin.y, (int)bots[i].prev_wpt_origin.z,
					//	bots[i].curr_path_index + 1, bots[i].prev_path_index + 1);

					ALERT(at_console, "curwpt = %d | pwpt0 = %d | pwpt1 = %d | pwpt2 = %d | pwpt3 = %d | pwpt4 = %d |\ncuwpt.x = %d | cuwpt.y = %d | cuwpt.z = %d| pwpt.x = %d | pwpt.y = %d | pwpt.z = %d | cupath = %d | pevpath = %d\n",
						bots[i].curr_wpt_index + 1, bots[i].prev_wpt_index.get() + 1,
						bots[i].prev_wpt_index.get(1) + 1, bots[i].prev_wpt_index.get(2) + 1,
						bots[i].prev_wpt_index.get(3) + 1, bots[i].prev_wpt_index.get(4) + 1,
						(int)bots[i].wpt_origin.x, (int)bots[i].wpt_origin.y, (int)bots[i].wpt_origin.z,
						(int)bots[i].prev_wpt_origin.x, (int)bots[i].prev_wpt_origin.y, (int)bots[i].prev_wpt_origin.z,
						bots[i].curr_path_index + 1, bots[i].prev_path_index + 1);

					sprintf(msg, "curr wpt %d | ladder end wpt %d | op_path_dir %d || stuck_time %.2f | changed_dirs %d\n",
						bots[i].curr_wpt_index + 1, bots[i].end_wpt_index + 1,
						bots[i].opposite_path_dir, bots[i].f_stuck_time, bots[i].changed_direction);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

					sprintf(msg, "f_waypoint time %.1f | wpt_wait %.1f | wpt_action %.1f | globtime %.1f\n",
						bots[i].f_waypoint_time, bots[i].wpt_wait_time,
						bots[i].wpt_action_time, gpGlobals->time);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

					bool used_as_goal = FALSE;
					if (bots[i].clay_action == ALTW_GOALPLACED)
						used_as_goal = TRUE;

					ALERT(at_console, "clayslot %d | clay_action %d | goalplaced %d | clay_time %.2f\n",
						bots[i].claymore_slot, bots[i].clay_action, used_as_goal,
						bots[i].f_use_clay_time);

					ALERT(at_console, "bot team %d (model says %s)| bot class %d | bot buttons %d\n",
						bots[i].bot_team, STRING(bots[i].pEdict->v.model),
						bots[i].bot_class, bots[i].pEdict->v.button);

					ALERT(at_console, "bot start action %d | not_started %d | bot promoted %d | bot velocity %.1f\n",
						bots[i].start_action, bots[i].not_started, bots[i].IsTask(TASK_PROMOTED),
						bots[i].pEdict->v.velocity.Length2D());

					ALERT(at_console, "Acti_t %.1f | TANK %d | Bandages %d\n",
						bots[i].wpt_action_time, bots[i].IsTask(TASK_USETANK),
						bots[i].bot_bandages);

					sprintf(msg, "bot chute %d | chute used %d | chute time %.1f | flags %d\n",
						bots[i].IsTask(TASK_PARACHUTE), bots[i].IsTask(TASK_PARACHUTE_USED),
						bots[i].chute_action_time, bots[i].pEdict->v.flags);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
				}
			}
		}

		return TRUE;
	}

	//TEMP: need cmd to test medical treatment
	else if (FStrEq(pcmd, "tbm"))
	{
		char values[96];

		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (bots[i].is_used)
			{
				strcpy(values, "<");

				if (bots[i].IsSubTask(ST_TREAT))
					strcat(values, " treat ");
				if (bots[i].IsSubTask(ST_HEAL))
					strcat(values, " heal ");
				if (bots[i].IsSubTask(ST_GIVE))
					strcat(values, " give ");
				if (bots[i].IsSubTask(ST_HEALED))
					strcat(values, " healed ");

				strcat(values, ">");

				sprintf(msg, "bot <%s> subtask flags: %s ||num %d\n",
					bots[i].name, values, bots[i].bot_subtasks);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

				sprintf(msg, "speakT %.1f | globT %.1f\n",
					bots[i].speak_time, gpGlobals->time);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

				sprintf(msg, "bothealhim %d | botbleeds %d | botdrowning %d | botbandages %d\n",
					bots[i].IsTask(TASK_HEALHIM), bots[i].IsTask(TASK_BLEEDING),
					bots[i].IsTask(TASK_DROWNING), bots[i].bot_bandages);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
			}
		}

		return TRUE;
	}

	//TEMP: remove it
	else if (FStrEq(pcmd, "tbf"))
	{
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (bots[i].is_used)
			{
				ALERT(at_console, "bot <%s> client flags (as string representation):\n",
					bots[i].name);

				if (bots[i].pEdict->v.flags & FL_SPECTATOR)
					ALERT(at_console, "<fl spectator>");
				if (bots[i].pEdict->v.flags & FL_CLIENT)
					ALERT(at_console, "<fl client>");
				if (bots[i].pEdict->v.flags & FL_FAKECLIENT)
					ALERT(at_console, "<fl fakeclient>");
				if (bots[i].pEdict->v.flags & FL_NOTARGET)
					ALERT(at_console, "<fl notarget>");
			}
		}

		return TRUE;
	}

	//TEMP: need cmd to test behaviour
	else if (FStrEq(pcmd, "tbb") || FStrEq(pcmd, "checkbehaviour"))
	{
		char bmsg[128];

		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if ((bots[i].is_used) &&
				((g_debug_bot_on == false) || (g_debug_bot_on && (g_debug_bot == bots[i].pEdict))))
			{
				sprintf(bmsg, "bot <%s> behaviour flags (as string representation):\n",
					bots[i].name);
				ClientPrint(pEntity, HUD_PRINTCONSOLE, bmsg);

				if (bots[i].IsBehaviour(ATTACKER))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<attacker>");
				if (bots[i].IsBehaviour(DEFENDER))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<defender>");
				if (bots[i].IsBehaviour(STANDARD))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<standard>");
				if (bots[i].IsBehaviour(SNIPER))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<sniper>");
				if (bots[i].IsBehaviour(MGUNNER))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<mgunner>");
				if (bots[i].IsBehaviour(CQUARTER))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<close quarter>");
				if (bots[i].IsBehaviour(COMMON))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<common soldier>");
				if (bots[i].IsBehaviour(BOT_STANDING))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<bot standing>");
				if (bots[i].IsBehaviour(BOT_CROUCHED))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<bot crouched>");
				if (bots[i].IsBehaviour(BOT_PRONED))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<bot proned>");
				if (bots[i].IsBehaviour(BOT_DONTGOPRONE))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<bot dont goprone>");
				if (bots[i].IsBehaviour(GOTO_STANDING))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<goto standing>");
				if (bots[i].IsBehaviour(GOTO_CROUCH))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<goto crouch>");
				if (bots[i].IsBehaviour(GOTO_PRONE))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<goto prone>");

				sprintf(bmsg, "\n and behaviour as the number: %d\n",
					bots[i].bot_behaviour);
				ClientPrint(pEntity, HUD_PRINTCONSOLE, bmsg);
			}
		}

		return TRUE;
	}

	//TEMP: need cmd to test tasks
	else if (FStrEq(pcmd, "tbt") || FStrEq(pcmd, "checktasks"))
	{
		char tmsg[128];

		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if ((bots[i].is_used) &&
				((g_debug_bot_on == false) || (g_debug_bot_on && (g_debug_bot == bots[i].pEdict))))
			{
				sprintf(tmsg, "bot <%s> all his tasks as strings\n", bots[i].name);
				ClientPrint(pEntity, HUD_PRINTCONSOLE, tmsg);

				if (bots[i].IsTask(TASK_PROMOTED))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<promoted>");
				if (bots[i].IsTask(TASK_DEATHFALL))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<deathfall>");
				if (bots[i].IsTask(TASK_DONTMOVEINCOMBAT))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<dontmoveincombat>");
				if (bots[i].IsTask(TASK_BREAKIT))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<breakit>");
				if (bots[i].IsTask(TASK_FIRE))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<fire>");
				if (bots[i].IsTask(TASK_IGNOREAIMS))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<ignoreaims>");
				if (bots[i].IsTask(TASK_PRECISEAIM))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<preciseaim>");
				if (bots[i].IsTask(TASK_CHECKAMMO))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<checkammo>");
				if (bots[i].IsTask(TASK_GOALITEM))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<goalitem>");
				if (bots[i].IsTask(TASK_NOJUMP))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<nojump>");
				if (bots[i].IsTask(TASK_SPRINT))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<sprint>");
				if (bots[i].IsTask(TASK_WPTACTION))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<wptaction>");
				if (bots[i].IsTask(TASK_BACKTOPATROL))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<backtopatrol>");
				if (bots[i].IsTask(TASK_PARACHUTE))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<parachute>");
				if (bots[i].IsTask(TASK_PARACHUTE_USED))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<parachute_used>");
				if (bots[i].IsTask(TASK_BLEEDING))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<bleeding>");
				if (bots[i].IsTask(TASK_DROWNING))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<drowning>");
				if (bots[i].IsTask(TASK_HEALHIM))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<healhim>");
				if (bots[i].IsTask(TASK_MEDEVAC))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<medevac>");
				if (bots[i].IsTask(TASK_FIND_ENEMY))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<find enemy>");
				if (bots[i].IsTask(TASK_USETANK))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<usetank>");
				if (bots[i].IsTask(TASK_BIPOD))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<bipod>");
				if (bots[i].IsTask(TASK_CLAY_IGNORE))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<clay ignore>");
				if (bots[i].IsTask(TASK_CLAY_EVADE))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<clay evade>");
				if (bots[i].IsTask(TASK_GOPRONE))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<goprone>");
				if (bots[i].IsTask(TASK_AVOID_ENEMY))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<avoidenemy>");
				if (bots[i].IsTask(TASK_IGNORE_ENEMY))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<ignoreenemy>");

				sprintf(tmsg, "\n and as the number %d\n", bots[i].bot_tasks);
				ClientPrint(pEntity, HUD_PRINTCONSOLE, tmsg);
			}
		}

		return TRUE;
	}


	//TEMP: need cmd to test subtasks
	else if (FStrEq(pcmd, "tbst") || FStrEq(pcmd, "checksubtasks"))
	{
		char stmsg[128];

		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if ((bots[i].is_used) &&
				((g_debug_bot_on == false) || (g_debug_bot_on && (g_debug_bot == bots[i].pEdict))))
			{
				sprintf(stmsg, "bot <%s> all his subtasks as strings\n", bots[i].name);
				ClientPrint(pEntity, HUD_PRINTCONSOLE, stmsg);

				if (bots[i].IsSubTask(ST_AIM_GETAIM))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<aim get aim>");
				if (bots[i].IsSubTask(ST_AIM_FACEIT))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<aim face it>");
				if (bots[i].IsSubTask(ST_AIM_SETTIME))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<aim set time>");
				if (bots[i].IsSubTask(ST_AIM_ADJUSTAIM))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<aim adjust aim>");
				if (bots[i].IsSubTask(ST_AIM_DONE))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<aim done>");
				if (bots[i].IsSubTask(ST_FACEITEM_DONE))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<face item done>");
				if (bots[i].IsSubTask(ST_BUTTON_USED))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<button_used>");
				if (bots[i].IsSubTask(ST_MEDEVAC_ST))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<medevac stomach>");
				if (bots[i].IsSubTask(ST_MEDEVAC_H))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<medevac head>");
				if (bots[i].IsSubTask(ST_MEDEVAC_F))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<medevac feet>");
				if (bots[i].IsSubTask(ST_MEDEVAC_DONE))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<medevac done>");
				if (bots[i].IsSubTask(ST_TREAT))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<treat>");
				if (bots[i].IsSubTask(ST_HEAL))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<heal>");
				if (bots[i].IsSubTask(ST_GIVE))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<give>");
				if (bots[i].IsSubTask(ST_HEALED))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<healed>");
				if (bots[i].IsSubTask(ST_SAY_CEASEFIRE))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<say cease fire>");
				if (bots[i].IsSubTask(ST_FACEENEMY))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<face enemy>");
				if (bots[i].IsSubTask(ST_RESETCLAYMORE))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<reset claymore>");
				if (bots[i].IsSubTask(ST_DOOR_OPEN))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<door open>");
				if (bots[i].IsSubTask(ST_TANK_SHORT))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<tank short>");
				if (bots[i].IsSubTask(ST_RANDOMCENTRE))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<random centre>");
				if (bots[i].IsSubTask(ST_NOOTHERNADE))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<no other grenade>");
				if (bots[i].IsSubTask(ST_W_CLIP))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<weapon clip>");
				if (bots[i].IsSubTask(ST_CANTPRONE))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<cant prone>");
				if (bots[i].IsSubTask(ST_EVASIONSTARTED))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<evasion started>");

				sprintf(stmsg, "\n and as number %d\n", bots[i].bot_subtasks);
				ClientPrint(pEntity, HUD_PRINTCONSOLE, stmsg);
			}
		}

		return TRUE;
	}

	//TEMP: need cmd to test needs
	else if (FStrEq(pcmd, "tbn") || FStrEq(pcmd, "checkneeds"))
	{
		char nmsg[128];

		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if ((bots[i].is_used) &&
				((g_debug_bot_on == false) || (g_debug_bot_on && (g_debug_bot == bots[i].pEdict))))
			{
				sprintf(nmsg, "bot <%s> all his needs as strings\n", bots[i].name);
				ClientPrint(pEntity, HUD_PRINTCONSOLE, nmsg);

				if (bots[i].IsNeed(NEED_AMMO))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<ammo>");
				if (bots[i].IsNeed(NEED_GOAL))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<goal>");
				if (bots[i].IsNeed(NEED_GOAL_NOT))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<goal not>");
				if (bots[i].IsNeed(NEED_NEXTWPT))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<next waypoint>");
				if (bots[i].IsNeed(NEED_RESETNAVIG))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<reset navigation>");

				if (bots[i].IsNeed(NEED_RESETPARACHUTE))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<reset parachute>");

				sprintf(nmsg, "\n and then as the number %d\n", bots[i].bot_needs);
				ClientPrint(pEntity, HUD_PRINTCONSOLE, nmsg);
			}
		}

		return TRUE;
	}


	//TEMP: need cmd to test weapon status
	else if (FStrEq(pcmd, "tbws") || FStrEq(pcmd, "checkweaponstatus"))
	{
		char wsmsg[128];
		
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if ((bots[i].is_used) &&
				((g_debug_bot_on == false) || (g_debug_bot_on && (g_debug_bot == bots[i].pEdict))))
			{
				sprintf(wsmsg, "bot <%s> all his weapon status bits as strings\n", bots[i].name);
				ClientPrint(pEntity, HUD_PRINTCONSOLE, wsmsg);

				if (bots[i].IsWeaponStatus(WS_ALLOWSILENCER))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<allow silencer>");
				if (bots[i].IsWeaponStatus(WS_MOUNTSILENCER))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<mount silencer>");
				if (bots[i].IsWeaponStatus(WS_SILENCERUSED))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<silencer used>");
				if (bots[i].IsWeaponStatus(WS_USEAKIMBO))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<use akimbo in old FAs>");
				if (bots[i].IsWeaponStatus(WS_PRESSRELOAD))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<press reload btn>");
				if (bots[i].IsWeaponStatus(WS_INVALID))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<invalidated reloading>");
				if (bots[i].IsWeaponStatus(WS_NOTEMPTYMAG))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<not empty magazine>");
				if (bots[i].IsWeaponStatus(WS_MERGEMAGS1))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<merge magazines1>");
				if (bots[i].IsWeaponStatus(WS_MERGEMAGS2))
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<merge magazines2>");

				sprintf(wsmsg, "\n and then as the number %d\n", bots[i].weapon_status);
				ClientPrint(pEntity, HUD_PRINTCONSOLE, wsmsg);
			}
		}

		return TRUE;
	}

	//TEMP: need cmd to test crowded wpt
	else if (FStrEq(pcmd, "tbcw"))
	{
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (bots[i].is_used)
			{
				float dist = 9999.999;
				if (bots[i].crowded_wpt_index != -1)
					dist = (bots[i].pEdict->v.origin - waypoints[bots[i].crowded_wpt_index].origin).Length();

				ALERT(at_console, "bot <%s> crowded_wpt #%d | curr_wpt #%d | prev_wpt #%d | move speed %d | distToCrowded %.2f\n",
					bots[i].name, bots[i].crowded_wpt_index + 1, bots[i].curr_wpt_index + 1,
					bots[i].prev_wpt_index.get() + 1, bots[i].prev_speed, dist);
			}
		}

		return TRUE;
	}

	//TEMP: need cmd to test path goal setting
	else if (FStrEq(pcmd, "tpg"))
	{
		if ((arg1 != NULL) && (*arg1 != 0))
		{
			int path_index = atoi(arg1);

			if (w_paths[path_index] == NULL)
			{
				if (w_paths[path_index]->flags & P_FL_MISC_GOAL_RED)
					ALERT(at_console, "Path #%d has a RED team goal priority\n", path_index + 1);
				if (w_paths[path_index]->flags & P_FL_MISC_GOAL_BLUE)
					ALERT(at_console, "Path #%d has a BLUE team goal priority\n", path_index + 1);
			}
		}
		else
		{
			for (int path_index = 0; path_index < num_w_paths; path_index++)
			{
				if (w_paths[path_index] == NULL)
					continue;
				if (w_paths[path_index]->flags & P_FL_MISC_GOAL_RED)
					ALERT(at_console, "Path #%d has a RED team goal priority\n", path_index + 1);
				if (w_paths[path_index]->flags & P_FL_MISC_GOAL_BLUE)
					ALERT(at_console, "Path #%d has a BLUE team goal priority\n", path_index + 1);
			}
		}

		ALERT(at_console, "\n");

		return TRUE;
	}

	//TEMP: need cmd to test fa skills
	else if (FStrEq(pcmd, "tfs"))
	{
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (bots[i].is_used)
			{
				sprintf(msg, "bot <%s> FAskills: ", bots[i].name);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

				if (bots[i].bot_fa_skill & MARKS1)
					ClientPrint(pEntity, HUD_PRINTNOTIFY, " marks1 ");
				if (bots[i].bot_fa_skill & MARKS2)
				{
					if (bots[i].bot_fa_skill & MARKS1)
						ClientPrint(pEntity, HUD_PRINTNOTIFY, " valid marks2 ");
					else
						ClientPrint(pEntity, HUD_PRINTNOTIFY, " !!! WARNING !!! invalid marks2 ");
				}
				if (bots[i].bot_fa_skill & NOMEN)
					ClientPrint(pEntity, HUD_PRINTNOTIFY, " nomen ");
				if (bots[i].bot_fa_skill & BATTAG)
					ClientPrint(pEntity, HUD_PRINTNOTIFY, " bagility ");
				if (bots[i].bot_fa_skill & LEADER)
					ClientPrint(pEntity, HUD_PRINTNOTIFY, " leader ");
				if (bots[i].bot_fa_skill & FAID)
					ClientPrint(pEntity, HUD_PRINTNOTIFY, " faid ");
				if (bots[i].bot_fa_skill & MEDIC)
				{
					if (bots[i].bot_fa_skill & FAID)
						ClientPrint(pEntity, HUD_PRINTNOTIFY, " valid fieldmed ");
					else
						ClientPrint(pEntity, HUD_PRINTNOTIFY, " !!! WARNING !!! invalid fieldmed ");
				}
				if (bots[i].bot_fa_skill & ARTY1)
					ClientPrint(pEntity, HUD_PRINTNOTIFY, " arty1 ");
				if (bots[i].bot_fa_skill & ARTY2)
				{
					if (bots[i].bot_fa_skill & ARTY1)
						ClientPrint(pEntity, HUD_PRINTNOTIFY, " valid arty2 ");
					else
						ClientPrint(pEntity, HUD_PRINTNOTIFY, " !!! WARNING !!! invalid arty2 ");
				}
				if (bots[i].bot_fa_skill & STEALTH)
					ClientPrint(pEntity, HUD_PRINTNOTIFY, " stealth ");

				sprintf(msg, " value %d\n", bots[i].bot_fa_skill);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
			}
		}

		return TRUE;
	}

	// TEMP: remove it
	else if (FStrEq(pcmd, "ttf"))
	{
		UTIL_MakeVectors(pEntity->v.v_angle);

		Vector vecStart = pEntity->v.origin + pEntity->v.view_ofs;
		Vector vecEnd = vecStart + gpGlobals->v_forward * 100;

		TraceResult tr;

		UTIL_TraceLine(vecStart, vecEnd, dont_ignore_monsters, dont_ignore_glass, pEntity, &tr);

		const char* texture = g_engfuncs.pfnTraceTexture(tr.pHit, vecStart, vecEnd);

		ALERT(at_console, "The name of the texture in front is %s\n", texture);

		return TRUE;
	}

	// get current value
	else if (FStrEq(pcmd, "txcm"))
	{
		extern float x_coord_mod;

		sprintf(msg, "x_coord current value is %.1f\n", x_coord_mod);
		ClientPrint(pRecipient, HUD_PRINTNOTIFY, msg);

		return TRUE;
	}
	// increment
	else if (FStrEq(pcmd, "txcma"))
	{
		extern float x_coord_mod;

		if ((arg1 != NULL) && (*arg1 != 0))
		{
			int temp = atoi(arg1);
			x_coord_mod = (float)temp;
		}
		else
			x_coord_mod += 0.5;

		sprintf(msg, "x_coord increased - current value is %.1f\n", x_coord_mod);
		ClientPrint(pRecipient, HUD_PRINTNOTIFY, msg);

		return TRUE;
	}
	// decrement
	else if (FStrEq(pcmd, "txcmd"))
	{
		extern float x_coord_mod;

		if ((arg1 != NULL) && (*arg1 != 0))
		{
			int temp = atoi(arg1);
			x_coord_mod = (float)temp * -1.0;
		}
		else
			x_coord_mod -= 0.5;

		sprintf(msg, "x_coord decreased - current value is %.1f\n", x_coord_mod);
		ClientPrint(pRecipient, HUD_PRINTNOTIFY, msg);

		return TRUE;
	}

	// get current value
	else if (FStrEq(pcmd, "tycm"))
	{
		extern float y_coord_mod;

		sprintf(msg, "y_coord current value is %.1f\n", y_coord_mod);
		ClientPrint(pRecipient, HUD_PRINTNOTIFY, msg);

		return TRUE;
	}
	// increment
	else if (FStrEq(pcmd, "tycma"))
	{
		extern float y_coord_mod;

		if ((arg1 != NULL) && (*arg1 != 0))
		{
			int temp = atoi(arg1);
			y_coord_mod = (float)temp;
		}
		else
			y_coord_mod += 0.5;

		sprintf(msg, "y_coord increased - current value is %.1f\n", y_coord_mod);
		ClientPrint(pRecipient, HUD_PRINTNOTIFY, msg);

		return TRUE;
	}
	// decrement
	else if (FStrEq(pcmd, "tycmd"))
	{
		extern float y_coord_mod;

		if ((arg1 != NULL) && (*arg1 != 0))
		{
			int temp = atoi(arg1);
			y_coord_mod = (float)temp * -1.0;
		}
		else
			y_coord_mod -= 0.5;

		sprintf(msg, "y_coord decreased - current value is %.1f\n", y_coord_mod);
		ClientPrint(pRecipient, HUD_PRINTNOTIFY, msg);

		return TRUE;
	}
	// reset
	else if (FStrEq(pcmd, "tcmr"))
	{
		extern float x_coord_mod;
		x_coord_mod = 0.0;

		extern float y_coord_mod;
		y_coord_mod = 0.0;

		extern bool inv_coords;
		inv_coords = false;

		extern bool dbl_coords;
		dbl_coords = false;

		extern bool mod_coords;
		mod_coords = false;

		extern float some_modifier;
		some_modifier = 1.0;

		ClientPrint(pRecipient, HUD_PRINTNOTIFY, "All special modifiers were reset\n");

		return TRUE;
	}
	// invert aka * -1.0
	else if (FStrEq(pcmd, "tcminvert") || FStrEq(pcmd, "tcminv"))
	{
		extern bool inv_coords;

		if (inv_coords)
		{
			inv_coords = false;
			sprintf(msg, "coords inversion was deactivated\n");
		}
		else
		{
			inv_coords = true;
			sprintf(msg, "coords inversion was activated\n");
		}

		ClientPrint(pRecipient, HUD_PRINTNOTIFY, msg);

		return TRUE;
	}
	else if (FStrEq(pcmd, "tcmdouble") || FStrEq(pcmd, "tcmdbl"))
	{
		extern bool dbl_coords;

		if (dbl_coords)
		{
			dbl_coords = false;
			sprintf(msg, "dbl_coords was deactivated\n");
		}
		else
		{
			dbl_coords = true;
			sprintf(msg, "dbl_coords was activated\n");
		}

		ClientPrint(pRecipient, HUD_PRINTNOTIFY, msg);

		return TRUE;
	}
	else if (FStrEq(pcmd, "tcmmod") || FStrEq(pcmd, "tcmm"))
	{
		extern bool mod_coords;

		if (mod_coords)
		{
			mod_coords = false;
			sprintf(msg, "mod_coords was deactivated\n");
		}
		else
		{
			mod_coords = true;
			sprintf(msg, "mod_coords was activated\n");
		}

		ClientPrint(pRecipient, HUD_PRINTNOTIFY, msg);

		return TRUE;
	}
	else if (FStrEq(pcmd, "tcmignore") || FStrEq(pcmd, "tcmie"))
	{
		extern bool ignore_aim_error_code;

		if (ignore_aim_error_code)
		{
			ignore_aim_error_code = false;
			sprintf(msg, "ignore_aim_error_code was deactivated\n");
		}
		else
		{
			ignore_aim_error_code = true;
			sprintf(msg, "ignore_aim_error_code IS NOW active\n");
		}

		ClientPrint(pRecipient, HUD_PRINTNOTIFY, msg);

		return TRUE;
	}
	else if (FStrEq(pcmd, "tcmval"))
	{
		float low_val = -1000;
		float hi_val = -1000;

		if ((arg1 != NULL) && (*arg1 != 0))
			low_val = atoi(arg1);

		if ((arg2 != NULL) && (*arg2 != 0))
			hi_val = atoi(arg2);

		if (low_val != -1000 && hi_val != -1000)
		{
			// make them float values
			low_val = low_val / 100;
			hi_val = hi_val / 100;

			float half_val = (low_val + hi_val) / 2;

			sprintf(msg, "The centre point between bottom value %.2f and top value %.2f is >> %.2f\nCentre-bottom is %.2f\nTop-centre is %.2f\n",
				low_val, hi_val, half_val, fabs(fabs(half_val) - fabs(low_val)), fabs(fabs(hi_val) - fabs(half_val)));
		}
		else
			sprintf(msg, "some error occured!\n");

		ClientPrint(pRecipient, HUD_PRINTNOTIFY, msg);

		return TRUE;
	}

	// get current value
	else if (FStrEq(pcmd, "tm"))
	{
		extern float some_modifier;

		sprintf(msg, "modifier current value is %.2f\n", some_modifier);
		ClientPrint(pRecipient, HUD_PRINTNOTIFY, msg);

		return TRUE;
	}
	// increment
	else if (FStrEq(pcmd, "tma"))
	{
		extern float some_modifier;

		if ((arg1 != NULL) && (*arg1 != 0))
		{
			int temp = atoi(arg1);
			some_modifier = (float)temp;
			some_modifier /= 100;
		}
		else
			some_modifier += 0.05;

		sprintf(msg, "modifier increased - current value is %.2f\n", some_modifier);
		ClientPrint(pRecipient, HUD_PRINTNOTIFY, msg);

		return TRUE;
	}
	// decrement
	else if (FStrEq(pcmd, "tmd"))
	{
		extern float some_modifier;

		if ((arg1 != NULL) && (*arg1 != 0))
		{
			int temp = atoi(arg1);
			some_modifier = (float)temp * -1.0;
			some_modifier /= 100;
		}
		else
			some_modifier -= 0.05;

		sprintf(msg, "modifier decreased - current value is %.2f\n", some_modifier);
		ClientPrint(pRecipient, HUD_PRINTNOTIFY, msg);

		return TRUE;
	}

	// TEMP: remove it
	else if (FStrEq(pcmd, "tud"))
	{
		//sprintf(msg, "user dmg_inflictor %x\n", pEntity->v.dmg_inflictor);
		sprintf(msg, "user dmg_inflictor %p\n", pEntity->v.dmg_inflictor);
		ClientPrint(pRecipient, HUD_PRINTNOTIFY, msg);

		return TRUE;
	}

	// TEMP to test ammobox use distance: remove it
	else if (FStrEq(pcmd, "tuda"))
	{
		Vector temp;
		edict_t* ab = NULL;
		ab = UTIL_FindEntityByClassname(ab, "ammobox");

		if (ab != NULL)
		{
			temp = ab->v.origin - pEntity->v.origin;

			sprintf(msg, "user distance to ammobox is %.3f\n", temp.Length());
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, msg);
		}
		else
		{
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "no ammobox!\n");
		}

		return TRUE;
	}

	// TEMP to test distance to given entity: remove it
	else if (FStrEq(pcmd, "tude"))
	{
		if ((arg1 != NULL) && (*arg1 != 0))
		{
			Vector temp;
			edict_t* ab = NULL;

			ab = UTIL_FindEntityByClassname(ab, arg1);

			if (ab != NULL)
			{
				//if (ab->v.origin == Vector(0, 0, 0))
				if (ab->v.origin == g_vecZero)
					temp = VecBModelOrigin(ab) - pEntity->v.origin;
				else
					temp = ab->v.origin - pEntity->v.origin;

				sprintf(msg, "user distance to %s is %.3f\n", arg1, temp.Length());
				ClientPrint(pRecipient, HUD_PRINTNOTIFY, msg);
			}
			else
			{
				ClientPrint(pRecipient, HUD_PRINTNOTIFY, "there's no entity with such classname!\n");
			}
		}
		else
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "missing argument (entity classname)!\n");

		return TRUE;
	}

	//TEMP: need cmd to test getting weapon IDs
	//else if (FStrEq(pcmd, "twid"))
	//{
	//	ALERT(at_console, "%s ID is %d\n", arg1, UTIL_GetIDFromName(arg1));
	//	ALERT(at_console, "weapon_knife ID is %d\n", UTIL_GetIDFromName("weapon_knife"));

	//	return TRUE;
	//}

	// TEMP: remove it
	else if (FStrEq(pcmd, "tuo"))
	{
		char str[256];

		Vector vec = pEntity->v.origin + pEntity->v.view_ofs;

		sprintf(str, "Orig_x %0.2f | Orig_y %0.2f | Orig_z %0.2f\n",
			pEntity->v.origin.x, pEntity->v.origin.y, pEntity->v.origin.z);
		ClientPrint(pEntity, HUD_PRINTCONSOLE, str);

		sprintf(str, "angles_x %0.2f | angles_y %0.2f | angles_z %0.2f\n",
			pEntity->v.angles.x, pEntity->v.angles.y, pEntity->v.angles.z);
		ClientPrint(pEntity, HUD_PRINTCONSOLE, str);

		sprintf(str, "v_angles_x %0.2f | v_angles_y %0.2f | v_angles_z %0.2f\n",
			pEntity->v.v_angle.x, pEntity->v.v_angle.y, pEntity->v.v_angle.z);
		ClientPrint(pEntity, HUD_PRINTCONSOLE, str);

		sprintf(str, "Eyes_x %0.2f | Eyes_y %0.2f | Eyes_z %0.2f\n",
			pEntity->v.view_ofs.x, pEntity->v.view_ofs.y, pEntity->v.view_ofs.z);
		ClientPrint(pEntity, HUD_PRINTCONSOLE, str);


		//sprintf(str, "Final_x %0.2f | Final_y %0.2f | Final_z %0.2f\n", vec.x, vec.y, vec.z);
		//ClientPrint(pEntity, HUD_PRINTCONSOLE, str);

		sprintf(str, "absmin_x %0.2f | absmin_y %0.2f | absmin_z %0.2f\nabsmax_x %0.2f | absmax_y %0.2f | absmax_z %0.2f\n",
			pEntity->v.absmin.x, pEntity->v.absmin.y, pEntity->v.absmin.z,
			pEntity->v.absmax.x, pEntity->v.absmax.y, pEntity->v.absmax.z);
		ClientPrint(pEntity, HUD_PRINTCONSOLE, str);

		float x = (float)pEntity->v.absmin.x + (float)pEntity->v.absmax.x;
		float y = (float)pEntity->v.absmin.y + (float)pEntity->v.absmax.y;
		float z = (float)pEntity->v.absmin.z + (float)pEntity->v.absmax.z;
		sprintf(str, "absmin+absmax -> x %0.2f | y %0.2f | z %0.2f\n", x, y, z);
		ClientPrint(pEntity, HUD_PRINTCONSOLE, str);

		sprintf(str, "mins_x %0.2f | mins_y %0.2f | mins_z %0.2f\nmaxs_x %0.2f | maxs_y %0.2f | maxs_z %0.2f\n",
			pEntity->v.mins.x, pEntity->v.mins.y, pEntity->v.mins.z,
			pEntity->v.maxs.x, pEntity->v.maxs.y, pEntity->v.maxs.z);
		ClientPrint(pEntity, HUD_PRINTCONSOLE, str);

		sprintf(str, "size_x %0.2f | size_y %0.2f | size_z %0.2f\n",
			pEntity->v.size.x, pEntity->v.size.y, pEntity->v.size.z);
		ClientPrint(pEntity, HUD_PRINTCONSOLE, str);

		if (bots[0].is_used)
		{
			sprintf(str, "bot[0]_aim_x %0.2f | bot[0]_aim__y %0.2f | bot[0]_aim__z %0.2f\n",
				bots[0].curr_aim_offset.x, bots[0].curr_aim_offset.y, bots[0].curr_aim_offset.z);
			ClientPrint(pEntity, HUD_PRINTCONSOLE, str);
		}

		return TRUE;
	}

	// TEMP: remove it
	else if (FStrEq(pcmd, "tuva"))
	{
		char str[256];

		Vector vec = pEntity->v.origin + pEntity->v.view_ofs;

		sprintf(str, "Orig_x %0.2f | Orig_y %0.2f | Orig_z %0.2f | and Eyes_z %0.2f\n",
			pEntity->v.origin.x, pEntity->v.origin.y, pEntity->v.origin.z, pEntity->v.view_ofs.z);
		ClientPrint(pEntity, HUD_PRINTCONSOLE, str);

		sprintf(str, "v_angles_x %0.2f | V_ANGLES_Y %0.2f | v_angles_z %0.2f\n",
			pEntity->v.v_angle.x, pEntity->v.v_angle.y, pEntity->v.v_angle.z);
		ClientPrint(pEntity, HUD_PRINTCONSOLE, str);

		sprintf(str, "origin+view_ofs x %0.2f | y %0.2f | z %0.2f\n", vec.x, vec.y, vec.z);
		ClientPrint(pEntity, HUD_PRINTCONSOLE, str);

		if (bots[0].is_used)
		{
			sprintf(str, "\nbot[0]_aim_x %0.2f | bot[0]_aim__y %0.2f | bot[0]_aim__z %0.2f\n",
				bots[0].curr_aim_offset.x, bots[0].curr_aim_offset.y, bots[0].curr_aim_offset.z);
			ClientPrint(pEntity, HUD_PRINTCONSOLE, str);

			sprintf(str, "bot[0] -> v_angles_x %0.2f | V_ANGLES_Y %0.2f | v_angles_z %0.2f\n",
				bots[0].pEdict->v.v_angle.x, bots[0].pEdict->v.v_angle.y, bots[0].pEdict->v.v_angle.z);
			ClientPrint(pEntity, HUD_PRINTCONSOLE, str);

			float something = 0.0;

			if (pEntity->v.v_angle.y < 0)
			{
				something = 360 - fabs(pEntity->v.v_angle.y);
			}
			else
				something = pEntity->v.v_angle.y;

			extern float some_modifier;
			extern float InvModYaw90(float yaw_angle);
			extern float ModYawAngle(float yaw_angle);

			float dist = (pEntity->v.origin - bots[0].pEdict->v.origin).Length();
			sprintf(str, "\ndist %.2f My Yaw on 360 scale (%.2f) current modifier %.2f\n", dist, something, some_modifier);
			ClientPrint(pEntity, HUD_PRINTCONSOLE, str);

			float something3 = InvModYaw90(pEntity->v.v_angle.y);
			float something2 = ModYawAngle(pEntity->v.v_angle.y);

			sprintf(str, "InvertedYaw90 (%.2f) ModYawAngle (%.2f)\n", something3, something2);
			ClientPrint(pEntity, HUD_PRINTCONSOLE, str);
		}

		return TRUE;
	}

	// TEMP: remove it
	else if (FStrEq(pcmd, "tu"))
	{
		char str[80];

		sprintf(str, "iu1-%d ||iu2-%d ||iu3-%d ||iu4-%d\n", pEntity->v.iuser1,
			pEntity->v.iuser2, pEntity->v.iuser3, pEntity->v.iuser4);
		ClientPrint(pEntity, HUD_PRINTCONSOLE, str);

		sprintf(str, "fu1-%.3f ||fu2-%.3f ||fu3-%.3f ||fu4-%.3f\n", pEntity->v.fuser1,
			pEntity->v.fuser2, pEntity->v.fuser3, pEntity->v.fuser4);
		ClientPrint(pEntity, HUD_PRINTCONSOLE, str);

		//sprintf(str, "flags %d || edict %x || pContainingEntity %x\n",
		sprintf(str, "flags %d || edict %p || pContainingEntity %p\n",
			pEntity->v.flags, pEntity, pEntity->v.pContainingEntity);
		ClientPrint(pEntity, HUD_PRINTNOTIFY, str);

		sprintf(str, "bInDuck-%d ||flDucktime-%d\n", pEntity->v.bInDuck, pEntity->v.flDuckTime);
		ClientPrint(pEntity, HUD_PRINTNOTIFY, str);

		sprintf(str, "Pitchspeed-%.2f\n", pEntity->v.pitch_speed);
		ClientPrint(pEntity, HUD_PRINTNOTIFY, str);
		sprintf(str, "Yawspeed-%.2f\n", pEntity->v.yaw_speed);
		ClientPrint(pEntity, HUD_PRINTNOTIFY, str);

		sprintf(str, "In team=%d\n", pEntity->v.team);
		ClientPrint(pEntity, HUD_PRINTNOTIFY, str);

		sprintf(str, "In team (using v.model record)=%s\n", STRING(pEntity->v.model));
		ClientPrint(pEntity, HUD_PRINTNOTIFY, str);

		return TRUE;
	}

	//TEMP: remove it
	else if (FStrEq(pcmd, "tuwater"))
	{
		sprintf(msg, "current waterlevel value %d\n", pRecipient->v.waterlevel);
		ClientPrint(pRecipient, HUD_PRINTNOTIFY, msg);

		if (pRecipient->v.flags & FL_INWATER)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "FL_INWATER\n");

		return TRUE;
	}

	else if (FStrEq(pcmd, "tuonground"))
	{
		ALERT(at_console, "standing still on ground??? <result(bool) %d> <flags %d>\n",
			(pRecipient->v.flags & FL_ONGROUND) == FL_ONGROUND, pRecipient->v.flags);

		return TRUE;
	}

	//TEMP: remove it
	else if (FStrEq(pcmd, "tuspec"))
	{
		if (pRecipient->v.flags & FL_SPECTATOR)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "fl spectator set\n");
		if (pRecipient->v.flags & FL_CLIENT)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "fl client set\n");
		if (pRecipient->v.flags & FL_FAKECLIENT)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "fl fakeclient set\n");
		if (pRecipient->v.flags & FL_NOTARGET)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "fl notarget set\n");

		return TRUE;
	}

	//TEMP: remove it
	else if (FStrEq(pcmd, "tuf"))
	{
		if (pRecipient->v.flags & FL_ONGROUND)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "FL_ONGROUND | ");
		if (pRecipient->v.flags & FL_PARTIALGROUND)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "FL_PARTIALGROUND | ");
		if (pRecipient->v.flags & FL_INWATER)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "FL_INWATER | ");
		if (pRecipient->v.flags & FL_WATERJUMP)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "FL_WATERJUMP | ");
		if (pRecipient->v.flags & FL_FROZEN)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "FL_FROZEN | ");
		if (pRecipient->v.flags & FL_FLOAT)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "FL_FLOAT | ");
		if (pRecipient->v.flags & FL_FLY)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "FL_FLY | ");
		if (pRecipient->v.flags & FL_CONVEYOR)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "FL_CONVEYOR | ");
		if (pRecipient->v.flags & FL_GRAPHED)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "FL_GRAPHED | ");
		if (pRecipient->v.flags & FL_SWIM)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "FL_SWIM | ");
		if (pRecipient->v.flags & FL_CLIENT)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "FL_CLIENT | ");
		if (pRecipient->v.flags & FL_MONSTER)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "FL_MONSTER | ");
		if (pRecipient->v.flags & FL_GODMODE)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "FL_GODMODE | ");
		if (pRecipient->v.flags & FL_NOTARGET)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "FL_NOTARGET | ");
		if (pRecipient->v.flags & FL_SKIPLOCALHOST)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "FL_SKIPLOCALHOST | ");
		if (pRecipient->v.flags & FL_FAKECLIENT)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "FL_FAKECLIENT | ");
		if (pRecipient->v.flags & FL_DUCKING)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "FL_DUCKING | ");
		if (pRecipient->v.flags & FL_IMMUNE_WATER)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "FL_IMMUNE_WATER | ");
		if (pRecipient->v.flags & FL_IMMUNE_SLIME)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "FL_IMMUNE_SLIME | ");
		if (pRecipient->v.flags & FL_IMMUNE_LAVA)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "FL_IMMUNE_LAVA | ");
		if (pRecipient->v.flags & FL_PROXY)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "FL_PROXY | ");
		if (pRecipient->v.flags & FL_ALWAYSTHINK)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "FL_ALWAYSTHINK | ");
		if (pRecipient->v.flags & FL_BASEVELOCITY)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "FL_BASEVELOCITY | ");
		if (pRecipient->v.flags & FL_MONSTERCLIP)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "FL_MONSTERCLIP | ");
		if (pRecipient->v.flags & FL_ONTRAIN)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "FL_ONTRAIN | ");
		if (pRecipient->v.flags & FL_WORLDBRUSH)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "FL_WORLDBRUSH | ");
		if (pRecipient->v.flags & FL_SPECTATOR)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "FL_SPECTATOR | ");
		if (pRecipient->v.flags & FL_PRONED)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "1<<27 (something) fully proned in FA | ");
		if (pRecipient->v.flags & FL_BROKENLEG)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "1<<28 (something2) broken leg in FA | ");
		if (pRecipient->v.flags & FL_CUSTOMENTITY)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "FL_CUSTOMENTITY | ");
		if (pRecipient->v.flags & FL_KILLME)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "FL_KILLME | ");
		if (pRecipient->v.flags & FL_DORMANT)
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "FL_DORMANT | ");

		return TRUE;
	}

	else if (FStrEq(pcmd, "ffs"))
	{
		if (bots[0].is_used == false)
			return TRUE;

		if (bots[0].is_forced)
		{
			bots[0].is_forced = false;
			ClientPrint(pRecipient, HUD_PRINTCONSOLE, "forced stance DISABLED\n");
		}
		else
		{
			bots[0].is_forced = true;
			ClientPrint(pRecipient, HUD_PRINTCONSOLE, "forced stance ENABLED\n");
		}
		return TRUE;
	}

	else if (FStrEq(pcmd, "fsnow"))
	{
		if (bots[0].is_used)
		{
			bots[0].is_forced = true;
			SetStance(&bots[0], bots[0].forced_stance);
			ClientPrint(pRecipient, HUD_PRINTCONSOLE, "enforcing forced stance NOW\n");
		}

		return TRUE;
	}

	else if (FStrEq(pcmd, "fss"))
	{
		if (bots[0].is_used)
		{
			bots[0].is_forced = true;
			bots[0].forced_stance = GOTO_STANDING;
			ClientPrint(pRecipient, HUD_PRINTCONSOLE, "forced stance set to STANDING\n");
		}

		return TRUE;
	}
	else if (FStrEq(pcmd, "fsc"))
	{
		if (bots[0].is_used)
		{
			bots[0].is_forced = true;
			bots[0].forced_stance = GOTO_CROUCH;
			ClientPrint(pRecipient, HUD_PRINTCONSOLE, "forced stance set to CROUCHED\n");
		}

		return TRUE;
	}
	else if (FStrEq(pcmd, "fsp"))
	{
		if (bots[0].is_used)
		{
			bots[0].is_forced = true;
			bots[0].forced_stance = GOTO_PRONE;
			ClientPrint(pRecipient, HUD_PRINTCONSOLE, "forced stance set to PRONED\n");
		}

		return TRUE;
	}

	else if (FStrEq(pcmd, "frwt") || FStrEq(pcmd, "forcedresetwaittime"))
	{
		if (bots[0].is_used)
		{
			bots[0].wpt_wait_time = gpGlobals->time;
			ClientPrint(pRecipient, HUD_PRINTCONSOLE, "forced wait time reset done\n");
		}

		return TRUE;
	}

	else if (FStrEq(pcmd, "dumpuser"))
	{
		UTIL_DumpEdictToFile(pEntity);
		return TRUE;
	}

	else if ((FStrEq(pcmd, "dumpbot")) || (FStrEq(pcmd, "db")))
	{
		if ((arg1 != NULL) && (*arg1 != 0))
		{
			int index = UTIL_FindBotByName(arg1);

			if (index != -1)
				UTIL_DumpEdictToFile(bots[index].pEdict);
		}
		else
		{
			for (int i = 0; i < MAX_CLIENTS; i++)
			{
				if (bots[i].is_used)
					UTIL_DumpEdictToFile(bots[i].pEdict);
			}
		}

		return TRUE;
	}

	else if ((FStrEq(pcmd, "dumpent")) || (FStrEq(pcmd, "dumpentity")))
	{
		edict_t* pent = NULL;
		float radius = PLAYER_SEARCH_RADIUS;
		char str[256];

		if ((arg1 != NULL) && (*arg1 != 0))
		{
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "scanning entities in sphere...\n");

			bool noresults = TRUE;

			while ((pent = UTIL_FindEntityInSphere(pent, pEntity->v.origin, radius)) != NULL)
			{
				if (strcmp(STRING(pent->v.classname), arg1) == 0)
				{
					//sprintf(str, "Found %s (pent %x) has been dumped to file!\n",
					sprintf(str, "Found %s (pent %p) has been dumped to file!\n",
						STRING(pent->v.classname), pent);
					ClientPrint(pEntity, HUD_PRINTCONSOLE, str);

					UTIL_DumpEdictToFile(pent);
				}
				else
					noresults = FALSE;
			}

			if (noresults)
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "Found nothing here!\n");
		}
		else
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "Nothing. Specify entity name!\n");

		return TRUE;
	}

	// cmd to test clients array
	else if (FStrEq(pcmd, "dumpclients"))
	{
		FILE* cfp;
		cfp = fopen(debug_fname, "a");

		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			fprintf(cfp, "\n********Dumping the client_t array slot #%d********\n", i);
			//fprintf(cfp, "pEntity: %x\n", clients[i].pEntity);
			fprintf(cfp, "pEntity: %p\n", clients[i].pEntity);
			fprintf(cfp, "client_is_human(bool): %d\n", clients[i].IsHuman());
			fprintf(cfp, "client_bleeds(bool): %d\n", clients[i].IsBleeding());
			fprintf(cfp, "number of humans in game: %d\n", clients[i].HumanCount());
			fprintf(cfp, "number of bots in game: %d\n", clients[i].BotCount());
			fprintf(cfp, "total number of clients: %d\n", clients[i].ClientCount());
			fprintf(cfp, "*********END OF THIS DUMP*********\n");
		}

		fclose(cfp);

		ClientPrint(pEntity, HUD_PRINTNOTIFY, "Whole clients array has been written to debugging file!\n");

		return TRUE;
	}

	else if (FStrEq(pcmd, "search"))
	{
		edict_t* pent = NULL;
		float radius = PLAYER_SEARCH_RADIUS;
		char str[80];

		ClientPrint(pEntity, HUD_PRINTCONSOLE, "searching...\n");

		while ((pent = UTIL_FindEntityInSphere(pent, pEntity->v.origin, radius)) != NULL)
		{
			sprintf(str, "Found %s at %.2f %.2f %.2f\n", STRING(pent->v.classname),
				pent->v.origin.x, pent->v.origin.y, pent->v.origin.z);
			ClientPrint(pEntity, HUD_PRINTCONSOLE, str);

			//FILE *fp=fopen(debug_fname,"a");
			//fprintf(fp, "ClientCommmand: search %s", str);
			//fclose(fp);
		}

		return TRUE;
	}

	else if (FStrEq(pcmd, "scan"))
	{
		edict_t* pent = NULL;
		float radius = PLAYER_SEARCH_RADIUS;
		char str[80];
		Vector entity_o;

		ClientPrint(pEntity, HUD_PRINTCONSOLE, "scanning...\n");

		while ((pent = UTIL_FindEntityInSphere(pent, pEntity->v.origin, radius)) != NULL)
		{
			entity_o = VecBModelOrigin(pent);

			sprintf(str, "Found %s at %.2f %.2f %.2f\n", STRING(pent->v.classname),
				entity_o.x, entity_o.y, entity_o.z);
			ClientPrint(pEntity, HUD_PRINTCONSOLE, str);

			//FILE *fp=fopen(debug_fname,"a");
			//fprintf(fp, "ClientCommmand: scan %s", str);
			//fclose(fp);
		}

		return TRUE;
	}

	else if (FStrEq(pcmd, "extscan"))
	{
		edict_t* pent = NULL;
		float radius = EXTENDED_SEARCH_RADIUS;
		char str[80];
		Vector entity_o;

		ClientPrint(pEntity, HUD_PRINTCONSOLE, "scanning...\n");

		while ((pent = UTIL_FindEntityInSphere(pent, pEntity->v.origin, radius)) != NULL)
		{
			entity_o = VecBModelOrigin(pent);

			sprintf(str, "Found %s at %.2f %.2f %.2f\n", STRING(pent->v.classname),
				entity_o.x, entity_o.y, entity_o.z);
			ClientPrint(pEntity, HUD_PRINTCONSOLE, str);

			//FILE *fp=fopen(debug_fname,"a");
			//fprintf(fp, "ClientCommmand: scan %s", str);
			//fclose(fp);
		}

		return TRUE;
	}

	else if (FStrEq(pcmd, "dscan"))
	{
		edict_t* pent = NULL;
		float radius = PLAYER_SEARCH_RADIUS;
		char str[80];
		Vector entity_o;

		if ((arg1 != NULL) && (*arg1 != 0))
		{
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "scanning the entity...\n");

			while ((pent = UTIL_FindEntityByClassname(pent, arg1)) != NULL)
			{
				entity_o = VecBModelOrigin(pent);

				//sprintf(str, "Found %s at %.2f %.2f %.2f (pent %x)\n",
				sprintf(str, "Found %s at %.2f %.2f %.2f (pent %p)\n",
					STRING(pent->v.classname), entity_o.x, entity_o.y, entity_o.z, pent);
				ClientPrint(pEntity, HUD_PRINTCONSOLE, str);

				//sprintf(str, "flags %d || deadflag %d || me (edict) %x\n",
				sprintf(str, "flags %d || deadflag %d || me (edict) %p\n",
					pent->v.flags, pent->v.deadflag, pEntity);
				ClientPrint(pEntity, HUD_PRINTCONSOLE, str);

				if (pent->v.model != NULL)
				{
					sprintf(str, "string1 %s\n", STRING(pent->v.model));
					ClientPrint(pEntity, HUD_PRINTCONSOLE, str);
				}


				Vector v_myhead = pEntity->v.origin + pEntity->v.view_ofs;
				TraceResult tr;
				UTIL_TraceLine(v_myhead, entity_o, ignore_monsters, pEntity, &tr);

				sprintf(str, "TraceResult: flfract %.2f | phit name %s\n",
					tr.flFraction, STRING(tr.pHit->v.classname));
				ClientPrint(pEntity, HUD_PRINTCONSOLE, str);
			}
		}

		return TRUE;
	}

	else if ((FStrEq(pcmd, "scangrenades")) || (FStrEq(pcmd, "scangr")))
	{
		edict_t* pent = NULL;
		edict_t* pEdict = pEntity;

		while ((pent = UTIL_FindEntityInSphere(pent, pEdict->v.origin, PLAYER_SEARCH_RADIUS)) != NULL)
		{
			if ((strcmp(STRING(pent->v.classname), "item_concussion") != 0) &&
				(strcmp(STRING(pent->v.classname), "item_frag") != 0) &&
				(strcmp(STRING(pent->v.classname), "item_stg24") != 0) &&
				(strcmp(STRING(pent->v.classname), "item_flashbang") != 0) &&
				(strcmp(STRING(pent->v.classname), "weapon_concussion") != 0) &&
				(strcmp(STRING(pent->v.classname), "weapon_frag") != 0) &&
				(strcmp(STRING(pent->v.classname), "weapon_stg24") != 0) &&
				(strcmp(STRING(pent->v.classname), "weapon_flashbang") != 0))
				continue;

			if ((pent->v.solid == SOLID_NOT) && (pent->v.movetype == MOVETYPE_FOLLOW))
				continue;

			ClientPrint(pEntity, HUD_PRINTCONSOLE, "FOUND some grenade AROUND\n");
			UTIL_DumpEdictToFile(pent);

			char msg[80];
			sprintf(msg, "grenade's owner team is %d\n", UTIL_GetTeam(pent));
			ClientPrint(pEntity, HUD_PRINTCONSOLE, msg);
			break;
		}

		if (pent == NULL)
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "no grenade around\n");

		return TRUE;
	}


	// TEMP: only a test of one of bots routines
	else if (FStrEq(pcmd, "scanbreak"))
	{
		edict_t* pent = NULL;
		edict_t* pEdict = pEntity;


		// search the surrounding for entities
		while ((pent = UTIL_FindEntityInSphere(pent, pEdict->v.origin, EXTENDED_SEARCH_RADIUS)) != NULL)
		{
			if ((strcmp(STRING(pent->v.classname), "func_breakable") != 0) &&
				(strcmp(STRING(pent->v.classname), "fa_sd_object") != 0))
				continue;

			if ((pent->v.spawnflags & SF_BREAK_TRIGGER_ONLY) || (pent->v.takedamage == DAMAGE_NO) ||
				(pent->v.impulse == 1))
				continue;

			if (((pent->v.team == TEAM_RED) && (pEdict->v.team == TEAM_BLUE)) ||
				((pent->v.team == TEAM_BLUE) && (pEdict->v.team == TEAM_RED)))
				continue;


			Vector entity_origin = VecBModelOrigin(pent);
			Vector entity = pent->v.origin - pEdict->v.origin;

			TraceResult tr;
			Vector v_src = pEdict->v.origin + pEdict->v.view_ofs;

			UTIL_TraceLine(v_src, entity_origin, dont_ignore_monsters, pEdict, &tr);
			DrawBeam(pEntity, v_src, entity_origin, 100, 10, 250, 10, 15);

			if ((strcmp(STRING(tr.pHit->v.classname), "func_breakable") == 0) ||
				(strcmp(STRING(tr.pHit->v.classname), "fa_sd_object") == 0))
			{
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "FOUND a BREAKABLE somewhere AROUND\n");
			}
			else if ((tr.flFraction == 1.0) && (pent->v.solid == SOLID_BSP))
			{
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "FOUND a BREAKABLE somewhere AROUND\n");
			}
		}

		if (pent == NULL)
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "no object there OR it cannot be destroyed\n");

		return TRUE;
	}


	else if (FStrEq(pcmd, "tracef"))
	{
		char str[80];
		Vector v_src, v_des;
		TraceResult tr;

		UTIL_MakeVectors(pEntity->v.v_angle);

		v_src = pEntity->v.origin + pEntity->v.view_ofs;
		v_des = v_src + gpGlobals->v_forward * (RANGE_MELEE / 2);

		UTIL_TraceLine(v_src, v_des, dont_ignore_monsters, pEntity->v.pContainingEntity, &tr);

		ClientPrint(pEntity, HUD_PRINTCONSOLE, "tracing (dont ignore monsters) forward from eyes...\n");

		sprintf(str, "Result=%.2f (%s)\nsrc_x%.2f src_y%.2f src_z%.2f\ndes_x%.2f des_y%.2f des_z%.2f\n",
			tr.flFraction, STRING(tr.pHit->v.classname), v_src.x, v_src.y, v_src.z, v_des.x, v_des.y, v_des.z);
		ClientPrint(pEntity, HUD_PRINTCONSOLE, str);

		return TRUE;
	}

	else if ((FStrEq(pcmd, "tracef2")) || (FStrEq(pcmd, "tracefadv")))
	{
		char str[80];
		Vector v_src, v_des;
		TraceResult tr;

		UTIL_MakeVectors(pEntity->v.v_angle);

		v_src = pEntity->v.origin + pEntity->v.view_ofs;
		v_des = v_src + gpGlobals->v_forward * (RANGE_MELEE / 2);

		UTIL_TraceLine(v_src, v_des, dont_ignore_monsters, dont_ignore_glass, pEntity->v.pContainingEntity, &tr);

		ClientPrint(pEntity, HUD_PRINTCONSOLE, "tracing (dont ignore monsters & glass) forward from eyes...\n");

		sprintf(str, "Result=%.2f (%s)\nsrc_x%.2f src_y%.2f src_z%.2f\ndes_x%.2f des_y%.2f des_z%.2f\n",
			tr.flFraction, STRING(tr.pHit->v.classname), v_src.x, v_src.y, v_src.z, v_des.x, v_des.y, v_des.z);
		ClientPrint(pEntity, HUD_PRINTCONSOLE, str);

		return TRUE;
	}

	else if (FStrEq(pcmd, "tracef3"))
	{
		char str[80];
		Vector v_src, v_des;
		TraceResult tr;

		UTIL_MakeVectors(pEntity->v.v_angle);

		v_src = pEntity->v.origin + pEntity->v.view_ofs;
		v_des = v_src + gpGlobals->v_forward * (RANGE_MELEE / 2);

		UTIL_TraceLine(v_src, v_des, ignore_monsters, ignore_glass, pEntity->v.pContainingEntity, &tr);

		ClientPrint(pEntity, HUD_PRINTCONSOLE, "tracing (IGNORE monsters and glass) forward from eyes...\n");

		sprintf(str, "Result=%.2f (%s)\nsrc_x%.2f src_y%.2f src_z%.2f\ndes_x%.2f des_y%.2f des_z%.2f\n",
			tr.flFraction, STRING(tr.pHit->v.classname), v_src.x, v_src.y, v_src.z, v_des.x, v_des.y, v_des.z);
		ClientPrint(pEntity, HUD_PRINTCONSOLE, str);

		return TRUE;
	}

	// to test what bot sees when checking for enemy
	else if (FStrEq(pcmd, "testvis"))
	{
		if (bots[0].is_used)
		{
			int visibility = FPlayerVisible(bots[0].pEdict->v.origin, pEntity);

			if (visibility == VIS_YES)
			{
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "My enemy is visible!\n");
			}
			else if (visibility == VIS_FENCE)
			{
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "My enemy is behind FENCE!\n");
			}
			else
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "I don't see my target!!!\n");
		}
		else
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "No bots in game!\n");

		return TRUE;
	}

	else if ((FStrEq(pcmd, "traceentities")) || (FStrEq(pcmd, "trace_entities")) ||
		(FStrEq(pcmd, "traceentity")) || (FStrEq(pcmd, "trace_entity")) ||
		(FStrEq(pcmd, "traceents")) || (FStrEq(pcmd, "trace_ents")) ||
		(FStrEq(pcmd, "scanents")) || (FStrEq(pcmd, "scan_ents")))
	{
		edict_t* pent = NULL;
		float radius = PLAYER_SEARCH_RADIUS;
		char str[512];

		if ((arg1 != NULL) && (*arg1 != 0))
		{
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "scanning the entity...\n");

			bool noresults = TRUE;

			while ((pent = UTIL_FindEntityInSphere(pent, pEntity->v.origin, radius)) != NULL)
			{
				if (strcmp(STRING(pent->v.classname), arg1) == 0)
				{
					sprintf(str, "Found %s (pent %p)\ngetting more details about it...\n",
						STRING(pent->v.classname), pent);
					ClientPrint(pEntity, HUD_PRINTCONSOLE, str);

					sprintf(str, "(rendermode %d) (renderamt %.2f) (renderfx %d) (health %.2f) (spawnflags %d) (flags %d) (solid %d) (movetype %d)\n",
						pent->v.rendermode, pent->v.renderamt, pent->v.renderfx, pent->v.health, pent->v.spawnflags,
						pent->v.flags, pent->v.solid, pent->v.movetype);
					ClientPrint(pEntity, HUD_PRINTCONSOLE, str);
				}
			}

			if (pent == NULL)
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "Found nothing here!\n");
		}
		else
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "Nothing. Specify entity name!\n");

		return TRUE;
	}


	else if ((FStrEq(pcmd, "testentitylength")) || (FStrEq(pcmd, "testentlen")) ||
		(FStrEq(pcmd, "getentlen")) || (FStrEq(pcmd, "tracelength")))
	{
		edict_t* pent = NULL;
		float radius = EXTENDED_SEARCH_RADIUS;
		char str[256];

		if ((arg1 != NULL) && (*arg1 != 0))
		{
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "scanning the entity...\n");

			bool noresults = TRUE;

			while ((pent = UTIL_FindEntityInSphere(pent, pEntity->v.origin, radius)) != NULL)
			{
				if (strcmp(STRING(pent->v.classname), arg1) == 0)
				{
					Vector entity_origin = VecBModelOrigin(pent);
					Vector entity = entity_origin - pEntity->v.origin;

					//sprintf(str, "Found %s (pent %x) | distance %.2f\n",
					sprintf(str, "Found %s (pent %p) | distance %.2f\n",
						STRING(pent->v.classname), pent, entity.Length());
					ClientPrint(pEntity, HUD_PRINTCONSOLE, str);
				}
			}

			if (pent == NULL)
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "Found nothing here!\n");
		}
		else
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "Nothing. Specify entity name!\n");

		return TRUE;
	}

	else if (FStrEq(pcmd, "testhudmsg"))
	{
		if ((arg1 != NULL) && (*arg1 != 0))
		{
			float temp = atoi(arg1);
			///////////////////////////////////////////////
			//
			// example of bot HudMessages by Shrike
			//
			/////////////////////

			// Standard message
			if (temp == 1)
				StdHudMessage(NULL, "Example of MarineBot HudMessages \n by Shrike !!", 1, NULL);

			else if (temp == 2)
				StdHudMessageToAll("Example of MarineBot HudMessages \n by Shrike !!", NULL, 7);

			/////////////////////
			// Custom Message
			//				
			else if (temp > 2)
			{
				Vector red, blue;
				int gfx, time;

				red.x = 250;	//rgb
				red.y = 0;
				red.z = 0;
				blue.x = 0;		//rbg
				blue.y = 0;
				blue.z = 250;
				gfx = 2;		// fade over effect // 0 no effect // 1 flashing
				time = 8;	// hold for 8 seconds 

				if (temp == 3)
					CustHudMessage(pEntity, "Example of MarineBot HudMessages \n by Shrike !!", red, blue, gfx, time);

				else if (temp == 4)
					CustHudMessageToAll("Example of MarineBot HudMessages \n by Shrike !!", red, blue, gfx, time);

				else	// set 1 color and gfx manualy
					CustHudMessageToAll("Example of MarineBot HudMessages \n by Shrike !!", Vector(0, 250, 0), blue, 1, time); // green flashing
			//
			/////////////////////////////////////////////////
			}
		}
		else
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "no arg specified! -> testhudmsg arg, arg is number from 1-5\n");

		return TRUE;
	}
	else if (FStrEq(pcmd, "debug_menu"))
	{
		char str[80];
		sprintf(str, "menustate=%d menunextstate=%d menuteam=%d\n", g_menu_state,
			g_menu_next_state, g_menu_team);
		ClientPrint(pEntity, HUD_PRINTCONSOLE, str);

		return TRUE;
	}
	else if (FStrEq(pcmd, "debug_stuck") || FStrEq(pcmd, "debugstuck"))
	{
		if (botdebugger.IsDebugStuck() || FStrEq(arg1, "off"))
		{
			botdebugger.ResetDebugStuck();
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_stuck DISABLED!\n");
		}
		else
		{
			botdebugger.SetDebugStuck(true);
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_stuck ENABLED!\n");
		}

		return TRUE;
	}

	// TEMP: needed some cmd to test mod version
	else if (FStrEq(pcmd, "debug_vers") || FStrEq(pcmd, "testver"))
	{
		sprintf(msg, "CurrMod: %d | ModVersion: %d | IsSteam (bool): %d\n",
			mod_id, g_mod_version, is_steam);
		ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

		return TRUE;
	}
	//@@@@ kota@ for debug only
	else if (FStrEq(pcmd, "samurai"))
	{
		// is it already on so turn it off OR turn it off via "off" parameter
		if (FStrEq(arg1, "off"))
			externals.be_samurai = FALSE;
		// otherwise turn it on
		else
			externals.be_samurai = TRUE;

		if (externals.be_samurai)
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "samurai mode ENABLED\n");
		else
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "samurai mode DISABLED\n");

		return TRUE;
	}

	return FALSE;
}
#endif


// play sound msg confirmation
void PlaySoundConfirmation(edict_t* pEntity, int msg_type)
{
	if (msg_type == SND_DONE)
	{
		EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "plats/elevbell1.wav", 1.0, ATTN_NORM, 0, 100);
	}
	else if (msg_type == SND_FAILED)
	{
		EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "buttons/button10.wav", 1.0, ATTN_NORM, 0, 100);
	}

	return;
}

void MBMenuSystem(edict_t* pEntity, const char* arg1)
{
	int is_wpt_near = -1;

	if (num_waypoints < 1)
		is_wpt_near = -1;
	else
		is_wpt_near = WaypointFindNearest(pEntity, 50.0, -1);

	if (g_menu_state == MENU_MAIN)	// in main menu
	{
		if (FStrEq(arg1, "1"))
		{
			g_menu_state = MENU_1;		// switch to bot menu
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_1);

			return;
		}
		else if (FStrEq(arg1, "2"))
		{
			g_menu_state = MENU_2;		// switch to waypoint menu

			// if no close wpt
			if (is_wpt_near == -1)
				UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2a);
			else
				UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2);

			return;
		}
		else if (FStrEq(arg1, "3"))
		{
			g_menu_state = MENU_3;		// switch to misc menu
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_3);

			return;
		}
		else if ((FStrEq(arg1, "5")) || (FStrEq(arg1, "6")) || (FStrEq(arg1, "7")) ||
			(FStrEq(arg1, "8")) || (FStrEq(arg1, "9")) || (FStrEq(arg1, "0")))
		{
			g_menu_state = MENU_MAIN;		// stay in this menu
			UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_main);

			return;
		}
	}
	else if (g_menu_state == MENU_1)  // in bot menu
	{
		if (FStrEq(arg1, "1"))
		{
			BotCreate(pEntity, "1", nullptr, nullptr, nullptr, nullptr);
			botmanager.SetBotCheckTime(gpGlobals->time + 2.5);
		}
		else if (FStrEq(arg1, "2"))
		{
			BotCreate(pEntity, "2", nullptr, nullptr, nullptr, nullptr);
			botmanager.SetBotCheckTime(gpGlobals->time + 2.5);
		}
		else if (FStrEq(arg1, "3"))
		{
			botmanager.SetListeServerFilling(true);
			botmanager.SetBotCheckTime(gpGlobals->time + 0.5);
		}
		else if (FStrEq(arg1, "4"))
		{
			g_menu_state = MENU_99;			// switch to team select
			g_menu_next_state = 24;			// to know thats kicking

			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_99a);

			return;
		}
		else if (FStrEq(arg1, "5"))
		{
			g_menu_state = MENU_99;			// switch to team select
			g_menu_next_state = 25;			// to know thats killing

			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_99a);

			return;
		}
		else if (FStrEq(arg1, "6"))
		{
			botmanager.SetTeamsBalanceValue(UTIL_BalanceTeams());
		}
		else if ((FStrEq(arg1, "6")) || (FStrEq(arg1, "7")) || (FStrEq(arg1, "8")))
		{
			g_menu_state = MENU_1;		// stay in this menu
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_1);

			return;
		}
		else if (FStrEq(arg1, "9"))
		{
			g_menu_state = MENU_1_9;		// switch to settings menu
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_1_9);

			return;
		}
		else if (FStrEq(arg1, "0"))
		{
			g_menu_state = MENU_MAIN;		// switch to main menu
			UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_main);

			return;
		}
	}
	else if (g_menu_state == MENU_1_9)  // in settings menu
	{
		if (FStrEq(arg1, "1"))
		{
			g_menu_state = MENU_1_9_1;	// switch to skill levels menu
			g_menu_next_state = 91;		// to know thats default botskill

			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_1_9_1);

			return;
		}
		else if (FStrEq(arg1, "2"))
		{
			g_menu_state = MENU_1_9_1;	// switch to skill levels menu
			g_menu_next_state = 92;		// to know thats botskill (in game)

			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_1_9_1);

			return;
		}
		else if (FStrEq(arg1, "3"))
			UTIL_ChangeBotSkillLevel(TRUE, 1);
		else if (FStrEq(arg1, "4"))
			UTIL_ChangeBotSkillLevel(TRUE, -1);
		else if (FStrEq(arg1, "5"))
		{
			g_menu_state = MENU_1_9_1;	// switch to skill levels menu
			g_menu_next_state = 95;		// to know thats aimskill

			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_1_9_1);

			return;
		}
		else if (FStrEq(arg1, "6"))
		{
			g_menu_state = MENU_1_9_6;	// switch to reactions menu
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_1_9_6);

			return;
		}
		else if (FStrEq(arg1, "7"))
		{
			if (externals.GetRandomSkill())
				externals.SetRandomSkill(FALSE);
			else
				externals.SetRandomSkill(TRUE);
		}
		else if ((FStrEq(arg1, "8")))
		{
			if (externals.GetPassiveHealing())
				externals.SetPassiveHealing(FALSE);
			else
				externals.SetPassiveHealing(TRUE);
		}
		else if (FStrEq(arg1, "9"))
		{
			g_menu_state = MENU_1;		// switch back to bot menu
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_1);

			return;
		}
		else if (FStrEq(arg1, "0"))
		{
			g_menu_state = MENU_MAIN;		// switch to main menu
			UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_main);

			return;
		}
	}
	else if (g_menu_state == MENU_1_9_1)	// in skill menu
	{
		int skill_level = -1;

		if (FStrEq(arg1, "1"))
		{
			// is next step default botskill
			if (g_menu_next_state == 91)
				externals.SetSpawnSkill(1);
			// is next step botskill or aimskill
			else if ((g_menu_next_state == 92) || (g_menu_next_state == 95))
				skill_level = 1;
		}
		else if (FStrEq(arg1, "2"))
		{
			if (g_menu_next_state == 91)
				externals.SetSpawnSkill(2);
			else if ((g_menu_next_state == 92) || (g_menu_next_state == 95))
				skill_level = 2;
		}
		else if (FStrEq(arg1, "3"))
		{
			if (g_menu_next_state == 91)
				externals.SetSpawnSkill(3);
			else if ((g_menu_next_state == 92) || (g_menu_next_state == 95))
				skill_level = 3;
		}
		else if (FStrEq(arg1, "4"))
		{
			if (g_menu_next_state == 91)
				externals.SetSpawnSkill(4);
			else if ((g_menu_next_state == 92) || (g_menu_next_state == 95))
				skill_level = 4;
		}
		else if (FStrEq(arg1, "5"))
		{
			if (g_menu_next_state == 91)
				externals.SetSpawnSkill(5);
			else if ((g_menu_next_state == 92) || (g_menu_next_state == 95))
				skill_level = 5;
		}
		else if ((FStrEq(arg1, "6")) || (FStrEq(arg1, "7")) || (FStrEq(arg1, "8")))
		{
			g_menu_state = MENU_1_9_1;		// stay in this menu
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_1_9_1);

			return;
		}
		else if (FStrEq(arg1, "9"))
		{
			g_menu_state = MENU_1_9;		// switch back to settings menu
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_1_9);

			return;
		}
		else if (FStrEq(arg1, "0"))
		{
			g_menu_state = MENU_MAIN;		// switch to main menu
			UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_main);

			return;
		}

		// set the bot skill
		if ((skill_level != -1) && (g_menu_next_state == 92))
			UTIL_ChangeBotSkillLevel(FALSE, skill_level);
		// set the aim skill
		else if ((skill_level != -1) && (g_menu_next_state == 95))
			UTIL_ChangeAimSkillLevel(skill_level);
	}
	else if (g_menu_state == MENU_1_9_6)	// in reactions menu
	{
		if (FStrEq(arg1, "1"))
			externals.SetReactionTime(0.0);
		else if (FStrEq(arg1, "2"))
			externals.SetReactionTime(0.1);
		else if (FStrEq(arg1, "3"))
			externals.SetReactionTime(0.2);
		else if (FStrEq(arg1, "4"))
			externals.SetReactionTime(0.5);
		else if (FStrEq(arg1, "5"))
			externals.SetReactionTime(1.0);
		else if (FStrEq(arg1, "6"))
			externals.SetReactionTime(1.5);
		else if (FStrEq(arg1, "7"))
			externals.SetReactionTime(2.5);
		else if (FStrEq(arg1, "8"))
			externals.SetReactionTime(5.0);
		else if (FStrEq(arg1, "9"))
		{
			g_menu_state = MENU_1_9;		// switch back to settings
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_1_9);

			return;
		}
		else if (FStrEq(arg1, "0"))
		{
			g_menu_state = MENU_MAIN;		// switch to main menu
			UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_main);

			return;
		}
	}
	else if (g_menu_state == MENU_2)  // in waypoint menu
	{
		if (FStrEq(arg1, "1"))
		{
			g_menu_state = MENU_2_1AND2_P1;		// switch to wpt list menu
			g_menu_next_state = 128;		// to know that's adding

			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_1);

			return;
		}
		// only if there is some wpt near player
		else if ((FStrEq(arg1, "2")) && (is_wpt_near != -1))
		{
			g_menu_state = MENU_2_1AND2_P1;		// switch to wpt list menu
			g_menu_next_state = 129;		// to know that's changing

			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_2);

			return;
		}
		else if ((FStrEq(arg1, "3")) && (is_wpt_near != -1))
		{
			g_menu_state = MENU_99;		// switch to team select
			g_menu_next_state = MENU_2_3;	// then to priority menu

			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_99);

			return;
		}
		else if ((FStrEq(arg1, "4")) && (is_wpt_near != -1))
		{
			g_menu_state = MENU_99;		// switch to team select
			g_menu_next_state = MENU_2_4;	// then to time menu

			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_99);

			return;
		}
		else if ((FStrEq(arg1, "5")) && (is_wpt_near != -1))
		{
			g_menu_state = MENU_2_5;		// switch to range menu
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_5);

			return;
		}
		else if (FStrEq(arg1, "6"))
		{
			g_menu_state = MENU_2_6;		// switch to autowpt menu
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_6);

			return;
		}
		else if (FStrEq(arg1, "7"))
		{
			g_menu_state = MENU_2_7;		// switch to path menu

			if (is_wpt_near != -1)
				UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_7);
			else
				UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_7a);

			return;
		}
		else if (FStrEq(arg1, "8"))
		{
			g_menu_state = MENU_2;		// stay in this menu

			// if no close wpt
			if (is_wpt_near == -1)
				UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2a);
			else
				UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2);

			return;
		}
		else if (FStrEq(arg1, "9"))
		{
			g_menu_state = MENU_2_9;		// switch to service menu
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_9);

			return;
		}
		else if (FStrEq(arg1, "0"))
		{
			g_menu_state = MENU_MAIN;		// switch to main menu
			UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_main);

			return;
		}
	}
	else if (g_menu_state == MENU_2_1AND2_P1)	// in wpt list menu - 1st part
	{
		if (FStrEq(arg1, "1"))
		{
			// is it adding cmd
			if (g_menu_next_state == 128)
			{
				WptAdd(pEntity, "normal");
			}
			// is it changing cmd
			if (g_menu_next_state == 129)
			{
				WptFlagChange(pEntity, "normal");
			}
		}
		else if (FStrEq(arg1, "2"))
		{
			// is it adding cmd
			if (g_menu_next_state == 128)
			{
				WaypointDelete(pEntity);
			}
			// is it changing cmd
			if (g_menu_next_state == 129)
			{
				g_menu_state = MENU_2_1AND2_P1;		// stay in this menu
				UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_2);

				return;
			}
		}
		else if (FStrEq(arg1, "3"))
		{
			if (g_menu_next_state == 128)
			{
				WptAdd(pEntity, "aim");
			}
			if (g_menu_next_state == 129)
			{
				WptFlagChange(pEntity, "aim");
			}
		}
		else if (FStrEq(arg1, "4"))
		{
			if (g_menu_next_state == 128)
			{
				WptAdd(pEntity, "ammobox");
			}
			if (g_menu_next_state == 129)
			{
				WptFlagChange(pEntity, "ammobox");
			}
		}
		else if (FStrEq(arg1, "5"))
		{
			if (g_menu_next_state == 128)
			{
				WptAdd(pEntity, "cross");
			}
			if (g_menu_next_state == 129)
			{
				WptFlagChange(pEntity, "cross");
			}
		}
		else if (FStrEq(arg1, "6"))
		{
			if (g_menu_next_state == 128)
			{
				WptAdd(pEntity, "crouch");
			}
			if (g_menu_next_state == 129)
			{
				WptFlagChange(pEntity, "crouch");
			}
		}
		else if (FStrEq(arg1, "7"))
		{
			if (g_menu_next_state == 128)
			{
				WptAdd(pEntity, "door");
			}
			if (g_menu_next_state == 129)
			{
				WptFlagChange(pEntity, "door");
			}
		}
		else if (FStrEq(arg1, "8"))
		{
			if (g_menu_next_state == 128)
			{
				WptAdd(pEntity, "usedoor");
			}
			if (g_menu_next_state == 129)
			{
				WptFlagChange(pEntity, "usedoor");
			}
		}
		else if (FStrEq(arg1, "9"))
		{
			g_menu_state = MENU_2_1AND2_P2;		// switch to 2nd part
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_1and2_p2);

			return;
		}
		else if (FStrEq(arg1, "0"))
		{
			g_menu_state = MENU_MAIN;		// switch to main menu
			UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_main);

			return;
		}
	}
	else if (g_menu_state == MENU_2_1AND2_P2)	// in wpt list menu - 2nd part
	{
		if (FStrEq(arg1, "1"))
		{
			if (g_menu_next_state == 128)
			{
				WptAdd(pEntity, "goback");
			}
			if (g_menu_next_state == 129)
			{
				WptFlagChange(pEntity, "goback");
			}
		}
		else if (FStrEq(arg1, "2"))
		{
			if (g_menu_next_state == 128)
			{
				WptAdd(pEntity, "jump");
			}
			if (g_menu_next_state == 129)
			{
				WptFlagChange(pEntity, "jump");
			}
		}
		else if (FStrEq(arg1, "3"))
		{
			if (g_menu_next_state == 128)
			{
				WptAdd(pEntity, "duckjump");
			}
			if (g_menu_next_state == 129)
			{
				WptFlagChange(pEntity, "duckjump");
			}
		}
		else if (FStrEq(arg1, "4"))
		{
			if (g_menu_next_state == 128)
			{
				WptAdd(pEntity, "ladder");
			}
			if (g_menu_next_state == 129)
			{
				WptFlagChange(pEntity, "ladder");
			}
		}
		else if (FStrEq(arg1, "5"))
		{
			if (g_menu_next_state == 128)
			{
				WptAdd(pEntity, "parachute");
			}
			if (g_menu_next_state == 129)
			{
				WptFlagChange(pEntity, "parachute");
			}
		}
		else if (FStrEq(arg1, "6"))
		{
			if (g_menu_next_state == 128)
			{
				WptAdd(pEntity, "prone");
			}
			if (g_menu_next_state == 129)
			{
				WptFlagChange(pEntity, "prone");
			}
		}
		else if (FStrEq(arg1, "7"))
		{
			if (g_menu_next_state == 128)
			{
				WptAdd(pEntity, "shoot");
			}
			if (g_menu_next_state == 129)
			{
				WptFlagChange(pEntity, "shoot");
			}
		}
		else if (FStrEq(arg1, "8"))
		{
			if (g_menu_next_state == 128)
			{
				WptAdd(pEntity, "use");
			}
			if (g_menu_next_state == 129)
			{
				WptFlagChange(pEntity, "use");
			}
		}
		else if (FStrEq(arg1, "9"))
		{
			g_menu_state = MENU_2_1AND2_P3;		// switch to 3rd part
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_1and2_p3);

			return;
		}
		else if (FStrEq(arg1, "0"))
		{
			g_menu_state = MENU_MAIN;		// switch to main menu
			UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_main);

			return;
		}
	}
	else if (g_menu_state == MENU_2_1AND2_P3)
	{
		if (FStrEq(arg1, "1"))
		{
			if (g_menu_next_state == 128)
			{
				WptAdd(pEntity, "claymore");
			}
			if (g_menu_next_state == 129)
			{
				WptFlagChange(pEntity, "claymore");
			}
		}
		else if (FStrEq(arg1, "2"))
		{
			if (g_menu_next_state == 128)
			{
				WptAdd(pEntity, "sniper");
			}
			if (g_menu_next_state == 129)
			{
				WptFlagChange(pEntity, "sniper");
			}
		}
		else if (FStrEq(arg1, "3"))
		{
			if (g_menu_next_state == 128)
			{
				WptAdd(pEntity, "sprint");
			}
			if (g_menu_next_state == 129)
			{
				WptFlagChange(pEntity, "sprint");
			}
		}
		else if (FStrEq(arg1, "4"))
		{
			if (g_menu_next_state == 128)
			{
				WptAdd(pEntity, "flag");
			}
			if (g_menu_next_state == 129)
			{
				WptFlagChange(pEntity, "flag");
			}
		}
		else if (FStrEq(arg1, "5"))
		{
			if (g_menu_next_state == 128)
			{
				WptAdd(pEntity, "trigger");
			}
			if (g_menu_next_state == 129)
			{
				WptFlagChange(pEntity, "trigger");
			}
		}
		else if ((FStrEq(arg1, "6")) || (FStrEq(arg1, "7")) || (FStrEq(arg1, "8")))
		{
			g_menu_state = MENU_2_1AND2_P3;		// stay in this menu
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_1and2_p3);

			return;
		}
		else if (FStrEq(arg1, "9"))
		{
			g_menu_state = MENU_2_1AND2_P1;		// switch to 1st part

			if (g_menu_next_state == 128)
				UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_1);
			else if (g_menu_next_state == 129)
				UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_2);

			return;
		}
		else if (FStrEq(arg1, "0"))
		{
			g_menu_state = MENU_MAIN;		// switch to main menu
			UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_main);

			return;
		}
	}
	else if (g_menu_state == MENU_2_3)	// in priority menu
	{
		char* the_team;

		if (g_menu_team == TEAM_RED)
			the_team = "red";
		if (g_menu_team == TEAM_BLUE)
			the_team = "blue";

		if (FStrEq(arg1, "1"))
			WptPriorityChange(pEntity, "1", the_team);
		else if (FStrEq(arg1, "2"))
			WptPriorityChange(pEntity, "2", the_team);
		else if (FStrEq(arg1, "3"))
			WptPriorityChange(pEntity, "3", the_team);
		else if (FStrEq(arg1, "4"))
			WptPriorityChange(pEntity, "4", the_team);
		else if (FStrEq(arg1, "5"))
			WptPriorityChange(pEntity, "5", the_team);
		else if (FStrEq(arg1, "6"))
			WptPriorityChange(pEntity, "0", the_team);
		else if ((FStrEq(arg1, "7")) || (FStrEq(arg1, "8")))
		{
			g_menu_state = MENU_2_3;		// stay in this menu
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_3);

			return;
		}
		else if (FStrEq(arg1, "9"))
		{
			g_menu_state = MENU_99;		// switch back to team menu
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_99);

			return;
		}
		else if (FStrEq(arg1, "0"))
		{
			g_menu_state = MENU_MAIN;		// switch to main menu
			UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_main);

			return;
		}
	}
	else if (g_menu_state == MENU_2_4)	// in time menu
	{
		char* the_team;

		if (g_menu_team == TEAM_RED)
			the_team = "red";
		if (g_menu_team == TEAM_BLUE)
			the_team = "blue";

		if (FStrEq(arg1, "1"))
			WptTimeChange(pEntity, "1", the_team);
		else if (FStrEq(arg1, "2"))
			WptTimeChange(pEntity, "2", the_team);
		else if (FStrEq(arg1, "3"))
			WptTimeChange(pEntity, "3", the_team);
		else if (FStrEq(arg1, "4"))
			WptTimeChange(pEntity, "4", the_team);
		else if (FStrEq(arg1, "5"))
			WptTimeChange(pEntity, "5", the_team);
		else if (FStrEq(arg1, "6"))
			WptTimeChange(pEntity, "10", the_team);
		else if (FStrEq(arg1, "7"))
			WptTimeChange(pEntity, "20", the_team);
		else if (FStrEq(arg1, "8"))
			WptTimeChange(pEntity, "30", the_team);
		else if (FStrEq(arg1, "9"))
		{
			g_menu_state = MENU_99;		// switch back to team menu
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_99);

			return;
		}
		else if (FStrEq(arg1, "0"))
		{
			g_menu_state = MENU_MAIN;		// switch to main menu
			UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_main);

			return;
		}
	}
	else if (g_menu_state == MENU_2_5)	// in range menu
	{
		if (FStrEq(arg1, "1"))
			WptRangeChange(pEntity, "10");
		else if (FStrEq(arg1, "2"))
			WptRangeChange(pEntity, "20");
		else if (FStrEq(arg1, "3"))
			WptRangeChange(pEntity, "30");
		else if (FStrEq(arg1, "4"))
			WptRangeChange(pEntity, "40");
		else if (FStrEq(arg1, "5"))
			WptRangeChange(pEntity, "50");
		else if (FStrEq(arg1, "6"))
			WptRangeChange(pEntity, "75");
		else if (FStrEq(arg1, "7"))
			WptRangeChange(pEntity, "100");
		else if (FStrEq(arg1, "8"))
			WptRangeChange(pEntity, "125");
		else if (FStrEq(arg1, "9"))
			WptRangeChange(pEntity, "150");
		else if (FStrEq(arg1, "0"))
		{
			g_menu_state = MENU_MAIN;		// switch to main menu
			UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_main);

			return;
		}
	}
	else if (g_menu_state == MENU_2_6)	// in autowpt menu
	{
		if (FStrEq(arg1, "1"))
		{
			StartAutoWaypointg(TRUE);
		}
		else if (FStrEq(arg1, "2"))
		{
			StartAutoWaypointg(FALSE);
		}
		else if (FStrEq(arg1, "3"))
		{
			g_menu_state = MENU_2_6_3;		// switch to distance menu
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_6_3);

			return;
		}
		else if ((FStrEq(arg1, "4")) || (FStrEq(arg1, "5")) || (FStrEq(arg1, "6")) ||
			(FStrEq(arg1, "7")) || (FStrEq(arg1, "8")) || (FStrEq(arg1, "9")))
		{
			g_menu_state = MENU_2_6;		// stay in this menu
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_6);

			return;
		}
		else if (FStrEq(arg1, "0"))
		{
			g_menu_state = MENU_MAIN;		// switch to main menu
			UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_main);

			return;
		}
	}
	else if (g_menu_state == MENU_2_6_3)	// in autowpt distance menu
	{
		if (FStrEq(arg1, "1"))
			wptser.SetAutoWaypointingDistance(80);
		else if (FStrEq(arg1, "2"))
			wptser.SetAutoWaypointingDistance(100);
		else if (FStrEq(arg1, "3"))
			wptser.SetAutoWaypointingDistance(120);
		else if (FStrEq(arg1, "4"))
			wptser.SetAutoWaypointingDistance(160);
		else if (FStrEq(arg1, "5"))
			wptser.SetAutoWaypointingDistance(200);
		else if (FStrEq(arg1, "6"))
			wptser.SetAutoWaypointingDistance(280);
		else if (FStrEq(arg1, "7"))
			wptser.SetAutoWaypointingDistance(340);
		else if (FStrEq(arg1, "8"))
			wptser.SetAutoWaypointingDistance(400);
		else if (FStrEq(arg1, "9"))
		{
			g_menu_state = MENU_2_6;		// switch back to autowpt menu
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_6);

			return;
		}
		else if (FStrEq(arg1, "0"))
		{
			g_menu_state = MENU_MAIN;		// switch to main menu
			UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_main);

			return;
		}
	}
	else if (g_menu_state == MENU_2_7)	// in path menu
	{
		if (FStrEq(arg1, "1"))
		{
			if (wptser.IsShowPaths())
				wptser.ResetShowPaths();
			else
			{
				wptser.SetShowWaypoints(true);		// turn waypoints on
				wptser.SetShowPaths(true);			// turn paths on
			}
		}
		else if ((FStrEq(arg1, "2")) && (is_wpt_near != -1))	// only if there is any wpt
		{
			wptser.SetShowPaths(true);
			WaypointCreatePath(pEntity);
		}
		else if (FStrEq(arg1, "3"))
			WaypointFinishPath(pEntity);
		else if ((FStrEq(arg1, "4")) && (is_wpt_near != -1))
			WaypointPathContinue(pEntity, -1);
		else if ((FStrEq(arg1, "5")) && (is_wpt_near != -1))
			WaypointAddIntoPath(pEntity);
		else if ((FStrEq(arg1, "6")) && (is_wpt_near != -1))
			WaypointRemoveFromPath(pEntity, -1);
		else if ((FStrEq(arg1, "7")) && (is_wpt_near != -1))
			WaypointDeletePath(pEntity, -1);
		else if (FStrEq(arg1, "8"))
		{
			if (wptser.IsAutoAddToPath())
				wptser.ResetAutoAddToPath();
			else
				wptser.SetAutoAddToPath(true);
		}
		else if ((FStrEq(arg1, "9")) && (is_wpt_near != -1))
		{
			g_menu_state = MENU_2_7_9_P1;	// switch to path flags menu
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_7_9_p1);

			return;
		}
		else if (FStrEq(arg1, "0"))
		{
			g_menu_state = MENU_MAIN;		// switch to main menu
			UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_main);

			return;
		}
	}
	else if (g_menu_state == MENU_2_7_9_P1)	// in path flags menu - 1st part
	{
		if (FStrEq(arg1, "1"))
			WptPathChangeWay(pEntity, "one", -1);
		else if (FStrEq(arg1, "2"))
			WptPathChangeWay(pEntity, "two", -1);
		else if (FStrEq(arg1, "3"))
			WptPathChangeWay(pEntity, "patrol", -1);
		else if (FStrEq(arg1, "4"))
			WptPathChangeTeam(pEntity, "red", -1);
		else if (FStrEq(arg1, "5"))
			WptPathChangeTeam(pEntity, "blue", -1);
		else if (FStrEq(arg1, "6"))
			WptPathChangeTeam(pEntity, "both", -1);
		else if (FStrEq(arg1, "7"))
			WptPathChangeClass(pEntity, "sniper", -1);
		else if (FStrEq(arg1, "8"))
			WptPathChangeClass(pEntity, "mgunner", -1);
		else if (FStrEq(arg1, "9"))
		{
			g_menu_state = MENU_2_7_9_P2;		// switch 2nd part
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_7_9_p2);

			return;
		}
		else if (FStrEq(arg1, "0"))
		{
			g_menu_state = MENU_MAIN;		// switch to main menu
			UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_main);

			return;
		}
	}
	else if (g_menu_state == MENU_2_7_9_P2)	// in path flags menu - 2nd part
	{
		if (FStrEq(arg1, "1"))
			WptPathChangeClass(pEntity, "all", -1);
		else if (FStrEq(arg1, "2"))
			WptPathChangeMisc(pEntity, "avoidenemy", -1);
		else if (FStrEq(arg1, "3"))
			WptPathChangeMisc(pEntity, "ignoreenemy", -1);
		else if (FStrEq(arg1, "4"))
			WptPathChangeMisc(pEntity, "carryitem", -1);
		else if ((FStrEq(arg1, "5")) || (FStrEq(arg1, "6")) || (FStrEq(arg1, "7")) ||
			(FStrEq(arg1, "8")))
		{
			g_menu_state = MENU_2_7_9_P2;		// stay in this menu
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_7_9_p2);

			return;
		}
		else if (FStrEq(arg1, "9"))
		{
			g_menu_state = MENU_2_7_9_P1;		// switch 1st part
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_7_9_p1);

			return;
		}
		else if (FStrEq(arg1, "0"))
		{
			g_menu_state = MENU_MAIN;		// switch to main menu
			UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_main);

			return;
		}
	}
	else if (g_menu_state == MENU_2_9)	// in service menu
	{
		if (FStrEq(arg1, "1"))
		{
			if (wptser.IsShowWaypoints())
				wptser.ResetShowWaypoints();
			else
				wptser.SetShowWaypoints(true);
		}
		else if (FStrEq(arg1, "2"))
		{
			g_menu_state = MENU_2_9_2;	// switch to adv. debugging menu
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_9_2);

			return;
		}
		else if (FStrEq(arg1, "3"))
		{
			WaypointLoad(pEntity, nullptr);
			WaypointPathLoad(pEntity, nullptr);
		}
		else if (FStrEq(arg1, "4"))
		{
			WaypointSave(nullptr);

			if (num_w_paths > 0)
				WaypointPathSave(nullptr);
		}
		else if (FStrEq(arg1, "5"))
		{
			if (WaypointLoadUnsupported(pEntity))
			{
				if (WaypointPathLoadUnsupported(pEntity))
				{
					WaypointSave(nullptr);
					WaypointPathSave(nullptr);
				}
			}
		}
		else if (FStrEq(arg1, "6"))
			WaypointInit();
		else if (FStrEq(arg1, "7"))
			WaypointDestroy();
		else if (FStrEq(arg1, "8"))
		{
			g_menu_state = MENU_2_9_8;		// switch to folder select menu
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_9_8);

			return;
		}
		else if (FStrEq(arg1, "9"))
		{
			g_menu_state = MENU_2_9;		// stay in this menu
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_9);

			return;
		}
		else if (FStrEq(arg1, "0"))
		{
			g_menu_state = MENU_MAIN;		// switch to main menu
			UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_main);

			return;
		}
	}
	else if (g_menu_state == MENU_2_9_2)	// in adv. debugging menu
	{
		if (FStrEq(arg1, "1"))
		{
			if (wptser.IsCheckCross())
				wptser.ResetCheckCross();
			else
			{
				wptser.SetCheckCross(true);
				wptser.ResetCheckRanges();
			}
		}
		else if (FStrEq(arg1, "2"))
		{
			if (wptser.IsCheckAims())
				wptser.ResetCheckAims();
			else
			{
				wptser.SetCheckAims(true);
				wptser.ResetCheckRanges();
			}
		}
		else if (FStrEq(arg1, "3"))
		{
			if (wptser.IsCheckRanges())
				wptser.ResetCheckRanges();
			else
			{
				wptser.SetCheckRanges(true);
				wptser.ResetCheckAims();
				wptser.ResetCheckCross();
			}
		}
		else if (FStrEq(arg1, "4"))
		{
			g_menu_state = MENU_2_9_2_4;	// switch to path highlight menu
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_9_2_4);

			return;
		}
		else if ((FStrEq(arg1, "5")) || (FStrEq(arg1, "6")) || (FStrEq(arg1, "7")) ||
			(FStrEq(arg1, "8")))
		{
			g_menu_state = MENU_2_9_2;		// stay in this menu
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_9_2);

			return;
		}
		else if (FStrEq(arg1, "9"))
		{
			g_menu_state = MENU_2_9_2_9;		// switch to draw distance menu
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_9_2_9);

			return;
		}
		else if (FStrEq(arg1, "0"))
		{
			g_menu_state = MENU_MAIN;		// switch to main menu
			UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_main);

			return;
		}
	}
	else if (g_menu_state == MENU_2_9_2_4)	// in path highlighting menu
	{
		if (FStrEq(arg1, "1"))
			wptser.SetPathToHighlight(HIGHLIGHT_RED);		// red team paths only
		else if (FStrEq(arg1, "2"))
			wptser.SetPathToHighlight(HIGHLIGHT_BLUE);		// blue team paths only
		else if (FStrEq(arg1, "3"))
			wptser.SetPathToHighlight(HIGHLIGHT_ONEWAY);	// one-way paths only
		else if (FStrEq(arg1, "4"))
			wptser.SetPathToHighlight(HIGHLIGHT_SNIPER);	// sniper paths only
		else if (FStrEq(arg1, "5"))
			wptser.SetPathToHighlight(HIGHLIGHT_MGUNNER);	// machine gunner paths only
		else if ((FStrEq(arg1, "6")) || (FStrEq(arg1, "7")) || (FStrEq(arg1, "8")))
		{
			g_menu_state = MENU_2_9_2_4;		// stay in this menu
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_9_2_4);

			return;
		}
		else if (FStrEq(arg1, "9"))
			wptser.ResetPathToHighlight();		// turn it off i.e. all paths show again
		else if (FStrEq(arg1, "0"))
		{
			g_menu_state = MENU_MAIN;		// switch to main menu
			UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_main);

			return;
		}
	}
	else if (g_menu_state == MENU_2_9_2_9)	// in waypoints draw distance menu
	{
		if (FStrEq(arg1, "1"))
			wptser.SetWaypointsDrawDistance(400);
		else if (FStrEq(arg1, "2"))
			wptser.SetWaypointsDrawDistance(500);
		else if (FStrEq(arg1, "3"))
			wptser.SetWaypointsDrawDistance(600);
		else if (FStrEq(arg1, "4"))
			wptser.SetWaypointsDrawDistance(700);
		else if (FStrEq(arg1, "5"))
			wptser.SetWaypointsDrawDistance(800);
		else if (FStrEq(arg1, "6"))
			wptser.SetWaypointsDrawDistance(900);
		else if (FStrEq(arg1, "7"))
			wptser.SetWaypointsDrawDistance(1000);
		else if (FStrEq(arg1, "8"))
			wptser.SetWaypointsDrawDistance(1200);
		else if (FStrEq(arg1, "9"))
			wptser.SetWaypointsDrawDistance(1400);
		else if (FStrEq(arg1, "0"))
		{
			g_menu_state = MENU_MAIN;		// switch to main menu
			UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_main);

			return;
		}
	}
	else if (g_menu_state == MENU_2_9_8)	// in folder select menu
	{
		if (FStrEq(arg1, "1"))
			internals.ResetIsCustomWaypoints();
		else if (FStrEq(arg1, "2"))
			internals.SetIsCustomWaypoints(true);
		else if ((FStrEq(arg1, "3")) || (FStrEq(arg1, "4")) || (FStrEq(arg1, "5")) ||
			(FStrEq(arg1, "6")) || (FStrEq(arg1, "7")) || (FStrEq(arg1, "8")))
		{
			g_menu_state = MENU_2_9_8;		// stay in this menu
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_9_8);

			return;
		}
		else if (FStrEq(arg1, "9"))
		{
			g_menu_state = MENU_2_9;		// switch back to service menu
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_9);

			return;
		}
		else if (FStrEq(arg1, "0"))
		{
			g_menu_state = MENU_MAIN;		// switch to main menu
			UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_main);

			return;
		}
	}
	else if (g_menu_state == MENU_3)	// in misc menu
	{
		if (FStrEq(arg1, "1"))
		{
			if (botdebugger.IsObserverMode())
				botdebugger.ResetObserverMode();
			else
				botdebugger.SetObserverMode(true);
		}
		else if (FStrEq(arg1, "2"))
		{
			if (botdebugger.IsFreezeMode())
				botdebugger.ResetFreezeMode();
			else
				botdebugger.SetFreezeMode(true);
		}
		else if (FStrEq(arg1, "3"))
		{
			if (botdebugger.IsDontShoot())
				botdebugger.ResetDontShoot();
			else
				botdebugger.SetDontShoot(true);
		}
		else if (FStrEq(arg1, "4"))
		{
			if (botdebugger.IsIgnoreAll())
				botdebugger.ResetIgnoreAll();
			else
				botdebugger.SetIgnoreAll(true);
		}
		else if (FStrEq(arg1, "5"))
		{
			if (externals.GetDontSpeak())
				externals.SetDontSpeak(FALSE);
			else
				externals.SetDontSpeak(TRUE);
		}
		else if (FStrEq(arg1, "6"))
		{
			if ((pEntity->v.movetype == MOVETYPE_NOCLIP) || (FStrEq(arg1, "off")))
				pEntity->v.movetype = MOVETYPE_WALK;
			else
				pEntity->v.movetype = MOVETYPE_NOCLIP;
		}
		else if ((FStrEq(arg1, "7")) || (FStrEq(arg1, "8")) || (FStrEq(arg1, "9")))
		{
			g_menu_state = MENU_3;		// stay in this menu
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_3);

			return;
		}
		else if (FStrEq(arg1, "0"))
		{
			g_menu_state = MENU_MAIN;		// switch to main menu
			UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_main);

			return;
		}
	}
	else if (g_menu_state == MENU_99)		// in team select menu
	{
		if (FStrEq(arg1, "1"))
		{
			g_menu_team = TEAM_RED;

			// is next step kick cmd
			if (g_menu_next_state == 24)
				UTIL_KickBot(100 + g_menu_team);
			// is next step kill cmd
			else if (g_menu_next_state == 25)
				UTIL_KillBot(100 + g_menu_team);
			// is next step priority menu
			else if (g_menu_next_state == MENU_2_3)
			{
				g_menu_state = MENU_2_3;
				UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_3);

				return;
			}
			// is next step time menu
			else if (g_menu_next_state == MENU_2_4)
			{
				g_menu_state = MENU_2_4;
				UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_4);

				return;
			}
		}
		else if (FStrEq(arg1, "2"))
		{
			g_menu_team = TEAM_BLUE;

			// is next step kick cmd
			if (g_menu_next_state == 24)
				UTIL_KickBot(100 + g_menu_team);
			// is next step kill cmd
			else if (g_menu_next_state == 25)
				UTIL_KillBot(100 + g_menu_team);
			// is next step priority menu
			else if (g_menu_next_state == MENU_2_3)
			{
				g_menu_state = MENU_2_3;
				UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_3);

				return;
			}
			// is next step time menu
			else if (g_menu_next_state == MENU_2_4)
			{
				g_menu_state = MENU_2_4;
				UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_4);

				return;
			}
		}
		else if (FStrEq(arg1, "3"))
		{
			// is next step kick cmd
			if (g_menu_next_state == 24)
				UTIL_KickBot(100);
			// is next step kill cmd
			else if (g_menu_next_state == 25)
				UTIL_KillBot(100);
			// or is "3"-both teams not allowed
			else
			{
				g_menu_state = MENU_99;		// stay in this menu
				UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_99);

				return;
			}
		}
		else if ((FStrEq(arg1, "4")) || (FStrEq(arg1, "5")) || (FStrEq(arg1, "6")) ||
			(FStrEq(arg1, "7")) || (FStrEq(arg1, "8")) || (FStrEq(arg1, "9")))
		{
			g_menu_state = MENU_99;		// stay in this menu

			if ((g_menu_next_state == 24) || (g_menu_next_state == 25))
				UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_99a);
			else
				UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_99);

			return;
		}
		else if (FStrEq(arg1, "0"))
		{
			g_menu_state = MENU_MAIN;		// switch to main menu
			UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_main);

			return;
		}
	}

	// clear all menu values
	g_menu_state = MENU_NONE;
	g_menu_next_state = 0;
	g_menu_team = 0;

	return;
}


/*
* the following section is used for commands that are same
* for both dedicated server as well as listenserver (ie. LAN game)
*/

void RandomSkillCommand(edict_t* pEntity, const char* arg1)
{
	// no argument specified then toggle between on and off
	// we have to check the length, because checking for nullity doesn't seems to work
	if (strlen(arg1) < 1)
	{
		// not dedictaed server then play a sound confirmation
		// and tell the user that he used toggle mode
		if (!is_dedicated_server)
		{
			PlaySoundConfirmation(pEntity, SND_DONE);
			PrintOutput(pEntity, "***toggle mode used***\n", MType::msg_null);
		}

		if (externals.GetRandomSkill())
		{
			externals.SetRandomSkill(FALSE);
			PrintOutput(pEntity, "random_skill mode DISABLED\n", MType::msg_default);
		}
		else
		{
			externals.SetRandomSkill(TRUE);
			PrintOutput(pEntity, "random_skill mode ENABLED\n", MType::msg_default);
		}
	}
	else
	{
		// if there's some argument and the argument do match current then print current state
		if (strcmp(arg1, "current") == 0)
		{
			if (externals.GetRandomSkill())
				PrintOutput(pEntity, "random_skill mode is currently ENABLED\n", MType::msg_default);
			else
				PrintOutput(pEntity, "random_skill mode is currently DISABLED\n", MType::msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		// do we need to turn this on?
		else if (strcmp(arg1, "on") == 0)
		{
			externals.SetRandomSkill(TRUE);
			PrintOutput(pEntity, "random_skill mode ENABLED\n", MType::msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		// do we need to turn this off?
		else if (strcmp(arg1, "off") == 0)
		{
			externals.SetRandomSkill(FALSE);
			PrintOutput(pEntity, "random_skill mode DISABLED\n", MType::msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		// otherwise the user must have used wrong/invalid argument
		else
		{
			PrintOutput(pEntity, "invalid argument!\n", MType::msg_error);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_FAILED);
		}
	}
}

void SpawnSkillCommand(edict_t* pEntity, const char* arg1)
{
	if (strlen(arg1) >= 1)
	{
		const int temp = atoi(arg1);

		if ((temp >= 1) && (temp <= BOT_SKILL_LEVELS))
		{
			externals.SetSpawnSkill(temp);

			char msg[80];
			sprintf(msg, "default spawnskill is %d\n", externals.GetSpawnSkill());
			PrintOutput(pEntity, msg, MType::msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else
		{
			PrintOutput(pEntity, "invalid spawnskill value!\n", MType::msg_error);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_FAILED);
		}
	}
	else
	{
		PrintOutput(pEntity, "missing argument!\n", MType::msg_error);

		if (!is_dedicated_server)
			PlaySoundConfirmation(pEntity, SND_FAILED);
	}
}

void SetBotSkillCommand(edict_t* pEntity, const char* arg1)
{
	if (strlen(arg1) >= 1)
	{
		const int temp = atoi(arg1);

		if ((temp >= 1) && (temp <= BOT_SKILL_LEVELS))
		{
			const int result = UTIL_ChangeBotSkillLevel(FALSE, temp);

			char msg[80];
			sprintf(msg, "botskill set to %d\n", result);
			PrintOutput(pEntity, msg, MType::msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else
		{
			PrintOutput(pEntity, "invalid botskill value!\n", MType::msg_error);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_FAILED);
		}
	}
	else
	{
		PrintOutput(pEntity, "missing argument!\n", MType::msg_error);

		if (!is_dedicated_server)
			PlaySoundConfirmation(pEntity, SND_FAILED);
	}
}

void BotSkillUpCommand(edict_t* pEntity, const char* arg1)
{
	if (UTIL_ChangeBotSkillLevel(TRUE, 1) > 0)
	{
		PrintOutput(pEntity, "botskill increased by one level\n", MType::msg_default);

		if (!is_dedicated_server)
			PlaySoundConfirmation(pEntity, SND_DONE);
	}
	else
	{
		PrintOutput(pEntity, "no bot on lower skill levels or no bot in game!\n", MType::msg_error);

		if (!is_dedicated_server)
			PlaySoundConfirmation(pEntity, SND_FAILED);
	}
}

void BotSkillDownCommand(edict_t* pEntity, const char* arg1)
{
	if (UTIL_ChangeBotSkillLevel(TRUE, -1) > 0)
	{
		PrintOutput(pEntity, "botskill decreased by one level\n", MType::msg_default);

		if (!is_dedicated_server)
			PlaySoundConfirmation(pEntity, SND_DONE);
	}
	else
	{
		PrintOutput(pEntity, "no bot on higher skill levels or no bot in game!\n", MType::msg_error);

		if (!is_dedicated_server)
			PlaySoundConfirmation(pEntity, SND_FAILED);
	}
}

void SetAimSkillCommand(edict_t* pEntity, const char* arg1)
{
	if (strlen(arg1) >= 1)
	{
		const int temp = atoi(arg1);

		if ((temp >= 1) && (temp <= BOT_SKILL_LEVELS))
		{
			const int result = UTIL_ChangeAimSkillLevel(temp);

			char msg[80];
			sprintf(msg, "aimskill set to %d\n", result);
			PrintOutput(pEntity, msg, MType::msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else
		{
			PrintOutput(pEntity, "invalid aimskill value!\n", MType::msg_error);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_FAILED);
		}
	}
	else
	{
		PrintOutput(pEntity, "missing argument!\n", MType::msg_error);

		if (!is_dedicated_server)
			PlaySoundConfirmation(pEntity, SND_FAILED);
	}
}

void SetReactionTimeCommand(edict_t* pEntity, const char* arg1)
{
	if (strlen(arg1) >= 1)
	{
		float temp = atoi(arg1);

		// first of all check if the argument is a digit
		if (isdigit(arg1[0]) && (temp >= 0) && (temp <= 50))
		{
			temp /= 10;
			externals.SetReactionTime((float)temp);

			char msg[80];
			sprintf(msg, "reaction_time is %.1fs\n", externals.GetReactionTime());
			PrintOutput(pEntity, msg, MType::msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else if (FStrEq(arg1, "current"))
		{
			char msg[80];
			sprintf(msg, "reaction_time is %.1fs\n", externals.GetReactionTime());
			PrintOutput(pEntity, msg, MType::msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else
		{
			PrintOutput(pEntity, "invalid reaction_time value!\n", MType::msg_error);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_FAILED);
		}
	}
	else
	{
		PrintOutput(pEntity, "missing argument!\n", MType::msg_error);

		if (!is_dedicated_server)
			PlaySoundConfirmation(pEntity, SND_FAILED);
	}
}

void RangeLimitCommand(edict_t* pEntity, const char* arg1)
{
	if (strlen(arg1) >= 1)
	{
		const float temp = atoi(arg1);

		if (isdigit(arg1[0]) && (temp >= 500) && (temp <= 7500))
		{
			internals.SetEnemyDistanceLimit((float)temp);
			internals.SetIsEnemyDistanceLimit(true);

			char msg[80];
			sprintf(msg, "range_limit is %.0f\n", internals.GetEnemyDistanceLimit());
			PrintOutput(pEntity, msg, MType::msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else if (FStrEq(arg1, "default"))
		{
			internals.ResetEnemyDistanceLimit();
			internals.ResetIsEnemyDistanceLimit();

			char msg[80];
			sprintf(msg, "range_limit is set to default value (%.0f)\n", internals.GetEnemyDistanceLimit());
			PrintOutput(pEntity, msg, MType::msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else if (FStrEq(arg1, "current"))
		{
			char msg[80];
			sprintf(msg, "range_limit is %.0f\n", internals.GetEnemyDistanceLimit());
			PrintOutput(pEntity, msg, MType::msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else if ((FStrEq(arg1, "help")) || (FStrEq(arg1, "?")))
		{
			PrintOutput(pEntity, "All valid range_limit options\n", MType::msg_null);
			PrintOutput(pEntity, "[range_limit <number>] the number can be from 500 to 7500\n", MType::msg_null);
			PrintOutput(pEntity, "[range_limit <default>] sets it to default (ie. to 7500)\n", MType::msg_null);
			PrintOutput(pEntity, "[range_limit <current>] returns current value\n", MType::msg_null);
			PrintOutput(pEntity, "[range_limit <help>] shows this help\n", MType::msg_null);
		}
		else
		{
			PrintOutput(pEntity, "invalid range_limit value!\n", MType::msg_error);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_FAILED);
		}
	}
	else
	{
		PrintOutput(pEntity, "missing argument!\n", MType::msg_error);

		if (!is_dedicated_server)
			PlaySoundConfirmation(pEntity, SND_FAILED);
	}
}

void SetWaypointDirectoryCommand(edict_t* pEntity, const char* arg1)
{
	if (strlen(arg1) >= 1)
	{
		if ((FStrEq(arg1, "help")) || (FStrEq(arg1, "?")))
		{
			PrintOutput(pEntity, "All valid directory options\n", MType::msg_null);
			PrintOutput(pEntity, "[directory <defaultwpts>] points to \"/marine_bot/defaultwpts\" directory\n", MType::msg_null);
			PrintOutput(pEntity, "[directory <customwpts>] points to \"/marine_bot/customwpts\" directory\n", MType::msg_null);
			PrintOutput(pEntity, "[directory <current>] returns which directory is used now\n", MType::msg_null);
			PrintOutput(pEntity, "[directory <help>] shows this help\n", MType::msg_null);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else if (FStrEq(arg1, "customwpts"))
		{
			internals.SetIsCustomWaypoints(true);
			PrintOutput(pEntity, "waypoint are now loaded from \"customwpts\" directory\n", MType::msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else if (FStrEq(arg1, "defaultwpts"))
		{
			internals.ResetIsCustomWaypoints();
			PrintOutput(pEntity, "waypoint are now loaded from \"defaultwpts\" directory\n", MType::msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else if (FStrEq(arg1, "current"))
		{
			if (internals.IsCustomWaypoints())
				PrintOutput(pEntity, "waypoint are now loaded from \"customwpts\" directory\n", MType::msg_default);
			else
				PrintOutput(pEntity, "waypoint are now loaded from \"defaultwpts\" directory\n", MType::msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else
		{
			PrintOutput(pEntity, "invalid argument!\n", MType::msg_error);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_FAILED);
		}
	}
	else
	{
		if (!is_dedicated_server)
		{
			PlaySoundConfirmation(pEntity, SND_DONE);
			PrintOutput(pEntity, "***toggle mode used***\n", MType::msg_null);
		}

		if (internals.IsCustomWaypoints())
		{
			internals.ResetIsCustomWaypoints();
			PrintOutput(pEntity, "waypoint are now loaded from \"defaultwpts\" directory\n", MType::msg_default);
		}
		else
		{
			internals.SetIsCustomWaypoints(true);
			PrintOutput(pEntity, "waypoint are now loaded from \"customwpts\" directory\n", MType::msg_default);
		}
	}
}

void PassiveHealingCommand(edict_t* pEntity, const char* arg1)
{
	if (strlen(arg1) < 1)
	{
		if (!is_dedicated_server)
		{
			PlaySoundConfirmation(pEntity, SND_DONE);
			PrintOutput(pEntity, "***toggle mode used***\n", MType::msg_null);
		}

		if (externals.GetPassiveHealing())
		{
			externals.SetPassiveHealing(FALSE);
			PrintOutput(pEntity, "passive_healing mode DISABLED\n", MType::msg_default);
		}
		else
		{
			externals.SetPassiveHealing(TRUE);
			PrintOutput(pEntity, "passive_healing mode ENABLED\n", MType::msg_default);
		}
	}
	else
	{
		if (strcmp(arg1, "current") == 0)
		{
			if (externals.GetPassiveHealing())
				PrintOutput(pEntity, "passive_healing mode is currently ENABLED\n", MType::msg_default);
			else
				PrintOutput(pEntity, "passive_healing mode is currently DISABLED\n", MType::msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else if (strcmp(arg1, "on") == 0)
		{
			externals.SetPassiveHealing(TRUE);
			PrintOutput(pEntity, "passive_healing mode ENABLED\n", MType::msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else if (strcmp(arg1, "off") == 0)
		{
			externals.SetPassiveHealing(FALSE);
			PrintOutput(pEntity, "passive_healing mode DISABLED\n", MType::msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else
		{
			PrintOutput(pEntity, "invalid argument!\n", MType::msg_error);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_FAILED);
		}
	}
}

void DontSpeakCommand(edict_t* pEntity, const char* arg1)
{
	if (strlen(arg1) < 1)
	{
		if (!is_dedicated_server)
		{
			PlaySoundConfirmation(pEntity, SND_DONE);
			PrintOutput(pEntity, "***toggle mode used***\n", MType::msg_null);
		}

		if (externals.GetDontSpeak())
		{
			externals.SetDontSpeak(FALSE);
			PrintOutput(pEntity, "dont_speak mode DISABLED\n", MType::msg_default);
		}
		else
		{
			externals.SetDontSpeak(TRUE);
			PrintOutput(pEntity, "dont_speak mode ENABLED\n", MType::msg_default);
		}
	}
	else
	{
		if (strcmp(arg1, "current") == 0)
		{
			if (externals.GetDontSpeak())
				PrintOutput(pEntity, "dont_speak mode is currently ENABLED\n", MType::msg_default);
			else
				PrintOutput(pEntity, "dont_speak mode is currently DISABLED\n", MType::msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else if (strcmp(arg1, "on") == 0)
		{
			externals.SetDontSpeak(TRUE);
			PrintOutput(pEntity, "dont_speak mode ENABLED\n", MType::msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else if (strcmp(arg1, "off") == 0)
		{
			externals.SetDontSpeak(FALSE);
			PrintOutput(pEntity, "dont_speak mode DISABLED\n", MType::msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else
		{
			PrintOutput(pEntity, "invalid argument!\n", MType::msg_error);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_FAILED);
		}

	}
}

void DontChatCommand(edict_t* pEntity, const char* arg1)
{
	if (strlen(arg1) < 1)
	{
		if (!is_dedicated_server)
		{
			PlaySoundConfirmation(pEntity, SND_DONE);
			PrintOutput(pEntity, "***toggle mode used***\n", MType::msg_null);
		}

		if (externals.GetDontChat())
		{
			externals.SetDontChat(FALSE);
			PrintOutput(pEntity, "dont_chat mode DISABLED\n", MType::msg_default);
		}
		else
		{
			externals.SetDontChat(TRUE);
			PrintOutput(pEntity, "dont_chat mode ENABLED\n", MType::msg_default);
		}
	}
	else
	{
		if (strcmp(arg1, "current") == 0)
		{
			if (externals.GetDontChat())
				PrintOutput(pEntity, "dont_chat mode is currently ENABLED\n", MType::msg_default);
			else
				PrintOutput(pEntity, "dont_chat mode is currently DISABLED\n", MType::msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else if (strcmp(arg1, "on") == 0)
		{
			externals.SetDontChat(TRUE);
			PrintOutput(pEntity, "dont_chat mode ENABLED\n", MType::msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else if (strcmp(arg1, "off") == 0)
		{
			externals.SetDontChat(FALSE);
			PrintOutput(pEntity, "dont_chat mode DISABLED\n", MType::msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else
		{
			PrintOutput(pEntity, "invalid argument!\n", MType::msg_error);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_FAILED);
		}
	}
}
