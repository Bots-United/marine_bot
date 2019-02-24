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
// (http://www.marinebot.tk)
//
//
// dll.cpp
// 
////////////////////////////////////////////////////////////////////////////////////////////////

#include "defines.h"

#include "extdll.h"
#include "enginecallback.h"
#include "util.h"
#include "cbase.h"
#include "entity_state.h"

// for new config system and map learning by Andrey Kotrekhov
#include "Config.h"
#include "PathMap.h"
using std::string;

#include "bot.h"
#include "bot_func.h"
#include "waypoint.h"


#include "bot_weapons.h"		// REMOVE THIS ONLY FOR TESTS - "tgr" command

Section *conf_weapons = NULL;

extern "C"
{
#include <stdio.h>
};
#include "bot_navigate.h"

const int SERVER_CMD_LEN = 80;

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


extern GETENTITYAPI other_GetEntityAPI;
extern GETNEWDLLFUNCTIONS other_GetNewDLLFunctions;
extern enginefuncs_t g_engfuncs;
extern int debug_engine;
extern globalvars_t  *gpGlobals;
extern char *g_argv;
extern bool g_waypoint_on;
extern float g_draw_distance;
extern bool g_auto_waypoint;
extern float g_auto_wpt_distance;
extern bool g_path_waypoint_on;
extern bool g_auto_path;

extern float wp_display_time[MAX_WAYPOINTS];
extern bool b_observer_mode;
extern bool b_freeze_mode;
extern bool b_botdontshoot;
extern bool b_botignoreall;
extern bool b_custom_wpts;
extern bool debug_aim_targets;
extern bool debug_actions;
extern bool debug_cross;
extern bool debug_stuck;
extern bool debug_waypoints;
extern bool debug_weapons;
extern bool debug_paths;

extern char wpt_author[32];
extern char wpt_modified[32];

extern botname_t bot_names[MAX_BOT_NAMES];	// array of all names read from external file

bot_weapon_select_t bot_weapon_select[MAX_WEAPONS];	// array of all weapons the bot can use
bot_fire_delay_t bot_fire_delay[MAX_WEAPONS];		// their delays between two shots

static FILE *fp;

DLL_FUNCTIONS other_gFunctionTable;
DLL_GLOBAL const Vector g_vecZero = Vector(0,0,0);

bool is_dedicated_server = FALSE;	// TRUE if the server is a dedicated server
bool Dedicated_Server_Init = FALSE;		// ensures only one run of DS init

externals_t externals;
internals_t internals;

int client_t::humans_num;
int client_t::bots_num;
client_t clients[MAX_CLIENTS];

int m_spriteTexture = 0;
int m_spriteTexturePath1 = 0;
int m_spriteTexturePath2 = 0;
int m_spriteTexturePath3 = 0;
float f_update_wpt_time;		// allow us to update waypoints in real-time based on latest game events or changes made by the waypointer
int isFakeClientCommand = 0;
int fake_arg_count;
float bot_check_time = 30.0;
int num_bots = 0;
int prev_num_bots = 0;
bool override_max_bots = FALSE;
bool g_GameRules = FALSE;
edict_t *listenserver_edict = NULL;
edict_t *pRecipient = NULL;		// the one who write the ClintCommand
edict_t *g_debug_bot = NULL;	// pointer on a single bot we want to debug -- NOT FULLY FINISHED YET
bool g_debug_bot_on = FALSE;	// do we debug a single bot?

bool server_filling = FALSE;	// for auto filling of the ListenServer
int  bots_to_add = 0;			// amount of bots we need to fill (ie. "arg filling")

bool b_wpt_autosave = FALSE;		// allow waypoints auto saving feature
float wpt_autosave_delay = 90.0;	// the delay between two auto save calls
float wpt_autosave_time = 0.0;		// holds time of last autosave

bool check_aims  = FALSE;	// connect "waittimed" wpt with it's aim wpts
bool check_cross = FALSE;	// connect cross wpt with wpt in "effective/search" range
bool check_ranges = FALSE;	// highlights a wpt range
int highlight_this_path = -1;	// highlights (show) only single path (stores its index)

float welcome_time = 0.0;
bool welcome_sent = FALSE;
bool welcome2_sent = FALSE;
bool welcome3_sent = FALSE;		// special that shows waypoint's authors
char welcome_msg[] = "reporting for duty!\nWrite \"help\" or \"?\" into console to show console help";
char welcome2_msg[] = "Visit our web page at:\nhttp://www.marinebot.xf.cz";

float error_time = 0.0;
bool error_occured = FALSE;		// TRUE when there is fatal error
bool warning_event = FALSE;		// TRUE when there is some non-fatal error
bool override_reset = FALSE;	// to prevent resetting both flags (eg. there is error in DLLInit and we need to print it)
char error_msg[1024];
float HUDmsg_time = 0.0;		// to prevent overloading when displaying waypointing info on HUD

#ifdef _DEBUG
// TEMP: only for team test then remove it
bool debug_user = FALSE;
bool debug_sound = FALSE;
bool debug_usr_weapon = FALSE;
bot_current_weapon_t usr_current_weapon;
bool test_origin = FALSE;

bool  deathfall_check = FALSE;
float deathfallcheck_time = 0.0;
#endif


int g_menu_state = 0;		// position in menu (which menu is curr shown)
int g_menu_team;			// for team based options
int g_menu_next_state;		// in cases that one step between curr and next is needed

float is_team_play = 0.0;
float skill_system = 1.0;	// how many skills are allowed at the spawn (now one skill)
bool checked_teamplay = FALSE;
bool checked_skillsystem = FALSE;			// did we checked skill system?
//edict_t *pent_info_firearms_detect = NULL;	// NOT USED

//edict_t *pent_item_tfgoal = NULL;		// should be useful (need rename :D)
//int team_class_limits[4];  // WAS for TFC & should be useful

Section *conf = NULL;		// for new config system by Andrey Kotrekhov
bool read_whole_cfg = TRUE;	// allows to read the whole configuration file after map change
bool using_default_cfg = TRUE;	// is FALSE when we changed to some map specific .cfg
bool need_to_open_cfg = TRUE;
float bot_cfg_pause_time = 0.0;
float respawn_time = 0.0;
bool spawn_time_reset = FALSE;


int team_balance_value = 0;		// keeps teams balanced: <=0-teams balanced, >0&&<100-blue balance value, >100-red balance value
bool do_auto_balance = FALSE;	// by kota@
bool override_auto_balance = FALSE;	// temporary disables autobalancing when reins of one team reaches 0
float check_auto_balance = 0.0;	// auto balance checks
float check_send_info = 0.0;	// send message checks
float presentation_time = 0.0;				// holds the time of last presentation
char presentation_msg[] = "This server runs Marine Bot in version";

#ifdef NOFAMAS
char mb_version_info[32] = "0.93b_noFamas";
#else
char mb_version_info[32] = "0.93b-[APG]";		// holds MarineBot version string
#endif

char bot_whine[MAX_BOT_WHINE][81];
int whine_count;
int recent_bot_whine[5];

// main menu
char *show_menu_main =
   {"MARINE_BOT main menu\n\n1. Bot Menu\n2. Waypoint Menu\n3. Misc\n4. CANCEL"};
// bot menu & all (possible) submenus
char *show_menu_1 =
   {"MARINE_BOT bot menu\n\n1. Add red marine\n2. Add blue marine\n3. Fill server\n4. Kick marine\n5. Kill marine\n6. Balance teams\n\n\n9. Settings\n0. MAIN"};
char *show_menu_1_9 =
   {"MARINE_BOT bot settings menu\n\n1. SpawnSkill\n2. BotSkill (bots in game)\n3. BotSkillUp\n4. BotSkillDown\n5. AimSkill\n6. Reactions\n7. Random SpawnSkill\n8. PassiveHealing\n9. BACK\n0. MAIN"};
char *show_menu_1_9_1 =
   {"MARINE_BOT skill levels menu\n\n1. level 1(best)\n2. level 2\n3. level 3(default)\n4. level 4\n5. level 5(worst)\n\n\n\n9. BACK\n0. MAIN"};
char *show_menu_1_9_6 =
   {"MARINE_BOT bot reactions menu\n\n1. 0.0 s(best)\n2. 0.1 s\n3. 0.2 s(default)\n4. 0.5 s\n5. 1.0 s\n6. 1.5 s\n7. 2.5 s\n8. 5.0 s(worst)\n9. BACK\n0. MAIN"};
// waypoint menu & all (possible) submenus
char *show_menu_2 =
   {"MARINE_BOT waypoint menu\n\n1. Placing\n2. Changing\n3. Priority\n4. Time\n5. Range\n6. Autowaypoint\n7. SettingPaths\n\n9. Service\n0. MAIN"};
char *show_menu_2a =
   {"MARINE_BOT waypoint menu\n\n1. Placing\n2. unavailable\n3. unavailable\n4. unavailable\n5. unavailable\n6. Autowaypoint\n7. SettingPaths\n\n9. Service\n0. MAIN"};
char *show_menu_2_1 =
   {"MARINE_BOT waypoint menu\n\n1. Normal/Std\n2. Delete\n3. Aim\n4. Ammobox\n5. Cross\n6. Crouch\n7. Door\n8. UseDoor\n9. NEXT\n0. MAIN"};
char *show_menu_2_2 =
   {"MARINE_BOT waypoint menu\n\n1. Normal/Std\n\n3. Aim\n4. Ammobox\n5. Cross\n6. Crouch\n7. Door\n8. UseDoor\n9. NEXT\n0. MAIN"};
char *show_menu_2_1and2_p2 =
   {"MARINE_BOT waypoint menu\n\n1. GoBack\n2. Jump\n3. DuckJump\n4. Ladder\n5. Parachute\n6. Prone\n7. Shoot\n8. Use\n9. NEXT\n0. MAIN"};
char *show_menu_2_1and2_p3 =
   {"MARINE_BOT waypoint menu\n\n1. Claymore\n2. Sniper\n3. Sprint \n4. PushPoint/Flag \n5. Trigger \n \n \n \n9. BACKTOTOP\n0. MAIN"};
char *show_menu_2_3 =
   {"MARINE_BOT priority menu\n\n1. level 1(highest)\n2. level 2\n3. level 3\n4. level 4\n5. level 5(lowest/default)\n6. level 0(no)\n\n\n9. BACK\n0. MAIN"};
char *show_menu_2_4 =
   {"MARINE_BOT time menu\n\n1. 1 second\n2. 2 seconds\n3. 3 seconds\n4. 4 seconds\n5. 5 seconds\n6. 10 seconds\n7. 20 seconds\n8. 30 seconds\n9. BACK\n0. MAIN"};
char *show_menu_2_5 =
   {"MARINE_BOT range menu\n\n1. 10 units\n2. 20 units\n3. 30 units\n4. 40 units\n5. 50 units\n6. 75 units\n7. 100 units\n8. 125 units\n9. 150 units\n0. MAIN"};
char *show_menu_2_6 =
   {"MARINE_BOT autowaypoint menu\n\n1. Start autowaypoint\n2. Stop autowaypoint\n3. Set distance\n\n\n\n\n\n\n0. MAIN"};
char *show_menu_2_6_3 =
   {"MARINE_BOT distance menu\n\n1. 80 units(minimum)\n2. 100 units\n3. 120 units\n4. 160 units\n5. 200 units(default)\n6. 280 units\n7. 340 units\n8. 400 units(maximum)\n9. BACK\n0. MAIN"};
char *show_menu_2_7 =
   {"MARINE_BOT path menu\n\n1. Show\\Hide paths\n2. Start path\n3. Stop path\n4. Continue path\n5. Add to\n6. Remove from\n7. Delete path\n8. AutoAdding on\\off\n9. Path flags\n0. MAIN"};
char *show_menu_2_7a =
   {"MARINE_BOT path menu\n\n1. Show\\Hide paths\n2. unavailable\n3. Stop path\n4. Continue path\n5. unavailable\n6. unavailable\n7. unavailable\n8. AutoAdding on\\off\n9. unavailable\n0. MAIN"};
char *show_menu_2_7_9_p1 =
   {"MARINE_BOT path flags menu\n\n1. One-way\n2. Two-way\n3. Patrol type\n4. Red team\n5. Blue team\n6. Both teams\n7. Snipers only\n8. MGunners only\n9. NEXT\n0. MAIN"};
char *show_menu_2_7_9_p2 =
   {"MARINE_BOT path flags menu\n\n1. All classes\n2. Avoid Enemy\n3. Ignore Enemy\n4. Carry Item\n\n\n\n\n9. BACK\n0. MAIN"};
char *show_menu_2_9 =
   {"MARINE_BOT service menu\n\n1. Show\\Hide wpts\n2. Adv. debugging\n3. Load wpts&paths\n4. Save wpts&paths\n5. Convert older wpts\n6. Clear wpts\n7. Destroy wpts\n8. Change wpt folder\n\n0. MAIN"};
char *show_menu_2_9_2 =
   {"MARINE_BOT debugging menu\n\n1. Check cross\n2. Check aim\n3. Check range\n4. Path highlight\n\n\n\n\n9. Draw distance\n0. MAIN"};
char *show_menu_2_9_2_4 =
   {"MARINE_BOT path highlight menu\n\n1. Red team\n2. Blue team\n3. One-way path\n\n\n\n\n\n9. Turn off\n0. MAIN"};
char *show_menu_2_9_2_9 =
   {"MARINE_BOT draw distance menu\n\n1. 400 units\n2. 500 units\n3. 600 units\n4. 700 units\n5. 800 units(default)\n6. 900 units\n7. 1000 units\n8. 1200 units\n9. 1400 units(maximum)\n0. MAIN"};
char *show_menu_2_9_8 =
   {"MARINE_BOT folder select menu\n\n1. Default\n2. Custom\n\n\n\n\n\n\n9. BACK\n0. MAIN"};
// misc menu & all (possible) submenus
char *show_menu_3 =
   {"MARINE_BOT misc menu\n\n1. Observer\n2. MrFreeze\n3. BotDontShoot\n4. BotIgnoreAll\n5. BotDontSpeak\n6. MBNoClip\n\n\n\n0. MAIN"};
// special case - team menu
char *show_menu_99 =
   {"MARINE_BOT team select menu\n\n1. Red team\n2. Blue team\n\n\n\n\n\n\n\n0. MAIN"};
char *show_menu_99a =
   {"MARINE_BOT team select menu\n\n1. Red team\n2. Blue team\n3. Both teams\n\n\n\n\n\n\n0. MAIN"};


// few function prototypes used in this file
void GameDLLInit(void);
void UpdateClientData(const struct edict_s *ent, int sendweapons, struct clientdata_s *cd);
void ProcessBotCfgFile(Section *conf);		// for new config system by Andrey Kotrekhov
void PlaySoundConfirmation(edict_t *pEntity, int msg_type);	// sound messages confirmation
inline void MBMenuSystem(edict_t *pEntity, const char *arg1); // MB game menu
void RandomSkillCommand(edict_t *pEntity, const char *arg1);
void SpawnSkillCommand(edict_t *pEntity, const char *arg1);
void SetBotSkillCommand(edict_t *pEntity, const char *arg1);
void BotSkillUpCommand(edict_t *pEntity, const char *arg1);
void BotSkillDownCommand(edict_t *pEntity, const char *arg1);
void SetAimSkillCommand(edict_t *pEntity, const char *arg1);
void SetReactionTimeCommand(edict_t *pEntity, const char *arg1);
void RangeLimitCommand(edict_t *pEntity, const char *arg1);
void SetWaypointDirectoryCommand(edict_t *pEntity, const char *arg1);
void PassiveHealingCommand(edict_t *pEntity, const char *arg1);
void DontSpeakCommand(edict_t *pEntity, const char *arg1);
void DontChatCommand(edict_t *pEntity, const char *arg1);
void MBServerCommand(void);

#ifdef _DEBUG
inline bool CheckForSomeDebuggingCommands(edict_t *pEntity, const char *pcmd, const char *arg1, const char *arg2, const char *arg3, const char *arg4, const char *arg5);	// debugging commands
#endif


client_t::client_t()
{
	pEntity = NULL;
	client_is_human = FALSE;
	client_bleeds = FALSE;
	max_speed_time = -1.0;
}

// what we do here is that we set all external variables to their default values
externals_t::externals_t()
{
	ResetIsLogging();
	ResetRandomSkill();
	ResetSpawnSkill();
	ResetReactionTime();
	ResetBalanceTime();
	ResetMinBots();
	ResetMaxBots();
	ResetCustomClasses();
	ResetInfoTime();
	ResetPresentationTime();
	ResetDontSpeak();
	ResetDontChat();
	ResetRichNames();
	ResetPassiveHealing();
}

// what we do here is that we set all internal variables to their default values
internals_t::internals_t()
{
	ResetIsDistLimit();
	ResetMaxDistance();
}

void GameDLLInit(void)
{
	int i;

	/*///	NOT USED YET

	char filename[256];
	char buffer[256];
	int length;
	FILE *bfp;
	char *ptr;
	/**/

	(*g_engfuncs.pfnAddServerCommand) ("m_bot", MBServerCommand);

	// is dedicated server
	if (IS_DEDICATED_SERVER())
		is_dedicated_server = TRUE;

	for (i = 0; i < MAX_BOT_NAMES; i++)
	{
		bot_names[i].name;
		bot_names[i].is_used = FALSE;
	}

	whine_count = 0;

	for (i=0; i < 5; i++)
		recent_bot_whine[i] = -1;

	BotNameInit();

	// we need to initialize this mod weapon IDs
	if (InitFAWeapons() == FALSE)
	{
		// TODO: Print some error message, use a hud message for this
	}

	// initialize the weapon arrays
	memset(bot_weapon_select, 0, sizeof(bot_weapon_select));
	memset(bot_fire_delay, 0, sizeof(bot_fire_delay));

	//kota@ we should read weapon configuration from the file.
	char msg[1024];
	char filename[1024];
	char FA_version_string[16];

	// we need to use string version here
	if (g_mod_version == FA_30)
		strcpy(FA_version_string,"3_0");
	else if (g_mod_version == FA_29)
		strcpy(FA_version_string,"2_9");
	else if (g_mod_version == FA_28)
		strcpy(FA_version_string,"2_8");
	else if (g_mod_version == FA_27)
		strcpy(FA_version_string,"2_7");
	else if (g_mod_version == FA_26)
		strcpy(FA_version_string,"2_6");
	else if (g_mod_version == FA_25)
		strcpy(FA_version_string,"2_5");
	else if (g_mod_version == FA_24)
		strcpy(FA_version_string,"2_4");

	UTIL_MarineBotFileName(filename, "weapons", FA_version_string);

	sprintf(msg, "Executing %s\n", filename);
	ALERT( at_console, msg );

	conf_weapons = parceConfig(filename);
				
	if (conf_weapons == NULL)
	{
		sprintf(msg, "There is a syntax error in %s, or the file cant be found\n", filename);
		PrintOutput(NULL, msg, msg_error);
			
		sprintf(msg, "Bots will not shoot\n");
		PrintOutput(NULL, msg, msg_warning);
		
		// show also this error message through the hud once client join game
		if (warning_event == FALSE)
		{
			// prepare error message
			strcpy(error_msg, "WARNING - MarineBot detected an error: weapon definitions!\nThere is a syntax error or the file can't be found\nBots will not shoot");
			
			// to know that something bad happened
			warning_event = TRUE;

			// we need to prevent resetting of this error in DispatchSpawn()
			override_reset = TRUE;
		}
	}

	BotWeaponArraysInit(conf_weapons);

	/*///		NOT USED YET

	UTIL_MarineBotFileName(filename, "bot_whine.txt", NULL);

	bfp = fopen(filename, "r");

	if (bfp != NULL)
	{
		while ((whine_count < MAX_BOT_WHINE) && (fgets(buffer, 80, bfp) != NULL))
		{
			length = strlen(buffer);

			if (buffer[length-1] == '\n')
			{
				buffer[length-1] = 0;  // remove '\n'
				length--;
			}

			if ((ptr = strstr(buffer, "%n")) != NULL)
			{
				*(ptr+1) = 's';  // change %n to %s
			}

			if (length > 0)
			{
				strcpy(bot_whine[whine_count], buffer);
				whine_count++;
			}
		}

		fclose(bfp);
	}
	/**/

#ifdef _DEBUG
	memset(&usr_current_weapon, 0, sizeof(usr_current_weapon));
#endif

	//fill method's map
	NMethodsDict["path_1"] = BotOnPath1;

	// initialize the bots array
	bots = new bot_t[MAX_CLIENTS];

	(*other_gFunctionTable.pfnGameInit)();
}

int DispatchSpawn( edict_t *pent )
{
	if (gpGlobals->deathmatch)
	{
		char *pClassname = (char *)STRING(pent->v.classname);

#ifdef _DEBUG
		if (debug_engine)
		{
			fp=fopen(debug_fname,"a");
			fprintf(fp, "DispatchSpawn: %x %s\n",pent,pClassname);

			if (pent->v.model != 0)
				fprintf(fp, " model=%s\n",STRING(pent->v.model));

			fclose(fp);
		}
#endif

		// this method is the first method that's being called on map change
		// so do level initialization stuff here
		if (strcmp(pClassname, "worldspawn") == 0)
		{
			/*/
#ifdef _DEBUG
			fp=fopen("!mb_engine_debug.txt","a");
			fprintf(fp, "\n======================\n\n<dll.cpp> Dispatchspawn() - worldspawn on %s\n",
				STRING(gpGlobals->mapname));
			fclose(fp);
#endif
			/**/

			// the map learning initialization by kota@
			::teamPathMap.clear();

			// clear signatures first
			strcpy(wpt_author, "unknown");
			strcpy(wpt_modified, "unknown");

			WaypointInit();

			// reset error messaging system only if allowed to do so
			if (override_reset == FALSE)
			{
				// prepare new error message header
				strcpy(error_msg, "WARNING - MarineBot detected an error");

				// reset detected problems
				error_occured = FALSE;
				warning_event = FALSE;
			}

			int result = WaypointLoad(NULL, NULL);

			// if the waypoint file doesn't exist switch to the other directory and check again
			if (result == -10)
			{
				if (b_custom_wpts)
					b_custom_wpts = FALSE;
				else
					b_custom_wpts = TRUE;

				PrintOutput(NULL, "There is invalid or missing waypoint file for this map. ",
					msg_error);
				PrintOutput(NULL, "Checking the other waypoint directory\n", msg_info);

				result = WaypointLoad(NULL, NULL);
			}

			// if old waypoints are detected try convert them automatically
			if (result == -1)
				WaypointLoadUnsupported(NULL);
			// was there any other problem
			else if ((result == 0) || (result == -10))
			{
				// print error message directly into DS console
				if (is_dedicated_server)
				{
					PrintOutput(NULL, "There is invalid or missing waypoint file for this map\n", msg_error);
					PrintOutput(NULL, "Bots may play incorrectly\n", msg_warning);
				}
				else
				{
					// show the error message once client join
					if (warning_event == FALSE)
					{
						strcat(error_msg, ": waypoints!\nThere is invalid or missing waypoint file for this map\nBots may play incorrectly");

						warning_event = TRUE;
					}
				}
			}
			else
				PrintOutput(NULL, "Loading waypoints...\n", msg_info);

			// load waypoint paths
			result = WaypointPathLoad(NULL, NULL);
			
			// if old waypoint paths are detected try convert them automatically
			if (result == -1)
				WaypointPathLoadUnsupported(NULL);
			else if (result == 0)
			{
				if (is_dedicated_server)
				{
					PrintOutput(NULL, "There is invalid or missing path waypoint file for this map\n", msg_error);
					PrintOutput(NULL, "Bots may play incorrectly\n", msg_warning);
				}
				else
				{
					if (warning_event == FALSE)
					{
						strcat(error_msg, ": paths!\nThere is invalid or missing path waypoint file for this map\nBots may play incorrectly");

						warning_event = TRUE;
					}
				}
			}

			// turn off all show & auto commands section
			
			// hide waypoints
			if (g_waypoint_on)
				g_waypoint_on = FALSE;
			// stop autowaypointing
			if (g_auto_waypoint)
				g_auto_waypoint = FALSE;
			// hide paths
			if (g_path_waypoint_on)
				g_path_waypoint_on = FALSE;
			// stop autoadd into path
			if (g_auto_path)
				g_auto_path = FALSE;
			// turn off path highlighting
			if (highlight_this_path != -1)
				highlight_this_path = -1;

//			pent_info_firearms_detect = NULL;	// @@@@ TEMP: only temp don't know if we use it!!!

			// THESE TWO SHOULD BE USEFUL
			//pent_item_tfgoal = NULL;
			//team_class_limits[index] = 0;  // no class limits

			PRECACHE_SOUND("weapons/xbow_hit1.wav");      // waypoint add
			PRECACHE_SOUND("weapons/mine_activate.wav");  // waypoint delete
			PRECACHE_SOUND("common/wpn_hudoff.wav");      // path add/delete start
			PRECACHE_SOUND("common/wpn_moveselect.wav");  // path add/delete cancel

			PRECACHE_SOUND("plats/elevbell1.wav");		// snd_done
			PRECACHE_SOUND("buttons/button10.wav");		// snd_failed

			m_spriteTexture = PRECACHE_MODEL("sprites/lgtning.spr");	// the waypoint beam
						
			m_spriteTexturePath1 = PRECACHE_MODEL("sprites/zbeam6.spr");// the one-way path beam
			m_spriteTexturePath2 = m_spriteTexture;	// other path types do use same beam as waypoint
			m_spriteTexturePath3 = PRECACHE_MODEL("sprites/rope.spr");// for paths with additional flags (avoid & ignore enemy and such like)
			f_update_wpt_time = 0.0;

			g_GameRules = TRUE;

			// set the view distance limit back to default value
			internals.ResetIsDistLimit();
			internals.ResetMaxDistance();

			// see if this map is one of maps where the bots can snipe through skybox
			char mapname[64];
			strcpy(mapname, STRING(gpGlobals->mapname));

			if (strcmp(mapname, "ps_island") == 0)
			{
				internals.SetIsDistLimit(true);
				internals.SetMaxDistance(3000);
			}

			is_team_play = 0.0;
			skill_system = 1.0;
			checked_teamplay = FALSE;
			checked_skillsystem = FALSE;

			bot_cfg_pause_time = 0.0;
			respawn_time = 0.0;
			spawn_time_reset = FALSE;

			prev_num_bots = num_bots;
			num_bots = 0;
			override_max_bots = FALSE;

			bot_check_time = gpGlobals->time + 30.0;
		}
	}

	return (*other_gFunctionTable.pfnSpawn)(pent);
}

void DispatchThink( edict_t *pent )
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"DispatchThink:\n"); fclose(fp); }
	(*other_gFunctionTable.pfnThink)(pent);
}

void DispatchUse( edict_t *pentUsed, edict_t *pentOther )
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"DispatchUse:\n"); fclose(fp); }
	(*other_gFunctionTable.pfnUse)(pentUsed, pentOther);
}

void DispatchTouch( edict_t *pentTouched, edict_t *pentOther )
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"DispatchTouch:\n"); fclose(fp); }
	(*other_gFunctionTable.pfnTouch)(pentTouched, pentOther);
}

void DispatchBlocked( edict_t *pentBlocked, edict_t *pentOther )
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"DispatchBlocked:\n"); fclose(fp); }
	(*other_gFunctionTable.pfnBlocked)(pentBlocked, pentOther);
}

void DispatchKeyValue( edict_t *pentKeyvalue, KeyValueData *pkvd )
{
#ifdef _DEBUG
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "DispatchKeyValue: %x %s=%s\n",pentKeyvalue,pkvd->szKeyName,pkvd->szValue); fclose(fp); }
#endif

	//static edict_t *temp_pent;
	//static int flag_index;

	/*
	if (pentKeyvalue == pent_info_firearms_detect)
	{
		if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "DispatchKeyValue: %x %s=%s\n",pentKeyvalue,pkvd->szKeyName,pkvd->szValue); fclose(fp); }
	}
	else if (pent_info_firearms_detect == NULL)
		pent_info_firearms_detect = pentKeyvalue;
	*/

	(*other_gFunctionTable.pfnKeyValue)(pentKeyvalue, pkvd);
}

void DispatchSave( edict_t *pent, SAVERESTOREDATA *pSaveData )
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"DispatchSave:\n"); fclose(fp); }
	(*other_gFunctionTable.pfnSave)(pent, pSaveData);
}

int DispatchRestore( edict_t *pent, SAVERESTOREDATA *pSaveData, int globalEntity )
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"DispatchRestore:\n"); fclose(fp); }
	return (*other_gFunctionTable.pfnRestore)(pent, pSaveData, globalEntity);
}

void DispatchObjectCollsionBox( edict_t *pent )
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"DispatchObjectCollsionBox:\n"); fclose(fp); }
	(*other_gFunctionTable.pfnSetAbsBox)(pent);
}

void SaveWriteFields( SAVERESTOREDATA *pSaveData, const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount )
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"SaveWriteFields:\n"); fclose(fp); }
	(*other_gFunctionTable.pfnSaveWriteFields)(pSaveData, pname, pBaseData, pFields, fieldCount);
}

void SaveReadFields( SAVERESTOREDATA *pSaveData, const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount )
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"SaveReadFields:\n"); fclose(fp); }
	(*other_gFunctionTable.pfnSaveReadFields)(pSaveData, pname, pBaseData, pFields, fieldCount);
}

void SaveGlobalState( SAVERESTOREDATA *pSaveData )
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"SaveGlobalState:\n"); fclose(fp); }
	(*other_gFunctionTable.pfnSaveGlobalState)(pSaveData);
}

void RestoreGlobalState( SAVERESTOREDATA *pSaveData )
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"RestoreGlobalState:\n"); fclose(fp); }
	(*other_gFunctionTable.pfnRestoreGlobalState)(pSaveData);
}

void ResetGlobalState( void )
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"ResetGlobalState:\n"); fclose(fp); }
	(*other_gFunctionTable.pfnResetGlobalState)();
}

BOOL ClientConnect( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ]  )
{
	if (gpGlobals->deathmatch)
	{
		int i;

#ifdef _DEBUG
		if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "ClientConnect: pent=%x name=%s\n",pEntity,pszName); fclose(fp); }
#endif

		// check if this client is the listen server client
		if (strcmp(pszAddress, "loopback") == 0)
		{
			// save the edict of the listen server client...
			listenserver_edict = pEntity;
		}

		// check if this is NOT a bot joining the server...
		if (strcmp(pszAddress, "127.0.0.1") != 0)
		{
			// don't try to add bots for 60 seconds, give client time to get added
			bot_check_time = gpGlobals->time + 60.0;

			// if there are currently more than the minimum number of bots running AND
			// there's also more than max_bots clients on the server
			// then kick one of the bots off the server
			// do this only on dedicated server
			if ((is_dedicated_server) && (clients[0].BotCount() > 0) &&
				(clients[0].BotCount() > externals.GetMinBots()) &&
				(externals.GetMinBots() != -1) &&
				(clients[0].ClientCount() > externals.GetMaxBots()) &&
				(externals.GetMaxBots() != -1))
			{
				for (i=0; i < MAX_CLIENTS; i++)
				{
					// is this slot used?
					if (bots[i].is_used)
					{
						char cmd[80];

						sprintf(cmd, "kick \"%s\"\n", bots[i].name);

						SERVER_COMMAND(cmd);  // kick the bot using (kick "name")

						break;
					}
				}
			}
		}
	}

	return (*other_gFunctionTable.pfnClientConnect)(pEntity, pszName, pszAddress, szRejectReason);
}

void ClientDisconnect( edict_t *pEntity )
{
	if (gpGlobals->deathmatch)
	{
		int i;

#ifdef _DEBUG
		if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "ClientDisconnect: %x\n",pEntity); fclose(fp); }
#endif

		i = 0;
		while ((i < MAX_CLIENTS) && (clients[i].pEntity != pEntity))
			i++;

		if (i < MAX_CLIENTS)
		{
			if (clients[i].pEntity->v.flags & FL_FAKECLIENT)
				clients[i].substr_bot();
			else
				clients[i].substr_human();

			clients[i].pEntity = NULL;
			clients[i].SetHuman(FALSE);
			clients[i].SetBleeding(FALSE);
		}

		/*
#ifdef _DEBUG
		///@@@@@@@@@@@@@@@@@@@@22
		char msg[128];
		sprintf(msg, "***dll.cpp|ClientDiconnect() - total number of clients: %d\n", clients[0].ClientCount());
		PrintOutput(NULL, msg, msg_null);
#endif
		/**/


		for (i = 0; i < MAX_CLIENTS; i++)
		{
			if (bots[i].pEdict == pEntity)
			{
				// someone kicked this bot off of the server...

				bots[i].is_used = FALSE;  // this slot is now free to use

				bots[i].kick_time = gpGlobals->time;  // save the kicked time

				// try to find the name this bot used and sign it free
				for (int j = 0; j < MAX_BOT_NAMES; j++)
				{
					if (strstr(bots[i].name, bot_names[j].name) != NULL)
					{
						// this clients name is free again
						bot_names[j].is_used = FALSE;
					}
				}

				break;
			}
		}

		// check if any other bot is aiming at this one, if so clear it
		for (i = 0; i < MAX_CLIENTS; i++)
		{
			if (bots[i].is_used == FALSE)
				continue;

			if (bots[i].pEdict == pEntity)
				continue;

			// so NULL its enemy
			if (bots[i].pBotEnemy == pEntity)
			{				
				bots[i].BotForgetEnemy();
			}
		}
	}

	// remove the fakeclient bit before kicking the bot
	if (pEntity->v.flags & FL_FAKECLIENT)
	{
		pEntity->v.flags &= ~FL_FAKECLIENT;

		(*other_gFunctionTable.pfnClientDisconnect)(pEntity);
	}
	else
		(*other_gFunctionTable.pfnClientDisconnect)(pEntity);
}

void ClientKill( edict_t *pEntity )
{
#ifdef _DEBUG
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "ClientKill: %x\n",pEntity); fclose(fp); }
#endif

	(*other_gFunctionTable.pfnClientKill)(pEntity);
}

void ClientPutInServer( edict_t *pEntity )
{
#ifdef _DEBUG
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "ClientPutInServer: %x\n",pEntity); fclose(fp); }
#endif

	int i = 0;

	while ((i < MAX_CLIENTS) && (clients[i].pEntity != NULL))
		i++;

	if (i < MAX_CLIENTS)
	{
		clients[i].pEntity = pEntity;  // store this clients edict in the clients array

		if (!(pEntity->v.flags & FL_FAKECLIENT))
		{
			clients[i].SetHuman(TRUE);
			clients[i].add_human();
		}
		else
		{
			clients[i].SetHuman(FALSE);
			clients[i].add_bot();
		}
	}




	/*
#ifdef _DEBUG
	//@@@@@@@@@@@@@@@@@@@@22
	char msg[128];
	sprintf(msg, "***dll.cpp|ClientPutInServer() - adding client %s | total number of clients: %d\n",
		STRING(pEntity->v.netname), clients[0].ClientCount());
	PrintOutput(NULL, msg, msg_null);
	//UTIL_DebugInFile(msg);
#endif
	/**/


	(*other_gFunctionTable.pfnClientPutInServer)(pEntity);
}

void ClientCommand( edict_t *pEntity )
{
	const char *pcmd = Cmd_Argv(0);
	const char *arg1 = Cmd_Argv(1);
	const char *arg2 = Cmd_Argv(2);
	const char *arg3 = Cmd_Argv(3);
	const char *arg4 = Cmd_Argv(4);
	const char *arg5 = Cmd_Argv(5);

	// save the ClCommand author if it is not a bot
	if (!(pEntity->v.flags & FL_FAKECLIENT))
		pRecipient = pEntity;



	/*/
	//@@@@@@@@@@@@@@@
	if (pEntity != listenserver_edict)
		ALERT(at_console, "ClientCommand:%s (arg1:%s)(arg2:%s)(arg3:%s)(arg4:%s)(arg5:%s)\n",
		pcmd,arg1,arg2,arg3,arg4,arg5);
	/**/



	if (debug_engine)
	{
		char edict_name[32];

		strcpy(edict_name, STRING(pEntity->v.netname));

		fp=fopen(debug_fname,"a"); fprintf(fp,"%s's ClientCommand: %s",edict_name,pcmd);
		if ((arg1 != NULL) && (*arg1 != 0))
			fprintf(fp," %s", arg1);
		if ((arg2 != NULL) && (*arg2 != 0))
			fprintf(fp," %s", arg2);
		if ((arg3 != NULL) && (*arg3 != 0))
			fprintf(fp," %s", arg3);
		if ((arg4 != NULL) && (*arg4 != 0))
			fprintf(fp," %s", arg4);
		if ((arg5 != NULL) && (*arg5 != 0))
			fprintf(fp," %s", arg5);

		fprintf(fp, "\n");
		fclose(fp);
	}

	// only allow custom commands if deathmatch mode and NOT dedicated server and
	// client sending command is the listen server client...
	if ((gpGlobals->deathmatch) && (!is_dedicated_server) && (pEntity == listenserver_edict))
	{
		char msg[126];		

		if ((FStrEq(pcmd, "help")) || (FStrEq(pcmd, "?")))
		{
			if (FStrEq(arg1, "botsetup"))
			{
				// we don't need to show help in notify area (top left corner on the screen)
				// therefore we send them directly to console
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "\n***Additional bot settings***\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[kick <\"name\">] kick bot with specified name (name must be in quotes)\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[killbot <arg>] 'arg' could be <all> - kill all bots, <red/blue> - kill all red/blue team bots currently in the game\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[randomskill] toggle between using default bot skill and generating the skill randomly for each bot\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[spawnskill <number>] set default bot skill on join (1-5 where 1=best, no effect to bots already in game)\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[setbotskill <number>] set bot skill level for all bots already in game (1-5 where 1=best)\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[botskillup] increase bot skill level for all bots already in game\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[botskilldown] decrease bot skill level for all bots already in game\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[setaimskill <number>] set aim skill level for all bots already in game (1-5 where 1=best)\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[botskillview] show botskill & aimskill for all bots\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[reactiontime <number>] set bot reaction time (0-50 where 1 will be converted to 0.1s and 50 to 5.0s)\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[passivehealing <off>] bot will not heal teammates automatically, off parameter returns to normal\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[rangelimit <number>] set the max distance of enemy the bot can see & attack (500-7500 units)\n");
			}
			else if (FStrEq(arg1, "cheats"))
			{
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "\n***Should help in troubles***\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[observer <off>] bot ignore you, off parameter returns to normal\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[mrfreeze <off>] bot dont move, off parameter returns to normal\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[botdontshoot <off>] bot dont press trigger, off parameter returns to normal\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[botignoreall <off>] bot ignore all enemies, off parameter returns to normal\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[botdontspeak <off>] bot will not use Voice&Radio commands, off parameter returns to normal\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[mbnoclip <off>] you will be able to fly through all on the map, off parameter returns to normal\n");
			}
			else if (FStrEq(arg1, "misc"))
			{
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "\n***Miscellaneous commands***\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[check_aims] connect 'wait timed' waypoint with its valid aim waypoints\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[check_cross] connect cross waypoint with all waypoints in range\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[check_ranges] draw (highlight) waypoint range\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[get_wpt_system] print the system the waypoint file is created in\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[mbwptmenu] shortcut to MB waypoint menu\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[mbwptmenuadd] shortcut to MB add waypoint menu\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[mbwptmenuchange] shortcut to MB change waypoint menu\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[mbwptmenupath] shortcut to MB path menu\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[mbwptmenupathflags] shortcut to MB change path flags menu\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[get_texture_name] print the name of the texture in front of you\n");
			}
			else
			{
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "\n-------------------------------------------\nMarineBot console commands help\n-------------------------------------------\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[recruit <team> <class> <skin> <skill> <name>] add bot with specified params, if no params all except skill is taken randomly\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[kickbot <arg>] 'arg' could be <all> - kick all bots, <red/blue> - kick one red/blue team bot out of the game\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[balanceteams <off>] try balancing teams on server (move bots to weaker team), 'off' disables autobalancing\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[fillserver <number>] fill the server with bots up to maxplayers limit or up to 'number' limit\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[marinebotmenu] show MB command menu\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[directory <name>] toggle through waypoint directories or directly set one via 'name'\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[versioninfo] print MB version\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "\n<><><><><><><><><><><><><><><><><><><<><><><><><><>\nfor additional help write one of following commands\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "<><><><><><><><><><><><><><><><><><><<><><><><><><>\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[help <botsetup>] botsetting help | [help <cheats>] few cheats | [help <misc>] various settings help\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[wpt <help>] waypointing help | [autowpt <help>] autowaypointing help | [pathwpt <help>] pathwaypointing help\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[debug <help>] debugging help | [test <help>] test help\n");
			}

			return;
		}
		// add one bot
		else if ((FStrEq(pcmd, "recruit")) || (FStrEq(pcmd, "addbot")) || (FStrEq(pcmd, "addmarine")))
		{
			BotCreate( pEntity, arg1, arg2, arg3, arg4, arg5 );

			bot_check_time = gpGlobals->time + 2.5;

			return;
		}
		else if (FStrEq(pcmd, "fillserver"))	// automatically add bots up to maxplayers
		{
			int total_clients;

			total_clients = 0;
			for (int i = 0; i < MAX_CLIENTS; i++)
			{
				if (clients[i].pEntity != NULL)
					total_clients++;
			}

			if (total_clients >= gpGlobals->maxClients)
			{
				server_filling = FALSE;

				PlaySoundConfirmation(pEntity, SND_FAILED);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "error - server full\n");
			}
			else
			{
				server_filling = TRUE;
				bot_check_time = gpGlobals->time + 0.5;

				// check if there is specified bot ammount to add (i.e. "arg filling")
				if ((arg1 != NULL) && (*arg1 != 0))
				{
					int temp = atoi(arg1);

					if ((temp >= 1) && (temp < gpGlobals->maxClients - total_clients))
					{
						bots_to_add = temp;

						PlaySoundConfirmation(pEntity, SND_DONE);
						sprintf(msg, "%d bots will be added\n", bots_to_add);
						ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
					}
					else
					{
						server_filling = FALSE;

						PlaySoundConfirmation(pEntity, SND_FAILED);
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "error - invalid argument or over maxplayers limit!\n");

						return;
					}
				}

				ClientPrint(pEntity, HUD_PRINTNOTIFY, "filling the server...\n");
			}

			return;
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
			else if ((arg1 == NULL) || (*arg1 == 0))	// no arg then kick random bot
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

			return;
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

			ClientPrint(pEntity, HUD_PRINTNOTIFY,msg);

			return;
		}
		// try balancing teams
		else if ((FStrEq(pcmd, "balanceteams"))	 || (FStrEq(pcmd, "balance_teams")))
		{
			// turn off autobalance for this map, it will be turned back on after a map change
			if (FStrEq(arg1, "off"))
			{
				// disable it only once
				if ((do_auto_balance == TRUE) && (override_auto_balance == FALSE))
				{
					override_auto_balance = TRUE;

					ClientPrint(pEntity, HUD_PRINTNOTIFY, "autobalance is temporary DISABLED\n");
				}
				else
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "autobalance is already DISABLED\n");

				return;
			}

			team_balance_value = UTIL_BalanceTeams();

			if (team_balance_value == -1)
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "teams balanced\n");
			else if (team_balance_value > 100)
			{
				sprintf(msg, "kick %d bots from RED team and add them to blue\n", team_balance_value - 100);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
			}
			else if ((team_balance_value > 0) && (team_balance_value < 100))
			{
				sprintf(msg, "kick %d bots from BLUE team and add them to red\n", team_balance_value);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
			}
			else if (team_balance_value < -1)
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "ERROR\n");
			}

			return;
		}
		// change if the bot will use default spawn skill or pick a skill randomly
		else if ((FStrEq(pcmd, "randomskill")) || (FStrEq(pcmd, "random_skill")))
		{
			RandomSkillCommand(pEntity, arg1);

			return;
		}
		// set bot skill at spawn
		else if ((FStrEq(pcmd, "spawnskill")) || (FStrEq(pcmd, "spawn_skill")))
		{
			SpawnSkillCommand(pEntity, arg1);

			return;
		}
		// change bot skill - apply to all bots
		else if ((FStrEq(pcmd, "setbotskill")) || (FStrEq(pcmd, "set_botskill")))
		{
			SetBotSkillCommand(pEntity, arg1);

			return;
		}
		// increase bot skill - apply to all bots
		else if ((FStrEq(pcmd, "botskillup")) || (FStrEq(pcmd, "botskill_up")))
		{
			BotSkillUpCommand(pEntity, arg1);

			return;
		}
		// decrease bot skill - apply to all bots
		else if ((FStrEq(pcmd, "botskilldown")) || (FStrEq(pcmd, "botskill_down")))
		{
			BotSkillDownCommand(pEntity, arg1);

			return;
		}
		// change aim skill - apply to all bots
		else if ((FStrEq(pcmd, "setaimskill")) || (FStrEq(pcmd, "set_aimskill")))
		{
			SetAimSkillCommand(pEntity, arg1);

			return;
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

			return;
		}
		// set bot reaction_time
		else if ((FStrEq(pcmd, "reactiontime")) || (FStrEq(pcmd, "reaction_time")))
		{
			SetReactionTimeCommand(pEntity, arg1);

			return;
		}
		// bot will not heal teammates automatically
		else if ((FStrEq(pcmd, "passivehealing")) || (FStrEq(pcmd, "passive_healing")))
		{
			PassiveHealingCommand(pEntity, arg1);

			return;
		}
		// set max distance of enemy the bot can see and thus can engage
		else if ((FStrEq(pcmd, "rangelimit")) || (FStrEq(pcmd, "range_limit")))
		{
			RangeLimitCommand(pEntity, arg1);

			return;
		}
		else if (FStrEq(pcmd, "directory"))	// change waypoint folder
		{
			SetWaypointDirectoryCommand(pEntity, arg1);

			return;
		}
		else if (FStrEq(pcmd, "marinebotmenu"))		// MarineBot command menu
		{
			g_menu_state = MENU_MAIN;
			UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_main);
			
			return;
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

			return;
		}
		else if (FStrEq(pcmd, "mbwptmenuadd"))		// shortcut to waypoint menu (add wpt)
		{			
			g_menu_state = MENU_2_1AND2_P1;
			g_menu_next_state = 128;		// to know that's adding

			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_1);
			
			return;
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

			return;
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

			return;
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

			return;
		}
		else if ((FStrEq(pcmd,"versioninfo")) || (FStrEq(pcmd,"version_info")))	// version info
		{
			sprintf(msg, "You are running MarineBot in version %s\n", mb_version_info);

			// display it also on HUD using same style as MB intro messages
			Vector color1 = Vector(200, 50, 0);
			Vector color2 = Vector(0, 250, 0);
			CustHudMessage(pEntity, msg, color1, color2, 2, 8);

			ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

			return;
		}
		else if (FStrEq(pcmd, "observer"))		// bot ignore player
		{
			if ((b_observer_mode) || (FStrEq(arg1, "off")))
				b_observer_mode = FALSE;
			else
				b_observer_mode = TRUE;

			if (b_observer_mode)
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "observer mode ENABLED\n");
			else
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "observer mode DISABLED\n");

			return;
		}
		else if (FStrEq(pcmd, "mrfreeze"))		// bot is stood/don't move
		{
			if ((b_freeze_mode) || (FStrEq(arg1, "off")))
				b_freeze_mode = FALSE;
			else
				b_freeze_mode = TRUE;

			if (b_freeze_mode)
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "freeze mode ENABLED\n");
			else
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "freeze mode DISABLED\n");

			return;
		}
		else if (FStrEq(pcmd, "botdontshoot"))	// bot will NOT press trigger
		{
			if ((b_botdontshoot) || (FStrEq(arg1, "off")))
				b_botdontshoot = FALSE;
			else
				b_botdontshoot = TRUE;

			if (b_botdontshoot)
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "botdontshoot mode ENABLED\n");
			else
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "botdontshoot mode DISABLED\n");

			return;
		}
		else if (FStrEq(pcmd, "botignoreall"))	// bot ignore all opponents
		{
			if ((b_botignoreall) || (FStrEq(arg1, "off")))
				b_botignoreall = FALSE;
			else
				b_botignoreall = TRUE;

			if (b_botignoreall)
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "botignoreall mode ENABLED\n");
			else
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "botignoreall mode DISABLED\n");

			return;
		}
		// bot will not use Voice&Radio cmds
		else if ((FStrEq(pcmd, "botdontspeak")) || (FStrEq(pcmd, "dont_speak")))
		{
			DontSpeakCommand(pEntity, arg1);

			return;
		}
		// bot will/won't use say and say_team commands
		else if ((FStrEq(pcmd, "botdontchat")) || (FStrEq(pcmd, "dont_chat")))
		{
			DontChatCommand(pEntity, arg1);

			return;
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

			return;
		}
		// show valid connections
		else if (FStrEq(pcmd, "check_aims") || FStrEq(pcmd,"checkaims"))
		{
			// turn off this first to prevent overload on netchannel
			check_ranges = FALSE;

			if ((check_aims) || (FStrEq(arg1, "off")))
				check_aims = FALSE;
			else
				check_aims = TRUE;

			if (check_aims)
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "check_aims ENABLED!\n");
			else
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "check_aims DISABLED!\n");

			return;
		}
		// show valid connections
		else if (FStrEq(pcmd, "check_cross") || FStrEq(pcmd,"checkcross"))
		{
			// turn off this first to prevent overload on netchannel
			check_ranges = FALSE;

			if ((check_cross) || (FStrEq(arg1, "off")))
				check_cross = FALSE;
			else
				check_cross = TRUE;

			if (check_cross)
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "check_cross ENABLED!\n");
			else
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "check_cross DISABLED!\n");

			return;
		}
		// highlight waypoint ranges
		else if (FStrEq(pcmd, "check_ranges") || FStrEq(pcmd,"checkranges"))
		{
			// turn off this first to prevent overload on netchannel
			check_aims = FALSE;
			check_cross = FALSE;

			if ((check_ranges) || (FStrEq(arg1, "off")))
				check_ranges = FALSE;
			else
				check_ranges = TRUE;

			if (check_ranges)
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "check_ranges ENABLED!\n");
			else
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "check_ranges DISABLED!\n");

			return;
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

			return;
		}
		else if ((FStrEq(pcmd, "gettexturename")) || (FStrEq(pcmd, "get_texture_name")))
		{
			UTIL_MakeVectors(pEntity->v.v_angle);
			
			Vector vecStart = pEntity->v.origin + pEntity->v.view_ofs;
			Vector vecEnd = vecStart + gpGlobals->v_forward * 100;
			
			TraceResult tr;
			UTIL_TraceLine(vecStart, vecEnd, dont_ignore_monsters, dont_ignore_glass, pEntity, &tr);
			
			const char *texture = g_engfuncs.pfnTraceTexture(tr.pHit, vecStart, vecEnd);
			
			if (texture == NULL)
				sprintf(msg, "unable to get the texture name or you are too far!\n");
			else
				sprintf(msg, "the name of the texture in front is \"%s\"\n", texture);
			ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
			
			return;
		}
		else if (FStrEq(pcmd, "debug"))
		{
			if ((FStrEq(arg1, "help")) || (FStrEq(arg1, "?")))
			{
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "\nAll valid debugging commands you can use\n***TIP: best results are when used with just one bot in game!***\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "--------------------------------\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[debug_aim_targets] print valid aim indexes\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[debug_actions] print info about actions the bot does\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[debug_cross] print info about bot's picks at cross\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[debug_paths] print info about path the bot is following\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[debug_waypoints] print info about waypoints the bot is heading towards (when not following a path)\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[debug_weapons] print all weapon indexes the bot spawn with\n");
				//ClientPrint(pEntity, HUD_PRINTCONSOLE, "[] \n");

#ifdef _DEBUG
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[debug_menu] print info about current state of the botmenu\n");
				//ClientPrint(pEntity, HUD_PRINTCONSOLE, "[debug_engine] \n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[debug_stuck] print if stuck on something\n");
#endif
			}

			return;
		}
		else if (FStrEq(pcmd, "debugaimtargets") || FStrEq(pcmd, "debug_aim_targets"))
		{
			if ((debug_aim_targets) || (FStrEq(arg1, "off")))
				debug_aim_targets = FALSE;
			else
				debug_aim_targets = TRUE;

			if (debug_aim_targets)
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_aim_targets ENABLED!\n");
			else
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_aim_targets DISABLED!\n");

			return;
		}
		else if (FStrEq(pcmd, "debugactions") || FStrEq(pcmd, "debug_actions"))
		{
			if ((debug_actions) || (FStrEq(arg1, "off")))
				debug_actions = FALSE;
			else
				debug_actions = TRUE;

			if (debug_actions)
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_actions ENABLED!\n");
			else
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_actions DISABLED!\n");

			return;
		}
		else if (FStrEq(pcmd, "debugcross") || FStrEq(pcmd, "debug_cross"))
		{
			if ((debug_cross) || (FStrEq(arg1, "off")))
				debug_cross = FALSE;
			else
				debug_cross = TRUE;

			if (debug_cross)
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_cross ENABLED!\n");
			else
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_cross DISABLED!\n");

			return;
		}
		else if (FStrEq(pcmd, "debugpaths") || FStrEq(pcmd, "debug_paths"))
		{
			if ((debug_paths) || (FStrEq(arg1, "off")))
				debug_paths = FALSE;
			else
				debug_paths = TRUE;

			if (debug_paths)
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_paths ENABLED!\n");
			else
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_paths DISABLED!\n");

			return;
		}
		else if (FStrEq(pcmd, "debugwaypoints") || FStrEq(pcmd, "debug_waypoints"))
		{
			if ((debug_waypoints) || (FStrEq(arg1, "off")))
				debug_waypoints = FALSE;
			else
				debug_waypoints = TRUE;

			if (debug_waypoints)
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_waypoints ENABLED!\n");
			else
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_waypoints DISABLED!\n");

			return;
		}
		else if (FStrEq(pcmd, "debugweapons") || FStrEq(pcmd, "debug_weapons"))
		{
			if ((debug_weapons) || (FStrEq(arg1, "off")))
				debug_weapons = FALSE;
			else
				debug_weapons = TRUE;

			if (debug_weapons)
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_weapons ENABLED!\n");
			else
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_weapons DISABLED!\n");

			return;
		}
		else if (FStrEq(pcmd, "test"))
		{
			if ((FStrEq(arg1, "help")) || (FStrEq(arg1, "?")))
			{
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "\nAll valid test commands\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[aimcode] toggle standard/new aim code; the new aim code isn't fully finished\n");
			}
			else if ((FStrEq(arg1, "aimcode")) || (FStrEq(arg1, "aim_code")))
			{
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
			}
			else
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid or missing argument!\n");
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "***TIP: use 'test help' for help***\n");
			}

			return;
		}
		else if (FStrEq(pcmd, "wpt"))
		{
			if ((FStrEq(arg1, "help")) || (FStrEq(arg1, "?")))
			{
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "\nAll waypoint commands\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[types] for help with the different types of waypoint\t\t[commands] for a list of all the commands you can use on a waypoint\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "--------------------------------\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[on] show waypoints\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[off] hide waypoints\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[show] toggle show/hide waypoints\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[autosave] toggle auto saving waypoints & paths to special files\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[save] save waypoints & paths to appropriate files\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[load] load waypoints & paths from HDD\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[load <name>] load waypoints & paths from specified 'name' files\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[loadunsupported] load older waypoints & paths from HDD (it's usually done automatically at map start)\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[loadunsupportedversion5] load old waypoints & paths made for MB0.8b (i.e. waypoint system version 5)\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[info <arg1> <arg2>] info about close or 'arg' waypoint; 'more' additional info; 'full' all info\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[count] info about total amounts of waypoints\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[printall] print all used waypoints and their flags; repeat this command to list through them\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[triggerevent <arg1> <arg2> <arg3>] allow dynamic priority gating (use 'wpt triggerevent help' for more info)\n");
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
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[add <type>] add 'type' waypoint\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[move <num>] move closest or specified waypoint to current player position\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[delete] delete close waypoint\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[change <type>] change close waypoint to specified type\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[setpriority <num> <team>] change 'team' (not specified=both) wpt priority to 'num' (0-5) where 0=no, 1=highest, 5=lowest\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[settime <num> <team>] change 'team' (not specified=both) wpt time to 'num' (0-500)\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[setrange <num>] set waypoint range for both teams (0-400)\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[reset <arg1> <arg2> <arg3> <arg4>] reset values specified in args back to default (use 'wpt reset help' for more info)\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[clear] delete all waypoints (can be still load back)\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[destroy] delete all waypoints & prepare for new creation (can NOT be load back)\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[author] show waypoints creator signature\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[subscribe <name>] add name (up to 32 chars - no spaces) signature to waypoints\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[modified <name>] add name (up to 32 chars - no spaces) signature to waypoints\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[drawdistance <num>] set the max distance the waypoint can be to show on screen\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[position <index>] print the position of 'index' waypoint and yours too\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[detect <index>] print all entities like buttons, bandages etc. that are around 'index' waypoint\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[repair <arg1> <arg2>] automatic waypoint repair tools (use 'wpt repair help' for more info)\n");
			}
			else if (FStrEq(arg1, "on"))
			{
				g_waypoint_on = TRUE;

				ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are ON\n");
			}
			else if (FStrEq(arg1, "off"))
			{
				g_waypoint_on = FALSE;

				// turn paths off too (we don't want to see path beams when waypoints are hidden)
				g_path_waypoint_on = FALSE;

				ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are OFF\n");
			}
			else if (FStrEq(arg1, "show"))
			{
				if (g_waypoint_on == FALSE)
					g_waypoint_on = TRUE;
				else
				{
					g_waypoint_on = FALSE;

					g_path_waypoint_on = FALSE;
				}

				ClientPrint(pEntity, HUD_PRINTNOTIFY, "***toggle mode used***\n");

				// print proper message
				if (g_waypoint_on)
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are ON\n");
				else
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are OFF\n");
			}
			else if (FStrEq(arg1, "add"))	// place a new waypoint
			{
				char *result;

				if (!g_waypoint_on)
					g_waypoint_on = TRUE;  // turn waypoints on if off

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

				if ((arg2 != NULL) && (*arg2 != 0))
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
				if (!g_waypoint_on)
					g_waypoint_on = TRUE;

				WaypointDelete(pEntity);
			}
			else if (FStrEq(arg1, "autosave"))		// store waypoint data to the file
			{
				if (b_wpt_autosave == FALSE)
					b_wpt_autosave = TRUE;
				else
					b_wpt_autosave = FALSE;

				ClientPrint(pEntity, HUD_PRINTNOTIFY, "***toggle mode used***\n");

				if (b_wpt_autosave)
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "auto saving waypoints ENABLED\n");
				else
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "auto saving waypoints DISABLED\n");
			}
			else if (FStrEq(arg1, "save"))		// store waypoint data to the file
			{
				if (WaypointSave(NULL))
				{
					if (WaypointPathSave(NULL))
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
				if ((arg2 != NULL) && (*arg2 != 0))
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
					if (WaypointLoad(pEntity, NULL) > 0)
					{
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints loaded\n");

						if (WaypointPathLoad(pEntity, NULL) > 0)
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
				char *result;

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
				else if (strcmp((const char *)result, "removed") == 0)
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

					if ((arg3 != NULL) && (*arg3 != 0))
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

					if ((arg3 != NULL) && (*arg3 != 0))
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
					sprintf(msg, "both times set to %.1f\n", result - (float) 1000.0);
				}
				else if (result == -100.0)
					sprintf(msg, "unknown event in 'settime'\n");
				else
				{
					PlaySoundConfirmation(pEntity, SND_DONE);

					if ((arg3 != NULL) && (*arg3 != 0))
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
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<all> reset all values of close waypoint\n");
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<type> or <flag> or <tag> reset close waypoint type back to normal waypoint\n");
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<priority> reset the priority value of close waypoint for both teams\n");
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<time> reset the wait time value of close waypoint for both teams\n");
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<range> reset the range value of close waypoint\n");
				}
				else if(WptResetAdditional(pEntity, arg2, arg3, arg4, arg5) == TRUE)
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
			else if (FStrEq(arg1, "printall"))
			{
				WaypointPrintAllWaypoints(pEntity);
			}
			else if ((FStrEq(arg1, "triggerevent")) || (FStrEq(arg1, "trigger_event")))	// handle the triggers
			{
				if ((FStrEq(arg2, "help")) || (FStrEq(arg2, "?")))
				{
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "\nAll valid trigger event options\n--------------------------------\n");
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<add> <trigger_name> <message> add a message to given trigger slot\n");
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<delete> <trigger_name> erase given trigger slot\n");
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<setpriority> <prior> <team> set the second (ie. trigger) priority, works same as standard priority\n");
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<settrigger> <trigger_name> <state> connect appropriate trigger msg to close waypoint, state means the 'on' or 'off' event\n");
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<removetrigger> <state> remove the trigger message from close waypoint based on the state value\n");
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<info> print info about the trigger waypoint\n");
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<showall> print all trigger event messages\n");
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<help> show this help\n");
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
						
						if ((arg4 != NULL) && (*arg4 != 0))
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

						if ((arg4 != NULL) && (*arg4 != 0))
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
							sprintf(msg, "trigger%d holds \"%s\"\n", index+1, trigger_events[index].message);
						}
						else
							sprintf(msg, "trigger%d is free\n", index+1);

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
						WaypointSave(NULL);		// save them

						// are there any paths then save them too
						if (num_w_paths > 0)
							WaypointPathSave(NULL);
						
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
						ClientPrint(pEntity, HUD_PRINTCONSOLE, "<clear> remove/erase current modified by tag\n");
						ClientPrint(pEntity, HUD_PRINTCONSOLE, "<fill_the_name_here> make the name the current modified by tag\n");
						ClientPrint(pEntity, HUD_PRINTCONSOLE, "<help> show this help\n");
					}
					else
					{
						char temp[32];
						
						strncpy(temp, arg2, 31);
						temp[31] = 0;
						
						if (WaypointSubscribe(temp, FALSE))
						{
							WaypointSave(NULL);		// save them
						
							if (num_w_paths > 0)
								WaypointPathSave(NULL);
							
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
				char *author_result;

				author_result = WaypointAuthors(TRUE);

				if (author_result == "nofile")
				{
					PlaySoundConfirmation(pEntity, SND_FAILED);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "no waypoint file!\n");
				}
				else
				{
					char *modif_result;

					modif_result = WaypointAuthors(FALSE);

					if ((author_result == "noauthor") && (modif_result == "nosig"))
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints aren't subscribed!\n");
					else if ((author_result == "noauthor") && (modif_result != "nosig"))
					{
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints author is unknown!\n");

						sprintf(msg, "waypoints were modified by %s\n", modif_result);
						ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
					}
					else if (modif_result == "nosig")
					{
						if (IsOfficialWpt(author_result))
							sprintf(msg, "official MarineBot waypoints by %s\n", author_result);
						else
							sprintf(msg, "waypoints were created by %s\n", author_result);

						// display it also on HUD using same style as MB intro messages
						Vector color1 = Vector(200, 50, 0);
						Vector color2 = Vector(0, 250, 0);
						CustHudMessage(pEntity, msg, color1, color2, 2, 8);

						ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
					}
					else
					{
						if ((IsOfficialWpt(author_result)) || (IsOfficialWpt(modif_result)))
							sprintf(msg, "official MarineBot waypoints by %s\nmodified by %s\n",
								author_result, modif_result);
						else
							sprintf(msg, "waypoints by %s\nmodified by %s\n",
								author_result, modif_result);

						Vector color1 = Vector(200, 50, 0);
						Vector color2 = Vector(0, 250, 0);
						CustHudMessage(pEntity, msg, color1, color2, 2, 8);

						ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
					}
				}
			}
			else if (FStrEq(arg1, "drawdistance"))	// change draw distance
			{
				if ((arg2 != NULL) && (*arg2 != 0))
				{
					int temp = atoi(arg2);

					if ((temp >= 100) && (temp <= 1400))
					{
						g_draw_distance = temp;

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
				if ((arg2 != NULL) && (*arg2 != 0))
				{
					int wpt_index = atoi(arg2) - 1;

					if ((wpt_index >= 0) && (wpt_index < num_waypoints))
					{
						if (IsWaypoint(wpt_index, wpt_no))
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
				if ((arg2 != NULL) && (*arg2 != 0))
				{
					int wpt_index = atoi(arg2) - 1;

					if ((wpt_index >= 0) && (wpt_index < num_waypoints))
					{
						if (IsWaypoint(wpt_index, wpt_no))
						{
							PlaySoundConfirmation(pEntity, SND_FAILED);
							ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoint doesn't exist (was deleted)!\n");
						}
						else
						{
							edict_t *pent = NULL;
							char entities[242];
							entities[0] = '\0';

							PlaySoundConfirmation(pEntity, SND_DONE);
							ClientPrint(pEntity, HUD_PRINTNOTIFY, "Checking entities around ...\n");

							while ((pent = UTIL_FindEntityInSphere(pent, waypoints[wpt_index].origin,
								PLAYER_SEARCH_RADIUS)) != NULL)
							{
								if (strlen(entities) > 0)
									strcat(entities, " & ");

								strcat(entities, STRING(pent->v.classname));
							}

							if (strlen(entities) == 0)
							{
								ClientPrint(pEntity, HUD_PRINTNOTIFY, "found nothing!\n");
							}
							else
							{
								sprintf(msg, "found: %s\n", entities);
								ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
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
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "[masterrepair] apply all available fixes except for range and position\n");
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "[range <index>] fix range on all waypoints or on the 'index' waypoint\n");
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "[range_and_position <index>] fix range and position on all waypoints or on the 'index' waypoint\n");
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "[pathend <index>] fix missing goback on 'index' path end or all paths if no index specified\n");
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "[pathmerge <index>] fix invalid path merge on 'index' path or all paths if no index specified\n");
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "[sniperspot <index>] fix wrongly set sniper spot on 'index' path or all paths if no index specified\n");
				}
				else if (FStrEq(arg2, "masterrepair"))
				{
					WaypointRepairInvalidPathMerge();
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "all paths were checked for invalid merge and found problems were repaired ...\n");
					
					WaypointRepairInvalidPathEnd();
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "all paths were checked for missing goback and all available fixes were applied ...\n");

					WaypointRepairSniperSpot();
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "any wrongly set sniper spot that was found was repaired! ...\n");

					PlaySoundConfirmation(pEntity, SND_DONE);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoint master repair DONE!\n");
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "***TIP: use 'wpt save' command if you want to keep all these changes***\n");
				}
				else if (FStrEq(arg2, "range"))
				{
					if ((arg3 != NULL) && (*arg3 != 0))
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
					if ((arg3 != NULL) && (*arg3 != 0))
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
					if ((arg3 != NULL) && (*arg3 != 0))
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
					if ((arg3 != NULL) && (*arg3 != 0))
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
					if ((arg3 != NULL) && (*arg3 != 0))
					{
						int path_index = atoi(arg1) - 1;
						
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

				if (g_waypoint_on)
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are ON\n");
				else
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoints are OFF\n");
			}

			return;
		}
		else if (FStrEq(pcmd, "autowpt"))
		{
			if ((FStrEq(arg1, "help")) || (FStrEq(arg1, "?")))
			{
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "\nAll autowaypointing commands\n------------------------------\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[on] start autowaypointing\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[off] stop autowaypointing\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[start] toggle start/stop autowaypointing\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[distance <number>] set distance (80-400) between two waypoints while autowaypointing\n");
			}
			else if (FStrEq(arg1, "on"))
			{
				g_auto_waypoint = TRUE;	// turn autowaypointing on
				g_waypoint_on = TRUE;	// turn waypoints on too just in case

				ClientPrint(pEntity, HUD_PRINTNOTIFY, "autowaypointing is ON\n");
			}
			else if (FStrEq(arg1, "off"))
			{
				g_auto_waypoint = FALSE;
				
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "autowaypointing is OFF\n");
			}
			else if (FStrEq(arg1, "start"))
			{
				if (g_auto_waypoint == FALSE)
				{
					g_auto_waypoint = TRUE;
					g_waypoint_on = TRUE;
				}
				else
					g_auto_waypoint = FALSE;	// turn autowaypointing off

				ClientPrint(pEntity, HUD_PRINTNOTIFY, "***toggle mode used***\n");

				// print proper msg
				if (g_auto_waypoint)
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "autowaypointing is ON\n");
				else
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "autowaypointing is OFF\n");
			}
			else if (FStrEq(arg1, "distance"))	// set distance between two wpts for autowpt
			{
				if ((arg2 != NULL) && (*arg2 != 0))
				{
					int temp = atoi(arg2);

					if ((temp >= MIN_WPT_DIST) && (temp <= MAX_WPT_DIST))
					{
						g_auto_wpt_distance = temp;

						PlaySoundConfirmation(pEntity, SND_DONE);
						sprintf(msg, "autowaypointing distance set to %d\n", temp);
					}
					else
					{
						PlaySoundConfirmation(pEntity, SND_FAILED);
						sprintf(msg, "invalid autowaypointing distance!\n");
					}

					ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
				}
			}
			else
			{
				PlaySoundConfirmation(pEntity, SND_FAILED);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "invalid or missing argument!\n");
				ClientPrint(pEntity, HUD_PRINTNOTIFY, "***TIP: use 'autowpt help' for help***\n");

				if (g_auto_waypoint)
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "autowaypointing is ON\n");
				else
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "autowaypointing is OFF\n");
			}

			return;
		}
		else if (FStrEq(pcmd, "pathwpt"))
		{
			if ((FStrEq(arg1, "help")) || (FStrEq(arg1, "?")))
			{
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "\nAll pathwaypointing commands\n----------------------------------\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[commands] edit commands help\n--------------------------------\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[on] show paths\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[off] hide paths\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[show] toggle show/hide paths\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[save] save all paths to the file\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[load] load all paths from file\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[load <mapname>] load all paths from 'mapname' file\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[count] print info about total amounts of paths\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[info <index>] print info about 'index' path or path on close waypoint\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[printall <arg>] print info about all paths or paths on 'arg' waypoint or close wpt if arg is 'nearby'\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[printpath <index>] print all waypoints and their types from 'index' path\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[checkinvalid <info>] remove all invalid paths ie. with length=1; if 'info' it will print more info\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[checkproblems <save>] print problems with paths; if 'save' it will write all in MB error log file\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[highlight <arg>] show only path or paths matching the 'arg' filter; used without 'arg' will turn it off\n");
#ifdef _DEBUG
				//!!!!!!!!!!!!!!!!!!!!!!!!!
				//TEMP
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "\n!!!Following cmds are for dev purposes!!!\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[flags <index>] print few info about actual or 'index' path\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[pap] print basic info about all paths\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[pll <index>] print all path wpts (whole llist)\n");
				
				//ClientPrint(pEntity, HUD_PRINTNOTIFY, "NOT DONE YET\n");
				//END TEMP
#endif
			}
			else if (FStrEq(arg1, "commands"))
			{
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "\nAll edit commands\n--------------------------------\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[start] start path from close waypoint\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[stop] finish actual (ie. currently eddited) path\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[continue <index>] make 'index' path actual or path on close waypoint if no 'index'\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[add] add close waypoint to actual path\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[autoadd <on/off>] auto add every waypoint player \"touch\"\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[insert <wpt#> <path#> <between1> <between2>] insert waypoint into path between its two waypoints\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[remove <index>] remove close waypoint from actual or 'index' path\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[delete <index>] delete whole 'index' path or path on close waypoint\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[reset <index>] reset 'index' path or path on close waypoint back to default\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[setteam <team> <index>] set team for 'index' path or path on close waypoint\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[setclass <class> <index>] set class for 'index' path or path on close waypoint\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[setdirection <dir> <index>] set direction for 'index' path or path on close waypoint\n");
				ClientPrint(pEntity, HUD_PRINTCONSOLE, "[setmisc <misc> <index>] set additional flags for 'index' path or path on close waypoint\n");
			}
			else if (FStrEq(arg1, "on"))
			{
				g_path_waypoint_on = TRUE;	// turn paths on
				g_waypoint_on = TRUE;	// turn wayponits on too just in case

				ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoint paths are ON\n");
			}
			else if (FStrEq(arg1, "off"))
			{
				g_path_waypoint_on = FALSE;

				ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoint paths are OFF\n");
			}
			else if (FStrEq(arg1, "show"))
			{
				if (g_path_waypoint_on == FALSE)
				{
					g_path_waypoint_on = TRUE;
					g_waypoint_on = TRUE;
				}
				else
					g_path_waypoint_on = FALSE;	// turn paths off

				ClientPrint(pEntity, HUD_PRINTNOTIFY, "***toggle mode used***\n");

				// print proper msg
				if (g_path_waypoint_on)
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoint paths are ON\n");
				else
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "waypoint paths are OFF\n");
			}
			else if (FStrEq(arg1, "save"))
			{
				if (WaypointPathSave(NULL))
				{
					PlaySoundConfirmation(pEntity, SND_DONE);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "paths successfully saved\n");
				}
			}
			else if (FStrEq(arg1, "load"))
			{
				if ((arg2 != NULL) && (*arg2 != 0))
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
					if (WaypointPathLoad(pEntity, NULL))
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
				if ((arg2 != NULL) && (*arg2 != 0))
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
				if ((arg2 != NULL) && (*arg2 != 0))
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
				if ((arg2 != NULL) && (*arg2 != 0))
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
					PlaySoundConfirmation(pEntity, SND_DONE);
					
					sprintf(msg, "total of %d invalid paths has been successfully removed\n",
						result);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
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

					sprintf(msg, "total of %d bugs/possible problems with paths have been found\n***TIP: use 'pathwpt highlight' to locate the problem paths***\n",
						result);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
				}
			}
			else if (FStrEq(arg1, "highlight"))
			{
				if ((arg2 != NULL) && (*arg2 != 0))
				{
					int path;

					if (FStrEq(arg2, "red"))
						path = HIGHLIGHT_RED;
					else if (FStrEq(arg2, "blue"))
						path = HIGHLIGHT_BLUE;
					else if ((FStrEq(arg2, "oneway")) || (FStrEq(arg2, "one-way")) || (FStrEq(arg2, "one_way")))
						path = HIGHLIGHT_ONEWAY;
					else
						path = atoi(arg2) - 1;

					if ((path == HIGHLIGHT_RED) || (path == HIGHLIGHT_BLUE) ||
						(path == HIGHLIGHT_ONEWAY) || ((path >= 0) && (path < MAX_W_PATHS)))
					{
						if (path == HIGHLIGHT_RED)
						{
							highlight_this_path = path;
							g_waypoint_on = TRUE;			// force displaying waypoints and
							g_path_waypoint_on = TRUE;		// paths too

							PlaySoundConfirmation(pEntity, SND_DONE);
							ClientPrint(pEntity, HUD_PRINTNOTIFY, "path highlighting is ON!\nall red team possible paths will be drawn\n");
						}
						else if (path == HIGHLIGHT_BLUE)
						{
							highlight_this_path = path;
							g_waypoint_on = TRUE;
							g_path_waypoint_on = TRUE;

							PlaySoundConfirmation(pEntity, SND_DONE);
							ClientPrint(pEntity, HUD_PRINTNOTIFY, "path highlighting is ON!\nall blue team possible paths will be drawn\n");
						}
						else if (path == HIGHLIGHT_ONEWAY)
						{
							highlight_this_path = path;
							g_waypoint_on = TRUE;
							g_path_waypoint_on = TRUE;

							PlaySoundConfirmation(pEntity, SND_DONE);
							ClientPrint(pEntity, HUD_PRINTNOTIFY, "path highlighting is ON!\nall one-way paths will be drawn\n");
						}
						else if (w_paths[path] != NULL)
						{
							highlight_this_path = path;
							g_waypoint_on = TRUE;
							g_path_waypoint_on = TRUE;

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
						ClientPrint(pEntity, HUD_PRINTNOTIFY, "valid arguments are: red, blue, oneway and index (where index is path number ie. a number between 1 and 512)\n");
					}
				}
				else
				{
					// turn it off
					highlight_this_path = -1;

					PlaySoundConfirmation(pEntity, SND_DONE);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "path highlighting is OFF!\n");
				}
			}
			else if (FStrEq(arg1, "start"))
			{
				// turn paths on
				if (g_path_waypoint_on == FALSE)
				{
					g_path_waypoint_on = TRUE;
					g_waypoint_on = TRUE;  // turn this on just in case
				}

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

				if ((arg2 != NULL) && (*arg2 != 0))
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
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "that path doesn't exist or not close enough to any waypoint!\n");
				}
			}
			else if (FStrEq(arg1, "autoadd"))
			{
				// check if exist arg2 and is valid
				if (FStrEq(arg2, "on"))
					g_auto_path = TRUE;
				else if (FStrEq(arg2, "off"))
					g_auto_path = FALSE;
				else
				{
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "***toggle mode used***\n");

					if (g_auto_path == FALSE)
						g_auto_path = TRUE;
					else
						g_auto_path = FALSE;	// turn auto adding off
				}

				// print proper msg
				if (g_auto_path)
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "autoadd into path is ON\n");
				else
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "autoadd into path is OFF\n");
			}
			else if (FStrEq(arg1, "insert"))
			{
				if ((arg2 == NULL) || (*arg2 == 0))
				{
					PlaySoundConfirmation(pEntity, SND_FAILED);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "missing arguments check for MB help!\n");

					return;
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

				return;
			}
			else if (FStrEq(arg1, "remove"))
			{
				bool result = FALSE;

				if ((arg2 != NULL) && (*arg2 != 0))
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
			else if (FStrEq(arg1, "delete"))
			{
				bool result = FALSE;

				if ((arg2 != NULL) && (*arg2 != 0))
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

				if ((arg2 != NULL) && (*arg2 != 0))
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
				int result = -128;

				if ((arg3 != NULL) && (*arg3 != 0))
				{
					int path_index = atoi(arg3);

					// is it valid path index (+1 due to array style)
					if ((path_index >= 1) && (path_index < num_w_paths + 1))
						result = WptPathChangeTeam(pEntity, arg2, path_index -1);
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
				int result;

				if ((arg3 != NULL) && (*arg3 != 0))
				{
					int path_index = atoi(arg3);

					// is it valid path index (+1 due to array style)
					if ((path_index >= 1) && (path_index < num_w_paths + 1))
						result = WptPathChangeClass(pEntity, arg2, path_index -1);
				}
				else if ((FStrEq(arg2, "help")) || (FStrEq(arg2, "?")))
				{
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "All valid class options\n-----------------------------\n");
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<all> all bots will use it - DEFAULT\n");
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<sniper> only snipers will use it\n");
					ClientPrint(pEntity, HUD_PRINTCONSOLE, "<mgunner> only mgunners will use it\n");
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
				int result;

				if ((arg3 != NULL) && (*arg3 != 0))
				{
					int path_index = atoi(arg3);

					// is it valid path index (+1 due to array style)
					if ((path_index >= 1) && (path_index < num_w_paths + 1))
						result = WptPathChangeWay(pEntity, arg2, path_index -1);
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
				int result;

				if ((arg3 != NULL) && (*arg3 != 0))
				{
					int path_index = atoi(arg3);

					// is it valid path index (+1 due to array style)
					if ((path_index >= 1) && (path_index < num_w_paths + 1))
						result = WptPathChangeMisc(pEntity, arg2, path_index -1);
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

//#ifdef _DEBUG

			else if (FStrEq(arg1, "flags"))
			{
				extern int WaypointGetPathLength(int path_index);
				W_PATH *p;
				char *team_fl, *class_fl, *way_fl;
				char misc[128];
				extern int g_path_to_continue;
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
					path_index = g_path_to_continue;

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

					strcpy(misc,"NOTHING");

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
				extern int g_path_to_continue;
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
					path_index = g_path_to_continue;

				if (path_index == -1 || w_paths[path_index] == NULL)
					return;

				sprintf(msg, "processing path no. %d\n", path_index + 1);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

				int safety = 0;

				W_PATH *p = w_paths[path_index];//head node

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

				if (g_path_waypoint_on)
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "pathwaypointing is ON\n");
				else
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "pathwaypointing is OFF\n");
			}

			return;
		}
		else if (FStrEq(pcmd, "vguimenuoption") && (g_menu_state != MENU_NONE))
		{
			// whole marinebot menu (the hud menu, not supported in FA2.8 and above)
			MBMenuSystem(pEntity, arg1);
		}
#ifdef _DEBUG
		else
			// nearly all debugging commands
			if (CheckForSomeDebuggingCommands(pEntity, pcmd, arg1, arg2, arg3, arg4, arg5))
				return;
#endif
	}

	(*other_gFunctionTable.pfnClientCommand)(pEntity);
}

void ClientUserInfoChanged( edict_t *pEntity, char *infobuffer )
{
#ifdef _DEBUG
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "ClientUserInfoChanged: pEntity=%x infobuffer=%s\n", pEntity, infobuffer); fclose(fp); }
#endif
	
	/*/
#ifdef _DEBUG
	//@@@@@@@@@@@@@@@
	ALERT(at_console, "ClientUserInfoChanged: pEntity=%x infobuffer=%s\n", pEntity, infobuffer);
#endif
	/**/

	(*other_gFunctionTable.pfnClientUserInfoChanged)(pEntity, infobuffer);
}

void ServerActivate( edict_t *pEdictList, int edictCount, int clientMax )
{
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"ServerActivate: edictCount%d clientMax%d\n", edictCount, clientMax); fclose(fp); }

	(*other_gFunctionTable.pfnServerActivate)(pEdictList, edictCount, clientMax);

	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"ServerActivate(engine-returned): edictCount%d clientMax%d\n", edictCount, clientMax); fclose(fp); }
}

void ServerDeactivate( void )
{
	(*other_gFunctionTable.pfnServerDeactivate)();
}

void PlayerPreThink( edict_t *pEntity )
{
   (*other_gFunctionTable.pfnPlayerPreThink)(pEntity);
}

void PlayerPostThink( edict_t *pEntity )
{
	(*other_gFunctionTable.pfnPlayerPostThink)(pEntity);
}

void StartFrame( void )
{
	if (gpGlobals->deathmatch)
	{
		edict_t *pPlayer;
		static int i, index, player_index, bot_index;
		static float previous_time = -1.0;
		static float client_update_time = 0.0;
		clientdata_s cd;
		char msg[256];
		int count;
		bool round_ended = FALSE;
		
		// if a new map has started then (MUST BE FIRST IN StartFrame)...
		if ((gpGlobals->time + 0.1) < previous_time)
		{
			char filename[256];
			char mapname[64];

			bot_t::harakiri_moment = 0.0;



			/*/
			#ifdef _DEBUG
			//@@@@@@@@@@@@@@@@@@@@
			fp=fopen("!mb_engine_debug.txt","a");
			fprintf(fp, "\n<dll.cpp> StartFrame() - new map started (%s)\n", STRING(gpGlobals->mapname));
			fclose(fp);
			#endif
			/**/

			// if autobalancing is enabled reset its time at start of new map
			// changed by kota@
			if ((do_auto_balance == TRUE) && (is_dedicated_server))
				check_auto_balance = gpGlobals->time + externals.GetBalanceTime();

			// turn off autobalance override (ie do the autobalance again if it is enabled)
			override_auto_balance = FALSE;

			// if info message autosending is enabled reset its time at start of new map
			if ((check_send_info != -1.0) && (is_dedicated_server))
				check_send_info = gpGlobals->time + externals.GetInfoTime();

			// if one of these is on turn it off to prevent engine overloading
			if (check_aims)
				check_aims = FALSE;

			if (check_cross)
				check_cross = FALSE;

			if (check_ranges)
				check_ranges = FALSE;

#ifdef _DEBUG
			if (debug_user)
			{
				debug_user = FALSE;
				pRecipient = NULL;
			}
#endif

			// reset welcome messages with new map
			welcome_time = 0.0;
			welcome_sent = FALSE;
			welcome2_sent = FALSE;
			welcome3_sent = FALSE;

			// reset error warnings time
			// (the message itself is cleared right after it has been displayed)
			error_time = 0.0;

			// reset HUD message display time
			HUDmsg_time = 0.0;

			// show the presentation after some time from map change
			presentation_time = gpGlobals->time + 90.0;

			// check if mapname_marine.cfg file exists ie are there any
			// specific settings & classes for this map
			strcpy(mapname, STRING(gpGlobals->mapname));
			strcat(mapname, "_marine.cfg");
			
			UTIL_MarineBotFileName(filename, "mapcfgs", mapname);
			FILE *temp_fp = fopen(filename, "r");
			
			// check if the map specific .cfg exists
			if (temp_fp != NULL)
			{				
				// we don't need the file so we should close it
				fclose(temp_fp);

				// forces opening configuration file
				need_to_open_cfg = TRUE;

				// mark the bots "fully kicked" ie. no auto respawn at the start on new map
				for (index = 0; index < MAX_CLIENTS; index++)
				{
					bots[index].is_used = FALSE;
					bots[index].respawn_state = 0;
					bots[index].kick_time = 0.0;
				}
			}
			// otherwise we are using default .cfg file ("marine.cfg")
			else
			{
				// there was map specific .cfg for the previous map, but there's none
				// for this map so we have to read the default .cfg again
				if (using_default_cfg == FALSE)
				{
					need_to_open_cfg = TRUE;

					for (index = 0; index < MAX_CLIENTS; index++)
					{
						bots[index].is_used = FALSE;
						bots[index].respawn_state = 0;
						bots[index].kick_time = 0.0;
					}
				}
				// otherwise we are still using the same default .cfg
				// so we have to do just respawn existing bots
				else
				{
					count = 0;
					
					// mark the bots as needing to be respawned...
					for (index = 0; index < MAX_CLIENTS; index++)
					{
						if (count >= prev_num_bots)
						{
							bots[index].is_used = FALSE;
							bots[index].respawn_state = 0;
							bots[index].kick_time = 0.0;
						}
						
						if (bots[index].is_used)  // is this slot used?
						{
							bots[index].respawn_state = RESPAWN_NEED_TO_RESPAWN;
							count++;
						}

						// check for any bots that were very recently kicked...
						if ((bots[index].kick_time + 5.0) > previous_time)
						{
							bots[index].respawn_state = RESPAWN_NEED_TO_RESPAWN;
							count++;
						}
						else
							bots[index].kick_time = 0.0;  // reset to prevent false spawns later
					}
				}
			}

			// set the respawn time
			if (is_dedicated_server)
				respawn_time = gpGlobals->time + 5.0;
			else
				respawn_time = gpGlobals->time + 20.0;
			
			// start updating client data again
			client_update_time = gpGlobals->time + 10.0;

			bot_check_time = gpGlobals->time + 30.0;
		}

		// listen server
		if (!is_dedicated_server)
		{
			if ((listenserver_edict != NULL) && IsAlive(listenserver_edict))
			{
				// we found some fatal error so set error message time
				if ((error_occured) && (error_time < 1.0))
					error_time = gpGlobals->time + 2.0;

				// or we found only some non-fatal error
				// so set warning time and also delay welcome
				else if ((warning_event) && (error_time < 1.0))
				{
					error_time = gpGlobals->time + 2.0;
					welcome_time = error_time + 16.0;
				}

				// otherwise all went fine so set standard welcome message time
				else if ((error_occured == FALSE) && (warning_event == FALSE) &&
					(welcome_sent == FALSE) && (welcome_time < 1.0))
					welcome_time = gpGlobals->time + 2.0;
			}

			// keep this order of agrs, it's the best setting (I hope)
			// due to short circuit feature in C++
			if (((error_occured) || (warning_event)) &&
				(error_time > 0.0) && (error_time < gpGlobals->time))
			{
				// send the error message to client
				Vector color1 = Vector(250, 50, 0);
				Vector color2 = Vector(255, 0, 20);
				CustHudMessageToAll(error_msg, color1, color2, 2, 10);
				
				// reset the error message back to default
				strcpy(error_msg, "WARNING - MarineBot detected an error");

				// clear it, the message must have been already printed
				override_reset = FALSE;

				// clear this so we only do it once
				if (warning_event)
					warning_event = FALSE;

				if (error_occured)
				{
					error_occured = FALSE;
				
					// prevents to show standard welcome messages
					welcome_sent = TRUE;
					welcome2_sent = TRUE;
					welcome3_sent = TRUE;
				}
			}

			else if ((welcome_sent == FALSE) &&
				(welcome_time > 0.0) && (welcome_time < gpGlobals->time))
			{
				char hi_msg[256];
				sprintf(hi_msg, "MarineBot %s %s", mb_version_info, welcome_msg);

				// let's send a welcome message to client
				Vector color1 = Vector(200, 50, 0);
				Vector color2 = Vector(0, 250, 0);
				CustHudMessageToAll(hi_msg, color1, color2, 2, 8);
				
				welcome_sent = TRUE;

				// next welcome in 5 seconds after current message disappear
				welcome_time = gpGlobals->time + 12.0;
			}

			else if ((welcome2_sent == FALSE) && (welcome_sent) &&
				(welcome_time > 0.0) && (welcome_time < gpGlobals->time))
			{
				Vector color1 = Vector(200, 50, 0);
				Vector color2 = Vector(0, 250, 0);
				CustHudMessageToAll(welcome2_msg, color1, color2, 2, 8);

				welcome2_sent = TRUE;

				welcome_time = gpGlobals->time + 12.0;
			}

			else if ((welcome3_sent == FALSE) && (welcome2_sent) &&
				(welcome_time > 0.0) && (welcome_time < gpGlobals->time))
			{
				char welcome3_msg[256];
				bool print_this = FALSE;

				// get waypoint's author
				if ((wpt_author[0] != 0) &&
					(strcmp(wpt_author, "unknown") != 0))
				{
					if (IsOfficialWpt(wpt_author))
						sprintf(welcome3_msg, "Official MarineBot waypoints by %s", wpt_author);
					else if ((wpt_modified[0] != 0) && (IsOfficialWpt(wpt_modified)))
						sprintf(welcome3_msg, "Official MarineBot waypoints by %s", wpt_author);
					else
						sprintf(welcome3_msg, "Waypoints by %s",
							wpt_author);

					// we have something to be printed so do it
					print_this = TRUE;
				}

				if ((wpt_modified[0] != 0) &&
					(strcmp(wpt_modified, "unknown") != 0))
				{
					char temp[128];

					sprintf(temp, "were modified by %s", wpt_modified);

					sprintf(welcome3_msg, "%s\n%s", welcome3_msg, temp);

					print_this = TRUE;
				}

				// do we have anything to print on screen?
				if (print_this)
				{
					Vector color1 = Vector(200, 50, 0);
					Vector color2 = Vector(0, 250, 0);
					CustHudMessageToAll(welcome3_msg, color1, color2, 2, 8);
				}

				welcome3_sent = TRUE;

				welcome_time = gpGlobals->time + 13.0;
			}
		}
		// dedicated server
		else
		{
			if ((welcome_sent == FALSE) && (welcome_time < gpGlobals->time) &&
				(welcome_time > 1.0))
			{
				PrintOutput(NULL, "\n", msg_null);
				PrintOutput(NULL, "write [m_bot help] or [m_bot ?] into your console for command help\n", msg_info);

				welcome_sent = TRUE;	// do it only once
			}
		}
		
		// do DS initialization only once
		if ((Dedicated_Server_Init == FALSE) && (is_dedicated_server))
		{
			Dedicated_Server_Init = TRUE;		// run only once

			// init auto balance checks time
			// changed by kota@
			if (do_auto_balance == true)
				check_auto_balance = gpGlobals->time + externals.GetBalanceTime();

			// sent info about console help into DS console some time after start
			if (welcome_time == 0.0)
				welcome_time = gpGlobals->time + 6.5;
			
			// init info msg time
			if (check_send_info == 0.0)
				check_send_info = gpGlobals->time + externals.GetInfoTime();

			// show the presentation message after some time from server intialization
			presentation_time = gpGlobals->time + 90.0;

			// use current bot version for this message
			sprintf(presentation_msg, "%s %s", presentation_msg, mb_version_info);
		}
		
		// is it time to update weapons & equipment
		if (client_update_time <= gpGlobals->time)
		{
			client_update_time = gpGlobals->time + 1.0;

			for (i=0; i < MAX_CLIENTS; i++)
			{
				if (bots[i].is_used)
				{
					memset(&cd, 0, sizeof(cd));

					UpdateClientData( bots[i].pEdict, 1, &cd );

					// see if a weapon was dropped...
					if (bots[i].bot_weapons != cd.weapons)
					{
						bots[i].bot_weapons = cd.weapons;
					}
				}
			}
		}

		count = 0;

		// run bot think method for each bot (ie. play for each bot)
		for (bot_index = 0; bot_index < gpGlobals->maxClients; bot_index++)
		{
			if ((bots[bot_index].is_used) &&  // is this slot used AND
				(bots[bot_index].respawn_state == RESPAWN_IDLE))  // not respawning
			{
				// if it is this bot we want to debug then turn the debugging on for him
				if (bots[bot_index].pEdict == g_debug_bot)
					g_debug_bot_on = TRUE;
				else
					g_debug_bot_on = FALSE;

				//BotThink(&bots[bot_index]); this is bot_t member now.
				bots[bot_index].BotThink();

				count++;
			}
		}

		if (count > num_bots)
			num_bots = count;

		// show/update waypoints, paths & stuff for player
		for (player_index = 1; player_index <= gpGlobals->maxClients; player_index++)
		{
			pPlayer = INDEXENT(player_index);

			if (pPlayer && !pPlayer->free)
			{
				// are the waypoints on AND this client is a human AND is alive
				// this should prevent overload on netchan error
				if ((g_waypoint_on) && FBitSet(pPlayer->v.flags, FL_CLIENT) &&
					!FBitSet(pPlayer->v.flags, FL_FAKECLIENT) && IsAlive(pPlayer))
				{
					WaypointThink(pPlayer);
				}
			}
		}

		// run a check for possible round end
		for (index = 0; index < MAX_CLIENTS; index++)
		{
			if (clients[index].pEntity != NULL)
			{
				if ((clients[index].GetMaxSpeedTime() + 0.5) < gpGlobals->time)
				{
					round_ended = FALSE;
					break;
				}
				else
					round_ended = TRUE;
			}
		}

		// check if we can update waypoint data (special event check)
		if (round_ended)
		{
			WaypointResetTriggers();
		}

		// check if we can update waypoint data yet (regular check)
		if (f_update_wpt_time < gpGlobals->time)
		{
			UpdateWaypointData();
			
			f_update_wpt_time = gpGlobals->time + 2.0;
		}


#ifdef _DEBUG

		// TEMP: just for testing various stuff
		if (debug_usr_weapon)
		{
			/**/
			sprintf(msg, "Active %d | ID %d | Clip %d | Attach(silencer) %d | FireMode %d\n",
					usr_current_weapon.isActive, usr_current_weapon.iId,
					usr_current_weapon.iClip, usr_current_weapon.iAttachment,
					usr_current_weapon.iFireMode);
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, msg);
			/**/
		}

		// TEMP: just for testing various stuff
		if (debug_user)
		{
			/**/
			sprintf(msg, "Speed (velocity) %.1f ||Movetype %d ||Flags %d ||Pitch %.2f ||IdealPitch %.2f ||Yaw %.2f ||IdealYaw %.2f\n",
				pRecipient->v.velocity.Length2D(), pRecipient->v.movetype, pRecipient->v.flags,
				pRecipient->v.v_angle.x, pRecipient->v.idealpitch, pRecipient->v.v_angle.y, 
				pRecipient->v.ideal_yaw);
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, msg);

			/**/

			/*/
			bool notdrawn = FALSE;

			if (pRecipient->v.effects & EF_NODRAW)
				notdrawn = TRUE;

			sprintf(msg, "v.effects %d || notdrawn %d\n",
				pRecipient->v.effects, notdrawn);
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, msg);

			/**/

			/*/
			sprintf(msg, "iu1=%d | iu2=%d | iu3=%d | iu4=%d |**| fu1=%.2f | fu2=%.2f | fu3=%.2f | fu4=%.2f |**| fov=%.2f |**| flags=%d | speed=%.1f\n",
				pRecipient->v.iuser1, pRecipient->v.iuser2, pRecipient->v.iuser3, pRecipient->v.iuser4,
				pRecipient->v.fuser1, pRecipient->v.fuser2, pRecipient->v.fuser3, pRecipient->v.fuser4,
				pRecipient->v.fov, pRecipient->v.flags, pRecipient->v.velocity.Length2D());
			ClientPrint(pRecipient, HUD_PRINTCONSOLE, msg);
			
			/**/

			/*/
			Vector temp = GetGunPosition(pRecipient);

			sprintf(msg, "Head x=%.1f y=%.1f z=%.1f || Body x=%.1f y=%.1f z=%.1f || Gun x=%.1f y=%.1f z=%.1f\n",
				pRecipient->v.origin.x, pRecipient->v.origin.y, pRecipient->v.origin.z,
				pRecipient->v.view_ofs.x, pRecipient->v.view_ofs.y, pRecipient->v.view_ofs.z,
				temp.x, temp.y, temp.z);
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, msg);

			/**/

			/*/

			sprintf(msg, "vu1 %.1f | vu2 %.1f | vu3 %.1f | vu4 %.1f |**| FOV=%d\n",
				pRecipient->v.vuser1.Length2D(), pRecipient->v.vuser2.Length2D(),
				pRecipient->v.vuser3.Length2D(), pRecipient->v.vuser4.Length2D(),
				pRecipient->v.fov);
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, msg);

			/**/
		  
			/*

			//Vector v_head = pRecipient->v.origin + pRecipient->v.view_ofs;
			//if (pRecipient->v.pContainingEntity != NULL)
			//sprintf(msg, "pContEnt %s ||Zhead ofs %.3f\n",
			//STRING(pRecipient->v.pContainingEntity->v.netname),
			//(float) v_head.z);

			//sprintf(msg, "dmg_take-%.2f ||dmg_save-%.2f ||dmg-%.2f ||dmgtime-%.2f\n",
			//  pRecipient->v.dmg_take, pRecipient->v.dmg_save, pRecipient->v.dmg,
			// pRecipient->v.dmgtime);

			float xlim, ylim;
			bool Isbipod = FALSE;

			
			xlim = fabs(pRecipient->v.vuser1.x - pRecipient->v.v_angle.x);
			ylim = fabs(pRecipient->v.vuser1.y - pRecipient->v.v_angle.y);

#define ORIG_BIPOD		(1<<7)	// value in FA2.8 and lower
#define BIPOD			(1<<5)	// value in FA2.9
			
			if (((pEdict->v.iuser3 & ORIG_BIPOD) && (g_mod_version != FA29)) ||
				((pEdict->v.iuser3 & BIPOD) && (g_mod_version == FA29)))
				Isbipod = TRUE;

			sprintf(msg, "Viewmodel %d ||Weaponanim %d ||Maxspeed %.1f ||FOV %.1f ||BipodXlimit %.2f ||BipodYlimit %.2f ||Isbipod %d\n",
				pRecipient->v.viewmodel, pRecipient->v.weaponanim, pRecipient->v.maxspeed,
				pRecipient->v.fov, xlim, ylim, Isbipod);
			/**/
			/*/
			sprintf(msg, "bInDuck %d ||flTimeStepSound %d ||flSwimTime %.1f ||flDuckTime %d ||flFallVelocity %.1f\n",
			  pRecipient->v.bInDuck, pRecipient->v.flTimeStepSound, pRecipient->v.flSwimTime,
			  pRecipient->v.flDuckTime, pRecipient->v.flFallVelocity);
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, msg);
			//sprintf(msg, "speed %.1f ||air_finished %.1f ||pain_finished %.1f ||radsuit_finished %.1f\n",
			//  pRecipient->v.speed, pRecipient->v.air_finished,
			//  pRecipient->v.pain_finished, pRecipient->v.radsuit_finished);

			//sprintf(msg, "max_health %.2f ||teleport_time %.2f ||armortype %.2f ||armorvalue %.2f ||waterlevel %d ||watertype %d\n",
			//pRecipient->v.max_health, pRecipient->v.teleport_time, pRecipient->v.armortype,
			//pRecipient->v.armorvalue, pRecipient->v.waterlevel, pRecipient->v.watertype);

			//ClientPrint(pRecipient, HUD_PRINTCONSOLE, msg);

			//sprintf(msg, "button %d ||impulse %d ||health %.1f ||takedamage %.1f|| frags %.1f\n",
			//pRecipient->v.button, pRecipient->v.impulse,
			//pRecipient->v.health, pRecipient->v.takedamage, pRecipient->v.frags);

			/**/

			/*/
			if (pRecipient->v.movetype == MOVETYPE_WALK)
			//	ClientPrint(pRecipient, HUD_PRINTNOTIFY, "MOVETYPE_WALK\n");
				;	//no need it's nearly all the time
			else if (pRecipient->v.movetype == MOVETYPE_STEP)
				ClientPrint(pRecipient, HUD_PRINTNOTIFY, "MOVETYPE_STEP\n");
			else if (pRecipient->v.movetype == MOVETYPE_FLY)
				ClientPrint(pRecipient, HUD_PRINTNOTIFY, "MOVETYPE_FLY\n");
			else if (pRecipient->v.movetype == MOVETYPE_TOSS)
				ClientPrint(pRecipient, HUD_PRINTNOTIFY, "MOVETYPE_TOSS\n");
			else if (pRecipient->v.movetype == MOVETYPE_PUSH)
				ClientPrint(pRecipient, HUD_PRINTNOTIFY, "MOVETYPE_PUSH\n");
			else if (pRecipient->v.movetype == MOVETYPE_NOCLIP)
				ClientPrint(pRecipient, HUD_PRINTNOTIFY, "MOVETYPE_NOCLIP\n");
			else if (pRecipient->v.movetype == MOVETYPE_FLYMISSILE)
				ClientPrint(pRecipient, HUD_PRINTNOTIFY, "MOVETYPE_FLYMISSILE\n");
			else if (pRecipient->v.movetype == MOVETYPE_BOUNCE)
				ClientPrint(pRecipient, HUD_PRINTNOTIFY, "MOVETYPE_BOUNCE\n");
			else if (pRecipient->v.movetype == MOVETYPE_BOUNCEMISSILE)
				ClientPrint(pRecipient, HUD_PRINTNOTIFY, "MOVETYPE_BOUNCEMISSILE\n");
			else if (pRecipient->v.movetype == MOVETYPE_FOLLOW)
				ClientPrint(pRecipient, HUD_PRINTNOTIFY, "MOVETYPE_FOLLOW\n");
			else if (pRecipient->v.movetype == MOVETYPE_PUSHSTEP)
				ClientPrint(pRecipient, HUD_PRINTNOTIFY, "MOVETYPE_PUSHSTEP\n");
			else if (pRecipient->v.movetype == MOVETYPE_NONE)
				ClientPrint(pRecipient, HUD_PRINTNOTIFY, "MOVETYPE_NONE\n");
			else
			{
				sprintf(msg, "***unknown MOVETYPE_ | its value %d\n", pRecipient->v.movetype);
				ClientPrint(pRecipient, HUD_PRINTNOTIFY, msg);
			}
			/**/

		}
#endif


		// look if team balance is needed and is it time for it
		// NOTE: SHOULD BE CHANGED TO DIFF _TIME NAME BECAUSE NOW DOBALANCE WHILE BOTS RESPAWNING!!!!!!!!
		if ((team_balance_value > 0) && (respawn_time <= gpGlobals->time))
		{
			int balance_in_progress;

			balance_in_progress = UTIL_DoTheBalance();

			// is still balancing teams in progress
			if (balance_in_progress == 0)
				respawn_time = gpGlobals->time + 2.0;
			else
				respawn_time = 0.0;
		}

		// are we currently respawning bots and is it time to spawn one yet?
		if ((respawn_time > 1.0) && (respawn_time <= gpGlobals->time))
		{
			int index = 0;

			// find bot needing to be respawned...
			while ((index < MAX_CLIENTS) && (bots[index].respawn_state != RESPAWN_NEED_TO_RESPAWN))
				index++;

			if (index < MAX_CLIENTS)
			{
				bots[index].respawn_state = RESPAWN_IS_RESPAWNING;
				bots[index].is_used = FALSE;      // free up this slot

				// respawn 1 bot then wait a while (otherwise engine might crash)
				if (mod_id == FIREARMS_DLL)
				{
					// these will temporary hold some settings
					char c_team[2];
					char c_class[3];
					char c_skin[2];
					char c_skill[2];
					char c_name[BOT_NAME_LEN+1];
					int temp_bot_skill = externals.GetSpawnSkill();
					int aim_skill = externals.GetSpawnSkill();

					// store the settings
					sprintf(c_team, "%d", bots[index].bot_team);
					sprintf(c_class, "%d", bots[index].bot_class);
					sprintf(c_skin, "%d", bots[index].bot_skin);
					sprintf(c_skill, "%d", bots[index].bot_skill);
					strncpy(c_name, bots[index].name, BOT_NAME_LEN-1);
					c_name[BOT_NAME_LEN] = 0;  // make sure c_name is null terminated
					aim_skill = bots[index].aim_skill;

					// fix the right skill value (due array based style)
					temp_bot_skill = atoi(c_skill);
					temp_bot_skill++;
					sprintf(c_skill, "%d", temp_bot_skill);

					BotCreate(NULL, c_team, c_class, c_skin, c_skill, bots[index].name);

					// set back stored settings
					bots[index].aim_skill = aim_skill;
				}

				respawn_time = gpGlobals->time + 0.5;  // set next respawn time

				bot_check_time = gpGlobals->time + 0.5;		// time to next adding
			}
			else
			{
				respawn_time = 0.0;
			}
		}

		if (g_GameRules)
		{
			if (need_to_open_cfg)  // have we open marine.cfg file yet?
			{



				/*/
				#ifdef _DEBUG
				//@@@@@@@@@@@@@@@@@@@@
				fp=fopen("!mb_engine_debug.txt","a");
				fprintf(fp, "\n<dll.cpp> StartFrame() - g_GameRules - need_to_open_cfg\n");
				fclose(fp);
				#endif
				/**/



				char filename[256];
				char mapname[64];

				need_to_open_cfg = FALSE;  // only do this once!!!

				// if there is some previous conf then flush it
				if (::conf != NULL)
				{
					delete ::conf;



				/*/
				#ifdef _DEBUG
				//@@@@@@@@@@@@@@@@@@@@
				fp=fopen("!mb_engine_debug.txt","a");
				fprintf(fp, "\n<dll.cpp> StartFrame() - g_GameRules - need_to_open_cfg - conf has been erased\n");
				fclose(fp);
				#endif
				/**/



				}
				::conf = NULL;

				// allows us to read the whole configuration file
				read_whole_cfg = TRUE;

				// if conf pointer is NULL then read the config file again
				if (conf == NULL)
				{
					// check if mapname_marine.cfg file exists
					strcpy(mapname, STRING(gpGlobals->mapname));
					strcat(mapname, "_marine.cfg");
					
					UTIL_MarineBotFileName(filename, "mapcfgs", mapname);

					// SECTION - for new config system by Andrey Kotrekhov
					if ((conf = parceConfig(filename)) != NULL)
					{
						sprintf(msg, "Executing %s\n", filename);
						PrintOutput(NULL, msg, msg_info);

						// to know that we changed to map specific .cfg
						// we will need to read the default "marine.cfg" again once the map
						// changes and there isn't any map specific .cfg for that map
						using_default_cfg = FALSE;




						/*/
						#ifdef _DEBUG
						//@@@@@@@@@@@@@@@@@@@@
						fp=fopen("!mb_engine_debug.txt","a");
						fprintf(fp, "\n<dll.cpp> StartFrame() - opening %s - this is CUSTOM cfg\n", filename);
						fclose(fp);
						#endif
						/**/




					}
					else
					{
						UTIL_MarineBotFileName(filename, "marine.cfg", NULL);

						sprintf(msg, "Executing %s\n", filename);
						PrintOutput(NULL, msg, msg_info);

						conf = parceConfig(filename);

						using_default_cfg = TRUE;

						if (conf == NULL)
						{
							// print error message directly into DS console
							if (is_dedicated_server)
							{
								PrintOutput(NULL, "MARINE_BOT CRITICAL ERROR - There is a syntax error in marine.cfg, or the file cant be found\n", msg_null);
								PrintOutput(NULL, "Bots may join or play incorrectly\n", msg_warning);
							}
							// print error message once client join the game
							else
							{
								strcat(error_msg, ": error in configuration file!\nThere is a syntax error in marine.cfg, or the file cant be found\nBots may join or play incorrectly");
								
								error_occured = TRUE;
							}
						}
						



						/*/
						#ifdef _DEBUG
						//@@@@@@@@@@@@@@@@@@@@
						fp=fopen("!mb_engine_debug.txt","a");
						fprintf(fp, "\n<dll.cpp> StartFrame() - opening %s - this is DEFAULT cfg\n", filename);
						fclose(fp);
						#endif
						/**/





					}
					// SECTION BY Andrey Kotrekhov END
				}

				// set the respawn time again, just for sure
				if (is_dedicated_server)
					bot_cfg_pause_time = gpGlobals->time + 5.0;
				else
					bot_cfg_pause_time = gpGlobals->time + 20.0;
			}

			if (!is_dedicated_server && !spawn_time_reset)
			{
				if (listenserver_edict != NULL)
				{
					if (IsAlive(listenserver_edict))
					{
						spawn_time_reset = TRUE;

						if (respawn_time >= 1.0)
							respawn_time = min(respawn_time, gpGlobals->time + (float)1.0);

						if (bot_cfg_pause_time >= 1.0)
							bot_cfg_pause_time = min(bot_cfg_pause_time, gpGlobals->time + (float)1.0);
					}
				}
			}

			// for new config system by Andrey Kotrekhov
			if ((conf != NULL) &&
				(bot_cfg_pause_time >= 1.0) && (bot_cfg_pause_time <= gpGlobals->time))
			{
				// process bot.cfg file options
				ProcessBotCfgFile(conf);
			}	
		}
      
		// check if it is DS and if it is time to see if a bot needs to be created or kicked
		if ((is_dedicated_server) && (bot_check_time < gpGlobals->time))
		{			
			bot_check_time = gpGlobals->time + 1.5;	// time to next check

			// if there are currently LESS than the maximum number of "players"
			// then add another bot using the default skill level...

			// if auto balance is ENABLED do team balancing right on join
			// changed by kota@
			if ((clients[0].ClientCount() < externals.GetMaxBots()) &&
				(externals.GetMaxBots() != -1) && (do_auto_balance == TRUE))
			{
				int reds, blues;

				reds = blues = 0;
				for (int i = 0; i < MAX_CLIENTS; i++)
				{
					if (clients[i].pEntity != NULL)
					{
						if (UTIL_GetTeam(clients[i].pEntity) == TEAM_RED)
							reds++;
						else if (UTIL_GetTeam(clients[i].pEntity) == TEAM_BLUE)
							blues++;
					}
				}

				// create bot (red, blue, random) based on team member count
				if (reds < blues)
					BotCreate( NULL, "1", NULL, NULL, NULL, NULL );
				else if (reds > blues)
					BotCreate( NULL, "2", NULL, NULL, NULL, NULL );
				else
					BotCreate( NULL, NULL, NULL, NULL, NULL, NULL );
			}
			// otherwise do random join
			else if ((clients[0].ClientCount() < externals.GetMaxBots()) &&
				(externals.GetMaxBots() != -1))
				BotCreate( NULL, NULL, NULL, NULL, NULL, NULL );

			// if there are currently MORE than the maximum number of "players"
			// and we can kick a bot then kick one

			// if auto balance is ENABLED then try to kick bot from "stronger" team
			if ((clients[0].ClientCount() > externals.GetMaxBots()) &&
				(externals.GetMaxBots() != -1) && (externals.GetMinBots() != -1) &&
				(clients[0].BotCount() > externals.GetMinBots()) &&
				(override_max_bots == FALSE) &&
				(do_auto_balance == TRUE))
			{
				int reds, blues;

				reds = blues = 0;
				for (int i = 0; i < MAX_CLIENTS; i++)
				{
					if (clients[i].pEntity != NULL)
					{
						if (UTIL_GetTeam(clients[i].pEntity) == TEAM_RED)
							reds++;
						else if (UTIL_GetTeam(clients[i].pEntity) == TEAM_BLUE)
							blues++;
					}
				}

				// the bot that has to be kicked is based on the number of members in each team

				// if the blue team is stronger then try to kick a blue bot
				if (reds < blues)
				{
					// wasn't the try to kick one blue bot successful?
					if (UTIL_KickBot(100 + TEAM_BLUE) == FALSE)
						// then kick random bot
						UTIL_KickBot(-100);
				}
				// otherwise try to kick a red bot
				else
				{
					if (UTIL_KickBot(100 + TEAM_RED) == FALSE)
						UTIL_KickBot(-100);
				}
			}
			// otherwise kick a random bot
			else if ((clients[0].ClientCount() > externals.GetMaxBots()) &&
				(externals.GetMaxBots() != -1) && (externals.GetMinBots() != -1) &&
				(clients[0].BotCount() > externals.GetMinBots()) &&
				(override_max_bots == FALSE))
			{
				UTIL_KickBot(-100);
			}
		}
		
		// is time to automatically check team balance AND we are allowed to do autobalancing
		// changed by kota@
		else if ((check_auto_balance < gpGlobals->time) && (do_auto_balance == TRUE) &&
			(override_auto_balance == FALSE))
		{
			if (externals.GetBalanceTime() == 0.0)
			{
				check_auto_balance = -1.0;
				do_auto_balance = FALSE;		// added by kota@

				PrintOutput(NULL, "auto balance DISABLED!\n", msg_default);
			}
			else
			{
				check_auto_balance = gpGlobals->time + externals.GetBalanceTime();

				team_balance_value = UTIL_BalanceTeams();				

				// we don't want to print the message into listen server console
				if (is_dedicated_server)
				{
					PrintOutput(NULL, "\n", msg_null);	// first seperate this message

					if (team_balance_value == -2)
						PrintOutput(NULL, "server is empty!\n", msg_warning);
					else if (team_balance_value == -1)
						PrintOutput(NULL, "there are no bots!\n", msg_warning);
					else if (team_balance_value == 0)
						PrintOutput(NULL, "teams are balanced\n", msg_info);
					else if (team_balance_value > 100)
					{
						sprintf(msg, "balancing in progress... (moving %d bots from RED team to blue)\n", team_balance_value - 100);
						PrintOutput(NULL, msg, msg_info);
					}
					else if ((team_balance_value > 0) && (team_balance_value < 100))
					{
						sprintf(msg, "balancing in progress... (moving %d bots from BLUE team to red)\n", team_balance_value);
						PrintOutput(NULL, msg, msg_info);
					}
					else if (team_balance_value < -2)
						PrintOutput(NULL, "internal error\n", msg_error);
				}
			}
		}
		
		// is time to automatically send information message AND can we do it
		else if ((check_send_info < gpGlobals->time) && (check_send_info > 1.0) &&
			(is_dedicated_server))
		{
			bool more_details = TRUE;
			int clients_info = UTIL_BalanceTeams();

			PrintOutput(NULL, "\n", msg_null);	// seperate this msg

			if (clients_info == -2)
			{
				PrintOutput(NULL, "there are NO players or bots (server is EMPTY)!\n", msg_warning);

				more_details = FALSE;
			}
			else if (clients_info == -1)
			{
				PrintOutput(NULL, "there are NO bots!\n", msg_warning);
			}
			else if (clients_info > 0)
			{
				PrintOutput(NULL, "teams are NOT balanced!\n", msg_warning);
			}

			if (more_details)
			{
				/*   USELESS
				int i, total_clients, players, the_bots;
				
				total_clients = players = the_bots = 0;

				
				// count human players and bots
				for (i = 0; i < MAX_CLIENTS; i++)
				{
					if (clients[i].pEntity == NULL)
						continue;

					if (clients[i].pEntity->v.flags & FL_FAKECLIENT)
						the_bots++;
					else
						players++;

					total_clients++;
				}
				*/

				// print correct message
				if (clients_info == -1)
				{
					sprintf(msg, "MARINE_BOT INFO - there are %d clients on the server (no bots)\n",
							//total_clients);
							clients[0].ClientCount());
					PrintOutput(NULL, msg, msg_null);
				}
				else
				{
					sprintf(msg, "MARINE_BOT INFO - there are %d clients on the server (%d humans and %d bots)\n",
							//total_clients, players, the_bots);
							clients[0].ClientCount(), clients[0].HumanCount(),
							clients[0].BotCount());
					PrintOutput(NULL, msg, msg_null);

					int j, k;
					char client_name[BOT_NAME_LEN];

					PrintOutput(NULL, "--------------MB skill values----------------------\n", msg_null);

					// print bot names and their skill levels
					for (j = 0; j < MAX_CLIENTS; j++)
					{
						if (bots[j].is_used == FALSE)
							continue;

						for (k = 0; k < MAX_CLIENTS; k++)
						{
							if (bots[j].pEdict == clients[k].pEntity)
								break;
						}
						
						strcpy(client_name, STRING(clients[k].pEntity->v.netname));

						sprintf(msg, "%s: botskill:%d and aimskill:%d\n", client_name,
								bots[j].bot_skill + 1, bots[j].aim_skill + 1);
						PrintOutput(NULL, msg, msg_null);
					}

					PrintOutput(NULL, "---------------------------------------------------\n", msg_null);
				}
			}

			check_send_info = gpGlobals->time + externals.GetInfoTime();
		}

		// do we need to fill listen server and is it time to add next bot
		else if ((server_filling) && (bot_check_time < gpGlobals->time))
		{
			bool bot_added = FALSE;
			bool server_full = FALSE;
			int bot_count = 0;
			int total_clients, reds, blues;
			int yet_to_fill = 0;		// if amount of bots was specified (i.e. "arg filling")

			total_clients = reds = blues = 0;

			// count clients in both teams
			for (int i = 0; i < MAX_CLIENTS; i++)
			{
				if (clients[i].pEntity != NULL)
				{
					total_clients++;

					if (clients[i].pEntity->v.team == TEAM_RED)
						reds++;
					else if (clients[i].pEntity->v.team == TEAM_BLUE)
						blues++;
				}
			}

			// is specified the amount of bots (i.e. "arg filling")
			if (bots_to_add > 0)
			{
				yet_to_fill = bots_to_add;
			}
			
			// first check if there are some free slots on the server
			if (total_clients >= gpGlobals->maxClients)
				server_filling = FALSE;
			// create bot (red, blue, random) based on team member count
			else if (reds < blues)
				bot_added = BotCreate( NULL, "1", NULL, NULL, NULL, NULL );
			else if (reds > blues)
				bot_added = BotCreate( NULL, "2", NULL, NULL, NULL, NULL );
			else
				bot_added = BotCreate( NULL, NULL, NULL, NULL, NULL, NULL );

			if (bot_added)
			{
				bot_check_time = gpGlobals->time + 0.5;	// time to next adding

				// decrease the amount of bots to be added (only if is "arg filling")
				if (yet_to_fill != 0)
				{
					bots_to_add--;

					// end filling if there is no bot to be added
					if (bots_to_add == 0)
						server_filling = FALSE;
				}
			}
		}

		// send presentation message to all clients of the server
		else if ((is_dedicated_server) &&
			(presentation_time < gpGlobals->time) && (presentation_time > 1.0))
		{
			presentation_time = gpGlobals->time + externals.GetPresentationTime();

			Vector color1 = Vector(200, 50, 0);
			Vector color2 = Vector(0, 250, 0);
			CustHudMessageToAll(presentation_msg, color1, color2, 2, 15);
		}

		// do save waypoints & paths automatically after given time
		else if (b_wpt_autosave && (wpt_autosave_time < gpGlobals->time))
		{
			// we don't want this message on DS
			if (is_dedicated_server == FALSE)
				PrintOutput(NULL, "***autosaving waypoints and paths***\n", msg_null);

			// check if all went fine
			if (WaypointAutoSave())
				wpt_autosave_time = gpGlobals->time + wpt_autosave_delay;
			// otherwise try it again sooner
			else
			{
				wpt_autosave_time = gpGlobals->time + 30.0;

				if (is_dedicated_server == FALSE)
					PrintOutput(NULL, "***waypoints and paths weren't saved***\n", msg_null);
			}
		}



		//@@@@@@@@@@@@@@@@@@@@@@@@@@@
		// just for tests
#ifdef _DEBUG

		if ((deathfall_check) && (deathfallcheck_time < gpGlobals->time))
		{
			if (IsDeathFall(listenserver_edict) == FALSE)
			{
				PlaySoundConfirmation(listenserver_edict, SND_DONE);
			}
			else
				PlaySoundConfirmation(listenserver_edict, SND_FAILED);

			deathfallcheck_time = gpGlobals->time + 1.0;
		}

		if (test_origin)
		{
			/*
			for (int i = 0; i < gpGlobals->maxClients; i++)
			{
				if (bots[i].is_used)
				{
					edict_t *pThisBot = bots[i].pEdict;
					Vector start = pThisBot->v.origin + pThisBot->v.view_ofs;
					Vector end;

					if (bots[i].pBotEnemy != NULL)
						end = bots[i].pBotEnemy->v.origin + bots[i].pBotEnemy->v.view_ofs;
					else
						end = pThisBot->v.origin + pThisBot->v.view_ofs;
#ifdef _DEBUG
					// draw it for listen server client
					ReDrawBeam(listenserver_edict, start, end, pThisBot->v.team);
#endif
				}
			}
			*/

			/*
			char str[128];

			sprintf(str, "angles_x %0.2f | angles_y %0.2f | angles_z %0.2f\n",				
				pRecipient->v.angles.x, pRecipient->v.angles.y, pRecipient->v.angles.z);
			ClientPrint(pRecipient, HUD_PRINTCONSOLE, str);
			*/


			/*
			edict_t *pent = NULL;
			edict_t *pEdict = listenserver_edict;
			char item_name[40];
			Vector vecStart, vecEnd;

			while ((pent = UTIL_FindEntityInSphere( pent, pEdict->v.origin, 200 )) != NULL)
			{
				strcpy(item_name, STRING(pent->v.classname));	
				
				// look for placed claymores
				if (strcmp("item_claymore", item_name) == 0)
				{
					vecStart = pEdict->v.origin + pEdict->v.view_ofs;
					vecEnd = pent->v.origin;
					Vector dest = vecEnd - vecStart;

					// find angles from source to destination...
					Vector entity_angles = UTIL_VecToAngles( dest );
					
					// make yaw angle 0 to 360 degrees if negative...
					if (entity_angles.y < 0)
						entity_angles.y += 360;
					
					// get current view angle...
					float view_angle = pEdict->v.v_angle.y;
					
					// make view angle 0 to 360 degrees if negative...
					if (view_angle < 0)
						view_angle += 360;
					
					// return the absolute value of angle to destination entity
					// zero degrees means straight ahead,  45 degrees to the left or
					// 45 degrees to the right is the limit of the normal view angle
					
					// rsm - START angle bug fix
					int angle = abs((int)view_angle - (int)entity_angles.y);
					
					if (angle > 180)
						angle = 360 - angle;

					char str[128];
					
					sprintf(str, "angle to clay (zero is straight ahead) %d | dist to clay %0.2f\n",
						angle, dest.Length());
					ClientPrint(listenserver_edict, HUD_PRINTCONSOLE, str);



					Vector2D vec2LOS;
					float    flDot;
					
					UTIL_MakeVectors ( pEdict->v.angles );
					
					vec2LOS = ( vecEnd - pEdict->v.origin ).Make2D();
					vec2LOS = vec2LOS.Normalize();
					
					flDot = DotProduct (vec2LOS , gpGlobals->v_forward.Make2D() );

					sprintf(str, "angle to clay (using FInViewCone()) %0.2f || using InFieldOfView %d\n",
						flDot, InFieldOfView(pEdict, dest));
					ClientPrint(listenserver_edict, HUD_PRINTCONSOLE, str);

					break;
				}
			}
			*/
		}
#endif

		previous_time = gpGlobals->time;
	} // is deathmatch END

	(*other_gFunctionTable.pfnStartFrame)();
}

void ParmsNewLevel( void )
{
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "ParmsNewLevel\n"); fclose(fp); }

	(*other_gFunctionTable.pfnParmsNewLevel)();
}

void ParmsChangeLevel( void )
{
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "ParmsChangeLevel\n"); fclose(fp); }

	(*other_gFunctionTable.pfnParmsChangeLevel)();
}

const char *GetGameDescription( void )
{
	// prepare development debugging file
	if (debug_fname[0] == '\0')
	{
		UTIL_MarineBotFileName(debug_fname, "!mb_devdebug.txt", NULL);
	}

	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "GetGameDescription\n"); fclose(fp); }

	return (*other_gFunctionTable.pfnGetGameDescription)();
}

void PlayerCustomization( edict_t *pEntity, customization_t *pCust )
{
#ifdef _DEBUG
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "PlayerCustomization: %x\n",pEntity); fclose(fp); }
#endif

	(*other_gFunctionTable.pfnPlayerCustomization)(pEntity, pCust);
}

void SpectatorConnect( edict_t *pEntity )
{
#ifdef _DEBUG
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "SpectatorConnect: %x\n",pEntity); fclose(fp); }
#endif

	(*other_gFunctionTable.pfnSpectatorConnect)(pEntity);
}

void SpectatorDisconnect( edict_t *pEntity )
{
#ifdef _DEBUG
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "SpectatorDisconnect: %x\n",pEntity); fclose(fp); }
#endif

	(*other_gFunctionTable.pfnSpectatorDisconnect)(pEntity);
}

void SpectatorThink( edict_t *pEntity )
{
#ifdef _DEBUG
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "SpectatorThink: %x\n",pEntity); fclose(fp); }
#endif

	(*other_gFunctionTable.pfnSpectatorThink)(pEntity);
}

void Sys_Error( const char *error_string )
{
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "Sys_Error: %s\n",error_string); fclose(fp); }

#ifdef _DEBUG
	fp=fopen(debug_fname,"a");
	fprintf(fp, "Sys_Error: %s\n",error_string);
	fclose(fp);
#endif

	// dump the error in error log ... useful when there is missing some model on map load etc.
	UTIL_DebugInFile((char *)error_string);

	(*other_gFunctionTable.pfnSys_Error)(error_string);
}

void PM_Move ( struct playermove_s *ppmove, int server )
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "PM_Move:\n"); fclose(fp); }

	(*other_gFunctionTable.pfnPM_Move)(ppmove, server);
}

void PM_Init ( struct playermove_s *ppmove )
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "PM_Init:\n"); fclose(fp); }

	(*other_gFunctionTable.pfnPM_Init)(ppmove);
}

char PM_FindTextureType( char *name )
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "PM_FindTextureType: %s\n", name); fclose(fp); }

	return (*other_gFunctionTable.pfnPM_FindTextureType)(name);
}

void SetupVisibility( edict_t *pViewEntity, edict_t *pClient, unsigned char **pvs, unsigned char **pas )
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "SetupVisibility: \n"); fclose(fp); }

	(*other_gFunctionTable.pfnSetupVisibility)(pViewEntity, pClient, pvs, pas);
}

void UpdateClientData ( const struct edict_s *ent, int sendweapons, struct clientdata_s *cd )
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "UpdateClientData: \n"); fclose(fp); }

	(*other_gFunctionTable.pfnUpdateClientData)(ent, sendweapons, cd);
}

int AddToFullPack( struct entity_state_s *state, int e, edict_t *ent, edict_t *host, int hostflags, int player, unsigned char *pSet )
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "AddToFullPack: \n"); fclose(fp); }

	return (*other_gFunctionTable.pfnAddToFullPack)(state, e, ent, host, hostflags, player, pSet);
}

void CreateBaseline( int player, int eindex, struct entity_state_s *baseline, struct edict_s *entity, int playermodelindex, vec3_t player_mins, vec3_t player_maxs )
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "CreateBaseline: \n"); fclose(fp); }

	(*other_gFunctionTable.pfnCreateBaseline)(player, eindex, baseline, entity, playermodelindex, player_mins, player_maxs);
}

void RegisterEncoders( void )
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "RegisterEncoders: \n"); fclose(fp); }

	(*other_gFunctionTable.pfnRegisterEncoders)();
}

int GetWeaponData( struct edict_s *player, struct weapon_data_s *info )
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "GetWeaponData: \n"); fclose(fp); }

	return (*other_gFunctionTable.pfnGetWeaponData)(player, info);
}

void CmdStart( const edict_t *player, const struct usercmd_s *cmd, unsigned int random_seed )
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "CmdStart: \n"); fclose(fp); }

	(*other_gFunctionTable.pfnCmdStart)(player, cmd, random_seed);
}

void CmdEnd ( const edict_t *player )
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "CmdEnd: \n"); fclose(fp); }

	(*other_gFunctionTable.pfnCmdEnd)(player);
}

int ConnectionlessPacket( const struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size )
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "ConnectionlessPacket: \n"); fclose(fp); }

	return (*other_gFunctionTable.pfnConnectionlessPacket)(net_from, args, response_buffer, response_buffer_size);
}

int GetHullBounds( int hullnumber, float *mins, float *maxs )
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "GetHullBounds: \n"); fclose(fp); }

	return (*other_gFunctionTable.pfnGetHullBounds)(hullnumber, mins, maxs);
}

void CreateInstancedBaselines( void )
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "CreateInstancedBaselines: \n"); fclose(fp); }

	(*other_gFunctionTable.pfnCreateInstancedBaselines)();
}

int InconsistentFile( const edict_t *player, const char *filename, char *disconnect_message )
{
#ifdef _DEBUG
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "InconsistentFile: %x filename=%s discon_message=%s\n",player,filename,disconnect_message); fclose(fp); }
#endif

	return (*other_gFunctionTable.pfnInconsistentFile)(player, filename, disconnect_message);
}

int AllowLagCompensation( void )
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "AllowLagCompensation: \n"); fclose(fp); }

	return (*other_gFunctionTable.pfnAllowLagCompensation)();
}


DLL_FUNCTIONS gFunctionTable =
{
	GameDLLInit,               //pfnGameInit
	DispatchSpawn,             //pfnSpawn
	DispatchThink,             //pfnThink
	DispatchUse,               //pfnUse
	DispatchTouch,             //pfnTouch
	DispatchBlocked,           //pfnBlocked
	DispatchKeyValue,          //pfnKeyValue
	DispatchSave,              //pfnSave
	DispatchRestore,           //pfnRestore
	DispatchObjectCollsionBox, //pfnAbsBox

	SaveWriteFields,           //pfnSaveWriteFields
	SaveReadFields,            //pfnSaveReadFields

	SaveGlobalState,           //pfnSaveGlobalState
	RestoreGlobalState,        //pfnRestoreGlobalState
	ResetGlobalState,          //pfnResetGlobalState

	ClientConnect,             //pfnClientConnect
	ClientDisconnect,          //pfnClientDisconnect
	ClientKill,                //pfnClientKill
	ClientPutInServer,         //pfnClientPutInServer
	ClientCommand,             //pfnClientCommand
	ClientUserInfoChanged,     //pfnClientUserInfoChanged
	ServerActivate,            //pfnServerActivate
	ServerDeactivate,          //pfnServerDeactivate

	PlayerPreThink,            //pfnPlayerPreThink
	PlayerPostThink,           //pfnPlayerPostThink

	StartFrame,                //pfnStartFrame
	ParmsNewLevel,             //pfnParmsNewLevel
	ParmsChangeLevel,          //pfnParmsChangeLevel

	GetGameDescription,        //pfnGetGameDescription    Returns string describing current .dll game.
	PlayerCustomization,       //pfnPlayerCustomization   Notifies .dll of new customization for player.

	SpectatorConnect,          //pfnSpectatorConnect      Called when spectator joins server
	SpectatorDisconnect,       //pfnSpectatorDisconnect   Called when spectator leaves the server
	SpectatorThink,            //pfnSpectatorThink        Called when spectator sends a command packet (usercmd_t)

	Sys_Error,                 //pfnSys_Error          Called when engine has encountered an error

	PM_Move,                   //pfnPM_Move
	PM_Init,                   //pfnPM_Init            Server version of player movement initialization
	PM_FindTextureType,        //pfnPM_FindTextureType

	SetupVisibility,           //pfnSetupVisibility        Set up PVS and PAS for networking for this client
	UpdateClientData,          //pfnUpdateClientData       Set up data sent only to specific client
	AddToFullPack,             //pfnAddToFullPack
	CreateBaseline,            //pfnCreateBaseline        Tweak entity baseline for network encoding, allows setup of player baselines, too.
	RegisterEncoders,          //pfnRegisterEncoders      Callbacks for network encoding
	GetWeaponData,             //pfnGetWeaponData
	CmdStart,                  //pfnCmdStart
	CmdEnd,                    //pfnCmdEnd
	ConnectionlessPacket,      //pfnConnectionlessPacket
	GetHullBounds,             //pfnGetHullBounds
	CreateInstancedBaselines,  //pfnCreateInstancedBaselines
	InconsistentFile,          //pfnInconsistentFile
	AllowLagCompensation,      //pfnAllowLagCompensation
};

#ifdef __BORLANDC__
int EXPORT GetEntityAPI( DLL_FUNCTIONS *pFunctionTable, int interfaceVersion )
#else
extern "C" EXPORT int GetEntityAPI( DLL_FUNCTIONS *pFunctionTable, int interfaceVersion )
#endif
{
   // check if engine's pointer is valid and version is correct...

   if ( !pFunctionTable || interfaceVersion != INTERFACE_VERSION )
      return FALSE;

   // pass engine callback function table to engine...
   memcpy( pFunctionTable, &gFunctionTable, sizeof( DLL_FUNCTIONS ) );

   
   // pass other DLLs engine callbacks to function table...
   if (!(*other_GetEntityAPI)(&other_gFunctionTable, INTERFACE_VERSION))
   {
      return FALSE;  // error initializing function table!!!
   }

   return TRUE;
}


#ifdef __BORLANDC__
int EXPORT GetNewDLLFunctions( NEW_DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion )
#else
extern "C" EXPORT int GetNewDLLFunctions( NEW_DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion )
#endif
{
   if (other_GetNewDLLFunctions == NULL)
      return FALSE;

   // pass other DLLs engine callbacks to function table...
   if (!(*other_GetNewDLLFunctions)(pFunctionTable, interfaceVersion))
   {
      return FALSE;  // error initializing function table!!!
   }

   return TRUE;
}


void FakeClientCommand(edict_t *pBot, char *arg1, char *arg2, char *arg3)
{
	int length;

	memset(g_argv, 0, 1024);

	isFakeClientCommand = 1;

	if ((arg1 == NULL) || (*arg1 == 0))
		return;

	if ((arg2 == NULL) || (*arg2 == 0))
	{
		length = sprintf(&g_argv[0], "%s", arg1);
		fake_arg_count = 1;
	}
	else if ((arg3 == NULL) || (*arg3 == 0))
	{
		length = sprintf(&g_argv[0], "%s %s", arg1, arg2);
		fake_arg_count = 2;
	}
	else
	{	
		length = sprintf(&g_argv[0], "%s %s %s", arg1, arg2, arg3);
		fake_arg_count = 3;
	}

	g_argv[length] = 0;  // null terminate just in case

	strcpy(&g_argv[64], arg1);

	if (arg2)
		strcpy(&g_argv[128], arg2);

	if (arg3)
		strcpy(&g_argv[192], arg3);

	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"FakeClientCommand=%s\n",g_argv); fclose(fp); }

	// allow the MOD DLL to execute the ClientCommand...
	ClientCommand(pBot);

	isFakeClientCommand = 0;
}

const char *Cmd_Args( void )
{
	if (isFakeClientCommand)
	{
		// a fix by Pierre-Marie Baty
		// is it a "say" or "say_team" client command ?
		if (strncmp ("say ", g_argv, 4) == 0)
			return (&g_argv[0] + 4); // skip the "say" bot client command (bug in HL engine)
		else if (strncmp ("say_team ", g_argv, 9) == 0)
			return (&g_argv[0] + 9); // skip the "say_team" bot client command (bug in HL engine)
		// a fix by Pierre-Marie Baty end

		return &g_argv[0];
	}
	else
	{
		return (*g_engfuncs.pfnCmd_Args)();
	}
}


const char *Cmd_Argv( int argc )
{
	if (isFakeClientCommand)
	{
		if (argc == 0)
		{
			return &g_argv[64];
		}
		else if (argc == 1)
		{
			return &g_argv[128];
		}
		else if (argc == 2)
		{
			return &g_argv[192];
		}
		else
		{
			return NULL;
		}
	}
	else
	{
		return (*g_engfuncs.pfnCmd_Argv)(argc);
	}
}


int Cmd_Argc( void )
{
	if (isFakeClientCommand)
	{
		return fake_arg_count;
	}
	else
	{
		return (*g_engfuncs.pfnCmd_Argc)();
	}
}


// SECTION - new config system by Andrey Kotrekhov
// ADDED validity checks, changed IS_DEDICATED_SERVER() to is_dedicated_server,
// debug also for Listen server and system debug by Frank McNeil
void ProcessBotCfgFile(Section *conf)
{
	static bool init_cfg_const = false;
	static bool init_commands = false;
	static int cur_command = 0;
	static int cur_recruit = 0;

	static char server_cmd[SERVER_CMD_LEN+1];	
	char msg[80];

	if (conf == NULL)
	{
#ifdef _DEBUG
		//@@@@@@@@@@
		PrintOutput(NULL, "*** conf==NULL ProcessBotCfgFile\n", msg_null);
#endif

		exit(1);

		return;
	}

	if (read_whole_cfg)
	{
#ifdef _DEBUG
		//@@@@@@@@@@
		PrintOutput(NULL, "ProcessBotCfgFile() - reading whole .cfg\n", msg_null);
#endif

		read_whole_cfg = FALSE;

		// reset these to allow us to read them again
		init_cfg_const = FALSE;
		init_commands = FALSE;
		cur_recruit = 0;
	}

	if (init_cfg_const == false)
	{
		// ADDED temp setting values checks by Frank McNeil
		int temp_default_bot_skill, temp_minbots, temp_maxbots;
		bool temp_islogging, temp_random_skill, temp_customclasses, temp_dontspeak, temp_botdontchat, temp_richnames, temp_passivehealing;
		float temp_bot_reaction_time, temp_auto_balance_time, temp_info_time, temp_send_presentation_time;

		init_cfg_const = true;
		SetupVars set_vars;
	    set_vars.Add("mb_log", new SetupBaseType_yesno(temp_islogging), false, "no");
	    set_vars.Add("spawn_skill", new SetupBaseType_int(temp_default_bot_skill), false, "3");
		set_vars.Add("random_skill", new SetupBaseType_yesno(temp_random_skill), false, "no");
	    set_vars.Add("min_bots", new SetupBaseType_int(temp_minbots), false, "2");
	    set_vars.Add("max_bots", new SetupBaseType_int(temp_maxbots), false, "6");
	    set_vars.Add("reaction_time", new SetupBaseType_float(temp_bot_reaction_time), false, "0.2");
	    set_vars.Add("auto_balance", new SetupBaseType_float(temp_auto_balance_time), false, "60.0");
	    set_vars.Add("custom_classes", new SetupBaseType_yesno(temp_customclasses), false, "yes");
	    set_vars.Add("send_info", new SetupBaseType_float(temp_info_time), false, "150.0");
		set_vars.Add("send_presentation", new SetupBaseType_float(temp_send_presentation_time), false, "210.0");
	    set_vars.Add("dont_speak", new SetupBaseType_yesno(temp_dontspeak), false, "no");
		set_vars.Add("dont_chat", new SetupBaseType_yesno(temp_botdontchat), false, "no");
		set_vars.Add("rich_names", new SetupBaseType_yesno(temp_richnames), false, "yes");
		set_vars.Add("passive_healing", new SetupBaseType_yesno(temp_passivehealing), false, "no");
		set_vars.Add("be_samurai", new SetupBaseType_yesno(externals.be_samurai), false, "no");
		set_vars.Add("harakiri_time", new SetupBaseType_float(externals.harakiri_time), false, "60.0");
	    
		try
		{
			set_vars.Set(conf);
		}
		catch (errGetVal &er_val)
		{
			sprintf(msg, "** missing variable '%s'", er_val.key);

			if (is_dedicated_server)
			{
				printf(msg);
			}
			else
			{
				ALERT(at_console, msg);
			}
		}

		if (temp_islogging == TRUE)
		{
			externals.SetIsLogging(TRUE);
			UTIL_MBLogPrint("MARINE_BOT CFG FILE INIT - logging ENABLED!\n");
		}
		else
		{
			externals.SetIsLogging(FALSE);
			// here must be standard printing to print that into console
			if (is_dedicated_server)
				PrintOutput(NULL, "MARINE_BOT CFG FILE INIT - logging DISABLED!\n", msg_null);
		}

		if ((temp_default_bot_skill < 1) || (temp_default_bot_skill > 5))
			sprintf(msg, "MARINE_BOT CFG FILE ERROR - default spawnskill reset to %d\n",
				externals.GetSpawnSkill());
		else
		{
			externals.SetSpawnSkill(temp_default_bot_skill);

			sprintf(msg, "MARINE_BOT CFG FILE INIT - default spawnskill set to %d\n",
				externals.GetSpawnSkill());
		}

		if (is_dedicated_server)
			PrintOutput(NULL, msg, msg_null);

		if (temp_random_skill == TRUE)
		{
			externals.SetRandomSkill(TRUE);
			sprintf(msg, "MARINE_BOT CFG FILE INIT - random_skill ENABLED!\n");
		}
		else
		{
			externals.SetRandomSkill(FALSE);
			sprintf(msg, "MARINE_BOT CFG FILE INIT - random_skill DISABLED!\n");
		}

		if (is_dedicated_server)
			PrintOutput(NULL, msg, msg_null);

		if ((temp_minbots < 0) || (temp_minbots > 31))
		{
			externals.ResetMinBots();
			sprintf(msg, "MARINE_BOT CFG FILE ERROR - min_bots reset to %d\n",
				externals.GetMinBots());
		}
		else if (temp_minbots == 31)
		{
			externals.SetMinBots(-1);
			sprintf(msg, "MARINE_BOT CFG FILE INIT - min_bots DISABLED!\n");
		}
		else
		{
			externals.SetMinBots((int) temp_minbots);
			sprintf(msg, "MARINE_BOT CFG FILE INIT - min_bots set to %d\n",
				externals.GetMinBots());
		}
		
		if (is_dedicated_server)
			PrintOutput(NULL, msg, msg_null);

		if ((temp_maxbots < 0) || (temp_maxbots > 31))
		{
			externals.ResetMaxBots();
			sprintf(msg, "MARINE_BOT CFG FILE ERROR - max_bots reset to %d\n",
				externals.GetMaxBots());
	    }
	    else if (temp_maxbots == 0)
		{
			externals.SetMaxBots(-1);
			sprintf(msg, "MARINE_BOT CFG FILE INIT - max_bots DISABLED!\n");
	    }
	    else
		{
			externals.SetMaxBots((int) temp_maxbots);
			sprintf(msg, "MARINE_BOT CFG FILE INIT - max_bots set to %d\n",
				externals.GetMaxBots());
	    }

		if (is_dedicated_server)
			PrintOutput(NULL, msg, msg_null);

		if ((temp_bot_reaction_time < 0.0) || (temp_bot_reaction_time > 50.0))
		{
			externals.ResetReactionTime();
			sprintf(msg, "MARINE_BOT CFG FILE ERROR - bots reaction_time reset to %.1fs\n",
				externals.GetReactionTime());
		}
		else
		{
			externals.SetReactionTime((float) temp_bot_reaction_time);

			sprintf(msg, "MARINE_BOT CFG FILE INIT - bots reaction_time set to %.1fs\n",
				externals.GetReactionTime());
		}

		if (is_dedicated_server)
			PrintOutput(NULL, msg, msg_null);

		if (temp_auto_balance_time == 0.0)
		{
			externals.SetBalanceTime(0.0);
			check_auto_balance = -1.0;
			do_auto_balance = FALSE;	// added by kota@

			sprintf(msg, "MARINE_BOT CFG FILE INIT - auto_balance DISABLED!\n");
	    }
		else if ((temp_auto_balance_time < 30.0) || (temp_auto_balance_time > 3600.0))
		{
			externals.ResetBalanceTime();
			do_auto_balance = TRUE;
			sprintf(msg, "MARINE_BOT CFG FILE ERROR - auto_balance time reset to %.1fs\n",
				externals.GetBalanceTime());
		}
		else
		{
			externals.SetBalanceTime((float) temp_auto_balance_time);
			do_auto_balance = TRUE;		// added by kota@

			sprintf(msg, "MARINE_BOT CFG FILE INIT - auto_balance time set to %.1fs\n",
				externals.GetBalanceTime());
	    }

	    if (is_dedicated_server)
			PrintOutput(NULL, msg, msg_null);

		if (temp_customclasses == TRUE)
		{
			externals.SetCustomClasses(TRUE);
			sprintf(msg, "MARINE_BOT CFG FILE INIT - using custom class setting\n");
		}
		else
		{
			externals.SetCustomClasses(FALSE);
			sprintf(msg, "MARINE_BOT CFG FILE INIT - using default/hard coded class setting\n");
		}

		if (is_dedicated_server)
			PrintOutput(NULL, msg, msg_null);

		if (temp_info_time == 0)
		{
			externals.SetInfoTime((float) 0.0);
			check_send_info = -1.0;
			sprintf(msg, "MARINE_BOT CFG FILE INIT - send_info DISABLED!\n");
		}
		else if ((temp_info_time < 30.0) || (temp_info_time > 3600.0))
		{
			externals.ResetInfoTime();
			sprintf(msg, "MARINE_BOT CFG FILE ERROR - send_info time reset to %.1fs\n",
				externals.GetInfoTime());
		}
		else
		{
			externals.SetInfoTime((float) temp_info_time);
			sprintf(msg, "MARINE_BOT CFG FILE INIT - send_info time set to %.1fs\n",
				externals.GetInfoTime());
		}

		if (is_dedicated_server)
			PrintOutput(NULL, msg, msg_null);

		if (temp_send_presentation_time == 0)
		{
			presentation_time = 0.0;
			sprintf(msg, "MARINE_BOT CFG FILE INIT - send_presentation DISABLED!\n");
		}
		else if ((temp_send_presentation_time < 0.0) || (temp_send_presentation_time > 3600.0))
		{
			externals.ResetPresentationTime();
			sprintf(msg, "MARINE_BOT CFG FILE ERROR - send_presentation time reset to %.1fs\n",
				externals.GetPresentationTime());
		}
		else
		{
			// just for sure ie. to prevent engine overloading
			if (temp_send_presentation_time < 10.0)
				externals.SetPresentationTime((float) 10.0);
			else
				externals.SetPresentationTime((float) temp_send_presentation_time);

			sprintf(msg, "MARINE_BOT CFG FILE INIT - send_presentation time set to %.1fs\n",
				externals.GetPresentationTime());
		}

		if (is_dedicated_server)
			PrintOutput(NULL, msg, msg_null);

		if (temp_dontspeak == TRUE)
		{
			externals.SetDontSpeak(TRUE);
			sprintf(msg, "MARINE_BOT CFG FILE INIT - dont_speak ENABLED!\n");
		}
		else
		{
			externals.SetDontSpeak(FALSE);
			sprintf(msg, "MARINE_BOT CFG FILE INIT - dont_speak DISABLED!\n");
		}
		
		if (is_dedicated_server)
			PrintOutput(NULL, msg, msg_null);

		if (temp_botdontchat == TRUE)
		{
			externals.SetDontChat(TRUE);
			sprintf(msg, "MARINE_BOT CFG FILE INIT - dont_chat ENABLED!\n");
		}
		else
		{
			externals.SetDontChat(FALSE);
			sprintf(msg, "MARINE_BOT CFG FILE INIT - dont_chat DISABLED!\n");
		}

		if (is_dedicated_server)
			PrintOutput(NULL, msg, msg_null);

		if (temp_richnames == TRUE)
		{
			externals.SetRichNames(TRUE);
			sprintf(msg, "MARINE_BOT CFG FILE INIT - rich_names ENABLED!\n");
		}
		else
		{
			externals.SetRichNames(FALSE);
			sprintf(msg, "MARINE_BOT CFG FILE INIT - rich_names DISABLED!\n");
		}
		
		if (is_dedicated_server)
			PrintOutput(NULL, msg, msg_null);

		if (temp_passivehealing == TRUE)
		{
			externals.SetPassiveHealing(TRUE);
			sprintf(msg, "MARINE_BOT CFG FILE INIT - passive_healing ENABLED!\n");
		}
		else
		{
			externals.SetPassiveHealing(FALSE);
			sprintf(msg, "MARINE_BOT CFG FILE INIT - passive_healing DISABLED!\n");
		}
		
		if (is_dedicated_server)
			PrintOutput(NULL, msg, msg_null);


		//setup method order from config file.
		SI method_i = conf->sectionList.find("method_order");
		NaviPair navi_pair;
		if (method_i != conf->sectionList.end())
 		{
			Section *method_sect = method_i->second;
			NavigationMethods::iterator meth_dict_i;

			NMethodsList.clear();
			if (is_dedicated_server)
			{
			   sprintf(msg, "MARINE_BOT CFG FILE INIT - Configuring navigation methods...\n");
			   //PrintOutput(NULL, msg, msg_null);	NOTE: don't print this if we aren't using it yet
			}
			for (II meth_ord_i = method_sect->item.begin();
					meth_ord_i != method_sect->item.end(); ++meth_ord_i) {
				if ((meth_dict_i = NMethodsDict.find(meth_ord_i->second)) != NMethodsDict.end()) {
					navi_pair.name = meth_dict_i->first;
					navi_pair.ptr = meth_dict_i->second;

					NMethodsList.push_back(navi_pair);
					//FIXME!!! we should inform user about adding next method.
					if (is_dedicated_server)
					{
					   sprintf(msg, "MARINE_BOT CFG FILE INIT - Adding navigation method %s\n", 
					           navi_pair.name.c_str());
					   
					   if (is_dedicated_server)
						   PrintOutput(NULL, msg, msg_null);
					}
				}
			}
		}
	}

	if (bot_cfg_pause_time > gpGlobals->time)
 	{
		return;
	}

	if (init_commands == false)
	{
		SI com_i = conf->sectionList.find("commands");

		if (com_i != conf->sectionList.end())
 		{
			Section *command_sect = com_i->second;
			int ii = 0;

			for (II command_i = command_sect->item.begin(); ; ++command_i, ++ii)
			{
				if (command_i == command_sect->item.end())
				{
					init_commands = true;
					return;
				}

				if (ii == cur_command)
				{
					strcpy(server_cmd,(command_i->second).c_str());
					strcat(server_cmd, "\n");

					if(command_i->second.size()<SERVER_CMD_LEN-2)
					{
						sprintf(server_cmd,"%s\n",(command_i->second).c_str());
						SERVER_COMMAND(server_cmd);
					}

					++cur_command;

					break;
				}
			}
	    }

	    return;
	}

	if ((externals.GetMaxBots() == -1) || (externals.GetMaxBots() == 0))
	{
#ifdef _DEBUG
		//@@@@@@@@@@
		PrintOutput(NULL, "ProcessBotCfgFile() - no bots to be auto spawned - skipping recruit section\n", msg_null);
#endif
		// we don't want to run this method all the time
		bot_cfg_pause_time = 0.0;

		return;
	}

	if (is_dedicated_server && (clients[0].ClientCount() >= externals.GetMaxBots()))
	{
#ifdef _DEBUG
		//@@@@@@@@@@
		PrintOutput(NULL, "ProcessBotCfgFile() - number of clients on the server >= max_bots - skipping recruit section\n", msg_info);
		//UTIL_DebugInFile("ProcessBotCfgFile() - number of clients on the server >= max_bots - skipping recruit section\n");
#endif
		// we don't want to run this method all the time
		bot_cfg_pause_time = 0.0;

		return;
	}

	string arg1="", arg2="", arg3="", arg4="", arg5="";
	SI ci = conf->sectionList.find("recruit");

	if (ci != conf->sectionList.end())
	{
		//need to add bots
#ifdef _DEBUG
		if (is_dedicated_server)
		{
			//@@@@@@@@@@
			printf("** ProcessBotCfgFile() - a try to add bot\n");
	    }
#endif

	    Section *recruit_sect = ci->second;
		int i = 0;

	    for (SI recruit_sect_i = recruit_sect->sectionList.begin(); ; ++recruit_sect_i, ++i)
		{
			if (recruit_sect_i == recruit_sect->sectionList.end())
			{
				//we added last bot
				bot_cfg_pause_time = 0.0;
				break;
			}

			if (i == cur_recruit)
			{
				++cur_recruit;
				SetupVars set_recruit;
				set_recruit.Add("team", new SetupBaseType_team(arg1), false, "");
				set_recruit.Add("class", new SetupBaseType_string(arg2), false, "");
				set_recruit.Add("skin", new SetupBaseType_string(arg3), false, "");
				set_recruit.Add("skill", new SetupBaseType_string(arg4), false, "");
				set_recruit.Add("name", new SetupBaseType_string(arg5), false, "");

				try
				{
					set_recruit.Set(recruit_sect_i->second);
				}
				catch (errGetVal &er_val)
				{
					//@@@@@@@@@@
					sprintf(msg, "** missing variable '%s'", er_val.key);
					
					PrintOutput(NULL, msg, msg_null);
				}
#ifdef _DEBUG
				//@@@@@@@@@@
				sprintf(msg, "** ProcessBotCfgFile() - a try to add bot number %d\n", cur_recruit);
				PrintOutput(NULL, msg, msg_null);

				sprintf(msg, "** ProcessBotCfgFile() - bot params %s %s %s %s %s\n", arg1.c_str(), arg2.c_str(),
					arg3.c_str(), arg4.c_str(), arg5.c_str());
				PrintOutput(NULL, msg, msg_null);
#endif

				BotCreate( NULL, arg1, arg2, arg3, arg4, arg5 );
				
				bot_cfg_pause_time = gpGlobals->time + 2.0;
				bot_check_time = gpGlobals->time + 2.5;

				break;
			}
		}
	}
}
// SECTION BY Andrey Kotrekhov END

// play sound msg confirmation
void PlaySoundConfirmation(edict_t *pEntity, int msg_type)
{
	if (msg_type == SND_DONE)
	{
		EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "plats/elevbell1.wav", 1.0,
			            ATTN_NORM, 0, 100);
	}
	else if (msg_type == SND_FAILED)
	{
		EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "buttons/button10.wav", 1.0,
			            ATTN_NORM, 0, 100);
	}

	return;
}

inline void MBMenuSystem(edict_t *pEntity, const char *arg1)
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
			BotCreate( pEntity, "1", NULL, NULL, NULL, NULL );
			bot_check_time = gpGlobals->time + 2.5;
		}
		else if (FStrEq(arg1, "2"))
		{
			BotCreate( pEntity, "2", NULL, NULL, NULL, NULL );
			bot_check_time = gpGlobals->time + 2.5;
		}
		else if (FStrEq(arg1, "3"))
		{
			server_filling = TRUE;
			bot_check_time = gpGlobals->time + 0.5;
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
			team_balance_value = UTIL_BalanceTeams();
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
				if (g_waypoint_on == FALSE)
					g_waypoint_on = TRUE;

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
				if (g_waypoint_on == FALSE)
					g_waypoint_on = TRUE;

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
				if (g_waypoint_on == FALSE)
					g_waypoint_on = TRUE;

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
				if (g_waypoint_on == FALSE)
					g_waypoint_on = TRUE;
						
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
				if (g_waypoint_on == FALSE)
					g_waypoint_on = TRUE;

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
				if (g_waypoint_on == FALSE)
					g_waypoint_on = TRUE;

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
				if (g_waypoint_on == FALSE)
					g_waypoint_on = TRUE;

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
				if (g_waypoint_on == FALSE)
					g_waypoint_on = TRUE;

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
				if (g_waypoint_on == FALSE)
					g_waypoint_on = TRUE;

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
				if (g_waypoint_on == FALSE)
					g_waypoint_on = TRUE;

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
				if (g_waypoint_on == FALSE)
					g_waypoint_on = TRUE;

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
				if (g_waypoint_on == FALSE)
					g_waypoint_on = TRUE;
						
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
				if (g_waypoint_on == FALSE)
					g_waypoint_on = TRUE;

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
				if (g_waypoint_on == FALSE)
					g_waypoint_on = TRUE;

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
				if (g_waypoint_on == FALSE)
					g_waypoint_on = TRUE;
						
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
				if (g_waypoint_on == FALSE)
					g_waypoint_on = TRUE;

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
				if (g_waypoint_on == FALSE)
					g_waypoint_on = TRUE;

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
				if (g_waypoint_on == FALSE)
					g_waypoint_on = TRUE;

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
				if (g_waypoint_on == FALSE)
					g_waypoint_on = TRUE;

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
				if (g_waypoint_on == FALSE)
					g_waypoint_on = TRUE;

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
				if (g_waypoint_on == FALSE)
					g_waypoint_on = TRUE;

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
		char *the_team;

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
		char *the_team;

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
			g_auto_waypoint = TRUE;
			g_waypoint_on = TRUE;		// just in case wpts aren't on
		}
		else if (FStrEq(arg1, "2"))
		{
			g_auto_waypoint = FALSE;
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
			g_auto_wpt_distance = 80;
		else if (FStrEq(arg1, "2"))
			g_auto_wpt_distance = 100;
		else if (FStrEq(arg1, "3"))
			g_auto_wpt_distance = 120;
		else if (FStrEq(arg1, "4"))
			g_auto_wpt_distance = 160;
		else if (FStrEq(arg1, "5"))
			g_auto_wpt_distance = 200;
		else if (FStrEq(arg1, "6"))
			g_auto_wpt_distance = 280;
		else if (FStrEq(arg1, "7"))
			g_auto_wpt_distance = 340;
		else if (FStrEq(arg1, "8"))
			g_auto_wpt_distance = 400;
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
			if (g_path_waypoint_on)
				g_path_waypoint_on = FALSE;
			else
			{
				g_waypoint_on = TRUE;		// turn waypoints on
				g_path_waypoint_on = TRUE;	// turn paths on
			}
		}
		else if ((FStrEq(arg1, "2")) && (is_wpt_near != -1))	// only if there is any wpt
		{
			if (g_path_waypoint_on == FALSE)
				g_path_waypoint_on = TRUE;

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
			if (g_auto_path)
				g_auto_path = FALSE;
			else
				g_auto_path = TRUE;
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
			if (g_waypoint_on)
				g_waypoint_on = FALSE;
			else
				g_waypoint_on = TRUE;
		}
		else if (FStrEq(arg1, "2"))
		{
			g_menu_state = MENU_2_9_2;	// switch to adv. debugging menu
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_9_2);

			return;
		}
		else if (FStrEq(arg1, "3"))
		{
			WaypointLoad(pEntity, NULL);
			WaypointPathLoad(pEntity, NULL);
		}
		else if (FStrEq(arg1, "4"))
		{
			WaypointSave(NULL);

			if (num_w_paths > 0)
				WaypointPathSave(NULL);
		}
		else if (FStrEq(arg1, "5"))
		{
			if (WaypointLoadUnsupported(pEntity))
			{
				if (WaypointPathLoadUnsupported(pEntity))
				{
					WaypointSave(NULL);
					WaypointPathSave(NULL);
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
			if (check_cross)
				check_cross = FALSE;
			else
			{
				check_cross = TRUE;
				check_ranges = FALSE;
			}
		}
		else if (FStrEq(arg1, "2"))
		{
			if (check_aims)
				check_aims = FALSE;
			else
			{
				check_aims = TRUE;
				check_ranges = FALSE;
			}
		}
		else if (FStrEq(arg1, "3"))
		{
			if (check_ranges)
				check_ranges = FALSE;
			else
			{
				check_ranges = TRUE;
				check_cross = FALSE;
				check_aims = FALSE;
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
			highlight_this_path = HIGHLIGHT_RED;	// red team paths only
		else if (FStrEq(arg1, "2"))
			highlight_this_path = HIGHLIGHT_BLUE;	// blue team paths only
		else if (FStrEq(arg1, "3"))
			highlight_this_path = HIGHLIGHT_ONEWAY;	// one-way paths only
		else if ((FStrEq(arg1, "4")) || (FStrEq(arg1, "5")) ||
			(FStrEq(arg1, "6")) || (FStrEq(arg1, "7")) || (FStrEq(arg1, "8")))
		{
			g_menu_state = MENU_2_9_2_4;		// stay in this menu
			UTIL_ShowMenu(pEntity, 0x3FF, -1, FALSE, show_menu_2_9_2_4);

			return;
		}
		else if (FStrEq(arg1, "9"))
			highlight_this_path = -1;	// turn it off ie all paths shows again
		else if (FStrEq(arg1, "0"))
		{
			g_menu_state = MENU_MAIN;		// switch to main menu
			UTIL_ShowMenu(pEntity, 0x1F, -1, FALSE, show_menu_main);

			return;
		}
	}
	else if (g_menu_state == MENU_2_9_2_9)	// in draw distance menu
	{
		if (FStrEq(arg1, "1"))
			g_draw_distance = 400;
		else if (FStrEq(arg1, "2"))
			g_draw_distance = 500;
		else if (FStrEq(arg1, "3"))
			g_draw_distance = 600;
		else if (FStrEq(arg1, "4"))
			g_draw_distance = 700;
		else if (FStrEq(arg1, "5"))
			g_draw_distance = 800;
		else if (FStrEq(arg1, "6"))
			g_draw_distance = 900;
		else if (FStrEq(arg1, "7"))
			g_draw_distance = 1000;
		else if (FStrEq(arg1, "8"))
			g_draw_distance = 1200;
		else if (FStrEq(arg1, "9"))
			g_draw_distance = 1400;
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
			b_custom_wpts = FALSE;
		else if (FStrEq(arg1, "2"))
			b_custom_wpts = TRUE;
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
			if (b_observer_mode)
				b_observer_mode = FALSE;
			else
				b_observer_mode = TRUE;
		}
		else if (FStrEq(arg1, "2"))
		{
			if (b_freeze_mode)
				b_freeze_mode = FALSE;
			else
				b_freeze_mode = TRUE;
		}
		else if (FStrEq(arg1, "3"))
		{
			if (b_botdontshoot)
				b_botdontshoot = FALSE;
			else
				b_botdontshoot = TRUE;
		}
		else if (FStrEq(arg1, "4"))
		{
			if (b_botignoreall)
				b_botignoreall = FALSE;
			else
				b_botignoreall = TRUE;
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
#ifdef _DEBUG
		// currently only as a test command
		else if (FStrEq(arg1, "7"))
		{
			char fname[1024];
			char fname_only[1024];

			for (TeamPathMap_CI tpm_ci = ::teamPathMap.begin(); tpm_ci != ::teamPathMap.end(); ++tpm_ci)
			{
				sprintf(fname_only, "%s.%d", STRING(gpGlobals->mapname),tpm_ci->first);

				UTIL_MarineBotFileName(fname, "learn", fname_only);

				tpm_ci->second.save(fname);
			}
		}
#endif
		else if ((FStrEq(arg1, "8")) || (FStrEq(arg1, "9")))
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
* will just handle the command
* used for commands that are same for both DS as well as listenserver (ie. LAN game)
*/
void RandomSkillCommand(edict_t *pEntity, const char *arg1)
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
			PrintOutput(pEntity, "***toggle mode used***\n", msg_null);
		}

		if (externals.GetRandomSkill())
		{
			externals.SetRandomSkill(FALSE);
			PrintOutput(pEntity, "random_skill mode DISABLED\n", msg_default);
		}
		else
		{
			externals.SetRandomSkill(TRUE);
			PrintOutput(pEntity, "random_skill mode ENABLED\n", msg_default);
		}
	}
	else
	{
		// if there's some argument and the argument do match current then print current state
		if (strcmp(arg1, "current") == 0)
		{
			if (externals.GetRandomSkill())
				PrintOutput(pEntity, "random_skill mode is currently ENABLED\n", msg_default);
			else
				PrintOutput(pEntity, "random_skill mode is currently DISABLED\n", msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		// do we need to turn this on?
		else if (strcmp(arg1, "on") == 0)
		{
			externals.SetRandomSkill(TRUE);
			PrintOutput(pEntity, "random_skill mode ENABLED\n", msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		// do we need to turn this off?
		else if (strcmp(arg1, "off") == 0)
		{
			externals.SetRandomSkill(FALSE);
			PrintOutput(pEntity, "random_skill mode DISABLED\n", msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		// otherwise the user must have used wrong/invalid argument
		else
		{
			PrintOutput(pEntity, "invalid argument!\n", msg_error);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_FAILED);
		}
	}
}

void SpawnSkillCommand(edict_t *pEntity, const char *arg1)
{
	if (strlen(arg1) >= 1)
	{
		int temp = atoi(arg1);
		
		if ((temp >= 1) && (temp <= BOT_SKILL_LEVELS))
		{
			externals.SetSpawnSkill(temp);
			
			char msg[80];
			sprintf(msg, "default spawnskill is %d\n", externals.GetSpawnSkill());
			PrintOutput(pEntity, msg, msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else
		{
			PrintOutput(pEntity, "invalid spawnskill value!\n", msg_error);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_FAILED);
		}
	}
	else
	{
		PrintOutput(pEntity, "missing argument!\n", msg_error);

		if (!is_dedicated_server)
			PlaySoundConfirmation(pEntity, SND_FAILED);
	}
}

void SetBotSkillCommand(edict_t *pEntity, const char *arg1)
{
	if (strlen(arg1) >= 1)
	{
		int temp = 0;
		temp = atoi(arg1);
		
		if ((temp >= 1) && (temp <= BOT_SKILL_LEVELS))
		{
			int result = UTIL_ChangeBotSkillLevel(FALSE, temp);
			
			char msg[80];
			sprintf(msg, "botskill set to %d\n", result);
			PrintOutput(pEntity, msg, msg_default);
			
			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else
		{
			PrintOutput(pEntity, "invalid botskill value!\n", msg_error);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_FAILED);
		}
	}
	else
	{
		PrintOutput(pEntity, "missing argument!\n", msg_error);

		if (!is_dedicated_server)
			PlaySoundConfirmation(pEntity, SND_FAILED);
	}
}

void BotSkillUpCommand(edict_t *pEntity, const char *arg1)
{
	if (UTIL_ChangeBotSkillLevel(TRUE, 1) > 0)
	{
		PrintOutput(pEntity, "botskill increased by one level\n", msg_default);

		if (!is_dedicated_server)
			PlaySoundConfirmation(pEntity, SND_DONE);
	}
	else
	{
		PrintOutput(pEntity, "no bot on lower skill levels or no bot in game!\n", msg_error);

		if (!is_dedicated_server)
			PlaySoundConfirmation(pEntity, SND_FAILED);
	}
}

void BotSkillDownCommand(edict_t *pEntity, const char *arg1)
{
	if (UTIL_ChangeBotSkillLevel(TRUE, -1) > 0)
	{
		PrintOutput(pEntity, "botskill decreased by one level\n", msg_default);

		if (!is_dedicated_server)
			PlaySoundConfirmation(pEntity, SND_DONE);
	}
	else
	{
		PrintOutput(pEntity, "no bot on higher skill levels or no bot in game!\n", msg_error);

		if (!is_dedicated_server)
			PlaySoundConfirmation(pEntity, SND_FAILED);
	}
}

void SetAimSkillCommand(edict_t *pEntity, const char *arg1)
{
	int temp = 0;
	
	if (strlen(arg1) >= 1)
	{
		temp = atoi(arg1);

		if ((temp >= 1) && (temp <= BOT_SKILL_LEVELS))
		{
			int result = UTIL_ChangeAimSkillLevel(temp);

			char msg[80];
			sprintf(msg, "aimskill set to %d\n", result);
			PrintOutput(pEntity, msg, msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else
		{
			PrintOutput(pEntity, "invalid aimskill value!\n", msg_error);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_FAILED);
		}
	}
	else
	{
		PrintOutput(pEntity, "missing argument!\n", msg_error);

		if (!is_dedicated_server)
			PlaySoundConfirmation(pEntity, SND_FAILED);
	}
}

void SetReactionTimeCommand(edict_t *pEntity, const char *arg1)
{
	if (strlen(arg1) >= 1)
	{
		float temp = atoi(arg1);
		
		// first of all check if the argument is a digit
		if (isdigit(arg1[0]) && (temp >= 0) && (temp <= 50))
		{
			temp /= 10;
			externals.SetReactionTime((float) temp);

			char msg[80];
			sprintf(msg, "reaction_time is %.1fs\n", externals.GetReactionTime());
			PrintOutput(pEntity, msg, msg_default);
			
			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else if (FStrEq(arg1, "current"))
		{
			char msg[80];
			sprintf(msg, "reaction_time is %.1fs\n", externals.GetReactionTime());
			PrintOutput(pEntity, msg, msg_default);
			
			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else
		{
			PrintOutput(pEntity, "invalid reaction_time value!\n", msg_error);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_FAILED);
		}
	}
	else
	{
		PrintOutput(pEntity, "missing argument!\n", msg_error);

		if (!is_dedicated_server)
			PlaySoundConfirmation(pEntity, SND_FAILED);
	}
}

void RangeLimitCommand(edict_t *pEntity, const char *arg1)
{
	if (strlen(arg1) >= 1)
	{
		float temp = atoi(arg1);
		
		if (isdigit(arg1[0]) && (temp >= 500) && (temp <= 7500))
		{
			internals.SetMaxDistance((float) temp);
			internals.SetIsDistLimit(true);

			char msg[80];
			sprintf(msg, "range_limit is %.0f\n", internals.GetMaxDistance());
			PrintOutput(pEntity, msg, msg_default);
			
			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else if (FStrEq(arg1, "default"))
		{
			internals.ResetMaxDistance();
			internals.ResetIsDistLimit();

			char msg[80];
			sprintf(msg, "range_limit is set to default value (%.0f)\n", internals.GetMaxDistance());
			PrintOutput(pEntity, msg, msg_default);
			
			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else if (FStrEq(arg1, "current"))
		{
			char msg[80];
			sprintf(msg, "range_limit is %.0f\n", internals.GetMaxDistance());
			PrintOutput(pEntity, msg, msg_default);
			
			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else if ((FStrEq(arg1, "help")) || (FStrEq(arg1, "?")))
		{
			PrintOutput(pEntity, "All valid range_limit options\n", msg_null);
			PrintOutput(pEntity, "[range_limit <number>] the number can be from 500 to 7500\n", msg_null);
			PrintOutput(pEntity, "[range_limit <default>] sets it to default (ie. to 7500)\n", msg_null);
			PrintOutput(pEntity, "[range_limit <current>] returns current value\n", msg_null);
			PrintOutput(pEntity, "[range_limit <help>] shows this help\n", msg_null);
		}
		else
		{
			PrintOutput(pEntity, "invalid range_limit value!\n", msg_error);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_FAILED);
		}
	}
	else
	{
		PrintOutput(pEntity, "missing argument!\n", msg_error);

		if (!is_dedicated_server)
			PlaySoundConfirmation(pEntity, SND_FAILED);
	}
}

void SetWaypointDirectoryCommand(edict_t *pEntity, const char *arg1)
{
	if (strlen(arg1) >= 1)
	{
		if ((FStrEq(arg1, "help")) || (FStrEq(arg1, "?")))
		{
			PrintOutput(pEntity, "All valid directory options\n", msg_null);
			PrintOutput(pEntity, "[directory <defaultwpts>] points to \"/marine_bot/defaultwpts\" directory\n", msg_null);
			PrintOutput(pEntity, "[directory <customwpts>] points to \"/marine_bot/customwpts\" directory\n", msg_null);
			PrintOutput(pEntity, "[directory <current>] returns which directory is used now\n", msg_null);
			PrintOutput(pEntity, "[directory <help>] shows this help\n", msg_null);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else if (FStrEq(arg1, "customwpts"))
		{
			b_custom_wpts = TRUE;
			PrintOutput(pEntity, "waypoint are now loaded from \"customwpts\" directory\n", msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else if (FStrEq(arg1, "defaultwpts"))
		{
			b_custom_wpts = FALSE;
			PrintOutput(pEntity, "waypoint are now loaded from \"defaultwpts\" directory\n", msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else if (FStrEq(arg1, "current"))
		{
			if (b_custom_wpts)
				PrintOutput(pEntity, "waypoint are now loaded from \"customwpts\" directory\n", msg_default);
			else
				PrintOutput(pEntity, "waypoint are now loaded from \"defaultwpts\" directory\n", msg_default);				

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else
		{
			PrintOutput(pEntity, "invalid argument!\n", msg_error);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_FAILED);
		}
	}
	else
	{
		if (!is_dedicated_server)
		{
			PlaySoundConfirmation(pEntity, SND_DONE);
			PrintOutput(pEntity, "***toggle mode used***\n", msg_null);
		}
		
		if (b_custom_wpts)
		{
			b_custom_wpts = FALSE;
			PrintOutput(pEntity, "waypoint are now loaded from \"defaultwpts\" directory\n", msg_default);
		}
		else
		{
			b_custom_wpts = TRUE;
			PrintOutput(pEntity, "waypoint are now loaded from \"customwpts\" directory\n", msg_default);
		}
	}
}

void PassiveHealingCommand(edict_t *pEntity, const char *arg1)
{
	if (strlen(arg1) < 1)
	{
		if (!is_dedicated_server)
		{
			PlaySoundConfirmation(pEntity, SND_DONE);
			PrintOutput(pEntity, "***toggle mode used***\n", msg_null);
		}

		if (externals.GetPassiveHealing())
		{
			externals.SetPassiveHealing(FALSE);
			PrintOutput(pEntity, "passive_healing mode DISABLED\n", msg_default);
		}
		else
		{
			externals.SetPassiveHealing(TRUE);
			PrintOutput(pEntity, "passive_healing mode ENABLED\n", msg_default);
		}
	}
	else
	{
		if (strcmp(arg1, "current") == 0)
		{
			if (externals.GetPassiveHealing())
				PrintOutput(pEntity, "passive_healing mode is currently ENABLED\n", msg_default);
			else
				PrintOutput(pEntity, "passive_healing mode is currently DISABLED\n", msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else if (strcmp(arg1, "on") == 0)
		{
			externals.SetPassiveHealing(TRUE);
			PrintOutput(pEntity, "passive_healing mode ENABLED\n", msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else if (strcmp(arg1, "off") == 0)
		{
			externals.SetPassiveHealing(FALSE);
			PrintOutput(pEntity, "passive_healing mode DISABLED\n", msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else
		{
			PrintOutput(pEntity, "invalid argument!\n", msg_error);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_FAILED);
		}
	}
}

void DontSpeakCommand(edict_t *pEntity, const char *arg1)
{
	if (strlen(arg1) < 1)
	{
		if (!is_dedicated_server)
		{
			PlaySoundConfirmation(pEntity, SND_DONE);
			PrintOutput(pEntity, "***toggle mode used***\n", msg_null);
		}

		if (externals.GetDontSpeak())
		{
			externals.SetDontSpeak(FALSE);
			PrintOutput(pEntity, "dont_speak mode DISABLED\n", msg_default);
		}
		else
		{
			externals.SetDontSpeak(TRUE);
			PrintOutput(pEntity, "dont_speak mode ENABLED\n", msg_default);
		}
	}
	else
	{
		if (strcmp(arg1, "current") == 0)
		{
			if (externals.GetDontSpeak())
				PrintOutput(pEntity, "dont_speak mode is currently ENABLED\n", msg_default);
			else
				PrintOutput(pEntity, "dont_speak mode is currently DISABLED\n", msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else if (strcmp(arg1, "on") == 0)
		{
			externals.SetDontSpeak(TRUE);
			PrintOutput(pEntity, "dont_speak mode ENABLED\n", msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else if (strcmp(arg1, "off") == 0)
		{
			externals.SetDontSpeak(FALSE);
			PrintOutput(pEntity, "dont_speak mode DISABLED\n", msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else
		{
			PrintOutput(pEntity, "invalid argument!\n", msg_error);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_FAILED);
		}
		
	}
}

void DontChatCommand(edict_t *pEntity, const char *arg1)
{
	if (strlen(arg1) < 1)
	{
		if (!is_dedicated_server)
		{
			PlaySoundConfirmation(pEntity, SND_DONE);
			PrintOutput(pEntity, "***toggle mode used***\n", msg_null);
		}

		if (externals.GetDontChat())
		{
			externals.SetDontChat(FALSE);
			PrintOutput(pEntity, "dont_chat mode DISABLED\n", msg_default);
		}
		else
		{
			externals.SetDontChat(TRUE);
			PrintOutput(pEntity, "dont_chat mode ENABLED\n", msg_default);
		}
	}
	else
	{
		if (strcmp(arg1, "current") == 0)
		{
			if (externals.GetDontChat())
				PrintOutput(pEntity, "dont_chat mode is currently ENABLED\n", msg_default);
			else
				PrintOutput(pEntity, "dont_chat mode is currently DISABLED\n", msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else if (strcmp(arg1, "on") == 0)
		{
			externals.SetDontChat(TRUE);
			PrintOutput(pEntity, "dont_chat mode ENABLED\n", msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else if (strcmp(arg1, "off") == 0)
		{
			externals.SetDontChat(FALSE);
			PrintOutput(pEntity, "dont_chat mode DISABLED\n", msg_default);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_DONE);
		}
		else
		{
			PrintOutput(pEntity, "invalid argument!\n", msg_error);

			if (!is_dedicated_server)
				PlaySoundConfirmation(pEntity, SND_FAILED);
		}
	}
}

// handles MB Dedicated Server Commands
void MBServerCommand(void)
{
	const char *cmd, *arg1, *arg2, *arg3, *arg4, *arg5;
	char msg[256];

	cmd = CMD_ARGV (1);
	arg1 = CMD_ARGV (2);
	arg2 = CMD_ARGV (3);
	arg3 = CMD_ARGV (4);
	arg4 = CMD_ARGV (5);
	arg5 = CMD_ARGV (6);

	//@@@@@@@@
	//sprintf(msg, "SHOW ME THE LINE (new system)!!! <cmd(%s)><arg1(%s)><arg2(%s)><arg3(%s)><arg4(%s)><arg5(%s)>\n",
	//	cmd, arg1, arg2, arg3, arg4, arg5);
	//PrintOutput(NULL, msg, msg_null);
	//UTIL_DebugDev(msg, -100, -100);



	if ((strcmp(cmd, "help") == 0) || (strcmp(cmd, "?") == 0))
	{
		// beause of the steam dedicated console string formating system we are
		// forced to use only printable signs
		// (ie. no \n or \t in the string, \n will automatically end the string
		// so anything that's behind it won't be printed to the console)
		
		PrintOutput(NULL, "\n", msg_null);
		PrintOutput(NULL, "----------------------------------------------------\n", msg_null);
		PrintOutput(NULL, "Marine Bot dedicated server commands help\n", msg_null);
		PrintOutput(NULL, "----------------------------------------------------\n", msg_null);
		PrintOutput(NULL, "CVAR for MarineBot is [m_bot] so all cmds must start with m_bot\n", msg_null);
		PrintOutput(NULL, "[addmarine]  add bot with random team, class, name and default skill\n", msg_null);
		PrintOutput(NULL, "[addmarine1] add bot to red team with random class, name and default skill\n", msg_null);
		PrintOutput(NULL, "[addmarine2] add bot to blue team with random class, name and default skill\n", msg_null);
		PrintOutput(NULL, "[addmarine <team> <class> <skin> <skill> <name>] add fully customized bot\n", msg_null);
		PrintOutput(NULL, "[min_bots <number>] specify minimum of bots on server (31=disabled - no bot is kicked)\n", msg_null);
		PrintOutput(NULL, "[max_bots <number>] specify maximum of bots on server (0=disabled)\n", msg_null);
		PrintOutput(NULL, "[random_skill <current>] toggle between using default bot skill and generating the skill randomly for each bot, \"current\" returns actual state\n", msg_null);
		PrintOutput(NULL, "[spawn_skill <number>] set default bot skill on join (1-5 where 1=best)\n", msg_null);
		PrintOutput(NULL, "[set_botskill <number>] set bot skill level for all bots already in game\n", msg_null);
		PrintOutput(NULL, "[botskill_up] increase bot skill level for all bots already in game\n", msg_null);
		PrintOutput(NULL, "[botskill_down] decrease bot skill level for all bots already in game\n", msg_null);
		PrintOutput(NULL, "[set_aimskill <number>] set aim skill level for all bots already in game\n", msg_null);
		PrintOutput(NULL, "[reaction_time <number>] set bot reaction time (0-50 where 1 will be converted to 0.1s and 50 to 5.0s)\n", msg_null);
		PrintOutput(NULL, "[range_limit <number>] set the max distance of enemy the bot can see & attack (500-7500 units)\n", msg_null);
		PrintOutput(NULL, "[balance_teams] try to balance teams on server (move bots to weaker team)\n", msg_null);
		PrintOutput(NULL, "[auto_balance <number>] set time for team balance checks ie. autobalancing (30-3600s where 30 is 30seconds and 3600 is 1hour, setting it to 0=never do team balance)\n", msg_null);
		PrintOutput(NULL, "[send_info <number>] set time for info message gets send (30-3600s; 0=off)\n", msg_null);
		PrintOutput(NULL, "[send_presentation <number>] set time for presentation message gets send (0-3600s; 0=off)\n", msg_null);
		PrintOutput(NULL, "[clients] print clients/bots count currently on server\n", msg_null);
		PrintOutput(NULL, "[version_info] print MB version\n", msg_null);
		PrintOutput(NULL, "[load_unsupported] convert & save older waypoint file\n", msg_null);
		PrintOutput(NULL, "[directory <current>] toggle through waypoint directories, \"current\" returns name of current directory\n", msg_null);
		PrintOutput(NULL, "[dont_speak <current>] bot will not use Voice&Radio commands, \"current\" returns actual state\n", msg_null);
		PrintOutput(NULL, "[dont_chat <current>] bot will not use say&say_team commands, \"current\" returns actual state\n", msg_null);
		PrintOutput(NULL, "[passive_healing <current>] bot will not heal teammates automatically, \"current\" returns actual state\n", msg_null);
		PrintOutput(NULL, "------------------------------------------\n", msg_null);
	}
	// spawn random team bot
	else if (strcmp(cmd, "addmarine") == 0)
	{
		// allow more than maxbots bots on the server for the rest of current map
		override_max_bots = TRUE;
		
		BotCreate( NULL, arg1, arg2, arg3, arg4, arg5 );
		
		bot_check_time = gpGlobals->time + 2.5;
	}
	// spawn red bot
	else if (strcmp(cmd, "addmarine1") == 0)
	{
		override_max_bots = TRUE;
		BotCreate( NULL, "1", NULL, NULL, NULL, NULL );
		
		bot_check_time = gpGlobals->time + 2.5;
	}
	// spawn blue bot
	else if (strcmp(cmd, "addmarine2") == 0)
	{
		override_max_bots = TRUE;
		BotCreate( NULL, "2", NULL, NULL, NULL, NULL );

		bot_check_time = gpGlobals->time + 2.5;
	}
	else if (strcmp(cmd, "min_bots") == 0)
	{
		if ((arg1 != NULL) && (arg1[0] != 0))
		{
			externals.SetMinBots((int) atoi( arg1 ));

			if ((externals.GetMinBots() < 0) || (externals.GetMinBots() > 31))
			{
				externals.ResetMinBots();
						
				sprintf(msg, "MARINE_BOT ERROR - min_bots reset to %d\n", externals.GetMinBots());
			}
			else if (externals.GetMinBots() == 31)
			{
				externals.SetMinBots(-1);
				
				sprintf(msg, "MARINE_BOT - min_bots DISABLED!\n");
			}
			else
				sprintf(msg, "MARINE_BOT - min_bots set to %d\n", externals.GetMinBots());
		}
		else
			sprintf(msg, "MARINE_BOT ERROR - invalid or missing argument!\n");
		
		// print the message into console
		// if logging is on it does log it to logfile
		PrintOutput(NULL, msg, msg_null);
	}
	else if (strcmp(cmd, "max_bots") == 0)
	{
		if ((arg1 != NULL) && (arg1[0] != 0))
		{
			externals.SetMaxBots((int) atoi( arg1 ));
			
			if ((externals.GetMaxBots() < 0) || (externals.GetMaxBots() > 31))
			{
				externals.ResetMaxBots();
				
				sprintf(msg, "MARINE_BOT ERROR - max_bots reset to %d\n", externals.GetMaxBots());
			}
			else if (externals.GetMaxBots() == 0)
			{
				externals.SetMaxBots(-1);
				
				sprintf(msg, "MARINE_BOT - max_bots DISABLED!\n");
			}
			else
				sprintf(msg, "MARINE_BOT - max_bots set to %d\n", externals.GetMaxBots());
		}
		else
			sprintf(msg, "MARINE_BOT ERROR - invalid or missing argument!\n");
		
		PrintOutput(NULL, msg, msg_null);
	}
	else if (strcmp(cmd, "random_skill") == 0)
	{
		RandomSkillCommand(NULL, arg1);
	}
	else if (strcmp(cmd, "spawn_skill") == 0)
	{
		SpawnSkillCommand(NULL, arg1);
	}
	else if (strcmp(cmd, "set_botskill") == 0)
	{
		SetBotSkillCommand(NULL, arg1);
	}				
	else if (strcmp(cmd, "botskill_up") == 0)
	{
		BotSkillUpCommand(NULL, arg1);
	}
	else if (strcmp(cmd, "botskill_down") == 0)
	{
		BotSkillDownCommand(NULL, arg1);
	}
	else if (strcmp(cmd, "set_aimskill") == 0)
	{
		SetAimSkillCommand(NULL, arg1);
	}
	else if (strcmp(cmd, "reaction_time") == 0)
	{
		SetReactionTimeCommand(NULL, arg1);
	}
	else if (strcmp(cmd, "range_limit") == 0)
	{
		RangeLimitCommand(NULL, arg1);
	}
	// set the time for team balance checking
	else if (strcmp(cmd, "auto_balance") == 0)
	{
		if ((arg1 != NULL) && (arg1 != 0))
		{
			float temp = atoi(arg1);
			
			if (temp <= 0)
			{
				do_auto_balance = FALSE;	// added by kota@
				externals.SetBalanceTime(0.0);
				// needed to proprerly deactivate autobalancing
				check_auto_balance = gpGlobals->time - 0.1;
				
				sprintf(msg, "MARINE_BOT - auto_balance DISABLED!\n");

				// PREVIOUS CODE
				// clear message ie will print nothing
				//*msg = NULL;
			}
			else if ((temp >= 30.0) && (temp <= 3600.0))
			{
				do_auto_balance = TRUE;	// added by kota@
				externals.SetBalanceTime((float) temp);
				sprintf(msg, "MARINE_BOT - auto_balance time is %.0fs\n",
					externals.GetBalanceTime());
				
				check_auto_balance = gpGlobals->time + externals.GetBalanceTime();
			}
			else
				sprintf(msg, "MARINE_BOT ERROR - invalid auto_balance time value!\n");
		}

		PrintOutput(NULL, msg, msg_null);
	}
	// do the team balancing manually
	else if (strcmp(cmd, "balance_teams") == 0)
	{
		team_balance_value = UTIL_BalanceTeams();
		
		if (team_balance_value == -2)
			sprintf(msg, "MARINE_BOT ERROR - server is empty!\n");
		else if (team_balance_value == -1)
			sprintf(msg, "MARINE_BOT ERROR - there are no bots!\n");
		else if (team_balance_value == 0)
			sprintf(msg, "MARINE_BOT - teams are balanced\n");
		else if (team_balance_value > 100)
			sprintf(msg, "MARINE_BOT - balancing in progress... (moving %d bots from RED team to blue)\n", team_balance_value - 100);
		else if ((team_balance_value > 0) && (team_balance_value < 100))
			sprintf(msg, "MARINE_BOT - balancing in progress... (moving %d bots from BLUE team to red)\n", team_balance_value);
		else if (team_balance_value < -2)
			sprintf(msg, "MARINE_BOT ERROR - internal error\n");
		
		PrintOutput(NULL, msg, msg_null);
	}
	// set the time between two info notifications
	else if (strcmp(cmd, "send_info") == 0)
	{
		if ((arg1 != NULL) && (arg1 != 0))
		{
			float temp = atoi(arg1);
			
			if (temp == 0)
			{
				externals.SetInfoTime(0.0);
				check_send_info = -1.0;
				
				sprintf(msg, "MARINE_BOT - send_info DISABLED!\n");
			}
			else if ((temp >= 30.0) && (temp <= 3600.0))
			{
				externals.SetInfoTime((float) temp);
				sprintf(msg, "MARINE_BOT - send_info time is %.0fs\n", externals.GetInfoTime());
				
				check_send_info = gpGlobals->time + externals.GetInfoTime();
			}
			else
				sprintf(msg, "MARINE_BOT ERROR - invalid send_info time value!\n");
		}
		
		PrintOutput(NULL, msg, msg_null);
	}
	// set the time between two presentation messages
	else if (strcmp(cmd, "send_presentation") == 0)
	{
		if ((arg1 != NULL) && (arg1 != 0))
		{
			float temp = atoi(arg1);

			if (temp == 0.0)
			{
				presentation_time = 0.0;

				sprintf(msg, "MARINE_BOT - send_presentation DISABLED!\n");
			}
			else if ((temp > 0.0) && (temp <= 3600.0))
			{
				// just for sure ie. to prevent engine overloading
				if (temp < 10.0)
					externals.SetPresentationTime((float) 30.0);
				else
					externals.SetPresentationTime((float) temp);

				sprintf(msg, "MARINE_BOT - send_presentation time is %.0fs\n",
					externals.GetPresentationTime());
				
				presentation_time = gpGlobals->time + externals.GetPresentationTime();
			}
			else
				sprintf(msg, "MARINE_BOT ERROR - invalid send_presentation time value!\n");
		}
		
		PrintOutput(NULL, msg, msg_null);
	}
	// get some info about all clients on the server
	else if (strcmp(cmd, "clients") == 0)
	{
		int actual_pl, red_clients, red_bots, blue_clients, blue_bots;
		
		actual_pl = red_clients = red_bots = blue_clients = blue_bots = 0;

		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (clients[i].pEntity != NULL)
			{
				actual_pl++;

				if (clients[i].pEntity->v.team == 1)
				{
					red_clients++;

					if (clients[i].pEntity->v.flags & FL_FAKECLIENT)
						red_bots++;
				}
				else if (clients[i].pEntity->v.team == 2)
				{
					blue_clients++;

					if (clients[i].pEntity->v.flags & FL_FAKECLIENT)
						blue_bots++;
				}
			}
		}
		
		if (actual_pl == 0)
			PrintOutput(NULL, "the server is empty\n", msg_warning);
		else
		{
			if ((red_bots == 0) && (blue_bots == 0))
			{
				PrintOutput(NULL, "there are no bots!\n", msg_warning);
				PrintOutput(NULL, "clients analyzed\n", msg_null);
				PrintOutput(NULL, "------------------------------------\n", msg_null);
				sprintf(msg, "total clients on server: %d\n", actual_pl);
				PrintOutput(NULL, msg, msg_null);
				sprintf(msg, "red clients: %d\n", red_clients);
				PrintOutput(NULL, msg, msg_null);
				sprintf(msg, "blue clients: %d\n", blue_clients);
				PrintOutput(NULL, msg, msg_null);
				PrintOutput(NULL, "------------------------------------\n", msg_null);
			}
			else
			{
				PrintOutput(NULL, "clients analyzed\n", msg_default);
				PrintOutput(NULL, "------------------------------------\n", msg_null);
				sprintf(msg, "total clients on server: %d\n", actual_pl);
				PrintOutput(NULL, msg, msg_null);
				sprintf(msg, "red team bots: %d out of %d red clients\n", red_bots, red_clients);
				PrintOutput(NULL, msg, msg_null);
				sprintf(msg, "blue team bots: %d out of %d blue clients\n",
					blue_bots, blue_clients);
				PrintOutput(NULL, msg, msg_null);
				PrintOutput(NULL, "------------------------------------\n", msg_null);
			}
		}
	}
	else if (strcmp(cmd, "version_info") == 0)
	{
		sprintf(msg, "You are using MarineBot version: %s\n", mb_version_info);
		PrintOutput(NULL, msg, msg_default);
	}
	else if (strcmp(cmd, "load_unsupported") == 0)
	{
		PrintOutput(NULL, "starting conversion...\n", msg_default);
		
		if (WaypointLoadUnsupported(NULL))
		{
			if (WaypointPathLoadUnsupported(NULL))
			{
				WaypointSave(NULL);
				
				PrintOutput(NULL, "older waypoints and paths converted and saved\n", msg_info);
			}
		}
	}
	else if (strcmp(cmd, "directory") == 0)
	{
		SetWaypointDirectoryCommand(NULL, arg1);
	}
	else if (strcmp(cmd, "passive_healing") == 0)
	{
		PassiveHealingCommand(NULL, arg1);
	}
	else if (strcmp(cmd, "dont_speak") == 0)
	{
		DontSpeakCommand(NULL, arg1);
	}
	else if (strcmp(cmd, "dont_chat") == 0)
	{
		DontChatCommand(NULL, arg1);
	}
	else
	{
		PrintOutput(NULL, "invalid or unknown command!\n", msg_error);
		PrintOutput(NULL, "write [m_bot help] or [m_bot ?] into your console for command help\n", msg_info);
	}
}



#ifdef _DEBUG
inline bool CheckForSomeDebuggingCommands(edict_t *pEntity, const char *pcmd, const char *arg1, const char *arg2, const char *arg3, const char *arg4, const char *arg5)
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
	else if (FStrEq(pcmd, "modif"))
	{
		extern float modif;

		if ((arg1 != NULL) && (*arg1 != 0))
		{
			int temp = atoi(arg1);
				
			modif = (float) temp;
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
				
			modif = (float) temp;
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



	else if ((strcmp(pcmd, "debugengine") == 0) || (strcmp(pcmd, "debug_engine") == 0))
	{
		if (debug_engine)
		{
			debug_engine = 0;
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_engine DISBLED!\n");
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
		for (int i=0; i < MAX_CLIENTS; i++)
		{
			if (clients[i].pEntity)
				UTIL_DumpEdictToFile(clients[i].pEntity);
		}
	}
	// store map learning information
	else if (strcmp(pcmd, "pathmap_save") == 0)
	{
		char fname[1024];
		char fname_only[1024];

		for (TeamPathMap_CI tpm_ci = ::teamPathMap.begin(); tpm_ci != ::teamPathMap.end();
				++tpm_ci)
		{
			sprintf(fname_only, "%s.%d", STRING(gpGlobals->mapname),tpm_ci->first);
			UTIL_MarineBotFileName(fname, "learn", fname_only);
			
			tpm_ci->second.save(fname);
		}
	}

	// @@@@@@@@@@@ just for test things
	else if ((strcmp(pcmd, "test_clientdetection") == 0) || (strcmp(pcmd, "testclientdetection") == 0))
	{
		int cl_count, bot_count, human_count;
		cl_count = bot_count = human_count = 0;

		for (int i=0; i < MAX_CLIENTS; i++)
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
		PrintOutput(NULL, msg, msg_null);
		
		sprintf(msg, "number of bots - (by cl array: %d) (counted now: %d)\n",
			clients[0].BotCount(), bot_count);
		PrintOutput(NULL, msg, msg_null);
		
		sprintf(msg, "number of humans: (by cl array: %d) (counted now: %d)\n",
			clients[0].HumanCount(), human_count);
		PrintOutput(NULL, msg, msg_null);
	}




	//TEMP: need cmd to change the default max effective range modifier for all weapons
	else if (FStrEq(pcmd, "trng"))
	{
		extern float rg_modif;

		if ((arg1 != NULL) && (*arg1 != 0))
		{
			int temp = atoi(arg1);

			rg_modif = (float) temp;
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

		for (int i=0; i<MAX_BOT_NAMES; i++)
		{
			if (bot_names[i].is_used)
				used++;
			else
				free++;
		}

		ALERT(at_console, "USED=%d | FREE=%d | TOTAL=%d\n", used, free, used+free);

		return TRUE;
	}

	//TEMP: need cmd to test sounds
	else if (FStrEq(pcmd, "debug_sound"))
	{
		if (debug_sound)
		{
			debug_sound = FALSE;
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_sound off\n");
		}
		else
		{
			debug_sound = TRUE;
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_sound on\n");
		}

		return TRUE;
	}

	else if (FStrEq(pcmd, "debug_bot"))		// print debug info about "bot name"
	{
		if ((arg1 == NULL) || (*arg1 == 0))
		{
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "missing or invalid argument!\n");

			return TRUE;
		}

		//char search_name[32];

		if (FStrEq(arg1, "off"))
		{
			g_debug_bot = NULL;

			ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_bot mode DISABLED\n");

			return TRUE;
		}

		int index = UTIL_FindBotByName(arg1);

		if (index != -1)
		{
			g_debug_bot = bots[index].pEdict;

			ClientPrint(pEntity, HUD_PRINTNOTIFY, "bot found and debug mode ENABLED\n");
		}
		else
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "no bot with that name!\n");

		return TRUE;
	}

	//TEMP: need cmd to test human player
	else if (FStrEq(pcmd, "debug_user"))
	{
		if (debug_user)
		{
			debug_user = FALSE;
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "test user off\n");
		}
		else
		{
			debug_user = TRUE;
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "test user on\n");
		}

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
			sprintf(msg, "The effective range of bot's main weapon is %.2f\n",
				bots[index].GetEffectiveRange());
			ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
		}
		else
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "no bot with that name!\n");

		return TRUE;
	}

	//TEMP: need cmd to test combat movements
	else if (FStrEq(pcmd, "tbc"))
	{
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (bots[i].is_used)
			{
				sprintf(msg, "<%d>advT %.1f | OverrideAdvT %.1f | CPosT %.1f | reloadT %.1f | globtime %.1f\n",
					i, bots[i].f_combat_advance_time, bots[i].f_overide_advance_time,
					bots[i].f_check_stance_time, bots[i].f_reload_time, gpGlobals->time);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

				sprintf(msg, "<%d>snpT %.1f | waitT %.1f | dntmove %d | wait_f_enem %.1f\n",
					i, bots[i].sniping_time, bots[i].wpt_wait_time,
					bots[i].IsTask(TASK_DONTMOVE), bots[i].f_bot_wait_for_enemy_time);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

				if (bots[i].bot_behaviour & BOT_PRONED)
					ALERT(at_console, "Proned\n");
				if (bots[i].bot_behaviour & BOT_CROUCHED)
					ALERT(at_console, "Crouched\n");
				if (bots[i].bot_behaviour & BOT_STANDING)
					ALERT(at_console, "Standing\n");

				if (bots[i].pEdict->v.flags & FL_PRONED)
					ALERT(at_console, "<%d> bot is proned\n", i);
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
						bots[i].curr_rgAmmo[weapon_defs[gren].iAmmo2],weapon_defs[gren].iAmmo2Max,
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

				if (!(bots[i].bot_weapons & (1<<gren)))
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
		bot_weapon_select_t *pSelect = NULL;
		bot_fire_delay_t *pDelay = NULL;

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


	//TEMP: need cmd to test weaponarrays
	else if (FStrEq(pcmd, "dumpw"))
	{
		bot_weapon_select_t *pSelect = NULL;
		bot_fire_delay_t *pDelay = NULL;

		pSelect = &bot_weapon_select[0];
		pDelay = &bot_fire_delay[0];

		FILE *fp;

		UTIL_DebugDev("***New Dump***", -100, -100);

		fp = fopen(debug_fname, "a");

		if (fp)
		{
			for (int i=0; i < MAX_WEAPONS; i++)
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

				if (bots[i].weapon_status & WA_ALLOWSILENCER)
					IsAllowSil = TRUE;
				if (bots[i].weapon_status & WA_MOUNTSILENCER)
					IsMountSil = TRUE;
				if (bots[i].weapon_status & WA_SILENCERUSED)
					IsSilUsed = TRUE;
				if (bots[i].weapon_action == W_READY)
					IsWeapReady = TRUE;
				if (bots[i].weapon_status & WA_USEAKIMBO)
					Akimbo = TRUE;

				ALERT(at_console, "weapon status value: %d | AllowSil %d | MountSil %d | SilUsed %d || Weap.iAttach %d || IsWeaponReady %d || iusr3 %d || akimbo %d (diff que. %d)\n",
					bots[i].weapon_status, IsAllowSil, IsMountSil, IsSilUsed, bots[i].current_weapon.iAttachment,
					IsWeapReady, bots[i].pEdict->v.iuser3,
					UTIL_IsBitSet(WA_USEAKIMBO, bots[i].weapon_status), Akimbo);
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
					bots[i].f_shoot_time,user_name,target_name);
			}
		}

		return TRUE;
	}

	//TEMP: need cmd to test speaking
	else if (FStrEq(pcmd, "tsay"))
	{
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (bots[i].is_used)
			{
				UTIL_Say(bots[i].pEdict, "Hello everyone");
			}
		}

		return TRUE;
	}

	//TEMP: need cmd to test speaking
	else if (FStrEq(pcmd, "ttsay"))
	{
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (bots[i].is_used)
			{
				UTIL_TeamSay(bots[i].pEdict, "Hello team");
			}
		}

		return TRUE;
	}

	//TEMP: need cmd to test wpts
	else if (FStrEq(pcmd, "dumpwpt"))
	{
		extern void Wpt_Dump(void);

		Wpt_Dump();

		return TRUE;
	}

	//TEMP: need cmd to test wpts
	else if (FStrEq(pcmd, "checkwpt"))
	{
		extern void Wpt_Check(void);

		Wpt_Check();

		return TRUE;
	}

	//TEMP: need cmd to test wpts
	else if (FStrEq(pcmd, "dumppth"))
	{
		extern void Pth_Dump(void);

		Pth_Dump();

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
					bots[i].curr_aim_index + 1, bots[i].aim_index[0]+1, bots[i].aim_index[1]+1,
					bots[i].aim_index[2]+1,bots[i].aim_index[3]+1);
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
	else if (FStrEq(pcmd, "tbw"))
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
				else if (bots[i].weapon_action == W_INRELOAD)
					strcpy(w_action, "INRELOAD");
				else
					strcpy(firemode, "NONE-ERROR");

				ALERT(at_console, "<%d>WeaponID %d | WeapFMode %s | iAttach %d | secFireActive %d | ZOOM(iuser3) %d | BotFOV %1.f\n",
					i, bots[i].current_weapon.iId, firemode, bots[i].current_weapon.iAttachment,
					bots[i].secondary_active, bots[i].pEdict->v.iuser3,
					bots[i].pEdict->v.fov);
	
				ALERT(at_console, "<%d>Ammo in chamber/clip %d | Primary reserve ammo/clips %d | Secondary reserve ammo/clips %d\n",
					i, bots[i].current_weapon.iClip, bots[i].current_weapon.iAmmo1,
					bots[i].current_weapon.iAmmo2);

				ALERT(at_console, "<%d>Using %s | WeaponAction %s | MainNoAMMO %d | BackupNoAMMO %d | EnemyDist %.1f\n",
					i, w_using, w_action, bots[i].main_no_ammo, bots[i].backup_no_ammo,
					bots[i].f_prev_enemy_dist);
	
				ALERT(at_console, "<%d>MainW ammo/clips needed to be taken %d | BackupW ammo/clips needed to be taken %d\n",
					i, bots[i].take_main_mags, bots[i].take_backup_mags);

				ALERT(at_console, "<%d>Is Bipod %d | MainW ID %d | BackupW ID %d\n",
					i, bots[i].IsTask(TASK_BIPOD), bots[i].main_weapon, bots[i].backup_weapon);

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

				ALERT(at_console, "<%d>MainW ID %d | MainW AmmoID %d | BackupW ID %d | BackupW AmmoID %d | Gren ID %d | Gren AmmoID %d\n",
					i, bots[i].main_weapon, mainW_ammoID, bots[i].backup_weapon, backupW_ammoID,
					bots[i].grenade_slot, nade_ammoID);
			}
		}
		else
		{
			for (int i = 0; i < MAX_CLIENTS; i++)
			{
				if (bots[i].is_used)
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
					else if (bots[i].weapon_action == W_INRELOAD)
						strcpy(w_action, "INRELOAD");
					else
						strcpy(firemode, "NONE-ERROR");

					ALERT(at_console, "<%d>WeaponID %d | WeapFMode %s | iAttach %d | secFireActive %d | ZOOM(iuser3) %d | BotFOV %1.f\n",
						i, bots[i].current_weapon.iId, firemode, bots[i].current_weapon.iAttachment,
						bots[i].secondary_active, bots[i].pEdict->v.iuser3,
						bots[i].pEdict->v.fov);
	
					ALERT(at_console, "<%d>Ammo in chamber/clip %d | Primary reserve ammo/clips %d | Secondary reserve ammo/clips %d\n",
						i, bots[i].current_weapon.iClip, bots[i].current_weapon.iAmmo1,
						bots[i].current_weapon.iAmmo2);
	
					ALERT(at_console, "<%d>Using %s | WeaponAction %s | MainNoAMMO %d | BackupNoAMMO %d | EnemyDist %.1f\n",
						i, w_using, w_action, bots[i].main_no_ammo, bots[i].backup_no_ammo,
						bots[i].f_prev_enemy_dist);
	
					ALERT(at_console, "<%d>MainW ammo/clips needed to be taken %d | BackupW ammo/clips needed to be taken %d\n",
						i, bots[i].take_main_mags, bots[i].take_backup_mags);
	
					ALERT(at_console, "<%d>Is Bipod %d | MainW ID %d | BackupW ID %d\n",
						i, bots[i].IsTask(TASK_BIPOD), bots[i].main_weapon, bots[i].backup_weapon);

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

				ALERT(at_console, "<%d>MainW ID %d | MainW AmmoID %d | BackupW ID %d | BackupW AmmoID %d | Gren ID %d | Gren AmmoID %d\n",
					i, bots[i].main_weapon, mainW_ammoID, bots[i].backup_weapon, backupW_ammoID,
					bots[i].grenade_slot, nade_ammoID);
				}
			}
		}

		return TRUE;
	}

	//TEMP: need cmd to set weapon usage to main weapon
	else if (FStrEq(pcmd, "sbwm"))
	{
		if ((arg1 != NULL) && (*arg1 != 0))
		{
			int i = UTIL_FindBotByName(arg1);

			if (i != -1 && bots[i].is_used)
			{
				bots[i].forced_usage = USE_MAIN;
				bots[i].main_no_ammo = FALSE;
				
				ClientPrint(pRecipient, HUD_PRINTCONSOLE, "Weapon usage set to main weapon\n");
			}
		}
		else
		{
			if (bots[0].is_used)
			{
				bots[0].forced_usage = USE_MAIN;
				bots[0].main_no_ammo = FALSE;
				
				ClientPrint(pRecipient, HUD_PRINTCONSOLE, "Weapon usage set to main weapon\n");
			}
		}

		return TRUE;
	}
	//TEMP: need cmd to set weapon usage to backup weapon
	else if (FStrEq(pcmd, "sbwb"))
	{
		if ((arg1 != NULL) && (*arg1 != 0))
		{
			int i = UTIL_FindBotByName(arg1);

			if (i != -1 && bots[i].is_used)
			{
				bots[i].forced_usage = USE_BACKUP;
				bots[i].main_no_ammo = TRUE;
				
				ClientPrint(pRecipient, HUD_PRINTCONSOLE, "Weapon usage set to backup weapon (main no ammo set too)\n");
			}
		}
		else
		{
			if (bots[0].is_used)
			{
				bots[0].forced_usage = USE_BACKUP;
				bots[0].main_no_ammo = TRUE;
				
				ClientPrint(pRecipient, HUD_PRINTCONSOLE, "Weapon usage set to backup weapon (main no ammo set too)\n");
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
					(int)bots[i].prev_wpt_origin.x,	(int)bots[i].prev_wpt_origin.y, (int)bots[i].prev_wpt_origin.z,
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
					bots[i].claymore_slot, bots[i].clay_action,	used_as_goal,
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

					/*
					ALERT(at_console, "curwpt = %d | pwpt0 = %d | pwpt1 = %d | pwpt2 = %d | pwpt3 = %d | pwpt4 = %d |\ncuwpt.x = %d | cuwpt.y = %d | cuwpt.z = %d| pwpt.x = %d | pwpt.y = %d | pwpt.z = %d | cupath = %d | pevpath = %d\n",
						bots[i].curr_wpt_index + 1, bots[i].prev_wpt_index[0] + 1,
						bots[i].prev_wpt_index[1] + 1, bots[i].prev_wpt_index[2] + 1,
						bots[i].prev_wpt_index[3] + 1, bots[i].prev_wpt_index[4] + 1,
						(int)bots[i].wpt_origin.x, (int)bots[i].wpt_origin.y, (int)bots[i].wpt_origin.z,
						(int)bots[i].prev_wpt_origin.x,	(int)bots[i].prev_wpt_origin.y, (int)bots[i].prev_wpt_origin.z,
						bots[i].curr_path_index + 1, bots[i].prev_path_index + 1);
					*/
					ALERT(at_console, "curwpt = %d | pwpt0 = %d | pwpt1 = %d | pwpt2 = %d | pwpt3 = %d | pwpt4 = %d |\ncuwpt.x = %d | cuwpt.y = %d | cuwpt.z = %d| pwpt.x = %d | pwpt.y = %d | pwpt.z = %d | cupath = %d | pevpath = %d\n",
						bots[i].curr_wpt_index + 1, bots[i].prev_wpt_index.get() + 1,
						bots[i].prev_wpt_index.get(1) + 1, bots[i].prev_wpt_index.get(2) + 1,
						bots[i].prev_wpt_index.get(3) + 1, bots[i].prev_wpt_index.get(4) + 1,
						(int)bots[i].wpt_origin.x, (int)bots[i].wpt_origin.y, (int)bots[i].wpt_origin.z,
						(int)bots[i].prev_wpt_origin.x,	(int)bots[i].prev_wpt_origin.y, (int)bots[i].prev_wpt_origin.z,
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
						bots[i].claymore_slot, bots[i].clay_action,	used_as_goal,
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

	//TEMP: need cmd to test behaviour
	else if (FStrEq(pcmd, "tbe"))
	{
		char behaviour_byte1[32], behaviour_byte2[32];

		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (bots[i].is_used)
			{
				if (bots[i].bot_behaviour & ATTACKER)
					strcpy(behaviour_byte1,"attacker");
				else if (bots[i].bot_behaviour & DEFENDER)
					strcpy(behaviour_byte1,"defender");
				else if (bots[i].bot_behaviour & STANDARD)
					strcpy(behaviour_byte1,"standard");
				else
					strcpy(behaviour_byte1,"unknown");

				if (bots[i].bot_behaviour & SNIPER)
					strcpy(behaviour_byte2,"sniper");
				else if (bots[i].bot_behaviour & MGUNNER)
					strcpy(behaviour_byte2,"mgunner");
				else if (bots[i].bot_behaviour & CQUARTER)
					strcpy(behaviour_byte2,"close_quarter");
				else if (bots[i].bot_behaviour & COMMON)
					strcpy(behaviour_byte2,"common_soldier");

				ALERT(at_console, "bot <%s> behaviour: %s - (%s) ||num %d\n",
					bots[i].name, behaviour_byte1, behaviour_byte2,
					bots[i].bot_behaviour);
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
					strcat(values," treat ");
				if (bots[i].IsSubTask(ST_HEAL))
					strcat(values," heal ");
				if (bots[i].IsSubTask(ST_GIVE))
					strcat(values," give ");
				if (bots[i].IsSubTask(ST_HEALED))
					strcat(values," healed ");

				strcat(values,">");

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
	else if (FStrEq(pcmd, "tbb"))
	{
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (bots[i].is_used)
			{
				ALERT(at_console, "bot <%s> additional behaviour flags (as string representation):\n",
					bots[i].name);
				
				if (bots[i].IsBehaviour(BOT_STANDING))
					ALERT(at_console, "<bot_standing>");
				if (bots[i].IsBehaviour(BOT_CROUCHED))
					ALERT(at_console, "<bot_crouched>");
				if (bots[i].IsBehaviour(BOT_PRONED))
					ALERT(at_console, "<bot_proned>");
				
				ALERT(at_console,"\n and behaviour as the number: %d\n",
					bots[i].bot_behaviour);
			}
		}

		return TRUE;
	}

	//TEMP: need cmd to test tasks
	else if (FStrEq(pcmd, "tbt"))
	{
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (bots[i].is_used)
			{
				ALERT(at_console, "bot <%s> all his tasks as strings\n", bots[i].name);

				if (bots[i].IsTask(TASK_PROMOTED))
					ALERT(at_console, "<promoted>");
				if (bots[i].IsTask(TASK_DEATHFALL))
					ALERT(at_console, "<deathfall>");
				if (bots[i].IsTask(TASK_DONTMOVE))
					ALERT(at_console, "<dontmove>");
				if (bots[i].IsTask(TASK_BREAKIT))
					ALERT(at_console, "<breakit>");
				if (bots[i].IsTask(TASK_FIRE))
					ALERT(at_console, "<fire>");
				if (bots[i].IsTask(TASK_IGNOREAIMS))
					ALERT(at_console, "<ignoreaims>");
				if (bots[i].IsTask(TASK_PRECISEAIM))
					ALERT(at_console, "<preciseaim>");
				if (bots[i].IsTask(TASK_CHECKAMMO))
					ALERT(at_console, "<checkammo>");
				if (bots[i].IsTask(TASK_GOALITEM))
					ALERT(at_console, "<goalitem>");
				if (bots[i].IsTask(TASK_NOJUMP))
					ALERT(at_console, "<nojump>");
				if (bots[i].IsTask(TASK_SPRINT))
					ALERT(at_console, "<sprint>");
				if (bots[i].IsTask(TASK_WPTACTION))
					ALERT(at_console, "<wptaction>");
				if (bots[i].IsTask(TASK_BACKTOPATROL))
					ALERT(at_console, "<backtopatrol>");
				if (bots[i].IsTask(TASK_PARACHUTE))
					ALERT(at_console, "<parachute>");
				if (bots[i].IsTask(TASK_PARACHUTE_USED))
					ALERT(at_console, "<parachute_used>");
				if (bots[i].IsTask(TASK_BLEEDING))
					ALERT(at_console, "<bleeding>");
				if (bots[i].IsTask(TASK_DROWNING))
					ALERT(at_console, "<drowning>");
				if (bots[i].IsTask(TASK_HEALHIM))
					ALERT(at_console, "<healhim>");
				if (bots[i].IsTask(TASK_MEDEVAC))
					ALERT(at_console, "<medevac>");
				if (bots[i].IsTask(TASK_FIND_ENEMY))
					ALERT(at_console, "<find enemy>");
				if (bots[i].IsTask(TASK_USETANK))
					ALERT(at_console, "<usetank>");
				if (bots[i].IsTask(TASK_BIPOD))
					ALERT(at_console, "<bipod>");				
				if (bots[i].IsTask(TASK_CLAY_IGNORE))
					ALERT(at_console, "<clay ignore>");
				if (bots[i].IsTask(TASK_CLAY_EVADE))
					ALERT(at_console, "<clay evade>");
				if (bots[i].IsTask(TASK_GOPRONE))
					ALERT(at_console, "<goprone>");				
				if (bots[i].IsTask(TASK_AVOID_ENEMY))
					ALERT(at_console, "<avoidenemy>");
				if (bots[i].IsTask(TASK_IGNORE_ENEMY))
					ALERT(at_console, "<ignoreenemy>");


				ALERT(at_console, "\n and as the number %d\n", bots[i].bot_tasks);
			}
		}

		return TRUE;
	}


	//TEMP: need cmd to test subtasks
	else if (FStrEq(pcmd, "tbst"))
	{
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (bots[i].is_used)
			{
				ALERT(at_console, "bot <%s> all his subtasks as strings\n", bots[i].name);

				if (bots[i].IsSubTask(ST_AIM_GETAIM))
					ALERT(at_console, "<aim get aim>");
				if (bots[i].IsSubTask(ST_AIM_FACEIT))
					ALERT(at_console, "<aim face it>");
				if (bots[i].IsSubTask(ST_AIM_SETTIME))
					ALERT(at_console, "<aim set time>");
				if (bots[i].IsSubTask(ST_AIM_ADJUSTAIM))
					ALERT(at_console, "<aim adjust aim>");
				if (bots[i].IsSubTask(ST_AIM_DONE))
					ALERT(at_console, "<aim done>");
				if (bots[i].IsSubTask(ST_FACEITEM_DONE))
					ALERT(at_console, "<face item done>");
				if (bots[i].IsSubTask(ST_BUTTON_USED))
					ALERT(at_console, "<button_used>");
				if (bots[i].IsSubTask(ST_MEDEVAC_ST))
					ALERT(at_console, "<medevac stomach>");
				if (bots[i].IsSubTask(ST_MEDEVAC_H))
					ALERT(at_console, "<medevac head>");
				if (bots[i].IsSubTask(ST_MEDEVAC_F))
					ALERT(at_console, "<medevac feet>");
				if (bots[i].IsSubTask(ST_MEDEVAC_DONE))
					ALERT(at_console, "<medevac done>");
				if (bots[i].IsSubTask(ST_TREAT))
					ALERT(at_console, "<treat>");
				if (bots[i].IsSubTask(ST_HEAL))
					ALERT(at_console, "<heal>");
				if (bots[i].IsSubTask(ST_GIVE))
					ALERT(at_console, "<give>");
				if (bots[i].IsSubTask(ST_HEALED))
					ALERT(at_console, "<healed>");
				if (bots[i].IsSubTask(ST_SAY_CEASEFIRE))
					ALERT(at_console, "<say cease fire>");
				if (bots[i].IsSubTask(ST_FACEENEMY))
					ALERT(at_console, "<face enemy>");
				if (bots[i].IsSubTask(ST_RESETCLAYMORE))
					ALERT(at_console, "<reset claymore>");
				if (bots[i].IsSubTask(ST_DOOR_OPEN))
					ALERT(at_console, "<door open>");
				if (bots[i].IsSubTask(ST_TANK_SHORT))
					ALERT(at_console, "<tank short>");
				if (bots[i].IsSubTask(ST_RANDOMCENTRE))
					ALERT(at_console, "<random centre>");

				ALERT(at_console, "\n and as number %d\n", bots[i].bot_subtasks);
			}
		}

		return TRUE;
	}

	//TEMP: need cmd to test needs
	else if (FStrEq(pcmd, "tbn"))
	{
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (bots[i].is_used)
			{
				ALERT(at_console, "bot <%s> all his needs as strings\n", bots[i].name);

				if (bots[i].IsNeed(NEED_AMMO))
					ALERT(at_console, "<ammo>");
				if (bots[i].IsNeed(NEED_GOAL))
					ALERT(at_console, "<goal>");
				if (bots[i].IsNeed(NEED_GOAL_NOT))
					ALERT(at_console, "<goal not>");
				if (bots[i].IsNeed(NEED_RESETPARACHUTE))
					ALERT(at_console, "<resetparachute>");

				ALERT(at_console, "\n and then as the number %d\n", bots[i].bot_needs);
			}
		}

		return TRUE;
	}

	// TEMP: need cmd to test the next waypoint the bot is going to
	else if (FStrEq(pcmd, "gbnw"))
	{
		if ((arg1 != NULL) && (*arg1 != 0))
		{
			int index = UTIL_FindBotByName(arg1);

			if (index != -1)
			{
				ALERT(at_console, "My next waypoint is %d\n", bots[index].GetNextWaypoint()+1);
				ALERT(at_console, "Zero means no wpt or unable to get it!\n");
			}
		}
		else
		{
			for (int i = 0; i < MAX_CLIENTS; i++)
			{
				if (bots[i].is_used)
				{
					ALERT(at_console, "My next waypoint is %d\n", bots[i].GetNextWaypoint()+1);
					ALERT(at_console, "Zero means no wpt or unable to get it!\n");
				}
			}
		}

		return TRUE;
	}

	// TEMP: need cmd to force bot his next waypoint
	else if (FStrEq(pcmd, "sbnw"))
	{
		if ((arg1 != NULL) && (*arg1 != 0))
		{
			int index = UTIL_FindBotByName(arg1);

			if (index != -1)
			{
				if ((arg2 != NULL) && (*arg2 != 0))
					bots[index].curr_wpt_index = atoi(arg2) - 1;

				ALERT(at_console, "My next waypoint is %d\n", bots[index].GetNextWaypoint()+1);
				ALERT(at_console, "Zero means no wpt or unable to get it!\n");
			}
		}
		else
			ALERT(at_console, "Force bot his next waypoint -> Usage is arg1==botname arg2==the_new_next_wpt_index\n");

		return TRUE;
	}

	//TEMP: need cmd to test patrol wpt
	else if (FStrEq(pcmd, "tbp"))
	{
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (bots[i].is_used)
			{
				ALERT(at_console, "bot <%s> patrol_path_wpt: %d\n",
					bots[i].name, bots[i].patrol_path_wpt);
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
					bots[i].name, bots[i].crowded_wpt_index+1, bots[i].curr_wpt_index+1,
					bots[i].prev_wpt_index.get()+1, bots[i].prev_speed, dist);
			}
		}

		return TRUE;
	}


	//TEMP: need cmd to test point_list
	else if (FStrEq(pcmd, "tpl"))
	{
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (bots[i].is_used)
			{
				for (int index = 0; index < MAX_CLIENTS; index++)
				{
					ALERT(at_console, " Pl[%d] %d ||",index, bots[i].point_list[index] + 1);
				}
			}
		}

		ALERT(at_console, "\n");

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
					ALERT(at_console, "Path #%d has a RED team goal priority\n", path_index+1);
				if (w_paths[path_index]->flags & P_FL_MISC_GOAL_BLUE)
					ALERT(at_console, "Path #%d has a BLUE team goal priority\n", path_index+1);
			}
		}
		else
		{
			for (int path_index = 0; path_index < num_w_paths; path_index++)
			{
				if (w_paths[path_index] == NULL)
					continue;
				if (w_paths[path_index]->flags & P_FL_MISC_GOAL_RED)
					ALERT(at_console, "Path #%d has a RED team goal priority\n", path_index+1);
				if (w_paths[path_index]->flags & P_FL_MISC_GOAL_BLUE)
					ALERT(at_console, "Path #%d has a BLUE team goal priority\n", path_index+1);
			}
		}

		ALERT(at_console, "\n");

		return TRUE;
	}

	//TEMP: needed cmd to test trigger events arrays
	else if (FStrEq(pcmd, "ttea"))
	{
		for (int i = 0; i < MAX_TRIGGERS; i++)
		{
			ALERT(at_console, "Array slot %d | TriggerName %d (TGamestate name %d) | TUsed %d | TMsg %s | Ttriggered %d | TTime %.1f\n",
				i, trigger_events[i].name, trigger_gamestate[i].GetName(),
				trigger_gamestate[i].GetUsed(), trigger_events[i].message,
				trigger_gamestate[i].GetTriggered(), trigger_gamestate[i].GetTime());
		}

		return TRUE;
	}

	//TEMP: needed cmd to test trigger event array
	else if (FStrEq(pcmd, "resettriggerarray"))
	{
		extern void FreeAllTriggers(void);

		FreeAllTriggers();
	}


	//TEMP: need cmd to test name tags
	else if (FStrEq(pcmd, "tnt"))
	{
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (bots[i].is_used)
			{
				if (bots[i].clan_tag & NAMETAG_MEDGUILD)
					sprintf(msg, "bot <%s> HAS MedGuild TAG!", bots[i].name);
				else
					sprintf(msg, "bot <%s> NO TAG", bots[i].name);

				ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

				sprintf(msg, " clantag value: %d", bots[i].clan_tag);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
			}
		}

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

		const char *texture = g_engfuncs.pfnTraceTexture(tr.pHit, vecStart, vecEnd);

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
			x_coord_mod = (float) temp;
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
			x_coord_mod = (float) temp * -1.0;
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
			y_coord_mod = (float) temp;
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
			y_coord_mod = (float) temp * -1.0;
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

		extern float modifier;
		modifier = 1.0;

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
	else if (FStrEq(pcmd, "tcmtest") || FStrEq(pcmd, "tcmt"))
	{
		extern bool test_code;

		if (test_code)
		{
			test_code = false;
			sprintf(msg, "test_code was deactivated\n");
		}
		else
		{
			test_code = true;
			sprintf(msg, "test_code IS NOW active\n");
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
		extern float modifier;

		sprintf(msg, "modifier current value is %.2f\n", modifier);
		ClientPrint(pRecipient, HUD_PRINTNOTIFY, msg);

		return TRUE;
	}
	// increment
	else if (FStrEq(pcmd, "tma"))
	{
		extern float modifier;

		if ((arg1 != NULL) && (*arg1 != 0))
		{
			int temp = atoi(arg1);
			modifier = (float) temp;
			modifier /= 100;
		}
		else
			modifier += 0.05;

		sprintf(msg, "modifier increased - current value is %.2f\n", modifier);
		ClientPrint(pRecipient, HUD_PRINTNOTIFY, msg);

		return TRUE;
	}
	// decrement
	else if (FStrEq(pcmd, "tmd"))
	{
		extern float modifier;

		if ((arg1 != NULL) && (*arg1 != 0))
		{
			int temp = atoi(arg1);
			modifier = (float) temp * -1.0;
			modifier /= 100;
		}
		else
			modifier -= 0.05;

		sprintf(msg, "modifier decreased - current value is %.2f\n", modifier);
		ClientPrint(pRecipient, HUD_PRINTNOTIFY, msg);

		return TRUE;
	}

	// TEMP: remove it
	else if (FStrEq(pcmd, "tusurr"))
	{
		extern bool IsEntityInSphere(const char* entity_name, edict_t *pEdict, float radius);

		if (IsEntityInSphere("bodyque", pEntity, 50))
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "BODYQUE has been found\n");
		else
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "CAN'T find the bodyque entity\n");

		return TRUE;
	}

	// TEMP: remove it
	else if (FStrEq(pcmd, "tuw"))
	{
		if (debug_usr_weapon)
		{
			debug_usr_weapon = FALSE;

			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "debug_usr_weapon DISABLED\n");
		}
		else
		{
			debug_usr_weapon = TRUE;

			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "debug_usr_weapon ENABLED\n");
		}

		return TRUE;
	}

	/*
	//TEMP: need cmd to test getting weapon IDs
	else if (FStrEq(pcmd, "twid"))
	{
		ALERT(at_console, "%s ID is %d\n", arg1, UTIL_GetIDFromName(arg1));
		ALERT(at_console, "weapon_knife ID is %d\n", UTIL_GetIDFromName("weapon_knife"));

		return TRUE;
	}
	*/

	// TEMP: remove it
	else if (FStrEq(pcmd, "tuoh"))
	{
		if (test_origin)
		{
			test_origin = FALSE;
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "higlight origins OFF\n");
		}
		else
		{
			test_origin = TRUE;
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "highlight origins ON\n");
		}

		return TRUE;
	}

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

		sprintf(str, "absmin+absmax -> x %0.2f | y %0.2f | z %0.2f\n",
			pEntity->v.absmin.x+pEntity->v.absmax.x,
			pEntity->v.absmin.y+pEntity->v.absmax.y,
			pEntity->v.absmin.z+pEntity->v.absmax.z);
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
				bots[0].curr_aim_location.x, bots[0].curr_aim_location.y, bots[0].curr_aim_location.z);
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
				bots[0].curr_aim_location.x, bots[0].curr_aim_location.y, bots[0].curr_aim_location.z);
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

			extern float modifier;
			extern float InvModYaw90(float yaw_angle);
			extern float ModYawAngle(float yaw_angle);

			float dist = (pEntity->v.origin - bots[0].pEdict->v.origin).Length();
			sprintf(str, "\ndist %.2f My Yaw on 360 scale (%.2f) current modifier %.2f\n", dist, something, modifier);
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

	  sprintf(str, "flags %d || edict %x || pContainingEntity %x\n",
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

	//TEMP: remove it
	else if (FStrEq(pcmd, "tufall"))
	{
		if (IsDeathFall(pRecipient))
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "There's a death fall depth in front of you\n");
		else
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "It's safe in front of you\n");

		if (deathfall_check)
			deathfall_check = FALSE;
		else
			deathfall_check = TRUE;

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

	else if ((FStrEq(pcmd, "twptfindtype")) || (FStrEq(pcmd, "twpt_findtype")))
	{
		for (int i = 0; i < num_waypoints; i++)
		{
			if (waypoints[i].flags & W_FL_FIRE)
				ALERT(at_console, "There is shoot wpt, its index is %d\n", i+1);
		}

		return TRUE;
	}

	else if (FStrEq(pcmd, "ffs"))
	{
		if (bots[0].is_used == false)
			return TRUE;

		if (bots[0].is_forced)
		{
			bots[0].is_forced = false;
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "forced stance DISABLED\n");
		}
		else
		{
			bots[0].is_forced = true;
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "forced stance ENABLED\n");
		}
		return TRUE;
	}

	else if (FStrEq(pcmd, "fsnow"))
	{
		if (bots[0].is_used)
		{
			bots[0].is_forced = true;
			SetStanceByte(&bots[0], bots[0].forced_stance);
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "enforcing forced stance NOW\n");
		}
		
		return TRUE;
	}

	else if (FStrEq(pcmd, "fss"))
	{
		if (bots[0].is_used)
		{
			bots[0].is_forced = true;
			bots[0].forced_stance = BOT_STANDING;
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "forced stance set to STANDING\n");
		}
		
		return TRUE;
	}
	else if (FStrEq(pcmd, "fsc"))
	{
		if (bots[0].is_used)
		{
			bots[0].is_forced = true;
			bots[0].forced_stance = BOT_CROUCHED;
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "forced stance set to CROUCHED\n");
		}
		
		return TRUE;
	}
	else if (FStrEq(pcmd, "fsp"))
	{
		if (bots[0].is_used)
		{
			bots[0].is_forced = true;
			bots[0].forced_stance = BOT_PRONED;
			ClientPrint(pRecipient, HUD_PRINTNOTIFY, "forced stance set to PRONED\n");
		}
		
		return TRUE;
	}

	else if (FStrEq(pcmd, "dumpuser"))
	{
		UTIL_DumpEdictToFile(pEntity);
		return TRUE;
	}

	else if (FStrEq(pcmd, "dumpbot"))
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
		edict_t *pent = NULL;
		float radius = PLAYER_SEARCH_RADIUS;
		char str[256];

		if ((arg1 != NULL) && (*arg1 != 0))
		{
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "scanning entities in sphere...\n");

			bool noresults = TRUE;
			
			while ((pent = UTIL_FindEntityInSphere( pent, pEntity->v.origin, radius )) != NULL)
			{
				if (strcmp(STRING(pent->v.classname), arg1) == 0)
				{
					sprintf(str, "Found %s (pent %x) has been dumped to file!\n",
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
		FILE *cfp;
		cfp = fopen(debug_fname, "a");

		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			fprintf(cfp, "\n********Dumping the client_t array slot #%d********\n", i);
			fprintf(cfp, "pEntity: %x\n", clients[i].pEntity);
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
		edict_t *pent = NULL;
		float radius = PLAYER_SEARCH_RADIUS;
		char str[80];

		ClientPrint(pEntity, HUD_PRINTCONSOLE, "searching...\n");

		while ((pent = UTIL_FindEntityInSphere( pent, pEntity->v.origin, radius )) != NULL)
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
		edict_t *pent = NULL;
		float radius = PLAYER_SEARCH_RADIUS;
		char str[80];
		Vector entity_o;

		ClientPrint(pEntity, HUD_PRINTCONSOLE, "scanning...\n");

		while ((pent = UTIL_FindEntityInSphere( pent, pEntity->v.origin, radius )) != NULL)
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
		edict_t *pent = NULL;
		float radius = EXTENDED_SEARCH_RADIUS;
		char str[80];
		Vector entity_o;

		ClientPrint(pEntity, HUD_PRINTCONSOLE, "scanning...\n");

		while ((pent = UTIL_FindEntityInSphere( pent, pEntity->v.origin, radius )) != NULL)
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
		edict_t *pent = NULL;
		float radius = PLAYER_SEARCH_RADIUS;
		char str[80];
		Vector entity_o;

		if ((arg1 != NULL) && (*arg1 != 0))
		{
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "scanning the entity...\n");
			 
			while ((pent = UTIL_FindEntityByClassname(pent, arg1)) != NULL)
			{
				entity_o = VecBModelOrigin(pent);
				 
				sprintf(str, "Found %s at %.2f %.2f %.2f (pent %x)\n",
					STRING(pent->v.classname), entity_o.x, entity_o.y, entity_o.z, pent);
				ClientPrint(pEntity, HUD_PRINTCONSOLE, str);

				sprintf(str, "flags %d || deadflag %d || me (edict) %x\n",
					pent->v.flags, pent->v.deadflag, pEntity);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, str);
				
				if (pent->v.model != NULL)
				{
					sprintf(str, "string1 %s\n",STRING(pent->v.model));
					ClientPrint(pEntity, HUD_PRINTNOTIFY, str);
				}


				Vector v_myhead = pEntity->v.origin + pEntity->v.view_ofs;
				TraceResult tr;
				UTIL_TraceLine(v_myhead, entity_o, ignore_monsters, pEntity, &tr);

				sprintf(str, "TraceResult: flfract %.2f | phit name %s\n",
					tr.flFraction, STRING(tr.pHit->v.classname));
				ClientPrint(pEntity, HUD_PRINTNOTIFY, str);
			}
		}

		return TRUE;
	}


	else if (FStrEq(pcmd, "scanwpt"))
	{
		edict_t *pent = NULL;
		float radius = PLAYER_SEARCH_RADIUS;
		char str[80];
		int index;
		Vector entity_o;

		ClientPrint(pEntity, HUD_PRINTCONSOLE, "scanning...\n");

		index = WaypointFindNearest(pEntity, radius, -1);
		if (index != -1)
		{
			extern void WptGetType(int wpt_index, char *the_type);

			char flags[256];
			WptGetType(index, flags);
			entity_o = waypoints[index].origin;

			sprintf(str, "Found %s waypoint at %.2f %.2f %.2f (flags\\tags <%d>)\n",
				flags, entity_o.x, entity_o.y, entity_o.z, waypoints[index].flags);
			ClientPrint(pEntity, HUD_PRINTCONSOLE, str);
		}

		return TRUE;
	}

	else if ((FStrEq(pcmd, "scangrenades")) || (FStrEq(pcmd, "scangr")))
	{
		edict_t *pent = NULL;
		edict_t *pEdict = pEntity;

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
		edict_t *pent = NULL;
		edict_t *pEdict = pEntity;


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
		v_des = v_src + gpGlobals->v_forward * 70;

		UTIL_TraceLine( v_src, v_des, dont_ignore_monsters, pEntity->v.pContainingEntity, &tr);

		ClientPrint(pEntity, HUD_PRINTCONSOLE, "tracing (dont ignore monsters) forward from eyes...\n");

		sprintf(str, "Result=%.2f (%s)\nsrc_x%.2f src_y%.2f src_z%.2f\ndes_x%.2f des_y%.2f des_z%.2f\n",
			tr.flFraction, STRING(tr.pHit->v.classname),v_src.x, v_src.y, v_src.z, v_des.x, v_des.y, v_des.z);
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
		v_des = v_src + gpGlobals->v_forward * 70;

		UTIL_TraceLine( v_src, v_des, dont_ignore_monsters, dont_ignore_glass, pEntity->v.pContainingEntity, &tr);

		ClientPrint(pEntity, HUD_PRINTCONSOLE, "tracing (dont ignore monsters & glass) forward from eyes...\n");

		sprintf(str, "Result=%.2f (%s)\nsrc_x%.2f src_y%.2f src_z%.2f\ndes_x%.2f des_y%.2f des_z%.2f\n",
			tr.flFraction, STRING(tr.pHit->v.classname),v_src.x, v_src.y, v_src.z, v_des.x, v_des.y, v_des.z);
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
		edict_t *pent = NULL;
		float radius = PLAYER_SEARCH_RADIUS;
		char str[512];

		if ((arg1 != NULL) && (*arg1 != 0))
		{
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "scanning the entity...\n");

			bool noresults = TRUE;
			
			while ((pent = UTIL_FindEntityInSphere( pent, pEntity->v.origin, radius )) != NULL)
			{
				if (strcmp(STRING(pent->v.classname), arg1) == 0)
				{
					sprintf(str, "Found %s (pent %x)\ngetting more details about it...\n",
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
		edict_t *pent = NULL;
		float radius = EXTENDED_SEARCH_RADIUS;
		char str[256];

		if ((arg1 != NULL) && (*arg1 != 0))
		{
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "scanning the entity...\n");

			bool noresults = TRUE;
			
			while ((pent = UTIL_FindEntityInSphere( pent, pEntity->v.origin, radius )) != NULL)
			{
				if (strcmp(STRING(pent->v.classname), arg1) == 0)
				{
					Vector entity_origin = VecBModelOrigin(pent);										
					Vector entity = entity_origin - pEntity->v.origin;
					
					sprintf(str, "Found %s (pent %x) | distance %.2f\n",
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


	else if (FStrEq(pcmd, "testname"))
	{
		extern void Test(char *name);
		char name[BOT_NAME_LEN];

		if ((arg1 != NULL) && (*arg1 != 0))
		{
			strcpy(name, arg1);
		}
		else
		{
			strcpy(name, STRING(pEntity->v.netname));
		}

		Test(name);
		ClientPrint(pEntity, HUD_PRINTCONSOLE, name);
		ClientPrint(pEntity, HUD_PRINTCONSOLE, "\n");

		return TRUE;
	}

	// testing waypoint self-controlled repair ability
	else if (FStrEq(pcmd, "testwptrepairrange") || FStrEq(pcmd, "twrr"))
	{
		if ((arg1 != NULL) && (*arg1 != 0))
		{
			int wpt_index = atoi(arg1) - 1;

			UTIL_RepairWaypointRangeandPosition(wpt_index, pEntity);
		}
		else
			UTIL_RepairWaypointRangeandPosition(pEntity);

		return TRUE;
	}

	// testing waypoint self-controlled repair ability
	else if (FStrEq(pcmd, "testwptrepairpathend") || FStrEq(pcmd, "twrpe"))
	{
		if ((arg1 != NULL) && (*arg1 != 0))
		{
			int path_index = atoi(arg1) - 1;

			int result = WaypointRepairInvalidPathEnd(path_index);

			switch (result)
			{
				case -1:
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "Error!\n");
					break;
				case 0:
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "ALL OKAY!\n");
					break;
				case 1:
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "Path was repaired!\n");
					break;
				case 10:
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "Unable to repair this path!\n");
					break;
			}
		}
		else
		{
			WaypointRepairInvalidPathEnd();
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "All path ends were repaired!\n");
		}

		return TRUE;
	}

	// testing waypoint self-controlled repair ability
	else if (FStrEq(pcmd, "testwptrepairpathmerge") || FStrEq(pcmd, "twrpm"))
	{
		if ((arg1 != NULL) && (*arg1 != 0))
		{
			int path_index = atoi(arg1) - 1;

			int result = WaypointRepairInvalidPathMerge(path_index);

			switch (result)
			{
				case -1:
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "Error!\n");
					break;
				case 0:
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "ALL OKAY!\n");
					break;
				case 1:
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "Path was repaired!\n");
					break;
				case 10:
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "Unable to repair this path!\n");
					break;
			}
		}
		else
		{
			WaypointRepairInvalidPathMerge();
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "Any invalid path merge that was found was repaired!\n");
		}

		return TRUE;
	}

	// testing waypoint self-controlled repair ability
	else if (FStrEq(pcmd, "testwptrepairsniperspot") || FStrEq(pcmd, "twrss"))
	{
		if ((arg1 != NULL) && (*arg1 != 0))
		{
			int path_index = atoi(arg1) - 1;

			int result = WaypointRepairSniperSpot(path_index);

			switch (result)
			{
				case -1:
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "Error!\n");
					break;
				case 0:
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "ALL OKAY!\n");
					break;
				case 1:
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "Sniper spot on this path was repaired!\n");
					break;
				case 10:
					ClientPrint(pEntity, HUD_PRINTNOTIFY, "Unable to repair this sniper spot!\n");
					break;
			}
		}
		else
		{
			WaypointRepairSniperSpot();
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "Any wrongly set sniper spot that was found was repaired!\n");
		}

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
					CustHudMessageToAll("Example of MarineBot HudMessages \n by Shrike !!", Vector (0, 250, 0), blue, 1, time); // green flashing
			//
			/////////////////////////////////////////////////
			}
		}
		else
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "no arg specified! -> testhudmsg arg, arg is number from 1-5\n");

		return TRUE;
	}
	else if (FStrEq(pcmd, "debug_engine"))	// print all into debug file
	{
		if ((debug_engine) || (FStrEq(arg1, "off")))
			debug_engine = 0;
		else
			debug_engine = 1;

		if (debug_engine)
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_engine ENABLED!\n");
		else
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_engine DISABLED!\n");

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
	else if (FStrEq(pcmd, "debug_stuck"))
	{
		if ((debug_stuck) || (FStrEq(arg1, "off")))
			debug_stuck = FALSE;
		else
			debug_stuck = TRUE;

		if (debug_stuck)
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_stuck ENABLED!\n");
		else
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "debug_stuck DISABLED!\n");

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
	if (FStrEq(pcmd, "samurai"))
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
