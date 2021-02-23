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
// bot.h
// 
////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef BOT_H
#define BOT_H

#include <cmath>

#include "Config.h"
#include "defines.h"

// stuff for Win32 vs. Linux builds

#if defined ( NEWSDKAM ) || defined ( NEWSDKVALVE )

#undef DLLEXPORT
#ifdef _WIN32
#define DLLEXPORT __stdcall
#else
#define DLLEXPORT __attribute__ ((visibility("default")))
#endif

#endif

#ifndef __linux__

typedef int (FAR *GETENTITYAPI)(DLL_FUNCTIONS *, int);
typedef int (FAR *GETNEWDLLFUNCTIONS)(NEW_DLL_FUNCTIONS *, int *);
typedef void (DLLEXPORT *GIVEFNPTRSTODLL)(enginefuncs_t *, globalvars_t *);
typedef void (FAR *LINK_ENTITY_FUNC)(entvars_t *);

#else

#include <dlfcn.h>
#define GetProcAddress dlsym

typedef int BOOL;

typedef int (*GETENTITYAPI)(DLL_FUNCTIONS *, int);
typedef int (*GETNEWDLLFUNCTIONS)(NEW_DLL_FUNCTIONS *, int *);
typedef void (*GIVEFNPTRSTODLL)(enginefuncs_t *, globalvars_t *);
typedef void (*LINK_ENTITY_FUNC)(entvars_t *);

#endif


extern int mod_id;
extern char mod_dir_name[32];

// define constants used to identify the MOD we are playing
#define FIREARMS_DLL	1


extern int g_mod_version;

// define constants used to identify the MOD version
#define FA_24			1
#define FA_25			2
#define FA_26			3
#define FA_27			4
#define FA_28			5
#define FA_29			6
#define FA_30			7

extern bool is_steam;

// used to test for existence (e.g. if bot doesn't have any grenade then the slot must use this "no value")
#define NO_VAL			-1

// for development debug file, this file is created in dll.cpp->GameDLLInit()
extern char debug_fname[256];

#define PUBLIC_DEBUG_FILE	"mb_error-log.txt"	// public debug file

// define constants used to sound confirmation
#define SND_DONE		0
#define SND_FAILED		1

// define constant used to check waypoint range
#define RANGE_SMALL		20.0

// define constant used to limit enemy search range while carrying a goal item
#define ENEMY_DIST_GOALITEM		200.0

// define constant used to check if the enemy is close enough to try a knife attack
#define RANGE_MELEE			300.0

// the maximum of names that can be used
#define MAX_BOT_NAMES 100

// max name lenght that can be used for bot (includes chars for '[MB]' tag)
#define BOT_NAME_LEN 32

#define MAX_BOT_WHINE 1				// NOT USED



// define some function prototypes...
BOOL ClientConnect( edict_t *pEntity, const char *pszName,
                    const char *pszAddress, char szRejectReason[ 128 ] );
void ClientPutInServer( edict_t *pEntity );
void ClientCommand( edict_t *pEntity );

void FakeClientCommand(edict_t *pBot, char *arg1, char *arg2, char *arg3);
void UTIL_BuildFileName(char *filename, char *arg1, char *arg2, bool separator);

const char *Cmd_Args();
const char *Cmd_Argv( int argc );
int Cmd_Argc();


// total bot difficulty levels (ie max number that can be used for skill arg when spawning bot)
#define BOT_SKILL_LEVELS	5

// pBot->clan_tag contants
#define NAMETAG_DONE		(1<<0)	// when the test for name tags is done
#define NAMETAG_MEDGUILD	(1<<1)	// }+{ tag behind name of bot that has medic1 or medic2 skill

// pBot->ladder_dir constants
#define LADDER_UNKNOWN	0
#define LADDER_UP		1
#define LADDER_DOWN		2

// pBot-> wander_dir constants
#define SIDE_LEFT		1
#define SIDE_RIGHT		2

// pBot->f_move_speed constants (keep this order otherwise some things may cause wrong behaviour)
#define SPEED_NO		1
#define SPEED_SLOWEST	2
#define SPEED_SLOW		3
#define SPEED_MAX		4

// pBot->respawn_state constants
#define RESPAWN_IDLE             1
#define RESPAWN_NEED_TO_RESPAWN  2
#define RESPAWN_IS_RESPAWNING    3

// game start message for Firearms
#define MSG_FA_IDLE						1
#define MSG_FA_TEAM_SELECT				2
#define MSG_FA_CLASS_SELECT				3
//#define MSG_FA_GEAR_SELECT			4	// not used it caused problems during the spawn
#define MSG_FA_SKILL_SELECT				5
#define MSG_FA_INTO_BATTLE				6
#define MSG_FA_MOTD_WINDOW				7
#define MSG_FA25_BASE_ARMOR_SELECT		10
#define MSG_FA25_ARMOR_SELECT			11
#define MSG_FA25_ITEMS_SELECT			12
#define MSG_FA25_SIDEARMS_SELECT		13
#define MSG_FA25_SHOTGUNS_SELECT		14
#define MSG_FA25_SMGS_SELECT			15
#define MSG_FA25_RIFLES_SELECT			16
#define MSG_FA25_SNIPES_SELECT			17
#define MSG_FA25_MGUNS_SELECT			18
#define MSG_FA25_GLS_SELECT				19


// pBot->behaviour constants
// behaviour types
#define STANDARD		(1<<0)		// standard play
#define ATTACKER		(1<<1)		// more aggressive play (short wait times etc.)
#define DEFENDER		(1<<2)		// more defensive play (longer wait times)
// additional behaviour info based on weapon
#define COMMON			(1<<10)		// common rifle
#define CQUARTER		(1<<11)		// bot with SMG & shotgun
#define MGUNNER			(1<<12)		// bot with machinegun
#define SNIPER			(1<<13)		// bot with sniper rifle
// to handle bot stance/position
#define BOT_DONTGOPRONE	(1<<25)		// temporary prevents going prone in order to allow successful weapon reload, merge magazines etc.
#define GOTO_STANDING	(1<<26)		// should stand up
#define GOTO_CROUCH		(1<<27)		// should go to crouch
#define GOTO_PRONE		(1<<28)		// should go to prone
#define BOT_STANDING	(1<<29)		// bot is in standing position
#define BOT_CROUCHED	(1<<30)		// bot is fully crouched
#define BOT_PRONED		(1<<31)		// bot is in prone

// pBot->bot_tasks constants
// some of these tasks aren't really tasks, they are more of flags or something, but having
// things set this way is probably better than having dozens of unique bool variables
#define TASK_PROMOTED			(1<<0)	// the bot has been promoted to higher rank (has to take some skills)
#define TASK_DEATHFALL			(1<<1)	// the bot is tracelining forward direction to detect deep pits
#define TASK_DONTMOVEINCOMBAT	(1<<2)	// the bot is forced to stay on one place (ie. not moving single step forward)
#define TASK_BREAKIT			(1<<3)	// trying to destroy breakable object to free his way forward
#define TASK_FIRE				(1<<4)	// the bot has to use primary fire out of combat mode
#define TASK_IGNOREAIMS			(1<<5)	// the bot will ignore aim wpts ie. won't target them
#define TASK_PRECISEAIM			(1<<6)	// the bot will try to face the aim wpt really accurately (useful for breakables)
#define TASK_CHECKAMMO			(1<<7)	// the bot has to check his ammo reserves so he know how many mags should he take from ammobox
#define TASK_GOALITEM			(1<<8)	// the bot is carrying a goal item
#define TASK_NOJUMP				(1<<9)	// the bot isn't allowed to jump, stamina is low (under 30)
#define TASK_SPRINT				(1<<10)	// the bot has to sprint
#define TASK_WPTACTION			(1<<11)	// the bot is asked for doing some waypoint based action (use something)
#define TASK_BACKTOPATROL		(1<<12)	// the bot has to return back on patrol path (being set when getting to combat while on patrol path)
#define TASK_PARACHUTE			(1<<13)	// the bot has a parachute pack
#define TASK_PARACHUTE_USED		(1<<14)	// the bot has opened the parachute
#define TASK_BLEEDING			(1<<15)	// the bot is bleeding
#define TASK_DROWNING			(1<<16)	// the bot is drowning
#define TASK_HEALHIM			(1<<17)	// the bot is going to heal someone (someone called for medic)
#define TASK_MEDEVAC			(1<<18)	// the bot has to treat downed teammate
#define TASK_FIND_ENEMY			(1<<19)	// the bot has to find different enemy then the one he has right now
#define TASK_USETANK			(1<<20)	// the bot is using a mounted gun (ie TANK)
#define TASK_BIPOD				(1<<21)	// the bot just used the bipod on current weapon (ie. can't move, limited pitch&yaw)
#define TASK_CLAY_IGNORE		(1<<22)	// the bot has to ignore this claymore
#define TASK_CLAY_EVADE			(1<<23)	// the bot has to evade this claymore
#define TASK_GOPRONE			(1<<24)	// the bot has to go prone
#define TASK_AVOID_ENEMY		(1<<25)	// the bot has to ignore distant enemies
#define TASK_IGNORE_ENEMY		(1<<26)	// the bot has to ignore all enemies except those who are right next to him
// other possible tasks (ie. change current code to use it)
//#define TASK_SETCLAYMORE

// pBot->bot_subtasks constants
#define ST_AIM_GETAIM		(1<<0)	// the bot has an order to pick one aim wpt from his aim array
#define ST_AIM_FACEIT		(1<<1)	// to face his current aim waypoint
#define ST_AIM_SETTIME		(1<<2)	// to count the time for current aim wpt
#define ST_AIM_ADJUSTAIM	(1<<3)	// to check if he's really facing current aim
#define ST_AIM_DONE			(1<<4)	// is facing current aim waypoint
#define ST_FACEITEM_DONE	(1<<5)	// is facing pItem entity
#define ST_BUTTON_USED		(1<<6)	// the bot just successfully used some usable object
#define ST_MEDEVAC_ST		(1<<7)	// trying to medevac corpse (aiming to its stomach)
#define ST_MEDEVAC_H		(1<<8)	// trying to medevac corpse (aiming to its head)
#define ST_MEDEVAC_F		(1<<9)	// trying to medevac corpse (aiming to its feet)
#define ST_MEDEVAC_DONE		(1<<10)	// the bot successfully finished the medevac
#define ST_TREAT			(1<<11)	// the bot is trying to stop patient's bleeding
#define ST_HEAL				(1<<12)	// the bot is trying to restore patient's health
#define ST_GIVE				(1<<13)	// the bot is trying to give bandages to his patient
#define ST_HEALED			(1<<14)	// the bot finished the medical treatment
#define ST_SAY_CEASEFIRE	(1<<15)	// the bot has to say a cease fire team message
#define ST_FACEENEMY		(1<<16) // the bot is currently facing his enemy (also possible new enemy)
#define ST_RESETCLAYMORE	(1<<17)	// the bot has to reset claymore usage because he wasn't able to place it correctly (ie. was too close to some object)
#define ST_DOOR_OPEN		(1<<18) // the bot is passing doors
#define ST_TANK_SHORT		(1<<19) // the bot is forced to use the mounted gun only for given time period (ie. waypoint wait time is set)
#define ST_RANDOMCENTRE		(1<<20)	// we need to randomize the final point the bot is aiming at (ie. a small range around the original point), useful if the bot has to target a breakable object that has a hole in its middle
//#define ST_CROUCHBREAK		(1<<21)	// the bot is crouched so try to use only origin istead of origin+head to send the traceline forward
#define ST_NOOTHERNADE		(1<<21)	// checked that the bot doesn't carry more than one grenade type
#define ST_W_CLIP			(1<<22)	// the bot has to check how much ammo is left in the clip
#define ST_CANTPRONE		(1<<23)	// bot cannot go prone ... engine prevents it due to being too close to a wall for example
#define ST_EVASIONSTARTED	(1<<24)	// bot started one evasion action (eg. will jump over claymore)

//pBot->bot_needs constants
#define NEED_AMMO			(1<<0)	// the bot has low ammo and needs to take some more
#define NEED_GOAL			(1<<1)	// the bot is in mood for reaching the map objective
#define NEED_GOAL_NOT		(1<<2)	// the bot isn't in mood for reaching any objective
#define NEED_NEXTWPT		(1<<3)	// the bot needs to get next waypoint right at the moment he can call navigation again
#define NEED_RESETNAVIG		(1<<4)	// the bot needs to reset current waypoint and path
#define NEED_RESETPARACHUTE (1<<31) // the bot needs to check if parachute is finally gone (for FA 2.65 and below)

// pBot->bot_fa_skill
// skills
#define MARKS1		(1<<1)
#define MARKS2		(1<<2)
#define NOMEN		(1<<3)
#define BATTAG		(1<<4)
#define LEADER		(1<<5)
#define FAID		(1<<6)
#define MEDIC		(1<<7)
#define ARTY1		(1<<8)
#define ARTY2		(1<<9)
#define STEALTH		(1<<10)

// constants used to detect the status of edict->v.fov
// (to detect scoped weapons zoom level in FA2.8 and lower)
#define NO_ZOOM		90.0	// standard view
#define ZOOM_05X	50.0	// anaconda and g36e
#define ZOOM_1X		20.0	// ssg3000 and dragunov (also m82)
#define ZOOM_2X		10.0	// m82

// constants used to detect the status of edict->v.flags (those not in const.h, but specific to FA)
#define FL_PRONED		(1<<27)		// player is fully proned (or goes to prone)
#define FL_BROKENLEG	(1<<28)		// player has broken his leg (slower movement)

// constants used to detect the status of edict->v.iuser3
#define USR3_SLOWED			(1<<0)		// no stamina -> slow speed
#define USR3_LOWSTAMINA		(1<<1)		// low stamina -> can't jump
#define USR3_INZOOM			(1<<2)		// scope used
#define USR3_STEALTH		(1<<4)		// stealth skill taken -> silent moves, silencers
//#define USR2_BIPOD			(1<<5)		// no need
#define USR3_NVGON			(1<<6)		// night vision goggles on
#define USR3_CANTSHOOT		(1<<7)		// can't shoot while moving message
#define USR3_CHUTEOPEN		(1<<9)		// when the chute was used (is open now)
// backward compatibility constants (FA28 and lower)
#define USR3_STEALTH_GE		(1<<6)

// pBot->weapon_action constants
#define W_READY				1		// ready to shoot
#define W_TAKEOTHER			2		// change to another weapon
#define W_INCHANGE			3		// in process of taking other weapon
#define W_INHANDS			4		// the weapon change is almost finished (current weapon message sent appropriate ID)
#define W_INRELOAD			5		// reloading it
#define W_INMERGEMAGS		6		// merging magazines
//#define W_INMOUNT					// mounting silencer	// not used yet

// pBot->clay_action & pBot->grenade_action constants
#define ALTW_NOTUSED		1
#define ALTW_TAKING			2
#define ALTW_PREPARED		3
#define ALTW_PLACED			4
#define ALTW_GOALPLACED		5
#define ALTW_USED			6

// pBot->weapon_status constants
#define WS_ALLOWSILENCER	(1<<0)
#define WS_MOUNTSILENCER	(1<<1)
#define WS_SILENCERUSED		(1<<2)
#define WS_USEAKIMBO		(1<<3)
#define WS_PRESSRELOAD		(1<<4)	// to know that bot just "pressed reload button" (set IN_RELOAD to Edict->v.button)
#define WS_INVALID			(1<<5)	// weapon based action (e.g. reloading) was invalidated by another action (e.g. someone started to heal this bot)
#define WS_NOTEMPTYMAG		(1<<6)	// when the bot is going to reload weapon with not completely empty magazine (used to detect the need to merge magazines)
#define WS_MERGEMAGS1		(1<<7)	// first almost empty magazine was reloaded
#define WS_MERGEMAGS2		(1<<8)	// second almost empty magazine was reloaded (now the bot needs to merge magazines)
#define WS_CANTBIPOD		(1<<9)	// when bipod cannot be deployed (e.g. bot is standing in open area)

// pBot->forced_usage constants
#define USE_MAIN		1
#define USE_BACKUP		2
#define USE_KNIFE		3
#define USE_GRENADE		4

// used to detect if bot is close enough to any entity bot searched
#define PLAYER_SEARCH_RADIUS		70.0
#define EXTENDED_SEARCH_RADIUS		300.0
#define FIND_ITEM_RADIUS			200.0

// constants used to recognize in which team the bot is
#define TEAM_NONE	-1
#define TEAM_RED	 1
#define TEAM_BLUE	 2

// pBot->current_weapon.iFireMode constants
#define FM_NONE			0
#define FM_SEMI			1
#define FM_BURST		2
#define FM_AUTO			4

typedef enum class FireMode_WTest {dont_test_weapons = 0, test_weapons = 1};

// constants used to specify text message the bot has to say
#define SAY_GRENADE_IN			1
#define SAY_GRENADE_OUT			2
#define SAY_CLAYMORE_FOUND		3
#define SAY_MEDIC_HELP_YOU		4
#define SAY_MEDIC_CANT_HELP		5
#define SAY_CEASE_FIRE			6


// visibility return constants
#define VIS_NO			0		// not visible (like bool FALSE)
#define VIS_YES			1		// visible (like bool TRUE)
#define VIS_FENCE		2		// visible but behind fence like object so only big caliber gun can shoot through


typedef struct
{
   int	isActive;	// 1 if this weapon is hands
   int  iId;		// weapon ID
   int  iClip;		// amount of ammo in the clip
   int	iAttachment;// supressor or gl (0-normal, 1-silenced) || (ammo2 in chamber)
   int	iFireMode;	// fire mode (1 - semi || 2 - 3rburst || 4 - auto || 0 - where no choice)
   int  iAmmo1;		// amount of ammo in primary reserve (i.e. mags)
   int  iAmmo2;		// amount of ammo in secondary reserve (i.e. mags)
} bot_current_weapon_t;

const int ROUTE_LENGTH = 1;//32;// NOTE: This is code by kota@ - I'm not going to use it, because it looks like he simply recreated the same thing that botman used for paths. Will remove it one day.

class HistoryPoint
{
public:
	HistoryPoint() { index = -1; next = prev = nullptr; }
	int index;
	HistoryPoint *next;
	HistoryPoint *prev;
};

/*
  kota@
  circle wpt history.
*/
class WptHistory 
{
private:
	HistoryPoint *start, *end;
	int size;
public:
	WptHistory()
	{
		size=5;
		start = end = new HistoryPoint;
		
		for (int i=0; i<size; ++i) 
		{
			HistoryPoint* cur = new HistoryPoint;
			cur->next = end;
			end->prev = cur;
			end = cur;
		}
		end->prev = start;
		start->next = end;
		start=end;
	}
	~WptHistory()
	{
		HistoryPoint* cur = start;
		while (cur->next == start){
			HistoryPoint* tmp = cur;
			cur = cur->next;
			delete tmp;
		}
		delete cur; //the last element.
		end = start = nullptr;
	}

	static void print(){
#ifdef _DEBUG
		HistoryPoint *cur;
		//FILE *f = fopen("\\debug1.txt", "a+");
		//fprintf(f, "==*= %p print list\n", this);
		//fclose(f);
		cur = start;
		do {
			//f = fopen("\\debug1.txt", "a+");
			//fprintf(f, "cur->prev=%p, cur =%p, cur->next=%p\n", cur->prev, cur, cur->next);
			//fclose(f);
			cur = cur->next;
		} while (cur!=start);
		//f = fopen("\\debug1.txt", "a+");
		//fprintf(f, "===========\n");
		//fclose(f);
#endif	
	}

	int get(int j=0) const
	{
		HistoryPoint *cur = start;
		for (int i=0; i<j && cur!=end; ++i)
		{
			cur = cur->next;
		}
		return (cur!=end) ? cur->index : -1;
	}

	void clear() {
		end = start;
		end->index = -1;
	}

	void push(int wpt_index) {
		start = start->prev;
		start->index = wpt_index;
		if (start==end)
		{
			end = end->prev;
		}
		
	}
	bool check(int wpt_index) const
	{
		HistoryPoint *cur = start;
		while (cur != end) 
		{
			if (cur->index == wpt_index)
			{
				return true;
			}
			cur = cur->next;
		}
		return false;
	}
};

class bot_t
{
public:
	bot_t();
	void BotSpawnInit();
	void BotThink();
	void BotStartGame();
	void BotPressFireToRespawn();
	void BotSpeak(int msg_type, const char *target = nullptr);
	void CheckMedicGuildTag();
	void BotDoMedEvac(float distance);
	float BotSetSpeed() const;
	bool UpdateSounds(edict_t *pPlayer);
	void FacepItem();
	void ResetAims(const char* loc = nullptr);
	void TargetAimWaypoint(const char* loc = nullptr);
	void BotWaitHere();
	void ReloadWeapon(const char* loc = nullptr);
	void SetReloadTime();
	bool CheckMainWeaponOutOfAmmo(const char* loc = nullptr);
	bool CheckBackupWeaponOutOfAmmo(const char* loc = nullptr);
	void DecideNextWeapon(const char* loc = nullptr);
	void BotSelectMainWeapon(const char* loc = nullptr);
	edict_t *BotFindEnemy();
	bool IsIgnorePath(int path_index = -1);
	int GetNextWaypoint() const;
	float GetEffectiveRange(int weapon_index = -1) const;
	void BotForgetEnemy();

	void clear_path()// NOTE: This is code by kota@ - I'm not going to use it, because it looks like he simply recreated the same thing that botman used for paths. Will remove it one day.
	{
		point_list[0] = -1;
	};

	void SetBehaviour(int behaviour)
	{
		bot_behaviour |= behaviour;
	};
	void RemoveBehaviour(int behaviour)
	{
		if (bot_behaviour & behaviour)
			bot_behaviour &= ~behaviour;
	};

	bool IsBehaviour(int behaviour) const
	{
		return (bot_behaviour & behaviour) == behaviour;
	}

	void SetTask(int task)
	{
		bot_tasks |= task;
	};
	void RemoveTask(int task)
	{
		if (bot_tasks & task)
			bot_tasks &= ~task;
	};

	bool IsTask(int task) const
	{
		return (bot_tasks & task) == task;
	}

	void SetSubTask(int subtask)
	{
		bot_subtasks |= subtask;
	};
	void RemoveSubTask(int subtask)
	{
		if (bot_subtasks & subtask)
			bot_subtasks &= ~subtask;
	};

	bool IsSubTask(int subtask) const
	{
		return (bot_subtasks & subtask) == subtask;
	}

	void SetNeed(int need)
	{
		bot_needs |= need;
	};
	void RemoveNeed(int need)
	{
		if (bot_needs & need)
			bot_needs &= ~need;
	};

	bool IsNeed(int need) const
	{
		return (bot_needs & need) == need;
	}

	void SetDontMoveTime(float time)
	{
		f_dontmove_time = gpGlobals->time + time;
	};

	void ClearDontMoveTime()
	{
		f_dontmove_time = gpGlobals->time;
	};

	void IncChangedDirection()
	{
		changed_direction = changed_direction + 1;
	};

	void ResetChangedDirection()
	{
		changed_direction = 0;
	};

	void SetPauseTime(float time = (float) 0.0)
	{
		f_pause_time = gpGlobals->time + time;
	};
	void ClearPauseTime(float time = (float) 0.0)
	{
		if (time == 0.0)
			f_pause_time = time;
		else
			f_pause_time = gpGlobals->time - static_cast<float>(std::fabs(time));
	};

	void SetWaitTime(float time = (float) 0.0)
	{
		wpt_wait_time = gpGlobals->time + time;
	};
	void ClearWaitTime(float time = (float) 0.0)
	{
		if (time == 0.0)
			wpt_wait_time = time;
		else
			wpt_wait_time = gpGlobals->time - static_cast<float>(std::fabs(time));
	};

	void UseWeapon(int weapon = USE_KNIFE)
	{
		forced_usage = weapon;
	}

	bool NotSeenEnemyfor(float time = (float) 0.0) const
	// not seen enemy for (time) seconds
	{
		return ((f_bot_see_enemy_time > 0) && ((f_bot_see_enemy_time + time) < gpGlobals->time));
	}
	void SetDontCheckStuck(const char* loc = nullptr, float time = (float)1.0);
	// true when the bot is going to/resume from prone
	bool IsGoingProne() const
	{
		// time from calling the command to a moment when FA allows firing from the gun (based on test in FA 3.0)
		// battlefield agility doesn't have any effect on this time (despite the fact that they say so)
		return (f_go_prone_time + 1.2 > gpGlobals->time);
	}

	void SetWeaponStatus(int status)
	{
		weapon_status |= status;
	}
	void RemoveWeaponStatus(int status)
	{
		if (weapon_status & status)
			weapon_status &= ~status;
	};

	bool IsWeaponStatus(int status) const
	{
		return (weapon_status & status) == status;
	}


	bool is_used;
	int respawn_state;
	edict_t *pEdict;
	bool need_to_initialize;
	char name[BOT_NAME_LEN+1];
	int clan_tag;		// bit map of tags the bot could wear
	int not_started;
	int start_action;
	float kick_time;
	float create_time;

// TheFatal - START
	int msecnum;
	float msecdel;
	float msecval;
// TheFatal - END

	int bot_skill;		// used in rate of fire, search for enemy, etc.
	int aim_skill;		// used in target off-sets

	int bot_team;
	int bot_class;
	Section *pcust_class;	// pointer to a class in custom_classes array
	int bot_skin;		// face/skin the bot uses (camo, asian ...)
	int bot_health;
	int bot_armor;
	int bot_weapons;	// bit map of weapons the bot is carrying
	int bot_behaviour;	// bit map, determines bots behaviour (checking also main weapon), NOT being cleared at respawn
	int bot_tasks;		// bit map, stores short time tasks (is being cleared at each respawn)
	int bot_subtasks;	// bit map, stores actions based on tasks (cleared at respawn)
	int bot_needs;		// bit map, stores things the bot currently needs, like need ammo etc. (cleared at respawn)
	float idle_angle;
	bool  round_end;	// TRUE when all flags are captured, reached goal etc.
	float blinded_time;

	int	  move_speed;
	float f_max_speed;
	int	  prev_speed;
	float prev_time;
	Vector v_prev_origin;
	float f_dontmove_time;		// the bot will not move during this time

	edict_t *pItem;	// pointer on item bot is trying to use (mounted gun for example) or pickup or destroy
	float f_face_item_time;		// time the bot needs to turn directly to the item

	int	  ladder_dir;			// 0 = unknown, 1 = upwards, 2 = downwards
	float f_start_ladder_time;
	bool  waypoint_top_of_ladder;
	int	  end_wpt_index;			// index of wpt on other end of the ladder

	float f_wall_check_time;
	float f_wall_on_right;
	float f_wall_on_left;
	float f_dont_avoid_wall_time;
	float f_dont_look_for_waypoint_time;
	//float f_jump_time;
	float f_duckjump_time;			// duckjump time
	float f_dont_check_stuck;
	float f_stuck_time;				// prevents long time stuck - bots do suicide
	int	  changed_direction;		// holds the number of times the bot turned to different direction trying to unstuck
	float f_check_deathfall;		// time of last check for deep pits in front of the bot

	int   wander_dir;
	float f_strafe_direction;  // 0 = none, negative = left, positive = right
	float f_strafe_time;
	float hide_time;		// stay hidden (mostly crouched) during this time

	Vector wpt_origin;
	Vector prev_wpt_origin;
	float  f_waypoint_time;
	int	   curr_wpt_index;
	WptHistory prev_wpt_index;
	int	   crowded_wpt_index;		// wpt with very small range < 20
	//int	   waypoint_goal;		// --- NOT USED YET ---
	//float  f_waypoint_goal_time;	// --- NOT USED YET ---
	//bool   waypoint_near_flag;	// --- NOT USED YET --- & NOT SURE IF NEEDED
	//Vector waypoint_flag_origin;	// --- NOT USED YET --- & NOT SURE IF NEEDED
	float  prev_wpt_distance;
	float  wpt_action_time;		// time the bot needs to do specified wpt action
	float  wpt_wait_time;		// time taken from current waypoint the bot will wait (camp) at this wpt
	float  f_face_wpt;			// time to face current/some wpt (ie to get the wpt in front of bot)

	int  curr_path_index;	// path index the bot is on/following
	int  prev_path_index;	// index of the last path bot was on; to prevent using the same path again and again
	bool opposite_path_dir;	// TRUE if bot follows the path in opposite direction (i.e. path end -> path start)
	int  patrol_path_wpt;	// stores last visited wpt on PATROL path for after combat return purposes

	edict_t *pBotEnemy;
	float	f_bot_see_enemy_time;
	float	f_bot_find_enemy_time;
	edict_t *pBotPrevEnemy;		// holds the pointer to current enemy when the bot is try to find another enemy
	float	f_bot_wait_for_enemy_time;	// bot will wait for this time to see if his enemy become visible again
	Vector	v_last_enemy_position;		// backup of the last known enemy position, to keep looking to that direction when the bot lost direct visibily of his enemy
	float	f_prev_enemy_dist;			// previous frame distance to enemy to check if enemy is moving
	float	f_reaction_time;			// delay between spotting the enemy and starting to fight back

	edict_t *pBotUser;
	float	f_bot_use_time;
	float	bot_spawn_time;

//								NOT USED YET
//	edict_t *killer_edict;
//	bool	b_bot_say_killed;
//	float	f_bot_say_killed;

	int	  aim_index[4];			// max of four aim waypoints
	int	  curr_aim_index;		// index of the aim waypoint the bot is currently aiming at (watching)
	float f_aim_at_target_time;	// time the bot is aiming at one target (ie. aim waypoint)
	float f_check_aim_time;		// time to check if the bot is really aiming at current aim waypoint
	int	  targeting_stop;		// safety stop for a case the bot isn't able to face current aim waypoint

	int   weapon_action;		// holds a flag of current weapon state (e.g. ready to fire or reloaded or changed)
	int   weapon_status;		// bitmap of current weapon status (e.g. silencer mounted etc.), it's NOT being reset at bot respawn
	float f_shoot_time;			// time the bot last fired (used also in weapon change)
	float f_fullauto_time;		// time the bot keeps the fire button pressed
	float f_reload_time;		// time needed to reload current weapon

	float f_pause_time;			// for additional waiting events like waiting for medic etc.
	float f_sound_update_time;	// time of the last sound check (e.g. sound of footsteps of possible enemy)
	float f_look_for_ground_items;	// time to check for placed claymores, fallen teammates etc.
	Vector v_ground_item;		// holds the location of the item (claymore, corpse etc.)

	int    claymore_slot;		// holds ID of the mine or NO_VAL if not equipped
	float  f_use_clay_time;
	int	   clay_action;			// holds a flag of last claymore action

	//bool  b_use_button;		// NOT USED
	//float f_use_button_time;	// NOT USED
	//bool  b_lift_moving;		// NOT USED

	//bool	b_use_capture;		// NOT SURE IF NEEDED --- NOT USED --- (PROBABLY FOR FRONTLINE)
	//float	f_use_capture_time;	// NOT SURE IF NEEDED --- NOT USED --- (PROBABLY FOR FRONTLINE)
	//edict_t *pCaptureEdict;	// NOT SURE IF NEEDED --- NOT USED --- (PROBABLY FOR FRONTLINE)

	Vector	v_door;				// door coords

	int	bot_fa_skill;			// bit map of fa skill the bot has (marks1, arty1, leadership, field med etc.)

	float f_bipod_time;			// time needed to handle bipod deploying or folding
	float chute_action_time;	// to check if the parachute was used or should be used

	int   bot_prev_health;		// for bleeding checks
	int	  bot_bandages;			// bandages ammount the bot have
	float bandage_time;			// time needed to fully bandage
	float f_medic_treat_time;	// the time bot needs to treat the wounded soldier

	float f_cant_prone;		// the time the bot will not try to go prone
	float f_go_prone_time;		// time needed to correctly finish going to/resuming from proned position

	float search_closest_time;	// handle search for closest enemy
	float speak_time;			// the time bot used one of radio&voice commands
	float text_msg_time;		// the time bot used say or say_team command
	int   prev_msg;				// holds the last message the bot said

	int   main_weapon;		// primary weapon - used most of the time, NO_VAL means not equipped
	int   backup_weapon;	// usually handgun, NO_VAL == not equipped
	int	  forced_usage;		// 1 main weapon, 2 backup, 3 knife, 4 any granade
	int	  grenade_slot;		// hold ID of the grenade the bot has, NO_VAL means not equipped
	float grenade_time;		// for handling bot grenade usage
	int   grenade_action;	// holds a flag of last grenade action (e.g. selecting it, priming it etc.)
	bool  secondary_active;	// TRUE when weapon is in secondary fire mode (usually zoomed)
	float sniping_time;		// stop and fire for this period of time
	bool  main_no_ammo;		// TRUE when no ammo for main weapon
	bool  backup_no_ammo;	// TRUE when no ammo for backup weapon

	float check_ammunition_time;	// holds the last time we checked the ammo reserves
	int   take_main_mags;			// number of mags for main weapon that the bot should take from ammobox
	int   take_backup_mags;			// number of mags for backup weapon that the bot should take from ammobox

	float f_combat_advance_time;	// if this time > globaltime bot moves towards his enemy
	float f_overide_advance_time;	// time to override advance time to allow bot to move even if not time to do so, used when the bot is out of effective range for current weapon
	float f_check_stance_time;		// time to check whether the bot can change stance (standing/crouched/prone) and keep direct visibility to current enemy
	float f_stance_changed_time;	// holds the time when the bot last changed his stance

	bot_current_weapon_t current_weapon;  // one current weapon for each bot
	int curr_rgAmmo[MAX_AMMO_SLOTS];	// total ammo amounts (1 array for each bot)

	int point_list[ROUTE_LENGTH];// NOTE: This is code by kota@ - I'm not going to use it, because it looks like he simply recreated the same thing that botman used for paths. Will remove it one day.
	//PointList point_list;	// current bot's route - chain of wpts
	
	bool harakiri;	// TRUE when the bot should commit suicide as a result of end game events (ie. no reins)

	// NOTE: by Frank
	// this variable should be changed to a global var., it has nothing to do with certain bot
	// it's only a global event timer same as timer for autobalancing checks
	static float harakiri_moment; //when they should do harakiri.


#ifdef _DEBUG
	Vector curr_aim_offset;		// debug stuff
	Vector target_aim_offset;		// debug stuff
	bool is_forced;					// debug stuff
	int forced_stance;				// debug stuff
#endif
};

extern bot_t *bots;

typedef struct
{
	char name[BOT_NAME_LEN+1];		// the name
	bool is_used;					// is already used or is still free
} botname_t;

typedef struct
{
	int		iId;					// the weapon ID value
	float	min_safe_distance;   // 0 - no, usefull for snipes, M79 and grenades (when attachment M203/GP25 set it as max effective and the min will be counted in FireMain... decreased by say 300)
	float	max_effective_distance;   // 9999 - no maximum
} bot_weapon_select_t;

extern bot_weapon_select_t bot_weapon_select[MAX_WEAPONS];

typedef struct
{
	int		iId;
	float	primary_base_delay;
	float	primary_min_delay[BOT_SKILL_LEVELS];
	float	primary_max_delay[BOT_SKILL_LEVELS];
} bot_fire_delay_t;

extern bot_fire_delay_t bot_fire_delay[MAX_WEAPONS];

typedef enum class MarineBotMessageType {msg_null = 0, msg_default, msg_info, msg_warning, msg_error};
typedef MarineBotMessageType MType;

#endif // BOT_H

