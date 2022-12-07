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
	bool IsHuman() const { return client_is_human; }
	void SetHuman(bool flag) { client_is_human = flag; }
	bool IsBleeding() const { return client_bleeds; }
	void SetBleeding(bool flag) { client_bleeds = flag; }
	void SetMaxSpeedTime(float time) { max_speed_time = time; }
	float GetMaxSpeedTime() const { return max_speed_time; }
	static int add_human() { return ++humans_num; }
	static int add_bot() { return ++bots_num; }
	static int substr_human() { return --humans_num; }
	static int substr_bot() { return --bots_num; }
	static int HumanCount() { return humans_num; }
	static int BotCount() { return bots_num; }
	static int ClientCount() { return (bots_num + humans_num); }
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
	void SetTeamsBalanceNeeded(bool newVal) { teams_balance_needed = newVal; }
	bool IsTeamsBalanceNeeded() const { return teams_balance_needed; }
	void ResetTeamsBalanceNeeded() { teams_balance_needed = false; }
	void SetOverrideTeamsBalance(bool newVal) { override_teams_balance = newVal; }
	bool IsOverrideTeamsBalance() const { return override_teams_balance; }
	void ResetOverrideTeamsBalance() { override_teams_balance = false; }
	void SetTimeOfTeamsBalanceCheck(float newVal) { time_of_teams_balance_check = newVal; }
	float GetTimeOfTeamsBalanceCheck() const { return time_of_teams_balance_check; }
	void ResetTimeOfTeamsBalanceCheck() { time_of_teams_balance_check = 0.0f; }
	void SetTeamsBalanceValue(int newVal) { teams_balance_value = newVal; }
	int GetTeamsBalanceValue() const { return teams_balance_value; }
	void ResetTeamsBalanceValue() { teams_balance_value = 0; }
	void SetListeServerFilling(bool newVal) { listenserver_filling = newVal; }
	bool IsListenServerFilling() const { return listenserver_filling; }
	void ResetListenServerFilling() { listenserver_filling = false; }
	void SetBotCheckTime(float newVal) { bot_check_time = newVal; }
	float GetBotCheckTime() const { return bot_check_time; }
	void ResetBotCheckTime() { bot_check_time = 0.0f; }
	void SetBotsToBeAdded(int newVal) { bots_to_be_added = newVal; }
	int GetBotsToBeAdded() const { return bots_to_be_added; }
	void ResetBotsToBeAdded() { bots_to_be_added = 0; }
	void DecreaseBotsToBeAdded() { bots_to_be_added--; }

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
	void SetObserverMode(bool newVal) { observer_mode = newVal; }
	bool IsObserverMode() const { return observer_mode; }
	void ResetObserverMode() { observer_mode = false; }
	void SetFreezeMode(bool newVal) { freeze_mode = newVal; }
	bool IsFreezeMode() const { return freeze_mode; }
	void ResetFreezeMode() { freeze_mode = false; }
	void SetDontShoot(bool newVal) { dont_shoot = newVal; }
	bool IsDontShoot() const { return dont_shoot; }
	void ResetDontShoot() { dont_shoot = false; }
	void SetIgnoreAll(bool newVal) { ignore_all = newVal; }
	bool IsIgnoreAll() const { return ignore_all; }
	void ResetIgnoreAll() { ignore_all = false; }
	void SetDebugAims(bool newVal) { debug_aims = newVal; }
	bool IsDebugAims() const { return debug_aims; }
	void ResetDebugAims() { debug_aims = false; }
	void SetDebugActions(bool newVal) { debug_actions = newVal; }
	bool IsDebugActions() const { return debug_actions; }
	void ResetDebugActions() { debug_actions = false; }
	void SetDebugCross(bool newVal) { debug_cross = newVal; }
	bool IsDebugCross() const { return debug_cross; }
	void ResetDebugCross() { debug_cross = false; }
	void SetDebugPaths(bool newVal) { debug_paths = newVal; }
	bool IsDebugPaths() const { return debug_paths; }
	void ResetDebugPaths() { debug_paths = false; }
	void SetDebugStuck(bool newVal) { debug_stuck = newVal; }
	bool IsDebugStuck() const { return debug_stuck; }
	void ResetDebugStuck() { debug_stuck = false; }
	void SetDebugWaypoints(bool newVal) { debug_waypoints = newVal; }
	bool IsDebugWaypoints() const { return debug_waypoints; }
	void ResetDebugWaypoints() { debug_waypoints = false; }
	void SetDebugWeapons(bool newVal) { debug_weapons = newVal; }
	bool IsDebugWeapons() const { return debug_weapons; }
	void ResetDebugWeapons() { debug_weapons = false; }

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
	void SetIsLogging(bool newVal) { is_logging = newVal; }
	bool GetIsLogging() const { return is_logging; }
	void ResetIsLogging() { is_logging = false; }	// will set it to deafult value
	void SetRandomSkill(bool newVal) { random_skill = newVal; }
	bool GetRandomSkill() const { return random_skill; }
	void ResetRandomSkill() { random_skill = false; }
	void SetSpawnSkill(int newVal) { spawn_skill = newVal; }
	int GetSpawnSkill() const { return spawn_skill; }
	void ResetSpawnSkill() { spawn_skill = 3; }
	void SetReactionTime(float newVal) { reaction_time = newVal; }
	float GetReactionTime() const { return reaction_time; }
	void ResetReactionTime() { reaction_time = 0.2f; }
	void SetBalanceTime(float newVal) { auto_balance_time = newVal; }
	float GetBalanceTime() const { return auto_balance_time; }
	void ResetBalanceTime() { auto_balance_time = 30.0f; }
	void SetMinBots(int newVal) { min_bots = newVal; }
	int GetMinBots() const { return min_bots; }
	void ResetMinBots() { min_bots = 2; }
	void SetMaxBots(int newVal) { max_bots = newVal; }
	int GetMaxBots() const { return max_bots; }
	void ResetMaxBots() { max_bots = 6; }
	void SetCustomClasses(bool newVal) { custom_classes = newVal; }
	bool GetCustomClasses() const { return custom_classes; }
	void ResetCustomClasses() { custom_classes = true; }
	void SetInfoTime(float newVal) { info_time = newVal; }
	float GetInfoTime() const { return info_time; }
	void ResetInfoTime() { info_time = 150.0f; }
	void SetPresentationTime(float newVal) { presentation_time = newVal; }
	float GetPresentationTime() const { return presentation_time; }
	void ResetPresentationTime() { presentation_time = 210.0f; }
	void SetDontSpeak(bool newVal) { dont_speak = newVal; }
	bool GetDontSpeak() const { return dont_speak; }
	void ResetDontSpeak() { dont_speak = false; }
	void SetDontChat(bool newVal) { dont_chat = newVal; }
	bool GetDontChat() const { return dont_chat; }
	void ResetDontChat() { dont_chat = false; }
	void SetRichNames(bool newVal) { rich_names = newVal; }
	bool GetRichNames() const { return rich_names; }
	void ResetRichNames() { rich_names = true; }
	void SetPassiveHealing(bool newVal) { passive_healing = newVal; }
	bool GetPassiveHealing() const { return passive_healing; }
	void ResetPassiveHealing() { passive_healing = false; }


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
	void ResetOnMapChange();
	void SetIsEnemyDistanceLimit(bool newVal) { is_enemy_distance_limit = newVal; }
	bool IsEnemyDistanceLimit() const { return is_enemy_distance_limit; }
	void ResetIsEnemyDistanceLimit() { is_enemy_distance_limit = false; }
	void SetEnemyDistanceLimit(float newVal) { enemy_distance_limit = newVal; }
	float GetEnemyDistanceLimit() const { return enemy_distance_limit; }
	void ResetEnemyDistanceLimit() { enemy_distance_limit = (float)7500; }

	void SetHUDMessageTime(float newVal) { hud_messsage_time = newVal; }
	float GetHUDMessageTime() const { return hud_messsage_time; }
	void ResetHUDMessageTime() { hud_messsage_time = 0.0f; }
	void SetTeamPlay(float newVal) { is_team_play = newVal; }
	float GetTeamPlay() const { return is_team_play; }
	void ResetTeamPlay() { is_team_play = 0.0f; }
	void SetTeamPlayChecked(bool newVal) { teamplay_checked = newVal; }
	bool IsTeamPlayChecked() const { return teamplay_checked; }
	void ResetTeamPlayChecked() { teamplay_checked = false; }
	void SetFASkillSystem(float newVal) { fa_skill_system = newVal; }
	float GetFASkillSystem() const { return fa_skill_system; }
	void ResetFASkillSystem() { fa_skill_system = 1.0f; }
	void SetSkillSystemChecked(bool newVal) { skillsystem_checked = newVal; }
	bool IsSkillSystemChecked() const { return skillsystem_checked; }
	void ResetSkillSystemChecked() { skillsystem_checked = false; }

	void SetIsCustomWaypoints(bool newVal) { is_custom_waypoints = newVal; }
	bool IsCustomWaypoints() const { return is_custom_waypoints; }
	void ResetIsCustomWaypoints() { is_custom_waypoints = false; }
	void SetWaypoitsAutoSave(bool newVal) { waypoints_autosave = newVal; }
	bool IsWaypointsAutoSave() const { return waypoints_autosave; }
	void ResetWaypointsAutoSave() { waypoints_autosave = false; }
	void SetPathToContinue(int newVal) { path_to_continue = newVal; }
	int GetPathToContinue() const { return path_to_continue; }
	void ResetPathToContinue() { path_to_continue = -1; }
	bool IsPathToContinue() const { return (path_to_continue != -1); }

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