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
// waypoint.h
// 
////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef WAYPOINT_H
#define WAYPOINT_H

#include <climits>

#define MAX_WAYPOINTS 2048		// the maximum of waypoints for a map

#define MIN_WPT_DIST 80.0		// used when auto waypointing - no other waypoint is added in this distance around any previously placed waypoint
#define MAX_WPT_DIST 400.0		// max distance between two waypoints bot can head toward to

#define WPT_RANGE 50.0			// default waypoint range ie radius around that waypoint

#define MIN_WPT_PRIOR 0			// using to test valid waypoint priority
#define MAX_WPT_PRIOR 5			// using to test valid waypoint priority


// flag types ie. fake waypoint flags
// to handle the problem with not enough space for all waypoint flags we (will/would) need
typedef enum class WaypointTypes
{
	scrapped_flagtype = 0,	// in case we would need to scrap any waypoint flag type
	wpt_normal,
	wpt_crouch,
	wpt_prone,
	wpt_jump,
	wpt_duckjump,
	wpt_sprint,
	wpt_ammobox,
	wpt_bandages,
	wpt_door,
	wpt_dooruse,
	wpt_ladder,
	wpt_cover,
	wpt_use,
	wpt_chute,
	wpt_sniper,
	wpt_mine,
	wpt_fire,
	wpt_aim,
	wpt_pushpoint,
	wpt_trigger,
	wpt_cross,
	wpt_goback,
	wpt_no,		// deleted
};
typedef WaypointTypes WptT;


// waypoints.flags constants (waypoint types)
#define W_FL_STD			(1<<0)	// normal/standing wpt (also used when autowaypointing)
#define W_FL_CROUCH			(1<<1)	// crouch wpt (must crouch to reach)
#define W_FL_PRONE			(1<<2)	// prone wpt (laying down)
#define W_FL_JUMP			(1<<3)	// jump wpt
#define W_FL_DUCKJUMP		(1<<4)	// jump wpt (press crouch and jump together)
#define W_FL_SPRINT			(1<<5)	// sprint wpt
#define W_FL_AMMOBOX		(1<<6)	// ammobox wpt
#define W_FL_BANDAGES		(1<<7)	// bandages laying here		// was unused before version 6
#define W_FL_DOOR			(1<<8)	// door wpt (wait for door to open)
#define W_FL_DOORUSE		(1<<9)	// door wpt (press "use" to open)
#define W_FL_LADDER			(1<<10)	// ladder wpt
#define W_FL_COVER			(1<<11)	// cover waypoint	// new addition with version 6
#define W_FL_USE			(1<<12)	// use wpt (press "use" to activate that)
#define W_FL_CHUTE			(1<<13)	// parachute jump (check if have chute if not turn back)
#define W_FL_SNIPER			(1<<14)	// sniper wpt (don't move while in combat)

#define W_FL_AVOID			(1<<15)	// NOT READY & NOT SURE ABOUT IT
#define W_FL_MINE			(1<<16)	// plant a claymore mine here

#define W_FL_TEAMMATE		(1<<17) // -- NOT READY YET -- wait here for teammate
#define W_FL_FIRE			(1<<18) // press weapon trigger (ie fire)
#define W_FL_AIMING			(1<<19)	// aim to this direction
#define W_FL_PUSHPOINT		(1<<20) // used to handle push point based map objectives	// new addition with version 6
#define W_FL_TRIGGER		(1<<21) // allow us to handle objective based map objectives (ie. obj_bocage)	// new addition with version 6

#define W_FL_CROSS			(1<<29)	// crossroad wpt (bot don't head directly to it, it's only a mark to start search code)
#define W_FL_GOBACK			(1<<30)	// turn back and continue in opposite direction (ie from this wpt back to start)

#define W_FL_DELETED		(1<<31)	// used by waypoint allocation code


//#define W_FL_TEAM_SPECIFIC (1<<25) // PREVIOUS BOTMANS SYSTEM - PROBABLY USELESS /* waypoint only for specified team */	// newly with version 6
//#define W_FL_LIFT        (1<<25)  /* wait for lift to be down before approaching this waypoint */
//#define W_FL_ARMOR       (1<<28)  /* armor (or HEV) location */
//#define W_FL_SNIPER      (1<<23) /* sniper waypoint (a good sniper spot) */

//#define W_FL_TFC_FLAG    (1<<24) /* flag position (or hostage or president) */
//#define W_FL_FLF_CAP     (1<<26) /* Front Line Force capture point */
//#define W_FL_FLF_DEFEND  (1<<18) /* Front Line Force defend point */


// last version was 6 in mb0.9b
#define WAYPOINT_VERSION 7	// actual waypoint version, must be changed when doing bigger
							// changes in waypointing system to prevent various bugs
							// like missing wpts, messed waypoint data etc.

// holds the version number of last waypoint system version
// used to detect known old system to allow MB to convert waypoints/paths
#define OLD_WAYPOINT_VERSION WAYPOINT_VERSION - 1

// define the structure for waypoints
typedef struct {
	int		flags;			// jump, crouch, button, lift, flag, ammo, etc.
	int		red_priority;	// 0-5 where 1 have highest priority for red team (0 - no priority, bot ingnores this waypoint)
	float	red_time;		// time the bot wait at this wpt for red team
	int		blue_priority;	// 0-5 where 1 have highest priority for blue team
	float	blue_time;		// time the bot wait at this wpt for blue team
	int		trigger_red_priority;	// 0-5 is being used only when trigger flag is set for this wpt and is trigger event
	int		trigger_blue_priority;
	int		trigger_event_on;		// the event that's triggering the waypoint with trigger flag
	int		trigger_event_off;		// the event that's resetting the waypoint with trigger flag back to normal
	float	range;			// circle around wpt where bot detects this waypoint
	Vector	origin;			// location of this waypoint in 3D space
} WAYPOINT;

// previous waypoint structure (used in cases we changed the structure itself)
// (this was used in version 6)
typedef struct {
	int		flags;			// jump, crouch, button, lift, flag, ammo, etc.
	int		red_priority;	// 0-5 where 1 have highest priority for red team (0 - no priority, bot ingnores this waypoint)
	float	red_time;		// time the bot wait at this wpt for red team
	int		blue_priority;	// 0-5 where 1 have highest priority for blue team
	float	blue_time;		// time the bot wait at this wpt for blue team
	int		trigger_red_priority;	// 0-5 is being used only when trigger flag is set for this wpt and is trigger even
	int		trigger_blue_priority;
	int		trigger_event_on;		// the event that's triggering the waypoint with trigger flag
	int		trigger_event_off;		// the event that's resetting the waypoint with trigger flag back to normal
	float	range;			// circle around wpt where bot detects this waypoint
	Vector	origin;			// location of this waypoint in 3D space
} OLD_WAYPOINT;

// define the waypoint file header structure
typedef struct {
   char filetype[8];			// should be "FAM_bot\0"
   int  waypoint_file_version;
   int  waypoint_file_flags;	// not currently used
   int  number_of_waypoints;
   char mapname[32];			// name of map for these waypoints
   char author[32];			// author signature - can't be modified
   char modified_by[32];	// signature of guy who modify them - could be changed
} WAYPOINT_HDR;


/*		NOT USED

// define the raw waypoint file header structure
typedef struct {
   char filetype[8];  // should be "FAM_bot\0"
   int  waypoint_file_version;
   int  transfer_flags;
   int  transfer_priority;
   int  transfer_time;
   int  transfer_class;
   int  number_of_waypoints;
   char mapname[32];  // name of map for these waypoints
   char author[32];
   char modified_by[32];
} RAW_FILE_HDR;

*/

// array of all waypoints for this map
extern WAYPOINT waypoints[MAX_WAYPOINTS];

// number of waypoints currently in use
extern int num_waypoints;

#define MAX_W_PATHS			512		// the maximum number of path for a map
#define AUTOADD_DISTANCE	10.0	// player must be this close to add a waypoint to the path


// constants used to highlight certain paths
// they must be negative because positive values are standard path indexs needed when we highlight one specific path
#define HIGHLIGHT_DISABLED		-1	// turned off
#define HIGHLIGHT_RED			-2
#define HIGHLIGHT_BLUE			-3
#define HIGHLIGHT_ONEWAY		-4
#define HIGHLIGHT_SNIPER		-5
#define HIGHLIGHT_MGUNNER		-6


// flag types ie. fake path flags
// to handle the problem with not enough space for all the flags we (will/would) need
typedef enum class WaypointPathTypes
{
	scrapped_flagtype = 0,	// in case we would need to scrap any path flag type
	path_both,
	path_red,
	path_blue,
	path_one,
	path_two,
	path_patrol,
	path_all,
	path_sniper,
	path_mgunner,
	path_miner,
	path_defender,
	path_ammo,
	path_bandages,
	path_turret,
	path_goal_red,
	path_goal_blue,
	path_avoid,
	path_ignore,
	path_gitem,
	path_danger,
};
typedef WaypointPathTypes PathT;


// team flag constants
#define P_FL_TEAM_NO			(1<<0)		// both teams use it
#define P_FL_TEAM_RED			(1<<1)
#define P_FL_TEAM_BLUE			(1<<2)

// way/direction constants
#define P_FL_WAY_ONE			(1<<5)		// one way (start-end)
#define P_FL_WAY_TWO			(1<<6)		// both way (start-end as well as end-start)
#define P_FL_WAY_PATROL			(1<<7)		// cycle (start-end-start)

// class flag constants
#define P_FL_CLASS_ALL			(1<<10)		// all bots use it
#define P_FL_CLASS_SNIPER		(1<<11)		// only bots using sniper rifle use it
#define P_FL_CLASS_MGUNNER		(1<<12)
#define P_FL_CLASS_MINER		(1<<13)		// the bot that's carrying a claymore mine
#define P_FL_CLASS_DEFENDER		(1<<14)		// the bot whose behaviour is defender

// misc flag constant
#define P_FL_MISC_AMMO			(1<<20)		// there's an ammobox waypoint on this path
#define P_FL_MISC_BANDAGES		(1<<21)		// there's a bandages waypoint on this path
#define P_FL_MISC_TURRET		(1<<22)		// there's a mounted gun by the use waypoint on this path
#define P_FL_MISC_GOAL_RED		(1<<23)		// there's a pushpoint waypoint on this path, red team needs to get there
#define P_FL_MISC_GOAL_BLUE		(1<<24)		// there's a pushpoint waypoint on this path, blue team needs to get there
#define P_FL_MISC_AVOID			(1<<25)		// the bot won't attack enemy that's outside current weapon effective range
#define P_FL_MISC_IGNORE		(1<<26)		// the bot will ignore enemy unless the enemy is right next to the bot
#define P_FL_MISC_GITEM			(1<<27)		// the bot will prefer this path whenever carrying the goal/captured item (case, flag, ammo ... whatever)
#define P_FL_MISC_DANGER		(1<<28)		// --UNDONE-- there's a danger of death fall somewhere on this path (designed for things like the bridge or the avanlanche spot on ps_coldwar)

// used to let the path algorithm know that bot should do a turn and continue in opposite direction
#define PATH_TURNBACK (W_FL_GOBACK | W_FL_AMMOBOX | W_FL_USE)


typedef struct w_path {
	int wpt_index;		// index of current waypoint
	int flags;
	struct w_path *prev;	// previous node in linked list
	struct w_path *next;	// next node in linked list
} W_PATH;

// previous path structure (used in cases we changed the structure itself)
// (this was used in version 6)
typedef struct old_w_path {
	int wpt_index;		// index of current waypoint
	int flags;
	struct w_path *prev;	// previous node in linked list
	struct w_path *next;	// next node in linked list
} OLD_W_PATH;

// define the paths file header structure
typedef struct {
   char filetype[8];			// should be "FAM_bot\0"
   int  waypoint_file_version;
   int  waypoint_flag;		// this must be same to WAYPOINT_HDR.number_of_waypoints
   int  number_of_paths;
   char mapname[32];			// name of map for these waypoints
   char author[32];			// author signature - can't be modified
   char modified_by[32];	// signature of guy who modify them - could be changed
} PATH_HDR;

// array of all paths for this map
extern W_PATH *w_paths[MAX_W_PATHS];

// number of paths currently in use
extern int num_w_paths;


// define the IDs for trigger event messages
// the order cannot be changed otherwise some
// conversion routines would return wrong data
typedef enum class WaypointTriggersIDs
{
	trigger_none = 0,
	trigger1,
	trigger2,
	trigger3,
	trigger4,
	trigger5,
	trigger6,
	trigger7,
	trigger8
};
typedef WaypointTriggersIDs TriggerId;

// max triggers we can use
#define MAX_TRIGGERS	8

// trigger event structure that's being saved into the waypoint files
typedef struct {
	TriggerId		name;			// it's ID
	char			message[256];
} TRIGGER_EVENT;

// trigger event array
extern TRIGGER_EVENT trigger_events[MAX_TRIGGERS];

// old trigger event structure (used for conversions)
// (this was used in version 6)
typedef struct {
	TriggerId		name;			// it's ID
	bool			used;
	char			message[256];
	bool			triggered;
} TRIGGER_EVENT_OLD;

// trigger event structure that's holding actual game state of the triggers
// this one is not being saved into the waypoint files, because these variables do change
// during the game so we don't need to save them
class trigger_event_gamestate_t
{
public:
	trigger_event_gamestate_t() { Init(); }
	void Init (void);
	void Reset (int i = -1);
	void SetName (TriggerId new_name) { name = new_name; }
	TriggerId GetName (void) const { return name; }
	void SetUsed (bool flag) { used = flag; }
	bool GetUsed (void) const { return used; }
	void SetTriggered (bool flag) { triggered = flag; }
	bool GetTriggered (void) const { return triggered; }
	void SetTime (float new_time = gpGlobals->time) { time = (float) new_time; }
	float GetTime (void) const { return (float) time; }

private:
	TriggerId		name;			// it's ID - the glue to the trigger structure
	bool			used;			// is this trigger being used?
	bool			triggered;		// did this event happened?
	float			time;			// the time when this event happened
};

// array of current game state of the trigger events
extern trigger_event_gamestate_t trigger_gamestate[MAX_TRIGGERS];

typedef struct {
	int wpt_index;			// the index of the waypoint
	int wpt_value;			// its value in terms of priority/usefulness over other waypoints
} WAYPOINT_VALUE;

// class of variables used to display waypoints, paths and connections
// basically tools to allow proper waypoints debugging 
class waypointsbrowser_t
{
public:
	waypointsbrowser_t();
	void ResetOnMapChange(void);
	void SetShowWaypoints(bool newVal) { waypoints_on = newVal; }
	bool IsShowWaypoints(void) const { return waypoints_on; }
	void ResetShowWaypoints(void) { waypoints_on = false; }
	void SetShowPaths(bool newVal) { paths_on = newVal; }
	bool IsShowPaths(void) const { return paths_on; }
	void ResetShowPaths(void) { paths_on = false; }
	void SetCheckAims(bool newVal) { check_aim_connections = newVal; }
	bool IsCheckAims(void) const { return check_aim_connections; }
	void ResetCheckAims(void) { check_aim_connections = false; }
	void SetCheckCross(bool newVal) { check_cross_connections = newVal; }
	bool IsCheckCross(void) const { return check_cross_connections; }
	void ResetCheckCross(void) { check_cross_connections = false; }
	void SetCheckRanges(bool newVal) { check_waypoints_ranges = newVal; }
	bool IsCheckRanges(void) const { return check_waypoints_ranges; }
	void ResetCheckRanges(void) { check_waypoints_ranges = false; }
	void SetAutoWaypointing(bool newVal) { auto_waypointing = newVal; }
	bool IsAutoWaypointing(void) const { return auto_waypointing; }
	void ResetAutoWaypointing(void) { auto_waypointing = false; }
	void SetAutoAddToPath(bool newVal) { auto_add_to_path = newVal; }
	bool IsAutoAddToPath(void) const { return auto_add_to_path; }
	void ResetAutoAddToPath(void) { auto_add_to_path = false; }
	void SetWaypointsDrawDistance(float newVal) { waypoints_draw_distance = newVal; }
	float GetWaypointsDrawDistance(void) const { return waypoints_draw_distance; }
	void ResetWaypointsDrawDistance(void) { waypoints_draw_distance = (float)800.0; }
	void SetAutoWaypointingDistance(float newVal) { auto_waypointing_distance = newVal; }
	float GetAutoWaypointingDistance(void) const { return auto_waypointing_distance; }
	void ResetAutoWaypointingDistance(void) { auto_waypointing_distance = (float)200.0; }
	void SetCompassIndex(int newVal) { waypoint_compass_index = newVal; }
	int GetCompassIndex(void) const { return waypoint_compass_index; }
	void ResetCompassIndex(void) { waypoint_compass_index = NO_VAL; }
	void SetPathToHighlight(int newVal) { path_to_highlight = newVal; }
	int GetPathToHighlight(void) const { return path_to_highlight; }
	void ResetPathToHighlight(void) { path_to_highlight = HIGHLIGHT_DISABLED; }
	bool IsPathToHighlightAPathIndex(void) const { return ((path_to_highlight >= 0) && (path_to_highlight < num_w_paths)); }

private:
	bool waypoints_on;				// to show waypoints
	bool paths_on;					// to show paths
	bool check_aim_connections;		// to show connections between waypoints with wait time and nearby aim waypoints
	bool check_cross_connections;	// to show connections between cross waypoint and all waypoints within its range
	bool check_waypoints_ranges;	// shows waypoint range using two thin beams intersecting in the middle of the waypoint
	bool auto_waypointing;			// allows to automatically add waypoints as the user moves around
	bool auto_add_to_path;			// allows to automatically add "touched" waypoint to current path
	float waypoints_draw_distance;	// max distance for a waypoint to show on screen, distance between user and waypoint
	float auto_waypointing_distance;// the distance between any two waypoints when auto waypointing
	int waypoint_compass_index;		// index of the waypoint the user is looking for
	int path_to_highlight;			// allows displaying of only certain paths or one specific path
};

extern waypointsbrowser_t wptser;

// waypoint function prototypes...
void WaypointInit(void);
int  WaypointFindNearest(edict_t *pEntity, float distance, int team);
bool WaypointReposition(edict_t *pEntity, int wpt_index);
void WaypointDelete(edict_t *pEntity);
void StartAutoWaypointg(bool switch_on);//															NEW CODE 094
void WaypointThink(edict_t *pEntity);
void SelfControlledWaypointReposition(float &the_range, Vector &new_origin, float move_d, float dec_r, bool dont_move, edict_t *pentIgnore);

bool  WaypointSubscribe(char *signature, bool author, bool enforced = false);
void  WaypointAuthors(char *author, char *modified_by);
void  WaypointDestroy(void);
void  WaypointResetTriggers(void);
int   WaypointAddTriggerEvent(const char *trigger_name, const char *trigger_message);
int   WaypointDeleteTriggerEvent(const char *trigger_name);
int   WaypointConnectTriggerEvent(edict_t *pEntity, const char *trigger_name, const char *state);
int   WaypointRemoveTriggerEvent(edict_t *pEntity, const char *state);
int	  WaypointValidatePath(int path_index);
void  WaypointValidatePath(void);
bool  WaypointScanPathForBadCombinationOfFlags(int path_index, PathT path_type, WptT waypoint_type);
int   WaypointRepairInvalidPathEnd(int path_index);
void  WaypointRepairInvalidPathEnd(void);
int   WaypointCheckInvalidPathEnd(int path_index, bool log_in_file);
int   WaypointRepairInvalidPathMerge(int path_index, bool repair_it = true, bool log_in_file = false);
void  WaypointRepairInvalidPathMerge(void);
//int	  WaypointCheckInvalidPathMerge(int path_index, bool log_in_file);				REPLACED BY THE REPAIR INVALID MERGE
int	  WaypointRepairSniperSpot(int path_index);
void  WaypointRepairSniperSpot(void);
float WaypointRepairCrossRange(int wpt_index);
void  WaypointRepairCrossRange(void);
int	  WaypointLoad(edict_t *pEntity, char *name);
bool  WaypointSave(const char *name);
bool  WaypointLoadUnsupported(edict_t *pEntity);
bool  WaypointLoadVersion5(edict_t *pEntity);
bool  WaypointAutoSave(void);
//bool  WaypointRawLoad(edict_t *pEntity, bool flags, bool priority, bool time, bool class_preference); // NOT USED
//void  WaypointRawSave(bool flags, bool priority, bool time, bool class_preference);	// NOT USED
int	  WaypointGetSystemVersion(void);
bool  IsFlagSet(int flag, int wpt_index, int team = TEAM_NONE);
int   WaypointGetPriority(int wpt_index, int team);
float WaypointGetWaitTime(int wpt_index, int team, const char* loc = nullptr);
int	  WaypointFindAvailable(bot_t *pBot);
int	  WaypointFindFirst(bot_t *pBot, float range, int skip_this_index);
int	  WaypointFindNearestAiming(bot_t *pBot, Vector *v_origin);
int	  WaypointSearchNewInRange(bot_t *pBot, int wpt_index, int curr_path);
bool  WaypointFindLastPatrol(bot_t *pBot);
void  DrawBeam(edict_t *pEntity, Vector start, Vector end, int life, int red, int green, int blue, int speed);
bool  IsWaypoint(int wpt_index, WptT flag_type);
char  *WptAdd(edict_t *pEntity, const char *wpt_type);
void  WaypointPrintInfo(edict_t *pEntity, const char *arg2, const char *arg3);
bool  WaypointCompass(edict_t *pEntity, const char *arg2);
void  WaypointPrintAllWaypoints(edict_t *pEntity);
void  WaypointTriggerPrintInfo(edict_t *pEntity);
char  *WptFlagChange(edict_t *pEntity, const char *arg2);
int   WptPriorityChange(edict_t *pEntity, const char *arg2, const char *arg3);
int	  WptTriggerPriorityChange(edict_t *pEntity, const char *priority, const char *for_team);
float WptTimeChange(edict_t *pEntity, const char *arg2, const char *arg3);
float WptRangeChange(edict_t *pEntity, const char *arg2);
bool  WptResetAdditional(edict_t *pEntity, const char* arg1, const char* arg2, const char* arg3, const char* arg4);

int   FindRightLadderWpt(bot_t *pBot);

// path functions
int	 FindPath(int current_wpt_index);
int  WaypointGetPathLength(int path_index);
int  WaypointInsertIntoPath(const char *arg2, const char *arg3, const char *arg4, const char *arg5);
bool WaypointCreatePath(edict_t *pEntity);
bool WaypointFinishPath(edict_t *pEntity);
bool WaypointPathContinue(edict_t *pEntity, int path_index);
bool WaypointAddIntoPath(edict_t *pEntity);
bool WaypointRemoveFromPath(edict_t *pEntity, int path_index);
int  WaypointSplitPath(edict_t *pEntity, const char *arg2, const char *arg3);
bool WaypointDeletePath(edict_t *pEntity, int path_index);
bool WaypointPathInfo(edict_t *pEntity, int path_index);
bool WaypointPrintAllPaths(edict_t *pEntity, int wpt_index);
bool WaypointPrintWholePath(edict_t *pEntity, int path_index);
bool WaypointResetPath(edict_t *pEntity, int path_index);
bool WaypointIsInPath(int wpt_index, int path_index);
bool IsPathPossible(bot_t *pBot, int path_index);
bool CheckPossiblePath(bot_t *pBot, int wpt_index);
int  WptPathChangeTeam(edict_t *pEntity, const char *new_value, int path_index);
int  WptPathChangeClass(edict_t *pEntity, const char *new_value, int path_index);
int  WptPathChangeWay(edict_t *pEntity, const char *new_value, int path_index);
int  WptPathChangeMisc(edict_t *pEntity, const char *new_value, int path_index);
bool WaypointMoveWholePath(int path_index, float , int coord);
int  WaypointPathLoad(edict_t *pEntity, char *name);
bool WaypointPathSave(const char *name);
bool WaypointPathLoadUnsupported(edict_t *pEntity);
bool WaypointPathLoadVersion5(edict_t *pEntity);

//SECTION BY kota@
W_PATH * FindPointInPath (int wpt_index, int path_index);
//int fillPointList(int *pl, int size, W_PATH *w_path_ptr, bool forward);	// not used
//bool SetPossiblePath(bot_t *pBot, int wpt_index);		// isn't used anywhere in the code
//SECTION BY kota@ END


#endif // WAYPOINT_H
