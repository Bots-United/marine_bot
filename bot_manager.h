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
// bot_manager.h
// 
////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "extdll.h"


// the max number of clients in the game
#define MAX_CLIENTS			32

class client_t
{
public:
	client_t();
	inline bool IsHuman() { return client_is_human; }
	inline void SetHuman(bool flag) { client_is_human = flag; }
	inline bool IsBleeding() { return client_bleeds; }
	inline void SetBleeding(bool flag) { client_bleeds = flag; }
	inline void SetMaxSpeedTime(float time) { max_speed_time = time; }
	inline float GetMaxSpeedTime(void) { return max_speed_time; }
	inline int add_human() { return ++humans_num; }
	inline int add_bot() { return ++bots_num; }
	inline int substr_human() { return --humans_num; }
	inline int substr_bot() { return --bots_num; }
	inline int HumanCount() { return humans_num; }
	inline int BotCount() { return bots_num; }
	inline int ClientCount() { return (bots_num + humans_num); }
	edict_t* pEntity;		// pEntity

private:
	bool client_is_human;				// not a fakeclient ie. not a bot
	bool client_bleeds;
	float max_speed_time;		// to check if round ended or if it is only a single death

	// more client and bot globals can be stored here
	// for example current possition for all players (bot and human)

	static int humans_num;
	static int bots_num;
};

extern client_t clients[MAX_CLIENTS];

// used to manage desired bot counts on game server (dedicated server as well as listen server)
class botmanager_t
{
public:
	botmanager_t();
	inline void SetTeamsBalanceNeeded(bool newVal) { teams_balance_needed = newVal; }
	inline bool IsTeamsBalanceNeeded(void) { return teams_balance_needed; }
	inline void ResetTeamsBalanceNeeded(void) { teams_balance_needed = false; }
	inline void SetOverrideTeamsBalance(bool newVal) { override_teams_balance = newVal; }
	inline bool IsOverrideTeamsBalance(void) { return override_teams_balance; }
	inline void ResetOverrideTeamsBalance(void) { override_teams_balance = false; }
	inline void SetTimeOfTeamsBalanceCheck(float newVal) { time_of_teams_balance_check = newVal; }
	inline float GetTimeOfTeamsBalanceCheck(void) { return time_of_teams_balance_check; }
	inline void ResetTimeOfTeamsBalanceCheck(void) { time_of_teams_balance_check = (float)0.0; }
	inline void SetTeamsBalanceValue(int newVal) { teams_balance_value = newVal; }
	inline int GetTeamsBalanceValue(void) { return teams_balance_value; }
	inline void ResetTeamsBalanceValue(void) { teams_balance_value = (int)0; }
	inline void SetListeServerFilling(bool newVal) { listenserver_filling = newVal; }
	inline bool IsListenServerFilling(void) { return listenserver_filling; }
	inline void ResetListenServerFilling(void) { listenserver_filling = false; }
	inline void SetBotCheckTime(float newVal) { bot_check_time = newVal; }
	inline float GetBotCheckTime(void) { return bot_check_time; }
	inline void ResetBotCheckTime(void) { bot_check_time = (float)0.0; }
	inline void SetBotsToBeAdded(int newVal) { bots_to_be_added = newVal; }
	inline int GetBotsToBeAdded(void) { return bots_to_be_added; }
	inline void ResetBotsToBeAdded(void) { bots_to_be_added = (int)0; }
	inline void DecreaseBotsToBeAdded(void) { bots_to_be_added--; }

private:
	bool teams_balance_needed;			// teams don't have same counts of players so bots must change team to make the counts equal - by kota@
	bool override_teams_balance;		// stops teams balancing when reinforcements of one team reached zero
	float time_of_teams_balance_check;	// the time of last teams balance check
	int teams_balance_value;		// keeps teams balanced: <=0-teams balanced, >0&&<100-blue balance value, >100-red balance value
	bool listenserver_filling;		// allows automatically add bots to ListenServer
	float bot_check_time;			// time to next bot addition
	int bots_to_be_added;			// amount of bots we need to add (ie. "arg filling")
};

extern botmanager_t botmanager;

// class of variables used to debug bot behaviour
class botdebugger_t
{
public:
	botdebugger_t();
	inline void SetObserverMode(bool newVal) { observer_mode = newVal; }
	inline bool IsObserverMode(void) { return observer_mode; }
	inline void ResetObserverMode(void) { observer_mode = false; }
	inline void SetFreezeMode(bool newVal) { freeze_mode = newVal; }
	inline bool IsFreezeMode(void) { return freeze_mode; }
	inline void ResetFreezeMode(void) { freeze_mode = false; }
	inline void SetDontShoot(bool newVal) { dont_shoot = newVal; }
	inline bool IsDontShoot(void) { return dont_shoot; }
	inline void ResetDontShoot(void) { dont_shoot = false; }
	inline void SetIgnoreAll(bool newVal) { ignore_all = newVal; }
	inline bool IsIgnoreAll(void) { return ignore_all; }
	inline void ResetIgnoreAll(void) { ignore_all = false; }
	inline void SetDebugAims(bool newVal) { debug_aims = newVal; }
	inline bool IsDebugAims(void) { return debug_aims; }
	inline void ResetDebugAims(void) { debug_aims = false; }
	inline void SetDebugActions(bool newVal) { debug_actions = newVal; }
	inline bool IsDebugActions(void) { return debug_actions; }
	inline void ResetDebugActions(void) { debug_actions = false; }
	inline void SetDebugCross(bool newVal) { debug_cross = newVal; }
	inline bool IsDebugCross(void) { return debug_cross; }
	inline void ResetDebugCross(void) { debug_cross = false; }
	inline void SetDebugPaths(bool newVal) { debug_paths = newVal; }
	inline bool IsDebugPaths(void) { return debug_paths; }
	inline void ResetDebugPaths(void) { debug_paths = false; }
	inline void SetDebugStuck(bool newVal) { debug_stuck = newVal; }
	inline bool IsDebugStuck(void) { return debug_stuck; }
	inline void ResetDebugStuck(void) { debug_stuck = false; }
	inline void SetDebugWaypoints(bool newVal) { debug_waypoints = newVal; }
	inline bool IsDebugWaypoints(void) { return debug_waypoints; }
	inline void ResetDebugWaypoints(void) { debug_waypoints = false; }
	inline void SetDebugWeapons(bool newVal) { debug_weapons = newVal; }
	inline bool IsDebugWeapons(void) { return debug_weapons; }
	inline void ResetDebugWeapons(void) { debug_weapons = false; }

private:
	bool observer_mode;		// makes the player "invisible" for bots, but they will still see each other, but they will still see each other and act accordingly
	bool freeze_mode;		// bots don't move and do any actions
	bool dont_shoot;		// bots will NOT start firing at enemy, but any weapon method will still run
	bool ignore_all;		// bots will ignore all - they won't seek enemies so no combat actions, but they will move and do other actions
	bool debug_aims;		// allows listen server console printing about targeting current and available aim waypoints
	bool debug_actions;		// console printing about current action (eg. parachute use, weapon change) and regular checks like a check for ammunition reserves for example
	bool debug_cross;		// console printing about cross behaviour
	bool debug_paths;		// console printing when the bot is following a path
	bool debug_stuck;		// console printing informing about current unstuck action
	bool debug_waypoints;	// console printing about current waypoint the bot heads towards (when bot doesn't follow any path)
	bool debug_weapons;		// console printing about weapons that the bot spawned with
};

extern botdebugger_t botdebugger;

// class of variables that are available to be set externally in .cfg file
class externals_t
{
public:
	externals_t();
	inline void SetIsLogging(bool newVal) { is_logging = newVal; }
	inline bool GetIsLogging(void) { return is_logging; }
	inline void ResetIsLogging(void) { is_logging = false; }	// will set it to deafult value
	inline void SetRandomSkill(bool newVal) { random_skill = newVal; }
	inline bool GetRandomSkill(void) { return random_skill; }
	inline void ResetRandomSkill(void) { random_skill = false; }
	inline void SetSpawnSkill(int newVal) { spawn_skill = newVal; }
	inline int GetSpawnSkill(void) { return spawn_skill; }
	inline void ResetSpawnSkill(void) { spawn_skill = 3; }
	inline void SetReactionTime(float newVal) { reaction_time = newVal; }
	inline float GetReactionTime(void) { return reaction_time; }
	inline void ResetReactionTime(void) { reaction_time = (float)0.2; }
	inline void SetBalanceTime(float newVal) { auto_balance_time = newVal; }
	inline float GetBalanceTime(void) { return auto_balance_time; }
	inline void ResetBalanceTime(void) { auto_balance_time = (float)30.0; }
	inline void SetMinBots(int newVal) { min_bots = newVal; }
	inline int GetMinBots(void) { return min_bots; }
	inline void ResetMinBots(void) { min_bots = 2; }
	inline void SetMaxBots(int newVal) { max_bots = newVal; }
	inline int GetMaxBots(void) { return max_bots; }
	inline void ResetMaxBots(void) { max_bots = 6; }
	inline void SetCustomClasses(bool newVal) { custom_classes = newVal; }
	inline bool GetCustomClasses(void) { return custom_classes; }
	inline void ResetCustomClasses(void) { custom_classes = true; }
	inline void SetInfoTime(float newVal) { info_time = newVal; }
	inline float GetInfoTime(void) { return info_time; }
	inline void ResetInfoTime(void) { info_time = (float)150.0; }
	inline void SetPresentationTime(float newVal) { presentation_time = newVal; }
	inline float GetPresentationTime(void) { return presentation_time; }
	inline void ResetPresentationTime(void) { presentation_time = (float)210.0; }
	inline void SetDontSpeak(bool newVal) { dont_speak = newVal; }
	inline bool GetDontSpeak(void) { return dont_speak; }
	inline void ResetDontSpeak(void) { dont_speak = false; }
	inline void SetDontChat(bool newVal) { dont_chat = newVal; }
	inline bool GetDontChat(void) { return dont_chat; }
	inline void ResetDontChat(void) { dont_chat = false; }
	inline void SetRichNames(bool newVal) { rich_names = newVal; }
	inline bool GetRichNames(void) { return rich_names; }
	inline void ResetRichNames(void) { rich_names = true; }
	inline void SetPassiveHealing(bool newVal) { passive_healing = newVal; }
	inline bool GetPassiveHealing(void) { return passive_healing; }
	inline void ResetPassiveHealing(void) { passive_healing = false; }


	// These two are still public, they should go private, but I don't know if they are
	// going to be used at all, because the code behind these doesn't work

	bool  be_samurai;			// will allow bots to commit suicide in certain situations (for example when there are no reains and bots are "stuck" at opposite team spawn area, ie. neither team can win the game)
	float harakiri_time;		// the delay bots have to finish the game using standard ways (ie. killing the other team and so on) before starting to commit suicides

private:
	bool  is_logging;		// will allow logging MB events into default HL log file
	bool  random_skill;		// do we use default skill or randomly generated skill
	int   spawn_skill;		// default skill when there's no skill specified in recruit command
	float reaction_time;	// applied the first time the bot sees an enemy
	float auto_balance_time;// the time between two team balance tests
	int   min_bots;			// the minimal number of bots on DS (won't be kicked when clients join)
	int   max_bots;			// the maximal number of bots on DS
	bool  custom_classes;	// will allow custom classes from .cfg
	float info_time;		// the time for printing various info/summary to DS console
	float presentation_time;// the time between sending two presentation messages
	bool  dont_speak;		// the bot will or won't use Voice commands
	bool  dont_chat;		// the bot will or won't use say or say_team commands
	bool  rich_names;		// will allow '[MB]' sign being a part of a bot name
	bool  passive_healing;	// the bot will not heal teammates automatically when this is TRUE
};

extern externals_t externals;

// class of global variables that can be set only by using console command (ie. not via .cfg file)
// and some other internal general purpose variables
class internals_t
{
public:
	internals_t();
	void ResetOnMapChange(void);
	inline void SetIsEnemyDistanceLimit(bool newVal) { is_enemy_distance_limit = newVal; }
	inline bool IsEnemyDistanceLimit(void) { return is_enemy_distance_limit; }
	inline void ResetIsEnemyDistanceLimit(void) { is_enemy_distance_limit = false; }
	inline void SetEnemyDistanceLimit(float newVal) { enemy_distance_limit = (float)newVal; }
	inline float GetEnemyDistanceLimit(void) { return enemy_distance_limit; }
	inline void ResetEnemyDistanceLimit(void) { enemy_distance_limit = (float)7500; }

	inline void SetHUDMessageTime(float newVal) { hud_messsage_time = newVal; }
	inline float GetHUDMessageTime(void) { return hud_messsage_time; }
	inline void ResetHUDMessageTime(void) { hud_messsage_time = (float)0.0; }
	inline void SetTeamPlay(float newVal) { is_team_play = newVal; }
	inline float GetTeamPlay(void) { return is_team_play; }
	inline void ResetTeamPlay(void) { is_team_play = (float)0.0; }
	inline void SetTeamPlayChecked(bool newVal) { teamplay_checked = newVal; }
	inline bool IsTeamPlayChecked(void) { return teamplay_checked; }
	inline void ResetTeamPlayChecked(void) { teamplay_checked = false; }
	inline void SetFASkillSystem(float newVal) { fa_skill_system = newVal; }
	inline float GetFASkillSystem(void) { return fa_skill_system; }
	inline void ResetFASkillSystem(void) { fa_skill_system = (float)1.0; }
	inline void SetSkillSystemChecked(bool newVal) { skillsystem_checked = newVal; }
	inline bool IsSkillSystemChecked(void) { return skillsystem_checked; }
	inline void ResetSkillSystemChecked(void) { skillsystem_checked = false; }

	inline void SetIsCustomWaypoints(bool newVal) { is_custom_waypoints = newVal; }
	inline bool IsCustomWaypoints(void) { return is_custom_waypoints; }
	inline void ResetIsCustomWaypoints(void) { is_custom_waypoints = false; }
	inline void SetWaypoitsAutoSave(bool newVal) { waypoints_autosave = newVal; }
	inline bool IsWaypointsAutoSave(void) { return waypoints_autosave; }
	inline void ResetWaypointsAutoSave(void) { waypoints_autosave = false; }
	inline void SetPathToContinue(int newVal) { path_to_continue = newVal; }
	inline int GetPathToContinue(void) { return path_to_continue; }
	inline void ResetPathToContinue(void) { path_to_continue = (int)-1; }
	inline bool IsPathToContinue(void) { return (path_to_continue != (int)-1); }

private:
	bool  is_enemy_distance_limit;	// do we limit the view distance?, useful on maps where the bot can see enemy through skybox - ps_island
	float enemy_distance_limit;		// used to limit the view distance, bots won't see/attack enemies that are farther than this number

	float hud_messsage_time;		// to prevent overloading when displaying waypointing info on HUD
	float is_team_play;
	bool teamplay_checked;
	float fa_skill_system;			// how many skills are allowed at the spawn (1.0 means one skill)
	bool skillsystem_checked;		// did we checked skill system?

	bool is_custom_waypoints;		// allows loading custom waypoints (read from different folder)
	bool waypoints_autosave;		// allows waypoints automatic save
	int path_to_continue;			// index of path that is currently edited
};

extern internals_t internals;