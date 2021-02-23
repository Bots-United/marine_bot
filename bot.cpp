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
// bot.cpp
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
#include "bot_weapons.h"

#include <sys/types.h>
#include <sys/stat.h>


#ifndef __linux__
extern HINSTANCE h_Library;
#else
extern void *h_Library;
#endif

extern bool is_dedicated_server;
extern bool g_debug_bot_on;
extern edict_t* g_debug_bot;

//extern int team_class_limits[4];		// this might be useful
extern char bot_whine[MAX_BOT_WHINE][81];
extern int whine_count;

static FILE *fp;

bot_t *bots = nullptr;   // no bots in a game yet

float bot_t::harakiri_moment = 0.0;

extern int debug_engine;
extern int recent_bot_whine[5];

int number_names = 0;

botname_t bot_names[MAX_BOT_NAMES];		// array of bot names read from external file


// few function prototypes used in this file
bool IsEntityInSphere(const char* entity_name, edict_t *pEdict, float radius);
//int BotInFieldOfView(bot_t *pBot, Vector dest);
bool BotEntityIsVisible( bot_t *pBot, Vector dest );
void BotFindItem( bot_t *pBot );
bool IsBleeding(edict_t *pPatient);
bool BotHealTeammate(bot_t *pBot);
bool BotCheckMidAir(edict_t *pEdict);
void BotCheckSkillSystem();
void BotSelectBackupWeapon(bot_t *pBot);
void BotSelectKnife(bot_t *pBot);


inline edict_t *CREATE_FAKE_CLIENT( const char *netname )
{
	return (*g_engfuncs.pfnCreateFakeClient)( netname );
}

inline char *GET_INFOBUFFER( edict_t *e )
{
	return (*g_engfuncs.pfnGetInfoKeyBuffer)( e );
}

inline char *GET_INFO_KEY_VALUE( char *infobuffer, char *key )
{
	return (g_engfuncs.pfnInfoKeyValue( infobuffer, key ));
}

inline void SET_CLIENT_KEY_VALUE( int clientIndex, char *infobuffer,
                                  char *key, char *value )
{
	(*g_engfuncs.pfnSetClientKeyValue)( clientIndex, infobuffer, key, value );
}


// this is the LINK_ENTITY_TO_CLASS function that creates a player (bot)
void player( entvars_t *pev )
{
	static LINK_ENTITY_FUNC otherClassName = nullptr;

	if (otherClassName == nullptr)
		otherClassName = (LINK_ENTITY_FUNC)GetProcAddress(h_Library, "player");

	if (otherClassName != nullptr)
	{
		(*otherClassName)(pev);
	}
}

bot_t::bot_t()
{
		is_used			= false;
		respawn_state	=0;
		pEdict			= nullptr;
		need_to_initialize = false;
		name[0]			= '\0';
		clan_tag		= 0;
		not_started		= 0;
		start_action	= 0;
		kick_time		= 0.0;
		create_time		= 0.0;
		bot_skill		= 0;
		aim_skill		= 0;
		bot_team		= NO_VAL;
		bot_class		= NO_VAL;
		pcust_class		= nullptr;
		bot_skin		= NO_VAL;
		bot_behaviour	= 0;
		idle_angle		= 0.0;
		round_end		= false;

		move_speed		= SPEED_NO;	// probably move it to spawninit()
		bot_spawn_time	= 0.0;
//		killer_edict	= NULL;
		weapon_status	= 0;
		claymore_slot	= NO_VAL;
		bot_fa_skill	= 0;
		bot_bandages	= 0;		// not sure about this it should probably be in spawninit()
		main_weapon		= NO_VAL;
		backup_weapon	= NO_VAL;
		forced_usage	= 0;
		grenade_slot	= NO_VAL;
		main_no_ammo	= true;
		backup_no_ammo	= true;

		take_main_mags	= 0;		// both most probably to spawninit() as well
		take_backup_mags= 0;
		
		BotSpawnInit();
		prev_wpt_index.print();
}

/*
* sets nearly all bot variables to initial/default value
* needed after the bot has been killed or when the bot is joining the game
*/
void bot_t::BotSpawnInit()
{
#ifdef _DEBUG
	HudNotify("Bot Spawn Init\n", this);



	//																				NEW CODE 094 (remove it)
	if (strlen(name) > 1)
	{
		if (g_debug_bot_on && (g_debug_bot == pEdict) || !g_debug_bot_on)
		{
			char dm[256];
			sprintf(dm, "%s is respawning\n", name);
			UTIL_DebugInFile(dm);
		}
	}
	//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#endif
	
	bot_tasks = 0;
	bot_subtasks = 0;
	bot_needs = 0;

	prev_time = gpGlobals->time;
	v_prev_origin = Vector(9999.0, 9999.0, 9999.0);
	f_dontmove_time = gpGlobals->time;

	pItem = nullptr;
	f_face_item_time = 0.0;

	wpt_origin = g_vecZero;//Vector(0, 0, 0);
	prev_wpt_origin = g_vecZero;//Vector(0, 0, 0);
	f_waypoint_time = gpGlobals->time;
	curr_wpt_index = -1;
	prev_wpt_index.clear();
	crowded_wpt_index = -1;

	//pBot->waypoint_goal = -1;
	//pBot->f_waypoint_goal_time = 0.0;
	//pBot->waypoint_near_flag = FALSE;
	//pBot->waypoint_flag_origin = Vector(0, 0, 0);
	prev_wpt_distance = 9999.0;
	wpt_wait_time = 0.0;	// must be 0.0 otherwise bot use it right after respawn
	wpt_action_time = 0.0;	// must be 0.0 otherwise bot use it right after respawn
	f_face_wpt = 0.0;

	clear_path();
	curr_path_index = -1;
	prev_path_index = -1;
	opposite_path_dir = FALSE;
	patrol_path_wpt = -1;

	msecnum = 0;
	msecdel = 0.0;
	msecval = 0.0;

	bot_health = 0;
	bot_armor = 0;
	bot_weapons = 0;
	blinded_time = 0.0;

	f_max_speed = CVAR_GET_FLOAT("sv_maxspeed");

	prev_speed = 0.0;  // fake "paused" since bot is NOT stuck

	f_strafe_direction = 0;
	f_strafe_time = 0.0;
	hide_time = -10.0;	// must be - bot test if (ht+5 < globtime)

	ladder_dir = LADDER_UNKNOWN;
	f_start_ladder_time = 0.0;
	waypoint_top_of_ladder = FALSE;
	end_wpt_index = -1;

	f_wall_check_time = 0.0;
	f_wall_on_right = 0.0;
	f_wall_on_left = 0.0;
	f_dont_avoid_wall_time = 0.0;
	f_dont_look_for_waypoint_time = 0.0;
	//pBot->f_jump_time = 0.0;
	f_duckjump_time = 0.0;
	SetDontCheckStuck();	// prevents false running of unstuck when spawning
	f_stuck_time = 0.0;
	changed_direction = 0;
	f_check_deathfall = 0.0;

	// pick a wander direction (50% of the time to the left, 50% to the right)
	if (RANDOM_LONG(1, 100) <= 50)
		wander_dir = SIDE_LEFT;
	else
		wander_dir = SIDE_RIGHT;

	pBotEnemy = nullptr;
	f_bot_see_enemy_time = 0.0;	// set it to zero to prevent "area clear" after spawn (at least for now)
	f_bot_find_enemy_time = gpGlobals->time;
	pBotPrevEnemy = nullptr;
	f_bot_wait_for_enemy_time = gpGlobals->time;
	v_last_enemy_position = g_vecZero;//Vector(0, 0, 0);
	f_prev_enemy_dist = 0.0;
	f_reaction_time = 0.0;

	pBotUser = nullptr;
	f_bot_use_time = 0.0;
//	b_bot_say_killed = FALSE;
//	f_bot_say_killed = 0.0;

	aim_index[0] = -1;
	aim_index[1] = -1;
	aim_index[2] = -1;
	aim_index[3] = -1;
	curr_aim_index = -1;
	f_aim_at_target_time = 0.0;
	f_check_aim_time = 0.0;
	targeting_stop = 0;

	weapon_action = W_READY;
	// weapon status is initialized just once at bot creation so we must clear these manually
	RemoveWeaponStatus(WS_PRESSRELOAD);
	RemoveWeaponStatus(WS_INVALID);
	RemoveWeaponStatus(WS_NOTEMPTYMAG);
	RemoveWeaponStatus(WS_MERGEMAGS1);
	RemoveWeaponStatus(WS_MERGEMAGS2);
	RemoveWeaponStatus(WS_CANTBIPOD);
	f_shoot_time = gpGlobals->time;
	f_fullauto_time = -1.0;
	f_reload_time = 0.0;

	ClearPauseTime();// f_pause_time = 0.0;
	f_sound_update_time = 0.0;
	f_look_for_ground_items = 0.0;
	v_ground_item = g_vecZero;//Vector(0,0,0);

	f_use_clay_time = 0.0;
	clay_action = ALTW_NOTUSED;

	//pBot->b_use_button = FALSE;
	//pBot->f_use_button_time = 0;
	//pBot->b_lift_moving = FALSE;

	//pBot->b_use_capture = FALSE;
	//pBot->f_use_capture_time = 0.0;
	//pBot->pCaptureEdict = NULL;

	v_door = g_vecZero;//Vector(0, 0, 0);

	f_bipod_time = 0.0;
	chute_action_time = 0.0;

	bot_prev_health = 100;
	bandage_time = 0.0;
	f_medic_treat_time = 0.0;

	f_cant_prone = 0.0;
	f_go_prone_time = 0.0;

	search_closest_time = 0.0;
	speak_time = 0.0;
	text_msg_time = 0.0;
	prev_msg = 0;

	grenade_time = 0.0;
	grenade_action = ALTW_NOTUSED;
	secondary_active = FALSE;
	sniping_time = 0.0;

	SetTask(TASK_CHECKAMMO);		// set this task so the bot has to check it right after spawn
	check_ammunition_time = gpGlobals->time + 2.0;	// don't check it during spawning

	f_combat_advance_time = gpGlobals->time;
	f_overide_advance_time = gpGlobals->time;
	f_check_stance_time = gpGlobals->time;
	f_stance_changed_time = 0.0;

	memset(&(current_weapon), 0, sizeof(current_weapon));
	memset(&(curr_rgAmmo), 0, sizeof(curr_rgAmmo));// array of current amount of mags

	harakiri = false;

#ifdef _DEBUG
	curr_aim_offset = g_vecZero;//Vector(0, 0, 0);
	target_aim_offset = g_vecZero;//Vector(0, 0, 0);

	is_forced = false;
	forced_stance = BOT_STANDING;
#endif
}


/*
* reads names from external file and puts them to global array of bot names
*/
void BotNameInit()
{
	char bot_name_filename[256];
	char name_buffer[80];

	UTIL_MarineBotFileName(bot_name_filename, "marine_dog-tags.txt", nullptr);

	FILE* bot_name_fp = fopen(bot_name_filename, "r");

	if (bot_name_fp != nullptr)
	{
		while ((number_names < MAX_BOT_NAMES) && (fgets(name_buffer, 80, bot_name_fp) != nullptr))
		{
			int length = strlen(name_buffer);

			// handle commented lines in the file (skip them)
			if (name_buffer[0] == '#')
				continue;

			if ((length > 0) && (name_buffer[length-1] == '\n'))
			{
				name_buffer[length-1] = 0;  // remove '\n'
				length--;
			}

			int str_index = 0;
			while (str_index < length)
			{
				if ((name_buffer[str_index] < ' ') || (name_buffer[str_index] > '~') ||
					(name_buffer[str_index] == '"'))
				{
					for (int index = str_index; index < length; index++)
						name_buffer[index] = name_buffer[index+1];
				}

				str_index++;
			}

			if (name_buffer[0] != 0)
			{
				// name can only be long up to 28 chars due to the '[MB]' sign
				strncpy(bot_names[number_names].name, name_buffer, BOT_NAME_LEN - 4);

				number_names++;
			}
		}

		fclose(bot_name_fp);
	}
}

/*
* pick any free (not used) name from global array of bot names
* adds Marine Bot sign/tag if needed (allowed by .cfg variable)
*/
void BotPickName( char *name_buffer )
{
	int attempts = 0;

	int name_index = RANDOM_LONG(1, number_names) - 1;  // zero based

	// check make sure this name isn't used
	bool used = TRUE;

	while (used)
	{
		// is there another bot using this name?
		if (bot_names[name_index].is_used)
		{
			name_index++;

			if (name_index == number_names)
				name_index = 0;

			attempts++;
		}
		// otherwise this name should be free, but we need to test all existing clients
		else
		{
			// check all existing clients for this name
			for (int index = 1; index <= gpGlobals->maxClients; index++)
			{
				edict_t *pPlayer = INDEXENT(index);
				
				if (pPlayer && !pPlayer->free)
				{
					// check if the name is part of this client's name
					if (strstr(STRING(pPlayer->v.netname), bot_names[name_index].name) != nullptr)
					{
						// we found that another bot uses this name so set correct flag
						bot_names[name_index].is_used = TRUE;
					}	
				}
			}

			// this name must really be free so use it
			if (bot_names[name_index].is_used == FALSE)
				used = FALSE;
		}

		// break out of loop even if already used
		if (attempts == number_names)
			used = FALSE;
	}

	// if needed insert the '[MB]' tag right before the name
	if (externals.GetRichNames())
	{
		// insert Marine Bot tag before the name
		strcpy(name_buffer, "[MB]");

		// and add the standard name behing the tag
		strcat(name_buffer, bot_names[name_index].name);

		// this name is beeing used from now
		bot_names[name_index].is_used = TRUE;
	}
	// otherwise use only the name
	else
	{
		strcpy(name_buffer, bot_names[name_index].name);

		bot_names[name_index].is_used = TRUE;
	}
}

// for new config system
bool BotCreate( edict_t *pPlayer, const std::string &s_arg1, const std::string &s_arg2,
		const std::string &s_arg3, const std::string &s_arg4, const std::string &s_arg5)
{
	return BotCreate( pPlayer, s_arg1.c_str(), s_arg2.c_str(),
				s_arg3.c_str(), s_arg4.c_str(), s_arg5.c_str());
}

/*
* puts bot in the game
* sets all values if corresponding arguments are valid otherwise are left untouched
* for further proccessing (methods in bot_start.cpp generates them)
* arg1 is team
* arg2 is class
* arg3 is skin
* arg4 is skill level
* arg5 is name
*/
bool BotCreate( edict_t *pPlayer, const char *arg1, const char *arg2,
                const char *arg3, const char *arg4, const char *arg5)
{
	char c_name[BOT_NAME_LEN+1];
	bool found = FALSE;

	// general initialization
	c_name[0] = 0;
	int skill = 1;

	if (mod_id == FIREARMS_DLL)
	{
		skill = 0;

		if ((arg4 != nullptr) && (*arg4 != 0))
			skill = atoi(arg4);

		if ((skill < 1) || (skill > 5))
		{
			// if there is a random skill request then generate the skill number
			if (externals.GetRandomSkill())
				skill = RANDOM_LONG(1, 5);
			// otherwise use default skill
			else
				skill = externals.GetSpawnSkill();
		}

		if ((arg5 != nullptr) && (*arg5 != 0))
		{
			strncpy( c_name, arg5, BOT_NAME_LEN-1 );
			c_name[BOT_NAME_LEN] = 0;  // make sure c_name is null terminated
		}
		else
		{
			if (number_names > 0)
				BotPickName( c_name );
			else
				strcpy(c_name, "[MB]marine");
		}
	}

	int length = strlen(c_name);

	// remove any illegal characters from name
	for (int i = 0; i < length; i++)
	{
		if ((c_name[i] < ' ') || (c_name[i] > '~') ||
			(c_name[i] == '"'))
		{
			for (int j = i; j < length; j++)  // shuffle chars left (and null)
				c_name[j] = c_name[j+1];

			length--;
		}         
	}

	edict_t* BotEnt = CREATE_FAKE_CLIENT(c_name);

	if (FNullEnt( BotEnt ))
	{
		if (pPlayer)
		{
			ClientPrint( pPlayer, HUD_PRINTNOTIFY, "Max. Players reached.  Can't create bot!\n");

			return FALSE;
		}
	}
	else
	{
		char ptr[128];  // allocate space for message from ClientConnect

		PrintOutput(pPlayer, "Creating MarineBot...\n", MType::msg_null);

		int index = 0;
		while ((bots[index].is_used) && (index < MAX_CLIENTS))
			index++;

		if (index == MAX_CLIENTS)
		{
			PrintOutput(pPlayer, "Can't create MarineBot server is full!\n", MType::msg_null);

			return FALSE;
		}

		// create the player entity by calling MOD's player function
		// (from LINK_ENTITY_TO_CLASS for player object)

		// kick & rejoin bug - a fix by Pierre-Marie Baty
		FREE_PRIVATE (BotEnt);
		BotEnt->pvPrivateData = nullptr;
		BotEnt->v.frags = 0;
		// a fix by Pierre-Marie Baty end

		player( VARS(BotEnt) );

		char* infobuffer = GET_INFOBUFFER(BotEnt);
		const int clientIndex = ENTINDEX(BotEnt);

		SET_CLIENT_KEY_VALUE( clientIndex, infobuffer, "model", "gina" );

		ClientConnect( BotEnt, c_name, "127.0.0.1", ptr );

		BotEnt->v.flags |= FL_FAKECLIENT;

		// Pieter van Dijk - use instead of DispatchSpawn() - Hip Hip Hurray!
		ClientPutInServer( BotEnt );

		// original position, but I've moved it above the client put in server because we need
		// to know if it is bot or not right in that method
		//BotEnt->v.flags |= FL_FAKECLIENT;

		// initialize all the variables for this bot

		bot_t* pBot = &bots[index];

		pBot->is_used = TRUE;
		pBot->respawn_state = RESPAWN_IDLE;
		pBot->create_time = gpGlobals->time;
		pBot->name[0] = 0;  // name not set by server yet
		pBot->clan_tag = 0;	// no clan tag yet

		pBot->pEdict = BotEnt;

		pBot->not_started = 1;  // hasn't joined game yet

		if (mod_id == FIREARMS_DLL)
			pBot->start_action = MSG_FA_IDLE;
		else
			pBot->start_action = 0;  // not needed for non-team MODs

		pBot->BotSpawnInit();

		pBot->need_to_initialize = FALSE;  // don't need to initialize yet

		BotEnt->v.idealpitch = BotEnt->v.v_angle.x;
		BotEnt->v.ideal_yaw = BotEnt->v.v_angle.y;
		BotEnt->v.pitch_speed = 20;
		BotEnt->v.yaw_speed = 20;
		pBot->idle_angle = 0.0;

		pBot->bot_skill = skill - 1;	// 0 based for array indexes
		pBot->aim_skill = skill - 1;	// use the same value we used for bot_skill

		pBot->bot_team = -1;
		pBot->bot_class = -1;
		pBot->bot_skin = -1;
		pBot->main_weapon = NO_VAL;
		pBot->backup_weapon = NO_VAL;
		pBot->grenade_slot = NO_VAL;
		pBot->claymore_slot = NO_VAL;
		pBot->weapon_status = 0;
		pBot->bot_behaviour = 0;
		pBot->bot_fa_skill = 0;
		pBot->round_end = FALSE;

		pBot->pcust_class = nullptr;
		for (int ii=0; ii<ROUTE_LENGTH; ++ii) {	// NOTE: This is code by kota@ - I'm not going to use it, because it looks like he simply recreated the same thing that botman used for paths. Will remove it one day.
			pBot->point_list[ii] = -1;			// Because I don't like that system in FA. For deathmatch it's great, but FA is different
		}

		if (mod_id == FIREARMS_DLL)
		{
			if ((arg1 != nullptr) && (arg1[0] != 0))
			{
				pBot->bot_team = atoi(arg1);

				if ((arg2 != nullptr) && (arg2[0] != 0))
				{
					pBot->bot_class = atoi(arg2);

					if ((arg3 != nullptr) && (*arg3 != 0))
					{
						pBot->bot_skin = atoi(arg3);
					}
				}
			}
		}
	}

	return TRUE;
}


/*
* returns true is there's the entity in given range we've search for
*/
bool IsEntityInSphere(const char* entity_name, edict_t *pEdict, float radius)
{
	edict_t *pent = nullptr;

	while ((pent = UTIL_FindEntityInSphere(pent, pEdict->v.origin, radius )) != nullptr)
	{
		char item_name[64];
		strcpy(item_name, STRING(pent->v.classname));

		if (strcmp(entity_name, item_name) == 0)
			return TRUE;
	}

	return FALSE;
}


/*
* returns true if the target object is fully visible
*/
bool BotEntityIsVisible( bot_t *pBot, Vector dest )
{
	TraceResult tr;
	
	// trace a line from bot's eyes to destination...
	UTIL_TraceLine( pBot->pEdict->v.origin + pBot->pEdict->v.view_ofs,
		dest, ignore_monsters, pBot->pEdict, &tr );
	
	// check if line of sight to object is not blocked (i.e. visible)
	if (tr.flFraction >= 1.0)
		return TRUE;
	else
		return FALSE;
}


/*
* scans the surrounding for certain items/objects
*/
void BotFindItem( bot_t *pBot )
{
	edict_t *pent = nullptr;
	char item_name[40];
	Vector vecStart, vecEnd;
	int angle_to_entity;
	edict_t *pEdict = pBot->pEdict;
	
	while ((pent = UTIL_FindEntityInSphere( pent, pEdict->v.origin, FIND_ITEM_RADIUS )) != nullptr)
	{
		strcpy(item_name, STRING(pent->v.classname));

		if (strncmp("item_", item_name, 5) == 0)
		{
			// look for placed claymores
			if (strcmp("item_claymore", item_name) == 0)
			{
				// bot already knows about this claymore
				// must be first otherwise we will "spot" the same mine more then once
				if (pBot->v_ground_item == pent->v.origin)
					continue;

				// we shouldn't avoid the mine we've just set - its not active yet
				if (pBot->f_use_clay_time > gpGlobals->time)
					continue;

				vecStart = pEdict->v.origin + pEdict->v.view_ofs;
				vecEnd = pent->v.origin;
				
				angle_to_entity = InFieldOfView(pEdict, vecEnd - vecStart);
				
				if (angle_to_entity > 45)
					continue;
				
				// can the bot see this object
				if (BotEntityIsVisible(pBot, vecEnd))
				{
					pBot->v_ground_item = vecEnd;

					// the best bots don't fail to spot it much, but worse bots
					// have higher chance not to see this mine
					const int chance_to_miss = 95 - (pBot->bot_skill * 5);

					// like if the bot didn't see it
					if (RANDOM_LONG(1, 100) > chance_to_miss)
					{
						pBot->SetTask(TASK_CLAY_IGNORE);

						if (botdebugger.IsDebugActions())
							HudNotify("***Ignoring this claymore -> like if didn't see it\n", pBot);
					}
					// bot spotted this claymore
					else
					{
						// let others know about it
						pBot->BotSpeak(SAY_CLAYMORE_FOUND);

						// set the evade flag
						pBot->SetTask(TASK_CLAY_EVADE);

						if (botdebugger.IsDebugActions())
						{
							HudNotify("***Found this claymore -> evading\n", pBot);


#ifdef _DEBUG
							//@@@@@@@@@@@@@
							//ALERT(at_console, "***Found this claymore -> evading gTime=%0.3f\n", gpGlobals->time);
#endif


						}

						// we don't need to look for other objects
						break;
					}
				}
			}
			// look for grenades thrown at bot
			else if ((strcmp("item_frag", item_name) == 0) ||
				(strcmp("item_concussion", item_name) == 0) ||
				(strcmp("item_stg24", item_name) == 0))
			{
				// this grenade is not thrown (ie. still in players inventory)
				if ((pent->v.solid == SOLID_NOT) && (pent->v.movetype == MOVETYPE_FOLLOW))
					continue;

				const int grenade_team = UTIL_GetTeam(pent);
				const int bot_team = UTIL_GetTeam(pEdict);
				
				// ignore grenades thrown by your teammate or unknown grenades
				// even if it might save lifes in certain cases (ie. TKs)
				if ((grenade_team == bot_team) || (grenade_team == TEAM_NONE))
					continue;
				
				vecStart = pEdict->v.origin + pEdict->v.view_ofs;
				vecEnd = pent->v.origin;
				
				angle_to_entity = InFieldOfView(pEdict, vecEnd - vecStart);
				
				if (angle_to_entity > 45)
					continue;
				
				if (BotEntityIsVisible(pBot, vecEnd))
				{
					pBot->BotSpeak(SAY_GRENADE_IN);

					break;
				}
			}
		}
		// handle thrown grenades in older versions or grenades fired from GLs
		else if (UTIL_IsOldVersion() || (strcmp("weapon_m79", item_name) == 0) ||
				(strcmp("weapon_m203", item_name) == 0) ||
				(strcmp("weapon_gp25", item_name) == 0))
		{
			if ((strcmp("weapon_frag", item_name) == 0) ||
				(strcmp("weapon_flashbang", item_name) == 0) ||
				(strcmp("weapon_stg24", item_name) == 0) ||
				(strcmp("grenade", item_name) == 0) ||
				(strcmp("weapon_m203", item_name) == 0) ||
				(strcmp("weapon_m79", item_name) == 0) ||
				(strcmp("weapon_gp25", item_name) == 0))
			{
				if ((pent->v.solid == SOLID_NOT) && (pent->v.movetype == MOVETYPE_FOLLOW))
					continue;

				const int grenade_team = UTIL_GetTeam(pent);
				const int bot_team = UTIL_GetTeam(pEdict);
				
				if ((grenade_team == bot_team) || (grenade_team == TEAM_NONE))
					continue;
				
				vecStart = pEdict->v.origin + pEdict->v.view_ofs;
				vecEnd = pent->v.origin;
				
				angle_to_entity = InFieldOfView(pEdict, vecEnd - vecStart);
				
				if (angle_to_entity > 45)
					continue;
				
				if (BotEntityIsVisible(pBot, vecEnd))
				{
					pBot->BotSpeak(SAY_GRENADE_IN);
					break;
				}
			}
		}
	} // end the while
}


/*
* picks the best message for current situation passed by message type argument
*/
void bot_t::BotSpeak(int msg_type, const char *target)
{
	// use say command only if allowed to do so
	if (externals.GetDontChat())
		return;

	// we don't want to spam the game
	if ((text_msg_time + 1.0 > gpGlobals->time) || (speak_time + 1.0 > gpGlobals->time))
		return;

	// if the bot said exactly this message in last 3 seconds then don't say it again
	if ((prev_msg == msg_type) && (text_msg_time + 3.0 > gpGlobals->time))
		return;

	// store the time the bot said something
	text_msg_time = gpGlobals->time;

	const int choice = RANDOM_LONG(1, 100);
	char msg[128];
	char name[BOT_NAME_LEN];
	name[0] = '\0';

	switch (msg_type)
	{
		case SAY_GRENADE_IN:
			if (choice < 50)
				UTIL_TeamSay(pEdict, "GRENADE!");
			else
				UTIL_TeamSay(pEdict, "GRENADE! Take cover");
			break;
		case SAY_GRENADE_OUT:
			if (choice < 50)
				UTIL_TeamSay(pEdict, "Fire in the hole!");
			else
				UTIL_TeamSay(pEdict, "Frag out!");
			break;
		case SAY_CLAYMORE_FOUND:
			if (choice < 50)
				UTIL_TeamSay(pEdict, "Claymore spotted! Watch your steps");
			else
				UTIL_TeamSay(pEdict, "There's a mine at my position. Watch out!");
			// to prevent spamming the game
			text_msg_time = gpGlobals->time + 1.0;
			break;
		case SAY_MEDIC_HELP_YOU:
			UTIL_HumanizeTheName(target, name);
			sprintf(msg, "Hey, %s, stay still and I'll treat you", name);
			UTIL_TeamSay(pEdict, msg);
			text_msg_time = gpGlobals->time + 1.0;
			break;
		case SAY_MEDIC_CANT_HELP:
			UTIL_HumanizeTheName(target, name);
			if (choice < 50)
				sprintf(msg, "Sorry, %s, I can't help you", name);
			else
				sprintf(msg, "I can't help you %s", name);
			UTIL_TeamSay(pEdict, msg);
			text_msg_time = gpGlobals->time + 1.0;
			break;
		case SAY_CEASE_FIRE:
			UTIL_HumanizeTheName(target, name);
			if (choice < 50)
				sprintf(msg, "Cease fire, %s", name);
			else
				sprintf(msg, "Hey, %s, cease fire!", name);
			UTIL_TeamSay(pEdict, msg);
			text_msg_time = gpGlobals->time + 1.0;
			break;
		default:
			break;
	}

	// remember the message that has to be said, to prevent spamming the game
	prev_msg = msg_type;
}


///////////////////////////////////////////////////////////////////////////////
//
// NOTE: Marine Bot and MedicGuild are partners so if you work on your own bot
// and your bot and MedicGiuld aren't partners remove the part of the code
// marked by >MedicGuild< and all connection to it!
//
///////////////////////////////////////////////////////////////////////////////
// >MedicGuild START<
/*
* checks for medic skills and if bot uses one sets a MedicGuild tag behind bot's name
* or it removes the tag if bot has no medic skill
*/
void bot_t::CheckMedicGuildTag()
{	
	// set MedicGuild name tag if bot uses one of medic skills
	if ((bot_fa_skill & (FAID | MEDIC)) && !(clan_tag & NAMETAG_MEDGUILD))
	{
		char change_name[32];

		clan_tag |= NAMETAG_MEDGUILD;

		// if the MedicGuild sign isn't set yet change bot's name
		if (strstr(name, "}+{") == nullptr)
		{
			strcpy(change_name, name);
			strcat(change_name, "}+{");

			player(VARS(pEdict));
			char* infobuffer = GET_INFOBUFFER(pEdict);
			const int clientIndex = ENTINDEX(pEdict);

			SET_CLIENT_KEY_VALUE(clientIndex, infobuffer, "name", change_name);

			// update bot's name with this new name
			strcpy(name, change_name);
		}
	}

	// remove the tag if bot isn't medic
	if (!(clan_tag & NAMETAG_DONE) && !(bot_fa_skill & (FAID | MEDIC)))
	{
		char change_name[32];

		// if there's MedicGuild tag remove it
		if (clan_tag & NAMETAG_MEDGUILD)
			clan_tag &= ~NAMETAG_MEDGUILD;

		strcpy(change_name, name);
		const int length = strlen(change_name);

		// check if the "}+{" sign is in bot's name
		if (strstr(name, "}+{") != nullptr)
		{
			strcpy(change_name, name);
			// remove the "}+{" sign from bot's name
			// we know that it's 3 char long
			change_name[length - 3] = '\0';

			player(VARS(pEdict));
			char* infobuffer = GET_INFOBUFFER(pEdict);
			const int clientIndex = ENTINDEX(pEdict);

			SET_CLIENT_KEY_VALUE(clientIndex, infobuffer, "name", change_name);

			// update bot's name with this new name
			strcpy(name, change_name);
		}

		clan_tag |= NAMETAG_DONE;
	}
}
// >MedicGuild END<


/*
* try to find the right spot to use the treat to medevac it
*/
void bot_t::BotDoMedEvac(float distance)
{
	// don't start if not close enough
	if ((distance > 70) || (UTIL_IsOldVersion() && (distance > 60)))
		return;

	// the bot is already doing something so wait till he finishes it
	if (f_medic_treat_time > gpGlobals->time)
	{
		SetDontCheckStuck("Bot do MedEvac() -> doing treatment");
		FakeClientCommand(pEdict, "useskill", "treat", nullptr);

		return;
	}

	// bot succeeded so clear all variables
	if (IsSubTask(ST_MEDEVAC_DONE))
	{
		SetDontCheckStuck("Bot do MedEvac() -> finishing it");
		RemoveTask(TASK_HEALHIM);
		pBotEnemy = nullptr;
		
		RemoveTask(TASK_MEDEVAC);
		RemoveSubTask(ST_MEDEVAC_DONE);
		RemoveSubTask(ST_MEDEVAC_ST);
		RemoveSubTask(ST_MEDEVAC_H);
		RemoveSubTask(ST_MEDEVAC_F);

		return;
	}

	edict_t *pCorpse = pBotEnemy;
	float bot_z, corpse_z;

	// if not proned then crouch for cover
	if (IsBehaviour(BOT_PRONED) == FALSE)
		SetStance(this, GOTO_CROUCH, "DoMedEvac()");

	// first try to aim at the centre of the corpse
	if (!IsSubTask(ST_MEDEVAC_ST))
	{
		bot_z = pEdict->v.origin.z + pEdict->v.view_ofs.z;
		corpse_z = pCorpse->v.origin.z;
		
		pEdict->v.v_angle.x = fabs((double) corpse_z - bot_z);

		SetSubTask(ST_MEDEVAC_ST);
		f_medic_treat_time = gpGlobals->time + 0.5;

		return;
	}

	// then try to aim at the head, assuming that the bot is standing at corpse's feet
	// if the bot isn't there, then he will basically try 3 different angles in order to
	// find the "working" spot
	if (!IsSubTask(ST_MEDEVAC_H))
	{
		bot_z = pEdict->v.origin.z + pEdict->v.view_ofs.z;
		corpse_z = pCorpse->v.origin.z + pCorpse->v.view_ofs.z;
		
		pEdict->v.v_angle.x = fabs((double) corpse_z - bot_z);

		SetSubTask(ST_MEDEVAC_H);
		f_medic_treat_time = gpGlobals->time + 0.5;

		return;
	}

	// finally try to aim at the feet
	if (!IsSubTask(ST_MEDEVAC_F))
	{
		bot_z = pEdict->v.origin.z + pEdict->v.view_ofs.z;
		corpse_z = pCorpse->v.origin.z - pCorpse->v.view_ofs.z;
		
		pEdict->v.v_angle.x = fabs((double) corpse_z - bot_z);

		SetSubTask(ST_MEDEVAC_F);
		f_medic_treat_time = gpGlobals->time + 0.5;

		return;
	}

	// if the bot tried all spots then break it, because we probably can't
	// medevac this corpse (the entity is most probably deep inside the map terrain)
	if (IsSubTask(ST_MEDEVAC_ST) && IsSubTask(ST_MEDEVAC_H) && IsSubTask(ST_MEDEVAC_F))
	{
		SetSubTask(ST_MEDEVAC_DONE);
		//UTIL_DebugInFile("Bot can't medevac this corpse, it's unreachable (inside wall etc.)");
	}

}


/*
* check if the patient still bleeds
* we assume that the patient bleeds (usually does)
* so we only need to check the case he doesn't bleed any more
*/
bool IsBleeding(edict_t *pPatient)
{
	int index = UTIL_GetBotIndex(pPatient);

	// is the patient a bot
	if (index != -1)
	{
		if (!bots[index].IsTask(TASK_BLEEDING))
			return FALSE;
	}
	// or is it a human player
	else
	{
		// search this patient in clients array to see if he still bleeds
		for (index = 0; index < MAX_CLIENTS; index++)
		{
			if ((clients[index].pEntity == pPatient) && (clients[index].IsBleeding() == FALSE))
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}

/*
* handles bot medic actions to allow him to heal wounder teammates
*/
bool BotHealTeammate(bot_t *pBot)
{
	edict_t *pEdict = pBot->pEdict;

	// prevents false unstuck behaviour
	pBot->SetDontCheckStuck("Bot HealTeammate()");
	pBot->f_dont_look_for_waypoint_time = gpGlobals->time;

	const float distance = (pBot->pBotEnemy->v.origin - pEdict->v.origin).Length();

	// if the wounded teammate is close enough to start the treatment
	if (((distance < 75) && (UTIL_IsOldVersion() == FALSE)) || ((distance < 50) && UTIL_IsOldVersion()))
	{
		edict_t *pWounded = pBot->pBotEnemy;
		const int skill = pBot->bot_skill;

		float delay[BOT_SKILL_LEVELS] = {0.0, 0.2, 0.5, 0.9, 1.5};

		if (!pBot->IsBehaviour(BOT_CROUCHED))
			//pEdict->v.button |= IN_DUCK;													NEW CODE 094 (prev code)
			SetStance(pBot, GOTO_CROUCH, "HealTeammate()");

		pBot->move_speed = SPEED_NO;	// don't move while treatment

		// handle the false healing
		const int player_team = UTIL_GetTeam(pWounded);
		const int bot_team = UTIL_GetTeam(pEdict);

		if (bot_team != player_team)
		{
			pBot->RemoveTask(TASK_HEALHIM);

			// debugging
			UTIL_DebugInFile("<<BUG>> - teams don't match --> Can't heal enemy!");

			return FALSE;
		}



		//@@@@@@@@@@@@@@@ TEMP					debugging
		if (bot_team == TEAM_NONE || player_team == TEAM_NONE)
			UTIL_DebugInFile("BotHealTeammate | !! TEST !! bot or patient team is TEAM_NONE\n");




		// don't heal when proned AND teammate isn't
		//if (pEdict->v.flags & FL_PRONED)											NEW CODE 094 (prev code)
		if (pBot->IsBehaviour(BOT_PRONED) && !(pBot->pBotEnemy->v.flags & FL_PRONED))
		{
			// stand up
			UTIL_GoProne(pBot, "HealTeammate()->Cannot heal in prone");
			return TRUE;
		}

		// patient is proned AND the bot is NOT fully crouched yet
		if ((pWounded->v.flags & FL_PRONED) && (!(pEdict->v.flags & FL_DUCKING)))
			return TRUE;

		const Vector v_patient = pWounded->v.origin - pEdict->v.origin;
		const Vector bot_angles = UTIL_VecToAngles(v_patient);
		
		pEdict->v.ideal_yaw = bot_angles.y;
		BotFixIdealYaw(pEdict);

		const float bots_head = pEdict->v.origin.z + pEdict->v.view_ofs.z;
		const float patients_head = pWounded->v.origin.z + pWounded->v.view_ofs.z;

		// is patient under the bot? so aim a bit down
		if (bots_head > patients_head)
			pEdict->v.v_angle.x = fabs((double) patients_head - bots_head);
		// is patient above the bot? so aim a bit up
		else if (bots_head < patients_head)
			pEdict->v.v_angle.x = - fabs((double) patients_head - bots_head);

		// we need to look/aim quite a lot down if the patient is proned in FA2.5
		// because the standard way doesn't work for some weird reason
		if ((pWounded->v.flags & FL_PRONED) && (g_mod_version == FA_25))
		{
			pEdict->v.v_angle.x = 50;
		}

		// does the bot already do one of healing processes?
		if (pBot->f_medic_treat_time > gpGlobals->time)
		{
			// if the patient is healed or this bot is giving him some spare bandages do nothing
			if (pBot->IsSubTask(ST_GIVE) || pBot->IsSubTask(ST_HEALED))
				;
			// otherwise try to treat the wounded soldier
			else
				FakeClientCommand(pEdict, "useskill", "treat", nullptr);
			
			return TRUE;
		}

		// the bot just reached his patient ie. didn't start the healing yet
		if ((pBot->f_pause_time + 1.0 < gpGlobals->time) &&
			(pBot->f_medic_treat_time < gpGlobals->time)  &&
			(pBot->f_medic_treat_time + 1.0 < gpGlobals->time))
		{
			// give the bot some time to correctly face his patient
			pBot->SetPauseTime(0.5);

			return TRUE;
		}

		// restores patient's life
		if (pBot->IsSubTask(ST_TREAT))
		{
			// set some time for the treatment
			pBot->f_medic_treat_time = gpGlobals->time + RANDOM_FLOAT(2.5, 3.5) + delay[skill];

			// remove the treat flag
			pBot->RemoveSubTask(ST_TREAT);

			pBot->SetSubTask(ST_HEAL);

			return TRUE;
		}

		// stops bleeding
		else if (pBot->IsSubTask(ST_HEAL))
		{
			// set some time to do this task only if the patient bleeds
			if (IsBleeding(pWounded))
			{
				pBot->f_medic_treat_time = gpGlobals->time + RANDOM_FLOAT(1.0, 2.0) + delay[skill];
			}

			pBot->RemoveSubTask(ST_HEAL);

			// set give flag to allow the bot to give some bandages to the patient
			pBot->SetSubTask(ST_GIVE);

			return TRUE;
		}

		// gives some bandages to the patient
		else if (pBot->IsSubTask(ST_GIVE))
		{
			// give the bandages only if have enough of them OR if there is a chance
			if ((pBot->bot_bandages > 7) ||
				((pBot->bot_bandages > 4) && (RANDOM_LONG(1, 100) <= 25)))
			{
				FakeClientCommand(pEdict, "useskill", "givebandage", nullptr);

				pBot->f_medic_treat_time = gpGlobals->time +
					RANDOM_FLOAT(0.8, 1.3) + delay[skill];
			}

			pBot->RemoveSubTask(ST_GIVE);

			// set healed flag to stop the whole treatment
			pBot->SetSubTask(ST_HEALED);

			return TRUE;
		}

		// stop the whole treatment
		else if (pBot->IsSubTask(ST_HEALED))
		{
			pBot->pBotEnemy = nullptr;
			pBot->RemoveTask(TASK_HEALHIM);
			// don't shoot healed teammate in next few seconds
			pBot->f_shoot_time = gpGlobals->time + 0.2;

			pBot->RemoveSubTask(ST_HEALED);

			return TRUE;
		}

		return TRUE;
	}

	// if the wounded soldier gets too far stop the healing process
	else if (distance > 1000)
	{
		pBot->RemoveTask(TASK_HEALHIM);

		return FALSE;
	}

	// is the bot quite close AND in prone
	//else if ((distance < 250) && (pBot->pEdict->v.flags & FL_PRONED))							NEW CODE 094 (prev code)
	else if ((distance < 250) && pBot->IsBehaviour(BOT_PRONED) && !(pBot->pBotEnemy->v.flags & FL_PRONED))
	{
		// so stand up (ie don't heal in proned position)
		UTIL_GoProne(pBot, "HealTeammate()->Less than 250 units away from patient AND proned");
	}

	// if the bot is closing try to shout on the patient if it is possible
	// to alert him
	else if ((distance < 500) && ((pBot->speak_time + RANDOM_FLOAT(3.0, 6.0)) < gpGlobals->time))
	{
		UTIL_Voice(pBot->pEdict, "medic");
		pBot->speak_time = gpGlobals->time;
			
	}

	return TRUE;
}

/*
* checks if bot is in mid-air
*/
bool BotCheckMidAir(edict_t *pEdict)
{
	// 8200 == FL_CLIENT and FL_FAKECLIENT together and no other flag
	// 1056776 == FL_CLIENT and FL_FAKECLIENT and FL_PROXY together and no other flag in FA 2.5
	// the movetype ensures that we filter a situation when bot is on ladder
	if (((pEdict->v.flags == 8200) && (pEdict->v.movetype != MOVETYPE_FLY)) ||
		((g_mod_version == FA_25) && (pEdict->v.flags == 1056776) &&
		(pEdict->v.movetype != MOVETYPE_FLY)))
		return TRUE;

	return FALSE;
}

/*
* sets the correct move speed based on the speed flag stored in bot structure
*/
float bot_t::BotSetSpeed() const
{
	float tmp_move_speed = f_max_speed;

	switch (move_speed)
	{
		case SPEED_NO:
			tmp_move_speed = 0.0;
			break;
		case SPEED_SLOWEST:
		{
			if (!(pEdict->v.flags & (FL_PRONED | FL_DUCKING)))
				tmp_move_speed = (float)f_max_speed / 4;

			break;
		}
		case SPEED_SLOW:
			{
				// is the speed fast enough and not proned or crouched then slow down
				if (!(pEdict->v.flags & (FL_PRONED | FL_DUCKING)))
					tmp_move_speed = (float) f_max_speed / 2;

				break;
			}
		default:
			break;
	}

	return tmp_move_speed;
}

/*
* when bot dies "fire key/button" must be pressed to respawn
* sets some variables that can't be set in BotSpawnInit() (ie. IMPORTANT RESPAWN ONLY SETTINGS)
* make it as a method for easier reading by Andrey Kotrekhov
*/
void bot_t::BotPressFireToRespawn()
{
	if (RANDOM_LONG(1, 100) > 50)
	{
		pEdict->v.button = IN_ATTACK;
	}

	g_engfuncs.pfnRunPlayerMove( pEdict, pEdict->v.v_angle, BotSetSpeed(),
		                        0, 0, pEdict->v.button, 0, msecval);

	// grenades info flag must be updated after death
	if (grenade_slot != NO_VAL)
		grenade_action = ALTW_NOTUSED;
	else
		grenade_action = ALTW_USED;

	// handling any unpredictable situations like when bot die in throwing grenade etc.
	// have both weapons (main and backup) so clear both flags and use main
	if ((main_weapon != NO_VAL) && (backup_weapon != NO_VAL))
	{
		UseWeapon(USE_MAIN);
		// flags must be set again after death
		main_no_ammo = FALSE;
		backup_no_ammo = FALSE;
	}
	// have main weapon AND no backup weapon so use main
	else if ((main_weapon != NO_VAL) && (backup_weapon == NO_VAL))
	{
		UseWeapon(USE_MAIN);
		main_no_ammo = FALSE;
		backup_no_ammo = TRUE;
	}
	// if no main weapon AND have backup weapon so use it
	else if ((main_weapon == NO_VAL) && (backup_weapon != NO_VAL))
	{
		UseWeapon(USE_BACKUP);
		main_no_ammo = TRUE;
		backup_no_ammo = FALSE;
	}
	// no main weapon AND no backup weapon so use knife
	else if ((main_weapon == NO_VAL) && (backup_weapon == NO_VAL))
	{
		UseWeapon(USE_KNIFE);
		main_no_ammo = TRUE;
		backup_no_ammo = TRUE;
	}

	// these all must be also cleared after death
	bandage_time = gpGlobals->time;
	grenade_time = 0.0;
	chute_action_time = 0.0;

	// older FA versions send parachute message only when the bot has the parachute pack so
	// if the bot dies then we have to reset them manually
	if (UTIL_IsOldVersion() || g_mod_version == FA_26)
	{
		RemoveTask(TASK_PARACHUTE);
		RemoveTask(TASK_PARACHUTE_USED);
		//chute_action_time = 0.0;

		//if (botdebugger.IsDebugActions())
		//	ALERT(at_console, "***BotRespawn resetting parachute\n");
	}

	// respawn with the same gear
	FakeClientCommand(pEdict, "vguimenuoption", "1", nullptr);

	//if (botdebugger.IsDebugActions())
	//	ALERT(at_console, "***BotRespawn called\n");

	return;
}

/* 
* get skill system that current game is using
* sv_useskills 0 - skills are off
* sv_useskills 1 - default, 1 skill on start
* sv_useskills 2 - 4 skills on start
* sv_useskills 3 - all skills on start
*/
void BotCheckSkillSystem(void)
{
	internals.SetFASkillSystem(CVAR_GET_FLOAT("sv_useskills"));  // which system is set?
	internals.SetSkillSystemChecked(true);
}

void bot_t::ReloadWeapon(const char* loc)
{
	// weapon is ready and not pressed the reload button yet
	if ((weapon_action == W_READY) && !IsWeaponStatus(WS_PRESSRELOAD) && (f_reload_time < gpGlobals->time))
	{


#ifdef DEBUG

		//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@														// NEW CODE 094 (remove it)
		if (loc != NULL)
		{
			char dm[256];
			sprintf(dm, "Reload Weapon() called @ <%s>\n", loc);
			HudNotify(dm, true, this);
		}
		//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#endif // DEBUG




		weapon_action = W_INRELOAD;

		// trying to go/resume prone will invalidate reloading in FA
		// so prevent the bot to do so
		SetBehaviour(BOT_DONTGOPRONE);



		//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@			NEW CODE 094 (remove it)
#ifdef DEBUG
		if ((g_debug_bot_on && (g_debug_bot == pEdict)) || !g_debug_bot_on)
		{
			char dp[256];
			sprintf(dp, "%s SET DONTGOPRONE behaviour @ Reload Weapon()\n", name);
			UTIL_DebugInFile(dp);
		}
#endif // DEBUG





		// weapon is going to be reloaded so there's no need to check how many rounds are left in it (it'll be full)
		RemoveSubTask(ST_W_CLIP);
	}
}

/*
* sets correct time to successfully finish reloading based on current weapon
*/
void bot_t::SetReloadTime(void)
{
	// faster reloading due to nomenclature skill
	if (bot_fa_skill & NOMEN)
	{
		if ((current_weapon.iId == fa_weapon_ber92f) || (current_weapon.iId == fa_weapon_coltgov))
			f_reload_time = gpGlobals->time + 2.0;

		else if ((current_weapon.iId == fa_weapon_ber93r) || (current_weapon.iId == fa_weapon_desert) ||
			(current_weapon.iId == fa_weapon_mp5a5) || (current_weapon.iId == fa_weapon_m4) ||
			(current_weapon.iId == fa_weapon_m16) || (current_weapon.iId == fa_weapon_ak47))
			f_reload_time = gpGlobals->time + 3.0;

		else if ((current_weapon.iId == fa_weapon_anaconda) || (current_weapon.iId == fa_weapon_ssg3000) ||
			(current_weapon.iId == fa_weapon_uzi) || (current_weapon.iId == fa_weapon_sterling) ||
			(current_weapon.iId == fa_weapon_m79) || (current_weapon.iId == fa_weapon_m14) ||
			(current_weapon.iId == fa_weapon_g36e) || (current_weapon.iId == fa_weapon_g3a3) ||
			(current_weapon.iId == fa_weapon_ak74))
			f_reload_time = gpGlobals->time + 4.0;

		else if ((current_weapon.iId == fa_weapon_svd) || (current_weapon.iId == fa_weapon_saiga) ||
			(current_weapon.iId == fa_weapon_bizon))
			f_reload_time = gpGlobals->time + 5.0;

		else if ((current_weapon.iId == fa_weapon_m82) || (current_weapon.iId == fa_weapon_m249) ||
			(current_weapon.iId == fa_weapon_m60))
			f_reload_time = gpGlobals->time + 7.0;

		else if (current_weapon.iId == fa_weapon_benelli)
			f_reload_time = gpGlobals->time + (8 - current_weapon.iClip);

		else if (current_weapon.iId == fa_weapon_pkm)
			f_reload_time = gpGlobals->time + 9.0;

		else
			f_reload_time = gpGlobals->time + 6.0;
	}
	// standard reload times
	else
	{
		if ((current_weapon.iId == fa_weapon_ber92f) || (current_weapon.iId == fa_weapon_coltgov))
			f_reload_time = gpGlobals->time + 3.0;

		else if ((current_weapon.iId == fa_weapon_ssg3000) || (current_weapon.iId == fa_weapon_m79) ||
			(current_weapon.iId == fa_weapon_ber93r))
			f_reload_time = gpGlobals->time + 4.0;

		else if ((current_weapon.iId == fa_weapon_bizon) || (current_weapon.iId == fa_weapon_saiga) ||
			(current_weapon.iId == fa_weapon_uzi) || (current_weapon.iId == fa_weapon_desert))
			f_reload_time = gpGlobals->time + 5.0;

		else if ((current_weapon.iId == fa_weapon_m14) || (current_weapon.iId == fa_weapon_g3a3))
			f_reload_time = gpGlobals->time + 7.0;

		else if ((current_weapon.iId == fa_weapon_m60))
			f_reload_time = gpGlobals->time + 9.0;

		else if ((current_weapon.iId == fa_weapon_m249) || (current_weapon.iId == fa_weapon_m82))
			f_reload_time = gpGlobals->time + 10.0;

		else if (current_weapon.iId == fa_weapon_pkm)
			f_reload_time = gpGlobals->time + 12.0;

		// the time varies based on how many slugs are left in the gun
		// remington is loaded with 7 slugs by default, but it allows
		// loading one more so we must work with 8 in the formula
		else if (current_weapon.iId == fa_weapon_benelli)
			f_reload_time = gpGlobals->time + (9 - current_weapon.iClip);

		else if (current_weapon.iId == fa_weapon_anaconda)
		{
			// reloading empty gun is a little faster
			if (current_weapon.iClip == 0)
				f_reload_time = gpGlobals->time + 7.0;
			else
				f_reload_time = gpGlobals->time + (9 - current_weapon.iClip);
		}

		// used for svd, m4, m16, ak47, ak74, g36e, famas, sterling, mp5
		else
			f_reload_time = gpGlobals->time + 6.0;
	}
}

/*
* sets the next best weapon to use based on carried weapons and ammo reserves
*/
void bot_t::DecideNextWeapon(const char* loc)
{


#ifdef DEBUG
	//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@														// NEW CODE 094 (remove it)
	if (loc != NULL)
	{
		char dm[256];
		sprintf(dm, "Decide NextWeapon() called @ %s\n", loc);
		HudNotify(dm, this);
	}
	//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#endif // DEBUG


	// we must first update main weapon and backup weapon empty status
	CheckMainWeaponOutOfAmmo("Decide NextWeapon()");
	CheckBackupWeaponOutOfAmmo("Decide NextWeapon()");

	// try to use main weapon if the bot has any and has enough ammo for it
	if ((main_no_ammo == false) && (main_weapon != NO_VAL))
		UseWeapon(USE_MAIN);
	
	// otherwise try to use backup weapon if exists and has enough ammo
	else if ((backup_no_ammo == false) && (backup_weapon != NO_VAL))
		UseWeapon(USE_BACKUP);
	
	// if all failed then there's only knife left for the use
	else
		UseWeapon(USE_KNIFE);
}

/*
* checks if main weapon is available and the bot can change weapons right now
* (ie bot isn't placing claymore mine or isn't already in process of weapon change)
* if all is fine then the bot sets weapon action to start the weapon change
*/
void bot_t::BotSelectMainWeapon(const char* loc)
{
	if ((main_no_ammo == false) && (current_weapon.iId != main_weapon) && (weapon_action == W_READY) &&
		(f_shoot_time < gpGlobals->time) && (f_use_clay_time < gpGlobals->time) &&
		((clay_action == ALTW_NOTUSED) || (clay_action == ALTW_USED)))
	{


#ifdef DEBUG
		//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@														// NEW CODE 094 (remove it)
		if (loc != NULL)
		{
			char dm[256];
			sprintf(dm, "Select MainWeapon() called @ %s\n", loc);
			HudNotify(dm, true, this);
			//HudNotify(dm);
		}
		//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#endif // DEBUG



		UseWeapon(USE_MAIN);
		
		// set this flag to know that we need to change weapon
		weapon_action = W_TAKEOTHER;
		
		if (botdebugger.IsDebugActions())
			HudNotify("is trying to switch back to MAIN weapon\n", this);
	}
}

/*
* checks if backup weapon is available and the bot can change weapons right now
* we also need to check if the bot couldn't use main weapon
*/
void BotSelectBackupWeapon(bot_t *pBot)
{
	if ((pBot->main_no_ammo) && (pBot->backup_no_ammo == false) &&
		(pBot->current_weapon.iId != pBot->backup_weapon) && (pBot->weapon_action == W_READY) &&
		(pBot->f_shoot_time < gpGlobals->time) && (pBot->f_use_clay_time < gpGlobals->time) &&
		((pBot->clay_action == ALTW_NOTUSED) || (pBot->clay_action == ALTW_USED)))
	{
		pBot->UseWeapon(USE_BACKUP);
		
		// set this flag to know that we need to change weapon
		pBot->weapon_action = W_TAKEOTHER;
		
		if (botdebugger.IsDebugActions())
			HudNotify("is trying to switch to BACKUP weapon\n", pBot);
	}
}

/*
* checks if the bot can change to knife right now
*/
void BotSelectKnife(bot_t *pBot)
{
	if ((pBot->f_shoot_time < gpGlobals->time) && (pBot->f_use_clay_time < gpGlobals->time) &&
		(pBot->weapon_action == W_READY) &&
		((pBot->clay_action == ALTW_NOTUSED) || (pBot->clay_action == ALTW_USED)))
	{
		pBot->UseWeapon(USE_KNIFE);
		
		// set this flag to know that we need to change weapon
		pBot->weapon_action = W_TAKEOTHER;
		
		if (botdebugger.IsDebugActions())
			HudNotify("is trying to switch to the KNIFE\n", pBot);
	}
}

/*
* routines we need to handle targeting a pItem entity
*/
void bot_t::FacepItem(void)
{
	// called in mistake? so don't continue
	if (!IsTask(TASK_IGNOREAIMS))
		return;

	// no pItem?
	if (pItem == nullptr)
	{
		// scan surrounding for some breakables
		UTIL_CheckForBreakableAround(this);

		// still no pItem?
		if (pItem == nullptr)
		{
#ifdef _DEBUG
			char msg[256];
			sprintf(msg, "(bot.cpp) - FacepItem() - pItem is NULL");
			HudNotify(msg, this);
			UTIL_DebugDev(msg, curr_wpt_index, curr_path_index);
#endif
			RemoveTask(TASK_IGNOREAIMS);

			return;
		}
	}

	Vector entity_origin = VecBModelOrigin(pItem);

	// the pItem is currently in FOV (very tight cone here) so don't face it
	if (FInNarrowViewCone(&entity_origin, pEdict, 0.95))
		f_face_item_time = gpGlobals->time;
	// otherwise set time to start facing it
	else
		f_face_item_time = gpGlobals->time + 0.1;
			
	// do nothing special when the bot isn't facing it yet, the only thing we need to do is to
	// prevent the bot pressing the fire button
	if (f_face_item_time > gpGlobals->time)
	{
		;
	}
	// otherwise the bot is facing it so he can start shooting
	else
	{
		RemoveTask(TASK_IGNOREAIMS);
		SetSubTask(ST_FACEITEM_DONE);
	}

	// we must keep the object in sight and looking at it at correct angle...

	if (IsSubTask(ST_RANDOMCENTRE))
		entity_origin = entity_origin + Vector((RANDOM_LONG(1, 10) - 5), (RANDOM_LONG(1, 10) - 5), (RANDOM_LONG(1, 10) - 5));

	const Vector entity = entity_origin - (pEdict->v.origin + pEdict->v.view_ofs);
	const Vector bot_angles = UTIL_VecToAngles(entity);

	pEdict->v.idealpitch = -bot_angles.x;
	BotFixIdealPitch(pEdict);
	pEdict->v.ideal_yaw = bot_angles.y;
	BotFixIdealYaw(pEdict);
}


/*
* resets all aim waypoint slots as well as current aiming stage
*/
void bot_t::ResetAims(const char* loc)
{
	aim_index[0] = -1;
	aim_index[1] = -1;
	aim_index[2] = -1;
	aim_index[3] = -1;
	curr_aim_index = -1;

	f_aim_at_target_time = 0.0;
	f_check_aim_time = 0.0;
	targeting_stop = 0;

	RemoveSubTask(ST_AIM_DONE);
	RemoveSubTask(ST_FACEITEM_DONE);
	RemoveTask(TASK_IGNOREAIMS);

	// if current waypoint isn't a shoot waypoint then reset also the precise aiming
	if (!IsFlagSet(W_FL_FIRE, curr_wpt_index))
		RemoveTask(TASK_PRECISEAIM);

#ifdef _DEBUG
	if (loc != NULL)
	{
		char msg[256];
		sprintf(msg, "ResetAims() called @ %s\n", loc);
		HudNotify(msg, this);
	}

	if (botdebugger.IsDebugActions() || botdebugger.IsDebugAims())
	{
		char msg[256];
		sprintf(msg, "*** aiming marks were cleared (curr wpt index %d)\n", curr_wpt_index + 1);
		HudNotify(msg, this);
	}
#endif
}


/*
* routines we need to handle aim waypoint targeting
*/
void bot_t::TargetAimWaypoint(const char* loc)
{
	// the bot is forced to ignore any aim waypoint so break this method right here
	// don't run it even if the bot is currently using bipod
	if (IsTask(TASK_IGNOREAIMS) || IsSubTask(ST_FACEITEM_DONE) ||
		IsTask(TASK_BIPOD) || (f_bipod_time > gpGlobals->time)) //							NEW CODE 094

	{


#ifdef DEBUG																	// NEW CODE 094 Test
		if (loc != NULL)
		{
			char msg[256];
			sprintf(msg, "TargetAimWpt() @ wpt_index=%d -> IgnoreAims OR FaceItemDone OR TaskBipod OR HandlingBipod -->> BREAK!!!\n",
				aim_index[0] + 1);
			HudNotify(msg, this);
		}
#endif // DEBUG


		RemoveSubTask(ST_AIM_FACEIT);
		return;
	}


	const int local_wpt = curr_wpt_index;

	if (local_wpt == -1)
	{
		// public debugging
		char msg[256];
		sprintf(msg, "(bot.cpp) - TargetAimWaypoint() - local/current wpt is #-1 (previous wpt is #%d, path is #%d)\n Possible reason: there's no goback wpt at the end of the path and the path doesn't end at cross (ie. wrongly set camping spot)\n",
			prev_wpt_index.get(), curr_path_index);
		UTIL_DebugInFile(msg);

		if (botdebugger.IsDebugActions() || botdebugger.IsDebugAims() || botdebugger.IsDebugWaypoints())
			HudNotify(msg);

		// set it so like we succeeded even if there aren't any waypoints/targets
		RemoveSubTask(ST_AIM_FACEIT);
		SetSubTask(ST_AIM_DONE);



#ifdef DEBUG																	// NEW CODE 094 Test
		char tm[256];
		sprintf(tm, "TargetAimWpt() FATAL ERROR -> curr_wpt is -1 -->> BREAK!!!\n");
		HudNotify(tm, this);
#endif // DEBUG


		return;
	}



#ifdef DEBUG																	// NEW CODE 094 Test
	if (loc != NULL)
	{
		char msg[256];
		sprintf(msg, "TargetAimWpt() for wpt=%d called @ %s\n", local_wpt + 1, loc);
		HudNotify(msg, this);
	}
#endif // DEBUG



	if (botdebugger.IsDebugAims())
		ALERT(at_console, "current target=%d || aim1=%d | aim2=%d | aim3=%d | aim4=%d\n",
		curr_aim_index + 1, aim_index[0] + 1, aim_index[1] + 1,
		aim_index[2] + 1, aim_index[3] + 1);

	// if the array of aims is empty then fill it
	if (aim_index[0] == -1)
		WaypointFindNearestAiming(this, &waypoints[local_wpt].origin);

	// there's no aim waypoint around
	if (f_aim_at_target_time == -1.0)
	{
		// then use next waypoint from the path as the aim target
		aim_index[0] = GetNextWaypoint();

		// if even that failed then there really is nothing to target...
		if (aim_index[0] == -1)
		{
			// so reset things and quit
			RemoveSubTask(ST_AIM_FACEIT);
			SetSubTask(ST_AIM_DONE);



#ifdef DEBUG																	// NEW CODE 094 Test
			char msg[256];
			sprintf(msg, "TargetAimWpt() @ wpt_index=%d -> NOTHING to aim at -->> BREAK!!!\n",
				aim_index[0] + 1);
			HudNotify(msg, this);
#endif // DEBUG


			return;
		}
		
		// now we know the bot will watch the next waypoint on current path but we must reset target time
		// because when we didn't find any aim waypoint around it was set to -1.0 as an 'error' marker
		// and we would create an infinite loop at the 'if' check above
		f_aim_at_target_time = 0.0;

		if (botdebugger.IsDebugAims() || botdebugger.IsDebugWaypoints())
		{
			char msg[256];
			sprintf(msg, "<<WARNING>> There are no aim waypoints around! Using the next path waypoint #%d as the target\n",
				aim_index[0] + 1);
			HudNotify(msg, this);
		}
	}

	// the bot has no aim "vector" yet, this statement must be first
	if (IsSubTask(ST_AIM_GETAIM))
	{
		int new_target = RANDOM_LONG(1, 4) - 1;
		
		// count how many aim waypoints the bot has
		int aim_count = 0;

		for (int i = 0; i < 4; i++)
		{
			if (aim_index[i] != -1)
				aim_count++;
		}

		// is there only one?
		if (aim_count < 2)
		{
			// search for valid aim waypoint
			while (aim_index[new_target] == -1)
				new_target = RANDOM_LONG(1, 4) - 1;
		}
		// there are at least two so ...
		else
		{
			// search for valid aim waypoint ignoring current aim waypoint
			while ((aim_index[new_target] == -1) && (aim_index[new_target] == curr_aim_index))
				new_target = RANDOM_LONG(1, 4) - 1;
		}
		
		// store new aim waypoint as current target
		curr_aim_index = aim_index[new_target];

		RemoveSubTask(ST_AIM_GETAIM);
	}

	// face the bot towards current aim waypoint
	if (IsSubTask(ST_AIM_FACEIT))
	{
		// no aim "vector" yet?
		if (curr_aim_index == -1)
		{
			// go get some
			SetSubTask(ST_AIM_GETAIM);
			// but we'll do it in next call in next game frame
			return;
		}

		// if the bot isn't able to precisely target this aim waypoint
		// just reset it back to where it should be and break it
		//if (targeting_stop >= 50)															NEW CODE 094 (prev code)
		if (targeting_stop >= 250)//														NEW CODE 094
		{
			const Vector v_aim = waypoints[curr_aim_index].origin - waypoints[curr_wpt_index].origin;
			const Vector aim_angles = UTIL_VecToAngles(v_aim);			
			pEdict->v.ideal_yaw = aim_angles.y;
			BotFixIdealYaw(pEdict);

			RemoveSubTask(ST_AIM_FACEIT);
			SetSubTask(ST_AIM_DONE);

			// remove this one as well, bot isn't able to handle it
			RemoveTask(TASK_PRECISEAIM);

			// send this problem to debug file
			char msg[256];
			sprintf(msg, "(WAYPOINT BUG) - Bot isn't able to target aim waypoint #%d from waypoint #%d!\nTry moving the aim waypoint a bit away from \"master\" waypoint.\n Or reduce the range on \"master\" waypoint.\n",
				curr_aim_index + 1, curr_wpt_index + 1);
			UTIL_DebugInFile(msg);

			if (botdebugger.IsDebugAims() || botdebugger.IsDebugWaypoints())
				HudNotify(msg);


#ifdef DEBUG																	// NEW CODE 094 Test
			char tmsg[256];
			sprintf(tmsg, "TargetAimWpt() for AIM wpt #%d -> Number of tries to target aim EXCEEDED 250 -->> BREAK!!!\n",
				curr_aim_index + 1);
			HudNotify(tmsg, this);
#endif // DEBUG


			return;
		}

		//																						NEW CODE 094
		// the bot is forced to face the aim before setting the done flag
		if (IsTask(TASK_PRECISEAIM))
		{
			// face current aim waypoint
			if (f_aim_at_target_time > gpGlobals->time)
			{
				const Vector v_aim = waypoints[curr_aim_index].origin - waypoints[curr_wpt_index].origin;
				const Vector aim_angles = UTIL_VecToAngles(v_aim);

				pEdict->v.ideal_yaw = aim_angles.y;
				BotFixIdealYaw(pEdict);

				// safety stop for a case the bot cannot face current aim waypoint (get it in his limited view cone)
				targeting_stop++;

				// we know that check aim time holds the moment when we started facing this aim waypoint
				// so if this statement pass we know the bot has already been adjusting his yaw angles for almost 1 second
				// also we can't go any higher, because 1 second is the minimal time people can set as wait time and
				// if we exceeded it, the bot won't be able to shoot at such waypoint at all
				if (f_check_aim_time + 0.8 < gpGlobals->time)
				{
					// so now we can try if the bot is really facing the aim waypoint by checking if
					// it is in his quite narrow view cone
					// if there's a combination of waypoint with bigger range and an aim waypoint quite close to it
					// then bot standing on the outer limit of the range may not be able to get the aim waypoint
					// in this narrow view cone, but the safety stop will break the loop and allow the bot to act
					if (FInNarrowViewCone(&waypoints[curr_aim_index].origin, pEdict, 0.7))
					{
						RemoveSubTask(ST_AIM_FACEIT);
						SetSubTask(ST_AIM_DONE);

						if (botdebugger.IsDebugAims())
						{
							char msg[256];
							sprintf(msg, "TargetAimWpt() for AIM wpt #%d -> Task PRECISE AIM -> in narrow view cone DONE\n",
								curr_aim_index + 1);
							HudNotify(msg, this);
						}

						return;
					}
				}
			}

			// set time to allow the bot to face the aim target (must be last in row)
			if (f_aim_at_target_time < gpGlobals->time)
			{
				f_aim_at_target_time = gpGlobals->time + RANDOM_FLOAT(2.0, 4.0);
				f_check_aim_time = gpGlobals->time;

				return;
			}


			/*/																		NEW CODE 094 (prev code) - causes bugs
			// the aim waypoint is currently in FOV so don't face it
			if (FInNarrowViewCone(&waypoints[curr_aim_index].origin, pEdict))
				f_face_wpt = gpGlobals->time;
			// otherwise set time to start facing it
			else
				f_face_wpt = gpGlobals->time + 0.1;
			
			// face current aim waypoint
			if (f_face_wpt > gpGlobals->time)
			{
				Vector v_aim = waypoints[curr_aim_index].origin - waypoints[curr_wpt_index].origin;
				Vector aim_angles = UTIL_VecToAngles(v_aim);
				
				// some off-set
				aim_angles.y += RANDOM_LONG(0, 10) - 5;
				
				pEdict->v.ideal_yaw = aim_angles.y;
				BotFixIdealYaw(pEdict);
				
				// safety stop for a case the bot cannot face current aim
				targeting_stop++;
			}
			// otherwise everything is done
			else
			{
				RemoveSubTask(ST_AIM_FACEIT);
				SetSubTask(ST_AIM_DONE);
			}
			/**/
		}
		// otherwise just turn towards it
		else
		{
			const Vector v_aim = waypoints[curr_aim_index].origin - waypoints[curr_wpt_index].origin;
			Vector aim_angles = UTIL_VecToAngles(v_aim);
				
			// some off-set
			aim_angles.y += RANDOM_LONG(0, 10) - 5;
				
			pEdict->v.ideal_yaw = aim_angles.y;
			BotFixIdealYaw(pEdict);

			RemoveSubTask(ST_AIM_FACEIT);
			SetSubTask(ST_AIM_DONE);

			if (botdebugger.IsDebugAims())
			{
				char msg[256];
				sprintf(msg, "TargetAimWpt() for AIM wpt #%d -> face it DONE\n", curr_aim_index + 1);
				HudNotify(msg, this);
			}

		}
	}

	// set the watch time
	if (IsSubTask(ST_AIM_SETTIME))
	{
		int aim_count = 0;
			
		// count how many valid aim waypoints the bot has
		for (int i = 0; i < 4; i++)
		{
			if (aim_index[i] != -1)
				aim_count++;
		}

		// if there are more aim waypoints divide the whole wait time between them
		if (aim_count > 1)
		{
			float the_time = WaypointGetWaitTime(local_wpt, bot_team);

			// if there is no wait time on the waypoint (bot is waiting for some special
			// command such as wait until something respawns) then generate some wait time
			if (the_time == 0.0)
				the_time = RANDOM_FLOAT(2.0, 5.0);
				
			// see how many times the bot should change aim waypoint
			// (i.e. the longer the wait time is the more changes are there)
			if ((the_time > 25.0) && (the_time < 50.0))
				aim_count = RANDOM_LONG(4, 8);
			else if ((the_time > 50.0) && (the_time < 120.0))
				aim_count = RANDOM_LONG(8, 12);
			else if (the_time > 120.0)
				aim_count = RANDOM_LONG(12, 24);

			the_time /= (float) aim_count;
			
			f_aim_at_target_time = gpGlobals->time + the_time;
			f_check_aim_time = gpGlobals->time;
		}
		// otherwise target the only aim waypoint and keep it for the whole wait time
		else if (aim_count == 1)
		{
			f_aim_at_target_time = wpt_wait_time;
			f_check_aim_time = gpGlobals->time;
		}
		// there are no aim waypoints around
		else if (aim_count == 0)
		{


			/// TODO: Change this, probably use of the ignoreaim task and the bot will then
			// forget about	any aim wpts targeting and use the lft, right... facing idea



			// NOTE: this should "somehow" be changed to fill bot->aim_indexes with all
			// four directions (forward, left, backward, right)
			// and sniper time = waittime / 4



			// for now do target previous waypoint
			aim_index[0] = prev_wpt_index.get();
				
			f_aim_at_target_time = wpt_wait_time;
			f_check_aim_time = gpGlobals->time;
		}

		RemoveSubTask(ST_AIM_SETTIME);

		return;
	}

	// make sure the bot is really watching current aim
	if (IsSubTask(ST_AIM_ADJUSTAIM))
	{
		// check it again in 2.5 sec
		f_check_aim_time = gpGlobals->time + 2.5;
		
		SetSubTask(ST_AIM_FACEIT);
		RemoveSubTask(ST_AIM_ADJUSTAIM);
	}
}

/*
* handles several things when the bot gets to waypoint with some wait time
*/
void bot_t::BotWaitHere()
{
	int local_wpt = -1;	// holds pBot->curr/prev_wpt_index
	
	f_waypoint_time = gpGlobals->time;
	f_dont_look_for_waypoint_time = gpGlobals->time;

	//																													NEW CODE 094
	// if the bot is doing side steps then
	// he is still in process of finding a free place to wait on this waypoint
	// (i.e. someone else is blocking him)
	if (f_strafe_time >= gpGlobals->time)
	{
		// so we must update the wait time to postpone the wait action
		// for the moment when the bot is finally positioned correctly
		wpt_wait_time = wpt_wait_time + 0.2;
		return;
	}
	//***************************************************************************************************************	NEW CODE 094 END
	
	move_speed = SPEED_NO;

#ifdef DEBUG
	SetDontCheckStuck();
	//SetDontCheckStuck("Bot WaitHere()");
#else
	SetDontCheckStuck();
#endif // DEBUG
	
	local_wpt = curr_wpt_index;
	
	// is the bot waiting until door opens
	if (IsSubTask(ST_DOOR_OPEN) && (waypoints[local_wpt].flags & (W_FL_DOOR | W_FL_DOORUSE)))
	{
		// there are no doors at all ... forget about it
		//if (v_door == Vector(0, 0, 0))
		if (v_door == g_vecZero)
		{
			if (botdebugger.IsDebugActions())
				HudNotify("***WaitHere()|WaitAtDoorWpt| !!! NO DOORS !!! -> leaving\n", this);

			RemoveSubTask(ST_DOOR_OPEN);
			wpt_wait_time = gpGlobals->time - 0.1;

			return;
		}

		// do increase the wait time so the bot can't move
		wpt_wait_time = gpGlobals->time + 1.0;
		f_dont_avoid_wall_time = gpGlobals->time + 2.0;//5.0; <- prev code

		// is the bot facing the doors
		if (FInNarrowViewCone(&v_door, pEdict, 0.95))
		{
			UTIL_MakeVectors(pEdict->v.v_angle);

			const Vector v_src = pEdict->v.origin + pEdict->v.view_ofs;
			const Vector v_dest = v_src + gpGlobals->v_forward * 70;
			
			TraceResult tr;
			UTIL_TraceLine( v_src, v_dest, ignore_monsters, pEdict->v.pContainingEntity, &tr);
			
#ifdef DEBUG

			//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ TEST
			//UTIL_HighlightTrace(v_src, v_dest, pEdict);

#endif // DEBUG

			bool doors = FALSE;
			
			if ((strcmp("func_door", STRING(tr.pHit->v.classname)) == 0) ||
				(strcmp("func_door_rotating", STRING(tr.pHit->v.classname)) == 0) ||
				(strcmp("momentary_door", STRING(tr.pHit->v.classname)) == 0))
				doors = TRUE;
			
			// are the doors still closed?
			if ((tr.flFraction < 1.0) && doors)
			{				
				// try touching the door again if not opened yet
				if (RANDOM_LONG(1, 100) <= 25)//5) <- prev code
				{
					move_speed = SPEED_SLOW;
					
					if (waypoints[local_wpt].flags & W_FL_DOORUSE)
						pEdict->v.button |= IN_USE;


#ifdef DEBUG
					//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
					HudNotify("***WaitHere()|WaitAtDoorWpt -> Touching doors\n", this);
#endif // DEBUG


				}
			}
			// the doors opened so break the waiting and continue in navigation
			else
			{
				RemoveSubTask(ST_DOOR_OPEN);
				wpt_wait_time = gpGlobals->time - 0.1;

#ifdef DEBUG
				//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
				HudNotify("***WaitHere()|WaitAtDoorWpt -> Doors OPENED so leaving\n", this);
#endif // DEBUG

			}
		}
		// otherwise do face it
		else
		{
			const Vector door_angle = v_door - pEdict->v.origin;
			const Vector bot_angles = UTIL_VecToAngles(door_angle);
			
			pEdict->v.ideal_yaw = bot_angles.y;
			BotFixIdealYaw(pEdict);
		}
	}
	
	// is bot waiting at claymore waypoint and is already facing the aim waypoint?
	else if ((waypoints[local_wpt].flags & W_FL_MINE) && IsSubTask(ST_AIM_DONE))
	{
		// select and prepare the claymore mine
		if ((clay_action == ALTW_NOTUSED) || (clay_action == ALTW_TAKING))
			BotPlantClaymore(this);
		// is claymore ready to be placed?
		else if ((clay_action == ALTW_PREPARED))
		{
			bool placed_as_goal = FALSE;

			// is a team priorities set to goal (ie. == 1) so set goal flag
			if (WaypointGetPriority(local_wpt, bot_team) == 1)
			{
				placed_as_goal = TRUE;
			}

			//																	NEW CODE 094 (prev code)
			/*/
			int result = -1;

			// try to find an aim waypoint around
			result = WaypointFindNearestAiming(this, &waypoints[local_wpt].origin);

			// no aim waypoint around then try to get the next waypoint
			if (result == -1)
				result = GetNextWaypoint();

			if (result != -1)
			{
				// if there was no aim waypoint but we have the next waypoint then we must set
				// the next waypoint as our target otherwise the bot won't face anything
				if (aim_index[0] == -1)
				{
					aim_index[0] = result;
					// we must reset it here because the find nearest aim routine didn't find
					// any aim waypoint so this has been set to "error" and thus the target aim
					// waypoint wouldn't then be able to turn the bot at the next waypoint
					f_aim_at_target_time = 0.0;
				}

				SetTask(TASK_PRECISEAIM);
				SetSubTask(ST_AIM_FACEIT);
			}
			// there's no aim waypoint as well as no next waypoint (we're at the end of the path)
			// so we must tell the bot that he's already facing the right direction
			else
			{
				SetSubTask(ST_AIM_DONE);
			}

			// don't let the bot set the mine until he's facing desired direction
			if (IsSubTask(ST_AIM_DONE))
			{
				f_shoot_time = gpGlobals->time - 0.1;



				#ifdef _DEBUG
				//@@@@@@@@@@@@@@@@@@@@@@
				HudNotify("CLAYMORE - already facing desired direction - time to USE it\n");
				#endif


			}
			else
			{
				f_shoot_time = gpGlobals->time + 1.0;
				wpt_wait_time = gpGlobals->time + 1.0;



				#ifdef _DEBUG
				//@@@@@@@@@@@@@@@@@@@@@@
				HudNotify("CLAYMORE -> in process of facing desired direction\n");
				#endif


			}
			/**/
			
			// is it time to finally use the mine?
			if ((f_shoot_time <= gpGlobals->time) && (f_use_clay_time < gpGlobals->time))
			{
				// put it on ground
				pEdict->v.button |= IN_ATTACK;
				
				// should it be detonated manually ie "goal clay"
				if (placed_as_goal)
				{
					// set quite long time to allow the bot to get to safe distance
					f_use_clay_time = gpGlobals->time + 6.0;
					
					// change the flag to know it's a goal to detonate it
					clay_action = ALTW_GOALPLACED;
				}
				// standard trip mode
				else
				{
					// set some time to leave the area before the mine is activated
					f_use_clay_time = gpGlobals->time + 3.0;

					// update the flag to know that the mine is on ground
					clay_action = ALTW_PLACED;
				}

				// and break the wait time to allow the bot to run away
				wpt_wait_time = gpGlobals->time;

				if (botdebugger.IsDebugActions())
					HudNotify("Claymore mine has been SUCCESSFULLY placed!\n", this);
			}
		}

		// meanwhile keep turning towards the aim waypoint
		TargetAimWaypoint();

		// the bot wasn't able to place the claymore correctly due to some obstruction
		// so we have to reset it
		if (IsSubTask(ST_RESETCLAYMORE))
		{
			wpt_wait_time = gpGlobals->time;
			clay_action = ALTW_NOTUSED;
			RemoveSubTask(ST_RESETCLAYMORE);

			if (botdebugger.IsDebugActions())
			{
				char dam[256];
				sprintf(dam, "FAILED to set claymore mine at waypoint #%d -> action canceled!\n", local_wpt + 1);
				HudNotify(dam, this);
			}
		}
	}

	// is the bot waiting at shoot waypoint and is already facing the aim waypoint?
	else if ((waypoints[local_wpt].flags & W_FL_FIRE) && (IsSubTask(ST_AIM_DONE)))
	{
		bool dont_shoot_yet = true;

		// if the priority is set to no priority then don't shoot
		if (WaypointGetPriority(local_wpt, bot_team) == 0)
			dont_shoot_yet = true;
		// if the waypoint has priority == 1 then don't check for object's existence
		// ie. the bot will always fire
		else if (WaypointGetPriority(local_wpt, bot_team) == 1)
			dont_shoot_yet = false;
		else
		{
			// initialize it to 'Yes, let's shoot', but go check...
			dont_shoot_yet = false;

			// if there is any breakable object in front of the bot
			if (UTIL_CheckForwardForBreakable(pEdict) == FALSE)
			{
				// the first check wasn't successful (ie. there's nothing in front of the bot)
				// so try to check the space around the bot
				if (UTIL_CheckForBreakableAround(this) == FALSE)
				{
					// there's completely nothing
					// so set it to NOT shoot
					// reset wait time and all tasks and subtask we used here
					// to return the bot back to normal navigation
					dont_shoot_yet = true;
					wpt_wait_time = gpGlobals->time;
					RemoveSubTask(ST_AIM_DONE);
					RemoveSubTask(ST_FACEITEM_DONE);
					RemoveTask(TASK_PRECISEAIM);





					//@@@@@@@@@@@@@@@@@@@@@@@
					#ifdef _DEBUG
					HudNotify("bot.cpp ->WAIT at the SHOOT wpt --- completely NO OBJECT around !!!!!\n", this);
					#endif




				}
				// we found something somewhere around the bot so...
				else
				{


					//@@@@@@@@@@@@@@@@@@@@@@@
					#ifdef _DEBUG
					HudNotify("bot.cpp ->WAIT at the SHOOT wpt --- there's SOME OBJECT in surrounding! YAY!\n", this);
					#endif


					// the bot has to ignore aim waypoints to successfully face the object
					SetTask(TASK_IGNOREAIMS);
					// we also need to set some wait time so the bot won't
					// run away if there are still any breakables in the vicinity
					wpt_wait_time = gpGlobals->time + 30.0;

					// don't let the bot fire until he's facing the item
					if (IsSubTask(ST_FACEITEM_DONE) == FALSE)
						dont_shoot_yet = true;
				}
			}
		}
		
		// can the bot shoot at the breakable?
		if (dont_shoot_yet == false)
		{
			//  																				 NEW CODE 094

			// allow any further actions only if the weapon is ready and the bot isn't going prone
			if ((weapon_action == W_READY) && (f_shoot_time < gpGlobals->time) &&
				(IsGoingProne() == FALSE))
			{
				// check if the weapon is empty (except for knife of course)
				if ((current_weapon.iClip == 0) && (current_weapon.iId != fa_weapon_knife))
				{
					dont_shoot_yet = true;

					// if the bot has any magazines for this weapon then reload it
					if (current_weapon.iAmmo1 != 0)
						ReloadWeapon("WaitHere() -> weapon is empty at shoot waypoint");
					else
						DecideNextWeapon("WaitHere() -> weapon is empty at shoot waypoint");
				}

				// if the bot is sniper he should zoom in
				// (that's due to stupid feature in FA3.0 where the no zoom accuracy of sniper rifle is really awful)
				else if (UTIL_IsSniperRifle(current_weapon.iId) && (pEdict->v.fov == NO_ZOOM))
				{
					pEdict->v.button |= IN_ATTACK2;
					f_shoot_time = gpGlobals->time + 1.0;
					dont_shoot_yet = true;

					if (botdebugger.IsDebugActions())
					{
						HudNotify("Sniper rifle zoom in when at shoot waypoint\n", this);
					}
				}

				// is it time to fire the weapon now?
				if (dont_shoot_yet == false)
				{
					// is weapon firemode NOT auto so press "fire" button
					if (current_weapon.iFireMode != 4)
					{
						pEdict->v.button |= IN_ATTACK;
						f_shoot_time = gpGlobals->time + RANDOM_FLOAT(0.3, 0.6);

						//if (botdebugger.IsDebugActions())
						//{
						//	HudNotify("going to shoot current weapon\n", this);
						//}
					}

					// otherwise hold down "fire" button
					if (current_weapon.iFireMode == 4)
					{
						pEdict->v.button |= IN_ATTACK;
						// store the time even here so the bot knows when did he shoot last
						f_shoot_time = gpGlobals->time;
					}
				}
			}
		}
	}
	// the bot must be waiting on standard wait command (ie. waypoint with some wait time)
	// we need to search for some aim waypoint in this case
	else
	{
		// no time for watching current aim waypoint?
		if (f_aim_at_target_time < gpGlobals->time)
		{
			// force the bot to pick one of aim waypoints from his aim array
			SetSubTask(ST_AIM_GETAIM);
			// force the bot to face it
			SetSubTask(ST_AIM_FACEIT);
			// tell the bot how long he should watch it
			SetSubTask(ST_AIM_SETTIME);
			// reset the safety counter
			targeting_stop = 0;
		}

		// is it time to adjust aiming to selected aim waypoint
		if ((f_aim_at_target_time > gpGlobals->time) && (f_check_aim_time < gpGlobals->time))
			SetSubTask(ST_AIM_ADJUSTAIM);

		// handle the aim waypoint targeting
		TargetAimWaypoint();

		//																														 NEW CODE 094
		// use the wait time to check if it is a good moment to merge magazines?
		// merge magazines must be before weapon reload check, because if the bot was left
		// with only partly used magazines then he would loop weapon reload
		if (IsWeaponStatus(WS_MERGEMAGS2) && (weapon_action == W_READY) &&
			(IsGoingProne() == FALSE) && (f_reload_time < gpGlobals->time))//(f_reload_time + 1.0 < gpGlobals->time))
		{
			BotMergeClips(this, "WaitHere()");
		}
		// also check if the weapon needs to be reloaded
		else if ((IsGoingProne() == FALSE) && UTIL_ShouldReload(this, "WaitHere() -> wait-timed waypoint"))
		{
			ReloadWeapon("WaitHere() -> weapon is empty while waiting at wait-timed waypoint");
		}
		// is the bot already facing his current aim waypoint AND does he have a weapon with bipod attachement AND
		// there's some time passed since last use AND there's quite long wait time on this waypoint AND
		// there is still enough wait time left (we don't want to deploy it and fold it right away, because
		// the wait time is about to end and the bot must be able to leave the waypoint)
		else if (IsBipodWeapon(current_weapon.iId) && IsSubTask(ST_AIM_DONE) &&
			!IsTask(TASK_BIPOD) && !IsWeaponStatus(WS_CANTBIPOD) &&
			(WaypointGetWaitTime(curr_wpt_index, bot_team) > 50.0) && (gpGlobals->time + 10.0 < wpt_wait_time))
		{
			// then try deploying the bipod
			BotUseBipod(this, FALSE, "WaitHere() -> DEPLOY IT");
		}

		// break the waiting if no ammo for main weapon
		// we don't want bots (especially snipers or machine gunners) to guard some spot if they can't kill the enemy
		// do NOT break the wait time if this waypoint is either shoot or claymore where the bot must do the action
		if (main_no_ammo && (main_weapon != NO_VAL) &&
			!IsWaypoint(local_wpt, WptT::wpt_fire) && !IsWaypoint(local_wpt, WptT::wpt_mine))
		{
			wpt_wait_time = gpGlobals->time;

			if (botdebugger.IsDebugActions())
			{
				char msg[256];
				sprintf(msg, "Ran OUT OF AMMO for main weapon! Breaking the wait at my current waypoint (index=%d)!\n",
					curr_wpt_index + 1);
				HudNotify(msg);
			}


#ifdef DEBUG

			//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@														// NEW CODE 094 (remove it)
			char dm[256];
			sprintf(dm, "%s breaking the wait at wait-timed wpt -> ran OUT OF AMMO for main weapon!\n", name);
			HudNotify(dm, true, this);
			//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#endif // DEBUG
		}
		//	****************************************************************************************************************	NEW CODE 094 END
	}
		
	// is the bot waiting at crouch waypoint keep "crouch key" pressed down
	if (waypoints[local_wpt].flags & W_FL_CROUCH)
		SetStance(this, GOTO_CROUCH, "WaitHere()|Waiting at crouch wpt -> GOTO crouch");
	// is the bot waiting at prone waypoint AND bot is NOT proned yet go prone
	else if ((waypoints[local_wpt].flags & W_FL_PRONE) && (IsBehaviour(BOT_PRONED) == FALSE))
	{
		//UTIL_GoProne(this, "WaitHere()|Waiting at prone wpt -> NOT in prone yet");														NEW CODE 094 (prev code)
		
		// is the bot about to go prone
		if (UTIL_GoProne(this, "WaitHere()|Waiting at prone wpt -> NOT in prone yet"))
		{
			// then we must reset don't check for stuck otherwise the bot won't be able to
			// deal with any obstacle that may block this action (e.g. someone else already laying there)
			SetDontCheckStuck("WaitHere()|Waiting at prone wpt|NOT in prone yet -> RESETTING DontCheck for STUCK!!!", -1.0);
		}
	}
	//																															NEW CODE 094
	else if (IsBehaviour(BOT_CROUCHED) && !IsWaypoint(local_wpt, WptT::wpt_crouch))
		SetStance(this, GOTO_STANDING, "WaitHere()|Bot in crouch but NOT at crouch wpt -> GOTO standing");
	//	*********************************************************************************************************************	NEW CODE 094 END
	
	if (botdebugger.IsDebugActions())
	{
		static char prev_msg[256] = "";
		char dmsg[256];

		sprintf(dmsg, "%s is waiting at waypoint #%d | prev_wpt[0] was #%d and prev_wpt[1] was #%d\n",
			name, curr_wpt_index + 1, prev_wpt_index.get() + 1, prev_wpt_index.get(1) + 1);

		// don't print the message every frame
		if (strcmp(prev_msg, dmsg) != 0)
		{
			ALERT(at_console, dmsg);
			strcpy(prev_msg, dmsg);
		}

		// but once in a while allow repeating this message
		if (RANDOM_LONG(1, 100) > 99)
			prev_msg[0] = '\0';
	}
	
	// is it time to check ammunition to know how many of additional mags we need
	if (check_ammunition_time + 5.0 < gpGlobals->time)
	{
		SetTask(TASK_CHECKAMMO);
	}

	// check if we need to switch to desired weapon
	//if (forced_usage == USE_MAIN && (current_weapon.iId != main_weapon))					// NEW CODE 094 (prev code)
	BotSelectMainWeapon("WaitHere()");

	// else if (forced_usage == USE_BACKUP && (current_weapon.iId != backup_weapon))		// NEW CODE 094 (prev code)
	if ((forced_usage == USE_BACKUP) && (current_weapon.iId != backup_weapon))
		BotSelectBackupWeapon(this);
	else if ((forced_usage == USE_KNIFE) && (current_weapon.iId != fa_weapon_knife))
		BotSelectKnife(this);
}

/*
* sets time period for which the bot won't be checking for being stuck
* if no value is set then default 1.0s is used
* 0.0 means setting it to current game time
* and using -1.0 will set the value to 0.0
*/
void bot_t::SetDontCheckStuck(const char* loc, float time)
{
	// we can't allow setting it if the bot is obviously in problems
	// the bot must be able to deal with any obstacle that may block
	// going/resume from prone (e.g. someone else already laying there)
	if (IsSubTask(ST_CANTPRONE))
	{
		f_dont_check_stuck = (float)0.0;

		return;
	}

	if (time == -1.0)
		f_dont_check_stuck = (float) 0.0;
	else
		f_dont_check_stuck = gpGlobals->time + time;



#ifdef DEBUG

	if (loc != NULL)
	{
		static char last_csmsg[256] = "";

		// print it only once
		if (strcmp(loc, last_csmsg) != 0)
		{
			char msg[128];
			sprintf(msg, "SetDontCheckStuck() called @ %s\n", loc);
			HudNotify(msg, true, this);

			strcpy(last_csmsg, loc);
		}
	}
#endif // DEBUG


};


/*
* main bot method
* all ...well almost all is directed from this method
*/
void bot_t::BotThink()
{
	int index = 0;
	Vector v_diff;             // vector from previous to current location
	float pitch_degrees;
	float yaw_degrees;
	float moved_distance;      // length of v_diff vector (distance bot moved)
	TraceResult tr;
	bool found_waypoint;
	bool is_idle;
	float f_strafe_speed;		// handle strafe moves

	pEdict->v.flags |= FL_FAKECLIENT;

	if (name[0] == 0)  // name filled in yet?
		strcpy(name, STRING(pEdict->v.netname));


// TheFatal - START from Advanced Bot Framework (Thanks Rich!)

	// adjust the millisecond delay based on the frame rate interval...
	if (msecdel <= gpGlobals->time)
	{
		msecdel = gpGlobals->time + 0.5;
		if (msecnum > 0)
			msecval = 450.0/msecnum;
		msecnum = 0;
	}
	else
		msecnum++;

	if (msecval < 1)    // don't allow msec to be less than 1...
		msecval = 1;

	if (msecval > 100)  // ...or greater than 100
		msecval = 100;
// TheFatal - END

	pEdict->v.button = 0;
	move_speed = SPEED_NO;

	f_strafe_speed = 0;

	// if the bot hasn't selected stuff to start the game yet, go do that...
	if (not_started)
	{
		// check to know how many skills can be spawned
		if (internals.IsSkillSystemChecked() == false)
			BotCheckSkillSystem();

		BotStartGame();

		g_engfuncs.pfnRunPlayerMove( pEdict, pEdict->v.v_angle, 0.0,
									0, 0, pEdict->v.button, 0, msecval);

		return;
	}

/*
			NOT USED YET - NO FILE FOR IT
	if ((b_bot_say_killed) && (f_bot_say_killed < gpGlobals->time))
	{
		int whine_index;
		bool used;
		int i, recent_count;
		char msg[120];

		b_bot_say_killed = FALSE;

		recent_count = 0;

		while (recent_count < 5)
		{
			whine_index = RANDOM_LONG(0, whine_count-1);

			used = FALSE;

			for (i=0; i < 5; i++)
			{
				if (recent_bot_whine[i] == whine_index)
					used = TRUE;
			}

			if (used)
				recent_count++;
			else
				break;
		}

		for (i=4; i > 0; i--)
			recent_bot_whine[i] = recent_bot_whine[i-1];

		recent_bot_whine[0] = whine_index;

		if (strstr(bot_whine[whine_index], "%s") != NULL)  // is "%s" in whine text?
			sprintf(msg, bot_whine[whine_index], STRING(killer_edict->v.netname));
		else
			sprintf(msg, bot_whine[whine_index]);

		UTIL_HostSay(pEdict, 0, msg);
	}
*/

	// if the bot is dead or if the round ended (ie all flags were captured etc.)
	// randomly press fire (as well as key "1") to respawn with the same config
	if (((pEdict->v.health < 1) || (pEdict->v.deadflag != DEAD_NO)) || (round_end))
	{
		if (need_to_initialize)
		{
			BotSpawnInit();

			/*		NOT USED YET
			// did another player kill this bot AND bot whine messages loaded AND
			// has the bot been alive for at least 15 seconds
			if ((killer_edict != NULL) && (whine_count > 0) &&
				((bot_spawn_time + 15.0) <= gpGlobals->time))
			{
				if ((RANDOM_LONG(1,100) <= 10))
				{
					b_bot_say_killed = TRUE;
					f_bot_say_killed = gpGlobals->time + 10.0 + RANDOM_FLOAT(0.0, 5.0);
				}
			}
			*/

			need_to_initialize = FALSE;
		}

		// set this for next time the round ends
		if (round_end)
		{
			round_end = FALSE;


			//@@@@@@@@@@@@@TEMPORARY
			//ALERT(at_console, "(bot)new round respawn !!! ROUND ENDED !!!\n");
		}

		// by Andrey Kotrekhov
		BotPressFireToRespawn();

		return;
	}

	// don't do anything while frozen by the game engine
	if (pEdict->v.flags & FL_FROZEN)
	{
		g_engfuncs.pfnRunPlayerMove( pEdict, pEdict->v.v_angle, 0.0, 0, 0, pEdict->v.button, 0, msecval);

		return;
	}

	// we have to handle team setting in old FAs (2.5 and below) after team balancing was done
	// we can't do it inside balancing method, because the model is updated later after respawn
	if (UTIL_IsOldVersion() && (bot_team == 0))
	{
		// the bot is in red team
		// but the bot_team still says something else than red team
		if ((strstr(STRING(pEdict->v.model), "red1") != nullptr) &&
			(bot_team != TEAM_RED))
			bot_team = TEAM_RED;

		// the bot is in blue team
		// but the bot_team still says something else than blue team
		if ((strstr(STRING(pEdict->v.model), "blue1") != nullptr) &&
			(bot_team != TEAM_BLUE))
			bot_team = TEAM_BLUE;
	}

	// if the bot has been promoted then choose new skill
	if (IsTask(TASK_PROMOTED))
	{
		BotStartGame();

		// we won't keep checking new skill
		RemoveTask(TASK_PROMOTED);
	}

	// check if the bot can use MedicGuild tag
	CheckMedicGuildTag();

	// set this for the next time the bot dies so it will initialize stuff
	if (need_to_initialize == FALSE)
	{
		need_to_initialize = TRUE;
		bot_spawn_time = gpGlobals->time;
	}

	is_idle = FALSE;

	// is the bot blinded by something (concussion grenade for example)
	if (blinded_time > gpGlobals->time)
	{
		// if the bot is NOT proned then crouch
		if (!(pEdict->v.flags & FL_PRONED))
			pEdict->v.button |= IN_DUCK;

		is_idle = TRUE;  // don't do anything while blinded
	}

	if (is_idle)
	{
		// turn towards ideal_yaw by yaw_speed degrees (slower than normal)
		BotChangeYaw( this, pEdict->v.yaw_speed / 2 );

		g_engfuncs.pfnRunPlayerMove(pEdict, pEdict->v.v_angle, BotSetSpeed(), 0, 0, pEdict->v.button, 0, msecval);

		return;
	}
	else
	{
		idle_angle = pEdict->v.v_angle.y;
	}

	// check if time to check for player sounds (if don't already have enemy OR
	// if lost clear view for current enemy AND if NOT on ladder)
	// ignoreall completely skips this part -> we're ignoring everything
	if ((f_sound_update_time <= gpGlobals->time) && (botdebugger.IsIgnoreAll() == FALSE) && 
		((pBotEnemy == nullptr) || ((pBotEnemy) && (f_bot_wait_for_enemy_time >= gpGlobals->time))) &&
		(pEdict->v.movetype != MOVETYPE_FLY))
	{
		int ind;
		edict_t *pPlayer;

		f_sound_update_time = gpGlobals->time + 1.0;

		for (ind = 1; ind <= gpGlobals->maxClients; ind++)
		{
			pPlayer = INDEXENT(ind);

			// if this player slot is valid and it's not this bot
			if ((pPlayer) && (!pPlayer->free) && (pPlayer != pEdict))
			{
				// if observer mode enabled, don't listen to this player
				if (botdebugger.IsObserverMode() && !(pPlayer->v.flags & FL_FAKECLIENT))
					continue;

				if (IsAlive(pPlayer) && (FBitSet(pPlayer->v.flags, FL_CLIENT) ||
					FBitSet(pPlayer->v.flags, FL_FAKECLIENT)))
				{
					// check for sounds being made by other players
					if (UpdateSounds(pPlayer))
					{
						// don't check for sounds for another 1.5 seconds
						f_sound_update_time = gpGlobals->time + 1.5;

						// the bot needs some time to face the direction of possible enemy
						f_dont_look_for_waypoint_time = gpGlobals->time + 0.2;

						// kota@ no reason to listen all players if we can turn to only one
						break;
					}
				}
			}
		}
	}

	// set to max speed
	move_speed = SPEED_MAX;

	if (prev_time <= gpGlobals->time)
	{
		// see how far bot has moved since the previous position
		v_diff = v_prev_origin - pEdict->v.origin;
		moved_distance = v_diff.Length();

		// save current position as previous
		v_prev_origin = pEdict->v.origin;
		prev_time = gpGlobals->time + 0.2;
	}
	else
	{
		moved_distance = 2.0;
	}
	
	// turn towards ideal_pitch by pitch_speed degrees
	pitch_degrees = BotChangePitch( this, pEdict->v.pitch_speed );

	// turn towards ideal_yaw by yaw_speed degrees
	yaw_degrees = BotChangeYaw( this, pEdict->v.yaw_speed );

	// do any speed adjustments only if bot is NOT proned or crouched (ie bot is moving fast) or NOT evading mine
	//if (!(pEdict->v.flags & (FL_PRONED | FL_DUCKING)))														NEW CODE 094 (prev code)
	if (!(pEdict->v.flags & (FL_PRONED | FL_DUCKING)) && !IsTask(TASK_CLAY_EVADE))//							NEW CODE 094
	{
		// slow down if pitch is positive ie looking down (for example going down the hill)
		// and not sprinting otherwise don't slow down

		// don't move while turning a lot
		if ((yaw_degrees >= 50) || ((pitch_degrees >= 50) && (pEdict->v.v_angle.x > 0)))
		{
			move_speed = SPEED_NO;
			// also don't try to run unstuck code																NEW CODE 094
			// 0.2 is duration of one movement check
			// so we will double this value for next stuck check
			SetDontCheckStuck("bot think()|adjust speed due to turning alot -> speed no", 0.4);
		}
		// slow down if we aren't turning that much
		else if ((yaw_degrees >= 25) ||
			((pitch_degrees >= 25) && (pEdict->v.v_angle.x > 0) && !IsTask(TASK_SPRINT)))//						AND not task sprint is the NEW CODE 094
		{
			move_speed = SPEED_SLOWEST;
		}
		// if we are turning only a bit use half of current speed
		else if ((yaw_degrees >= 10) ||
			((pitch_degrees >= 10) && (pEdict->v.v_angle.x > 0) && !IsTask(TASK_SPRINT)))
		{
			move_speed = SPEED_SLOW;
		}
	}

#ifdef _DEBUG
	if (g_debug_bot_on && (g_debug_bot == pEdict))
	{
		/*/
		char msg[80];
		
		if (IsSubTask(ST_FACEENEMY))
		{
			sprintf(msg, "This bot is facing enemy/possible enemy at yaw speed of %.2f\n", yaw_degrees);
			HudNotify(msg, this);
		}
		/**/
	}
#endif

	// before we start any actions we must find out in which stance the bot currently is...
	if (f_duckjump_time >= gpGlobals->time)
	{
		// don't change current stance while the bot is in duckjump
		;





#ifdef DEBUG
		//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		if (botdebugger.IsDebugStuck() || botdebugger.IsDebugActions())
		{
			char djmsg[128];
			sprintf(djmsg, "I'm doing DUCKJUMP right NOW!\n");
			HudNotify(djmsg, this);

			if (f_duckjump_time == gpGlobals->time)
				HudNotify("DUCKJUMP ends in this frame!\n", this);
		}
		//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#endif // DEBUG




	}
	else
	{
		if (pEdict->v.flags & FL_PRONED)
		{
			// clear all flags because there can only be one correct stance at a time
			UTIL_ClearBit(BOT_PRONED, bot_behaviour);
			UTIL_ClearBit(BOT_CROUCHED, bot_behaviour);
			UTIL_ClearBit(BOT_STANDING, bot_behaviour);

			// and set it for this whole frame
			SetBehaviour(BOT_PRONED);

			// we must check for case when bot is proned, called 'prone' command in last frame, but engine doesn't allow him
			// to stand up, because someone or something is blocking him (e.g. another bot standing on him at wait-timed wpt)
			if (IsBehaviour(GOTO_STANDING) && (f_go_prone_time + 0.6f < gpGlobals->time))//			was 0.4
			{
				if (moved_distance < 2.0)
				{
					// just set this bit and the clearing section at the bottom of BotThink() will fix it
					SetSubTask(ST_CANTPRONE);


#ifdef DEBUG
					//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@														// NEW CODE 094 (remove it)
					char dm[256];
					sprintf(dm, "%s SET CANTPRONE subtask @ BotThink()|INIT stance sect -> moved_dist < 2.0 (moved_dist=%.2f)\n",
						name, moved_distance);
					HudNotify(dm, true, this);
					//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#endif // DEBUG
				}
				else if (moved_distance == 2.0)
				{
					;		// do nothing
				}
				else
				{





#ifdef DEBUG
					//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@	 													// NEW CODE 094 (remove it)
					char dm[256];
					sprintf(dm, "%s GOTO_STANDING and BOT_PRONED @ INIT stance sect (moved_dist=%.2f) !! SEE WHEN THIS HAPPENED\n",
						name, moved_distance);
					HudNotify(dm, true, this);
					//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#endif // DEBUG



				}
			}
		}
		else if (pEdict->v.flags & FL_DUCKING)
		{
			UTIL_ClearBit(BOT_PRONED, bot_behaviour);
			UTIL_ClearBit(BOT_CROUCHED, bot_behaviour);
			UTIL_ClearBit(BOT_STANDING, bot_behaviour);

			SetBehaviour(BOT_CROUCHED);
		}
		// if NOT going to/resume from prone then the bot is standing
		else if (IsGoingProne() == FALSE)
		{
			UTIL_ClearBit(BOT_PRONED, bot_behaviour);
			UTIL_ClearBit(BOT_CROUCHED, bot_behaviour);
			UTIL_ClearBit(BOT_STANDING, bot_behaviour);

			SetBehaviour(BOT_STANDING);
		}
		// bot can only go/resume from prone now
		// (this statement also gets called when the bot tried to go prone, but engine didn't allow it,
		// for example due to being to close to wall or other player)
		// going prone seems to be almost instant, but resume from prone takes several game frames so...
		else
		{
			// don't check for stuck for a short while to prevent bot running false unstuck codes
			// (the bot would have most probably started to strafe)
			if (IsSubTask(ST_CANTPRONE) == false)
			{
				SetDontCheckStuck("bot think()|INIT stance sect -> going/resume from prone (set dont check for stuck to 0.4s)", 0.4);
			}
			else
			{



#ifdef DEBUG
				//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
				if (botdebugger.IsDebugStuck() || botdebugger.IsDebugActions())
					HudNotify("Am I really going to/resume from prone RIGHT NOW???\n", this);
				//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#endif // DEBUG


			}
		}
	}

	// is the bot frozen?
	if (botdebugger.IsFreezeMode())
	{
		// update current waypoint time to prevent false waypoint lost/stuck
		// ie bot doesn't start to move to next waypoint
		f_waypoint_time = gpGlobals->time;
		// prevent search for new waypoint
		f_dont_look_for_waypoint_time = gpGlobals->time;

		// don't move
		move_speed = SPEED_NO;
		// prevents false running of unstuck code
		SetDontCheckStuck();
		//SetDontCheckStuck("bot think()->freeze mode enabled");

		// is the wait time near its end then increase it
		if (wpt_wait_time == gpGlobals->time)
			wpt_wait_time = gpGlobals->time + 0.1;

		// is the action time near its end then increase it
		if (wpt_action_time == gpGlobals->time)
			wpt_action_time = gpGlobals->time + 0.1;
	}

	// else handle movement related actions
	else
	{
		// is it time to check for placed claymores and other ground items yet?
		if (f_look_for_ground_items <= gpGlobals->time)
		{
			// set time for the next check
			f_look_for_ground_items = gpGlobals->time + 0.5;

			BotFindItem(this);
		}

		if (botdebugger.IsIgnoreAll() == FALSE)
		{
			// did the bot already plant claymore mine
			if (clay_action == ALTW_GOALPLACED)
			{
				if (f_bot_find_enemy_time <= gpGlobals->time)
				{
					f_bot_find_enemy_time = gpGlobals->time + 5.0;

					if (RANDOM_LONG(1, 100) <= 45)
						pBotEnemy = BotFindEnemy();
					else
						pBotEnemy = nullptr;

					// found the bot an enemy
					if (pBotEnemy != nullptr)
					{
						// in most of the time detonate the mine even if bot isn't in safe distance
						if (RANDOM_LONG(1, 100) <= 75)
						{
							// pressing it detonates the claymore
							pEdict->v.button |= IN_ATTACK;

							// bot already used the mine
							clay_action = ALTW_USED;
						}
					}
				}
			}
			else
			{
				pBotEnemy = BotFindEnemy();
			}
		}
		else
		{
			pBotEnemy = nullptr;  // clear enemy pointer (no ememy for you!)
		}

		// has the bot NO enemy
		// this must be before the combat statement, it's special "clearing" section
		if (pBotEnemy == nullptr)
		{
			// does the bot use any sniper rifle AND has scope on AND NOT at shoot waypoint
			if ((pEdict->v.fov != NO_ZOOM) && (f_shoot_time < gpGlobals->time) && (pItem == nullptr) &&
				(IsWaypoint(curr_wpt_index, WptT::wpt_fire) == FALSE))//									NEW CODE 094 (bug fix)
			{
				if (UTIL_IsSniperRifle(current_weapon.iId))
				{
					pEdict->v.button |= IN_ATTACK2;		// scope off
					//f_shoot_time = gpGlobals->time + 1.0;	// time to take effect							NEW CODE 094 (prev code)
					f_shoot_time = gpGlobals->time + 0.6;	// time to take effect

					if (botdebugger.IsDebugActions())
					{
						HudNotify("Sniper rifle zoom out (no target or after battle)\n", this);
					}
				}
			}

			// if the bot was on PATROL path so he should return back
			if ((patrol_path_wpt != -1) && IsTask(TASK_BACKTOPATROL) &&
				(f_bot_see_enemy_time > 0) && (f_bot_see_enemy_time + 0.1 <= gpGlobals->time))
			{
				// run this only once
				RemoveTask(TASK_BACKTOPATROL);

				// returns bot on PATROL path (if last PATROL path waypoint is reachable)
				if (WaypointFindLastPatrol(this))
				{
					if (botdebugger.IsDebugPaths())
						HudNotify("PATROL path return flag cleared (Bot gets back to patrol)\n", this);
				}
			}
		}

		// used the bot a grenade recently so check if he still has any grenade left
		if ((grenade_action != ALTW_USED) && (grenade_time + 1.0 > gpGlobals->time) && (grenade_slot != NO_VAL) &&
			(!(bot_weapons & (1<<grenade_slot))))
		{
			grenade_action = ALTW_USED;
		}

		// look if the bot is doing bandage treatment
		// this statement must be before battle actions, because
		// FA doesn't allow combat while bandaging (it even hides the weapon)
		if (bandage_time > gpGlobals->time)
		{
			f_waypoint_time = gpGlobals->time;
			f_dont_look_for_waypoint_time = gpGlobals->time;

			move_speed = SPEED_NO;
			SetDontCheckStuck("bot think() -> bandage time");

			// if NOT proned or currently doing so then crouch for cover
			if ((IsBehaviour(BOT_PRONED) == FALSE) && !IsGoingProne())
				SetStance(this, GOTO_CROUCH, "BotThink()|BandageTime -> GOTO crouch");
		}

		// is the bot paused (being under medical treatment from teammate or he's merging magazines)
		// AND it is NOT a bot medic trying to give a treatment to someone else
		// this also must be before combat actions, because there's no weapon to shoot from
		else if ((f_pause_time >= gpGlobals->time) && !IsTask(TASK_HEALHIM))
		{
			f_waypoint_time = gpGlobals->time;
			f_dont_look_for_waypoint_time = gpGlobals->time;

			move_speed = SPEED_NO;
			SetDontCheckStuck("bot think() -> pause time");

			// if NOT proned or currently doing so then crouch for cover
			if ((IsBehaviour(BOT_PRONED) == FALSE) && !IsGoingProne())
				SetStance(this, GOTO_CROUCH, "BotThink()|PauseTime -> GOTO crouch");
		}

		// does an enemy exist?
		else if (pBotEnemy != nullptr)
		{
			// should the bot heal one of teammates
			if (IsTask(TASK_HEALHIM))
			{
				// handle a bot medic trying to treat downed teammate (must be first)
				if (IsTask(TASK_MEDEVAC))
				{
					float distance = 0;

					if (BotMedicGetToCorpse(this, distance))
					{
						BotDoMedEvac(distance);
					}
					// can't get to it
					else
					{
						RemoveTask(TASK_HEALHIM);
						pBotEnemy = nullptr;

						RemoveTask(TASK_MEDEVAC);
						RemoveSubTask(ST_MEDEVAC_DONE);
						RemoveSubTask(ST_MEDEVAC_ST);
						RemoveSubTask(ST_MEDEVAC_H);
						RemoveSubTask(ST_MEDEVAC_F);
					}
				}
				// is that player okay
				else if (pBotEnemy->v.health >= pBotEnemy->v.max_health)
				{
					RemoveTask(TASK_HEALHIM);
					pBotEnemy = nullptr;
				}
				else
				{
					// needs this teammate still medical help AND can the bot see his patient
					if (UTIL_PatientNeedsTreatment(pBotEnemy) &&
						BotMedicGetToPatient(this))
					{
						if (BotHealTeammate(this) == FALSE)
						{
							// player is healed, null the pointer
							pBotEnemy = nullptr;							
							// stop healing behaviour
							RemoveTask(TASK_HEALHIM);
							// remove the healed flag
							RemoveSubTask(ST_HEALED);
						}
					}
					// bot can't get to patient so forget on him
					else
					{
						RemoveTask(TASK_HEALHIM);
						pBotEnemy = nullptr;
					}
				}
			}
			/*/																								// NEW CODE 094
			// or is the bot in mid-air, has chute and didn't use it yet
			else if (BotCheckMidAir(pEdict) && IsTask(TASK_PARACHUTE) &&
				!IsTask(TASK_PARACHUTE_USED) && (chute_action_time <= gpGlobals->time))
			{
				// set some time to check if bot is really in mid-air
				// ie to prevent opening chute when the bot just steps down or
				// jumps from some low object
				if (chute_action_time < 1.0)
				{
					chute_action_time = gpGlobals->time + 0.7;
				}
				// bot must be really in mid-air so use the chute
				if ((chute_action_time > 1.0) && (chute_action_time < gpGlobals->time))
				{
					pEdict->v.button |= IN_USE;
					SetTask(TASK_PARACHUTE_USED);
				}
			}
			/**/
			// or shoot at the enemy
			else
			{
				BotShootAtEnemy( this );
			}

			// update waypoint time if the bot has some waypoint
			if (curr_wpt_index != -1)
				f_waypoint_time = gpGlobals->time;
		}

		// the bot doing medical treatment to teammate or medevac
		else if (f_medic_treat_time >= gpGlobals->time)
		{
			f_waypoint_time = gpGlobals->time;
			f_dont_look_for_waypoint_time = gpGlobals->time;

			move_speed = SPEED_NO;
			SetDontCheckStuck("bot think() -> medical treatment time or medevac");
		}

		// is the bot in mid-air AND parachuting
		else if (IsTask(TASK_PARACHUTE_USED) && BotCheckMidAir(pEdict))
		{
			f_dont_look_for_waypoint_time = gpGlobals->time;

			// if the bot has no waypoint to head towards to then don't move
			// (otherwise the bot will try to land at his waypoint)
			if (curr_wpt_index == -1)
				move_speed = SPEED_NO;

			SetDontCheckStuck("bot think() -> parachuting - in air");

			// keep turning towards the waypoint
			Vector v_direction = wpt_origin - pEdict->v.origin;			
			Vector v_angles = UTIL_VecToAngles(v_direction);
			
			pEdict->v.idealpitch = -v_angles.x;
			BotFixIdealPitch(pEdict);

			pEdict->v.ideal_yaw = v_angles.y;
			BotFixIdealYaw(pEdict);
		}

		// is the bot being "used" and can still follow "user"?
		else if (pBotUser != nullptr)
		{
			if (BotFollowTeammate(this) == TRUE)
			{
				edict_t *pMaster = pBotUser;

				// is master crouched go crouch as well
				if (pMaster->v.flags & FL_DUCKING)
					SetStance(this, GOTO_CROUCH,"BotThink()|FollowingMaster -> Matching master's crouch stance");

				// is master proned AND bot isn't so go prone 
				if ((pMaster->v.flags & FL_PRONED) && (IsBehaviour(BOT_PRONED) == FALSE))
					UTIL_GoProne(this, "BotThink()|FollowingMaster -> Matching master's prone stance");

				// is bot proned but master isn't so stand up
				if (IsBehaviour(BOT_PRONED) && !(pMaster->v.flags & FL_PRONED))
					UTIL_GoProne(this, "BotThink()|FollowingMaster -> Matching master's standing stance");

				// has bot set this flag during waypoint navigation so clear it				NEW CODE 094 (prev code)
				//if (IsTask(TASK_GOPRONE))
				//	RemoveTask(TASK_GOPRONE);

				// got the bot on ladder while following his master
				if (pEdict->v.movetype == MOVETYPE_FLY)
				{
					if (ladder_dir == LADDER_UNKNOWN)
					{
						// master is under the bot so climb down
						if (pMaster->v.origin.z < pEdict->v.origin.z)
							ladder_dir = LADDER_DOWN;
						// otherwise climb up
						else
							ladder_dir = LADDER_UP;
					}

					if (ladder_dir == LADDER_UP)
					{
						pEdict->v.v_angle.x = -80;	// look upwards
						pEdict->v.button |= IN_FORWARD;	// and move forward
					}
					else if (ladder_dir == LADDER_DOWN)
					{
						pEdict->v.v_angle.x = 80;	// look downwards
						pEdict->v.button |= IN_FORWARD;	// and move forward
					}
				}
			}
			
			// reset current waypoint and path
			// prevents unwanted behaviour based on bot's current waypoint and
			// allows the bot to continue right from his actual position
			curr_wpt_index = -1;
			curr_path_index = -1;
		}

		// is the bot using bipod while not being in combat
		else if (IsTask(TASK_BIPOD))
		{
			// stop using bipod if snipe time is over
			// this one handles only sudden firefights when the bot isn't at camper spot (i.e. NOT at wait-timed waypoint)
			if ((wpt_wait_time < gpGlobals->time) && (sniping_time + 3.5 < gpGlobals->time))
			{
				BotUseBipod(this, FALSE, "BotThink()|TaskBipod -> battle SnipingTime is over so FOLD IT");
			}
			// did the bot ran out of ammo
			else if (main_no_ammo && (main_weapon != NO_VAL))
			{
				// stop using bipod immediately
				BotUseBipod(this, TRUE, "BotThink()|TaskBipod -> NOAMMO so FOLD IT");
			}
			// does the bot need to target another aim waypoint while waiting at wait-timed wpt...
			else if ((f_aim_at_target_time < gpGlobals->time) && (wpt_wait_time > gpGlobals->time))
			{
				// stop using bipod immediately
				BotUseBipod(this, TRUE, "BotThink()|TaskBipod -> Need to FACE new AIM wpt so FOLD IT");
			}
			// if the wait time is about to end we must fold bipod so the bot can leave the waypoint
			else if ((wpt_wait_time > gpGlobals->time) &&
				(gpGlobals->time + SetBipodHandlingTime(current_weapon.iId, FALSE) > wpt_wait_time))
			{
				// stop using bipod immediately
				BotUseBipod(this, TRUE, "BotThink()|TaskBipod -> WAIT time is about to end so FOLD IT");
			}
			
			// otherwise keep using it
			f_waypoint_time = gpGlobals->time;
			f_dont_look_for_waypoint_time = gpGlobals->time;
			move_speed = SPEED_NO;
			SetDontCheckStuck("bot think()|TaskBipod -> keep using it");
		}

		// is the bot in process of deploying or folding the bipod...
		else if (f_bipod_time >= gpGlobals->time)
		{
			// then just continue updating these to prevent calling "I must be stuck" code
			f_waypoint_time = gpGlobals->time;
			f_dont_look_for_waypoint_time = gpGlobals->time;

			move_speed = SPEED_NO;
			SetDontCheckStuck("bot think() -> deploying or folding bipod");
		}

		// is the bot forced to turn to aim waypoint or turn to some item stored in pItem
		else if ((IsSubTask(ST_AIM_FACEIT)) || (IsTask(TASK_IGNOREAIMS)))
		{
			f_waypoint_time = gpGlobals->time;
			f_dont_look_for_waypoint_time = gpGlobals->time;
			
			move_speed = SPEED_NO;
			SetDontCheckStuck("bot think() -> facing pItem or AimWpt");

			if (IsTask(TASK_IGNOREAIMS))
			{
				FacepItem();
			}
			else
				TargetAimWaypoint();
		}

		// is the bot in process of reloading his current weapon?
		// we need this statement here to prevent calling any other movement based actions that are below this one
		// where the bot can eventually use his weapon
		else if (f_reload_time >= gpGlobals->time)
		{
			;	// we do nothing here, because everything is handled below ... inside weapon action == inreload section
		}

		// is the bot forced to use primary fire outside combat
		else if (IsTask(TASK_FIRE))
		{
			if (IsSubTask(ST_AIM_DONE))
			{
				f_waypoint_time = gpGlobals->time;
				f_dont_look_for_waypoint_time = gpGlobals->time;
			
				move_speed = SPEED_NO;
				SetDontCheckStuck("bot think() -> TaskFire");

				if ((IsGoingProne() == FALSE) && (weapon_action == W_READY))
				{
					// if the waypoint has highest priority then just shoot
					// waypoint creator set it up so do it (could be just for the fun)
					if (WaypointGetPriority(curr_wpt_index, bot_team) == 1)
					{
						pEdict->v.button |= IN_ATTACK;
						f_shoot_time = gpGlobals->time;
					}
					// otherwise...
					else
					{
						// check if there's a breakable object in front of the bot
						if (UTIL_CheckForwardForBreakable(pEdict))
						{
							// if so then shoot at it
							pEdict->v.button |= IN_ATTACK;
							f_shoot_time = gpGlobals->time;
						}
					}

					// and reset all tasks and substasks once we are done here
					RemoveTask(TASK_FIRE);
					RemoveSubTask(ST_AIM_DONE);
					RemoveTask(TASK_PRECISEAIM);
				}
			}
			else
				SetSubTask(ST_AIM_FACEIT);
		}

		// is the bot forced to kill breakable object
		else if (IsTask(TASK_BREAKIT))
		{
			edict_t *pBreakable = UTIL_FindEntityInSphere(pEdict, "func_breakable");

			// no breakable object around, remove this task
			if ((pBreakable == nullptr) || (pBreakable->v.health <= 0) || (pBreakable->v.spawnflags & SF_BREAK_TRIGGER_ONLY))
			{
				RemoveTask(TASK_BREAKIT);
				SetDontCheckStuck("bot think()|TaskBreakIt -> no breakable object around", 0.0);
			}
			// otherwise break it
			else
			{
				f_waypoint_time = gpGlobals->time;
				f_dont_look_for_waypoint_time = gpGlobals->time;

				move_speed = SPEED_NO;
				SetDontCheckStuck("bot think()|TaskBreakIt -> going to break it");

				// face it
				Vector entity_origin = VecBModelOrigin(pBreakable);
				Vector entity = entity_origin - pEdict->v.origin;
				Vector bot_angles = UTIL_VecToAngles(entity);
				pEdict->v.idealpitch = -bot_angles.x;
				BotFixIdealPitch(pEdict);
				pEdict->v.ideal_yaw = bot_angles.y;
				BotFixIdealYaw(pEdict);

				// check if weapon is ready																		 NEW CODE 094
				if ((weapon_action == W_READY) && (f_shoot_time < gpGlobals->time) &&
					(IsGoingProne() == FALSE))
				{
					// check if the weapon is empty
					if ((current_weapon.iClip == 0) && (current_weapon.iAmmo1 != 0))
					{
						ReloadWeapon("BotThink()|TaskBreakIt -> empty weapon");
					}

					// otherwise shoot it
					else
					{
						if (current_weapon.iFireMode != 4)
						{
							f_shoot_time = gpGlobals->time + RANDOM_FLOAT(0.3, 0.6);
							pEdict->v.button |= IN_ATTACK;
						}
						else if (current_weapon.iFireMode == 4)
							pEdict->v.button |= IN_ATTACK;
					}
				}
			}
		}

		// is the bot doing waypoint action
		else if ((wpt_action_time > gpGlobals->time) && (bot_spawn_time + 0.5 < gpGlobals->time))
		{
			bool do_action;

			f_waypoint_time = gpGlobals->time;
			f_dont_look_for_waypoint_time = gpGlobals->time;

			move_speed = SPEED_NO;
			SetDontCheckStuck("bot think() -> wpt action time");

			do_action = false;

			// don't have any item yet so see which entities are around
			if (pItem == nullptr)
			{
				if (botdebugger.IsDebugActions())
					HudNotify("Bot has no item yet. Going to check for it now...\n", this);

				do_action = UTIL_CheckForUsablesAround(this);

				// nothing useful has been found so
				// press the "use" just in case and clear action time to prevent false behavior
				if (do_action == false)
				{
					pEdict->v.button |= IN_USE;
					wpt_action_time = gpGlobals->time - 0.1;

					if (botdebugger.IsDebugActions())
						HudNotify("NO item has been found, but hitting the use key anyway and leaving this waypoint!\n", this);
				}
				//																			NEW CODE 094
				// otherwise we found something usable so we'll set this task to make the bot face the item/entity
				else
				{
					ResetAims("BotThink() -> WptActionTime");
					SetTask(TASK_IGNOREAIMS);
				}
			}/*/																			NEW CODE 094 (prev code)
			// keep turning to the item
			else if (f_face_item_time > gpGlobals->time)
			{
				// handle eventual crash bug
				if (pItem == NULL)
				{
					// log this problem
					char msg[256];
					sprintf(msg, "BotThink() | Wpt_Action_Time | facing item | Item pointer is NULL | on map %s (wpt=%d | path=%d)\n",
						STRING(gpGlobals->mapname), curr_wpt_index + 1, curr_path_index + 1);
					UTIL_DebugInFile(msg);

					// clear wait time
					wpt_action_time = gpGlobals->time - 0.1;

					// finish bot move
					g_engfuncs.pfnRunPlayerMove(pEdict, pEdict->v.v_angle, BotSetSpeed(), 0, 0,
						pEdict->v.button, 0, msecval);

					// and end this method
					return;
				}

				char item_name[64];
				Vector entity;

				strcpy(item_name, STRING(pItem->v.classname));

				if ((strcmp("ammobox", item_name) == 0) || (strcmp("momentary_rot_button", item_name) == 0))
				{
					entity = pItem->v.origin - pEdict->v.origin;
				}
				// it is a BModel type entity
				else
				{
					Vector entity_origin = VecBModelOrigin(pItem);

					entity = entity_origin - pEdict->v.origin;
				}

				Vector bot_angles = UTIL_VecToAngles(entity);

				// face the entity
				pEdict->v.ideal_yaw = bot_angles.y;
				BotFixIdealYaw(pEdict);
			}
			// bot must be already turned to item so use it
			else/**/
			// bot must be already turned to the entity/item so use it
			if (IsSubTask(ST_FACEITEM_DONE))
			{
				// handle eventual crash bug
				if (pItem == nullptr)
				{
					// log it
					char msg[256];
					sprintf(msg, "BotThink() | Wpt_Action_Time | using item | Item pointer is NULL | on map %s (wpt=%d | path=%d)\n",
						STRING(gpGlobals->mapname), curr_wpt_index + 1, curr_path_index + 1);
					UTIL_DebugInFile(msg);

					// clear wait time
					wpt_action_time = gpGlobals->time - 0.1;

					// finish bot move
					g_engfuncs.pfnRunPlayerMove(pEdict, pEdict->v.v_angle, BotSetSpeed(), 0, 0,
						pEdict->v.button, 0, msecval);

					// and end this method
					return;
				}

				char item_name[64];
				strcpy(item_name, STRING(pItem->v.classname));

				// keep the "use" button down for these entities
				if ((strcmp("ammobox", item_name) == 0) || (strcmp("momentary_rot_button", item_name) == 0))
				{
					pEdict->v.button |= IN_USE;
					SetSubTask(ST_BUTTON_USED);
				}
				// otherwise press the "use" only once (don't use it if already behind TANK)
				else if (!IsSubTask(ST_BUTTON_USED) && !IsTask(TASK_USETANK))
				{
					pEdict->v.button |= IN_USE;
					SetSubTask(ST_BUTTON_USED);

					// button will be pressed so we must break the "stop" time in order to return the bot back to action
					// unless there's a 'wait' time on this waypoint to force the bot stay there for a moment
					// (e.g. to let the doors fully open before moving forward)
					if (WaypointGetWaitTime(curr_wpt_index, bot_team) == 0.0)
						wpt_action_time = gpGlobals->time;

					if (botdebugger.IsDebugActions())
						HudNotify("button used only once\n");


					//@@@@@@@@@@@@@@@
#ifdef _DEBUG
					char dm[256];
					sprintf(dm, "BotThink()|WptActionTime -> USE button PRESSED at wpt=%d\n", curr_wpt_index + 1);
					HudNotify(dm, this);
#endif


				}

				// check if bot found mounted gun and should use it
				if (!IsTask(TASK_USETANK))
				{
					if ((pEdict->v.viewmodel == 0) && (pEdict->v.health > 0))
						SetTask(TASK_USETANK);	// bot is now using mounted gun
					else
						RemoveTask(TASK_USETANK);

					if (IsTask(TASK_USETANK))
					{
						// find any aim waypoints around
						WaypointFindNearestAiming(this, &waypoints[curr_wpt_index].origin);

						if (botdebugger.IsDebugActions())
							HudNotify("(bot.cpp)using TANK\n");
					}

				}
			}
		} // END waypoint action time

		// is it waypoint wait time so stop at current waypoint and wait
		else if (wpt_wait_time > gpGlobals->time)
		{
			BotWaitHere();

			// if we use aim waypoints then ...
			if (!IsTask(TASK_IGNOREAIMS) && !IsSubTask(ST_FACEITEM_DONE))
			{
				// look directly forward
				// (ie ignore pitch angles obtained from vector to aim waypoint conversion)
				pEdict->v.idealpitch = 0;
				BotFixIdealPitch(pEdict);
			}
		}

		else
		{
			// no enemy, let's just wander around


			// is it short after bot spawned/respawned?
			if ((gpGlobals->time > bot_spawn_time + 1.0) && (gpGlobals->time < bot_spawn_time + 2.0))
			{				
				// is the bot NOT holding any of "single firemode" weapons
				// so change the firemode to semi (single shot)
				if ((current_weapon.iFireMode != FM_NONE) && (current_weapon.iFireMode != FM_SEMI))
				{
					UTIL_ChangeFireMode(this, FM_SEMI, FireMode_WTest::test_weapons);
				}
			}

			// give the mod some time to finish all the spawning/respawning stuff
			// before going to check for following things
			if (bot_spawn_time + 2.5 < gpGlobals->time)
			{
				// then check if the bot can use silencer for his current weapon
				if (IsWeaponStatus(WS_ALLOWSILENCER) && !IsWeaponStatus(WS_SILENCERUSED) &&
					UTIL_IsStealth(pEdict) && UTIL_CanMountSilencer(this))
				{
					// tell the bot to mount the silencer as soon as possible
					SetWeaponStatus(WS_MOUNTSILENCER);
				}

				// check if the bot can start to mount the silencer
				// weapon must be ready we can't mount silencer during
				// reloading or changing weapons
				if (IsWeaponStatus(WS_MOUNTSILENCER) && !IsWeaponStatus(WS_SILENCERUSED) && (weapon_action == W_READY))
				{
					if (UTIL_IsStealth(pEdict) && UTIL_CanMountSilencer(this))
					{
						// to know that we already start to mount the silencer
						RemoveWeaponStatus(WS_MOUNTSILENCER);
						SetWeaponStatus(WS_SILENCERUSED);
						
						pEdict->v.button |= IN_ATTACK2;
					}
					else
					{
						// remove this flag to prevent false behaviour
						RemoveWeaponStatus(WS_MOUNTSILENCER);
					}
				}

				// if the bot didn't set this need or didn't refused it yet then do it
				if (!IsNeed(NEED_GOAL) && !IsNeed(NEED_GOAL_NOT))
				{
					int chance = RANDOM_LONG(1, 100);
					
					// the goal need is based on behaviour and generated chance
					if (IsBehaviour(DEFENDER) && (chance > 45))
						SetNeed(NEED_GOAL);
					else if (IsBehaviour(STANDARD) && (chance > 66))
						SetNeed(NEED_GOAL);
					else if (IsBehaviour(ATTACKER) && (chance > 80))
						SetNeed(NEED_GOAL);
					// otherwise this bot isn't in mood for reaching map goals
					else
						SetNeed(NEED_GOAL_NOT);
					
					if (g_debug_bot_on && (g_debug_bot == pEdict))
					{
						if (IsNeed(NEED_GOAL))
							HudNotify("***the bot decided to go for map objectives\n", this);
						else if (IsNeed(NEED_GOAL_NOT))
							HudNotify("***the bot decided NOT to go for map objectives\n", this);
					}
				}
			}

			// is bot NOT under water?
			if ((pEdict->v.waterlevel != 2) && (pEdict->v.waterlevel != 3))
			{
				// reset pitch to 0 (level horizontally)
				pEdict->v.idealpitch = 0;
				pEdict->v.v_angle.x = 0;
			}

			// is some time after we've reloaded the weapon so it's time to check ammo reserves
			if ((f_reload_time < gpGlobals->time) && (f_reload_time + 1.0f > gpGlobals->time) &&
				!IsTask(TASK_CHECKAMMO) && (check_ammunition_time + 2.0f < gpGlobals->time))
			{
				SetTask(TASK_CHECKAMMO);

				if (botdebugger.IsDebugActions())
					HudNotify("Checking ammunition after weapon reload\n", this);
			}

			// or is some time after last ammo reserves checking
			// we must set this task just once, if the bot already has it then
			// he'll do the check in next free moment
			// (ie. this cannot be run several frames in row otherwise the standard navigation
			// would NOT be called at all and the bot would move strange and/or get stuck)
			else if (!IsTask(TASK_CHECKAMMO) && (check_ammunition_time + 25.0f < gpGlobals->time))
			{
				SetTask(TASK_CHECKAMMO);

				if (botdebugger.IsDebugActions())
					HudNotify("Regular ammunition check is needed\n", this);
			}

			// the bot must first deal with partly used magazines before
			// going to reload his current weapon so
			// is now a good moment to merge magazines?
			// (i.e. NOT doing any weapon based action, NOT going prone AND NOT reloading)
			else if (IsWeaponStatus(WS_MERGEMAGS2) && (weapon_action == W_READY) &&
				(IsGoingProne() == FALSE) && (f_reload_time < gpGlobals->time))												// was (f_reload_time + 1.0 < gpGlobals->time))
			{
				BotMergeClips(this, "BotThink->try some navig");
			}

			// reload second m11 in FA25
			else if ((current_weapon.iId == fa_weapon_m11) && (current_weapon.iAttachment == 0) &&
				(current_weapon.iAmmo2 > 0) && (pBotEnemy == nullptr) && (f_shoot_time < gpGlobals->time) &&
				(f_reload_time < gpGlobals->time) && (bot_spawn_time + 5.0f < gpGlobals->time) &&
				(wpt_action_time < gpGlobals->time))
			{

				//																											 NEW CODE 094
				ReloadWeapon("BotThink() -> NO enemy -> 2nd m11 is empty");

				if (botdebugger.IsDebugActions())
					HudNotify("Going to reload current weapon\n", this);
			}

			// reload weapon if clip is empty OR not almost full in case of benelli/remington AND
			// have magazine AND no enemy AND NOT going to/resume from prone
			// AND NOT currently firing AND NOT currenly reloading
			// AND some time after spawn AND NOT doing any waypoint based action

			//if ((pBotEnemy == NULL) && (IsGoingProne() == FALSE) && (f_shoot_time < gpGlobals->time) &&					// orig code
			//	(f_reload_time < gpGlobals->time) && (bot_spawn_time + 5.0 < gpGlobals->time) &&
			//	(wpt_action_time < gpGlobals->time) && UTIL_ShouldReload(this, "BotThink() -> NO enemy"))

			else if ((IsGoingProne() == FALSE) && (f_shoot_time < gpGlobals->time) &&
				(f_reload_time < gpGlobals->time) && (bot_spawn_time + 5.0f < gpGlobals->time) &&
				UTIL_ShouldReload(this, "BotThink() -> NO enemy"))
			{
				ReloadWeapon("BotThink() -> NO enemy");

				if (botdebugger.IsDebugActions())
					HudNotify("Going to reload current weapon\n", this);
			}

			// otherwise try some navigation
			else
			{
				// is main weapon empty and not already set empty flag so set it now
				if (CheckMainWeaponOutOfAmmo("BotThink() -> NO enemy & navigation"))
					;
				// otherwise try to use main weapon at all cost
				//else if ((forced_usage != USE_MAIN) ||									// NEW CODE 094 (prev code)
				//	((forced_usage == USE_MAIN) && (current_weapon.iId != main_weapon)))
				else																		// NEW CODE 094
				{
					BotSelectMainWeapon("BotThink() -> NO enemy & navigation");
				}
				
				// is backup weapon empty and not already set empty flag so set it
				if (CheckBackupWeaponOutOfAmmo("BotThink() -> NO enemy & navigation"))
					;
				// otherwise check if the bot couldn't use main weapon and
				// if main weapon is unavailable then try to use backup
				else if ((main_no_ammo && (forced_usage != USE_BACKUP)) ||
					((forced_usage == USE_BACKUP) && (current_weapon.iId != backup_weapon)))
					BotSelectBackupWeapon(this);

				// if there's no other weapon available take knife
				else if ((main_no_ammo && backup_no_ammo && (forced_usage != USE_KNIFE)) ||
					((forced_usage == USE_KNIFE) && (current_weapon.iId != fa_weapon_knife)))
					BotSelectKnife(this);

				// check if the bot is underwater
				if (pEdict->v.waterlevel == 3)
				{
					BotUnderWater( this );
				}
				
				found_waypoint = FALSE;

				// it is time to look for a waypoint if there are some waypoints for this map
				if ((f_dont_look_for_waypoint_time < gpGlobals->time) && (num_waypoints != 0))
				{
					found_waypoint = BotHeadTowardWaypoint(this);



#ifdef _DEBUG
					if (found_waypoint == FALSE)//						TEMPORARY for tests
					{
						//@@@@@@@@@@@@@@@@@@@@@@@@@@@@
						if (botdebugger.IsDebugStuck())
						{
							char dbgmsg[256];
							sprintf(dbgmsg, "BotThink() -> HeadTowardWaypoint() RETURNED FALSE (currWtp=%d currPth=%d) !!!\n",
								curr_wpt_index + 1, curr_path_index + 1);
							HudNotify(dbgmsg, this);
						}
					}
#endif




				}

				// is the bot on ladder
				if (pEdict->v.movetype == MOVETYPE_FLY)
				{
					f_dont_avoid_wall_time = gpGlobals->time + 2.0;

					// then do all the ladder navigation
					BotHandleLadder(this, moved_distance);
				}

				// if the bot isn't headed toward a waypoint...
				if (found_waypoint == FALSE)
				{
					TraceResult tr;

					// check if we should be avoiding walls
					if (f_dont_avoid_wall_time <= gpGlobals->time)
					{
						// let's just randomly wander around
						if (BotStuckInCorner( this ))
						{
							pEdict->v.ideal_yaw += 180;  // turn 180 degrees

							BotFixIdealYaw(pEdict);

							move_speed = SPEED_NO;  // don't move while turning
							f_dont_avoid_wall_time = gpGlobals->time + 1.0;

							moved_distance = 2.0;  // dont use bot stuck code
						}
						else
						{
							// check if there is a wall on the left...
							if (!BotCheckWallOnLeft( this ))
							{
								// if there was a wall on the left over 1/2 a second ago then
								// 20% of the time randomly turn between 45 and 60 degrees

								if ((f_wall_on_left != 0) &&
									(f_wall_on_left <= gpGlobals->time - 0.5) &&
									(RANDOM_LONG(1, 100) <= 20))
								{
									pEdict->v.ideal_yaw += RANDOM_LONG(45, 60);

									BotFixIdealYaw(pEdict);

									move_speed = SPEED_NO;  // don't move while turning
									f_dont_avoid_wall_time = gpGlobals->time + 1.0;
								}

								f_wall_on_left = 0;  // reset wall detect time
							}
							else if (!BotCheckWallOnRight( this ))
							{
								// if there was a wall on the right over 1/2 a second ago then
								// 20% of the time randomly turn between 45 and 60 degrees

								if ((f_wall_on_right != 0) &&
									(f_wall_on_right <= gpGlobals->time - 0.5) &&
									(RANDOM_LONG(1, 100) <= 20))
								{
									pEdict->v.ideal_yaw -= RANDOM_LONG(45, 60);

									BotFixIdealYaw(pEdict);

									move_speed = SPEED_NO;  // don't move while turning
									f_dont_avoid_wall_time = gpGlobals->time + 1.0;
								}

								f_wall_on_right = 0;  // reset wall detect time
							}
						}
					}

					// check if bot is about to hit a wall.  TraceResult gets returned
					if ((f_dont_avoid_wall_time <= gpGlobals->time) && BotCantMoveForward( this, &tr ))
					{
						// ADD LATER
						// need to check if bot can jump up or duck under here...
						// ADD LATER


						BotTurnAtWall( this, &tr );
					}
				}
			}
		}
	}

	//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
	//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
	//
	// section of actions that are done no matter whether the bot does have enemy or not
	// (for example weapon reload that needs to be done in battle as well as during standard navigation)
	//
	//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
	//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


	// does the bot have a waypoint?
	if ((curr_wpt_index != -1) &&
		(f_dont_look_for_waypoint_time < gpGlobals->time))//																	NEW CODE 094 (fix for unstuck code)
	{
		// is next waypoint the bot is heading towards to...
		
		// a crouch waypoint then duck down while moving forward
		// this one must be here else the bot would NOT go crouch while heading towards this waypoint
		// he would crouch only for a short moment after passing through the waypoint
		if (waypoints[curr_wpt_index].flags & W_FL_CROUCH)
		{
			if (IsBehaviour(BOT_PRONED))
				UTIL_GoProne(this, "BotThink()|curr_wpt not -1 -> At CROUCH WPT but in prone!");
			else
				SetStance(this, GOTO_CROUCH, "BotThink()|curr_wpt not -1 -> GOTO crouch at CROUCH WPT");
		}

		// a sniper waypoint then set don't move flag
		if (waypoints[curr_wpt_index].flags & W_FL_SNIPER)
		{
			//SetTask(TASK_DEATHFALL);
		}
	}

	// the bot has to scan forward direction to detect dangerous depths ie. deathfalls
	if (IsTask(TASK_DEATHFALL) && (f_check_deathfall < gpGlobals->time))
	{
		// set next check
		f_check_deathfall = gpGlobals->time + 0.2;		//was 1.0

		// if the bot has enemy and there's a dangerous depth in front of the bot then stop
		// we check both far and close distance to make sure that there really isn't even a
		// narrow pit/gap, once one is true then we stop the bot
		if ((pBotEnemy != nullptr) && (IsDeathFall(pEdict) || IsForwardBlocked(this)))
		{
			move_speed = SPEED_NO;
			SetDontMoveTime(1.0);
			SetDontCheckStuck("bot think() -> TaskDeathFall", 1.5);
		}

		// no enemy ie normal navigation
		//if ((pBotEnemy == NULL) && (IsDeathFall(pEdict) || IsForwardBlocked(this)))
		//{
			// TODO: turn the bot back (something like with parachute wpt
			//		 when the bot has no chute
		//}
	}

	// the bot must be changing his current weapon
	if ((weapon_action == W_TAKEOTHER) || (weapon_action == W_INCHANGE) || (weapon_action == W_INHANDS))
	{
		// we can't change current weapon if the bot is paused, because that means
		// he's under medical treatment from his teammate
		// and FA doesn't allow any weapon based action in such cases
		if (f_pause_time < gpGlobals->time)
		{
			

			//@@@@@@@@@@@@@22
#ifdef _DEBUG
			if (botdebugger.IsDebugActions() || botdebugger.IsDebugStuck())
			{
				char cwmsg[128];
				sprintf(cwmsg, "bot think() -> weaponaction == takeother or inchange\n");
				HudNotify(cwmsg, true, this);
			}
#endif





			UTIL_ChangeWeapon(this);
		}
	}

	// or is the bot reloading his current weapon?
	// we must check for pause time, because teammate giving a medical treatment
	// invalidates the reload action and the bot must wait till it's over
	// then he can reset the reloading and start it anew 
	else if ((weapon_action == W_INRELOAD) && (f_pause_time < gpGlobals->time))
	{
		f_waypoint_time = gpGlobals->time;
		f_dont_look_for_waypoint_time = gpGlobals->time;

		move_speed = SPEED_NO;
		SetDontCheckStuck("bot think() -> reloading weapon");

		// to prevent bot going prone while reloading weapon, because
		// doing so in FA will invalidate whole reload proccess and you end up with empty gun
		// (although you can see weapon reloading animation)
		SetBehaviour(BOT_DONTGOPRONE);

		// didn't the bot press reload button yet?
		if ((f_reload_time < gpGlobals->time) && (IsGoingProne() == FALSE) &&// (f_pause_time < gpGlobals->time) &&
			(f_bipod_time < gpGlobals->time) && !IsWeaponStatus(WS_PRESSRELOAD))
		{
			// then do so now
			pEdict->v.button |= IN_RELOAD;

			// to know the bot did it
			SetWeaponStatus(WS_PRESSRELOAD);

			// we need correct time to finish reloading the weapon
			SetReloadTime();



			//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@			NEW CODE 094 (remove it)
#ifdef DEBUG
			if (g_debug_bot_on && (g_debug_bot == pEdict) || !g_debug_bot_on)
			{
				char dp[256];
				sprintf(dp, "%s's weapon reloading will be finished at %.3f\n", name, f_reload_time);
				UTIL_DebugInFile(dp);
			}
#endif // DEBUG




			// see if are reloading only partly used magazine
			if (IsWeaponStatus(WS_NOTEMPTYMAG))
			{
				// reaching two partly used magazines will call a mergeclips command
				// what we don't handle here is fact that each partly used magazine may be from different weapon
				// (main or backup), however merging magazines doesn't cause any fatal problems in Firearms
				// so if the bot is going to use it at wrong moment isn't big issue
				if (IsWeaponStatus(WS_MERGEMAGS1))
					SetWeaponStatus(WS_MERGEMAGS2);
				else
					SetWeaponStatus(WS_MERGEMAGS1);

				RemoveWeaponStatus(WS_NOTEMPTYMAG);
			}

			if (botdebugger.IsDebugActions())
				HudNotify("(botthink()|in reload) -> reload button pressed\n", this);
		}
		// isn't weapon still reloaded?
		else if ((current_weapon.iClip == 0) && (f_reload_time < gpGlobals->time))
		{
			// if the reloading was invalidated ...
			if (IsWeaponStatus(WS_INVALID))
			{
				// clear the "button pressed" bit
				RemoveWeaponStatus(WS_PRESSRELOAD);

				// clear the reset bit to prevent running this again
				RemoveWeaponStatus(WS_INVALID);

				// prevent the bot to go crouch, because there's no reloading now
				f_stance_changed_time = gpGlobals->time + 0.2f;

				// and reset the action
				f_reload_time = gpGlobals->time;
				weapon_action = W_READY;

				// seems like these weapons don't want to work standard way
				// so we'll force a switching to knife and let the weapon management system
				// return back to these and reload them properly.
				if ((current_weapon.iId == fa_weapon_g36e) || (current_weapon.iId == fa_weapon_pkm))
				{
					UseWeapon(USE_KNIFE);
					weapon_action = W_TAKEOTHER;
				}

				if (botdebugger.IsDebugActions())
					HudNotify("botthink()|in reload) -> reloading was invalidated -> going to do it again", this);
			}
			else
			{
				// something must have gone wrong so we'll invalidate this try and try it anew
				SetWeaponStatus(WS_INVALID);
				// to keep the bot in reload "action"
				f_reload_time = gpGlobals->time + 0.1f;

#ifdef _DEBUG

				if (botdebugger.IsDebugActions())
				{
					if (IsWeaponStatus(WS_PRESSRELOAD))
					{
						char dbgmsg[256];
						sprintf(dbgmsg, "(botthink()|in reload) weaponID=%d still not loaded -> reload was invalidated (FAver=%d)\n",
							current_weapon.iId, g_mod_version);
						HudNotify(dbgmsg, this);
					}
					else
						HudNotify("(botthink()|in reload) -> button wasn't pressed yet! -> THIS IS BUG !!!", this);
				}
#endif
			}

		}
		// seeing the clip isn't zero then the weapon must be loaded now
		else if ((current_weapon.iClip != 0) && (f_reload_time < gpGlobals->time))
		{
			// so let the bot know about it
			f_reload_time = gpGlobals->time;
			weapon_action = W_READY;
			RemoveWeaponStatus(WS_PRESSRELOAD);
			RemoveWeaponStatus(WS_INVALID);

			if (botdebugger.IsDebugActions())
				HudNotify("(botthink()|in reload) weapon fully loaded -> WEAPON is READY\n", this);

			
			
			// NEW CODE 094 (remove it)
#ifdef _DEBUG
			//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
			//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
			char dbgmsg[256];
			sprintf(dbgmsg, "(bot.cpp|botthink()|in reload) weapon fully loaded (iclip=%d) -> weapon action set to ready\n",
				current_weapon.iClip);
			if (IsWeaponStatus(WS_MERGEMAGS1))
				strcat(dbgmsg, "<merge magazines1>");
			if (IsWeaponStatus(WS_MERGEMAGS2))
				strcat(dbgmsg, "<merge magazines2>");

			//HudNotify(dbgmsg, true, this);
#endif


		}

		// if NOT proned then crouch
		if (IsBehaviour(BOT_PRONED) == FALSE)
			SetStance(this, GOTO_CROUCH, "BotThink()|In_Reload -> GOTO crouch");
	}

	// show main_weapon, backup_weapon, grenade_slot and claymore_slot ID
	// and which weapon is currently forced to use
	if (botdebugger.IsDebugWeapons())
		UTIL_ShowWeapon(this);

	// do we need to check weapons for ammunition that should be taken from ammobox AND
	// is it time to do it yet?
	if (IsTask(TASK_CHECKAMMO) && (check_ammunition_time < gpGlobals->time))
		UTIL_CheckAmmoReserves(this, "BotThink() -> is task AND is time for it");

	// the bot is being hit by his teammate
	if (IsSubTask(ST_SAY_CEASEFIRE))
	{
		// check if the damage inflictor didn't changed yet and if it is really a teammate
		if ((pEdict->v.dmg_inflictor != pEdict) && (pEdict->v.dmg_inflictor->v.netname != NULL) &&
			(UTIL_GetTeam(pEdict->v.dmg_inflictor) == UTIL_GetTeam(pEdict)))
			BotSpeak(SAY_CEASE_FIRE, STRING(pEdict->v.dmg_inflictor->v.netname));

		RemoveSubTask(ST_SAY_CEASEFIRE);
	}

	// check whether the bot is stuck
	// in other words he hasn't moved much since the last location
	// (and NOT on a ladder since ladder stuck handled elsewhere)
	// (don't check for stuck if bot is turning a lot at waypoint or if it isn't time)
	
	//if ((moved_distance <= 1.0) && (prev_speed != SPEED_NO) && (pEdict->v.movetype != MOVETYPE_FLY) &&									NEW CODE 094 (prev code)
	//	(f_face_wpt < gpGlobals->time) && (f_dont_check_stuck < gpGlobals->time))
	if ((moved_distance <= 1.0) && (pEdict->v.movetype != MOVETYPE_FLY) &&
		(f_face_wpt < gpGlobals->time) && (f_dont_check_stuck < gpGlobals->time))
	{
		float max_strafe_time = 1.5f;	// used when setting the duration of side steps movements
		TraceResult tr;
		Vector v_src, v_dest;

		f_dont_avoid_wall_time = gpGlobals->time + 1.0;
		f_dont_look_for_waypoint_time = gpGlobals->time + 1.0;



#ifdef DEBUG

		if (botdebugger.IsDebugStuck())
		{
			char smsg[128];
			sprintf(smsg, "[[Stuck(bot.cpp)]] has been called!!! Check why! (prevspeed=%d||movespeed=%d||moveddist=%.2f)\n",
				prev_speed, move_speed, moved_distance);
			//HudNotify(smsg, this);
		}

#endif // DEBUG




		// if the bot is already trying side steps to free self
		// then don't start any new action until he finishes it
		if (f_strafe_time < gpGlobals->time)
		{
			UTIL_MakeVectors(pEdict->v.v_angle);

			v_src = pEdict->v.origin + pEdict->v.view_ofs;
			v_dest = v_src + gpGlobals->v_forward * 70;

			UTIL_TraceLine(v_src, v_dest, dont_ignore_monsters, pEdict, &tr);

			// the bot is in front of a breakable object
			if ((strcmp(STRING(tr.pHit->v.classname), "func_breakable") == 0) && (tr.pHit->v.takedamage == DAMAGE_YES))
			{
				move_speed = SPEED_NO;

				//													NEW CODE 094 (checking if the weapon is ready)
				//													but IN_ATTACK probably doesn't need to be here at all
				//													because we set the task breakit here which will
				//													handle it anyways ( TODO: Needs to be tested )
				if (weapon_action == W_READY)
					pEdict->v.button |= IN_ATTACK;

				SetTask(TASK_BREAKIT);

				if (botdebugger.IsDebugStuck())
					HudNotify("[[Stuck(bot.cpp)]] on breakable object, just shoot it \n", this);
			}

			// if the bot is beeing "used" AND is very close to user then just stop
			else if ((pBotUser != nullptr) && ((pEdict->v.origin - pBotUser->v.origin).Length() < 150))
			{
				move_speed = SPEED_NO;

				if (botdebugger.IsDebugStuck())
					HudNotify("[[Stuck(bot.cpp)]] while following user, is quite close so just stop\n", this);
			}

			// can the bot jump (ie has enough stamina to jump and not proned)
			// and can jump onto something?
			else if (!IsTask(TASK_NOJUMP) && (IsBehaviour(BOT_PRONED) == FALSE) && (IsGoingProne() == FALSE) &&
				BotCanJumpUp(this))
			{
				pEdict->v.button |= IN_JUMP; // jump up and move forward

				if (botdebugger.IsDebugStuck())
					HudNotify("[[Stuck(bot.cpp)]] bot can jump over it\n", this);
			}
			// can the bot duck jump up the object
			else if (!IsTask(TASK_NOJUMP) && (IsBehaviour(BOT_PRONED) == FALSE) && (IsGoingProne() == FALSE) &&
				BotCanDuckJumpUp(this))
			{
				pEdict->v.button |= IN_JUMP;
				pEdict->v.button |= IN_DUCK; // duck is a must in this case
				f_duckjump_time = gpGlobals->time + 1.0; // keep the "courch" key pressed

				if (botdebugger.IsDebugStuck())
					HudNotify("[[Stuck(bot.cpp)]] bot can DUCKJUMP over it\n", this);
			}
			// can the bot duck under something?
			else if (BotCanDuckUnder(this))
			{
				// if NOT proned AND NOT going to/resume from prone...
				if ((IsBehaviour(BOT_PRONED) == FALSE) && (IsGoingProne() == FALSE))
					SetStance(this, GOTO_CROUCH, "BotThink()|UnStuck-CanDuckUnder() -> GOTO crouch");

				if (botdebugger.IsDebugStuck())
					HudNotify("[[Stuck(bot.cpp)]] bot can crouch under it\n", this);
			}
			else if (BotCantStrafeLeft(pEdict) == FALSE)
			{
				pEdict->v.button |= IN_MOVELEFT;
				move_speed = SPEED_SLOW;

				f_strafe_direction = -1.0;

				// test this not sure about it (if bugged then lower range top value)
				//f_strafe_time = gpGlobals->time + RANDOM_FLOAT(0.5, 1.0);					NEW CODE 094 (prev code)
				f_strafe_time = gpGlobals->time + RANDOM_FLOAT(0.5, max_strafe_time);

				if (botdebugger.IsDebugStuck())
					HudNotify("[[Stuck(bot.cpp)]] can strafe LEFT\n", this);
			}
			else if (BotCantStrafeRight(pEdict) == FALSE)
			{
				pEdict->v.button |= IN_MOVERIGHT;
				move_speed = SPEED_SLOW;

				f_strafe_direction = 1.0;
				//f_strafe_time = gpGlobals->time + RANDOM_FLOAT(0.5, 1.0);					NEW CODE 094 (prev code)
				f_strafe_time = gpGlobals->time + RANDOM_FLOAT(0.5, max_strafe_time);

				if (botdebugger.IsDebugStuck())
					HudNotify("[[Stuck(bot.cpp)]] can strafe RIGHT\n", this);
			}
			else
			{
				BotRandomTurn(this);

				if (botdebugger.IsDebugStuck())
					HudNotify("[[Stuck(bot.cpp)]] random turn\n", this);
			}

			// is the bot stuck again in a quite short time AND
			// finished his tries to sidestep the obstacle
			if ((f_stuck_time + max_strafe_time + 0.2f) > gpGlobals->time)
			{
				// is the bot fully proned so stand up from prone becuase prone
				// can easily cause stuck
				// exception is when the bot is crawling through claymore mine
				// in such case standing up would detonate the mine
				if (IsBehaviour(BOT_PRONED) && !IsTask(TASK_CLAY_EVADE))
				{
					UTIL_GoProne(this, "BotThink()|UnStuck -> STUCK in prone stance");
					SetStance(this, GOTO_STANDING, "BotThink()|UnStuck->STUCK in prone -> GOTO standing");

					if (botdebugger.IsDebugStuck())
						HudNotify("[[Stuck(bot.cpp)]] when PRONED --> standing up\n", this);
				}
				// is the bot stuck while crouched then stand up
				else if (IsBehaviour(BOT_CROUCHED))
				{
					SetStance(this, GOTO_STANDING, "BotThink()|UnStuck->STUCK in crouch -> GOTO standing");

					if (botdebugger.IsDebugStuck())
						HudNotify("[[Stuck(bot.cpp)]] when CROUCHED --> standing up\n", this);
				}
				// is the bot stuck on dead player then just wait awhile till the body disappers
				else if (IsEntityInSphere("bodyque", pEdict, 50))
				{
					move_speed = SPEED_NO;
					SetDontMoveTime(1.0);

					if (botdebugger.IsDebugStuck())
					{
						HudNotify("[[Stuck(bot.cpp)]] on DEAD BODY --> waiting till it disappers\n", this);
						UTIL_DebugDev("(bot.cpp)Stuck on DEAD BODY --> waiting till it disappers\n", -100, -100);
					}
				}
				else
				{
					// try turning to any direction, it might be free
					BotRandomTurn(this);

					IncChangedDirection();

					if (botdebugger.IsDebugStuck())
					{
						char dsm[256];
						sprintf(dsm, "[[Stuck(bot.cpp)]] BOT IS COMPLETELY STUCK - trying turn to random direction (# of tries before committing suicide %d)\n", changed_direction);
						HudNotify(dsm, this);
					}
				}

				// if the bot tried turning to different direction a few times and he's still
				// stuck then the last option is to commit a suicide
				if (changed_direction > 20)
				{
					extern DLL_FUNCTIONS other_gFunctionTable;

					if (botdebugger.IsDebugStuck())
						HudNotify("[[Stuck(bot.cpp)]] COMPLETELY & NO WAY TO UNSTUCK --> commit SUICIDE\n", this);

					(*other_gFunctionTable.pfnClientKill)(pEdict);
				}
			}
			// otherwise store the time of current stuck
			else
				f_stuck_time = gpGlobals->time;
		}
	}

	// if the bot is drowning keep the jump key pressed to get him above the water level
	else if (IsTask(TASK_DROWNING))
	{
		pEdict->v.button |= IN_JUMP;


		//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@										NEW CODE 094 (remove it)
#ifdef _DEBUG
		HudNotify("(bot.cpp|botthink()) -> TASK DROWNING -> JUMP!\n", true, this);
#endif


	}

	// do we lost some health AND is it time to bandage the bot (i.e. NOT in battle right now)
	// AND is already some time left from last bandage treatment AND pause time is over
	else if ((bot_health < bot_prev_health) && (bandage_time != -1.0) && (bandage_time < gpGlobals->time) &&	//was (bandage_time + 1.0 < gpGlobals->time) &&
		(f_pause_time < gpGlobals->time) && !IsTask(TASK_DROWNING))
	{
		char dmg_inflictor[64], bot[64];

		// get damage source name
		if (pEdict->v.dmg_inflictor)
			strcpy(dmg_inflictor, STRING(pEdict->v.dmg_inflictor->v.netname));

		// get this bot name
		strcpy(bot, STRING(pEdict->v.netname));

		// does he really bleed (ie doesn't the bot just fall off something,
		// if both names match bot bleeds) OR he "heard" his own bleeding
		if ((strcmp(dmg_inflictor, bot) == 0) || IsTask(TASK_BLEEDING))
		{
			// has the bot some bandages and doesn't reload or change weapon
			//if (bot_bandages > 0)																								// NEW CODE 094 (prev code)
			if ((bot_bandages > 0) && (weapon_action == W_READY))
			{
				if (botdebugger.IsDebugActions())
					HudNotify("***Start bandage treatment\n", this);

				// so use bandages (ie start bandage treatment)
				FakeClientCommand(pEdict, "usebandage", nullptr, nullptr);

				// based on tests in FA 3.0 bandaging takes 4.2 seconds,
				// but we are going to set a little longer delay to successfully finish the treatment
				bandage_time = gpGlobals->time + 4.5f;

				// the bot doesn't bleed
				RemoveTask(TASK_BLEEDING);
			}
			// no bandages so then call for medic if not a medic AND if it is time to speak
			else if (!(bot_fa_skill & (FAID | MEDIC)) && ((speak_time + RANDOM_FLOAT(2.0, 5.0)) < gpGlobals->time))
			{
				UTIL_Voice(pEdict, "medic");

				// update speak time
				speak_time = gpGlobals->time;

				// randomly set some pause time to see if any medic comes
				if (RANDOM_LONG(1, 100) <= 15)
				{
					SetPauseTime(RANDOM_FLOAT(1.5, 3.5));

					if (botdebugger.IsDebugActions())
						HudNotify("Waiting if medic comes\n", this);
				}
			}
		}
	}

	// reset chute time if the bot isn't in mid-air
	else if ((chute_action_time > 1.0) && !IsTask(TASK_PARACHUTE_USED) && (pEdict->v.flags & FL_ONGROUND))
	{
		chute_action_time = 0.0;

		if (botdebugger.IsDebugActions())
			HudNotify("Resetting parachute - not in air\n");
	}

	// is the bot in mid-air AND has parachute AND didn't open it yet AND
	// is it the right time (ie open the chute after some time in mid-air)
	else if (BotCheckMidAir(pEdict) && IsTask(TASK_PARACHUTE) && !IsTask(TASK_PARACHUTE_USED) &&
		(chute_action_time <= gpGlobals->time) && (bot_spawn_time + 1.0 < gpGlobals->time))
	{
		// set some time to check if the bot is really in mid-air
		// ie to prevent opening chute when he steps down from or jumps off some low object
		if (chute_action_time < 1.0)
		{
			//chute_action_time = gpGlobals->time + 0.7;								// NEW CODE 094 (prev code)
			
			//																			NEW CODE 094
			// having the time of check as a random value should prevent multiple bots to open the parachute
			// at the same moment and get stuck in air unable to do anything
			if (strcmp(STRING(gpGlobals->mapname), "obj_vengeance") == 0)
				chute_action_time = gpGlobals->time + RANDOM_FLOAT(1.0, 1.5);
			else if (strcmp(STRING(gpGlobals->mapname), "ps_outlands") == 0)
				chute_action_time = gpGlobals->time + RANDOM_FLOAT(0.5, 0.8);
			else
				chute_action_time = gpGlobals->time + RANDOM_FLOAT(0.7, 1.5);

			if (botdebugger.IsDebugActions())
				HudNotify("Testing if really in mid-air - prevention of false parachuting when a fall from low object happens\n", this);
		}
		// the bot must be really in mid-air so use the chute
		if ((chute_action_time > 1.0) && (chute_action_time < gpGlobals->time))
		{
			pEdict->v.button |= IN_USE;
			SetTask(TASK_PARACHUTE_USED);

			if (botdebugger.IsDebugActions())
				HudNotify("Testing if really in mid-air - yes really in air -> using parachute\n", this);
		}
	}

	// is the bot back on ground so it's time to throw the chute off
	else if (IsTask(TASK_PARACHUTE) && IsTask(TASK_PARACHUTE_USED) && (chute_action_time < gpGlobals->time) &&
		(pEdict->v.movetype != MOVETYPE_FLY) && (pEdict->v.flags & (FL_ONGROUND | FL_INWATER)))
	{
		pEdict->v.button |= IN_USE;

		chute_action_time = gpGlobals->time + 2.0;

		// we have to clear these variables manually, because FA 2.65 and below call parachute message
		// in a different way than all versions above
		if (UTIL_IsOldVersion() || (g_mod_version == FA_26))
		{
			RemoveTask(TASK_PARACHUTE);
			SetNeed(NEED_RESETPARACHUTE);
		}

		if (botdebugger.IsDebugActions())
			HudNotify("Back on ground - throwing parachute away\n", this);
	}

	// see if it is time to finalize the parachute reset in FA 2.65 and below
	else if (IsNeed(NEED_RESETPARACHUTE) && (chute_action_time < gpGlobals->time))
	{
		chute_action_time = 0.0;
		RemoveTask(TASK_PARACHUTE_USED);
		RemoveNeed(NEED_RESETPARACHUTE);

		if (botdebugger.IsDebugActions())
			HudNotify("***finalizing parachute reset\n", this);
	}

	// is it time to activate the claymore or detonate it at goal area 
	else if (((clay_action == ALTW_PLACED) || (clay_action == ALTW_GOALPLACED)) && (f_use_clay_time <= gpGlobals->time))
	{
		if (botdebugger.IsDebugActions())
			HudNotify("Activating/detonating claymore mine\n", this);

		if (clay_action == ALTW_GOALPLACED)
		{
			// detonate it
			pEdict->v.button |= IN_ATTACK;
		}
		else
		{
			// activate it
			pEdict->v.button |= IN_ATTACK2;
		}

		// bot already used the mine
		clay_action = ALTW_USED;

		// set some time to successfully finish the action
		// if you change to different weapon too fast the mine will be disabled in firearms
		f_use_clay_time = gpGlobals->time + 1.0;
	}

	// is the bot tasked to evade a claymore OR pretend he didn't see it?
	else if (IsTask(TASK_CLAY_EVADE) || IsTask(TASK_CLAY_IGNORE))
	{
		BotEvadeClaymore(this);
	}

	// is the bot forced to sprint and has no enemy and is not reloading weapon
	// (ie. we can't shoot while sprinting so when the bot has an enemy we can't set these two
	// otherwise the bot won't be able to shoot his weapon, and we can't reload as well)
	else if (IsTask(TASK_SPRINT) && (pBotEnemy == nullptr) && (f_reload_time < gpGlobals->time) &&
		(current_weapon.iId != fa_weapon_claymore))
	{
		// both must be set, because bot doesn't use in_forward normally
		pEdict->v.button |= IN_FORWARD;
		pEdict->v.button |= IN_RUN;
	}

	// did the bot decide to go prone AND NOT already in prone?
	else if (IsBehaviour(GOTO_PRONE) && (IsBehaviour(BOT_PRONED) == FALSE))
	{
		UTIL_GoProne(this, "BotThink() -> Behaviour is GOTO prone");
	}

	// the bot is using mounted gun
	else if (IsTask(TASK_USETANK))
	{
		// if the bot already has his own weapon back in hands
		// (ie doesn't hold the tank and also isn't still facing it)
		if ((f_face_item_time < gpGlobals->time) && (pEdict->v.viewmodel != 0) && (pEdict->v.health > 0))
		{
			// stop using tank
			RemoveTask(TASK_USETANK);
			RemoveSubTask(ST_TANK_SHORT);

			// also reset action time
			if (wpt_action_time > gpGlobals->time)
				wpt_action_time = gpGlobals->time - 0.1;

			if (botdebugger.IsDebugActions())
				HudNotify("(bot.cpp)***stopped using TANK\n", this);
		}
		// the bot is still using tank
		else
		{
			f_waypoint_time = gpGlobals->time;
			f_dont_look_for_waypoint_time = gpGlobals->time;

			move_speed = SPEED_NO;
			SetDontCheckStuck("bot think() -> bot is using TANK");

			// the bot is already facing the gun and has no enemy at the moment
			if ((f_face_item_time < gpGlobals->time) && (pBotEnemy == nullptr))
			{
				// the bot has been told to use the tank only for given time
				// and the time is over now
				if (IsSubTask(ST_TANK_SHORT) && (wpt_action_time < gpGlobals->time))
				{
					// then we need to stop using the mounted gun
					RemoveTask(TASK_USETANK);
					RemoveSubTask(ST_TANK_SHORT);

					if (botdebugger.IsDebugActions())
						HudNotify("(bot.cpp)stopped using TANK (the use time is over)\n", this);
				}

				// has the bot any (at least one) aim waypoint
				if (aim_index[0] != -1)
				{
					// is it time to target new aim waypoint
					if (f_aim_at_target_time < gpGlobals->time)
					{
						int new_target;
						
						new_target = RANDOM_LONG(1, 4) - 1;
						
						// search for valid aim waypoint
						while (aim_index[new_target] == -1)
							new_target = RANDOM_LONG(1, 4) - 1;
						
						// store new aim waypoint as current target
						curr_aim_index = aim_index[new_target];
						
						Vector v_aim = waypoints[aim_index[new_target]].origin - pEdict->v.origin;
						Vector aim_angles = UTIL_VecToAngles(v_aim);
						
						pEdict->v.ideal_yaw = aim_angles.y;
						BotFixIdealYaw(pEdict);
						
						// keep aiming at this waypoint for some time
						f_aim_at_target_time = gpGlobals->time + RANDOM_FLOAT(5.0, 10.0);
					}
					
					Vector v_aim = waypoints[curr_aim_index].origin - pEdict->v.origin;
					Vector aim_angles = UTIL_VecToAngles(v_aim);
					
					pEdict->v.ideal_yaw = aim_angles.y;
					BotFixIdealYaw(pEdict);
				}
				// there isn't any aim waypoint so try to aim at the direction of tank origin
				else
				{
					// search func_tank entity if the bot hasn't one yet
					if (pItem == nullptr)
					{
						edict_t *pent = nullptr;
						
						while ((pent = UTIL_FindEntityInSphere(pent, pEdict->v.origin, 100)) != nullptr)
						{
							if (strcmp("func_tank", STRING(pent->v.classname)) == 0)
								pItem = pent;
						}
					}
					
					// has the bot already an item and is it a tank entity
					if ((pItem) && (strcmp("func_tank", STRING(pItem->v.classname)) == 0))
					{
						Vector v_aim = pItem->v.origin - pEdict->v.origin;
						Vector aim_angles = UTIL_VecToAngles(v_aim);
						
						pEdict->v.ideal_yaw = aim_angles.y;
						BotFixIdealYaw(pEdict);
					}
				}
			}
		}
	}

	// determine strafe moves
	else if ((f_strafe_time > gpGlobals->time) && (bot_spawn_time + 2.0 < gpGlobals->time))
	{
		f_dont_avoid_wall_time = gpGlobals->time + 1.0;
		f_dont_look_for_waypoint_time = gpGlobals->time + 0.1;

		if (f_strafe_direction == -1.0)
		{
			pEdict->v.button |= IN_MOVELEFT;
			move_speed = SPEED_NO;
		}
		else
		{
			pEdict->v.button |= IN_MOVERIGHT;
			move_speed = SPEED_NO;
		}

		f_strafe_speed = f_strafe_direction * (f_max_speed / 2.0);
	}

	//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
	//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
	//
	// various flags clearing section
	//
	//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
	//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

	// is some time after waypoint action AND TANK is NOT used
	if (IsTask(TASK_WPTACTION) && (wpt_action_time + 0.5 < gpGlobals->time) && !IsTask(TASK_USETANK))
	{
		pItem = nullptr;					// no item
		RemoveTask(TASK_WPTACTION);		// clear it
		RemoveSubTask(ST_FACEITEM_DONE);
		RemoveSubTask(ST_BUTTON_USED);
		RemoveSubTask(ST_TANK_SHORT);	// there's no mounted gun at all so this must be removed

		if (botdebugger.IsDebugActions())
			HudNotify("***Use waypoint action time over - (actions cleared)\n", this);



		//@@@@@@@@@@@@@@@
#ifdef _DEBUG
		char dm[256];
		sprintf(dm, "BotThink()|ClearingSection -> (curr wpt #%d) Task WptAction RESET (wpt action Time is OVER)\n",
			curr_wpt_index + 1);
		HudNotify(dm, this);
#endif



	}

	// clear the item pointer once the bot doesn't need it
	if ((pItem != nullptr) && !IsTask(TASK_WPTACTION) && !IsTask(TASK_USETANK) && (wpt_wait_time + 0.5 < gpGlobals->time))
	{
		pItem = nullptr;					// no item
		RemoveSubTask(ST_RANDOMCENTRE);

		if (botdebugger.IsDebugActions())
			HudNotify("***Clearing pItem pointer)\n", this);
	}

	// if next waypoint isn't sniper waypoint AND the bot has no enemy
	// clear the don't move flag (ie bot is allowed to move in next combat)
	if (IsTask(TASK_DONTMOVEINCOMBAT) && (wpt_wait_time < gpGlobals->time) && !IsWaypoint(curr_wpt_index, WptT::wpt_sniper) &&
		(pBotEnemy == nullptr))
	{
		RemoveTask(TASK_DONTMOVEINCOMBAT);

		if (botdebugger.IsDebugActions() || botdebugger.IsDebugStuck())
			HudNotify("***Now moving again (dontmove cleared)\n", this);
	}

	// the bot still has a door vector and the wait time is over (ie. doors probably opened)
	// then clear it
	//if (v_door != Vector(0, 0, 0) && (wpt_wait_time + 0.5 < gpGlobals->time))
	if ((v_door != g_vecZero) && (wpt_wait_time + 0.5 < gpGlobals->time))
	{
		v_door = g_vecZero;//Vector(0, 0, 0);
		RemoveSubTask(ST_DOOR_OPEN);		// just in case

		if (botdebugger.IsDebugActions())
			HudNotify("***Cleared open door actions\n", this);
	}

	// is the bot NOT on ladder but he still has set ladder variables so clear them all
	if (((ladder_dir == LADDER_UP) || (ladder_dir == LADDER_DOWN)) && (pEdict->v.movetype != MOVETYPE_FLY) &&
		(pEdict->v.flags & FL_ONGROUND) && (f_start_ladder_time + 2.0 < gpGlobals->time))
	{
		ladder_dir = LADDER_UNKNOWN;
		end_wpt_index = -1;
		waypoint_top_of_ladder = FALSE;

		if (botdebugger.IsDebugActions())
			HudNotify("***Cleared ladder actions\n", this);
	}

	// if the drowning flag is set AND the bot is no more under water then clear the flag
	// this is mainly needed to handle FA2.9 (due to absence of some standard engine messages)
	if (IsTask(TASK_DROWNING) && (pEdict->v.waterlevel != 3))
	{
		RemoveTask(TASK_DROWNING);

		if (botdebugger.IsDebugActions())
			HudNotify("***Bot is no more drowning (drowning flag cleared)\n", this);
	}

	// is stuck time over and the suicide timer/counter isn't cleared yet so do it now
	if ((changed_direction > 0) && (f_stuck_time + 1.5 < gpGlobals->time))
	{
		ResetChangedDirection();

		if (botdebugger.IsDebugActions() || botdebugger.IsDebugStuck())
			HudNotify("***Resetting suicide timer/counter (bot is no more stuck)\n", this);
	}

	// if the bot moved then reset cannot bipod bit
	if (IsWeaponStatus(WS_CANTBIPOD) && (moved_distance > 2.0))
	{
		RemoveWeaponStatus(WS_CANTBIPOD);



#ifdef DEBUG
		//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@														// NEW CODE 094 (remove it)
		char dm[256];
		sprintf(dm, "%s cleared CANTBIPOD bit @ botthink()|moved distance > 2.0\n", name);
		HudNotify(dm, true, this);
		//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#endif // DEBUG



	}

	// did the bot finished mergining magazines?
	if ((weapon_action == W_INMERGEMAGS) && (f_pause_time < gpGlobals->time))
	{
		// then set the weapon back to ready for action
		weapon_action = W_READY;

		if (botdebugger.IsDebugActions())
			HudNotify("***Merging magazines has been finished -> WEAPON is READY again\n", this);
	}

	// tried the bot go prone, but engine sent him 'cannot prone here' message...
	if (IsSubTask(ST_CANTPRONE) && IsGoingProne())
	{

#ifdef DEBUG
		//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@														// NEW CODE 094 (remove it)
		char dm[256];
		sprintf(dm, "%s did reset goproneTIME(=%.3f) and cleared GOTO standing @ botthink()|Clearing section. CHECK why does it happen!\n",
			name, f_go_prone_time + 1.2);
		HudNotify(dm, true, this);
		//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#endif // DEBUG




		// then reset going prone time so the bot can act normally in next game frame
		f_go_prone_time = 0.0;

		// and reset 'stand up' bit as well if the bot was trying to stand up, because it cannot be done now
		RemoveBehaviour(GOTO_STANDING);
	}

	// bot moved a little so see if we can reset the cannot go prone subtask
	if (IsSubTask(ST_CANTPRONE))
	{
		if ((weapon_action == W_READY) && !IsTask(TASK_BIPOD) && (f_bipod_time < gpGlobals->time) &&
			(IsGoingProne() == FALSE) && (f_pause_time < gpGlobals->time))
		{
			if (moved_distance > 20.0)
			{
				RemoveSubTask(ST_CANTPRONE);




#ifdef DEBUG
				//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@											// NEW CODE 094 (remove it)
				char dm[256];
				sprintf(dm, "%s botthink()|CANTPRONE subtask -> cleared CANTPRONE subtask (moved_dist=%.2f)\n",
					name, moved_distance);
				HudNotify(dm, true, this);
				//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#endif // DEBUG
			}
			else if ((moved_distance < 20.0) && (f_strafe_time < gpGlobals->time))
			{

				RemoveSubTask(ST_CANTPRONE);


#ifdef DEBUG
				//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@											// NEW CODE 094 (remove it)
				char dm[256];
				sprintf(dm, "%s botthink()|CANTPRONE subtask -> cleared CANTPRONE subtask (moved_dist=%.2f)\n",
					name, moved_distance);
				HudNotify(dm, true, this);
				//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#endif // DEBUG





				if (f_dont_check_stuck < gpGlobals->time)
				{
#ifdef DEBUG
					//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@											// NEW CODE 094 (remove it)
					char dm[256];
					sprintf(dm, "%s botthink()|CANTPRONE subtask -> a try to STRAFE (moved_dist=%.2f)\n",
						name, moved_distance);
					HudNotify(dm, true, this);
					//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#endif // DEBUG

					if (BotCantStrafeLeft(pEdict) == FALSE)
					{
						f_strafe_direction = -1.0;
						f_strafe_time = gpGlobals->time + RANDOM_FLOAT(0.5, 1.0);
					}
					else if (BotCantStrafeRight(pEdict) == FALSE)
					{
						f_strafe_direction = 1.0;
						f_strafe_time = gpGlobals->time + RANDOM_FLOAT(0.5, 1.0);
					}
				}
			}
		}
	}

	// if the bot is tasked to go/resume prone, but something else is blocking this action...
	if (IsTask(TASK_GOPRONE) && IsSubTask(ST_CANTPRONE))
	{
		// then prevent calling the command
		RemoveTask(TASK_GOPRONE);


#ifdef DEBUG
		//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@														// NEW CODE 094 (remove it)
		char dm[256];
		sprintf(dm, "%s REMOVED TASK_GOPRONE @ BotThink()|Clearing section -> IsSubtask(cantprone)!\n", name);
		HudNotify(dm, true, this);
		//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#endif // DEBUG


	}

	if (IsTask(TASK_GOPRONE) && IsBehaviour(BOT_DONTGOPRONE))
	{
		// then prevent calling the command
		RemoveTask(TASK_GOPRONE);


#ifdef DEBUG
		//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@														// NEW CODE 094 (remove it)
		char dm[256];
		sprintf(dm, "%s REMOVED TASK_GOPRONE @ BotThink()|Clearing section -> Isbehaviour(bot dontgoprone)!\n", name);
		HudNotify(dm, true, this);
		//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#endif // DEBUG


	}

	// is there still behaviour don't go prone AND the bot already finished reloading his weapon
	// AND doesn't use bipod anymore AND finished merging magazines
	// AND the engine allows going prone on current location
	// AND bot has no enemy or his enemy is quite far from him
	if (IsBehaviour(BOT_DONTGOPRONE) && (weapon_action == W_READY) && (IsTask(TASK_BIPOD) == false) &&
		(f_bipod_time < gpGlobals->time) && (f_pause_time < gpGlobals->time) &&
		(IsGoingProne() == false) && (IsSubTask(ST_CANTPRONE) == false) &&
		((pBotEnemy == nullptr) || (pBotEnemy && ((pBotEnemy->v.origin - pEdict->v.origin).Length() > 1000.0f))))
	{
		// then it's time to remove such behaviour
		RemoveBehaviour(BOT_DONTGOPRONE);


#ifdef DEBUG
		//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@														// NEW CODE 094 (remove it)
		char dm[256];
		sprintf(dm, "%s REMOVED behaviour BOT DONTGOPRONE @ BotThink() -> clearing section\n", name);
		HudNotify(dm, true, this);
		//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#endif // DEBUG


	}

	// if the bot is proned
	if (IsBehaviour(BOT_PRONED))
	{
		//  then he CANNOT crouch
		if (IsBehaviour(GOTO_CROUCH))
			RemoveBehaviour(GOTO_CROUCH);

		// and he CANNOT go prone again
		if (IsBehaviour(GOTO_PRONE))
			RemoveBehaviour(GOTO_PRONE);
	}

	// if the bot is crouched
	if (IsBehaviour(BOT_CROUCHED))
	{
		// he CANNOT go prone
		if (IsBehaviour(GOTO_PRONE))
		{
			RemoveBehaviour(GOTO_PRONE);


#ifdef DEBUG
			//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@														// NEW CODE 094 (remove it)
			char dm[256];
			sprintf(dm, "%s REMOVED GOTO prone @ BotThink()|Clearing section -> Behaviour is BOT CROUCHED\n", name);
			HudNotify(dm, true, this);
			//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#endif // DEBUG
		}

		// he CANNOT go prone
		if (IsTask(TASK_GOPRONE))
		{
			RemoveTask(TASK_GOPRONE);


#ifdef DEBUG
			//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@														// NEW CODE 094 (remove it)
			char dm[256];
			sprintf(dm, "%s REMOVED TASK GoProne @ BotThink()|Clearing section -> Behaviour is BOT CROUCHED\n", name);
			HudNotify(dm, true, this);
			//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#endif // DEBUG
		}
	}

	// if the bot is standing
	if (IsBehaviour(BOT_STANDING))
	{
		// then he CANNOT be even more standing
		if (IsBehaviour(GOTO_STANDING))
		{
			RemoveBehaviour(GOTO_STANDING);



#ifdef DEBUG
			//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@														// NEW CODE 094 (remove it)
			char dm[256];
			sprintf(dm, "%s REMOVED GOTO standing @ BotThink()|Clearing section -> Behaviour is BOT STANDING\n", name);
			//HudNotify(dm, true, this);
			//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#endif // DEBUG
		}
	}

	//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
	//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
	//
	// END - various flags clearing section
	//
	//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
	//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

	// see if the bot is trying to resume from prone, but is missing the task to do so
	// This happens when the bot was stuck in prone, the obstacle blocking him is gone now and he is free again
	if (IsBehaviour(GOTO_STANDING) && IsBehaviour(BOT_PRONED) && !IsTask(TASK_GOPRONE))
		UTIL_GoProne(this, "BotThink() -> Behaviour is Proned + gotoStanding but NO TASK GoProne!");

	// see if bot has to go/resume prone AND NOT already doing so
	if (IsTask(TASK_GOPRONE) && (IsGoingProne() == FALSE))
	{
		// if the bot is proned then don't check for stuck for a little while
		// because FireArms stops the player (no forward speed so no position change)
		// when calling the prone command therefore we must disable unstuck routines
		// for next few frames otherwise the bot would incorrectly start to strafe
		if (pEdict->v.flags & FL_PRONED)
		{
			SetDontCheckStuck("Bot Think()|Task GOPRONE -> standing up", 0.5f);

#ifdef DEBUG
			//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@														// NEW CODE 094 (remove it)
			char dm[256];
			sprintf(dm, "%s called cmd 'prone' -> SET goproneTIME, cleared TASK GOPRONE and SET DontCheckSTUCK %0.1fs\n",
				name, (double)f_dont_check_stuck - gpGlobals->time);
			HudNotify(dm, true, this);
			//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#endif // DEBUG




		}
		else
		{

#ifdef DEBUG
			//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@														// NEW CODE 094 (remove it)
			char dm[256];
			sprintf(dm, "%s called cmd 'prone' -> SET goproneTIME and cleared TASK GOPRONE\n", name);
			HudNotify(dm, true, this);
			//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#endif // DEBUG



		}



		// call the client command
		FakeClientCommand(pEdict, "prone", nullptr, nullptr);

		// store the moment when the bot called the command
		f_go_prone_time = gpGlobals->time;

		// finally we have to reset the task
		RemoveTask(TASK_GOPRONE);
	}

	// make the body face the same way the bot is looking
	if (pEdict->v.movetype != MOVETYPE_FLY)
		pEdict->v.angles.y = pEdict->v.v_angle.y;//									NEW CODE 094 NOTE: (what ??? WHY???)
	//																				it's set like this at the end of
	//																				botthink() anyways so why this ???

	// is the bot still in duckjump OR has to be crouched?
	if ((f_duckjump_time > gpGlobals->time) || IsBehaviour(GOTO_CROUCH) ||
		(IsBehaviour(BOT_CROUCHED) && !IsBehaviour(GOTO_STANDING)))
	{
		pEdict->v.button |= IN_DUCK;		// then press the crouch key
	}

	// is there some crowded waypoint AND the bot doesn't sprint AND he is still quite fast
	// AND is not crouched or proned
	if ((crowded_wpt_index != -1) && !IsTask(TASK_SPRINT) && (move_speed > SPEED_SLOW) &&
		!IsWaypoint(crowded_wpt_index, WptT::wpt_crouch) && !IsWaypoint(crowded_wpt_index, WptT::wpt_prone))
	{
		// is the bot close to the waypoint
		if ((pEdict->v.origin - waypoints[crowded_wpt_index].origin).Length() < (WPT_RANGE * (float) 2))
		{
			// then slow down
			move_speed = SPEED_SLOW;


#ifdef _DEBUG
			//@@@@@@@@@@@@@
			//ALERT(at_console, "***botThink() - crowded wpt behaviour sets -> speed slow!!!\n");
#endif


		}
		// otherwise clear it
		else
			crowded_wpt_index = -1;
	}

	//																													NEW CODE 094
	// if the bot is trying to duckjump over something then we must keep full movement speed
	// we must have this statement here to override all previous 'slow down' commands
	if (f_duckjump_time > gpGlobals->time)
	{
		move_speed = SPEED_MAX;

		// we must also override all previously set times that disabled checking for
		// possible stuck because jumping can lead to a 'I'm stuck' situation
		SetDontCheckStuck("bot think() -> it's duckjump time", 0.4);


#ifdef _DEBUG
		//@@@@@@@@@@@@@
		//ALERT(at_console, "***botThink() - it's duckjump time -> max speed!!!\n");
#endif



	}
	// *************************************************************************************************************	NEW CODE 094 END

	// if the bot cannot move then his speed must be no speed
	if (f_dontmove_time > gpGlobals->time)
		move_speed = SPEED_NO;

	// save the previous speed (for checking if stuck)
	prev_speed = move_speed;

	// store previous health value (for bleeding checks)
	bot_prev_health = bot_health;

	// print some debugging info only about one bot
	if (g_debug_bot_on && (g_debug_bot == pEdict))
	{
		/*/
		char msg[255];

		if (pEdict->v.movetype == MOVETYPE_FLY)
		{
			sprintf(msg, "The bot is on ladder (Dir<1-up,2-down,0-uknown> %d | LadderStartT %.1f | GlobT %.1f)\n",
				ladder_dir, f_start_ladder_time, gpGlobals->time);
			HudNotify(msg);
		}

		if (f_reaction_time >= gpGlobals->time)
		{
			sprintf(msg, "(GlobTime:%.2f)The bot can't shoot (ReactionTime>GlobTime)\n",
				gpGlobals->time);
			HudNotify(msg);
		}

		if (f_reload_time >= gpGlobals->time)
		{
			sprintf(msg, "(GlobTime:%.2f)The bot can't shoot now (ReloadTime>GlobTime)\n",
				gpGlobals->time);
			HudNotify(msg);
		}

#ifdef _DEBUG
		if ((f_shoot_time > gpGlobals->time) && ((weapon_action == W_TAKEOTHER) || (weapon_action == W_INCHANGE)))
		{
			sprintf(msg, "(GlobTime:%.2f)The bot can't shoot because of weapon change\n",
				gpGlobals->time);
			HudNotify(msg);
			sprintf(msg, "NOTE If this message is printed over and over again for a long time then there seems to be a bug somewhere in weapon change code\n");
			HudNotify(msg);
			sprintf(msg, "The bot class is %d. Current weapn ID is %d. Forced weapon is %d. Report it on forums\n",
				bot_class, current_weapon.iId, forced_usage);
			HudNotify(msg);
		}
#endif
		/**/
		
		/*/
		sprintf(msg, "CurrZoom=%.0f | IsSec=%d | iuser3(Scope)=%d | WeaponSilencer=%d | FireMode=%d\n",
			pEdict->v.fov, secondary_active, pEdict->v.iuser3,
			current_weapon.iAttachment, current_weapon.iFireMode);
		/**/

		/*/
		sprintf(msg, "CurrSpeed=%.0f | PrevSpeed=%d | DistToCurrWpt=%.0f\n",
			pEdict->v.velocity.Length2D(), prev_speed,
			(pEdict->v.origin - wpt_origin).Length2D());
		/**/
		//HudNotify(msg);

		/*/
		if (pBotEnemy)
		{
			sprintf(msg, "Current enemy is %s\n", STRING(pBotEnemy->v.netname));
			HudNotify(msg);
		}
		/**/

		/*/
		if (botdebugger.IsDebugWeapons())
			UTIL_ShowWeapon(this);
		/**/
	}


	// is the bot fully crouched and doesn't heal his teammate?
	if ((pEdict->v.flags & FL_DUCKING) && !IsTask(TASK_HEALHIM) && !IsTask(TASK_BREAKIT) &&
		!IsTask(TASK_IGNOREAIMS) && !IsSubTask(ST_FACEITEM_DONE))
	{
		// look/aim directly forward
		pEdict->v.idealpitch = 0;

		BotFixIdealPitch(pEdict);
	}

	pEdict->v.v_angle.z = 0;  // reset roll to 0 (straight up and down)

	// set the body angles same as the bot head angles are
	// (ie to the direction bot is looking/aiming)
	pEdict->v.angles.x = -pEdict->v.v_angle.x / 3;
	pEdict->v.angles.y = pEdict->v.v_angle.y;
	pEdict->v.angles.z = pEdict->v.v_angle.z;


	// NOTE: by Frank - this should be moved to dll as any other global event (like autobalancing feature)
	extern DLL_FUNCTIONS other_gFunctionTable;
	if(externals.be_samurai == true && harakiri == true )
	{
		//ALERT(at_console, "bot DEFEATS and NEEDS to do harakiri in time=%f, now=%f\n", harakiri_moment, gpGlobals->time);
		if(harakiri_moment < gpGlobals->time) 
		{
			//this should be at console of all players IMHO.
			//ALERT(at_console, "bot has lose and now he is TRYING TO DO harakiri\n");
			(*other_gFunctionTable.pfnClientKill)(pEdict);
			return;
		}
	}

	g_engfuncs.pfnRunPlayerMove(pEdict, pEdict->v.v_angle, BotSetSpeed(), f_strafe_speed, 0, pEdict->v.button, 0, msecval);

	return;
}
