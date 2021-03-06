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
// dll.cpp
// 
////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(WIN32)
#pragma warning(disable: 4005 91 4477)
#endif

#include "defines.h"

// for new config system and map learning by Andrey Kotrekhov
#include "Config.h"
using std::string;

#include "extdll.h"
#include "enginecallback.h"
#include "util.h"
#include "cbase.h"
#include "entity_state.h"

#include "bot.h"
#include "bot_func.h"
#include "bot_manager.h"
#include "client_commands.h"
#include "waypoint.h"


Section *conf_weapons = NULL;

extern "C"
{
#include <stdio.h>
};

const int SERVER_CMD_LEN = 80;

extern GETENTITYAPI other_GetEntityAPI;
extern GETNEWDLLFUNCTIONS other_GetNewDLLFunctions;
extern enginefuncs_t g_engfuncs;
extern int debug_engine;
extern globalvars_t  *gpGlobals;
extern char *g_argv;


extern char wpt_author[32];
extern char wpt_modified[32];


extern botname_t bot_names[MAX_BOT_NAMES];				// array of all names read from external file
bot_weapon_select_t bot_weapon_select[MAX_WEAPONS];		// array of all weapons the bot can use
bot_fire_delay_t bot_fire_delay[MAX_WEAPONS];			// their delays between two shots

static FILE *fp;

DLL_FUNCTIONS other_gFunctionTable;
DLL_GLOBAL const Vector g_vecZero = Vector(0,0,0);

externals_t externals;
internals_t internals;
botmanager_t botmanager;
botdebugger_t botdebugger;

int client_t::humans_num;
int client_t::bots_num;
client_t clients[MAX_CLIENTS];

int m_spriteTexture = 0;
int m_spriteTexturePath1 = 0;
int m_spriteTexturePath2 = 0;
int m_spriteTexturePath3 = 0;

bool is_dedicated_server = FALSE;	// TRUE if the server is a dedicated server

Section* conf = NULL;		// for new config system by Andrey Kotrekhov

//edict_t *pent_info_firearms_detect = NULL;	// NOT USED

edict_t *listenserver_edict = NULL;
edict_t *pRecipient = NULL;		// the one who write the ClintCommand
edict_t *g_debug_bot = NULL;	// pointer on a single bot we want to debug (not all actions are logged, add the code to those that are needed at the moment)
bool g_debug_bot_on = FALSE;	// do we debug a single bot?

#ifdef NOFAMAS
char mb_version_info[32] = "0.94b_noFamas-[APG]";
#else
char mb_version_info[32] = "0.94b-[APG]";		// holds MarineBot version string
#endif

// Marine Bot doesn't use these yet and most probably will never spam the game with these
char bot_whine[MAX_BOT_WHINE][81];
int whine_count;
int recent_bot_whine[5];

// following variables are used only in this file
bool Dedicated_Server_Init = FALSE;		// ensures only one run of DS init
bool g_GameRules = FALSE;
int isFakeClientCommand = 0;
int fake_arg_count;
bool read_whole_cfg = TRUE;				// allows to read the whole configuration file after map change
bool using_default_cfg = TRUE;			// is FALSE when we changed to some map specific .cfg
bool need_to_open_cfg = TRUE;
float bot_cfg_pause_time = 0.0;
float respawn_time = 0.0;
bool spawn_time_reset = FALSE;
int num_bots = 0;
int prev_num_bots = 0;
bool override_max_bots = FALSE;
float f_update_wpt_time;				// allows us to update waypoints in real-time based on latest game events or changes made by the waypointer
const float wpt_autosave_delay = 90.0;	// constant delay between two attempts to automatic waypoints save
float wpt_autosave_time = 0.0;			// holds time of the next attempt to automatic waypoints save

char presentation_msg[] = "This server runs Marine Bot in version";
float check_send_info = 0.0;			// send message checks
float presentation_time = 0.0;			// holds the time of last presentation
bool welcome_sent = FALSE;				// Marine Bot welcome messages (fancy HUD messages on Listen server and simple one sentence text to Dedicated server console)
bool welcome2_sent = FALSE;
bool welcome3_sent = FALSE;				// special that shows waypoints authors
char welcome_msg[] = "reporting for duty!\nWrite \"help\" or \"?\" into console to show console help";
char welcome2_msg[] = "Visit our web page at:\nhttp://www.marinebot.xf.cz";
float welcome_time = 0.0;
bool error_occured = FALSE;				// TRUE when fatal error was found during initialization (missing .cfg file)
bool warning_event = FALSE;				// TRUE when there was some non-fatal error found like missing waypoints
bool override_reset = FALSE;			// to prevent resetting both flags (eg. there is error in DLLInit and we need to print it)
char hud_error_msg[1024];				// the error/warning message that will be printed using Listen Server HUD message system
float hud_error_msg_time = 0.0;			// holds the time the error message is printed


// few function prototypes used in this file
void GameDLLInit(void);
void UpdateClientData(const struct edict_s *ent, int sendweapons, struct clientdata_s *cd);
void ProcessBotCfgFile(Section *conf);		// for new config system by Andrey Kotrekhov
void MBServerCommand(void);		// Dedicated server console commands


void GameDLLInit(void)
{
	int i;

	(*g_engfuncs.pfnAddServerCommand) ("m_bot", MBServerCommand);

	// is dedicated server
	if (IS_DEDICATED_SERVER())
		is_dedicated_server = TRUE;

	for (i = 0; i < MAX_BOT_NAMES; i++)
	{
		bot_names[i].name;
		bot_names[i].is_used = FALSE;
	}

	// whines aren't used at all ... probably useless and will be removed
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
		PrintOutput(NULL, msg, MType::msg_error);
			
		sprintf(msg, "Bots will not shoot\n");
		PrintOutput(NULL, msg, MType::msg_warning);
		
		// show also this error message through the hud once client join game
		if (warning_event == FALSE)
		{
			// prepare error message
			strcpy(hud_error_msg, "WARNING - MarineBot detected an error: weapon definitions!\nThere is a syntax error or the file can't be found\nBots will not shoot");
			
			// to know that something bad happened
			warning_event = TRUE;

			// we need to prevent resetting of this error in DispatchSpawn()
			override_reset = TRUE;
		}
	}

	BotWeaponArraysInit(conf_weapons);

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
			fprintf(fp, "DispatchSpawn: %p %s\n", pent, pClassname);

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

			// set these internal variables back to defaults when the map changed to a new one
			// or the user restarted actual map
			// normally this should have been at the beginning of Start Frame method
			// but Start Frame gets called after Dispatch Spawn
			// which means that in Start Frame we would reset variables (like
			// enemy distance limit for example)
			// that we've already set here in Dispatch Spawn so it has to be here
			internals.ResetOnMapChange();

			// clear signatures first
			strcpy(wpt_author, "unknown");
			strcpy(wpt_modified, "unknown");

			WaypointInit();

			// reset error messaging system only if allowed to do so
			if (override_reset == FALSE)
			{
				// prepare new error message header
				strcpy(hud_error_msg, "WARNING - MarineBot detected an error");

				// reset detected problems
				error_occured = FALSE;
				warning_event = FALSE;
			}

			int result = WaypointLoad(NULL, NULL);

			// if the waypoint file doesn't exist switch to the other directory and check again
			if (result == -10)
			{
				if (internals.IsCustomWaypoints())
					internals.ResetIsCustomWaypoints();
				else
					internals.SetIsCustomWaypoints(true);

				PrintOutput(NULL, "There is invalid or missing waypoint file for this map. ",
					MType::msg_error);
				PrintOutput(NULL, "Checking the other waypoint directory\n", MType::msg_info);

				result = WaypointLoad(NULL, NULL);

				//TODO: Switch back to default wpts directory if there are no wpts in custom
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
					PrintOutput(NULL, "There is invalid or missing waypoint file for this map\n", MType::msg_error);
					PrintOutput(NULL, "Bots may play incorrectly\n", MType::msg_warning);
				}
				else
				{
					// show the error message once client join
					if (warning_event == FALSE)
					{
						strcat(hud_error_msg, ": waypoints!\nThere is invalid or missing waypoint file for this map\nBots may play incorrectly");

						warning_event = TRUE;
					}
				}
			}
			else
				PrintOutput(NULL, "Loading waypoints...\n", MType::msg_info);

			// load waypoint paths
			result = WaypointPathLoad(NULL, NULL);
			
			// if old waypoint paths are detected try convert them automatically
			if (result == -1)
				WaypointPathLoadUnsupported(NULL);
			else if (result == 0)
			{
				if (is_dedicated_server)
				{
					PrintOutput(NULL, "There is invalid or missing path waypoint file for this map\n", MType::msg_error);
					PrintOutput(NULL, "Bots may play incorrectly\n", MType::msg_warning);
				}
				else
				{
					if (warning_event == FALSE)
					{
						strcat(hud_error_msg, ": paths!\nThere is invalid or missing path waypoint file for this map\nBots may play incorrectly");

						warning_event = TRUE;
					}
				}
			}

//			pent_info_firearms_detect = NULL;	// @@@@ TEMP: only temp don't know if we use it!!!


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

			// see if this map is one of maps where the bots can snipe through skybox
			char mapname[64];
			strcpy(mapname, STRING(gpGlobals->mapname));

			if (strcmp(mapname, "ps_island") == 0)
			{
				internals.SetIsEnemyDistanceLimit(true);
				internals.SetEnemyDistanceLimit(3000);
			}

			bot_cfg_pause_time = 0.0;
			respawn_time = 0.0;
			spawn_time_reset = FALSE;

			prev_num_bots = num_bots;
			num_bots = 0;
			override_max_bots = FALSE;

			botmanager.SetBotCheckTime(gpGlobals->time + 30.0);
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
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "DispatchKeyValue: %p %s=%s\n", pentKeyvalue, pkvd->szKeyName, pkvd->szValue); fclose(fp); }
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
		if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "ClientConnect: pent=%p name=%s address=%s\n", pEntity, pszName, pszAddress); fclose(fp); }
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
			botmanager.SetBotCheckTime(gpGlobals->time + 60.0);

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
		if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "ClientDisconnect: %p\n", pEntity); fclose(fp); }
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
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "ClientKill: %p\n", pEntity); fclose(fp); }
#endif

	(*other_gFunctionTable.pfnClientKill)(pEntity);
}

void ClientPutInServer( edict_t *pEntity )
{
#ifdef _DEBUG
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "ClientPutInServer: %p\n", pEntity); fclose(fp); }
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

		fp=fopen(debug_fname,"a"); fprintf(fp,"%s's ClientCommand: %s ",edict_name,pcmd);
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

		fprintf(fp, " (gametime=%.3f)\n", gpGlobals->time);
		fclose(fp);
	}

	// only allow custom commands if deathmatch mode and NOT dedicated server and
	// client sending command is the listen server client...
	if ((gpGlobals->deathmatch) && (!is_dedicated_server) && (pEntity == listenserver_edict))
	{
		if (CustomClientCommands(pEntity, pcmd, arg1, arg2, arg3, arg4, arg5))
			return;
	}

	(*other_gFunctionTable.pfnClientCommand)(pEntity);
}

void ClientUserInfoChanged( edict_t *pEntity, char *infobuffer )
{
#ifdef _DEBUG
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "ClientUserInfoChanged: pEntity=%p infobuffer=%s\n", pEntity, infobuffer); fclose(fp); }
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
		// this statement will be called on 2nd map load or after the user used commands like
		// 'restart' to load your current map again or 'map <mapname>' to change map
		// the 1st map (after 'create a game' in the main menu) doesn't call this statement
		// constructor defaults are used there
		if ((gpGlobals->time + 0.1) < previous_time)
		{
			char filename[256];
			char mapname[64];

			bot_t::harakiri_moment = 0.0;

			// if automatic teams balancing is enabled reset its time at start of new map
			// changed by kota@
			if (botmanager.IsTeamsBalanceNeeded() && is_dedicated_server)
				botmanager.SetTimeOfTeamsBalanceCheck(gpGlobals->time + externals.GetBalanceTime());

			// turn off teams balance override (ie do teams balance checks again if it is enabled)
			botmanager.ResetOverrideTeamsBalance();

			// if info message autosending is enabled reset its time at start of new map
			if ((check_send_info != -1.0) && (is_dedicated_server))
				check_send_info = gpGlobals->time + externals.GetInfoTime();

			// turn off all waypoints show & auto adding commands
			// to prevent engine overloading
			wptser.ResetOnMapChange();

			pRecipient = NULL;

			// reset welcome messages with new map
			welcome_time = 0.0;
			welcome_sent = FALSE;
			welcome2_sent = FALSE;
			welcome3_sent = FALSE;

			// reset error warnings time
			// (the message itself is cleared right after it has been displayed)
			hud_error_msg_time = 0.0;

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
				// so we have to just respawn existing bots
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

			botmanager.SetBotCheckTime(gpGlobals->time + 30.0);
		}

		// listen server
		if (!is_dedicated_server)
		{
			if ((listenserver_edict != NULL) && IsAlive(listenserver_edict))
			{
				// we found some fatal error so set error message time
				if ((error_occured) && (hud_error_msg_time < 1.0))
					hud_error_msg_time = gpGlobals->time + 2.0;

				// or we found only some non-fatal error
				// so set warning time and also delay welcome
				else if ((warning_event) && (hud_error_msg_time < 1.0))
				{
					hud_error_msg_time = gpGlobals->time + 2.0;
					welcome_time = hud_error_msg_time + 16.0;
				}

				// otherwise all went fine so set standard welcome message time
				else if ((error_occured == FALSE) && (warning_event == FALSE) &&
					(welcome_sent == FALSE) && (welcome_time < 1.0))
					welcome_time = gpGlobals->time + 2.0;
			}

			// is it time to print the error message
			if (((error_occured) || (warning_event)) &&
				(hud_error_msg_time > 0.0) && (hud_error_msg_time < gpGlobals->time))
			{
				// send the error message to clients
				Vector color1 = Vector(250, 50, 0);
				Vector color2 = Vector(255, 0, 20);
				CustHudMessageToAll(hud_error_msg, color1, color2, 2, 10);
				
				// reset the error message back to default
				strcpy(hud_error_msg, "WARNING - MarineBot detected an error");

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
				PrintOutput(NULL, "\n", MType::msg_null);
				PrintOutput(NULL, "write [m_bot help] or [m_bot ?] into your console for command help\n", MType::msg_info);

				welcome_sent = TRUE;	// do it only once
			}
		}
		
		// do DS initialization only once
		if ((Dedicated_Server_Init == FALSE) && (is_dedicated_server))
		{
			Dedicated_Server_Init = TRUE;		// run only once

			// init automatic teams balance checks time
			// changed by kota@
			if (botmanager.IsTeamsBalanceNeeded())
				botmanager.SetTimeOfTeamsBalanceCheck(gpGlobals->time + externals.GetBalanceTime());

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
				if (wptser.IsShowWaypoints() && FBitSet(pPlayer->v.flags, FL_CLIENT) &&
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

		// look if team balance is needed and is it time for it
		// NOTE: SHOULD BE CHANGED TO DIFF _TIME NAME BECAUSE NOW DOBALANCE WHILE BOTS RESPAWNING!!!!!!!!
		if ((botmanager.GetTeamsBalanceValue() > 0) && (respawn_time <= gpGlobals->time))
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
				botmanager.SetBotCheckTime(gpGlobals->time + 0.5);		// time to next adding
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
				char filename[256];
				char mapname[64];

				need_to_open_cfg = FALSE;  // only do this once!!!

				// if there is some previous conf then flush it
				if (::conf != NULL)
				{
					delete ::conf;
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
						PrintOutput(NULL, msg, MType::msg_info);

						// to know that we changed to map specific .cfg
						// we will need to read the default "marine.cfg" again once the map
						// changes and there isn't any map specific .cfg for that map
						using_default_cfg = FALSE;
					}
					else
					{
						UTIL_MarineBotFileName(filename, "marine.cfg", NULL);

						sprintf(msg, "Executing %s\n", filename);
						PrintOutput(NULL, msg, MType::msg_info);

						conf = parceConfig(filename);

						using_default_cfg = TRUE;

						if (conf == NULL)
						{
							// print error message directly into DS console
							if (is_dedicated_server)
							{
								PrintOutput(NULL, "MARINE_BOT CRITICAL ERROR - There is a syntax error in marine.cfg, or the file cant be found\n", MType::msg_null);
								PrintOutput(NULL, "Bots may join or play incorrectly\n", MType::msg_warning);
							}
							// print error message once client join the game
							else
							{
								strcat(hud_error_msg, ": error in configuration file!\nThere is a syntax error in marine.cfg, or the file cant be found\nBots may join or play incorrectly");
								
								error_occured = TRUE;
							}
						}
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
							respawn_time = std::min(respawn_time, gpGlobals->time + (float)1.0);

						if (bot_cfg_pause_time >= 1.0)
							bot_cfg_pause_time = std::min(bot_cfg_pause_time, gpGlobals->time + (float)1.0);
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
		if ((is_dedicated_server) && (botmanager.GetBotCheckTime() < gpGlobals->time))
		{			
			botmanager.SetBotCheckTime(gpGlobals->time + 1.5);	// time to next check

			// if there are currently LESS than the maximum number of "players"
			// then add another bot using the default skill level...

			// if automatic teams balancing is ENABLED do team balancing right on join
			// changed by kota@
			if ((clients[0].ClientCount() < externals.GetMaxBots()) &&
				(externals.GetMaxBots() != -1) && botmanager.IsTeamsBalanceNeeded())
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

			// if automatic teams balance is ENABLED then try to kick bot from "stronger" team
			if ((clients[0].ClientCount() > externals.GetMaxBots()) &&
				(externals.GetMaxBots() != -1) && (externals.GetMinBots() != -1) &&
				(clients[0].BotCount() > externals.GetMinBots()) &&
				(override_max_bots == FALSE) && botmanager.IsTeamsBalanceNeeded())
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
		
		// is time to automatically check teams balance AND we are allowed to do teams balancing
		// changed by kota@
		else if ((botmanager.GetTimeOfTeamsBalanceCheck() < gpGlobals->time) && botmanager.IsTeamsBalanceNeeded() &&
			(botmanager.IsOverrideTeamsBalance() == FALSE))
		{
			if (externals.GetBalanceTime() == 0.0)
			{
				botmanager.ResetTimeOfTeamsBalanceCheck();
				botmanager.ResetTeamsBalanceNeeded();		// added by kota@

				PrintOutput(NULL, "auto balance DISABLED!\n", MType::msg_default);
			}
			else
			{
				botmanager.SetTimeOfTeamsBalanceCheck(gpGlobals->time + externals.GetBalanceTime());
				botmanager.SetTeamsBalanceValue(UTIL_BalanceTeams());

				// we don't want to print the message into listen server console
				if (is_dedicated_server)
				{
					PrintOutput(NULL, "\n", MType::msg_null);	// first seperate this message

					if (botmanager.GetTeamsBalanceValue() == -2)
						PrintOutput(NULL, "server is empty!\n", MType::msg_warning);
					else if (botmanager.GetTeamsBalanceValue() == -1)
						PrintOutput(NULL, "there are no bots!\n", MType::msg_warning);
					else if (botmanager.GetTeamsBalanceValue() == 0)
						PrintOutput(NULL, "teams are balanced\n", MType::msg_info);
					else if (botmanager.GetTeamsBalanceValue() > 100)
					{
						sprintf(msg, "balancing in progress... (moving %d bots from RED team to blue)\n",
							botmanager.GetTeamsBalanceValue() - 100);
						PrintOutput(NULL, msg, MType::msg_info);
					}
					else if ((botmanager.GetTeamsBalanceValue() > 0) && (botmanager.GetTeamsBalanceValue() < 100))
					{
						sprintf(msg, "balancing in progress... (moving %d bots from BLUE team to red)\n",
							botmanager.GetTeamsBalanceValue());
						PrintOutput(NULL, msg, MType::msg_info);
					}
					else if (botmanager.GetTeamsBalanceValue() < -2)
						PrintOutput(NULL, "internal error\n", MType::msg_error);
				}
			}
		}
		
		// is time to automatically send information message AND can we do it
		else if ((check_send_info < gpGlobals->time) && (check_send_info > 1.0) &&
			(is_dedicated_server))
		{
			bool more_details = TRUE;
			int clients_info = UTIL_BalanceTeams();

			PrintOutput(NULL, "\n", MType::msg_null);	// seperate this msg

			if (clients_info == -2)
			{
				PrintOutput(NULL, "there are NO players or bots (server is EMPTY)!\n", MType::msg_warning);

				more_details = FALSE;
			}
			else if (clients_info == -1)
			{
				PrintOutput(NULL, "there are NO bots!\n", MType::msg_warning);
			}
			else if (clients_info > 0)
			{
				PrintOutput(NULL, "teams are NOT balanced!\n", MType::msg_warning);
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
					PrintOutput(NULL, msg, MType::msg_null);
				}
				else
				{
					sprintf(msg, "MARINE_BOT INFO - there are %d clients on the server (%d humans and %d bots)\n",
							//total_clients, players, the_bots);
							clients[0].ClientCount(), clients[0].HumanCount(),
							clients[0].BotCount());
					PrintOutput(NULL, msg, MType::msg_null);

					int j, k;
					char client_name[BOT_NAME_LEN];

					PrintOutput(NULL, "--------------MB skill values----------------------\n", MType::msg_null);

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
						PrintOutput(NULL, msg, MType::msg_null);
					}

					PrintOutput(NULL, "---------------------------------------------------\n", MType::msg_null);
				}
			}

			check_send_info = gpGlobals->time + externals.GetInfoTime();
		}

		// do we need to fill listen server and is it time to add next bot
		else if (botmanager.IsListenServerFilling() && (botmanager.GetBotCheckTime() < gpGlobals->time))
		{
			bool bot_added = FALSE;
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
			if (botmanager.GetBotsToBeAdded() > 0)
			{
				yet_to_fill = botmanager.GetBotsToBeAdded();
			}
			
			// first check if there are some free slots on the server
			if (total_clients >= gpGlobals->maxClients)
				botmanager.ResetListenServerFilling();
			// create bot (red, blue, random) based on team member count
			else if (reds < blues)
				bot_added = BotCreate( NULL, "1", NULL, NULL, NULL, NULL );
			else if (reds > blues)
				bot_added = BotCreate( NULL, "2", NULL, NULL, NULL, NULL );
			else
				bot_added = BotCreate( NULL, NULL, NULL, NULL, NULL, NULL );

			if (bot_added)
			{
				botmanager.SetBotCheckTime(gpGlobals->time + 0.5);	// time to next adding

				// decrease the amount of bots to be added (only if is "arg filling")
				if (yet_to_fill != 0)
				{
					botmanager.DecreaseBotsToBeAdded();

					// end filling if there is no bot to be added
					if (botmanager.GetBotsToBeAdded() == 0)
						botmanager.ResetListenServerFilling();
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

		// do save waypoints & paths automatically after set time period
		else if (internals.IsWaypointsAutoSave() && (wpt_autosave_time < gpGlobals->time))
		{
			// we don't want this message on DS
			if (is_dedicated_server == FALSE)
				PrintOutput(NULL, "***autosaving waypoints and paths***\n", MType::msg_null);

			// check if all went fine
			if (WaypointAutoSave())
				wpt_autosave_time = gpGlobals->time + wpt_autosave_delay;
			// otherwise try it again sooner (in other words postpone current try)
			else
			{
				wpt_autosave_time = gpGlobals->time + 30.0;

				if (is_dedicated_server == FALSE)
					PrintOutput(NULL, "***waypoints and paths weren't saved***\n", MType::msg_null);
			}
		}

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
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "PlayerCustomization: %p\n", pEntity); fclose(fp); }
#endif

	(*other_gFunctionTable.pfnPlayerCustomization)(pEntity, pCust);
}

void SpectatorConnect( edict_t *pEntity )
{
#ifdef _DEBUG
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "SpectatorConnect: %p\n", pEntity); fclose(fp); }
#endif

	(*other_gFunctionTable.pfnSpectatorConnect)(pEntity);
}

void SpectatorDisconnect( edict_t *pEntity )
{
#ifdef _DEBUG
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "SpectatorDisconnect: %p\n", pEntity); fclose(fp); }
#endif

	(*other_gFunctionTable.pfnSpectatorDisconnect)(pEntity);
}

void SpectatorThink( edict_t *pEntity )
{
#ifdef _DEBUG
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "SpectatorThink: %p\n", pEntity); fclose(fp); }
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

#ifndef NEWSDKAM
// if you're getting errors here then go to defines.h and change the NEWSDKAM setting
char PM_FindTextureType( char *name )
#else
char PM_FindTextureType( const char* name)
#endif
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
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp, "InconsistentFile: %p filename=%s discon_message=%s\n", player, filename, disconnect_message); fclose(fp); }
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
		PrintOutput(NULL, "*** conf==NULL ProcessBotCfgFile\n", MType::msg_null);
#endif

		exit(1);

		return;
	}

	if (read_whole_cfg)
	{
#ifdef _DEBUG
		//@@@@@@@@@@
		PrintOutput(NULL, "ProcessBotCfgFile() - reading whole .cfg\n", MType::msg_null);
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
			sprintf(msg, "** missing variable '%s'\n", er_val.key.c_str());

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
				PrintOutput(NULL, "MARINE_BOT CFG FILE INIT - logging DISABLED!\n", MType::msg_null);
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
			PrintOutput(NULL, msg, MType::msg_null);

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
			PrintOutput(NULL, msg, MType::msg_null);

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
			PrintOutput(NULL, msg, MType::msg_null);

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
			PrintOutput(NULL, msg, MType::msg_null);

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
			PrintOutput(NULL, msg, MType::msg_null);

		if (temp_auto_balance_time == 0.0)
		{
			externals.SetBalanceTime(0.0);
			botmanager.ResetTimeOfTeamsBalanceCheck();
			botmanager.ResetTeamsBalanceNeeded();	// added by kota@

			sprintf(msg, "MARINE_BOT CFG FILE INIT - auto_balance DISABLED!\n");
	    }
		else if ((temp_auto_balance_time < 30.0) || (temp_auto_balance_time > 3600.0))
		{
			externals.ResetBalanceTime();
			botmanager.SetTeamsBalanceNeeded(true);
			sprintf(msg, "MARINE_BOT CFG FILE ERROR - auto_balance time reset to %.1fs\n",
				externals.GetBalanceTime());
		}
		else
		{
			externals.SetBalanceTime((float) temp_auto_balance_time);
			botmanager.SetTeamsBalanceNeeded(true);		// added by kota@

			sprintf(msg, "MARINE_BOT CFG FILE INIT - auto_balance time set to %.1fs\n",
				externals.GetBalanceTime());
	    }

	    if (is_dedicated_server)
			PrintOutput(NULL, msg, MType::msg_null);

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
			PrintOutput(NULL, msg, MType::msg_null);

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
			PrintOutput(NULL, msg, MType::msg_null);

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
			PrintOutput(NULL, msg, MType::msg_null);

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
			PrintOutput(NULL, msg, MType::msg_null);

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
			PrintOutput(NULL, msg, MType::msg_null);

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
			PrintOutput(NULL, msg, MType::msg_null);

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
			PrintOutput(NULL, msg, MType::msg_null);
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
		PrintOutput(NULL, "ProcessBotCfgFile() - no bots to be auto spawned - skipping recruit section\n", MType::msg_null);
#endif
		// we don't want to run this method all the time
		bot_cfg_pause_time = 0.0;

		return;
	}

	if (is_dedicated_server && (clients[0].ClientCount() >= externals.GetMaxBots()))
	{
#ifdef _DEBUG
		//@@@@@@@@@@
		PrintOutput(NULL, "ProcessBotCfgFile() - number of clients on the server >= max_bots - skipping recruit section\n", MType::msg_info);
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
					sprintf(msg, "** ProcessBotCfgFile() - missing variable '%s'\n", er_val.key.c_str());					
					PrintOutput(NULL, msg, MType::msg_error);
				}
#ifdef _DEBUG
				//@@@@@@@@@@
				sprintf(msg, "** ProcessBotCfgFile() - a try to add bot number %d\n", cur_recruit);
				PrintOutput(NULL, msg, MType::msg_null);

				sprintf(msg, "** ProcessBotCfgFile() - bot params %s %s %s %s %s\n", arg1.c_str(), arg2.c_str(),
					arg3.c_str(), arg4.c_str(), arg5.c_str());
				PrintOutput(NULL, msg, MType::msg_null);
#endif

				BotCreate( NULL, arg1, arg2, arg3, arg4, arg5 );
				
				bot_cfg_pause_time = gpGlobals->time + 2.0;
				botmanager.SetBotCheckTime(gpGlobals->time + 2.5);

				break;
			}
		}
	}
}
// SECTION BY Andrey Kotrekhov END


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
		
		PrintOutput(NULL, "\n", MType::msg_null);
		PrintOutput(NULL, "----------------------------------------------------\n", MType::msg_null);
		PrintOutput(NULL, "Marine Bot dedicated server commands help\n", MType::msg_null);
		PrintOutput(NULL, "----------------------------------------------------\n", MType::msg_null);
		PrintOutput(NULL, "CVAR for MarineBot is [m_bot] so all cmds must start with m_bot\n", MType::msg_null);
		PrintOutput(NULL, "[addmarine]  add bot with random team, class, name and default skill\n", MType::msg_null);
		PrintOutput(NULL, "[addmarine1] add bot to red team with random class, name and default skill\n", MType::msg_null);
		PrintOutput(NULL, "[addmarine2] add bot to blue team with random class, name and default skill\n", MType::msg_null);
		PrintOutput(NULL, "[addmarine <team> <class> <skin> <skill> <name>] add fully customized bot\n", MType::msg_null);
		PrintOutput(NULL, "[min_bots <number>] specify minimum of bots on server (31=disabled - no bot is kicked)\n", MType::msg_null);
		PrintOutput(NULL, "[max_bots <number>] specify maximum of bots on server (0=disabled)\n", MType::msg_null);
		PrintOutput(NULL, "[random_skill <current>] toggle between using default bot skill and generating the skill randomly for each bot, \"current\" returns actual state\n", MType::msg_null);
		PrintOutput(NULL, "[spawn_skill <number>] set default bot skill on join (1-5 where 1=best)\n", MType::msg_null);
		PrintOutput(NULL, "[set_botskill <number>] set bot skill level for all bots already in game\n", MType::msg_null);
		PrintOutput(NULL, "[botskill_up] increase bot skill level for all bots already in game\n", MType::msg_null);
		PrintOutput(NULL, "[botskill_down] decrease bot skill level for all bots already in game\n", MType::msg_null);
		PrintOutput(NULL, "[set_aimskill <number>] set aim skill level for all bots already in game\n", MType::msg_null);
		PrintOutput(NULL, "[reaction_time <number>] set bot reaction time (0-50 where 1 will be converted to 0.1s and 50 to 5.0s)\n", MType::msg_null);
		PrintOutput(NULL, "[range_limit <number>] set the max distance of enemy the bot can see & attack (500-7500 units)\n", MType::msg_null);
		PrintOutput(NULL, "[balance_teams] try to balance teams on server (move bots to weaker team)\n", MType::msg_null);
		PrintOutput(NULL, "[auto_balance <number>] set time for team balance checks ie. autobalancing (30-3600s where 30 is 30seconds and 3600 is 1hour, setting it to 0=never do team balance)\n", MType::msg_null);
		PrintOutput(NULL, "[send_info <number>] set time for info message gets send (30-3600s; 0=off)\n", MType::msg_null);
		PrintOutput(NULL, "[send_presentation <number>] set time for presentation message gets send (0-3600s; 0=off)\n", MType::msg_null);
		PrintOutput(NULL, "[clients] print clients/bots count currently on server\n", MType::msg_null);
		PrintOutput(NULL, "[version_info] print MB version\n", MType::msg_null);
		PrintOutput(NULL, "[load_unsupported] convert & save older waypoint file\n", MType::msg_null);
		PrintOutput(NULL, "[directory <current>] toggle through waypoint directories, \"current\" returns name of current directory\n", MType::msg_null);
		PrintOutput(NULL, "[dont_speak <current>] bot will not use Voice&Radio commands, \"current\" returns actual state\n", MType::msg_null);
		PrintOutput(NULL, "[dont_chat <current>] bot will not use say&say_team commands, \"current\" returns actual state\n", MType::msg_null);
		PrintOutput(NULL, "[passive_healing <current>] bot will not heal teammates automatically, \"current\" returns actual state\n", MType::msg_null);
		PrintOutput(NULL, "------------------------------------------\n", MType::msg_null);
	}
	// spawn random team bot
	else if (strcmp(cmd, "addmarine") == 0)
	{
		// allow more than maxbots bots on the server for the rest of current map
		override_max_bots = TRUE;
		
		BotCreate( NULL, arg1, arg2, arg3, arg4, arg5 );

		// wait for a while before allowing the next addition
		botmanager.SetBotCheckTime(gpGlobals->time + 2.5);
	}
	// spawn red bot
	else if (strcmp(cmd, "addmarine1") == 0)
	{
		override_max_bots = TRUE;
		BotCreate( NULL, "1", NULL, NULL, NULL, NULL );		
		botmanager.SetBotCheckTime(gpGlobals->time + 2.5);
	}
	// spawn blue bot
	else if (strcmp(cmd, "addmarine2") == 0)
	{
		override_max_bots = TRUE;
		BotCreate( NULL, "2", NULL, NULL, NULL, NULL );
		botmanager.SetBotCheckTime(gpGlobals->time + 2.5);
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
		PrintOutput(NULL, msg, MType::msg_null);
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
		
		PrintOutput(NULL, msg, MType::msg_null);
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
	// set the time for teams balance checking
	else if (strcmp(cmd, "auto_balance") == 0)
	{
		if ((arg1 != NULL) && (arg1[0] != 0))
		{
			float temp = atoi(arg1);
			
			if (temp <= 0)
			{
				externals.SetBalanceTime(0.0);
				botmanager.ResetTeamsBalanceNeeded();	// added by kota@
				// needed to properly deactivate automatic teams balancing
				botmanager.ResetTimeOfTeamsBalanceCheck();
				
				sprintf(msg, "MARINE_BOT - auto_balance DISABLED!\n");
			}
			else if ((temp >= 30.0) && (temp <= 3600.0))
			{
				externals.SetBalanceTime((float) temp);
				sprintf(msg, "MARINE_BOT - auto_balance time is %.0fs\n", externals.GetBalanceTime());
				
				botmanager.SetTeamsBalanceNeeded(true);	// added by kota@
				botmanager.SetTimeOfTeamsBalanceCheck(gpGlobals->time + externals.GetBalanceTime());
			}
			else
				sprintf(msg, "MARINE_BOT ERROR - invalid auto_balance time value!\n");
		}

		PrintOutput(NULL, msg, MType::msg_null);
	}
	// do teams balancing manually
	else if (strcmp(cmd, "balance_teams") == 0)
	{
		botmanager.SetTeamsBalanceValue(UTIL_BalanceTeams());
		
		if (botmanager.GetTeamsBalanceValue() == -2)
			sprintf(msg, "MARINE_BOT ERROR - server is empty!\n");
		else if (botmanager.GetTeamsBalanceValue() == -1)
			sprintf(msg, "MARINE_BOT ERROR - there are no bots!\n");
		else if (botmanager.GetTeamsBalanceValue() == 0)
			sprintf(msg, "MARINE_BOT - teams are balanced\n");
		else if (botmanager.GetTeamsBalanceValue() > 100)
			sprintf(msg, "MARINE_BOT - balancing in progress... (moving %d bots from RED team to blue)\n",
				botmanager.GetTeamsBalanceValue() - 100);
		else if ((botmanager.GetTeamsBalanceValue() > 0) && (botmanager.GetTeamsBalanceValue() < 100))
			sprintf(msg, "MARINE_BOT - balancing in progress... (moving %d bots from BLUE team to red)\n",
				botmanager.GetTeamsBalanceValue());
		else if (botmanager.GetTeamsBalanceValue() < -2)
			sprintf(msg, "MARINE_BOT ERROR - internal error\n");
		
		PrintOutput(NULL, msg, MType::msg_null);
	}
	// set the time between two info notifications
	else if (strcmp(cmd, "send_info") == 0)
	{
		if ((arg1 != NULL) && (arg1[0] != 0))
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
		
		PrintOutput(NULL, msg, MType::msg_null);
	}
	// set the time between two presentation messages
	else if (strcmp(cmd, "send_presentation") == 0)
	{
		if ((arg1 != NULL) && (arg1[0] != 0))
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
		
		PrintOutput(NULL, msg, MType::msg_null);
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
			PrintOutput(NULL, "the server is empty\n", MType::msg_warning);
		else
		{
			if ((red_bots == 0) && (blue_bots == 0))
			{
				PrintOutput(NULL, "there are no bots!\n", MType::msg_warning);
				PrintOutput(NULL, "clients analyzed\n", MType::msg_null);
				PrintOutput(NULL, "------------------------------------\n", MType::msg_null);
				sprintf(msg, "total clients on server: %d\n", actual_pl);
				PrintOutput(NULL, msg, MType::msg_null);
				sprintf(msg, "red clients: %d\n", red_clients);
				PrintOutput(NULL, msg, MType::msg_null);
				sprintf(msg, "blue clients: %d\n", blue_clients);
				PrintOutput(NULL, msg, MType::msg_null);
				PrintOutput(NULL, "------------------------------------\n", MType::msg_null);
			}
			else
			{
				PrintOutput(NULL, "clients analyzed\n", MType::msg_default);
				PrintOutput(NULL, "------------------------------------\n", MType::msg_null);
				sprintf(msg, "total clients on server: %d\n", actual_pl);
				PrintOutput(NULL, msg, MType::msg_null);
				sprintf(msg, "red team bots: %d out of %d red clients\n", red_bots, red_clients);
				PrintOutput(NULL, msg, MType::msg_null);
				sprintf(msg, "blue team bots: %d out of %d blue clients\n",
					blue_bots, blue_clients);
				PrintOutput(NULL, msg, MType::msg_null);
				PrintOutput(NULL, "------------------------------------\n", MType::msg_null);
			}
		}
	}
	else if (strcmp(cmd, "version_info") == 0)
	{
		sprintf(msg, "You are using MarineBot version: %s\n", mb_version_info);
		PrintOutput(NULL, msg, MType::msg_default);
	}
	else if (strcmp(cmd, "load_unsupported") == 0)
	{
		PrintOutput(NULL, "starting conversion...\n", MType::msg_default);
		
		if (WaypointLoadUnsupported(NULL))
		{
			if (WaypointPathLoadUnsupported(NULL))
			{
				WaypointSave(NULL);
				
				PrintOutput(NULL, "older waypoints and paths converted and saved\n", MType::msg_info);
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
		PrintOutput(NULL, "invalid or unknown command!\n", MType::msg_error);
		PrintOutput(NULL, "write [m_bot help] or [m_bot ?] into your console for command help\n", MType::msg_info);
	}
}
