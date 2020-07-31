//////////////////////////////////////////////////////////////////////////////////////////////
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
// waypoint.cpp
// 
////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(WIN32)
#pragma warning(disable: 4005 91)
#endif

#ifndef __linux__
#include <io.h>
#endif
#include <fcntl.h>
#ifndef __linux__
#include <sys\stat.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "defines.h"

#include "extdll.h"
#include "enginecallback.h"
#include "util.h"
#include "cbase.h"

#include "bot.h"
#include "bot_func.h"
#include "bot_manager.h"
#include "waypoint.h"

#define RANGE_20_WPT (W_FL_AMMOBOX | W_FL_DOOR | W_FL_DOORUSE | W_FL_JUMP | W_FL_DUCKJUMP | \
						W_FL_LADDER | W_FL_USE)

extern int m_spriteTexture;
extern int m_spriteTexturePath1;
extern int m_spriteTexturePath2;
extern int m_spriteTexturePath3;


// waypoints with information bits (flags)
WAYPOINT waypoints[MAX_WAYPOINTS];

// number of waypoints currently in use
int num_waypoints;

// declare the array of paths
W_PATH *w_paths[MAX_W_PATHS];

// number of w_paths currently in use (must be < MAX_W_PATHS)
int num_w_paths;

// declare the array of trigger events and the array of current game state of the events
TRIGGER_EVENT trigger_events[MAX_TRIGGERS];
trigger_event_gamestate_t trigger_gamestate[MAX_TRIGGERS];

waypointsbrowser_t wptser;

// set all waypoints browser variables to defaults
waypointsbrowser_t::waypointsbrowser_t()
{
	ResetShowWaypoints();
	ResetShowPaths();
	ResetCheckAims();
	ResetCheckCross();
	ResetCheckRanges();
	ResetAutoWaypointing();
	ResetAutoAddToPath();
	ResetWaypointsDrawDistance();
	ResetAutoWaypointingDistance();
	ResetCompassIndex();
	ResetPathToHighlight();
};

// reset all waypoints browser variables to default values on map change
// to prevent overloading game engine
void waypointsbrowser_t::ResetOnMapChange(void)
{
	ResetShowWaypoints();
	ResetShowPaths();
	ResetCheckAims();
	ResetCheckCross();
	ResetCheckRanges();
	ResetAutoWaypointing();
	ResetAutoAddToPath();
	ResetWaypointsDrawDistance();
	ResetAutoWaypointingDistance();
	ResetCompassIndex();
	ResetPathToHighlight();
};


char wpt_author[32];
char wpt_modified[32];

// variables used only in this file
static FILE *fp;
Vector last_waypoint;								// for autowaypointing
bool g_waypoint_paths = FALSE;						// have any paths been allocated?
float wp_display_time[MAX_WAYPOINTS];				// time that this waypoint was displayed (while editing)
float f_path_time[MAX_W_PATHS];						// time that this path was diplayed (while editing)
float f_conn_time[MAX_WAYPOINTS][MAX_WAYPOINTS];	// time that the connections (cross&aim) were displayed while editing
float f_ranges_display_time[MAX_WAYPOINTS];			// time that this waypoint range was displayed (while editing)
float f_compass_time;								// time that the compass was displayed

// functions prototypes used in this file
void WaypointDebug(void);
void FreeAllPaths(void);
void FreeAllTriggers(void);
void WaypointInit(int wpt_index);
float GetWaypointDistance(int wpt1_index = -1, int wpt2_index = -1);
bool IsMessageValid(char *msg);
int WaypointReturnTriggerWaypointPriority(int wpt_index, int team);
int TriggerNameToIndex(const char *trigger_name);
TriggerId TriggerNameToId(const char *trigger_name);
TriggerId TriggerIndexToId(int i);
void TriggerIntToName(int id, char *trigger_name);
TriggerId IntToTriggerId(int i);
int TriggerIdToInt(TriggerId triggerId);
int IsPathOkay(int path_index);
int MergePaths(int path1_index = -1, int path2_index = -1, bool reverse_order = false);
int MergePathsInverted(int path1_index = -1, int path2_index = -1, bool reverse_order = false);
int WaypointFixInvalidFlags(int wpt_index);
void UpdatePathStatus(int path_index);
void UpdateGoalPathStatus(int wpt_index, int path_index);
int GetPathStart(int path_index);
int GetPathEnd(int path_index);
int GetPathPreviousWaypoint(int wpt_index, int path_index);
bool IsPath(int path_index, PathT path_type);
bool PathMatchingFlag(int path_index1, int path_index2, PathT path_type);
bool PathCompareTeamDownwards(int checked_path, int standard_path);
bool PathCompareClassDownwards(int checked_path, int standard_path);
int ContinueCurrPath(int current_wpt_index, bool check_presence = true);
int ExcludeFromPath(int current_wpt_index, int path_index);
int ExcludeFromPath(W_PATH *p = NULL, int path_index = -1);
bool DeleteWholePath(int path_index);
W_PATH *GetWaypointPointer(int wpt_index, int path_index);
W_PATH *GetWaypointTypePointer(WptT flag_type, int path_index);
bool WaypointTypeIsInPath(WptT flag_type, int path_index);
void WaypointSetValue(bot_t *pBot, int wpt_index, WAYPOINT_VALUE *wpt_value);
bool IsWaypointPossible(bot_t *pBot, int wpt_index);
int WaypointFindNearestType(edict_t *pEntity, float range, int type);
int WaypointFindNearestCross(const Vector &source_origin);
int WaypointFindNearestCross(const Vector &source_origin, bool see_through_doors);
int WaypointFindNearestStandard(int end_waypoint, int path_index);
int WaypointFindAimingAround(int source_waypoint);
int Wpt_CountFlags(int wpt_index);
WptT WaypointAdd(const Vector position, WptT wpt_type);
int PathClassFlagCount(int path_index);
void GetPathClass(int path_index, char *the_class);
bool WaypointReachable(Vector v_srv, Vector v_dest, edict_t *pEntity);
void WptGetType(int wpt_index, char *the_type);
void SetWaypointSize(int wpt_index, Vector &start, Vector &end);
Vector SetWaypointColor(int wpt_index);
int SetPathTexture(int path_index);
Vector SetPathColor(int path_index);


/*
* returns true if the path passed by path_index has an ignore enemy flag
*/
bool bot_t::IsIgnorePath(int path_index)
{
	if (path_index == -1)
		path_index = curr_path_index;

	// this will handle passing through a cross waypoint the bot won't remove the ignore enemy flag in this case,
	// but will wait for next path to decide if the flag should or should not be removed
	if (IsWaypoint(curr_wpt_index, WptT::wpt_cross) || IsWaypoint(prev_wpt_index.get(0), WptT::wpt_cross))
		return TRUE;

	if (IsPath(path_index, PathT::path_ignore))
		return TRUE;

	return FALSE;
}


/*
* returns the next waypoint index from a path the bot's using
*/
int bot_t::GetNextWaypoint(void)
{
	if ((curr_wpt_index == -1) || (curr_path_index == -1))
		return -1;

	W_PATH *cur_w_path = w_paths[curr_path_index];

	while (cur_w_path)
	{
		if (cur_w_path->wpt_index == curr_wpt_index)
		{
			// the bot is using the path start to end so return next
			if (!opposite_path_dir && (cur_w_path->next))
				return cur_w_path->next->wpt_index;

			// the bot is using the path from end to start so return previous
			if (opposite_path_dir && (cur_w_path->prev))
				return cur_w_path->prev->wpt_index;
		}

		cur_w_path = cur_w_path->next;
	}

	return -1;
}


void trigger_event_gamestate_t::Init(void)
{
	SetName(TriggerId::trigger_none);
	SetUsed(false);
	SetTriggered(false);
	SetTime(0.0);
}


void trigger_event_gamestate_t::Reset(int i)
{
	if (i == -1)
		SetName(TriggerId::trigger_none);
	else
		SetName(TriggerIndexToId(i));
	SetTriggered(false);
	SetTime(0.0);
}


void WaypointDebug(void)
{
   int y = 1, x = 1;

   fp=fopen(debug_fname,"a");
   fprintf(fp,"WaypointDebug: LINKED LIST ERROR!!!\n");
   fclose(fp);

   x = x - 1;  // x is zero
   y = y / x;  // cause an divide by zero exception

   return;
}


/*
* free all paths
*/
void FreeAllPaths(void)
{
	for (int path_index = 0; path_index < MAX_W_PATHS; path_index++)
	{
		int safety_stop = 0;		// to stop it if encounters an error

		if (w_paths[path_index])
		{
			W_PATH *p = w_paths[path_index];	// set the pointer to the head node
			W_PATH *p_next;

			while (p)
			{
				p_next = p->next;	// save the link to next
				free(p);			// free this node
				p = p_next;			// update the head node

#ifdef _DEBUG
				safety_stop++;
				if (safety_stop > 1000)
					WaypointDebug();
#endif
			}

			w_paths[path_index] = NULL;
		}
	}
}


/*
* free all trigger event messages
*/
void FreeAllTriggers(void)
{
	for (int trigger_index = 0; trigger_index < MAX_TRIGGERS; trigger_index++)
	{
		trigger_events[trigger_index].name = TriggerIndexToId(trigger_index);
		trigger_events[trigger_index].message[0] = '\0';
		// we also have to initialize the current game state of the triggers
		trigger_gamestate[trigger_index].Init();
	}
}


/*
* add wpts author signature or sig of guy who modify them into file depending on bool author
* authors sig can't be changed
*/
bool WaypointSubscribe(char *signature, bool author, bool enforced)
{
	char mapname[64];
	char filename[256];
	WAYPOINT_HDR header;

	strcpy(mapname, STRING(gpGlobals->mapname));
	strcat(mapname, ".wpt");

	if (internals.IsCustomWaypoints())
		UTIL_MarineBotFileName(filename, "customwpts", mapname);
	else
		UTIL_MarineBotFileName(filename, "defaultwpts", mapname);

	FILE *bfp = fopen(filename, "rb");

	if (bfp != NULL)
	{
		fread(&header, sizeof(header), 1, bfp);

		// is it authors sig
		if (author)
		{
			// is wpt file NOT subcribed OR do we force it so access granted to change wpt_author (no file writing)
			if ((strcmp(header.author, "") == 0) || (strcmp(header.author, "unknown") == 0) || enforced)
			{
				strcpy(wpt_author, signature);
				wpt_author[31] = 0;		// must be ended properly

				fclose(bfp);
				return TRUE;
			}
			// otherwise access denied
			else
			{
				fclose(bfp);
				return FALSE;
			}
		}
		// sig of guy who modify them (no file writing)
		else
		{
			if (FStrEq(signature, "clear"))
			{
				wpt_modified[0] = '\0';		// wipe the modified by variable
			}
			else
			{
				strcpy(wpt_modified, signature);
				wpt_modified[31] = 0;		// must be ended properly
			}

			fclose(bfp);
			return TRUE;
		}
	}
	else
	{
		return FALSE;
	}
}


/*
* read waypoints author signature as well as waypoint modifier signature from waypoint file header
*/
void WaypointAuthors(char *author, char *modified_by)
{
	char mapname[64];
	char filename[256];
	WAYPOINT_HDR header;

	strcpy(mapname, STRING(gpGlobals->mapname));
	strcat(mapname, ".wpt");

	if (internals.IsCustomWaypoints())
		UTIL_MarineBotFileName(filename, "customwpts", mapname);
	else
		UTIL_MarineBotFileName(filename, "defaultwpts", mapname);

	FILE* bfp = fopen(filename, "rb");

	if (bfp != NULL)
	{
		fread(&header, sizeof(header), 1, bfp);

		// get the author signature if there's any written in the file header
		if ((strcmp(header.author, "") == 0) || (strcmp(header.author, "unknown") == 0))
			strcpy(author, "noauthor");		// there's NO author tag in this waypoint file
		else
			strcpy(author, header.author);
		// get the signature of guy who modified them if exists
		if ((strcmp(header.modified_by, "") == 0) || (strcmp(header.modified_by, "unknown") == 0))
			strcpy(modified_by, "nosig");	// waypoint file is NOT subscribed
		else
			strcpy(modified_by, header.modified_by);

		fclose(bfp);
	}
	// waypoint file doesn't exist
	else
	{
		strcpy(author, "nofile");
		strcpy(modified_by, "nofile");
	}
}


/*
* initialize just one waypoint
*/
void WaypointInit(int wpt_index)
{
	waypoints[wpt_index].flags = W_FL_DELETED;
	waypoints[wpt_index].red_priority = MAX_WPT_PRIOR;									// lowest priority
	waypoints[wpt_index].red_time = 0.0;												// no "stay here" time
	waypoints[wpt_index].blue_priority = MAX_WPT_PRIOR;
	waypoints[wpt_index].blue_time = 0.0;
	waypoints[wpt_index].trigger_red_priority = MAX_WPT_PRIOR;
	waypoints[wpt_index].trigger_blue_priority = MAX_WPT_PRIOR;
	waypoints[wpt_index].trigger_event_on = TriggerIdToInt(TriggerId::trigger_none);	// no trigger event
	waypoints[wpt_index].trigger_event_off = TriggerIdToInt(TriggerId::trigger_none);
	waypoints[wpt_index].range = WPT_RANGE;												// default range
	waypoints[wpt_index].origin = g_vecZero;//Vector(0, 0, 0);										// located at map origin
}


/*
* initialize the waypoint structures
*/
void WaypointInit(void)
{
	int i;

	// initialize the trigger event messages
	FreeAllTriggers();

	// have any waypoint path nodes been allocated yet?
	if (g_waypoint_paths)
	{
		FreeAllPaths();
	}

	// destroy all waypoints with all their flags, priorities etc.
	for (i=0; i < MAX_WAYPOINTS; i++)
	{
		WaypointInit(i);

		wp_display_time[i] = 0.0;
		f_ranges_display_time[i] = 0.0;

		// fill also the "sub array"
		for (int j = 0; j < MAX_WAYPOINTS; j++)
			f_conn_time[i][j] = 0.0;
	}

	// prepare w_paths array
	for (int path_index = 0; path_index < MAX_W_PATHS; path_index++)
	{
		f_path_time[path_index] = 0.0;	// reset path display time
	}

	num_waypoints = 0;
	num_w_paths = 0;

	last_waypoint = g_vecZero;//Vector(0,0,0);
}


/*
* returns distance between two waypoints passed by their indexes
* if either of the indexes isn't valid it will return 9999.0
*/
float GetWaypointDistance(int wpt1_index, int wpt2_index)
{
	if ((wpt1_index == -1) || (wpt2_index == -1))
		return 9999.0;

	return (waypoints[wpt1_index].origin - waypoints[wpt2_index].origin).Length();
}


/*
* clear all waypoints & paths
* remove both signatures
* wipe both files from the HDD
*/
void WaypointDestroy(void)
{
	char mapname[64];
	char filename[256];

	WaypointInit();
	
	// reset author's signature
	strcpy(wpt_author, "unknown");
	wpt_author[31] = 0;
	// reset modified_by signature
	strcpy(wpt_modified, "unknown");
	wpt_modified[31] = 0;

	strcpy(mapname, STRING(gpGlobals->mapname));
	strcat(mapname, ".wpt");

	// build the filename/filepath for this file
	if (internals.IsCustomWaypoints())
		UTIL_MarineBotFileName(filename, "customwpts", mapname);
	else
		UTIL_MarineBotFileName(filename, "defaultwpts", mapname);

	// and then erase it from HDD
	remove(filename);

	strcpy(mapname, STRING(gpGlobals->mapname));
	strcat(mapname, ".pth");
	
	if (internals.IsCustomWaypoints())
		UTIL_MarineBotFileName(filename, "customwpts", mapname);
	else
		UTIL_MarineBotFileName(filename, "defaultwpts", mapname);

	remove(filename);

	return;
}


/*
* checks the message for double space or new line characters as these would invalidate the string comparison
*/
bool IsMessageValid(char *msg)
{
	// just in case it's not handled before calling this method
	if (msg == NULL)
		return FALSE;

	int length = strlen(msg);
	for (int i = 0; i < length; i++)
	{
		if (msg[i] == '\n')
			return FALSE;

		if (msg[i] == ' ' && msg[i+1] == ' ')
			return FALSE;
	}

	return TRUE;
}


/*
* initialize the trigger event triggered variable
*/
void WaypointResetTriggers(void)
{
	if (botdebugger.IsDebugWaypoints())
		HudNotify("All triggers have been set back to \"not triggered\"\n");

	for (int i = 0; i < MAX_TRIGGERS; i++)
	{
		trigger_gamestate[i].Reset(i);
	}
}


/*
* compare the text with trigger messages and search for matching one
*/
bool WaypointMatchingTriggerMessage(char *the_text)
{
	if (the_text == NULL)
		return FALSE;

	int len;

	//@@@@@@@@@@@@@@@
	/*
	#ifdef _DEBUG
	//if (botdebugger.IsDebugWaypoints())
	//	ALERT(at_console, "WptMatchingTriggerMSG() -> the msg before while statement is: (%s)\n", the_text);
	//UTIL_DebugDev(the_text, -100, -100);
	#endif
	/**/

	// see if the message is nice and clean ie. no additional space or new line characters
	while (IsMessageValid(the_text) == false)
	{
		len = strlen(the_text);
		for (int ch = 0; ch < len; ch++)
		{
			// replace new line character with a space
			if (the_text[ch] == '\n')
				the_text[ch] = ' ';

			// is there a space following another space then we have probably touched the string already
			// so we need to remove the additional space by shifting the rest of the string
			if (the_text[ch] == ' ' && the_text[ch+1] == ' ')
			{
				for (int s = ch; s < len; s++)
					the_text[s] = the_text[s+1];
				len--;
			}
		}
	}

	//@@@@@@@@@@@@@@@
	/*
	#ifdef _DEBUG
	//if (botdebugger.IsDebugWaypoints())
	//	ALERT(at_console, "WptMatchingTriggerMSG() -> the final msg is: (%s)\n", the_text);
	//UTIL_DebugDev(the_text, -100, -100);
	#endif
	/**/

	// then compare it with the trigger events
	for (int i = 0; i < MAX_TRIGGERS; i++)
	{
		// is this slot used and does the text match stored trigger message?
		if (trigger_events[i].name == trigger_gamestate[i].GetName() &&
			trigger_gamestate[i].GetUsed() &&
			(strstr(the_text, trigger_events[i].message) != NULL))
		{
			if (botdebugger.IsDebugWaypoints())
				ALERT(at_console, "Trigger%d has been triggered\n", i+1);

			// then mark it as event happened
			trigger_gamestate[i].SetTriggered(true);
			// also store the time of this event
			trigger_gamestate[i].SetTime();

			return TRUE;
		}
	}

	return FALSE;
}


/*
* return correct priority for trigger waypoint based on the state of game events
*/
int WaypointReturnTriggerWaypointPriority(int wpt_index, int team)
{
	int return_no_priority = 0;

	if (wpt_index == -1)
		return return_no_priority;				// like no priority

	if (IsWaypoint(wpt_index, WptT::wpt_trigger))
	{
		// this is ugly conversion
		//char name[32];

		//TriggerIntToName(waypoints[wpt_index].trigger_event_on, name);						NEW CODE 094 (prev code)
		//TriggerN trigger_on = TriggerNameToId(name);

		//TriggerIntToName(waypoints[wpt_index].trigger_event_off, name);
		//TriggerN trigger_off = TriggerNameToId(name);

		TriggerId trigger_on = IntToTriggerId(waypoints[wpt_index].trigger_event_on);//						NEW CODE 094
		TriggerId trigger_off = IntToTriggerId(waypoints[wpt_index].trigger_event_off);

		float trigger_on_time, trigger_off_time;
		trigger_on_time = trigger_off_time = 0.0;

		if (trigger_on != TriggerId::trigger_none)
		{
			for (int i = 0; i < MAX_TRIGGERS; i++)
			{
				// this trigger slot is used and
				// the game event linked to it has already been triggered
				if (trigger_gamestate[i].GetUsed() && trigger_gamestate[i].GetTriggered())
				{
					// then check if this trigger event is set for the waypoint and 
					// get the time of the trigger_on event (ie. when it happened)
					if (trigger_on == trigger_gamestate[i].GetName())
						trigger_on_time = trigger_gamestate[i].GetTime();
					
					// get the time of the trigger_off event (if it is used on the waypoint)
					if (trigger_off != TriggerId::trigger_none &&		// is there any trigger_off event set for this waypoint
						(trigger_off == trigger_gamestate[i].GetName()))
						trigger_off_time = trigger_gamestate[i].GetTime();
				}
			}
					
			// if the trigger_on event happened after the trigger_off event
			// then we have to return the trigger priorities, because this
			// waypoints is triggered on
			if (trigger_on_time > trigger_off_time)
			{
				// so we must return the trigger priorities instead of standard ones
				if (team == TEAM_RED)
					return waypoints[wpt_index].trigger_red_priority;
				if (team == TEAM_BLUE)
					return waypoints[wpt_index].trigger_blue_priority;

				return return_no_priority;		// just for sure if something went wrong
			}
		}
	}

	// return normal priority because of:
	// 1) this waypoint is not a trigger waypoint or
	// 2) the trigger_off event has been called (ie. happened) so the trigger waypoint is being
	//    turned off and thus we have to use the normal priorities again
	if (team == TEAM_RED)
		return waypoints[wpt_index].red_priority;
	if (team == TEAM_BLUE)
		return waypoints[wpt_index].blue_priority;
	
	return return_no_priority;		// just for sure if something went wrong
}


/*
* convert trigger name (string) to trigger array index
*/
int TriggerNameToIndex(const char *trigger_name)
{
	if (FStrEq(trigger_name, "trigger1"))
		return 0;
	else if (FStrEq(trigger_name, "trigger2"))
		return 1;
	else if (FStrEq(trigger_name, "trigger3"))
		return 2;
	else if (FStrEq(trigger_name, "trigger4"))
		return 3;
	else if (FStrEq(trigger_name, "trigger5"))
		return 4;
	else if (FStrEq(trigger_name, "trigger6"))
		return 5;
	else if (FStrEq(trigger_name, "trigger7"))
		return 6;
	else if (FStrEq(trigger_name, "trigger8"))
		return 7;

	return -1;
}


/*
* convert trigger name (string) to trigger ID
*/
TriggerId TriggerNameToId(const char *trigger_name)
{
	if (FStrEq(trigger_name, "trigger1"))
		return TriggerId::trigger1;
	else if (FStrEq(trigger_name, "trigger2"))
		return TriggerId::trigger2;
	else if (FStrEq(trigger_name, "trigger3"))
		return TriggerId::trigger3;
	else if (FStrEq(trigger_name, "trigger4"))
		return TriggerId::trigger4;
	else if (FStrEq(trigger_name, "trigger5"))
		return TriggerId::trigger5;
	else if (FStrEq(trigger_name, "trigger6"))
		return TriggerId::trigger6;
	else if (FStrEq(trigger_name, "trigger7"))
		return TriggerId::trigger7;
	else if (FStrEq(trigger_name, "trigger8"))
		return TriggerId::trigger8;

	return TriggerId::trigger_none;
}


/*
* convert trigger array index to trigger ID
*/
TriggerId TriggerIndexToId(int i)
{
	if (i == 0)
		return TriggerId::trigger1;
	else if (i == 1)
		return TriggerId::trigger2;
	else if (i == 2)
		return TriggerId::trigger3;
	else if (i == 3)
		return TriggerId::trigger4;
	else if (i == 4)
		return TriggerId::trigger5;
	else if (i == 5)
		return TriggerId::trigger6;
	else if (i == 6)
		return TriggerId::trigger7;
	else if (i == 7)
		return TriggerId::trigger8;
	
	return TriggerId::trigger_none;
}


/*
* convert trigger ID to trigger name (string)
*/
void TriggerIntToName(int i, char *trigger_name)
{
	if (i == 0)
		strcpy(trigger_name, "no event");
	else if ((i >= 1) && (i <= MAX_TRIGGERS))
	{
		sprintf(trigger_name, "trigger%d", i);
	}
	else
		strcpy(trigger_name, "UKNOWN");
}


/*
* convert integer to trigger ID
* this is needed because waypoint structure works with integer value at the moment
* next update to waypoint stuct should change it so we can use trigger ID right away
*/
TriggerId IntToTriggerId(int i)
{
	if (i == 1)
		return TriggerId::trigger1;
	else if (i == 2)
		return TriggerId::trigger2;
	else if (i == 3)
		return TriggerId::trigger3;
	else if (i == 4)
		return TriggerId::trigger4;
	else if (i == 5)
		return TriggerId::trigger5;
	else if (i == 6)
		return TriggerId::trigger6;
	else if (i == 7)
		return TriggerId::trigger7;
	else if (i == 8)
		return TriggerId::trigger8;
	else
		return TriggerId::trigger_none;
}


/*
* convert trigger ID to integer
*/
int TriggerIdToInt(TriggerId triggerId)
{
	if (triggerId == TriggerId::trigger1)
		return 1;
	else if (triggerId == TriggerId::trigger2)
		return 2;
	else if (triggerId == TriggerId::trigger3)
		return 3;
	else if (triggerId == TriggerId::trigger4)
		return 4;
	else if (triggerId == TriggerId::trigger5)
		return 5;
	else if (triggerId == TriggerId::trigger6)
		return 6;
	else if (triggerId == TriggerId::trigger7)
		return 7;
	else if (triggerId == TriggerId::trigger8)
		return 8;
	else
		return 0;
}


/*
* connect the game message to certain trigger slot
*/
int WaypointAddTriggerEvent(const char *trigger_name, const char *trigger_message)
{
	// incorrect input
	if ((strlen(trigger_name) < 1) || (strlen(trigger_message) < 1))
		return -1;

	int index = -1;
	
	index = TriggerNameToIndex(trigger_name);

	// invalid trigger name
	if (index == -1)
		return -2;

	// already used
	if (trigger_gamestate[index].GetUsed())
		return -3;

	// the max length of the trigger event message is 256
	// so inform the user if he is trying to use longer one
	if (strlen(trigger_message) > 255)
		return -4;

	// store the message
	strcpy(trigger_events[index].message, trigger_message);
	trigger_gamestate[index].SetUsed(true);

	return 1;
}


/*
* free the trigger slot
*/
int WaypointDeleteTriggerEvent(const char *trigger_name)
{
	// incorrect input
	if (strlen(trigger_name) < 1)
		return -1;

	int index = -1;
	
	index = TriggerNameToIndex(trigger_name);

	// invalid trigger name
	if (index == -1)
		return -2;

	// already free
	if (!trigger_gamestate[index].GetUsed())
		return -3;

	// delete the message
	trigger_events[index].message[0] = '\0';
	trigger_gamestate[index].SetUsed(false);

	return 1;
}


/*
* connect appropriate trigger message to nearest trigger waypoint
* the state argument can be "on" or "off" and determines if this message
* will trigger the waypoint on or off
*/
int WaypointConnectTriggerEvent(edict_t *pEntity, const char *trigger_name, const char *state)
{
	// incorrect input
	if (strlen(trigger_name) < 1)
		return -1;

	int index = -1;
	bool trigger_on;

	index = TriggerNameToIndex(trigger_name);

	// invalid trigger name
	if (index == -1)
		return -2;

	// find if this message will trigger it on or off
	if ((strlen(state) < 1) || (FStrEq(state, "on")))
		trigger_on = TRUE;
	else if (FStrEq(state, "off"))
		trigger_on = FALSE;
	else
		return -3;

	index = WaypointFindNearestType(pEntity, 50.0, W_FL_TRIGGER);

	// no trigger waypoint around
	if (index == -1)
		return -4;

	// connect the trigger message to this waypoint
	if (trigger_on)
		waypoints[index].trigger_event_on = TriggerIdToInt(TriggerNameToId(trigger_name));
	else
		waypoints[index].trigger_event_off = TriggerIdToInt(TriggerNameToId(trigger_name));

	return 1;
}


/*
* remove appropriate trigger message from nearest trigger waypoint based on the state argument
*/
int WaypointRemoveTriggerEvent(edict_t *pEntity, const char *state)
{
	int index = -1;
	bool trigger_on;

	// find if it is a trigger on or off that's going to be removed
	if (strlen(state) < 1)
		return -1;
	else if (FStrEq(state, "on"))
		trigger_on = TRUE;
	else if (FStrEq(state, "off"))
		trigger_on = FALSE;
	else
		return -2;

	index = WaypointFindNearestType(pEntity, 50.0, W_FL_TRIGGER);

	// no trigger waypoint around
	if (index == -1)
		return -3;

	// reset appropriate trigger event for this waypoint
	if (trigger_on)
		waypoints[index].trigger_event_on = TriggerIdToInt(TriggerId::trigger_none);
	else
		waypoints[index].trigger_event_off = TriggerIdToInt(TriggerId::trigger_none);

	return 1;
}


/*
* check path for various problems and fix them if possible
* returns -1 if there is error that cannot be fixed
* returns 0 if the path was okay (or doesn't even exist)
* returns 1 if there was error that has been fixed
* returns 2 if it needs to be called again after successful path repair
*/
int IsPathOkay(int path_index)
{
	W_PATH *p;
	int safety_stop = 0;
	static bool there_was_error = FALSE;			// needed to remember we did some repairs in previous call
	bool error_in_path = FALSE;						// used to track unfixable problem

	// stop it if the index isn't valid
	if (path_index == -1)
		return 0;

	p = w_paths[path_index];

	while (p)
	{
#ifdef _DEBUG
		char msg[128];
		safety_stop++;
		if (safety_stop > 1000)
		{
			sprintf(msg, "IsPathOkay() | LLERROR\n***Path no. %d\n", path_index + 1);

			ALERT(at_error, msg);
			UTIL_DebugInFile(msg);

			WaypointDebug();
		}
#endif

		// cross or aim waypoint cannot be part of any path
		// (this shouldn't happen, because it's handled in other methods,
		// but if the waypointer manually changed waypoint type to one of these then
		// they are present in a path)
		if (waypoints[p->wpt_index].flags & (W_FL_CROSS | W_FL_AIMING))
		{
			// we must do this first so that we know there is some error in this path
			there_was_error = error_in_path = TRUE;

			if (ExcludeFromPath(p, path_index) == 1)
				return 2;
		}

		// check if current waypoint has been added more than once to this path
		W_PATH *rp = p->next;				// start on the next path waypoint ...
		int stop = 0;

		while (rp)
		{
			// and go through the rest of the path looking for a match
			if (rp->wpt_index == p->wpt_index)
			{
				there_was_error = error_in_path = TRUE;

				// if we found any other addition of this waypoint and successfully removed it
				// then call this method again in order to catch all problems, because this waypoint
				// may have been added several times to this path, not just twice				
				if (ExcludeFromPath(rp, path_index) == 1)
					return 2;
			}

			rp = rp->next;

#ifdef _DEBUG
			stop++;
			if (stop > 1000)
			{
				char msg[128];

				sprintf(msg, "IsPathOkay() | LLERROR in sub-search\n***Path no. %d\n", path_index + 1);

				ALERT(at_error, msg);
				UTIL_DebugInFile(msg);

				WaypointDebug();
			}
#endif
		}
		
		// go to/check the next node
		p = p->next;
	}

	// there is error in path that cannot be fixed so we have to return 'bug'
	if (error_in_path)
	{
		// reset the static error tracker before leaving
		there_was_error = FALSE;

		return -1;
	}

	// there was some error that had been fixed so we have to return 'warning'
	if (there_was_error)
	{
		there_was_error = FALSE;

		return 1;
	}

	return 0;
}


/*
* run path repair routine multiple times if needed to make the path valid
* ie. there aren't any aim or cross waypoints in it and every path waypoint was added just once to the path
* returns -1 if there is such error and it cannot be fixed
* returns 0 if the path is okay (or doesn't exist)
* returns 1 if there was error that has been fixed
*/
int WaypointValidatePath(int path_index)
{
	int path_validity = 2;
	int safety_stop = 0;

	// keep repairing the path until it's valid
	while (path_validity == 2)
	{
		path_validity = IsPathOkay(path_index);

		// if things go really wrong then break the loop
		if (safety_stop > 1000)
			path_validity = -1;

		safety_stop++;
	}

	return path_validity;
}


/*
* go through all paths and try to validate them
*/
void WaypointValidatePath(void)
{
	int result;
	char msg[64];

	for (int path_index = 0; path_index < num_w_paths; path_index++)
	{
		result = WaypointValidatePath(path_index);

		if (result == 1)
		{
			sprintf(msg, "path #%d was repaired\n", path_index + 1);

			ALERT(at_console, msg);
			UTIL_DebugInFile(msg);		// send this event in error log
		}
		else if (result == -1)
		{
			sprintf(msg, "unable to repair path #%d\n", path_index + 1);

			ALERT(at_console, msg);
			UTIL_DebugInFile(msg);
		}
	}

	return;
}


/*
* look for specific waypoint type in given path type (eg. goback waypoint in one-way path)
* return true if such problem combination was found
*/
bool WaypointScanPathForBadCombinationOfFlags(int path_index, PathT path_type, WptT waypoint_type)
{
	// does this path match the one we look for
	if (IsPath(path_index, path_type))
	{
		// then see if there is a problem combination in it
		W_PATH *p = GetWaypointTypePointer(waypoint_type, path_index);

		// we found such combination
		if (p != NULL)
		{
			// now we just need to ignore team limited case, because the bot will also ignore such waypoint there
			// (goback waypoint with red team priority == 0 in red team only path is valid waypoint placement)
			if ((IsPath(path_index, PathT::path_red) && (waypoints[p->wpt_index].red_priority == 0)) ||
				(IsPath(path_index, PathT::path_blue) && (waypoints[p->wpt_index].blue_priority == 0)))
				return FALSE;
			
			return TRUE;
		}
	}

	return FALSE;
}


/*
* check either path end for correct ending
* (ie. there's a connection to a cross waypoint or one of the go back waypoints)
* return -1 if some error occured, 0 if the path was okay
* return 1 if there was anything repaired and 10 if there was an unfixable problem
*/
int WaypointRepairInvalidPathEnd(int path_index)
{
	// first check the validity
	if ((path_index == -1) || (w_paths[path_index] == NULL))
		return -1;

	int end_waypoint = -1;

	// if it is a one way path then we will check just the path end
	if (IsPath(path_index, PathT::path_one))
	{
		end_waypoint = GetPathEnd(path_index);

		// check validity
		if (end_waypoint == -1)
			return -1;

		// check for cases when a one-way path ends with a goback waypoint
		if (IsWaypoint(end_waypoint, WptT::wpt_goback))
		{
			// remove one-way direction bit
			w_paths[path_index]->flags &= ~P_FL_WAY_ONE;

			// and make the path a two-way path from now
			w_paths[path_index]->flags |= P_FL_WAY_TWO;

			return 1;
		}
		
		// if this path ends at cross waypoint then everything is fine and we can stop it here
		if (WaypointFindNearestCross(waypoints[end_waypoint].origin) != -1)
		{
			return 0;
		}

		// there is a waypoint nearby so this path doesn't end in a void and we can stop it here 
		if (WaypointFindNearestStandard(end_waypoint, path_index) != -1)
		{
			return 0;
		}

		// if neither of these waypoints is in the path then turn the path into a two-way path ...
		if (!WaypointTypeIsInPath(WptT::wpt_chute, path_index) && !WaypointTypeIsInPath(WptT::wpt_duckjump, path_index) &&
			!WaypointTypeIsInPath(WptT::wpt_jump, path_index))
		{
			w_paths[path_index]->flags &= ~P_FL_WAY_ONE;
			w_paths[path_index]->flags |= P_FL_WAY_TWO;

			// and make the end waypoint a goback one unless there already is a turn back marker set
			if (waypoints[end_waypoint].flags & PATH_TURNBACK)
				;
			else
				waypoints[end_waypoint].flags |= W_FL_GOBACK;

			return 1;
		}

		return 10;
	}
	
	// this path is a two way or patrol path ... so let's check start first	
	end_waypoint = GetPathStart(path_index);

	if (end_waypoint == -1)
		return -1;

	bool repair_done = false;

	// if this path is ended in a right way we will just continue because we still need to check the other end as well
	if (waypoints[end_waypoint].flags & PATH_TURNBACK)
		;
	else
	{
		// is there no cross at path start
		if (WaypointFindNearestCross(waypoints[end_waypoint].origin) == -1)
		{
			// is there no other available waypoint at path start
			if (WaypointFindNearestStandard(end_waypoint, path_index) == -1)
			{
				// turn the waypoint into goback and set repair done so we know we have fixed something
				waypoints[end_waypoint].flags |= W_FL_GOBACK;
				repair_done = true;
			}
		}
	}

	// now check the path end
	end_waypoint = GetPathEnd(path_index);

	if (end_waypoint == -1)
		return -1;

	// we can't stop it here because there's a chance we would return false result if the start was repaired
	if (waypoints[end_waypoint].flags & PATH_TURNBACK)
		;
	else
	{
		if (WaypointFindNearestCross(waypoints[end_waypoint].origin) == -1)
		{
			if (WaypointFindNearestStandard(end_waypoint, path_index) == -1)
			{
				waypoints[end_waypoint].flags |= W_FL_GOBACK;
				repair_done = true;
			}
		}
	}

	// did we repair something?
	if (repair_done)
		return 1;

	// everything was okay
	return 0;
}


/*
* go through all paths and check either path end for correct ending
* (ie. there's a connection to a cross waypoint or one of the go back waypoints)
*/
void WaypointRepairInvalidPathEnd(void)
{
	int result;
	char msg[64];

	for (int path_index = 0; path_index < num_w_paths; path_index++)
	{
		result = WaypointRepairInvalidPathEnd(path_index);

		switch (result)
		{
			case 1:
			{
				sprintf(msg, "path #%d was repaired\n", path_index + 1);
				
				ALERT(at_console, msg);
				UTIL_DebugInFile(msg);		// send this event in error log

				break;
			}
			case 10:
			{
				sprintf(msg, "unable to repair path #%d\n", path_index + 1);

				ALERT(at_console, msg);
				UTIL_DebugInFile(msg);

				break;
			}
		}
	}

	return;
}


/*
* check either path end for correct ending
* pretty much the same as WaypointRepairInvalidPathEnd() but we don't repair anything
* but we check things to a greater depth here and report them all
* return -1 if some error occured, 0 if the path was okay
* return 1 if there was invalid path end found
*/
int WaypointCheckInvalidPathEnd(int path_index, bool log_in_file)
{
	if (path_index == -1 || w_paths[path_index] == NULL)
		return -1;

	int end_waypoint = -1;
	char msg[256];

	if (IsPath(path_index, PathT::path_one))
	{
		end_waypoint = GetPathEnd(path_index);

		if (end_waypoint == -1)
			return -1;

		if (IsWaypoint(end_waypoint, WptT::wpt_goback))
		{
			sprintf(msg, "BUG: One-way path no. %d ends with a goback waypoint!\n", path_index + 1);
			HudNotify(msg, log_in_file);

			return 1;
		}

		if (WaypointFindNearestCross(waypoints[end_waypoint].origin) != -1)
		{
			return 0;
		}

		if (WaypointFindNearestStandard(end_waypoint, path_index) != -1)
		{
			sprintf(msg, "WARNING: One-way path no. %d doesn't end at cross waypoint, but there seems to be a way from there. Check it!\n", path_index + 1);
			HudNotify(msg, log_in_file);

			return 1;
		}
		else
		{
			sprintf(msg, "BUG: One-way path no. %d ends in a void ie. there's no cross waypoint or any other way out of there!\n", path_index + 1);
			HudNotify(msg, log_in_file);

			return 1;
		}

		sprintf(msg, "Unknown state of one-way path no. %d Check it!\n", path_index + 1);
		HudNotify(msg, log_in_file);

		return 10;
	}

	// two way or patrol path ... so let's check paths' start first	
	end_waypoint = GetPathStart(path_index);

	if (end_waypoint == -1)
		return -1;

	bool problem_found = false;

	if (waypoints[end_waypoint].flags & PATH_TURNBACK)
		;
	else
	{
		if (WaypointFindNearestCross(waypoints[end_waypoint].origin) == -1)
		{
			if (WaypointFindNearestStandard(end_waypoint, path_index) != -1)
			{
				sprintf(msg, "WARNING: Path no. %d doesn't start at cross waypoint, but there seems to be other way from there. Check it!\n", path_index + 1);
				HudNotify(msg, log_in_file);

				// print the additional message just into console, don't log it in file
				sprintf(msg, "        It could be missing goback waypoint or small cross waypoint range.\n");
				HudNotify(msg);
				sprintf(msg, "        Or there are doors or breakable object between cross waypoint and paths' 1st waypoint.\n");
				HudNotify(msg);

				problem_found = true;
			}
			else
			{
				sprintf(msg, "BUG: Path no. %d starts in a void ie. there's no cross waypoint or any other way to follow there!\n", path_index + 1);
				HudNotify(msg, log_in_file);
				
				problem_found = true;
			}
		}
	}

	// now check the path end
	end_waypoint = GetPathEnd(path_index);

	if (end_waypoint == -1)
		return -1;

	if (waypoints[end_waypoint].flags & PATH_TURNBACK)
		;
	else
	{
		if (WaypointFindNearestCross(waypoints[end_waypoint].origin) == -1)
		{
			if (WaypointFindNearestStandard(end_waypoint, path_index) != -1)
			{
				sprintf(msg, "WARNING: Path no. %d doesn't end at cross waypoint, but there seems to be other way from there. Check it!\n", path_index + 1);
				HudNotify(msg, log_in_file);

				sprintf(msg, "        It could be missing goback waypoint or small cross waypoint range.\n");
				HudNotify(msg);
				sprintf(msg, "        Or there is breakable object or doors between cross and last path waypoint.\n");
				HudNotify(msg);
				
				problem_found = true;
			}
			else
			{
				sprintf(msg, "BUG: Path no. %d ends in a void ie. there's no cross waypoint, no goback waypoint or any other way to follow there!\n", path_index + 1);
				HudNotify(msg, log_in_file);

				problem_found = true;
			}
		}
	}

	if (problem_found)
		return 1;

	// everything was okay
	return 0;
}


/*
* merges two paths into one where path2_index path will be appended to the end of path1_index path
* reverse_order == false means path1 (start->end) + path2 (start->end)
* reverse_order == true means path1(start->end) + path2 (end->start)
* path2_index flags are discarded and the path basically gets deleted after the merge
* returns path1_index if everything is okay otherwise it returns -1 as error
*/
int MergePaths(int path1_index, int path2_index, bool path2_in_reverse_order)
{
	if ((path1_index == -1) || (path2_index == -1))
		return -1;
	
	int path2_waypoint = -1;

	// first we must set this otherwise the Continue Current Path method would return error 
	internals.SetPathToContinue(path1_index);
	
	// path2_index path is added to the end of path1_index path in order from its start to its end
	if (path2_in_reverse_order == false)
	{
		// get the 1st waypoint from the other path
		path2_waypoint = GetPathStart(path2_index);

		// now keep removing the start waypoint from the path2_index path and add it to the end of path1_index path
		// untill there's nothing left in path2_index path
		while (path2_waypoint != -1)
		{
			// remove the waypoint from the path2_index path
			ExcludeFromPath(path2_waypoint, path2_index);

			// add it to path1_index path
			ContinueCurrPath(path2_waypoint);

			// get the new start waypoint ...
			path2_waypoint = GetPathStart(path2_index);
		}
	}
	// path2_index path is added to the end of path1_index path in reversed order (ie. from its end to its start)
	else
	{
		// get the last waypoint from the other path
		path2_waypoint = GetPathEnd(path2_index);

		while (path2_waypoint != -1)
		{
			ExcludeFromPath(path2_waypoint, path2_index);
			ContinueCurrPath(path2_waypoint);
			path2_waypoint = GetPathEnd(path2_index);
		}
	}

	// we're done here so we must reset it
	internals.ResetPathToContinue();

	// we should also check our path for errors
	if (WaypointValidatePath(path1_index) != -1)
	{
		// and let it update its status seeing we just added some new waypoints in it
		UpdatePathStatus(path1_index);

		return path1_index;
	}

	return -1;
}


/*
* merges two paths into one where path2_index path will be inserted to the beginning of path1_index path
* reverse_order == false means path2 (start->end) + path1 (start->end)
* reverse_order == true means path2 (end->start) + path1 (start->end)
* path2_index flags are discarded and the path basically gets deleted after the merge
* returns path1_index if everything is okay otherwise it returns -1 as error
*/
int MergePathsInverted(int path1_index, int path2_index, bool path2_in_reverse_order)
{
	if ((path1_index == -1) || (path2_index == -1))
		return -1;

	int path2_waypoint = -1;
	// the method for inserting waypoint into path works with string arguments
	// we must convert them first
	char path_index1_as_char[6];
	char path2_waypoint_as_char[6];

	sprintf(path_index1_as_char, "%d", path1_index + 1);
	
	// the order of the final path will be (path2 start -> path2 end -> path1 start -> path1 end)
	if (path2_in_reverse_order == false)
	{
		// to keep the desired order of paths we have to go from the end of the 2nd path
		// because we are inserting them one after another to the start of path1_index path
		path2_waypoint = GetPathEnd(path2_index);

		while (path2_waypoint != -1)
		{
			ExcludeFromPath(path2_waypoint, path2_index);
			
			sprintf(path2_waypoint_as_char, "%d", path2_waypoint + 1);
			WaypointInsertIntoPath(path2_waypoint_as_char, path_index1_as_char, "start", NULL);

			path2_waypoint = GetPathEnd(path2_index);
		}
	}
	// the order of the final path will be (path2 end -> path2 start -> path1 start -> path1 end)
	else
	{
		path2_waypoint = GetPathStart(path2_index);

		while (path2_waypoint != -1)
		{
			ExcludeFromPath(path2_waypoint, path2_index);

			sprintf(path2_waypoint_as_char, "%d", path2_waypoint + 1);
			WaypointInsertIntoPath(path2_waypoint_as_char, path_index1_as_char, "start", NULL);

			path2_waypoint = GetPathStart(path2_index);
		}
	}

	if (WaypointValidatePath(path1_index) != -1)
	{
		UpdatePathStatus(path1_index);

		return path1_index;
	}

	return -1;
}


/*
* checks both ends of path whether there is a start or end of another path there without connection to cross waypoint
* (ie. the path start/end waypoint isn't connected to a cross waypoint yet there starts or ends another path)
* we are also checking for either of the turn back markers (goback, ammobox or use)
* it can be used in purely checking mode where there is no repairing done
* return -1 if some error occured, 0 if the path was okay
* return 1 if there was anything repaired
* NOT USED --- return 2 if there was detected suspicious connection
* NOT USED --- return 3 if there was anything repaired as well as detected suspicious connection
* return 4 if there was detected unfixable connection
*/
int WaypointRepairInvalidPathMerge(int path_index, bool repair_it, bool log_in_file)
{
	if ((path_index == -1) || (w_paths[path_index] == NULL))
		return -1;

	int end_waypoint = -1;
	int start_waypoint = -1;
	int other_end_waypoint = -1;
	int other_start_waypoint = -1;
	bool starts_at_cross = false;
	bool ends_at_cross = false;
	bool invalid_merge_at_start = false;
	bool invalid_merge_at_end = false;
	char msg[256];

	start_waypoint = GetPathStart(path_index);
	end_waypoint = GetPathEnd(path_index);

	if ((start_waypoint == -1) || (end_waypoint == -1))
		return -1;

	if (WaypointFindNearestCross(waypoints[start_waypoint].origin, true) != -1)
		starts_at_cross = true;

	if (WaypointFindNearestCross(waypoints[end_waypoint].origin, true) != -1)
		ends_at_cross = true;

	// if both path ends are connected to a cross waypoint then all is fine and we can stop right away
	if (starts_at_cross && ends_at_cross)
	{
		return 0;
	}

	// if it starts at cross waypoint AND ends with a valid turn-back marker then all is fine
	if (starts_at_cross && (waypoints[end_waypoint].flags & PATH_TURNBACK))
	{
		return 0;
	}

	// if it ends at cross waypoint AND starts with a valid turn-back marker then all is fine again
	if (ends_at_cross && (waypoints[start_waypoint].flags & PATH_TURNBACK))
	{
		return 0;
	}

	for (int other_path = 0; other_path < num_w_paths; other_path++)
	{
		if (other_path == path_index)
			continue;

		other_start_waypoint = GetPathStart(other_path);
		other_end_waypoint = GetPathEnd(other_path);

		// path does NOT start at cross waypoint but it starts on the same waypoint as
		// the other path starts OR ends so this must be invalid merge of paths
		if (!starts_at_cross && ((start_waypoint == other_start_waypoint) || (start_waypoint == other_end_waypoint)))
		{
			// fix false positive
			// because one-way paths can start on one waypoint even outside any cross waypoint
			if ((start_waypoint == other_start_waypoint) &&
				IsPath(path_index, PathT::path_one) && IsPath(other_path, PathT::path_one))
				;
			else
				invalid_merge_at_start = true;
		}

		// path does NOT end at cross waypoint but it ends on the same waypoint as
		// the other path starts OR ends so this must be invalid merge of paths
		if (!ends_at_cross && ((end_waypoint == other_start_waypoint) || (end_waypoint == other_end_waypoint)))
			invalid_merge_at_end = true;

		// if there is invalid merge at one or the other path end then report it
		if (invalid_merge_at_start || invalid_merge_at_end)
		{
			// are we going to repair it?
			if (repair_it)
			{
				// now we must decide how will the paths be merged and seeing we always want to keep this path
				// and discard the other path then there are these 4 cases ...

				// first case is typical invalid merge where the waypointer started a new path instead of
				// continuing in the current one so it's a simple append to the end of path_index path
				// the final path will be path_index (start->end) + other path (start->end)
				if (invalid_merge_at_end && (end_waypoint == other_start_waypoint))
				{
					if (MergePaths(path_index, other_path, false) != path_index)
						return -1;
				}
				// second case is that both paths have the ending waypoint same so
				// the final path will be path_index (start->end) + other path (end->start)
				else if (invalid_merge_at_end && (end_waypoint == other_end_waypoint))
				{
					if (MergePaths(path_index, other_path, true) != path_index)
						return -1;
				}
				// 3rd case is that the other path ends on the starting waypoint of this path so
				// the final path will be other path (start->end) + path_index (start->end)
				// basically this is the same as case no. 1, but from the 'other path' point of view
				// but we want to keep this path - it has lower index so it was probably created first
				// and is probably more important
				else if (invalid_merge_at_start && (start_waypoint == other_end_waypoint))
				{
					if (MergePathsInverted(path_index, other_path, false) != path_index)
						return -1;
				}
				// the final case is when both paths have the starting waypoint same so
				// the final path will be other path (end->start) + path_index (start->end)
				// same as above we want to keep the path with lower index
				// so we will use inverted merging again
				else// like -> else if (invalid_merge_at_start && (start_waypoint == other_start_waypoint))
				{
					if (MergePathsInverted(path_index, other_path, true) != path_index)
						return -1;
				}

				if (log_in_file)
				{
					// send this event in error log
					char msg[64];
					sprintf(msg, "Invalid merge on paths #%d and #%d has been REPAIRED!\n", path_index + 1, other_path + 1);
					UTIL_DebugInFile(msg);
				}
			}
			// or we will just report it
			else
			{
				sprintf(msg, "BUG: Invalid merge on paths no. %d and no. %d has been detected!\n", path_index + 1, other_path + 1);
				HudNotify(msg, log_in_file);
			}

			return 1;
		}

		// we also need to check for cases when this path starts inside another path
		// (not just the end or start waypoint of the other path
		//  but any waypoint from the other path == our path start waypoint)
		if (!starts_at_cross && WaypointIsInPath(start_waypoint, other_path))
		{
			if (repair_it == false)
			{
				sprintf(msg, "BUG: Invalid merge of paths detected! Path no. %d starts inside path no. %d\n", path_index + 1, other_path + 1);
				HudNotify(msg, log_in_file);
			}
			// not enough data to figure out how such path should have looked like
			// so we just report it
			else
			{
				sprintf(msg, "BUG: Invalid merge of paths detected! Path no. %d starts inside path no. %d\n     Unable to automatically fix such merge!\n",
					path_index + 1, other_path + 1);
				HudNotify(msg, log_in_file);
			}

			return 4;
		}

		// and when this path ends inside another path (any waypoint from the other path == our path end waypoint)
		if (!ends_at_cross && WaypointIsInPath(end_waypoint, other_path))
		{
			if (repair_it == false)
			{
				sprintf(msg, "BUG: Invalid merge of paths detected! Path no. %d ends inside path no. %d\n", path_index + 1, other_path + 1);
				HudNotify(msg, log_in_file);
			}
			else
			{
				sprintf(msg, "BUG: Invalid merge of paths detected! Path no. %d ends inside path no. %d\n     Unable to automatically fix such merge!\n",
					path_index + 1, other_path + 1);
				HudNotify(msg, log_in_file);
			}

			return 4;
		}
	}

	return 0;
}

/*/																													ORIGINAL CODE
int WaypointRepairInvalidPathMerge(int path_index)
{
	// first check the validity
	if ((path_index == -1) || (w_paths[path_index] == NULL))
		return -1;

	int end_waypoint = -1;
	int other_end_waypoint = -1;
	int other_start_waypoint = -1;
	bool repair_done = false;
	bool detected_suspicious = false;

	end_waypoint = GetPathEnd(path_index);

	// check validity
	if (end_waypoint == -1)
		return -1;

	// TODO: we also need to check for cases when this path ends in another path
	//		(I mean not just end or start waypoint but any waypoint from the other path == our path end waypoint)


	// if this path ends at cross waypoint then everything is fine and we can stop it here
	if (WaypointFindNearestCross(waypoints[end_waypoint].origin, true) != -1)
	{
		return 0;
	}

	// it's not a one-way path and it ends on one of turn back waypoints then everything is fine
	if (!IsPath(path_index, path_one) && waypoints[end_waypoint].flags & (W_FL_GOBACK | W_FL_AMMOBOX | W_FL_USE))
	{
		return 0;
	}

	// go through all paths
	for (int other_path = 0; other_path < num_w_paths; other_path++)
	{
		// skip our path
		if (other_path == path_index)
			continue;

		other_start_waypoint = GetPathStart(other_path);
		other_end_waypoint = GetPathEnd(other_path);

		// we just found an invalid merge of two paths
		if (end_waypoint == other_start_waypoint)
		{
			// in order to fix it we need to continue in the path that ends here (ie. our path)
			g_path_to_continue = path_index;

			// first remove the first waypoint from the other path (ie. the waypoint both paths share)
			ExcludeFromPath(other_start_waypoint, other_path);
			
			// get the pointer on the other path
			W_PATH *p = w_paths[other_path];

			// now keep removing the start waypoint from the other path and adding it at the end of our path
			while (p)
			{
				// get the new start waypoint ...
				other_start_waypoint = GetPathStart(other_path);

				// remove it from the other path ...
				ExcludeFromPath(other_start_waypoint, other_path);

				// add it into our path
				ContinueCurrPath(other_start_waypoint);

				// finally update the pointer
				p = w_paths[other_path];
			}

			// we're done here so we must reset these
			g_path_to_continue = NO_VAL;
			g_path_last_wpt = -1;

			// we should also check our path for errors
			WaypointValidatePath(path_index);//											NEW CODE 094
			//WaypointIsPathOkay(path_index);											// NEW CODE 094 (prev code)

			// and let it update its status seeing we just added some new waypoints in it
			UpdatePathStatus(path_index);

			repair_done = true;

			// send this event in error log
			char msg[64];
			sprintf(msg, "Invalid merge on paths #%d and #%d has been REPAIRED!\n", path_index + 1, other_path + 1);
			UTIL_DebugInFile(msg);
		}

		if (end_waypoint == other_end_waypoint)
		{
			detected_suspicious = true;

			char msg[64];
			sprintf(msg, "Detected suspicious path connection! Check paths #%d and #%d\n", path_index + 1, other_path + 1);
			UTIL_DebugInFile(msg);
		}
	}

	if (repair_done && detected_suspicious)
		return 3;

	if (repair_done)
		return 1;

	if (detected_suspicious)
		return 2;

	// everything was okay
	return 0;
}
/**/


/*
* go through all paths and check them for invalid merge of paths
* (ie. the end waypoint isn't connected to a cross waypoint yet there starts another path)
* 
*/
void WaypointRepairInvalidPathMerge(void)
{
	int result;

	for (int path_index = 0; path_index < num_w_paths; path_index++)
	{
		//result = WaypointRepairInvalidPathMerge(path_index);																	NEW CODE 094 (prev code)
		result = WaypointRepairInvalidPathMerge(path_index, true, true);//														NEW CODE 094

		switch (result)
		{
			case 1:
				ALERT(at_console, "path #%d was repaired\n", path_index + 1);
				break;
			case 2:
				ALERT(at_console, "check path #%d because there was detected suspicious path connection\n", path_index + 1);
				break;
			case 3:
				ALERT(at_console, "path #%d was repaired, but there's also suspicious path connection on this path\n", path_index + 1);
				break;
		}
	}

	return;
}


/*/			REPLACED BY THE NON FIXING VERSION OF REPAIR INVALID PATH MERGE METHOD
int WaypointCheckInvalidPathMerge(int path_index, bool log_in_file)
{
	if ((path_index == -1) || (w_paths[path_index] == NULL))
		return -1;

	int end_waypoint = -1;
	int start_waypoint = -1;
	int other_end_waypoint = -1;
	int other_start_waypoint = -1;
	bool starts_at_cross = false;
	bool ends_at_cross = false;
	bool invalid_merge_at_start = false;
	bool invalid_merge_at_end = false;
	char msg[128];

	start_waypoint = GetPathStart(path_index);
	end_waypoint = GetPathEnd(path_index);

	if ((start_waypoint == -1) || (end_waypoint == -1))
		return -1;

	if (WaypointFindNearestCross(waypoints[start_waypoint].origin, true) != -1)
		starts_at_cross = true;

	if (WaypointFindNearestCross(waypoints[end_waypoint].origin, true) != -1)
		ends_at_cross = true;

	// if both path ends are connected to a cross waypoint then all is fine and we can stop right away
	if (starts_at_cross && ends_at_cross)
	{
		return 0;
	}

	// if it starts at cross waypoint AND ends with a valid turn-back marker then all is fine
	if (starts_at_cross && (waypoints[end_waypoint].flags & (W_FL_GOBACK | W_FL_AMMOBOX | W_FL_USE)))
	{
		return 0;
	}

	// if it ends at cross waypoint AND starts with a valid turn-back marker then all is fine again
	if (ends_at_cross && (waypoints[start_waypoint].flags & (W_FL_GOBACK | W_FL_AMMOBOX | W_FL_USE)))
	{
		return 0;
	}

	for (int other_path = 0; other_path < num_w_paths; other_path++)
	{
		if (other_path == path_index)
			continue;

		other_start_waypoint = GetPathStart(other_path);
		other_end_waypoint = GetPathEnd(other_path);

		// path does NOT start at cross waypoint but it starts on the same waypoint as
		// the other path starts OR ends so this must be invalid merge of paths
		if (!starts_at_cross &&
			((start_waypoint == other_start_waypoint) || (start_waypoint == other_end_waypoint)))
		{
			// fix false positive
			// because one-way paths can start on one waypoint even outside any cross waypoint
			if ((start_waypoint == other_start_waypoint) &&
				IsPath(path_index, path_one) && IsPath(other_path, path_one))
				;
			else
				invalid_merge_at_start = true;
		}

		// path does NOT end at cross waypoint but it ends on the same waypoint as
		// the other path starts OR ends so this must be invalid merge of paths
		if (!ends_at_cross &&
			((end_waypoint == other_start_waypoint) || (end_waypoint == other_end_waypoint)))
			invalid_merge_at_end = true;

		// if there is invalid merge at one or the other path end then report it
		if (invalid_merge_at_start || invalid_merge_at_end)
		{
			sprintf(msg, "BUG: Invalid merge on paths no. %d and no. %d has been detected!\n", path_index + 1, other_path + 1);
			HudNotify(msg, log_in_file);

			return 1;
		}

		// this statement is not used now
		//if ()
		//{
		//	sprintf(msg, "WARNING: Detected suspicious path connection! Check paths no. %d and no. %d\n", path_index + 1, other_path + 1);
		//	HudNotify(msg, log_in_file);

		//	return 2;
		//}

		// we also need to check for cases when this path ends inside another path
		// (not just the end or start waypoint of the other path
		//  but any waypoint from the other path == our path end waypoint)
		if (!ends_at_cross && WaypointIsInPath(end_waypoint, other_path))
		{
			sprintf(msg, "BUG: Invalid merge of paths detected! Path no. %d ends inside path no. %d\n", path_index + 1, other_path + 1);
			HudNotify(msg, log_in_file);

			return 1;
		}
	}

	return 0;
}
/**/


/*
* check path end for correct sniper spot
*/
int WaypointRepairSniperSpot(int path_index)
{
	// first check the validity
	if (path_index == -1 || w_paths[path_index] == NULL)
		return -1;

	int end_waypoint = -1;
	bool missing_aim_waypoint = false;
	bool repair_done = false;

	end_waypoint = GetPathEnd(path_index);

	// check validity
	if (end_waypoint == -1)
		return -1;

	// if this path ends at cross waypoint then this isn't true sniper spot and we can stop it here
	if (WaypointFindNearestCross(waypoints[end_waypoint].origin, true) != -1)
	{
		return 0;
	}

	// if this path is an ammobox or use something path then this isn't a sniper spot case either
	if (waypoints[end_waypoint].flags & (W_FL_AMMOBOX | W_FL_USE))
	{
		return 0;
	}

	// see if there already is a wait time
	if (waypoints[end_waypoint].red_time > 0 || waypoints[end_waypoint].blue_time > 0)
	{
		// if this waypoint misses sniper flag then set it
		if (!IsWaypoint(end_waypoint, WptT::wpt_sniper))
			waypoints[end_waypoint].flags |= W_FL_SNIPER;

		// also if there is any aiming waypoint nearby then everything is fine
		if (WaypointFindAimingAround(end_waypoint) != -1)
			return 0;
		// otherwise see if we can place a new aiming waypoint
		else
		{
			missing_aim_waypoint = true;
		}
	}

	// now we know this path doesn't end at a cross waypoint and it also doesn't go to an ammobox or usable entity so ...

	// if this path is a sniper or machinegunner path then it most probably misses the sniper spot
	if (!missing_aim_waypoint && (IsPath(path_index, PathT::path_sniper) || IsPath(path_index, PathT::path_mgunner)))
	{
		// set wait time on this waypoint
		waypoints[end_waypoint].red_time = 60.0;
		waypoints[end_waypoint].blue_time = 60.0;

		if (!IsWaypoint(end_waypoint, WptT::wpt_sniper))
			waypoints[end_waypoint].flags |= W_FL_SNIPER;

		// also check for aiming waypoint nearby and if none found try to fix it
		if (WaypointFindAimingAround(end_waypoint) == -1)
		{
			missing_aim_waypoint = true;
		}

		repair_done = true;
	}

	if (missing_aim_waypoint)
	{
		// get previous waypoint from this path
		int prev_waypoint = GetPathPreviousWaypoint(end_waypoint, path_index);
		
		if (prev_waypoint != -1)
		{
			// use the previous waypoint to get a direction where the bot should aim to
			Vector aim_vec = (waypoints[end_waypoint].origin - waypoints[prev_waypoint].origin).Normalize();
			
			// now use the normalized aiming vector to place new aiming waypoint in range of 75 units
			// from the end waypoint position
			Vector aim_origin = waypoints[end_waypoint].origin + aim_vec * 75;

			// first we need to test if there isn't any solid obstacle in that direction ... we don't want the bot
			// facing wall while having open backs
			// so use eyes origins because sandbag or window would invalidate the trace line using standard origins
			Vector head_wpt = waypoints[end_waypoint].origin + Vector(0, 0, 24);
			Vector head_aim = aim_origin + Vector(0, 0, 24);
			
			TraceResult tr;
			UTIL_TraceLine(head_wpt, head_aim, ignore_monsters, NULL, &tr);

			// if there isn't free space for the new aiming waypoint ...
			if (tr.flFraction != 1.0)
			{
				// then make the aim vector opposite ... ie. the bot would face the previous path waypoint
				aim_vec = (waypoints[prev_waypoint].origin - waypoints[end_waypoint].origin).Normalize();
				aim_origin = waypoints[end_waypoint].origin + aim_vec * 75;
			}
			
			// finally add the aiming waypoint and set 'repair done'
			if (WaypointAdd(aim_origin, WptT::wpt_aim) == WptT::wpt_aim)
				repair_done = true;
		}
	}

	if (repair_done)
		return 1;

	return 0;
}


/*
* go through all paths and check them for correct sniper spot
*/
void WaypointRepairSniperSpot(void)
{
	int result;

	for (int path_index = 0; path_index < num_w_paths; path_index++)
	{
		result = WaypointRepairSniperSpot(path_index);

		if (result == 1)
		{
			ALERT(at_console, "sniper spot on path #%d was repaired\n", path_index + 1);
		}
	}

	return;
}


/*
* checks cross waypoint surroundings whether there's a free path end there
* if so the range on cross waypoint will be increased to connect with the path end
* returns -1.0 if the waypoint isn't valid
* otherwise it returns waypoint range
*/
float WaypointRepairCrossRange(int wpt_index)
{
	int path_end_wpt = -1;
	float distance = MAX_WPT_DIST;

	if ((wpt_index == -1) || !IsWaypoint(wpt_index, WptT::wpt_cross))
		return -1.0;

	// go through all paths
	for (int path_index = 0; path_index < num_w_paths; path_index++)
	{
		// skip deleted paths
		if (w_paths[path_index] == NULL)
			continue;

		// get this path last waypoint
		path_end_wpt = GetPathEnd(path_index);

		// this end of the path isn't connected to cross waypoint and
		// it isn't ended with one of turn back markers so it needs to be checked
		if ((path_end_wpt != -1) && (WaypointFindNearestCross(waypoints[path_end_wpt].origin) == -1) &&
			!IsWaypoint(path_end_wpt, WptT::wpt_goback) && !IsWaypoint(path_end_wpt, WptT::wpt_ammobox) &&
			!IsWaypoint(path_end_wpt, WptT::wpt_use))
		{
			distance = GetWaypointDistance(path_end_wpt, wpt_index);

			// path end waypoint is past the range of this cross waypoint
			// but close enough to be reachable for this cross waypoint
			if ((distance < MAX_WPT_DIST) && (distance > waypoints[wpt_index].range))
			{
				// so let's increase cross waypoint range to connect it to this path end waypoint
				waypoints[wpt_index].range = truncf(distance + 1.0);
			}
		}

		// check also path first waypoint
		path_end_wpt = GetPathStart(path_index);

		if ((path_end_wpt != -1) && (WaypointFindNearestCross(waypoints[path_end_wpt].origin) == -1) &&
			!IsWaypoint(path_end_wpt, WptT::wpt_goback) && !IsWaypoint(path_end_wpt, WptT::wpt_ammobox) &&
			!IsWaypoint(path_end_wpt, WptT::wpt_use))
		{
			distance = GetWaypointDistance(path_end_wpt, wpt_index);

			if ((distance < MAX_WPT_DIST) && (distance > waypoints[wpt_index].range))
			{
				waypoints[wpt_index].range = truncf(distance + 1.0);
			}
		}
	}

	return waypoints[wpt_index].range;
}


/*
* goes through all cross waypoints in order to connect them
* with free path ends in their surrounding
*/
void WaypointRepairCrossRange(void)
{
	float original_range = 0.0;
	float new_range = 0.0;

	for (int wpt_index = 0; wpt_index < num_waypoints; wpt_index++)
	{
		if (IsWaypoint(wpt_index, WptT::wpt_cross))
		{
			// backup current range
			original_range = waypoints[wpt_index].range;

			new_range = WaypointRepairCrossRange(wpt_index);

			if (new_range != original_range)
				ALERT(at_console, "The range on cross waypoint no. %d was repaired!\n", wpt_index + 1);
		}
	}
}


/*
* check waypoint for invalid flag/tag combination and fix them
* fix also zero flag/tag by making such waypoint a deleted one
* return 1 if everything was okay
* return 0 if there was any flag/tag repair done
*/
int WaypointFixInvalidFlags(int wpt_index)
{
	// to let us know we had to fix something
	bool fixed = false;

	if (wpt_index == -1)
		return -1;

	// some of these might eventually be removed in conversion from version 8 to 9

	if ((waypoints[wpt_index].flags & W_FL_AMMOBOX) || (waypoints[wpt_index].flags & W_FL_USE))
	{
		// ammobox and use waypoints are taken as gobacks by default if they are ending a path
		// therefore the goback flag/tag shouldn't be there
		if (waypoints[wpt_index].flags & W_FL_GOBACK)
		{
			waypoints[wpt_index].flags &= ~W_FL_GOBACK;
			fixed = true;
		}
	}

	// once the waypoint is ammobox then it cannot be a use one
	if (waypoints[wpt_index].flags & W_FL_AMMOBOX)
	{
		if(waypoints[wpt_index].flags & W_FL_USE)
		{
			waypoints[wpt_index].flags &= ~W_FL_USE;
			fixed = true;
		}
	}
	
	// parachute waypoint is meant as a check gate ... you can pass it only if you have the parachute ...
	// nothing is opened or used there
	if (waypoints[wpt_index].flags & W_FL_CHUTE)
	{
		if (waypoints[wpt_index].flags & W_FL_DOOR)
		{
			waypoints[wpt_index].flags &= ~W_FL_DOOR;
			fixed = true;
		}
		
		if (waypoints[wpt_index].flags & W_FL_DOORUSE)
		{
			waypoints[wpt_index].flags &= ~W_FL_DOORUSE;
			fixed = true;
		}
		
		if (waypoints[wpt_index].flags & W_FL_USE)
		{
			waypoints[wpt_index].flags &= ~W_FL_USE;
			fixed = true;
		}

		// you can't go prone during parachuting
		if (waypoints[wpt_index].flags & W_FL_PRONE)
		{
			waypoints[wpt_index].flags &= ~W_FL_PRONE;
			fixed = true;
		}
	}
	
	// 'use door' is a stand-alone waypoint not a combination of door and use waypoints
	if (waypoints[wpt_index].flags & W_FL_DOORUSE)
	{
		if (waypoints[wpt_index].flags & W_FL_DOOR)
		{
			waypoints[wpt_index].flags &= ~W_FL_DOOR;
			fixed = true;
		}

		if (waypoints[wpt_index].flags & W_FL_USE)
		{
			waypoints[wpt_index].flags &= ~W_FL_USE;
			fixed = true;
		}
	}

	// either duckjump or just jump ... you can't do both
	if (waypoints[wpt_index].flags & W_FL_DUCKJUMP)
	{
		if (waypoints[wpt_index].flags & W_FL_JUMP)
		{
			waypoints[wpt_index].flags &= ~W_FL_JUMP;
			fixed = true;
		}
	}

	// Firearms mod does not allow using a claymore mine while crouched
	if (waypoints[wpt_index].flags & W_FL_MINE)
	{
		if (waypoints[wpt_index].flags & W_FL_CROUCH)
		{
			waypoints[wpt_index].flags &= ~W_FL_CROUCH;
			fixed = true;
		}
	}

	// you can't sprint when crouched or proned
	if (waypoints[wpt_index].flags & W_FL_SPRINT)
	{
		if (waypoints[wpt_index].flags & W_FL_CROUCH)
		{
			waypoints[wpt_index].flags &= ~W_FL_CROUCH;
			fixed = true;
		}

		if (waypoints[wpt_index].flags & W_FL_PRONE)
		{
			waypoints[wpt_index].flags &= ~W_FL_PRONE;
			fixed = true;
		}
	}

	// you can be either proned or crouched not both
	if (waypoints[wpt_index].flags & W_FL_PRONE)
	{
		if (waypoints[wpt_index].flags & W_FL_CROUCH)
		{
			waypoints[wpt_index].flags &= ~W_FL_CROUCH;
			fixed = true;
		}

		// also no jump or duckjump when one is proned
		if (waypoints[wpt_index].flags & W_FL_DUCKJUMP)
		{
			waypoints[wpt_index].flags &= ~W_FL_DUCKJUMP;
			fixed = true;
		}

		if (waypoints[wpt_index].flags & W_FL_JUMP)
		{
			waypoints[wpt_index].flags &= ~W_FL_JUMP;
			fixed = true;
		}
	}
	
	// if it happened that this waypoint flag is zero (which causes weird things such as aim & cross connections
	// going to map origin etc.) convert it to deleted flag
	// don't remove this statement - it's a safety catch!
	if (waypoints[wpt_index].flags == 0)
	{
		waypoints[wpt_index].flags |= W_FL_DELETED;
		fixed = true;
	}

	// did we fix anything?
	if (fixed)
	{
		// then make the client know about it (this way it will be visible only when developer mode is on)
		ALERT(at_console, "Waypoint self cleaning fixed invalid flag/tag/type combination on waypoint #%d\n", wpt_index+1);

		return 0;
	}
	// everything was okay
	else
		return 1;
}


/*
* updates path status/flag based on certain waypoints this path includes
*/
void UpdatePathStatus(int path_index)
{
	if (path_index == -1)
		return;

	W_PATH *p;
	int safety_stop = 0;
	bool is_ammo_wpt_present = FALSE;//															NEW CODE 094

	p = w_paths[path_index];

	while (p)
	{
#ifdef _DEBUG
		safety_stop++;
		if (safety_stop > 1000)
		{
			ALERT(at_error, "UpdatePathStatus() | LLERROR\n***Path no. %d\n", path_index + 1);

			WaypointDebug();
		}
#endif
		if (IsWaypoint(p->wpt_index, WptT::wpt_ammobox))
			//UTIL_SetBit(P_FL_MISC_AMMO, w_paths[path_index]->flags);//						NEW CODE 094 (prev code)
		//																						NEW CODE 094
		{
			UTIL_SetBit(P_FL_MISC_AMMO, w_paths[path_index]->flags);
			is_ammo_wpt_present = TRUE;
		}
		//********************************************************************************		NEW CODE 094 END

		if (IsWaypoint(p->wpt_index, WptT::wpt_pushpoint))
			UpdateGoalPathStatus(p->wpt_index, path_index);

		p = p->next;
	}

	//																						NEW CODE 094

	// if the path is marked as 'use me if you are low on ammo'
	// but there was no ammobox waypoint in it then...
	if ((w_paths[path_index] != NULL) && UTIL_IsBitSet(P_FL_MISC_AMMO, w_paths[path_index]->flags) &&
		!is_ammo_wpt_present)
	{
		// fix it
		UTIL_ClearBit(P_FL_MISC_AMMO, w_paths[path_index]->flags);
	}
	//********************************************************************************		NEW CODE 094 END
}


/*
* will scan the surrounding of the pushpoint/flag waypoints for the pushpoint entity
* if we find it then it we'll set correct goal path flag on all paths going over this waypoint
*/
void UpdateGoalPathStatus(int wpt_index, int path_index)
{
	if ((!IsWaypoint(wpt_index, WptT::wpt_pushpoint)) || (path_index == -1))
		return;
	
	// get an origin of the flag waypoint
	Vector wpt_origin = waypoints[wpt_index].origin;
	
	// invalid waypoint
	if (wpt_origin == Vector(0, 0, 0))
		return;
	
	edict_t *pent = NULL;
	// search for any capture point around this waypoint
	while ((pent = UTIL_FindEntityInSphere(pent, wpt_origin, 50)) != NULL)
	{
		if (strcmp(STRING(pent->v.classname), "fa_push_point") == 0)
		{
			int team = pent->v.team;
		
			// set it only for blue team
			// red team just captured or already owns this flag
			if (team == TEAM_RED)
			{
				UTIL_ClearBit(P_FL_MISC_GOAL_RED, w_paths[path_index]->flags);
				UTIL_SetBit(P_FL_MISC_GOAL_BLUE, w_paths[path_index]->flags);
			}
			else if (team == TEAM_BLUE)
			{
				UTIL_ClearBit(P_FL_MISC_GOAL_BLUE, w_paths[path_index]->flags);
				UTIL_SetBit(P_FL_MISC_GOAL_RED, w_paths[path_index]->flags);
			}
			// set it for both teams
			else
			{
				UTIL_SetBit(P_FL_MISC_GOAL_RED, w_paths[path_index]->flags);
				UTIL_SetBit(P_FL_MISC_GOAL_BLUE, w_paths[path_index]->flags);
			}

			break;
		}
	}
}


/*
* goes through all paths and searches one that contains current waypoint
* all return values: index of a path that match, -1 means wpt isn't in any path
*/
int FindPath(int current_wpt_index)
{
	W_PATH *p;
	int path_index;

	int safety_stop = 0;

	for (path_index = 0; path_index < num_w_paths; path_index++)
	{
		// skip free slots
		if (w_paths[path_index] == NULL)
			continue;

		safety_stop = 0;
			
		p = w_paths[path_index];

		// search whole path for current wpt
		while (p)
		{
#ifdef _DEBUG
			safety_stop++;
			if (safety_stop > 1000)
			{
				char msg[128];

				sprintf(msg, "FindPath() | LLERROR\n***Path no. %d || paths already used %d\n",
					path_index + 1, num_w_paths);

				ALERT(at_error, msg);
				UTIL_DebugInFile(msg);

				WaypointDebug();
			}
#endif

			if (p->wpt_index == current_wpt_index)
				return path_index;

			p = p->next;	// check next node
		}
	}

	return -1;	// this wpt isn't in any path
}


/*
* returns path start (ie the first waypoint index in the path)
* returns -1 if path doesn't exist
*/
int GetPathStart(int path_index)
{
	// set the pointer to the head node
	W_PATH *p = w_paths[path_index];

	// get the first waypoint
	if (p)
	{
		return p->wpt_index;
	}

	return -1;
}


/*
* returns path end (ie the last waypoint index in the path)
* returns -1 if path doesn't exist
*/
int GetPathEnd(int path_index)
{
	int the_end_wpt = -1;
	W_PATH *p;

	// check if path exist
	if (w_paths[path_index] == NULL)
		return -1;

	// set the pointer to the head node
	p = w_paths[path_index];

	// go through whole path and update the_end_wpt with path wpt_index
	while (p)
	{
		the_end_wpt = p->wpt_index;

		p = p->next;
	}

	return the_end_wpt;
}


/*
* return previous waypoint to given waypoint in given path if exists of course otherwise return -1
* works only with start to end direction at the moment
*/
int GetPathPreviousWaypoint(int wpt_index, int path_index)
{
	// check validity first
	if (wpt_index == -1 || path_index == -1)
		return -1;

	W_PATH *p = w_paths[path_index];
	int safety_stop = 0;

	while (p)
	{
		// check if there isn't any linked list error
		if (safety_stop > 1000)
		{
#ifdef NDEBUG
			UTIL_DebugInFile("***G_P_P_W|LLE\n");
#endif
#ifdef _DEBUG
			ALERT(at_error, "GetPathPreviousWaypoint() | LLError\n");
			UTIL_DebugDev("GetPathPreviousWaypoint() | LLError\n", -100, path_index + 1);
#endif
			return -1;
		}

		// is the path waypoint index same to the waypoint we are looking for ...
		if (p->wpt_index == wpt_index)
		{
			W_PATH *prev = p->prev;

			// check if there is a previous waypoint and return it
			if (prev)
			{
				return prev->wpt_index;
			}
		}
						
		p = p->next;
		safety_stop++;
	}

	return -1;
}


/*
* returns path length (ie total amount of nodes)
* returns -1 if path doesn't exist
*/
int WaypointGetPathLength(int path_index)
{
	int the_length;
	W_PATH *p;

	// check if path exist
	if (w_paths[path_index] == NULL)
		return -1;

	// init length
	the_length = 0;

	// set the pointer to the head node
	p = w_paths[path_index];

	// go through whole path and increase the_length
	while (p)
	{
		the_length++;

		p = p->next;
	}

	return the_length;
}


/*
* returns TRUE if the flag is set on this path
*/
bool IsPath(int path_index, PathT path_type)
{
	if ((path_index == -1) || (path_type <= PathT::scrapped_flagtype))
		return FALSE;

	if (w_paths[path_index] == NULL)
		return FALSE;

	int flag = 0;

	if (path_type == PathT::path_both)
		flag = P_FL_TEAM_NO;
	else if (path_type == PathT::path_red)
		flag = P_FL_TEAM_RED;
	else if (path_type == PathT::path_blue)
		flag = P_FL_TEAM_BLUE;
	else if (path_type == PathT::path_one)
		flag = P_FL_WAY_ONE;
	else if (path_type == PathT::path_two)
		flag = P_FL_WAY_TWO;
	else if (path_type == PathT::path_patrol)
		flag = P_FL_WAY_PATROL;
	else if (path_type == PathT::path_all)
		flag = P_FL_CLASS_ALL;
	else if (path_type == PathT::path_sniper)
		flag = P_FL_CLASS_SNIPER;
	else if (path_type == PathT::path_mgunner)
		flag = P_FL_CLASS_MGUNNER;
	else if (path_type == PathT::path_ammo)
		flag = P_FL_MISC_AMMO;
	else if (path_type == PathT::path_goal_red)
		flag = P_FL_MISC_GOAL_RED;
	else if (path_type == PathT::path_goal_blue)
		flag = P_FL_MISC_GOAL_BLUE;
	else if (path_type == PathT::path_avoid)
		flag = P_FL_MISC_AVOID;
	else if (path_type == PathT::path_ignore)
		flag = P_FL_MISC_IGNORE;
	else if (path_type == PathT::path_gitem)
		flag = P_FL_MISC_GITEM;
	
	// is the flag set on this path?
	if (w_paths[path_index]->flags & flag)
		return TRUE;

	return FALSE;
}


/*
* used to compare two paths
* returns TRUE if both path1 and path2 have given path type
*/
bool PathMatchingFlag(int path_index1, int path_index2, PathT path_type)
{
	if ((path_index1 == -1) || (path_index2 == -1) || (path_type <= PathT::scrapped_flagtype))
		return FALSE;

	if (IsPath(path_index1, path_type) && IsPath(path_index2, path_type))
		return TRUE;

	return FALSE;
}


/*
* compare two paths based on team limits
* return TRUE if the checked path is less than or equally limitative to the restrictive path
*/
bool PathCompareTeamDownwards(int restrictive_path, int checked_path)
{
	// from red team only path ...
	if (IsPath(restrictive_path, PathT::path_red))
	{
		// we can always access any red team only path or any both team path
		if (PathMatchingFlag(restrictive_path, checked_path, PathT::path_red) || IsPath(checked_path, PathT::path_both))
			return TRUE;
	}

	if (IsPath(restrictive_path, PathT::path_blue))
	{
		if (PathMatchingFlag(restrictive_path, checked_path, PathT::path_blue) || IsPath(checked_path, PathT::path_both))
			return TRUE;
	}

	// from both team path ...
	if (IsPath(restrictive_path, PathT::path_both))
	{
		// we can always access only another both team path
		if (PathMatchingFlag(restrictive_path, checked_path, PathT::path_both))
			return TRUE;
	}

	return FALSE;
}


/*
* compare two paths based on class limits
* return TRUE if the checked path is less than or equally limitative to the restrictive path
*/
bool PathCompareClassDownwards(int restrictive_path, int checked_path)
{
	// from sniper only path we can always ...
	if (IsPath(restrictive_path, PathT::path_sniper))
	{
		// access another sniper path or path for all classes
		if (PathMatchingFlag(restrictive_path, checked_path, PathT::path_sniper) || IsPath(checked_path, PathT::path_all))
			return TRUE;
	}

	if (IsPath(restrictive_path, PathT::path_mgunner))
	{
		if (PathMatchingFlag(restrictive_path, checked_path, PathT::path_mgunner) || IsPath(checked_path, PathT::path_all))
			return TRUE;
	}

	if (IsPath(restrictive_path, PathT::path_all))
	{
		if (PathMatchingFlag(restrictive_path, checked_path, PathT::path_all))
			return TRUE;
	}
	
	return FALSE;
}


/*
* create completely new path starting at current waypoint
*/
bool StartNewPath(int current_wpt_index)
{
	W_PATH *p;
	int free_path_index;

	// is reached the maximum of w_paths
	if (num_w_paths >= MAX_W_PATHS)
	{
		ALERT(at_error, "MarineBot - you've reached the maximum of paths!\n");

		return FALSE;
	}

	// end if is one of these problem things
	if ((current_wpt_index == -1) || (waypoints[current_wpt_index].flags & W_FL_DELETED) ||
		(waypoints[current_wpt_index].flags & W_FL_CROSS) ||
		(waypoints[current_wpt_index].flags & W_FL_AIMING))
		return FALSE;
	
	free_path_index = 0;

	// find the next available slot for the new waypoint
	while (free_path_index < num_w_paths)
	{
		if (w_paths[free_path_index] == NULL)
			break;

		free_path_index++;
	}

	p = (W_PATH *)malloc(sizeof(W_PATH));	// create new head node

	if (p == NULL)
	{
		ALERT(at_error, "MarineBot - Error allocating memory for path!\n");

		return FALSE;
	}

	p->wpt_index = current_wpt_index;	// save the wpt index to first/head node

	p->prev = NULL;		// no previous node yet (this one it the first - the head node)
	p->next = NULL;		// no next node yet

	w_paths[free_path_index] = p;	// store the pointer to new path

	// init the flags value just for sure
	w_paths[free_path_index]->flags = 0;

	// set all flags to default
	w_paths[free_path_index]->flags |= P_FL_TEAM_NO | P_FL_CLASS_ALL | P_FL_WAY_TWO;

	internals.SetPathToContinue(free_path_index);	// save path index to continue in

	// increase total number of paths if adding at the end of the array
	if (free_path_index == num_w_paths)
		num_w_paths++;

	ALERT(at_console, "starting new path ...\n");

	return TRUE;
}


/*
* continue in current path
* returns -3 if invalid wpt (ie this wpt type can't be in path)
* returns -2 if no path to continue (ie internals::path to continue is NO_VAL)
* returns -1 if not enough memory
* returns 0 if path doesn't exist (not sure if this can happen)
* returns 1 if everything is OK
* returns 2 if the wpt is already present in our path							--- NEW CODE 094
*/
int ContinueCurrPath(int current_wpt_index, bool check_presence)
{
	int path_index;

	// don't add these wpt types into paths
	if ((current_wpt_index == -1) || (waypoints[current_wpt_index].flags & (W_FL_DELETED | W_FL_AIMING | W_FL_CROSS)))
		return -3;

	path_index = internals.GetPathToContinue();

	if (path_index == -1)
	{
		ALERT(at_console, "there's no path to continue on waypoint no. %d\n", current_wpt_index + 1);

		return -2;			// no path to continue
	}

	// if the path is valid add current_wpt_index to the end of that path
	if (w_paths[path_index])
	{
		W_PATH *p = w_paths[path_index];	// set the pointer to the head node
		W_PATH *prev_node = NULL;			// temp pointer to previous node
		int safety_stop = 0;

		while (p)
		{
			//																						NEW CODE 094 END
			// by default check if the new waypoint isn't already in this path
			// (ie. the same waypoint would have been added twice in one path), because
			// the bot would then enter infinite loop on such path
			// so all we do here is that we simply skip adding it into the path
			// this check is disabled on map load
			if ((p->wpt_index == current_wpt_index) && check_presence)
				return 2;
			//************************************************************************************	NEW CODE 094 END

			prev_node = p;	// save the previous node in linked list

			p = p->next;	// go to next node in linked list

#ifdef _DEBUG
			safety_stop++;
			if (safety_stop > 1000)
				WaypointDebug();
#endif
		}

		p = (W_PATH *)malloc(sizeof(W_PATH));	// create new node

		if (p == NULL)
		{
			ALERT(at_error, "MarineBot - Error allocating memory for path!\n");

			return -1;		// no memory for path
		}
		
		p->wpt_index = current_wpt_index;	// store new wpt index
		
		p->next = NULL;			// NULL next node

		if (prev_node != NULL)
		{
			p->prev = prev_node;	// save pointer to prev node in linked list

			prev_node->next = p;	// link new node into existing list
		}

	}
	else
		return 0;	// path isn't valid

	return 1;	// everything is OK
}


/*
* insert a waypoint specified in arg2 into path in arg3 between its two waypoints in arg4 and arg5
* allows also insertion to the beginning or end of the path through keyword passed in arg4 or arg5
*/
int WaypointInsertIntoPath(const char *arg2, const char *arg3, const char *arg4, const char *arg5)
{
	int wpt_index, path_index, path_wpt1, path_wpt2;
	W_PATH *p;
	int special_case;		// holds 1 when we are inserting waypoint right to the start of the path
							// holds 2 when we are inserting waypoint to the end of the path
							// holds 0 when the waypoint is inserted 'into' path (between its two waypoints)

	// init all values
	wpt_index = path_index = path_wpt1 = path_wpt2 = -1;
	special_case = 0;										// we assume the insertion is 'into' the path by default

	// get the index of waypoint we want to insert if exist
	if ((arg2 != NULL) && (*arg2 != 0))
	{
		wpt_index = atoi(arg2);
		// due to the array based style
		wpt_index--;
	}

	// get the index of path which we want to insert into if exist
	if ((arg3 != NULL) && (*arg3 != 0))
	{
		path_index = atoi(arg3);
		path_index--;
	}

	// get the index of waypoint we want to insert behind if exist
	if ((arg4 != NULL) && (*arg4 != 0))
	{
		if (FStrEq(arg4, "start") || FStrEq(arg4, "beginning") || FStrEq(arg4, "tostart") || FStrEq(arg4, "to_start") ||
			FStrEq(arg4, "tobeginning") || FStrEq(arg4, "to_beginning"))
			special_case = 1;
		else if (FStrEq(arg4, "end") || FStrEq(arg4, "toend") || FStrEq(arg4, "to_end"))
			special_case = 2;
		else
		{
			path_wpt1 = atoi(arg4);
			path_wpt1--;
		}
	}

	// get the index of waypoint we want to insert before if exist
	if ((special_case == 0) && (arg5 != NULL) && (*arg5 != 0))
	{
		if (FStrEq(arg5, "start") || FStrEq(arg5, "beginning") || FStrEq(arg5, "tostart") || FStrEq(arg5, "to_start") ||
			FStrEq(arg5, "tobeginning") || FStrEq(arg5, "to_beginning"))
			special_case = 1;
		else if (FStrEq(arg5, "end") || FStrEq(arg5, "toend") || FStrEq(arg5, "to_end"))
			special_case = 2;
		else
		{
			path_wpt2 = atoi(arg5);
			path_wpt2--;
		}
	}

	char msg[128];

	if (special_case == 1)
		sprintf(msg, "Trying to insert waypoint #%d to the beginning of path #%d ...\n", wpt_index + 1, path_index + 1);
	else if (special_case == 2)
		sprintf(msg, "Trying to insert waypoint #%d to the end of path #%d ...\n", wpt_index + 1, path_index + 1);
	else
		sprintf(msg, "Trying to insert waypoint #%d into path #%d between wpts #%d and #%d ...\n",
			wpt_index + 1, path_index + 1, path_wpt1 + 1, path_wpt2 + 1);

	PrintOutput(NULL, msg);

	// is one of path waypoints same to the waypoint for insertion
	if ((wpt_index == path_wpt1) || (wpt_index == path_wpt2))
		return -6;

	// are both path waypoints same and NOT special insertion
	if ((path_wpt1 == path_wpt2) && (special_case == 0))
		return -5;

	// is the waypoint for insertion valid
	if ((wpt_index == -1) || (waypoints[wpt_index].flags == 0) ||
		(waypoints[wpt_index].flags & (W_FL_DELETED | W_FL_AIMING | W_FL_CROSS)))
		return -4;

	// does the path exist or has the path less then two waypoints in case of common insertion
	if ((path_index == -1) || ((WaypointGetPathLength(path_index) < 2) && (special_case == 0)))
		return -3;

	// waypoint already is in this path
	if (WaypointIsInPath(wpt_index, path_index))
		return -6;

	// if we are inserting 'into' path (ie. between its two waypoints) then we must check their validity
	if ((special_case == 0) && ((path_wpt1 == -1) || (WaypointIsInPath(path_wpt1, path_index) == FALSE)))
		return -2;

	// if we are inserting 'into' path (ie. between its two waypoints) then we must check their validity
	if ((special_case == 0) && ((path_wpt2 == -1) || (WaypointIsInPath(path_wpt2, path_index) == FALSE)))
		return -1;

	// if we are inserting the waypoint at the beginning or end of the path
	// then just point at the path in the array of all paths
	if (special_case != 0)
		p = w_paths[path_index];
	// otherwise we must find one of the two path waypoints to get a pointer on it
	else
		p = GetWaypointPointer(path_wpt1, path_index);

	// check if the pointer exists
	if (p)
	{
		W_PATH *next;
		W_PATH *new_node = NULL;	// a node for the waypoint we need to insert

		// first we will handle the case when we are inserting new waypoint at the beginning of the path
		if (special_case == 1)
		{
			new_node = (W_PATH *)malloc(sizeof(W_PATH));	// create new node

			if (new_node == NULL)
			{
				ALERT(at_error, "MarineBot - Error allocating memory for path!\n");
				return 0;		// no memory for path
			}

			p->prev = new_node;		// put the new node in front of the first path waypoint
			new_node->next = p;		// and connect the rest of the path behind this new start point
			new_node->prev = NULL;	// NULL this pointer, cause this node is the head node so there's nothing before it

			new_node->wpt_index = wpt_index;	// next step is to store the waypoint index into the new node

			// this path has a new start point now so let's update the array of all paths
			w_paths[path_index] = new_node;

			// finally we must also update the path data
			w_paths[path_index]->flags = p->flags;

			return 1;
		}
		// then we will handle the second special insertion ... at the end of the path
		else if (special_case == 2)
		{
			W_PATH *prev_node = NULL;		// temp pointer to previous node (ie. the end of our path)
			int safety_stop = 0;
			
			// first we must go through the path in order to reach its end point ...
			while (p)
			{
				prev_node = p;				// save the previous node in linked list
				p = p->next;				// go to next node in linked list (ie. go through the path)

#ifdef _DEBUG
				safety_stop++;
				if (safety_stop > 1000)
					WaypointDebug();
#endif
			}
			
			new_node = (W_PATH *)malloc(sizeof(W_PATH));		// create new node
			
			if (new_node == NULL)
			{
				ALERT(at_error, "MarineBot - Error allocating memory for path!\n");
				return 0;
			}
			
			new_node->wpt_index = wpt_index;		// store new waypoint
			new_node->next = NULL;					// NULL next node pointer, cause we are at the end of the path
			
			if (prev_node != NULL)
			{
				// and finally connect the new node to the end of the path
				new_node->prev = prev_node;
				prev_node->next = new_node;

				return 1;
			}

			// seeing the pointer to prev node is NULL we weren't able to go through the path to reach its end point so
			// we must end it here with unknown error
			return 0;
		}

		// this is the default case where we are inserting new waypoint 'into' the path (between its two waypoints)
		// so first go to next waypoint in the path ...
		next = p->next;

		// does the next waypoint exist AND
		// are first and second path waypoints really neighbours (ie. is this waypoint index the one we are looking for)
		if (next && (next->wpt_index == path_wpt2))
		{
			// so insert the new waypoint behind path_wpt1...

			new_node = (W_PATH *)malloc(sizeof(W_PATH));

			if (new_node == NULL)
			{
				ALERT(at_error, "MarineBot - Error allocating memory for path!\n");
				return 0;
			}

			p->next = new_node;		// connect the new node behind path waypoint1
			new_node->prev = p;		// set also the opposite connection

			new_node->wpt_index = wpt_index;

			new_node->next = next;	// connect the other end of the path to this new node
			next->prev = new_node;	// set also the opposite connection

			return 1;
		}

		// next waypoint wasn't the second path waypoint we're looking for so try the previous waypoint
		// doing it this way gives the end-user freedom ... the order of the last two arguments doesn't matter now,
		// because command 'pathwpt insert 7 20 5 6' is the same as 'pathwpt insert 7 20 6 5'
		next = p->prev;

		// does the previous waypoint exist AND is it the one we are looking for
		if (next && (next->wpt_index == path_wpt2))
		{
			// so insert the new waypoint before path_wpt1...

			new_node = (W_PATH *)malloc(sizeof(W_PATH));

			if (new_node == NULL)
			{
				ALERT(at_error, "MarineBot - Error allocating memory for path!\n");
				return 0;
			}

			p->prev = new_node;
			new_node->next = p;

			new_node->wpt_index = wpt_index;

			new_node->prev = next;
			next->next = new_node;

			return 1;
		}
	}

	return 0;
}

/*/																								NEW CODE 094 (prev code)
int WaypointInsertIntoPath(const char *arg2, const char *arg3, const char *arg4, const char *arg5)
{
	int wpt_index, path_index, path_wpt1, path_wpt2;
	W_PATH *p;

	// init all values
	wpt_index = path_index = path_wpt1 = path_wpt2 = -1;

	// get the index of waypoint we want to insert if exist
	if ((arg2 != NULL) && (*arg2 != 0))
	{
		wpt_index = atoi(arg2);
		// due to the array based style
		wpt_index--;
	}
	
	// get the index of path which we want to insert into if exist
	if ((arg3 != NULL) && (*arg3 != 0))
	{
		path_index = atoi(arg3);
		path_index--;
	}

	// get the index of waypoint we want to insert behind if exist
	if ((arg4 != NULL) && (*arg4 != 0))
	{
		path_wpt1 = atoi(arg4);
		path_wpt1--;
	}
	
	// get the index of waypoint we want to insert before if exist
	if ((arg5 != NULL) && (*arg5 != 0))
	{
		path_wpt2 = atoi(arg5);
		path_wpt2--;
	}

	char msg[128];

	sprintf(msg, "Trying to insert waypoint #%d into path #%d between wpts #%d and #%d ...\n",
		wpt_index + 1, path_index + 1, path_wpt1 + 1, path_wpt2 + 1);
	PrintOutput(NULL, msg);

	// is one of path waypoints same to the waypoint for insertion
	if ((wpt_index == path_wpt1) || (wpt_index == path_wpt2))
		return -6;

	// are both path waypoints same
	if ((path_wpt1 == path_wpt2))
		return -5;

	// is the waypoint for insertion valid
	if ((wpt_index == -1) || (waypoints[wpt_index].flags == 0) || 
		(waypoints[wpt_index].flags & (W_FL_DELETED | W_FL_AIMING | W_FL_CROSS)))
		return -4;

	// does the path exist or has the path less then two waypoints
	if ((path_index == -1) || (WaypointGetPathLength(path_index) < 2))
		return -3;

	// waypoint already is in this path
	if (WaypointIsInPath(wpt_index, path_index))
		return -6;

	// is this path waypoint valid
	if ((path_wpt1 == -1) || (WaypointIsInPath(path_wpt1, path_index) == FALSE))
		return -2;

	// is this path waypoint valid
	if ((path_wpt2 == -1) || (WaypointIsInPath(path_wpt2, path_index) == FALSE))
		return -1;

	// find the first path waypoint, get a pointer on it
	p = GetWaypointPointer(path_wpt1, path_index);

	// if it exists find the second path waypoint
	if (p)
	{
		W_PATH *next;
		W_PATH *new_node = NULL;	// a node for the waypoint we need to insert

		// go to next waypoint in the path
		next = p->next;

		// does the next waypoint exist AND
		// are first and second path waypoints neighbours ie are they right next to each other
		if (next && (next->wpt_index == path_wpt2))
		{
			// so insert the new one behind path_wpt1...

			new_node = (W_PATH *)malloc(sizeof(W_PATH));	// create new node

			p->next = new_node;		// connect the new node behind path waypoint1
			new_node->prev = p;		// set also the opposite connection

			new_node->wpt_index = wpt_index;	// store the waypoint index into the new node

			new_node->next = next;	// connect the other end of the path to this new node
			next->prev = new_node;	// set also the opposite connection

			return 1;
		}

		// next waypoint wasn't the second path waypoint we're looking for
		// so try the previous waypoint
		next = p->prev;

		// does the next waypoint exist AND are they neighbours
		if (next && (next->wpt_index == path_wpt2))
		{
			// so insert the new one before path_wpt1...

			new_node = (W_PATH *)malloc(sizeof(W_PATH));

			p->prev = new_node;
			new_node->next = p;

			new_node->wpt_index = wpt_index;

			new_node->prev = next;
			next->next = new_node;

			return 1;
		}
	}

	return 0;
}
/**/


/*
* remove waypoint (passed by index - current_wpt_index) from specified (path_index) path
*/
int ExcludeFromPath(int current_wpt_index, int path_index)
{
	int path_wpt_index = -1;
	int safety_stop = 0;

	// check if waypoint and path_index are valid
	if ((current_wpt_index == -1) || (path_index == -1))
		return -2;

	// set the pointer to the head node
	W_PATH *p = w_paths[path_index];

	// does the path exist
	if (w_paths[path_index] == NULL)
		return -2;

	// get index from head node
	path_wpt_index = p->wpt_index;

	// search for current_wpt_index in path
	while ((p) && (current_wpt_index != path_wpt_index))
	{
		// get wpt index from this node
		path_wpt_index = p->wpt_index;

		// are both same so the node is found
		if (current_wpt_index == path_wpt_index)
			break;

		// go to next node in linked list
		p = p->next;

#ifdef _DEBUG
		safety_stop++;
		if (safety_stop > 1000)
		{
			ALERT(at_error, "ExcludeFromPath() | LLError\n");
			WaypointDebug();
		}
#endif
	}

	return ExcludeFromPath(p, path_index);
}


/*
* remove waypoint (passed by pointer) from specified (path_index) path
*/
int ExcludeFromPath(W_PATH *p, int path_index)
{
	// check validity first
	if ((p == NULL) || (path_index == -1))
		return -1;

	// exist prev node as well as next node
	if ((p->prev) && (p->next))
	{
		p->prev->next = p->next;	// link previous node (its next pointer) to next node
	}
	// if exist only prev node (removing last wpt from ll)
	else if (p->prev)
		p->prev->next = NULL;	// NULL prev pointer for next node

	// exist prev node as well as next node
	if ((p->next) && (p->prev))
	{
		p->next->prev = p->prev;	// link next node (its prev pointer) to previous node
	}
	// if exist only next node (removing first wpt from ll)
	else if (p->next)
	{
		p->next->prev = NULL;	// NULL prev pointer for next node

		// temp store path flags for transfer
		int path_flags = w_paths[path_index]->flags;

		w_paths[path_index] = p->next;	// update paths array

		// write them back to path
		w_paths[path_index]->flags = path_flags;
	}
	// if exist only head node ie only one wpt in whole path
	else if ((p->next == NULL) && (p->prev == NULL))
	{
		// free this path slot
		w_paths[path_index] = NULL;
	}

	// free/destroy this node
	free(p);

	return 1;	// everything is OK
}

/*																							NEW CODE 094 (prev code)
int ExcludeFromPath(int current_wpt_index, int path_index)
{
	int path_wpt_index = -1;
	int safety_stop = 0;

	// check if waypoint and path_index are valid
	if ((current_wpt_index == -1) || (path_index == -1))
		return -2;

	// set the pointer to the head node
	W_PATH *p = w_paths[path_index];

	// does the path exist
	if (w_paths[path_index] == NULL)
		return -2;

	// get index from head node
	path_wpt_index = p->wpt_index;

	// search for current_wpt_index in path
	while ((p) && (current_wpt_index != path_wpt_index))
	{
		// get wpt index from this node
		path_wpt_index = p->wpt_index;

		// are both same so the node is found
		if (current_wpt_index == path_wpt_index)
			break;

		// go to next node in linked list
		p = p->next;

#ifdef _DEBUG
		safety_stop++;
		if (safety_stop > 1000)
		{
			ALERT(at_error, "ExcludeFromPath() | LLError\n");
			WaypointDebug();
		}
#endif
	}

	// current_wpt_index is NOT in path
	if ((p == NULL) || (path_wpt_index == -1))
		return -1;

	// current_wpt_index is in path
	if (current_wpt_index == path_wpt_index)
	{
		// exist prev node as well as next node
		if ((p->prev) && (p->next))
		{
			p->prev->next = p->next;	// link previous node (its next pointer) to next node
		}
		// if exist only prev node (removing last wpt from ll)
		else if (p->prev)
			p->prev->next = NULL;	// NULL prev pointer for next node

		// exist prev node as well as next node
		if ((p->next) && (p->prev))
		{
			p->next->prev = p->prev;	// link next node (its prev pointer) to previous node
		}
		// if exist only next node (removing first wpt from ll)
		else if (p->next)
		{
			p->next->prev = NULL;	// NULL prev pointer for next node

			// temp store path flags for transfer
			int path_flags = w_paths[path_index]->flags;

			w_paths[path_index] = p->next;	// update paths array

			// write them back to path
			w_paths[path_index]->flags = path_flags;
		}
		// if exist only head node ie only one wpt in whole path
		else if ((p->next == NULL) && (p->prev == NULL))
		{
			// free this path slot
			w_paths[path_index] = NULL;
		}

		// free/destroy this node
		free(p);

		return 1;	// everything is OK
	}

	return 0;	// something went wrong
}
*/


/*
* remove whole path (frees linked list) and null the pointer in w_paths array
*/
bool DeleteWholePath(int path_index)
{
	// check again if the path is valid (ie if exist)
	if (path_index == -1 || w_paths[path_index] == NULL)
	{
		/* // should be logged into some MB debug file
		if (debug_)
		{
			fp=fopen(debug_fname,"a");
			fprintf(fp,"WaypointDebug: PATH DOESN'T EXIST!!!\n");
			fclose(fp);
		}
		*/

		return FALSE;
	}
	
	W_PATH *p = w_paths[path_index];	// set the pointer to the head node
	W_PATH *p_next;

	int safety_stop = 0;		// to stop it if encounter an error

	while (p)
	{
		p_next = p->next;	// save the link to next
		p->prev = NULL;		// clear pointer on prev node
		free(p);			// free this node
		p = p_next;			// update the head node

#ifdef _DEBUG
		safety_stop++;
		if (safety_stop > 1000)
			WaypointDebug();
#endif
	}

	w_paths[path_index] = NULL;

	return TRUE;
}


/*
* return a pointer on a waypoint if it is in the path otherwise return NULL
*/
W_PATH *GetWaypointPointer(int wpt_index, int path_index)
{
	if ((path_index < 0) || (path_index >= MAX_W_PATHS))//																NEW CODE 094
		return NULL;

	W_PATH *p = w_paths[path_index];
	int safety_stop = 0;

	while (p)
	{
		// check if there isn't any linked list error
		if (safety_stop > 1000)
		{
#ifdef NDEBUG
			UTIL_DebugInFile("***W_I_I_P|LLE\n");
#endif
#ifdef _DEBUG
			ALERT(at_error, "WaypointIsInPath() | LLError\n");
			UTIL_DebugDev("WaypointIsInPath() | LLError\n", -100, path_index + 1);
#endif
			return NULL;
		}

		// is the path wpt index same to searched wpt index
		if (p->wpt_index == wpt_index)
			return p;
						
		p = p->next;
		safety_stop++;
	}

	return NULL;
}


/*
* returns TRUE if waypoint (wpt_index) is in path (path_index)
* returns FALSE if waypoint is NOT in path or waypoint/path is invalid
*/
bool WaypointIsInPath(int wpt_index, int path_index)
{
	//if ((wpt_index == -1) || (path_index == -1))
	if ((wpt_index == -1) || (path_index < 0))
		return FALSE;

	if (GetWaypointPointer(wpt_index, path_index) != NULL)
		return TRUE;	

	return FALSE;
}


/*
* return a pointer on a waypoint of given type if it is in the path otherwise return NULL
*/
W_PATH *GetWaypointTypePointer(WptT flag_type, int path_index)
{
	W_PATH *p = w_paths[path_index];
	int safety_stop = 0;

	while (p)
	{
		if (safety_stop > 1000)
		{
			UTIL_DebugInFile("***GetWTypePointer() | LLE\n");
#ifdef _DEBUG
			ALERT(at_error, "GetWTypePointer() | LLError\n");
#endif
			return NULL;
		}

		// is this path waypoint the type we are looking for?
		if (IsWaypoint(p->wpt_index, flag_type))
			return p;
						
		p = p->next;
		safety_stop++;
	}

	return NULL;
}


/*
* returns TRUE if there is given waypoint type in path (path_index)
* returns FALSE if waypoint is NOT in path or waypoint/path is invalid
*/
bool WaypointTypeIsInPath(WptT flag_type, int path_index)
{
	if (path_index == -1)
		return FALSE;

	if (GetWaypointTypePointer(flag_type, path_index) != NULL)
		return TRUE;	

	return FALSE;
}


/*
* returns TRUE if waypoint (wpt_index) is closer to end of the path (path_index)
*/
bool WaypointIsPathEndCloser(int wpt_index, int path_index)
{
	if ((wpt_index == -1) || (path_index == -1))
		return FALSE;

	W_PATH *p = w_paths[path_index];
	int safety_stop = 0, nodes_count = 0, wpt_position = 0;

	while (p)
	{
		// check if there isn't any linked list error
		if (safety_stop > 1000)
		{
#ifdef NDEBUG
			UTIL_DebugInFile("***W_I_P_E_C|LLE\n");
#endif
#ifdef _DEBUG
			ALERT(at_error, "WaypointIsPathEndCloser() | LLError\n");
			UTIL_DebugDev("WaypointIsPathEndCloser() | LLError\n", -100, path_index + 1);
#endif
			return FALSE;
		}

		// is the path wpt index same to searched wpt index
		if (p->wpt_index == wpt_index)
			wpt_position = nodes_count;
						
		p = p->next;

		nodes_count++;
	}

	// is wpt closer than centre of the path return FALSE
	if (wpt_position < int (nodes_count/2))
		return FALSE;
	// otherwise wpt is closer to path end
	else
		return TRUE;
}


/*
* checks if bot can use the path (ie does bot team & class match to paths flags)
* returns TRUE if bot can use it
* returns FALSE if can't
*/
bool IsPathPossible(bot_t *pBot, int path_index)
{
	if (path_index == -1)
		return FALSE;

	// is bot in the same team this path is created for
	if (((w_paths[path_index]->flags & P_FL_TEAM_RED) && (pBot->bot_team == TEAM_RED)) ||
		((w_paths[path_index]->flags & P_FL_TEAM_BLUE) && (pBot->bot_team == TEAM_BLUE)))
	{
		// is it a patrol type path AND bot is NOT defender (ie only defenders can use this path)
		if ((w_paths[path_index]->flags & P_FL_WAY_PATROL) && !(pBot->bot_behaviour & DEFENDER))
			return FALSE;

		// is this path for snipers AND bot is sniper
		if ((w_paths[path_index]->flags & P_FL_CLASS_SNIPER) && (pBot->bot_behaviour & SNIPER))
			return TRUE;
		// is this path for mgunners AND bot is mgunner
		if ((w_paths[path_index]->flags & P_FL_CLASS_MGUNNER) && (pBot->bot_behaviour & MGUNNER))
			return TRUE;
		// is this path all class path (ie no class restriction)
		if (w_paths[path_index]->flags & P_FL_CLASS_ALL)
			return TRUE;
	}

	// or is that path for both teams
	else if (w_paths[path_index]->flags & P_FL_TEAM_NO)
	{
		if ((w_paths[path_index]->flags & P_FL_WAY_PATROL) && !(pBot->bot_behaviour & DEFENDER))
			return FALSE;

		// is this path only for some classes and has the bot that assignment?

		// is this path for snipers
		if (w_paths[path_index]->flags & P_FL_CLASS_SNIPER)
		{
			//  and bot is sniper OR CQuarter OR Common soldier with Defensive behaviour
			if ((pBot->bot_behaviour & SNIPER) || (pBot->bot_behaviour & CQUARTER) ||
				((pBot->bot_behaviour & COMMON) && (pBot->bot_behaviour & DEFENDER)))
				return TRUE;
		}
		// is this path for mgunners
		if (w_paths[path_index]->flags & P_FL_CLASS_MGUNNER)
		{
			//  and bot is mgunner OR CQuarter OR Common soldier with Defensive behaviour
			if ((pBot->bot_behaviour & MGUNNER) || (pBot->bot_behaviour & CQUARTER) ||
				((pBot->bot_behaviour & COMMON) && (pBot->bot_behaviour & DEFENDER)))
				return TRUE;
		}
		// is this path all class path (ie no class restriction)
		if (w_paths[path_index]->flags & P_FL_CLASS_ALL)
			return TRUE;
	}

	return FALSE;
}


/*
* checks if there is a path on wpt_index if so then we try to find the best path for the bot
* if we find at least one path than we set it as bot's current path and we are returning TRUE
*/
bool CheckPossiblePath(bot_t *pBot, int wpt_index)
{
	// is waypoint valid and are there any paths
	if ((wpt_index == -1) || (num_w_paths < 1))
		return FALSE;

	int path_index = -1;
	const int max_best = 3;		// just pick some decent number
	int best_path[max_best];
	const int max_usables = 8;		// just pick some decent number
	int usable_path[max_usables];


	//@@@@@@@@@
	/*
		#ifdef _DEBUG
		if (botdebugger.IsDebugPaths())
		{
			UTIL_DebugDev("waypoint.cpp | CheckPossiblePAth() begins\n", pBot->curr_wpt_index, pBot->curr_path_index);
			ALERT(at_console, "waypoint.cpp | CheckPossiblePAth() begins\n");
		}
		#endif
		/**/



	// initialize the usable path record first
	for (int i = 0; i < max_usables; i++)
	{
		usable_path[i] = -1;
	}

	// initialize the best path record too
	for (int i = 0; i < max_best; i++)
	{
		best_path[i] = -1;
	}

	// go through all paths
	// first skip paths that don't match bot team and/or class
	// and then check the rest of them for the waypoint (wpt index)
	for (path_index = 0; path_index < num_w_paths; path_index++)
	{
		// skip free slots
		if (w_paths[path_index] == NULL)
			continue;

		// is this path possible for bot (ie do team and class restriction match)
		if (IsPathPossible(pBot, path_index))
		{
			// is the waypoint (wpt_index) in this path
			if (WaypointIsInPath(wpt_index, path_index))
			{
				// skip all one-way paths that end on this waypoint
				if ((w_paths[path_index]->flags & P_FL_WAY_ONE) && (wpt_index == GetPathEnd(path_index)))
					continue;
				
				// store this path as a path the bot can use
				for (int i = 0; i < max_usables; i++)
				{
					if (usable_path[i] == -1)
					{
						usable_path[i] = path_index;
						break;
					}
				}

				// now we will try to find something better, using this system will allow
				// the bot to use also other paths starting on/going over this waypoint
				// the order is based on importance

				// TODO:	Redo this section in a way similar to cross waypoint NEEDS
				//			ie. use incremental value variable
				//
				//			if goalitem & task goal value=+10
				//			if ammo & needammo value=+5
				//			if pushpoint & task goal value=+3
				//			if sniper | mgunner | patrol then value++
				//
				//			no breaks or continues cause we need to read all flags to set correct value/weight

				int chance = RANDOM_LONG(1, 100);

				if (IsPath(path_index, PathT::path_gitem) && pBot->IsTask(TASK_GOALITEM) && (chance < 95))
				{
					// store it if we still have a free slot
					for (int i = 0; i < max_best; i++)
					{
						if (best_path[i] == -1)
						{
							best_path[i] = path_index;
							break;
						}
					}

					// this path is already "marked" so check the next one
					continue;
				}

				if (IsPath(path_index, PathT::path_ammo) && pBot->IsNeed(NEED_AMMO) && (chance < 95))
				{
					for (int i = 0; i < max_best; i++)
					{
						if (best_path[i] == -1)
						{
							best_path[i] = path_index;
							break;
						}
					}

					continue;
				}
				
				if (((IsPath(path_index, PathT::path_goal_red) && (pBot->bot_team == TEAM_RED)) ||
					(IsPath(path_index, PathT::path_goal_blue) && (pBot->bot_team == TEAM_BLUE))) &&
					pBot->IsNeed(NEED_GOAL) && (chance < 90))
				{
					for (int i = 0; i < max_best; i++)
					{
						if (best_path[i] == -1)
						{
							best_path[i] = path_index;
							break;
						}
					}

					continue;
				}
				
				if (IsPath(path_index, PathT::path_sniper) && pBot->IsBehaviour(SNIPER) && (chance < 65))
				{
					for (int i = 0; i < max_best; i++)
					{
						if (best_path[i] == -1)
						{
							best_path[i] = path_index;
							break;
						}
					}

					continue;
				}
				
				if (IsPath(path_index, PathT::path_mgunner) && pBot->IsBehaviour(MGUNNER) && (chance < 65))
				{
					for (int i = 0; i < max_best; i++)
					{
						if (best_path[i] == -1)
						{
							best_path[i] = path_index;
							break;
						}
					}

					continue;
				}
				
				if (IsPath(path_index, PathT::path_patrol) && pBot->IsBehaviour(DEFENDER) && (chance < 50))
				{
					for (int i = 0; i < max_best; i++)
					{
						if (best_path[i] == -1)
						{
							best_path[i] = path_index;
							break;
						}
					}

					continue;
				}
			}
		}
	}

	// did we find any path?
	if (usable_path[0] != -1)
	{
		// did we find any better paths than a common path
		if (best_path[0] != -1)
		{
			// use the one we know that exist in case we don't have anything else
			path_index = best_path[0];

			// now check if we have even more better paths ... skipping the first one as we already marked it for use
			for (int i = 1; i < max_best; i++)
			{
				// is there anything stored in this slot
				if (best_path[i] != -1)
				{
					/*
					// is it a carry goal item path
					if (w_paths[best_path[i]]->flags & P_FL_MISC_GITEM)
					{
						// and we already have such path then in 50% of the time ignore this new one
						if ((w_paths[path_index]->flags & P_FL_MISC_GITEM) && (RANDOM_LONG(1, 100) < 50))
							continue;
						// otherwise pick this new one
						else
							path_index = best_path[i];
					}*/

					int chance = RANDOM_LONG(1, 100);

					// first we need to handle the cases when we already have the most valuable paths ...

					// do we already have a carry goal item path
					if (IsPath(path_index, PathT::path_gitem))
					{
						// then decide its replacing only with another carry goal item path
						if (IsPath(best_path[i], PathT::path_gitem) && (chance < 50))
							path_index = best_path[i];
					}
					// do we have a path that runs through pushpoint
					else if (IsPath(path_index, PathT::path_goal_red) || IsPath(path_index, PathT::path_goal_blue))
					{
						// carry goal item has higher value so always pick that one
						if (IsPath(best_path[i], PathT::path_gitem))
							path_index = best_path[i];

						// decide whether to pick the other pushpoint path or not
						if (IsPath(best_path[i], PathT::path_goal_red) || IsPath(best_path[i], PathT::path_goal_blue))
						{
							if (chance < 50)
								path_index = best_path[i];
						}
					}
					// do we have a path with an ammobox already
					else if (IsPath(path_index, PathT::path_ammo))
					{
						// carry item or pushpoint are more important
						if (IsPath(best_path[i], PathT::path_gitem) ||
							IsPath(best_path[i], PathT::path_goal_red) || IsPath(best_path[i], PathT::path_goal_blue))
							path_index = best_path[i];

						// if it is another ammobox path then decide
						if (IsPath(best_path[i], PathT::path_ammo) && (chance < 50))
							path_index = best_path[i];
					}
					// ... then there are all the other cases (pretty simple though)
					else
					{
						// these are the most important so always pick them
						if (IsPath(best_path[i], PathT::path_gitem) ||
							IsPath(best_path[i], PathT::path_goal_red) || IsPath(best_path[i], PathT::path_goal_blue))
							path_index = best_path[i];
						// otherwise decide yes or no
						else
						{
							if (chance < 50)
								path_index = best_path[i];
						}
					}
				}
			}

			if (botdebugger.IsDebugCross())
			{
				char msg[126];
				
				if (best_path[2] == -1 && best_path[1] == -1)
					sprintf(msg, "Picking path #%d based on current NEEDS on waypoint #%d\n",
						path_index+1, wpt_index+1);
				else if (best_path[2] == -1 && best_path[1] != -1)
					sprintf(msg, "Picking path #%d based on current NEEDS on waypoint #%d. The options were #%d or #%d\n",
						path_index+1, wpt_index+1, best_path[0]+1, best_path[1]+1);
				else
					sprintf(msg, "Picking path #%d based on current NEEDS on waypoint #%d. The options were #%d or #%d or #%d\n",
						path_index+1, wpt_index+1, best_path[0]+1, best_path[1]+1, best_path[2]+1);
				PrintOutput(NULL, msg);
			}
		}
		// otherwise if there're just common paths then pick one randomly
		else
		{
			int found_paths = 0;

			// find how many paths did we have on this waypoint
			for (int i = 0; i < max_usables; i++)
			{
				if (usable_path[i] != -1)
					found_paths++;
			}

			// is there only one path then it must be the stored in the first slot so use it
			if (found_paths == 1)
			{
				path_index = usable_path[0];

				if (botdebugger.IsDebugCross())
				{
					char msg[64];
					
					sprintf(msg, "Picking path #%d on waypoint #%d\n",path_index+1, wpt_index+1);
					PrintOutput(NULL, msg);
				}
			}
			// otherwise (there's more than one path) pick one of them randomly
			else
			{
				// we are working with array indexes so we have to decrease it by 1
				path_index = usable_path[RANDOM_LONG(0, found_paths-1)];

				if (botdebugger.IsDebugCross())
				{
					char msg[126];
					
					sprintf(msg, "Picking path #%d from total of %d paths on waypoint #%d\n",
							path_index+1, found_paths, wpt_index+1);
					PrintOutput(NULL, msg);
				}
			}
		}

		//@@@@@@@@@@@@@@@@@@
		if (w_paths[path_index] == NULL)
		{
#ifdef _DEBUG
			char msg[128];
			sprintf(msg, "<wpt.cpp | Check possible paths()> Oooops the path points to null path!!\n");
			UTIL_DebugDev(msg, wpt_index, path_index);
#endif

			return FALSE;
		}

		// update path history
		pBot->prev_path_index = pBot->curr_path_index;
		
		// set the new path
		pBot->curr_path_index = path_index;
		
		// if waypoint is closer to the path end AND NOT on one-way path
		// use opposite direction path moves
		if ((WaypointIsPathEndCloser(wpt_index, path_index)) && !(w_paths[path_index]->flags & P_FL_WAY_ONE))
			pBot->opposite_path_dir = TRUE;

		// for debugging
		if (botdebugger.IsDebugPaths())
		{
			ALERT(at_console, "<<PATHS>> ***Got new path! (index=%d)\n", pBot->curr_path_index + 1);
		}

		return TRUE;
	}

	return FALSE;
}


/*
* scan all paths on the waypoint and return the appropriate value of the waypoint in terms
* of usefulness that fits best current bot needs
*/
void WaypointSetValue(bot_t *pBot, int wpt_index, WAYPOINT_VALUE *wpt_value)
{
	if (wpt_index == -1)
		return;

	int chance = RANDOM_LONG(1, 100);
	int last_good = -1;

	for (int path_index = 0; path_index < num_w_paths; path_index++)
	{
		// skip free slots
		if (w_paths[path_index] == NULL)
			continue;

		// not in this path then skip it
		if (WaypointIsInPath(wpt_index, path_index) == FALSE)
			continue;

		// skip paths dedicated to opposite team
		if (((w_paths[path_index]->flags & P_FL_TEAM_RED) && (pBot->bot_team == TEAM_BLUE)) ||
			((w_paths[path_index]->flags & P_FL_TEAM_BLUE) && (pBot->bot_team == TEAM_RED)))
			continue;

		// skip the last visited path
		if (path_index == pBot->prev_path_index)
			continue;

		// skip one-way paths ending on this waypoint
		if ((w_paths[path_index]->flags & P_FL_WAY_ONE) && (wpt_index == GetPathEnd(path_index)))
			continue;

		// the order is based on importance

		if ((w_paths[path_index]->flags & P_FL_MISC_GITEM) && pBot->IsTask(TASK_GOALITEM) &&
			(chance < 95))
		{
			wpt_value->wpt_index = wpt_index;
			wpt_value->wpt_value = 95;

			return;
		}

		if ((w_paths[path_index]->flags & P_FL_MISC_AMMO) && pBot->IsNeed(NEED_AMMO) &&
			(chance < 95))
		{
			wpt_value->wpt_index = wpt_index;
			wpt_value->wpt_value = 95;

			return;
		}

		if ((((w_paths[path_index]->flags & P_FL_MISC_GOAL_RED) && (pBot->bot_team == TEAM_RED)) ||
			((w_paths[path_index]->flags & P_FL_MISC_GOAL_BLUE) && (pBot->bot_team == TEAM_BLUE))) &&
			pBot->IsNeed(NEED_GOAL) && (chance < 90))
		{
			wpt_value->wpt_index = wpt_index;
			wpt_value->wpt_value = 90;

			return;
		}

		if ((w_paths[path_index]->flags & P_FL_CLASS_SNIPER) && pBot->IsBehaviour(SNIPER) &&
			(chance < 75))
		{
			// if there already is a path that fits bots needs then use random chance to
			// decide if we overwrite the last values
			if (last_good == -1 || RANDOM_LONG(1, 100) < 50)
			{
				wpt_value->wpt_index = wpt_index;
				wpt_value->wpt_value = 75;
				last_good = wpt_index;
			}
		}

		if ((w_paths[path_index]->flags & P_FL_CLASS_MGUNNER) && pBot->IsBehaviour(MGUNNER) &&
			(chance < 75))
		{
			if (last_good == -1 || RANDOM_LONG(1, 100) < 50)
			{
				wpt_value->wpt_index = wpt_index;
				wpt_value->wpt_value = 75;
				last_good = wpt_index;
			}
		}

		if ((w_paths[path_index]->flags & P_FL_WAY_PATROL) && pBot->IsBehaviour(DEFENDER) &&
			(chance < 50))
		{
			if (last_good == -1 || RANDOM_LONG(1, 100) < 25)
			{
				wpt_value->wpt_index = wpt_index;
				wpt_value->wpt_value = 50;
				last_good = wpt_index;
			}
		}
	}

	return;
}


/*
* check if waypoint is possible for the bot
* waypoint is okay if bot team, class matches with at least one path on this waypoint
* if there is no path on this waypoint then it is possible as well
*/
bool IsWaypointPossible(bot_t *pBot, int wpt_index)
{
	// aim waypoints are always restricted to head toward them
	// just like any deleted waypoint
	if (waypoints[wpt_index].flags & (W_FL_AIMING | W_FL_DELETED))
		return FALSE;

	// cross waypoints are always possible
	if (waypoints[wpt_index].flags & W_FL_CROSS)
		return TRUE;

	// if there are no paths then all other waypoints are valid
	if (num_w_paths < 1)
		return TRUE;

	// to know if/that there are some paths on this waypoint
	int num_of_paths_on_wpt = 0;

	// go through all paths
	// first we must check if the waypoint is in path to get correct amount of paths on it
	// and then check if that/those path/paths is/are acceptable for this bot
	for (int path_index = 0; path_index < num_w_paths; path_index++)
	{
		// skip free path slots
		if (w_paths[path_index] == NULL)
			continue;
		
		// is the waypoint (wpt_index) in this path
		if (WaypointIsInPath(wpt_index, path_index))
		{
			// mark this waypoint as a pathwpt ie there's some path on it
			// we also want to know the exact amount of paths on this waypoint
			num_of_paths_on_wpt++;

			// is this path possible for bot
			// ie do bot's team & class and path team & class restriction match
			if (IsPathPossible(pBot, path_index))
			{
				// is this waypoint the ending waypoint of one-way path
				if ((w_paths[path_index]->flags & P_FL_WAY_ONE) && (wpt_index == GetPathEnd(path_index)))
					continue;
				else
					return TRUE;
			}
		}
	}

	// there are paths on this wpt, but they don't match so waypoint cannot be used
	if (num_of_paths_on_wpt > 0)
		return FALSE;

	// there is no path on this waypoint so it can be used
	return TRUE;
}

//SECTION BY kota@

/*
* Search wpt in path, return W_PATH * or NULL if false
*/
W_PATH * FindPointInPath (int wpt_index, int path_index)
{
	if (path_index == -1)
		return NULL;

	W_PATH *cur_w_path = w_paths[path_index];

	while (cur_w_path)
	{
		if (cur_w_path->wpt_index == wpt_index)
		{
			return cur_w_path;
		}

		cur_w_path = cur_w_path->next;
	}

	return NULL;
}

//SECTION BY kota@ END


/*
* checks if waypoint flag is set for this bot
*/
bool IsFlagSet(int flag, int wpt_index, int team) 
{
	if (waypoints[wpt_index].flags & flag)
	{
		switch (team) 
		{
		case TEAM_RED:
			if(WaypointGetPriority(wpt_index, TEAM_RED) != 0) 
			{
				return true;
			}
			break;
		case TEAM_BLUE:
			if (WaypointGetPriority(wpt_index, TEAM_BLUE) != 0) 
			{
				return true;
			}
			break;
		default:	//don't check team, only flag's set.
			return true;
		}
	}
	return false;
}


/*
* returns the correct waypoints priority based on team
*/
int WaypointGetPriority(int wpt_index, int team)
{	
	if (wpt_index == -1)
		return 0;			// like no priority

	if (IsWaypoint(wpt_index, WptT::wpt_trigger))
		return WaypointReturnTriggerWaypointPriority(wpt_index, team);

	if (team == TEAM_RED)
		return waypoints[wpt_index].red_priority;

	if (team == TEAM_BLUE)
		return waypoints[wpt_index].blue_priority;

	return 0;
}

/*
* returns the correct waypoints wait time based on team
*/
float WaypointGetWaitTime(int wpt_index, int team, const char* loc)
{
	if (wpt_index == -1)
		return 0.0;			// like no wait time

	if (team == TEAM_RED)
		return waypoints[wpt_index].red_time;

	if (team == TEAM_BLUE)
		return waypoints[wpt_index].blue_time;

	return 0.0;
}

/*
* sets the next waypoint for bot to head towards
* fixed by Alexander
*/
int WaypointFindAvailable(bot_t *pBot)
{
	// maximum of waypoints around cross waypoint that are taken to final decision routine
	const int max_at_cross = 8;

	int i, j, high_prior_count, the_source, the_found, priority, num_found_wpt;
	int w_index[max_at_cross], w_prior[max_at_cross];
	float distance;
	TraceResult tr;

	if (num_waypoints < 1)
		return -1;

	// store the current waypoint index
	the_source = pBot->curr_wpt_index;

	// is current waypoint a goback waypoint?
	if (waypoints[the_source].flags & W_FL_GOBACK)
	{
		// take the previous waypoint as the waypoint to continue to (ie turn back)
		// only if team priority isn't set to no priority
		if (WaypointGetPriority(the_source, pBot->bot_team) != 0)
		{
			// get last visited waypoint
			int wpt_index = pBot->prev_wpt_index.get();

			// clear visited waypoints history
			pBot->prev_wpt_index.clear();

			return wpt_index;
		}
	}

	// is current waypoint a crossroad waypoint?
	if (waypoints[the_source].flags & W_FL_CROSS)
	{


//@@@@@@@@@@@@@@@@
		/*
#ifdef _DEBUG
		char dmsg[256];
		sprintf(dmsg, "Curr wpt %d | Curr path %d   Prev path %d\n",
			pBot->curr_wpt_index+1, pBot->curr_path_index+1, pBot->prev_path_index+1);

		ALERT(at_console, dmsg);
		//UTIL_DebugDev(dmsg);

		sprintf(dmsg, "Prev visited wpts are: %d %d %d %d %d\n",
			pBot->prev_wpt_index.get(0)+1, pBot->prev_wpt_index.get(1)+1,
			pBot->prev_wpt_index.get(2)+1, pBot->prev_wpt_index.get(3)+1,
			pBot->prev_wpt_index.get(4)+1);

		ALERT(at_console, dmsg);
		//UTIL_DebugDev(dmsg);
#endif
		/**/




		// initialize the possible waypoints arrays
		for (i = 0; i < max_at_cross; i++)
		{
			w_index[i] = -1;
			w_prior[i] = MAX_WPT_PRIOR;
		}

		num_found_wpt = 0;

		// go through all waypoints
		for (i = 0; i < num_waypoints; i++)
		{
			// skip all "invalid" waypoints
			if (waypoints[i].flags & (W_FL_CROSS | W_FL_AIMING | W_FL_DELETED))
				continue;

			// skip current waypoint and waypoints the bot already visited
			if ((i == the_source) || pBot->prev_wpt_index.check(i)) 
			{
				continue;
			}

			// get distance to this waypoint
			//distance = (waypoints[i].origin - waypoints[the_source].origin).Length();									NEW CODE 094 (prev code)
			distance = GetWaypointDistance(i, the_source);//															NEW CODE 094

			// waypoint must be far enough, but not too far
			// ie. must be in the range of this cross waypoint
			if ((distance > 10) && (distance <= waypoints[the_source].range))
			{
				// get the priority of this waypoint
				priority = WaypointGetPriority(i, pBot->bot_team);

				// skip all waypoints with no priority
				if (priority == 0)
					continue;

				// skip all waypoints that are in a path that doesn't match bot team and/or class
				if ((IsWaypointPossible(pBot, i)) == FALSE)
					continue;

			    // make sure the waypoint is reachable (i.e. not behind wall, rock, etc.)
				UTIL_TraceLine(waypoints[the_source].origin, waypoints[i].origin, ignore_monsters, ignore_glass,
					pBot->pEdict, &tr);

				// line of sight is not established so try to find another waypoint
				if (tr.flFraction != 1.0)
					continue;

				// find a free position and store this waypoint on it
				for (j = 0; j < max_at_cross; j++)
				{
					if (w_index[j] == -1)
					{
						w_index[j] = i;
						w_prior[j] = priority;

						// store this waypoint just once
						break;
					}
				}

				// count the found waypoints
				num_found_wpt++;
			}
		}

		// take only max_at_cross (8) waypoints
		if (num_found_wpt > max_at_cross)
			num_found_wpt = max_at_cross;

		if (botdebugger.IsDebugCross())
		{
			char msg[256];
			
			sprintf(msg, "<<CROSS>>Number of found wpts is %d || The wpts are (index|priority): #%d|%d #%d|%d #%d|%d #%d|%d #%d|%d #%d|%d #%d|%d #%d|%d\n",
				num_found_wpt, w_index[0]+1, w_prior[0], w_index[1]+1, w_prior[1], w_index[2]+1, w_prior[2],
				w_index[3]+1, w_prior[3], w_index[4]+1, w_prior[4], w_index[5]+1, w_prior[5],
				w_index[6]+1, w_prior[6], w_index[7]+1, w_prior[7]);
			PrintOutput(NULL, msg);
		}

		// make sure the bot won't take a waypoint that would get him back to a place he just was
		// (example: a path starting and ending at the same cross waypoint) therefore
		// we have to reduce the priority of the path starting waypoint even below the default (lowest) value
		//if ((pBot->prev_path_index != -1) && (RANDOM_LONG(1, 100) < 90))												NEW CODE 094 (prev code) - no need to make this random, the randomization is later on in decision if he should act predictable or not
		if (pBot->prev_path_index != -1)//																				NEW CODE 094
		{
			for (i = 0; i < num_found_wpt; i++)
			{
				// see if one of the found waypoints is part of the path the bot just followed
				if (WaypointIsInPath(w_index[i], pBot->prev_path_index))
				{
					// if so then set super low priority so this waypoint can barely be taken
					w_prior[i] = MAX_WPT_PRIOR + 1;

					if (botdebugger.IsDebugCross())
					{
						char msg[256];
						char wpt_flags[128];
						WptGetType(w_index[i], wpt_flags);

						sprintf(msg, "<<CROSS>>Lowering the chance to pick wpt #%d<%s> -> it has been used on current path #%d\n",
							w_index[i] + 1, wpt_flags, pBot->prev_path_index + 1);
						PrintOutput(NULL, msg);
					}
				}
			}
		}

		// was at least one waypoint found?
		if (num_found_wpt > 0)
		{
			int choice;
			WAYPOINT_VALUE wpt_value[max_at_cross];
			int best_wpt = -1;
			bool run_value_based_decision = false;
			bool act_predictable = false;																			// NEW CODE 094

			// go through found waypoints to find their value
			for (i = 0; i < num_found_wpt; i++)
			{
				// init this slot value first
				wpt_value[i].wpt_index = -1;
				wpt_value[i].wpt_value = -1;
			
				// set the usefulness value for this waypoint
				WaypointSetValue(pBot, w_index[i], &wpt_value[i]);

				if (wpt_value[i].wpt_index != -1)
					run_value_based_decision = true;
			}

			// were there any "valuable waypoints" at all?
			if (run_value_based_decision)
			{
				int last_value = -1;

				// set the chance to pick a waypoint that hasn't the highest value
				choice = RANDOM_LONG(1, 100);
				
				// go through found waypoints again and compare their values
				// we should pick the one with highest value in most of the time
				for (i = 0; i < num_found_wpt; i++)
				{
					// are there two same weighted waypoints then pick one of them randomly
					if ((last_value == wpt_value[i].wpt_value) && (choice < 50))
					{
						best_wpt = wpt_value[i].wpt_index;
						last_value = wpt_value[i].wpt_value;
					}
					// otherwise try to always pick the most weighted waypoint
					else if ((last_value < wpt_value[i].wpt_value) || (choice < 5))
					{
						best_wpt = wpt_value[i].wpt_index;
						last_value = wpt_value[i].wpt_value;
					}
				}

				// we found the best waypoint for current situation so return it
				// ie. skip the priority based decision
				if (best_wpt != -1)
				{
					if (botdebugger.IsDebugCross())
					{
						char msg[256];
						char wpt_flags[128];
						WptGetType(best_wpt, wpt_flags);
						
						sprintf(msg, "<<CROSS>>Taking wpt based on current NEEDS -> it's wpt #%d<%s>\n", best_wpt + 1, wpt_flags);
						PrintOutput(NULL, msg);
					}
					
					return best_wpt;
				}
			}

			// order found waypoints by priority
			for (i = 0; i < (num_found_wpt - 1); i++)
			{
				for (j = i + 1; j < num_found_wpt; j++)
				{
					// is this waypoint priority lower than previous waypoint priority
					if (w_prior[i] > w_prior[j])
					{
						// swap indexes
						high_prior_count = w_index[i];
						w_index[i] = w_index[j];
						w_index[j] = high_prior_count;

						// swap priorities
						high_prior_count = w_prior[i];
						w_prior[i] = w_prior[j];
						w_prior[j] = high_prior_count;
					}
				}
			}

			// count waypoints that have equal priority as the "first" waypoint, because
			// we know that that waypoint has the highest priority around this cross waypoint
			i = 0;
			high_prior_count = 1;

			while ((i < (num_found_wpt - 1)) && (w_prior[i] == w_prior[i + 1]))
			{
				i++;
				high_prior_count++;
			}

			// make a choice percentage for picking the waypoint
			choice = RANDOM_LONG(1, 100);

			//																								NEW CODE 094

			// set how often the bot should act predictable (ie. behaviour based on waypoint priorities)
			// priority == 5 case cannot fall directly to completely random choice
			// because that would completely disable 'super low priority' system (above) for this priority setting
			// and this system prevents from using the same waypoint & path again where possible

			// if there is at least one priority == 1 (highest) waypoint then in 90% of time we're acting predictable
			if ((w_prior[0] == 1) && (choice > 9))
				act_predictable = true;
			// if there is/are only priority == 2 waypoint/s then act predictable in 80% of time
			else if ((w_prior[0] == 2) && (choice > 19))
				act_predictable = true;
			// is/are there only priority == 3 waypoint/s ... then act predictable in 70% of time
			else if ((w_prior[0] == 3) && (choice > 29))
				act_predictable = true;
			// is/are only priority == 4 waypoint/s then act predictable in 60% of time
			else if ((w_prior[0] == 4) && (choice > 39))
				act_predictable = true;
			// finally there is/are only priority == 5 waypoint/s then act predictable only in 50% of time
			else if ((w_prior[0] == 5) && (choice > 49))
				act_predictable = true;
			
			//******************************************************************************************	NEW CODE 094 END

			//if (choice > 14)																								NEW CODE 094 (prev code)
			if (act_predictable)//																							NEW CODE 094
			{
				// if there is only one waypoint with the highest priority then take it
				if (high_prior_count == 1)
				{
					the_found = w_index[0];
					
					if (botdebugger.IsDebugCross())
					{
						char msg[256];
						char wpt_flags[128];
						WptGetType(the_found, wpt_flags);

						sprintf(msg, "<<CROSS>>Taking wpt based on priority -> HIGHEST ONE is wpt #%d<%s>\n", the_found + 1,
							wpt_flags);
						PrintOutput(NULL, msg);
					}
				}
				// there are more waypoints with the same priority 
				else
				{
					// so pick one of them
					choice = RANDOM_LONG(1, high_prior_count);
					the_found = w_index[choice - 1];
					
					if (botdebugger.IsDebugCross())
					{
						char msg[256];
						char wpt_flags[128];
						WptGetType(the_found, wpt_flags);

						sprintf(msg, "<<CROSS>>Taking wpt based on priority -> MULTIPLE same priority wpts -> picking wpt #%d<%s>\n",
							the_found + 1, wpt_flags);
						PrintOutput(NULL, msg);
					}
				}
			}
			// otherwise the bot is acting unpredictably (or at least less predictable)
			else
			{
				// see if there is at least one waypoint with higher priority (but not highest) that means ...
				// 1st position cannot be lowest priority because then there would have been only lowest priority waypoints there
				// also position right behind the highest priority waypoint/waypoints cannot be the default priority
				// because then there would have been only highest priority and the rest would be default priority waypoints
				if ((w_prior[0] != MAX_WPT_PRIOR) && (w_prior[high_prior_count] != MAX_WPT_PRIOR) && (RANDOM_LONG(1, 100) > 30))
				{
					// ignore highest priority waypoints, but still try to find a waypoint with higher priority
					// (ie. look for waypoints with priorities between the highest and the default/lowest one)
					// so start at the position right behind the highest priority waypoint
					i = high_prior_count;
					priority = 0;	// it's used as a counter
					
					while ((i < (num_found_wpt - 1)) && (w_prior[i] == w_prior[i + 1]))
					{
						i++;
						priority++;
					}

					// there's just one waypoint with 2nd highest priority
					if (priority == 0)
					{
						the_found = w_index[high_prior_count];

						if (botdebugger.IsDebugCross())
						{
							char msg[256];
							char wpt_flags[128];
							WptGetType(the_found, wpt_flags);
							
							sprintf(msg, "<CROSS>>Ignoring ONLY highest priority -> picking wpt #%d<%s>\n",
								the_found + 1, wpt_flags);
							PrintOutput(NULL, msg);
						}
					}
					// there must be more of them
					else
					{
						choice = high_prior_count + RANDOM_LONG(0, priority);
						the_found = w_index[choice];

						if (botdebugger.IsDebugCross())
						{
							char msg[256];
							char wpt_flags[128];
							WptGetType(the_found, wpt_flags);
							
							sprintf(msg, "<CROSS>>Ignoring ONLY highest priority -> picking wpt #%d<%s> from total of %d like waypoints\n",
								the_found + 1, wpt_flags, priority + 1);
							PrintOutput(NULL, msg);
						}
					}

					// check for error in the code and "patch" it on the fly
					if (the_found == -1)
					{
						// this one has to exist
						the_found = w_index[0];

						UTIL_DebugInFile("<<BUG>> Cross waypoint decision based on priority -> ignoring highest priority -> refered to non-existent waypoint\n");
					}

				}
				// finally if everything failed then just pick one waypoint randomly
				else
				{
					// pick from all waypoints found around this cross waypoint
					choice = RANDOM_LONG(1, num_found_wpt);
					the_found = w_index[choice - 1];
					
					if (botdebugger.IsDebugCross())
					{
						char msg[256];
						char wpt_flags[128];
						WptGetType(the_found, wpt_flags);
						
						sprintf(msg, "<<CROSS>>Ignoring all priorities -> entirely RANDOM PICK of wpt #%d<%s>\n",
							the_found + 1, wpt_flags);
						PrintOutput(NULL, msg);
					}
				}
			}

			return the_found;
		}
	}
	// otherwise go towards next waypoint in row
	else
	{
		Vector vecWpt;
		bool wpt_visible;
		float the_nearest = MAX_WPT_DIST;
		w_index[0] = -1;

		edict_t *pEdict = pBot->pEdict;

		// find the nearest waypoint
		for (i = 0; i < num_waypoints; i++)
		{
			// skip this waypoint and all the waypoints the bot already visited
			if ((i == the_source) || (i == pBot->end_wpt_index) || pBot->prev_wpt_index.check(i))
			{
				continue;
			}

			// skip the aim and deleted waypoints
			//if (waypoints[i].flags & W_FL_AIMING)																		NEW CODE 094 (prev code)
			if (waypoints[i].flags & (W_FL_AIMING | W_FL_DELETED))//													NEW CODE 094
				continue;

			// skip ladder waypoints if on end ladder
			if ((pBot->waypoint_top_of_ladder) && (waypoints[i].flags & W_FL_LADDER))
				continue;

			//distance = (waypoints[i].origin - waypoints[the_source].origin).Length();									NEW CODE 094 (prev code)
			distance = GetWaypointDistance(i, the_source);//															NEW CODE 094

			// we are looking only for the nearest waypoint
			if (distance > the_nearest)
				continue;

			vecWpt = waypoints[i].origin;

			// is the bot going to leave a ladder so don't check visibility just target the nearest
			if (pBot->waypoint_top_of_ladder)
				wpt_visible = TRUE;
			// is this the first waypoint we are trying to find then we don't need to face it yet
			else if (pBot->prev_wpt_index.get() == -1)
			{
				wpt_visible = FVisible(vecWpt, pEdict);
			}
			else
				wpt_visible = (FInViewCone( &vecWpt, pEdict ) && FVisible( vecWpt, pEdict ));

			// waypoint must be visible/reachable
			if (wpt_visible)
			{
				// skip all waypoints that are in a path that doesn't match bot team and/or class
				if (IsWaypointPossible(pBot, i))
				{
					the_nearest = distance;
					w_index[0] = i;
				}
			}
		}

		if (w_index[0] != -1)
		{
			if (botdebugger.IsDebugWaypoints())
			{
				char msg[256];
				char wpt_flags[128];
				WptGetType(w_index[0], wpt_flags);

				sprintf(msg, "ONLY WPT NAV - Heading towards to waypoint #%d<%s> (curr wpt #%d)\n",
					w_index[0] + 1, wpt_flags, pBot->curr_wpt_index + 1);
				PrintOutput(NULL, msg);
			}

			return w_index[0];	// now the bot has next waypoint
		}
	}

	return -1;
}

/*
* searches the nearest waypoint (checking if we should ignore particular index)
* to the bot and returns its index
* returns -1 if waypoint was NOT found
*/
int WaypointFindFirst(bot_t *pBot, float range, int skip_this_index)
{
	int i, min_index;
	float distance;
	float min_distance;
	TraceResult tr;
	edict_t *pEdict = pBot->pEdict;

	if (num_waypoints < 1)
		return -1;

	// find the nearest waypoint
	min_index = -1;
	min_distance = 9999.0;

	for (i=0; i < num_waypoints; i++)
	{
		// skip any deleted waypoints
		if (waypoints[i].flags & W_FL_DELETED)
			continue;

		// skip any aiming waypoints
		if (waypoints[i].flags & W_FL_AIMING)
			continue;

		// skip team no priority waypoints
		if (((waypoints[i].red_priority == 0) && (pBot->bot_team == TEAM_RED)) ||
			((waypoints[i].blue_priority == 0) && (pBot->bot_team == TEAM_BLUE)))
			continue;

		// check if we should ignore these indexes, bot had some problems at that wpt
		// (most probably bot got stuck there)
		if ((i == skip_this_index) || pBot->prev_wpt_index.check(i))
		{
			continue;
		}

		// is this waypoint origin higher than max jump height then skip it
		if (waypoints[i].origin.z > (pEdict->v.origin.z + 45.0))
			continue;

		distance = (waypoints[i].origin - pEdict->v.origin).Length();

		if ((distance < min_distance) && (distance <= range))
		{
			// is this waypoint acceptable
			// ie is there at least one path which could be used by this bot
			if (IsWaypointPossible(pBot, i))
			{
				// if waypoint is visible from current position (even behind head)
				UTIL_TraceLine(pEdict->v.origin + pEdict->v.view_ofs, waypoints[i].origin,
						ignore_monsters, pEdict->v.pContainingEntity, &tr);

				if (tr.flFraction >= 1.0)
				{
					min_index = i;
					min_distance = distance;
				}
			}
		}
	}

	return min_index;
}

/*
* searches for the nearest waypoint of specified type to the player and returns its index
* returns -1 if waypoint was not found
*/
int WaypointFindNearestType(edict_t *pEntity, float range, int type)
{
	int i, min_index;
	float distance, min_distance;
	TraceResult tr;

	if (num_waypoints < 1)
		return -1;

	// find the nearest waypoint
	min_index = -1;
	min_distance = 9999.0;

	for (i=0; i < num_waypoints; i++)
	{
		// skip any deleted waypoints
		if (waypoints[i].flags & W_FL_DELETED)
			continue;

		// skip all not matching waypoints
		if ((waypoints[i].flags & type) == 0)
			continue;

		distance = (waypoints[i].origin - pEntity->v.origin).Length();

		if ((distance < min_distance) && (distance < range))
		{
			// if waypoint is visible from current position (even behind head)
			UTIL_TraceLine( pEntity->v.origin + pEntity->v.view_ofs, waypoints[i].origin,
				ignore_monsters, pEntity->v.pContainingEntity, &tr );

			if (tr.flFraction >= 1.0)
			{
				min_index = i;
				min_distance = distance;
			}
		}
	}

	return min_index;
}

/*
* returns ending waypoint index of current ladder
* if none is found return -1
*/
// CURRENTLY THIS ISN'T USED AT ALL (There's a call for this in the code, but that call cannot happen, because the condition isn't met)
int FindRightLadderWpt(bot_t *pBot)
{

	// TODO: This needs to be changed to work for both cases. 1) when the ladder is waypointed with the use of ladder type waypoint,
	//		 2) when the ladder is done using just normal waypoints.
	//		 Also it needs to check for waypoints that aren't just straight up or down, because some ladders aren't straight either.


	int i, curr_index;
	float x_curr, y_curr, z_curr;		// current or nearest ladder wpt (start destination)
	float x_end, y_end, z_end;			// end ladder wpt (end destination)

	bool have_it;
	TraceResult tr;

	if (num_waypoints < 1)
		return -1;

	have_it = FALSE;

	// find the nearest ladder wpt
	curr_index = WaypointFindNearestType(pBot->pEdict, 50.0, W_FL_LADDER);

	// is there any ladder wpt nearby
	if (curr_index != -1)
	{
		x_curr = waypoints[curr_index].origin.x;
		y_curr = waypoints[curr_index].origin.y;
		z_curr = waypoints[curr_index].origin.z;
	}
	else
	{
		x_curr = 0.0;
		y_curr = 0.0;
		z_curr = 0.0;
	}

	for (i = 0; i < num_waypoints; i++)
	{
		if (waypoints[i].flags & W_FL_DELETED)
			continue;

		// skip all non ladder wpts
		if ((waypoints[i].flags & W_FL_LADDER) == 0)
			continue;

		x_end = waypoints[i].origin.x;
		y_end = waypoints[i].origin.y;
		z_end = waypoints[i].origin.z;

		// skip the same
		if (z_end == z_curr)
			continue;

		//UTIL_TraceLine(waypoints[curr_index].origin, waypoints[i].origin, ignore_monsters, pBot->pEdict, &tr);							NEW CODE 094 (prev code)
		UTIL_TraceLine(Vector(x_curr, y_curr, y_curr), Vector(x_end, y_end, z_end), ignore_monsters, pBot->pEdict, &tr);//					NEW CODE 094

		if (tr.flFraction >= 1.0)
		{
			have_it = TRUE;
			break;
		}
	}

	if (have_it)
	{
		pBot->end_wpt_index = i;

		//@@@@@@@@@@
		//char ms[80];
		//sprintf(ms, "endwpt=%d\n", i+1);
		//ALERT(at_console, ms);

		return i;
	}

	//@@@@@@@
	//ALERT(at_console, "PROBLEM NO LADDER END\n");

	return -1;
}

/*
* find the nearest waypoint to the player and return the index
* return -1 if not found
*/
int WaypointFindNearest(edict_t *pEntity, float range, int team)
{
	int i, min_index;
	float distance;
	float min_distance;
	TraceResult tr;

	if (num_waypoints < 1)
		return -1;

	// find the nearest waypoint
	min_index = -1;
	min_distance = 9999.0;

	for (i=0; i < num_waypoints; i++)
	{
		// skip any deleted waypoints
		if (waypoints[i].flags & W_FL_DELETED)
			continue;

		distance = (waypoints[i].origin - pEntity->v.origin).Length();

		if ((distance < min_distance) && (distance < range))
		{
			// if waypoint is visible from current position (even behind head)...
			UTIL_TraceLine( pEntity->v.origin + pEntity->v.view_ofs, waypoints[i].origin,
				ignore_monsters, pEntity->v.pContainingEntity, &tr );

			if (tr.flFraction >= 1.0)
			{
				min_index = i;
				min_distance = distance;
			}
		}
	}

	return min_index;
}

/*
* find the nearest "special" aiming waypoint (for sniper aiming)
* return -1 if not found
*/
int WaypointFindNearestAiming(bot_t *pBot, Vector *v_origin)
{
	int index;
	int min_index = -1;
	float distance;
	float max_range = 100.0;

	if (num_waypoints < 1)
	{
		pBot->f_aim_at_target_time = -1.0;
		return -1;
	}

	// search for nearby aiming waypoint...
	for (index = 0; index < num_waypoints; index++)
	{
		// skip any deleted waypoints
		if (waypoints[index].flags & W_FL_DELETED)
			continue;

		// skip any NON aiming waypoint
		if ((waypoints[index].flags & W_FL_AIMING) == 0)
			continue;

		// skip aim waypoints with NO PRIORITY for bot's team
		if (((waypoints[index].red_priority == 0) && (pBot->bot_team == TEAM_RED)) ||
			((waypoints[index].blue_priority == 0) && (pBot->bot_team == TEAM_BLUE)))
			continue;

		distance = (*v_origin - waypoints[index].origin).Length();

		if (distance <= max_range)
		{
			min_index = index;

			// set it to right index
			if (pBot->aim_index[0] == -1)
				pBot->aim_index[0] = index;
			else if ((pBot->aim_index[0] != -1) && (pBot->aim_index[1] == -1))
				pBot->aim_index[1] = index;
			else if ((pBot->aim_index[0] != -1) && (pBot->aim_index[1] != -1) &&
				(pBot->aim_index[2] == -1))
				pBot->aim_index[2] = index;
			else if ((pBot->aim_index[0] != -1) && (pBot->aim_index[1] != -1) &&
				(pBot->aim_index[2] != -1) && (pBot->aim_index[3] == -1))
				pBot->aim_index[3] = index;
		}
	}

	// there is no aim waypoint in range so set negative target time
	if (min_index == -1)
		pBot->f_aim_at_target_time = -1.0;

	return min_index;
}

/*
* find the nearest cross waypoint to given position/origin
* return -1 if none is found
*/
int WaypointFindNearestCross(const Vector &source_origin)
{
	return WaypointFindNearestCross(source_origin, false);
}

/*
* find the nearest cross waypoint to given position/origin
* return -1 if none is found
* Overloaded to allow ignoring only certain entities (we need to be able to see through doors and breakables)
*/
int WaypointFindNearestCross(const Vector &source_origin, bool see_through_doors)
{
	TraceResult tr;
	float distance, min_distance = 9999.0;
	int min_index = -1;

	// go through all waypoints
	for (int cross = 0; cross < num_waypoints; cross++)
	{
		// skip all deleted waypoint
		if (waypoints[cross].flags & W_FL_DELETED)
			continue;

		// work only with cross waypoints
		if (waypoints[cross].flags & W_FL_CROSS)
		{
			distance = (source_origin - waypoints[cross].origin).Length();

			// is the given origin in this cross waypoint range
			if ((distance < min_distance) && (distance <= waypoints[cross].range))
			{
				if (see_through_doors)
				{
					UTIL_TraceLine(source_origin, waypoints[cross].origin, dont_ignore_monsters, NULL, &tr);

					// is this cross waypoint reachable
					if (tr.flFraction >= 1.0)
					{
						// remember this cross waypoint
						min_distance = distance;
						min_index = cross;
					}
					// we need to check for doors and breakables now, because the waypoint could be on the other side
					else
					{
						if (strcmp(STRING(tr.pHit->v.classname), "func_door") ||
							strcmp(STRING(tr.pHit->v.classname), "func_door_rotating") ||
							strcmp(STRING(tr.pHit->v.classname), "momentary_door") ||
							strcmp(STRING(tr.pHit->v.classname), "func_breakable"))
						{
							// do a second traceline and ignore the previously hit door or breakable entity
							UTIL_TraceLine(source_origin, waypoints[cross].origin, dont_ignore_monsters, tr.pHit, &tr);
							
							if (tr.flFraction >= 1.0)
							{
								// remember this cross waypoint
								min_distance = distance;
								min_index = cross;
							}
						}
					}
				}
				else
				{
					UTIL_TraceLine(source_origin, waypoints[cross].origin, ignore_monsters, NULL, &tr);
					
					if (tr.flFraction >= 1.0)
					{
						// remember this cross waypoint
						min_distance = distance;
						min_index = cross;
					}
				}
			}
		}
	}

	return min_index;
}

/*
* find the nearest available standard waypoint from path end waypoint
* return found waypoint index or -1 if none found
*/
int WaypointFindNearestStandard(int end_waypoint, int path_index)
{
	// check validity first
	if (end_waypoint == -1 || path_index == -1)
		return -1;

	int found_waypoint = -1;
	float min_distance = MAX_WPT_DIST / 2;

	// go through all waypoints
	for (int i = 0; i < num_waypoints; i++)
	{
		// skip these
		if (waypoints[i].flags & (W_FL_DELETED | W_FL_AIMING | W_FL_CROSS))
			continue;
		
		// skip all waypoints in this path
		if (WaypointIsInPath(i, path_index))
			continue;
		
		// see how far is this waypoint from our waypoint ...
		float distance = (waypoints[i].origin - waypoints[end_waypoint].origin).Length();
		
		// because we are looking only for close waypoints
		if (distance < min_distance)
		{
			int num_paths_on_this_waypoint = 0;
			
			// check if we can use this waypoint so go through all paths ... 
			for (int this_path = 0; this_path < num_w_paths; this_path++)
			{
				// to see if this waypoint is in any path if so then ...
				if (WaypointIsInPath(i, this_path))
				{
					num_paths_on_this_waypoint++;
					
					// see if this path is accessible
					// without exact data about bot team and class we can only do general comparison ...
					// that means we can only access path that is equally or less restrictive than our current one
					// (eg. from red team path to another red team or to default both team path)
					// therefore we have to assume our current path is the more restrictive path
					if (PathCompareTeamDownwards(path_index, this_path) && PathCompareClassDownwards(path_index, this_path))
					{
						// this waypoint is the end waypoint of this path ... we cannot access it then
						if (IsPath(this_path, PathT::path_one) && i == GetPathEnd(this_path))
							continue;
						else
						{
							TraceResult tr;
							UTIL_TraceLine(waypoints[i].origin, waypoints[end_waypoint].origin, ignore_monsters, NULL, &tr);

							// check if this waypoint is reachable
							if (tr.flFraction >= 1.0)
							{
								min_distance = distance;
								found_waypoint = i;
							}
						}
					}
				}
			}
			
			// if there is no path on this waypoint then we may use it too ...
			if (num_paths_on_this_waypoint < 1)
			{
				// ... unless it's a lone standing goback waypoint
				// (this probably happened when the waypointer tried to change the path end waypoint into a goback,
				// but used command to add a new waypoint instead of using the change waypoint command)
				if (!IsWaypoint(i, WptT::wpt_goback))
				{
					TraceResult tr;
					UTIL_TraceLine(waypoints[i].origin, waypoints[end_waypoint].origin, ignore_monsters, NULL, &tr);
					
					// if it is reachable of course
					if (tr.flFraction >= 1.0)
					{
						min_distance = distance;
						found_waypoint = i;
					}
				}
			}
		}
	}

	return found_waypoint;
}

/*
* find any "special" aiming waypoint (for sniper aiming) around given waypoint
* return found waypoint index or -1 if none found
*/
int WaypointFindAimingAround(int source_waypoint)
{
	// check validity first
	if ((source_waypoint == -1) || (waypoints[source_waypoint].flags & W_FL_DELETED))
		return -1;

	float aim_waypoint_range = 100.0;

	// go through all waypoints
	for (int index = 0; index < num_waypoints; index++)
	{
		// skip any deleted waypoints
		if (waypoints[index].flags & W_FL_DELETED)
			continue;

		// skip any NON aiming waypoint
		if ((waypoints[index].flags & W_FL_AIMING) == 0)
			continue;

		float distance = (waypoints[source_waypoint].origin - waypoints[index].origin).Length();

		// if this aiming waypoint is nearby then return its index
		if (distance <= aim_waypoint_range)
		{
			return index;
		}
	}

	return -1;
}

/*
* search for new waypoint to head towards when the bot reached the end of his current path
* first look for a cross waypoint if none is found look for standard waypoint
* return found waypoint index
* return -1 if nothing was found
*/
int WaypointSearchNewInRange(bot_t *pBot, int wpt_index, int curr_path)
{
	TraceResult tr;
	float distance, min_distance;
	int min_index;

	// check for problem stuff
	if (wpt_index == -1)
		return -1;

	// first we try to find a cross waypoint connected to given waypoint
	min_index = WaypointFindNearestCross(waypoints[wpt_index].origin);

	// if we found any return it
	if (min_index != -1)
		return min_index;

	// we need to init these
	min_distance = 9999.0;
	min_index = -1;

	// go through all waypoints
	for (int index = 0; index < num_waypoints; index++)
	{
		// skip all deleted waypoints
		if (waypoints[index].flags & W_FL_DELETED)
			continue;

		// skip all aim waypoints
		if (waypoints[index].flags & W_FL_AIMING)
			continue;

		// skip all waypoints in current path
		if (WaypointIsInPath(index, curr_path))
			continue;

		distance = (waypoints[wpt_index].origin - waypoints[index].origin).Length();

		// is this waypoint in current waypoint range OR in half of the maximum reachable range
		// this prevents checking high amount of waypoints --> lower CPU usage
		if ((distance <= waypoints[wpt_index].range) ||	(distance < (MAX_WPT_DIST / 2)))
		{
			// is this waypoint acceptable
			// ie is there at least one path that could be used by this bot
			if (IsWaypointPossible(pBot, index))
			{
				// is this waypoint closer than previous "in range" waypoint
				if (distance < min_distance)
				{
					UTIL_TraceLine(waypoints[wpt_index].origin, waypoints[index].origin, ignore_monsters, NULL, &tr);

					// check if this waypoint is reachable
					if (tr.flFraction >= 1.0)
					{
						// we will return the nearest waypoint
						min_distance = distance;
						min_index = index;
					}
				}
			}
		}
	}

	// if we found any waypoint return its index
	if (min_index != -1)
		return min_index;

	return -1;
}

/*
* sets last PATROL path waypoint as bots new waypoint
* sets it and returns TRUE if that waypoint is reachable
* returns FALSE if not
*/
bool WaypointFindLastPatrol(bot_t *pBot)
{
	edict_t *pEdict;
	TraceResult tr;

	// check for problem stuff
	if ((num_waypoints < 1) || (pBot->patrol_path_wpt == -1))
		return FALSE;

	pEdict = pBot->pEdict;

	// get bots distance from that wpt
	float distance = (pEdict->v.origin - waypoints[pBot->patrol_path_wpt].origin).Length();

	// is bot still in reachable range
	if (distance <= MAX_WPT_DIST)
	{
		// if waypoint is visible from source position
		UTIL_TraceLine(pEdict->v.origin, waypoints[pBot->patrol_path_wpt].origin, ignore_monsters,
					pEdict->v.pContainingEntity, &tr );

		if (tr.flFraction >= 1.0)
		{
			// set it as current wpt to head to
			pBot->curr_wpt_index = pBot->patrol_path_wpt;
			pBot->wpt_origin = waypoints[pBot->patrol_path_wpt].origin;
			pBot->f_waypoint_time = gpGlobals->time;

			return TRUE;
		}
	}

	return FALSE;
}

void DrawTheBeam(edict_t *pEntity, Vector start, Vector end, int sprite, int width,
			  int noise, int red, int green, int blue, int brightness, int speed)
{
	// just for sure
	if ((pEntity == NULL) || (UTIL_GetBotIndex(pEntity) > -1))
		return;

	MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, NULL, pEntity);
	WRITE_BYTE( TE_BEAMPOINTS);
	WRITE_COORD(start.x);
	WRITE_COORD(start.y);
	WRITE_COORD(start.z);
	WRITE_COORD(end.x);
	WRITE_COORD(end.y);
	WRITE_COORD(end.z);
	WRITE_SHORT( sprite );	// a texture the bream uses
	WRITE_BYTE( 1 ); // framestart
	WRITE_BYTE( 10 ); // framerate
	WRITE_BYTE( 10 ); // life in 0.1's
	WRITE_BYTE( width ); // width
	WRITE_BYTE( noise );  // noise

	WRITE_BYTE( red );   // r, g, b
	WRITE_BYTE( green );   // r, g, b
	WRITE_BYTE( blue );   // r, g, b

	WRITE_BYTE( brightness );   // brightness
	WRITE_BYTE( speed );    // speed
	MESSAGE_END();
}

// allows to specify duration of the beam
void DrawBeam(edict_t *pEntity, Vector start, Vector end, int life, int red, int green, int blue, int speed)
{
	if ((pEntity == NULL) || (UTIL_GetBotIndex(pEntity) > -1))
		return;

	MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, NULL, pEntity);
	WRITE_BYTE(TE_BEAMPOINTS);
	WRITE_COORD(start.x);
	WRITE_COORD(start.y);
	WRITE_COORD(start.z);
	WRITE_COORD(end.x);
	WRITE_COORD(end.y);
	WRITE_COORD(end.z);
	WRITE_SHORT(m_spriteTexture);
	WRITE_BYTE(1); // framestart
	WRITE_BYTE(10); // framerate
	WRITE_BYTE(life); // life in 0.1's
	WRITE_BYTE(10); // width
	WRITE_BYTE(0);  // noise

	WRITE_BYTE(red);   // r, g, b
	WRITE_BYTE(green);   // r, g, b
	WRITE_BYTE(blue);   // r, g, b

	WRITE_BYTE(250);   // brightness
	WRITE_BYTE(speed);    // speed
	MESSAGE_END();
}

void WaypointBeam(edict_t *pEntity, Vector start, Vector end, int width,
				  int noise, int red, int green, int blue, int brightness, int speed)
{
	if (pEntity == NULL)
		return;

	DrawTheBeam(pEntity, start, end, m_spriteTexture, width,
				noise, red, green, blue, brightness, speed);
}

void PathBeam(edict_t *pEntity, Vector start, Vector end, int sprite, int width,
			  int noise, int red, int green, int blue, int brightness, int speed)
{
	if (pEntity == NULL)
		return;

	DrawTheBeam(pEntity, start, end, sprite, width,
				noise, red, green, blue, brightness, speed);
}

/*
* returns the number of flags set on this waypoint
* doesn't count additional/"secondary" flags like sniper, shoot etc.
*/
int Wpt_CountFlags(int wpt_index)
{
	int the_count = 0;

	// handle errors
	//if ((wpt_index == -1) || (waypoints[wpt_index].flags & W_FL_DELETED))
	if (wpt_index == -1)
		return -1;

	// increase the count with each valid flag
	if (waypoints[wpt_index].flags & W_FL_STD)
		the_count++;
	if (waypoints[wpt_index].flags & W_FL_CROUCH)
		the_count++;
	if (waypoints[wpt_index].flags & W_FL_PRONE)
		the_count++;
	if (waypoints[wpt_index].flags & W_FL_JUMP)
		the_count++;
	if (waypoints[wpt_index].flags & W_FL_DUCKJUMP)
		the_count++;
	if (waypoints[wpt_index].flags & W_FL_SPRINT)
		the_count++;
	if (waypoints[wpt_index].flags & W_FL_AMMOBOX)
		the_count++;
	if (waypoints[wpt_index].flags & W_FL_DOOR)
		the_count++;
	if (waypoints[wpt_index].flags & W_FL_DOORUSE)
		the_count++;
	if (waypoints[wpt_index].flags & W_FL_LADDER)
		the_count++;
	if (waypoints[wpt_index].flags & W_FL_USE)
		the_count++;
	if (waypoints[wpt_index].flags & W_FL_CHUTE)
		the_count++;
	if (waypoints[wpt_index].flags & W_FL_MINE)
		the_count++;
	if (waypoints[wpt_index].flags & W_FL_PUSHPOINT)
		the_count++;
	if (waypoints[wpt_index].flags & W_FL_TRIGGER)
		the_count++;
	if (waypoints[wpt_index].flags & W_FL_GOBACK)
		the_count++;
	if (waypoints[wpt_index].flags & W_FL_DELETED)
		the_count++;
	// keep these special flags here so we can find possible problems
	if (waypoints[wpt_index].flags & W_FL_AIMING)
		the_count++;
	if (waypoints[wpt_index].flags & W_FL_CROSS)
		the_count++;

	return the_count;
}


/*
* returns TRUE if the flag is set on this waypoint
*/
bool IsWaypoint(int wpt_index, WptT flag_type)
{
	if ((wpt_index == -1) || (flag_type <= WptT::scrapped_flagtype))
		return FALSE;

	int flag = 0;
	//bool flag_level2 = FALSE;

	// convert the flag_type to a real waypoint flag
	// the order is set by how much are particular types used
	if (flag_type == WptT::wpt_normal)
		flag = W_FL_STD;
	else if (flag_type == WptT::wpt_crouch)
		flag = W_FL_CROUCH;
	else if (flag_type == WptT::wpt_prone)
		flag = W_FL_PRONE;
	else if (flag_type == WptT::wpt_cross)
		flag = W_FL_CROSS;
	else if (flag_type == WptT::wpt_aim)
		flag = W_FL_AIMING;
	else if (flag_type == WptT::wpt_no)
		flag = W_FL_DELETED;
	else if (flag_type == WptT::wpt_goback)
		flag = W_FL_GOBACK;
	else if (flag_type == WptT::wpt_sniper)
		flag = W_FL_SNIPER;
	else if (flag_type == WptT::wpt_jump)
		flag = W_FL_JUMP;
	else if (flag_type == WptT::wpt_duckjump)
		flag = W_FL_DUCKJUMP;
	else if (flag_type == WptT::wpt_ammobox)
		flag = W_FL_AMMOBOX;
	else if (flag_type == WptT::wpt_ladder)
		flag = W_FL_LADDER;
	else if (flag_type == WptT::wpt_sprint)
		flag = W_FL_SPRINT;
	else if (flag_type == WptT::wpt_fire)
		flag = W_FL_FIRE;
	else if (flag_type == WptT::wpt_mine)
		flag = W_FL_MINE;
	else if (flag_type == WptT::wpt_chute)
		flag = W_FL_CHUTE;
	else if (flag_type == WptT::wpt_pushpoint)
		flag = W_FL_PUSHPOINT;
	else if (flag_type == WptT::wpt_trigger)
		flag = W_FL_TRIGGER;
	else if (flag_type == WptT::wpt_cover)
		flag = W_FL_COVER;
	else if (flag_type == WptT::wpt_bandages)
		flag = W_FL_BANDAGES;
	else if (flag_type == WptT::wpt_use)
		flag = W_FL_USE;
	else if (flag_type == WptT::wpt_door)
		flag = W_FL_DOOR;
	else if (flag_type == WptT::wpt_dooruse)
		flag = W_FL_DOORUSE;
	
	// is the flag set on this waypoint?
	if (waypoints[wpt_index].flags & flag)
		return TRUE;

	return FALSE;
}


/*
* add new waypoint to actual player location and specify all waypoint values (priority, time etc.)
* return the type (normal, aim etc.) of that waypoint
*/
char *WptAdd(edict_t *pEntity, const char *wpt_type)
{
	int index;
	Vector start, end;
	
	if (num_waypoints >= MAX_WAYPOINTS)
		return "too many wpts";

	index = 0;

	// find the next available slot for the new waypoint...
	while (index < num_waypoints)
	{
		if (waypoints[index].flags & W_FL_DELETED)
			break;

		index++;
	}

	// don't allow placing waypoints too close
	if (WaypointFindNearest(pEntity, 20.0, -1) != -1)
	{
		return "already_is";
	}

	// no type specified? so use default type
	if ((wpt_type == NULL) || (*wpt_type == 0))
	{
		wpt_type = "normal";		
	}

	// reset this waypoint slot before using it
	WaypointInit(index);

	waypoints[index].flags = 0;		// no flag yet, must be to clear the deleted flag

	// set standard flag used as a default
	waypoints[index].flags |= W_FL_STD;

	// now add all other flags based on the waypoint type
	if (FStrEq(wpt_type, "normal"))
	{
		// we don't need to add anything else, the flag is already set
		//																						NEW CODE 094 END
		// so we only check if autowaypointing is turned on and ...
		if (wptser.IsAutoWaypointing())
		{
			edict_t* pent = NULL;
			// search the surrounding for ammobox entity
			while ((pent = UTIL_FindEntityInSphere(pent, pEntity->v.origin, PLAYER_SEARCH_RADIUS)) != NULL)
			{
				// if we found it then change the waypoint type to ammobox waypoint
				if (strcmp(STRING(pent->v.classname), "ammobox") == 0)
				{
					waypoints[index].flags |= W_FL_AMMOBOX;	// add another flag that makes this type
					waypoints[index].range = (float)20;
				}
			}
		}
		//************************************************************************************	NEW CODE 094 END
	}
	else if (FStrEq(wpt_type, "ammobox"))
	{
		waypoints[index].flags |= W_FL_AMMOBOX;
		waypoints[index].range = (float) 20;
	}
	else if (FStrEq(wpt_type, "parachute"))
	{
		waypoints[index].flags |= W_FL_CHUTE;
	}
	else if (FStrEq(wpt_type, "crouch"))
	{
		waypoints[index].flags |= W_FL_CROUCH;
	}
	else if (FStrEq(wpt_type, "door"))
	{
		waypoints[index].flags |= W_FL_DOOR;
		waypoints[index].range = (float) 20;
	}
	else if (FStrEq(wpt_type, "usedoor"))
	{
		waypoints[index].flags |= W_FL_DOORUSE;
		waypoints[index].range = (float) 20;
	}
	else if (FStrEq(wpt_type, "shoot"))
	{
		waypoints[index].flags |= W_FL_FIRE;
	}
	else if (FStrEq(wpt_type, "goback"))
	{
		waypoints[index].flags |= W_FL_GOBACK;
	}
	else if (FStrEq(wpt_type, "jump"))
	{
		waypoints[index].flags |= W_FL_JUMP;
		waypoints[index].range = (float) 20;
	}
	else if (FStrEq(wpt_type, "duckjump"))
	{
		waypoints[index].flags |= W_FL_DUCKJUMP;
		waypoints[index].range = (float) 20;
	}
	else if (FStrEq(wpt_type, "ladder"))
	{
		waypoints[index].flags |= W_FL_LADDER;
		waypoints[index].range = (float) 20;
	}
	else if (FStrEq(wpt_type, "claymore"))
	{
		waypoints[index].flags |= W_FL_MINE;
	}
	else if (FStrEq(wpt_type, "prone"))
	{
		waypoints[index].flags |= W_FL_PRONE;
	}
	else if ((FStrEq(wpt_type, "flag")) || (FStrEq(wpt_type, "pushpoint")))
	{
		waypoints[index].flags |= W_FL_PUSHPOINT;
		waypoints[index].range = (float) 20;
	}
	else if (FStrEq(wpt_type, "sprint"))
	{
		waypoints[index].flags |= W_FL_SPRINT;
	}
	else if (FStrEq(wpt_type, "sniper"))
	{
		waypoints[index].flags |= W_FL_SNIPER;
	}
	else if (FStrEq(wpt_type, "trigger"))
	{
		waypoints[index].flags |= W_FL_TRIGGER;
	}
	else if (FStrEq(wpt_type, "use"))
	{
		waypoints[index].flags |= W_FL_USE;
		waypoints[index].range = (float) 20;
	}
	else if (FStrEq(wpt_type, "aim"))
	{
		waypoints[index].flags = 0;				// we need to clear the STD flag
		waypoints[index].flags |= W_FL_AIMING;
		waypoints[index].range = (float) 0;
	}
	else if (FStrEq(wpt_type, "cross"))
	{
		waypoints[index].flags = 0;
		waypoints[index].flags |= W_FL_CROSS;
		waypoints[index].range = WPT_RANGE * (float) 3;

		//																						NEW CODE 094 END
		// stop autowaypointing when a cross waypoint is added manually
		if (wptser.IsAutoWaypointing())
		{
			StartAutoWaypointg(FALSE);

			// also change its range to match the autowaypointing distance
			// in order to reach (be able to connect to) normal waypoint/s added by the autowaypointing
			waypoints[index].range = wptser.GetAutoWaypointingDistance() + (float) 10;
		}
		//************************************************************************************	NEW CODE 094 END
	}
	else
	{
		// reset the flag back to default ie to deleted waypoint
		waypoints[index].flags = W_FL_DELETED;

		return "unknown";
	}

	// get size/hight of the beam based on waypoint type
	SetWaypointSize(index, start, end);

	// use temp vector to set color for this waypoint
	Vector color = SetWaypointColor(index);

	// draw the waypoint
	WaypointBeam(pEntity, start, end, 30, 0, color.x, color.y, color.z, 250, 5);

	// store the origin (location) of this waypoint (use entity origin)
	waypoints[index].origin = pEntity->v.origin;

	// store the last used waypoint for the auto waypoint code...
	//last_waypoint = pEntity->v.origin;																			NEW CODE 094 (prev code)
	// unless it is a cross or aiming waypoint
	if (!(waypoints[index].flags & (W_FL_CROSS | W_FL_AIMING)))//													NEW CODE 094
		last_waypoint = pEntity->v.origin;

	// set the time that this waypoint was originally displayed...
	wp_display_time[index] = 0.0;

	// set the time for possible range highlighting
	f_ranges_display_time[index] = 0.0;

	// set the time for possible in range status of any cross or aim wpt (to draw connection)
	f_conn_time[index][index] = 0.0;

	EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "weapons/xbow_hit1.wav", 1.0, ATTN_NORM, 0, 100);

	// increment total number of waypoints if adding at end of array...
	if (index == num_waypoints)
		num_waypoints++;

	// and start displaying the waypoints
	wptser.SetShowWaypoints(true);//																				NEW CODE 094

	return (char *) wpt_type;
}

/*
*	THIS ISN'T USED YET (except for one case)
* add new waypoint to given location and specify all waypoint values (priority, time etc.)
* return the type (normal, aim etc.) of that waypoint
*/
WptT WaypointAdd(const Vector position, WptT wpt_type)
{
	int index;
	//Vector start, end;
	
	if (num_waypoints >= MAX_WAYPOINTS)
		return WptT::wpt_no;

	index = 0;

	// find the next available slot for the new waypoint...
	while (index < num_waypoints)
	{
		if (waypoints[index].flags & W_FL_DELETED)
			break;

		index++;
	}

	// don't allow placing waypoints too close
	/*if (WaypointFindNearest(pEntity, 20.0, -1) != -1)
	{
		return wpt_no;
	}
	*/

	// no type specified? so use default type
	//if ((wpt_type == wpt_no) || (wpt_type < 0))
	if ((wpt_type == WptT::wpt_no) || (wpt_type <= WptT::scrapped_flagtype))
	{
		wpt_type = WptT::wpt_normal;
	}

	// reset this waypoint slot before using it
	WaypointInit(index);

	waypoints[index].flags = 0;		// no flag yet, must be to clear the deleted flag

	// set standard flag used as a default
	waypoints[index].flags |= W_FL_STD;

	// now add all other flags based on the waypoint type
	if (wpt_type == WptT::wpt_normal)
	{
		// we don't need to add anything else, the flag is already set
		//																						NEW CODE 094 END
		// so we only check if autowaypointing is turned on and ...
		if (wptser.IsAutoWaypointing())
		{
			edict_t* pent = NULL;
			// search the surrounding for ammobox entity
			while ((pent = UTIL_FindEntityInSphere(pent, position, PLAYER_SEARCH_RADIUS)) != NULL)
			{
				// if we found it then change the waypoint type to ammobox waypoint
				if (strcmp(STRING(pent->v.classname), "ammobox") == 0)
				{
					waypoints[index].flags |= W_FL_AMMOBOX;	// add another flag that makes this type
					waypoints[index].range = (float)20;
				}
			}
		}
		//************************************************************************************	NEW CODE 094 END
	}
	else if (wpt_type == WptT::wpt_ammobox)
	{
		waypoints[index].flags |= W_FL_AMMOBOX;	// add another flag that makes this type
		waypoints[index].range = (float) 20;
	}
	else if (wpt_type == WptT::wpt_chute)
	{
		waypoints[index].flags |= W_FL_CHUTE;
	}
	else if (wpt_type == WptT::wpt_crouch)
	{
		waypoints[index].flags |= W_FL_CROUCH;
	}
	else if (wpt_type == WptT::wpt_door)
	{
		waypoints[index].flags |= W_FL_DOOR;
		waypoints[index].range = (float) 20;
	}
	else if (wpt_type == WptT::wpt_dooruse)
	{
		waypoints[index].flags |= W_FL_DOORUSE;
		waypoints[index].range = (float) 20;
	}
	else if (wpt_type == WptT::wpt_fire)
	{
		waypoints[index].flags |= W_FL_FIRE;
	}
	else if (wpt_type == WptT::wpt_goback)
	{
		waypoints[index].flags |= W_FL_GOBACK;
	}
	else if (wpt_type == WptT::wpt_jump)
	{
		waypoints[index].flags |= W_FL_JUMP;
		waypoints[index].range = (float) 20;
	}
	else if (wpt_type == WptT::wpt_duckjump)
	{
		waypoints[index].flags |= W_FL_DUCKJUMP;
		waypoints[index].range = (float) 20;
	}
	else if (wpt_type == WptT::wpt_ladder)
	{
		waypoints[index].flags |= W_FL_LADDER;
		waypoints[index].range = (float) 20;
	}
	else if (wpt_type == WptT::wpt_mine)
	{
		waypoints[index].flags |= W_FL_MINE;
	}
	else if (wpt_type == WptT::wpt_prone)
	{
		waypoints[index].flags |= W_FL_PRONE;
	}
	else if (wpt_type == WptT::wpt_pushpoint)
	{
		waypoints[index].flags |= W_FL_PUSHPOINT;
		waypoints[index].range = (float) 20;
	}
	else if (wpt_type == WptT::wpt_sprint)
	{
		waypoints[index].flags |= W_FL_SPRINT;
	}
	else if (wpt_type == WptT::wpt_sniper)
	{
		waypoints[index].flags |= W_FL_SNIPER;
	}
	else if (wpt_type == WptT::wpt_trigger)
	{
		waypoints[index].flags |= W_FL_TRIGGER;
	}
	else if (wpt_type == WptT::wpt_use)
	{
		waypoints[index].flags |= W_FL_USE;
		waypoints[index].range = (float) 20;
	}
	else if (wpt_type == WptT::wpt_aim)
	{
		waypoints[index].flags = 0;				// we need to clear the STD flag
		waypoints[index].flags |= W_FL_AIMING;
		waypoints[index].range = (float) 0;
	}
	else if (wpt_type == WptT::wpt_cross)
	{
		waypoints[index].flags = 0;
		waypoints[index].flags |= W_FL_CROSS;
		waypoints[index].range = WPT_RANGE * (float) 3;

		//																						NEW CODE 094 END
		// stop autowaypointing when a cross waypoint is added manually
		if (wptser.IsAutoWaypointing())
		{
			StartAutoWaypointg(FALSE);

			// also change its range to match the autowaypointing distance
			// in order to reach (be able to connect to) normal waypoint/s added by the autowaypointing
			waypoints[index].range = wptser.GetAutoWaypointingDistance() + (float) 10;
		}
		//************************************************************************************	NEW CODE 094 END
	}
	else
	{
		// reset the flag back to default ie to deleted waypoint
		waypoints[index].flags = W_FL_DELETED;

		return WptT::wpt_no;
	}

	// get size/hight of the beam based on waypoint type
	//SetWaypointSize(index, start, end);

	// use temp vector to set color for this waypoint
	//Vector color = SetWaypointColor(index);

	// draw the waypoint
	//WaypointBeam(pEntity, start, end, 30, 0, color.x, color.y, color.z, 250, 5);

	// store the origin (location) of this waypoint (use entity origin)
	waypoints[index].origin = position;

	// store the last used waypoint for the auto waypoint code...
	if ((wpt_type != WptT::wpt_aim) && (wpt_type != WptT::wpt_cross))//													NEW CODE 094
		last_waypoint = position;

	wp_display_time[index] = 0.0;
	f_ranges_display_time[index] = 0.0;
	f_conn_time[index][index] = 0.0;

	// increment total number of waypoints if adding at end of array...
	if (index == num_waypoints)
		num_waypoints++;

	return wpt_type;
}

/*
* change origin of passed waypoint
* for the new origin is used current player position
*/
bool WaypointReposition(edict_t *pEntity, int wpt_index)
{
	// try to search for some close waypoints
	if (wpt_index == -1)
		wpt_index = WaypointFindNearest(pEntity, 50.0, -1);
	
	// is it valid waypoint?
	if ((wpt_index != -1) && (wpt_index < num_waypoints) &&
		(!(waypoints[wpt_index].flags & W_FL_DELETED)))
	{
		// use the current player's origin/position
		waypoints[wpt_index].origin = pEntity->v.origin;

		// reset draw times so the waypoint is redrawn immediately
		wp_display_time[wpt_index] = 0.0;
		f_ranges_display_time[wpt_index] = 0.0;

		return TRUE;
	}

	return FALSE;
}

/*
* delete nearest waypoint to actual player position
* remove it also from any path the waypoint is in
* clear/free this waypoint slot in global waypoint array
*/
void WaypointDelete(edict_t *pEntity)
{
	int index;
	int count = 0;

	if (num_waypoints < 1)
		return;

	index = WaypointFindNearest(pEntity, 50.0, -1);

	if (index == -1)
		return;

	// check if there are any paths
	if (num_w_paths >= 1)
	{
		// go through all paths
		for (int path_index = 0; path_index < num_w_paths; path_index++)
		{
			// remove this waypoint from any path this waypoint was added to
			ExcludeFromPath(index, path_index);
		}
	}

	//																								NEW CODE 094
	// if we are deleting the last added waypoint
	// then we also need to reset the position for autowaypointing
	// otherwise we won't be able to start autowaypointing nearby
	if (waypoints[index].origin == last_waypoint)
		last_waypoint = g_vecZero;//Vector(0, 0, 0);

	//******************************************************************************************	NEW CODE 094 END

	// call the init method which will reset the waypoint
	WaypointInit(index);

	wp_display_time[index] = 0.0;
	f_ranges_display_time[index] = 0.0;

	// clear the connections "sub array" too
	for (int j = 0; j < MAX_WAYPOINTS; j++)
		f_conn_time[index][j] = 0.0;

	// deactivate compass if this is the waypoint the user was looking for
	if (wptser.GetCompassIndex() == index)
		wptser.ResetCompassIndex();

	EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "weapons/mine_activate.wav", 1.0, ATTN_NORM, 0, 100);
}

/*
* starts new path from the nearest waypoint to player position
* returns TRUE if path was successfully started
* return FALSE if not (no waypoint close or path can't be created)
*/
bool WaypointCreatePath(edict_t *pEntity)
{
	static int current_waypoint = -1;  // initialized to unassigned

	current_waypoint = WaypointFindNearest(pEntity, 50.0, -1);

	if (current_waypoint == -1)
	{
		// here shoud be detailed print for some sort of debugging

		return FALSE;
	}

	if (StartNewPath(current_waypoint))
	{
		return TRUE;
	}

	return FALSE;
}

/*
* stops pointing on current path (clear "pointers" if needed)
* returns TRUE if any path was finished
*/
bool WaypointFinishPath(edict_t *pEntity)
{
	// is any path currenly in edit
	if (internals.IsPathToContinue())
	{
		int path_validity;

		//path_validity = WaypointIsPathOkay(g_path_to_continue);					// NEW CODE 094 (prev code)
		
		path_validity = WaypointValidatePath(internals.GetPathToContinue());//					NEW CODE 094
		
		
		if (path_validity == 1)
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "there was an error in this path that has been automatically fixed\n");
		else if (path_validity == -1)
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "there is an error in this path that couldn't be fixed automatically\n");

		// also update this path status
		UpdatePathStatus(internals.GetPathToContinue());

		// we aren't going to edit this path anymore so we must reset the "pointer"
		internals.ResetPathToContinue();

		return TRUE;
	}

	return FALSE;
}

/*
* continue in path specified in path_index
* if no path_index is specified do search for the nearest waypoint to check if there is any path
* (the first path that was found)
*/
bool WaypointPathContinue(edict_t *pEntity, int path_index)
{
	// if path isn't specified find first one
	if (path_index == -1)
	{
		int current_waypoint = -1;

		current_waypoint = WaypointFindNearest(pEntity, 50.0, -1);

		if (current_waypoint == -1)
			return FALSE;

		// find the first path on this wpt 
		path_index = FindPath(current_waypoint);
	}

	// is the path valid
	if ((path_index != -1) && (w_paths[path_index] != NULL))
	{
		// update global "pointer"
		internals.SetPathToContinue(path_index);

		return TRUE;
	}

	return FALSE;
}

/*
* adds the nearest waypoint to currently edited path
*/
bool WaypointAddIntoPath(edict_t *pEntity)
{
	int current_waypoint = -1;
	int result;

	// if automatic additions to path is active then the waypoint must be really close
	if (wptser.IsAutoAddToPath())
		current_waypoint = WaypointFindNearest(pEntity, AUTOADD_DISTANCE, -1);
	// otherwise use standard distance
	else
		current_waypoint = WaypointFindNearest(pEntity, 50.0, -1);

	if (current_waypoint == -1)
	{
		// here shoud be detailed print for some sort of debugging

		return FALSE;
	}

	result = ContinueCurrPath(current_waypoint);

	//if (result > 0)																				NEW CODE 094 (prev code)
	if (result == 1)//																				NEW CODE 094
	{
		return TRUE;
	}

	// here shoud be few else with detailed print for some sort of debugging

	return FALSE;
}

/*
* remove the nearest waypoint from specified or currently edited path
*/
bool WaypointRemoveFromPath(edict_t *pEntity, int path_index)
{
	int current_waypoint = -1;
	int result;

	current_waypoint = WaypointFindNearest(pEntity, 50.0, -1);

	if (current_waypoint == -1)
	{
		// here shoud be detailed print for some sort of debugging

		return FALSE;
	}

	// if not specified path use actual (currently in edit) path
	if (path_index == -1)
		path_index = internals.GetPathToContinue();

	// if still no path then try to find any path on nearby wpt
	if (path_index == -1)
		path_index = FindPath(current_waypoint);

	result = ExcludeFromPath(current_waypoint, path_index);

	if (result > 0)
	{
		return TRUE;
	}
	else if (result == 0)
	{
#ifdef NDEBUG
			UTIL_DebugInFile("***E_F_P|UNK\n");
#endif
#ifdef _DEBUG
			UTIL_DebugDev("ExcludeFromPath()->unk\n", current_waypoint, path_index);
			ALERT(at_console, "unknown error in ExcludeFromPath()\n");
#endif
		return FALSE;
	}

	return FALSE;
}

/*
* split path into two parts, either by given path index as arg2 or the first found path on given waypoint as arg3 or on nearby waypoint if arg3 is NULL
* returns the index of the new path if we successfully split original path
* returns -1 if not close to any wpt or there's no path on it
* returns -2 if the path is too short (ie. path has less than 4 wpts -> new paths after the split must have at least 2 wpts each)
* returns -3 if the waypoint isn't in this path
* returns -4 if we can't split the path at this position (ie. the waypoint is its starting point or ending point or is at the penultimate position in the path)
* returns -5 if we are unable to start a new path (most probably reached the max. amount of paths)
* returns -6 if something went wrong when shuffling the waypoints between the paths or if either of the path offsprings didn't pass validation
* returns -7 if the path doesn't exist or was deleted
* returns -8 if the waypoint doesn't exist or was deleted
*/
int WaypointSplitPath(edict_t *pEntity, const char *arg2, const char *arg3)
{
	int path_index = -1;
	int wpt_index = -1;
	int path_length = 0;
	int min_length = 5;		// min length of the path to allow the split
	w_path *p = NULL;

	// get path index from argument
	if ((arg2 != NULL) && (*arg2 != 0))
	{
		path_index = atoi(arg2);
		path_index--;					// due to array based style

		// return error if such index doesn't exist or it's a deleted path
		if ((path_index == -1) || (w_paths[path_index] == NULL))
			return -7;

		path_length = WaypointGetPathLength(path_index);

		// return error if the path is too short to create two valid path "offsprings" after we divide it
		if ((path_length > 0) && (path_length < min_length))
			return -2;
	}

	// get waypoint index from argument
	if ((arg3 != NULL) && (*arg3 != 0))
	{
		wpt_index = atoi(arg3);
		wpt_index--;

		// return error if such waypoint doesn't exist or was deleted
		if ((wpt_index < 0) || (wpt_index > num_waypoints) || (waypoints[wpt_index].flags & W_FL_DELETED))
			return -8;
	}

	// try to find nearby waypoint if there was no argument
	if (wpt_index == -1)
	{
		wpt_index = WaypointFindNearest(pEntity, 50.0, -1);

		// there's no waypoint where the split could be done so return error
		if (wpt_index == -1)
			return -1;
	}

	// if there was no path given as argument then ...
	if (path_index == -1)
	{
		// find the first path on waypoint
		path_index = FindPath(wpt_index);

		if (path_index == -1)
			return -1;

		path_length = WaypointGetPathLength(path_index);

		if ((path_length > 0) && (path_length < min_length))
			return -2;
	}

	// find the position of the waypoint in the path
	p = GetWaypointPointer(wpt_index, path_index);

	// return error if the waypoint isn't present in our path
	if (p == NULL)
		return -3;

	// if there is no previous node then this waypoint must be at the beginning of the path so we can't split it here
	if (p->prev == NULL)
		return -4;

	// if there aren't at least two nodes preceding this one then we can't split the path
	// in other words this must be at least 3rd waypoint from the beginning of the path to allow the split
	// this is due to the fact that if the splitting is done automatically as a result of waypoint change
	// to a cross waypoint then such waypoint will be excluded from the path ...
	// therefore there must be at least three waypoints left in the first part of path
	if (p->prev->prev == NULL)
		return -4;

	// if there is no next node then the waypoint is at the end of the path so we can't split the path here either
	if (p->next == NULL)
		return -4;

	// if there aren't at least two nodes after this then we can't split the path at this position
	// in other words there must be at least two waypoints left in the path
	// so the part we'll cut off will be a valid path (ie. a path with at least two waypoints)
	if (p->next->next == NULL)
		return -4;

	// set the pointer to next node
	// (ie. next waypoint in the path, because that is the waypoint which will be the starting point of the new path)
	p = p->next;

	// return error if we can't start new path
	if (StartNewPath(p->wpt_index) == FALSE)
		return -5;

	// back up the index of the new path
	int other_path = internals.GetPathToContinue();

	// copy all path data from current path to the new one
	w_paths[other_path]->flags = w_paths[path_index]->flags;

	// and remove the waypoint from current path
	if (ExcludeFromPath(p, path_index) != 1)
		return -6;

	// the pointer was freed so we must find the position of the waypoint in the path again
	p = GetWaypointPointer(wpt_index, path_index);

	// so that we can again set the pointer to next node and proceed the rest of the path
	p = p->next;

	// now go through the rest of the path and move it to the other path
	while (p)
	{
		if (ContinueCurrPath(p->wpt_index) < 1)
			return -6;

		if (ExcludeFromPath(p, path_index) != 1)
			return -6;

		p = GetWaypointPointer(wpt_index, path_index);
		p = p->next;
	}

	// we're done here so we must reset it
	internals.ResetPathToContinue();

	// check both paths for errors
	if (WaypointValidatePath(path_index) != -1)
	{
		if (WaypointValidatePath(other_path) != -1)
		{
			// and update both path status, because we changed them both
			UpdatePathStatus(path_index);
			UpdatePathStatus(other_path);

			return other_path;
		}
	}

	return -6;
}

/*
* delete whole path specified in path_index
* or path that is on the nearest waypoint (the first path that is found)
*/
bool WaypointDeletePath(edict_t *pEntity, int path_index)
{
	// if path isn't specified find first one
	if (path_index == -1)
	{
		int current_waypoint = -1;

		current_waypoint = WaypointFindNearest(pEntity, 50.0, -1);

		if (current_waypoint == -1)
			return FALSE;

		// find the first path on this wpt 
		path_index = FindPath(current_waypoint);
	}

	if (DeleteWholePath(path_index))
		return TRUE;

	return FALSE;
}

/*
* returns the number of path class flags
*/
int PathClassFlagCount(int path_index)
{
	int count = 0;

	if (w_paths[path_index]->flags & P_FL_CLASS_ALL)
		count++;
	if (w_paths[path_index]->flags & P_FL_CLASS_SNIPER)
		count++;
	if (w_paths[path_index]->flags & P_FL_CLASS_MGUNNER)
		count++;

	return count;
}

/*
* returns the class of path passed in by path_index
* return "unknown" if class is invalid or there is unknown flag
*/
void GetPathClass(int path_index, char *the_class)
{
	int flags;

	// just in case
	if (path_index == -1)
	{
		strcpy(the_class, "unknown");
		return;
	}

	// get path flags
	flags = w_paths[path_index]->flags;

	// init this string
	strcpy(the_class, "");

	if (flags & P_FL_CLASS_SNIPER)
		strcat(the_class, "snipers ");
	if (flags & P_FL_CLASS_MGUNNER)
	{
		if (PathClassFlagCount(path_index) > 1)
			strcat(the_class, "& ");

		strcat(the_class, "mgunners ");
	}

	if (flags & P_FL_CLASS_ALL)
	{
		// if there is no other flag
		if (!(flags & (P_FL_CLASS_SNIPER | P_FL_CLASS_MGUNNER)))
			strcat(the_class, "all classes ");
	}
	else
		strcat(the_class, "only! ");

	// if there are no flags on this path
	if (PathClassFlagCount(path_index) < 1)
		strcpy(the_class, "unknown ");

	// cut the space at the end of the string
	int length = strlen(the_class);
	the_class[length-1] = '\n';

	return;
}

/*
* print info about path
* specified in path_index
* or the first path on the nearest waypoint
* or the currently edited path
*/
bool WaypointPathInfo(edict_t *pEntity, int path_index)
{
	char msg[128];
	char steam_msg[512];	// 512 due to TE_TEXTMESSAGE limitiation
	W_PATH *p;
	bool have_path = FALSE;

	// if no path is specified
	if (path_index == -1)
	{
		// find the nearest waypoint first
		int closest_wpt = WaypointFindNearest(pEntity, 50.0, -1);

		// get path from the nearest wpt
		// if more paths on that wpt get the first
		for (path_index = 0; path_index < num_w_paths; path_index++)
		{
			// skip free slots
			if (w_paths[path_index] == NULL)
				continue;

			// are we highlighting red team accessible path AND path is NOT for red team
			if ((wptser.GetPathToHighlight() == HIGHLIGHT_RED) &&
				!(w_paths[path_index]->flags & (P_FL_TEAM_NO | P_FL_TEAM_RED)))
				continue;	// so skip it
			// are we highlighting blue team accessible path AND path is NOT for blue team
			else if ((wptser.GetPathToHighlight() == HIGHLIGHT_BLUE) &&
				!(w_paths[path_index]->flags & (P_FL_TEAM_NO | P_FL_TEAM_BLUE)))
				continue;
			// are we highlighting one-way path AND path is NOT one-way
			else if ((wptser.GetPathToHighlight() == HIGHLIGHT_ONEWAY) &&
				!(w_paths[path_index]->flags & P_FL_WAY_ONE))
				continue;
			// are we highlighting sniper path AND path is NOT one-way
			else if ((wptser.GetPathToHighlight() == HIGHLIGHT_SNIPER) &&
				!(w_paths[path_index]->flags & P_FL_CLASS_SNIPER))
				continue;
			// are we highlighting machine gunner path AND path is NOT one-way
			else if ((wptser.GetPathToHighlight() == HIGHLIGHT_MGUNNER) &&
				!(w_paths[path_index]->flags & P_FL_CLASS_MGUNNER))
				continue;
			// are we highlighting specific path AND this index doesn't match it
			else if ((wptser.GetPathToHighlight() != HIGHLIGHT_DISABLED) && (wptser.GetPathToHighlight() != path_index) &&
				(wptser.GetPathToHighlight() != HIGHLIGHT_RED) && (wptser.GetPathToHighlight() != HIGHLIGHT_BLUE) &&
				(wptser.GetPathToHighlight() != HIGHLIGHT_ONEWAY) && (wptser.GetPathToHighlight() != HIGHLIGHT_SNIPER) &&
				(wptser.GetPathToHighlight() != HIGHLIGHT_MGUNNER))
				continue;
			
			p = w_paths[path_index];

			// search whole path for this waypoint
			while (p)
			{
				if (p->wpt_index == closest_wpt)
				{
					have_path = TRUE;
					break;
				}

				p = p->next;	// check next node
			}

			// we already found correct path so there's no need to search through
			// the rest of paths
			if (have_path)
				break;
		}
	}

	// if not close to any waypoint, but there is a path that is currently edited
	if ((path_index == -1) && internals.IsPathToContinue())
		path_index = internals.GetPathToContinue();			// then use it

	// is path specified AND the path doesn't exist
	// OR path_index is -1 (ie path == NULL)
	if (((path_index != -1) && (w_paths[path_index] == NULL)) || (path_index == -1))
		return FALSE;
	
	// print path index
	sprintf(msg, "Path %d out of %d | Path length is %d | Path starts on wpt %d\n",
		path_index + 1, num_w_paths, WaypointGetPathLength(path_index),	GetPathStart(path_index) + 1);
	
	// prepare additional steam message
	if (is_steam)
	{
		strcpy(steam_msg, msg);
		strcat(steam_msg, "Path flags: ");
	}
	
	ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

	if (w_paths[path_index]->flags & P_FL_TEAM_NO)
	{
		sprintf(msg, "This path is used by both teams\n");
		if (is_steam)
			strcat(steam_msg, "BothTeams ");
	}
	else if (w_paths[path_index]->flags & P_FL_TEAM_RED)
	{
		sprintf(msg, "This path is used by red team only!\n");
		if (is_steam)
			strcat(steam_msg, "RedTeam ");
	}
	else if (w_paths[path_index]->flags & P_FL_TEAM_BLUE)
	{
		sprintf(msg, "This path is used by blue team only!\n");
		if (is_steam)
			strcat(steam_msg, "BlueTeam ");
	}
	
	ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

	if (w_paths[path_index]->flags & P_FL_WAY_ONE)
	{
		sprintf(msg, "This path is one way only!\n");
		if (is_steam)
			strcat(steam_msg, "OneWay ");
	}
	else if (w_paths[path_index]->flags & P_FL_WAY_TWO)
	{
		sprintf(msg, "This path is two-way\n");
		if (is_steam)
			strcat(steam_msg, "TwoWay ");
	}
	else if (w_paths[path_index]->flags & P_FL_WAY_PATROL)
	{
		sprintf(msg, "This path is patrol type (cycle start-end-start)!\n");
		if (is_steam)
			strcat(steam_msg, "PatrolPath ");
	}
	
	ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

	GetPathClass(path_index, msg);

	if (is_steam)
	{
		if (strstr(msg, "snipers") != NULL)
			strcat(steam_msg, "Snipers ");
		if (strstr(msg, "mgunners") != NULL)
			strcat(steam_msg, "MGunners ");
		if (strstr(msg, "classes") != NULL)
			strcat(steam_msg, "AllClasses ");
	}
	
	ClientPrint(pEntity, HUD_PRINTNOTIFY, "This path is used by ");
	ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

	if (w_paths[path_index]->flags & P_FL_MISC_IGNORE)
	{
		ClientPrint(pEntity, HUD_PRINTNOTIFY, "The bot will ignore enemies while on this path\n");
		if (is_steam)
			strcat(steam_msg, "IgnoreEnemy ");
	}
	else if (w_paths[path_index]->flags & P_FL_MISC_AVOID)
	{
		ClientPrint(pEntity, HUD_PRINTNOTIFY, "The bot will avoid distant enemies while on this path\n");
		if (is_steam)
			strcat(steam_msg, "AvoidEnemy ");
	}

	if (w_paths[path_index]->flags & P_FL_MISC_GITEM)
	{
		ClientPrint(pEntity, HUD_PRINTNOTIFY, "The bot will look for this path while carrying goal item\n");
		if (is_steam)
			strcat(steam_msg, "CarryItem ");
	}

	if (is_steam)
	{
		// make an empty line at the end of standard console message so
		// it doesn't collide with the hud message
		ClientPrint(pEntity, HUD_PRINTNOTIFY, "\n");
		// terminate the hud message
		steam_msg[strlen(steam_msg)-1] = '\0';
		// display the HUD message on screen if we are on steam
		DisplayMsg(pEntity, steam_msg);
	}

	return TRUE;
}

/*
* print info about all paths on given waypoint index
* all paths on nearest waypoint if we pass -10 as waypoint index
* all used paths (ie. all stored in the path waypoint file for this map) if we pass -1 as waypoint index
*/
bool WaypointPrintAllPaths(edict_t *pEntity, int wpt_index = -1)
{
	W_PATH *p;
	char *team_fl, *class_fl, *way_fl;
	char misc[64];
	char msg[128];
	// these 3 allow us to split the output and print 22 paths each time this method gets called
	// (it's used to deal with non steam console limits as well as limited steam console history)
	static int printed = 0;
	static int slot = 0;
	int max_on_screen[] = { 22, 44, 66, 88, 110, 132, 154, 176, 198, 220, 242, 264, 286, 308, 330, 352, 374, 396, 418,
							440, 462, 484, 506, 528 };

	if (wpt_index == -10)
	{
		// find the nearest waypoint
		wpt_index = WaypointFindNearest(pEntity, 50.0, -1);

		// if there is no waypoint nearby we have to stop it here in this case
		if (wpt_index == -1)
			return FALSE;
	}

	// aim and cross waypoints cannot be in a path
	if (wpt_index != -1 && waypoints[wpt_index].flags & (W_FL_AIMING | W_FL_CROSS))
		return FALSE;

	// if we aren't printing all used paths or we already printed them all
	if (wpt_index != -1 || ((printed + 1) >= num_w_paths))
	{
		// we have to reset these two
		printed = 0;
		slot = 0;
	}
	
	for (int path_index = printed; path_index < num_w_paths; path_index++)
	{
		// are we printing only paths on given waypoint and this path isn't on this waypoint then skip it
		if (wpt_index != -1 && !WaypointIsInPath(wpt_index, path_index))
			continue;

		// split the output only when printing all used paths
		if (wpt_index == -1)
			printed++;

		if (w_paths[path_index] == NULL)
			sprintf(msg, "path #%d | doesn't exist (it's deleted)!\n", path_index + 1);
		else
		{
			p = w_paths[path_index];
			
			if (p->flags & P_FL_TEAM_NO)
				team_fl = "both team";
			else if (p->flags & P_FL_TEAM_RED)
				team_fl = "red team";
			else if (p->flags & P_FL_TEAM_BLUE)
				team_fl = "blue team";
			else
				team_fl = "ERROR";
			
			if (p->flags & P_FL_CLASS_ALL)
				class_fl = "all class";
			else if (p->flags & P_FL_CLASS_SNIPER)
				class_fl = "sniper only";
			else if (p->flags & P_FL_CLASS_MGUNNER)
				class_fl = "mgunner only";
			else
				class_fl = "ERROR";
			
			if (p->flags & P_FL_WAY_ONE)
				way_fl = "one-way";
			else if (p->flags & P_FL_WAY_TWO)
				way_fl = "two-way";
			else if (p->flags & P_FL_WAY_PATROL)
				way_fl = "patrol";
			else
				way_fl = "ERROR";
			
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

			sprintf(msg, "path #%d | length %d | starts on wpt #%d | %s, %s, %s (misc:%s)\n",
				path_index + 1, WaypointGetPathLength(path_index), p->wpt_index + 1, team_fl, way_fl, class_fl, misc);
		}

		ClientPrint(pEntity, HUD_PRINTCONSOLE, msg);

		// did we already use all lines/rows in this slot (ie. we printed 22 paths)
		if (printed == max_on_screen[slot])
		{
			// then select next slot and "wait for next call"
			slot++;
			return TRUE;
		}
	}

	return TRUE;
}

/*
* print all waypoint indexes and their types that are in given path
*/
bool WaypointPrintWholePath(edict_t *pEntity, int path_index)
{
	if ((path_index == -1) || (w_paths[path_index] == NULL))
		return FALSE;

	W_PATH *p = w_paths[path_index];
	char msg[256];
	char wpt_flags[242];

	static int printed = 0;
	static int slot = 0;
	// there's no maximum at how long a path can be
	// so let's expect here that no one would add more than 352 waypoints in one path
	int max_on_screen[] = { 22, 44, 66, 88, 110, 132, 154, 176, 198, 220, 242, 264, 286, 308, 330, 352 };
	
	// we need to remember which path we are printing in order to reset things if the client changes mind
	// and start printing another path before we finished printing 'current one'
	static int last_path = -1;

	// reset things if we are going to print a new path
	if (last_path != path_index)
	{
		printed = 0;
		slot = 0;
	}

	// the path is too long for one printing and we have already printed part of its waypoints
	// now we are calling it again so we must set the path pointer on the last printed waypoint
	// before we start printing next part
	if (printed != 0)
	{
		int fast_forward = 0;

		while (p)
		{
			p = p->next;
			fast_forward++;

			if (fast_forward == printed)
				break;
		}
	}

	// print the intro message only when we start from the beginning
	if (printed == 0)
		ClientPrint(pEntity, HUD_PRINTNOTIFY, "Printing whole path waypoint by waypoint (in order from start to end) ...\n");

	while (p)
	{
		WptGetType(p->wpt_index, wpt_flags);

		sprintf(msg, "[#%d.] waypoint no. %d - %s\n", printed + 1, p->wpt_index + 1, wpt_flags);
		ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

		p = p->next;
		printed++;

		// did we print 22 lines on the screen already?
		if (printed == max_on_screen[slot])
		{
			// then select next slot, remember this path and "wait for next call"
			slot++;
			last_path = path_index;
			return TRUE;
		}
	}

	printed = 0;
	slot = 0;

	return TRUE;
}

/*
* reset path back to default values (both team, all classes, one way)
* specified in path_index
* or the first path on the nearest waypoint
* or the currently edited path
*/
bool WaypointResetPath(edict_t *pEntity, int path_index)
{
	// if no path is specified
	if (path_index == -1)
	{
		// find wpt first
		int closest_wpt = WaypointFindNearest(pEntity, 50.0, -1);

		// get path from the nearest wpt
		// if more paths on that wpt get the first
		path_index = FindPath(closest_wpt);
	}

	// if not close to any waypoint, but there is a path
	// that is currently edited then use it
	if ((path_index == -1) && internals.IsPathToContinue())
		path_index = internals.GetPathToContinue();

	// is path specified AND the path doesn't exist
	// OR path_index is -1 (ie path == NULL)
	if (((path_index != -1) && (w_paths[path_index] == NULL)) ||
		(path_index == -1))
		return FALSE;

	// clear all flags
	w_paths[path_index]->flags = 0;

	// and set back the default values
	w_paths[path_index]->flags |= P_FL_TEAM_NO | P_FL_CLASS_ALL | P_FL_WAY_TWO;

	return TRUE;
}

/*
* changes team flag on path that is specified in path_index
* or the first path on the nearest waypoint
* or the currently edited path
* returns 1 if everything is OK
* returns -1 if new type is NOT specified or is invalid
* returns -2 if path is invalid
*/
int WptPathChangeTeam(edict_t *pEntity, const char *new_value, int path_index)
{
	int new_team_val = P_FL_TEAM_NO;

	// if there is no new_value (ie arg2) specified
	if ((new_value == NULL) || (*new_value == 0))
		return -1;

	if (FStrEq(new_value, "both"))
		new_team_val = P_FL_TEAM_NO;
	else if ((FStrEq(new_value, "red")) || (FStrEq(new_value, "1")))
		new_team_val = P_FL_TEAM_RED;
	else if ((FStrEq(new_value, "blue")) || (FStrEq(new_value, "2")))
		new_team_val = P_FL_TEAM_BLUE;
	else
		return -1;

	// is path specified AND the path doesn't exist
	if ((path_index != -1) && (w_paths[path_index] == NULL))
		return -2;

	// if no path is specified
	if (path_index == -1)
	{
		// find waypoint first
		int closest_wpt = WaypointFindNearest(pEntity, 50.0, -1);

		// get path from the nearest waypoint
		path_index = FindPath(closest_wpt);
	}

	// if not close to any waypoint, but there is a path
	// that is currently edited then use it
	if ((path_index == -1) && internals.IsPathToContinue())
		path_index = internals.GetPathToContinue();

	// change the team flag only if the path exist
	if ((path_index != -1) && w_paths[path_index])
	{
		// remove the old value first
		if (w_paths[path_index]->flags & P_FL_TEAM_NO)
			w_paths[path_index]->flags &= ~P_FL_TEAM_NO;
		else if (w_paths[path_index]->flags & P_FL_TEAM_RED)
			w_paths[path_index]->flags &= ~P_FL_TEAM_RED;		
		else if (w_paths[path_index]->flags & P_FL_TEAM_BLUE)
			w_paths[path_index]->flags &= ~P_FL_TEAM_BLUE;

		// set the new team flag
		w_paths[path_index]->flags |= new_team_val;

		return 1;
	}

	return -2;
}

/*
* changes class flag on path that is specified in path_index
* or the first path on the nearest waypoint
* or the currently edited path
* this method works a bit different from TeamChange and WayChange
* we check if the flag already is on this path if so we will remove it otherwise we'll set it
* the only exception is flag "all" then we have to remove all other class flags
* returns 1 if everything is OK
* returns -1 if new type is NOT specified or is invalid
* returns -2 if path is invalid
*/
int WptPathChangeClass(edict_t *pEntity, const char *new_value, int path_index)
{
	int new_class_val;

	if ((new_value == NULL) || (*new_value == 0))
		return -1;

	if (FStrEq(new_value, "all"))
		new_class_val = P_FL_CLASS_ALL;
	else if (FStrEq(new_value, "sniper"))
		new_class_val = P_FL_CLASS_SNIPER;
	else if (FStrEq(new_value, "mgunner"))
		new_class_val = P_FL_CLASS_MGUNNER;
	else
		return -1;

	if ((path_index != -1) && (w_paths[path_index] == NULL))
		return -2;

	if (path_index == -1)
	{
		int closest_wpt = WaypointFindNearest(pEntity, 50.0, -1);

		path_index = FindPath(closest_wpt);
	}

	if ((path_index == -1) && internals.IsPathToContinue())
		path_index = internals.GetPathToContinue();

	if ((path_index != -1) && w_paths[path_index])
	{
		// remove the "all" flag at first
		// we assume that the path will be restricted for some classes
		if (w_paths[path_index]->flags & P_FL_CLASS_ALL)
			w_paths[path_index]->flags &= ~P_FL_CLASS_ALL;

		// do we change this to path for all
		if (new_class_val & P_FL_CLASS_ALL)
		{
			// remove all class restrictions,
			// becuase this path is going to be accessible for all again
			if (w_paths[path_index]->flags & P_FL_CLASS_SNIPER)
				w_paths[path_index]->flags &= ~P_FL_CLASS_SNIPER;
			if (w_paths[path_index]->flags & P_FL_CLASS_MGUNNER)
				w_paths[path_index]->flags &= ~P_FL_CLASS_MGUNNER;

			// and finally set the "all" flag
			w_paths[path_index]->flags |= P_FL_CLASS_ALL;

			return 1;
		}

		// if the flag already is on the path remove it
		if (w_paths[path_index]->flags & new_class_val)
			w_paths[path_index]->flags &= ~new_class_val;
		// otherwise set the new class flag
		else
			w_paths[path_index]->flags |= new_class_val;

		// if there's no class flag on this path set the default value
		if (PathClassFlagCount(path_index) < 1)
			w_paths[path_index]->flags |= P_FL_CLASS_ALL;

		// NOTE: This (above) system isn't nice it should be changed so that
		// the default flag ("all") is kept on the path all the time, but we will probably
		// need to change some methods in bot_navigate.cpp

		return 1;
	}

	return -2;
}

/*
* changes way/direction flag on path that is specified in path_index
* or the first path on the nearest waypoint
* or the currently edited path
* returns 1 if everything is OK
* returns -1 if new type is NOT specified or is invalid
* returns -2 if path is invalid
*/
int WptPathChangeWay(edict_t *pEntity, const char *new_value, int path_index)
{
	int new_class_val = P_FL_WAY_TWO;

	if ((new_value == NULL) || (*new_value == 0))
		return -1;

	if (FStrEq(new_value, "one"))
		new_class_val = P_FL_WAY_ONE;
	else if (FStrEq(new_value, "two"))
		new_class_val = P_FL_WAY_TWO;
	else if (FStrEq(new_value, "patrol"))
		new_class_val = P_FL_WAY_PATROL;
	else
		return -1;

	if ((path_index != -1) && (w_paths[path_index] == NULL))
		return -2;

	if (path_index == -1)
	{
		int closest_wpt = WaypointFindNearest(pEntity, 50.0, -1);

		path_index = FindPath(closest_wpt);
	}

	if ((path_index == -1) && internals.IsPathToContinue())
		path_index = internals.GetPathToContinue();

	if ((path_index != -1) && w_paths[path_index])
	{
		if (w_paths[path_index]->flags & P_FL_WAY_ONE)
			w_paths[path_index]->flags &= ~P_FL_WAY_ONE;
		else if (w_paths[path_index]->flags & P_FL_WAY_TWO)
			w_paths[path_index]->flags &= ~P_FL_WAY_TWO;		
		else if (w_paths[path_index]->flags & P_FL_WAY_PATROL)
			w_paths[path_index]->flags &= ~P_FL_WAY_PATROL;

		// set the new direction flag
		w_paths[path_index]->flags |= new_class_val;

		return 1;
	}

	return -2;
}


/*
* changes miscellaneous/additional flag on path that is specified in path_index
* or the first path on the nearest waypoint
* or the currently edited path
* returns 1 if everything is OK
* returns -1 if new type is NOT specified or is invalid
* returns -2 if path is invalid
*/
int WptPathChangeMisc(edict_t *pEntity, const char *new_value, int path_index)
{
	int new_misc_val; 

	if ((new_value == NULL) || (*new_value == 0))
		return -1;

	if ((FStrEq(new_value, "avoid_enemy")) || (FStrEq(new_value, "avoidenemy")))
		new_misc_val = P_FL_MISC_AVOID;
	else if ((FStrEq(new_value, "ignore_enemy")) || (FStrEq(new_value, "ignoreenemy")))
		new_misc_val = P_FL_MISC_IGNORE;
	else if ((FStrEq(new_value, "carry_item")) || (FStrEq(new_value, "carryitem")))
		new_misc_val = P_FL_MISC_GITEM;
	else
		return -1;

	if ((path_index != -1) && (w_paths[path_index] == NULL))
		return -2;

	if (path_index == -1)
	{
		int closest_wpt = WaypointFindNearest(pEntity, 50.0, -1);

		path_index = FindPath(closest_wpt);
	}

	if ((path_index == -1) && internals.IsPathToContinue())
		path_index = internals.GetPathToContinue();

	if ((path_index != -1) && w_paths[path_index])
	{
		// if the flag already is on this path then remove it
		if (w_paths[path_index]->flags & new_misc_val)
			w_paths[path_index]->flags &= ~new_misc_val;
		// otherwise set it
		else
			w_paths[path_index]->flags |= new_misc_val;

		// fix unwanted\crazy combinations ...

		// remove ignore when we are setting avoid
		// (ie. it's a bit strange to tell the bot to ignore all enemies
		//  and avoid just the distant ones at the same time)
		if ((new_misc_val & P_FL_MISC_AVOID) && (w_paths[path_index]->flags & P_FL_MISC_IGNORE))
			w_paths[path_index]->flags &= ~P_FL_MISC_IGNORE;
		
		// the same issue as above
		if ((new_misc_val & P_FL_MISC_IGNORE) && (w_paths[path_index]->flags & P_FL_MISC_AVOID))
			w_paths[path_index]->flags &= ~P_FL_MISC_AVOID;

		return 1;
	}

	return -2;
}


/*
* move all waypoints (its origin) in given path by value units in given coordinate
* coord values are 1 for x coord, 2 for y coord and 3 for z coord (height)
*/
bool WaypointMoveWholePath(int path_index, float value, int coord)
{
#ifdef _DEBUG
	// validity check
	if ((path_index == -1) || (w_paths[path_index] == NULL))
		return FALSE;

	w_path *p = w_paths[path_index];

	while (p)
	{
		switch (coord)
		{
			case 1:
				waypoints[p->wpt_index].origin.x += value;
				break;
			case 2:
				waypoints[p->wpt_index].origin.y += value;
				break;
			case 3:
				waypoints[p->wpt_index].origin.z += value;
				break;
			default:
				return FALSE;
		}

		p = p->next;
	}
#endif

	return TRUE;
}


/*
* load path stucture from file
* return -1 if old path structure is detected (to allow auto conversion)
* return 0 if something went wrong
* return 1 if everything is OK
*/
int WaypointPathLoad(edict_t *pEntity, char *name)
{
	extern bool is_dedicated_server;

	char mapname[64];
	char filename[256];
	PATH_HDR header;
	char msg[256];
	bool show_paths_again = FALSE;		// TRUE if paths were turned on
	W_PATH *p = NULL;
	int path_index, path_length, flags, wpt_index;
	
	if (name == NULL)
	{
		strcpy(mapname, STRING(gpGlobals->mapname));
		strcat(mapname, ".pth");
	}
	else
	{
		strcpy(mapname, name);
		strcat(mapname, ".pth");
	}

	if (internals.IsCustomWaypoints())
		UTIL_MarineBotFileName(filename, "customwpts", mapname);
	else
		UTIL_MarineBotFileName(filename, "defaultwpts", mapname);

	if (is_dedicated_server)
	{
		sprintf(msg, "loading paths file: %s\n", filename);
		PrintOutput(NULL, msg, MType::msg_info);
	}

	FILE *bfp = fopen(filename, "rb");

	// if file exists, read the path structure from it
	if (bfp != NULL)
	{
		fread(&header, sizeof(header), 1, bfp);

		header.filetype[7] = 0;
		if (strcmp(header.filetype, "FAM_bot") == 0)
		{
			if (header.waypoint_file_version != WAYPOINT_VERSION)
			{
				sprintf(msg, "Incompatible version of MarineBot paths!\n");
				PrintOutput(pEntity, msg, MType::msg_error);

				sprintf(msg, "Paths not loading!\n");
				PrintOutput(pEntity, msg, MType::msg_warning);
				
				// "wpt load" command shouldn't let run this method at all, but just in case
				if (pEntity == NULL)
				{
					sprintf(msg, "Auto conversion started...\n");
					PrintOutput(pEntity, msg, MType::msg_info);
				}

				fclose(bfp);
				return -1;	// to start automatic conversion
			}

			// if current waypoint count doesn't match with path file record
			if (header.waypoint_flag != num_waypoints)
			{
				fclose(bfp);

				ALERT(at_error, "MarineBot - Error wpts doesn't match to path file record!\n");

				return 0;
			}

			// init total count of paths
			num_w_paths = header.number_of_paths;

			// check if path count is valid
			if (num_w_paths >= MAX_W_PATHS)
			{
				fclose(bfp);

				ALERT(at_error, "MarineBot - Error the path count exceeded max limit!\n");

				return 0;
			}

			header.mapname[31] = 0;

			if ((strcmp(header.mapname, STRING(gpGlobals->mapname)) == 0) ||
				(name != NULL))
			{
				// remove any existing paths
				FreeAllPaths();

				// if paths are displayed then hide them until the rebuild is done
				if (wptser.IsShowPaths())
				{
					wptser.ResetShowPaths();
					show_paths_again = TRUE;
				}

				// load and rebuild all paths
				for (int all_paths = 0; all_paths < num_w_paths; all_paths++)
				{
					// read the stored path_index
					fread(&path_index, sizeof(path_index), 1, bfp);

					p = (W_PATH *)malloc(sizeof(W_PATH));	// create new head node

					if (p == NULL)
					{
						ALERT(at_error, "MarineBot - Error allocating memory for path!\n");

						return 0;
					}

					// init pointers to previous and next node
					p->prev = NULL;
					p->next = NULL;

					// store head node of the new path into w_paths array
					w_paths[path_index] = p;

					// init actual path (a must for Continue Curr Path())
					internals.SetPathToContinue(path_index);

					// read the path length
					fread(&path_length, sizeof(path_length), 1, bfp);

					// delete the path and go to next path
					if (path_length == 0)
					{
						DeleteWholePath(path_index);
						continue;
					}

					// read path flags to local temp flags
					fread(&flags, sizeof(w_paths[path_index]->flags), 1, bfp);

					// read the head node wpt_index
					fread(&wpt_index, sizeof(p->wpt_index), 1, bfp);
					
					p->wpt_index = wpt_index;

					// read rest of paths wpt_index one by one from 1 up to path_length,
					// because the first is already done (in head node)
					for (int index = 1; index < path_length; index++)
					{
						fread(&wpt_index, sizeof(p->wpt_index), 1, bfp);

						// check if waypoint was added correctly
						//if (ContinueCurrPath(wpt_index) != 1)												NEW CODE 094 (prev code)
						if (ContinueCurrPath(wpt_index, FALSE) < 1)//										NEW CODE 094
							return 0;
					}

					// finally set all path flags
					w_paths[path_index]->flags = flags;
				}

				g_waypoint_paths = TRUE;	// keep track so path can be freed
			}
			else
			{
				sprintf(msg, "MarineBot paths are not for this map! %s\n", filename);
				PrintOutput(pEntity, msg, MType::msg_warning);
				
				sprintf(msg, "Bots will not play well this map!\n");
				PrintOutput(pEntity, msg, MType::msg_info);

				fclose(bfp);
				return 0;
			}

			header.author[31] = 0;
			if (strcmp(header.author, "") == 0)
				strcpy(wpt_author, "unknown");
			else
				strcpy(wpt_author, header.author);

			header.modified_by[31] = 0;
			if (strcmp(header.modified_by, "") == 0)
				strcpy(wpt_modified, "unknown");
			else
				strcpy(wpt_modified, header.modified_by);
		}
		else
		{
			sprintf(msg, "Not MarineBot path file! %s\n", filename);
			PrintOutput(pEntity, msg, MType::msg_warning);

			sprintf(msg, "Bots will not play well this map!\n");
			PrintOutput(pEntity, msg, MType::msg_info);
			
			fclose(bfp);
			return 0;
		}

		fclose(bfp);
	}
	else
	{
		sprintf(msg, "path file not found! %s\n", filename);
		PrintOutput(pEntity, msg, MType::msg_warning);

		sprintf(msg, "Bots will not play well this map!\n");
		PrintOutput(pEntity, msg, MType::msg_info);

		return 0;
	}

	// clear current path "pointer"
	internals.ResetPathToContinue();

	// if the paths were temporarily hidden then display them again
	if (show_paths_again)
		wptser.SetShowPaths(true);

	return 1;
}


/*
* save path stucture into the file
* return TRUE if everything is OK
* return FALSE if something went wrong or if there are no paths
*/
bool WaypointPathSave(const char *name)
{
	char filename[256];
	char mapname[64];
	PATH_HDR header;
	int path_index, path_length, path_count;

	// remove all invalid paths first
	UTIL_RemoveFalsePaths(FALSE);

	// check if at least one path exists
	path_count = 0;
	for (int paths = 0; paths < num_w_paths; paths++)
	{
		if (w_paths[paths] != NULL)
		{
			// break it we know that path exists
			path_count++;
			break;
		}
	}
	
	// don't write .pth file if there are no paths
	if (path_count == 0)
	{
		return FALSE;
	}

	// init the path file header with all important data

	strcpy(header.filetype, "FAM_bot");

	header.waypoint_file_version = WAYPOINT_VERSION;
	header.waypoint_flag = num_waypoints;		// critical for compatibility
	header.number_of_paths = num_w_paths;

	memset(header.mapname, 0, sizeof(header.mapname));
	strncpy(header.mapname, STRING(gpGlobals->mapname), 31);
	header.mapname[31] = 0;

	// write authors signature
	memset(header.author, 0, sizeof(header.author));
	if (wpt_author[0] == 0)
		strncpy(header.author, "unknown", 31);
	else
		strncpy(header.author, wpt_author, 31);
	header.author[31] = 0;

	// write the one who modified them
	memset(header.modified_by, 0, sizeof(header.modified_by));
	if (wpt_modified[0] == 0)
		strncpy(header.modified_by, "unknown", 31);
	else
		strncpy(header.modified_by, wpt_modified, 31);
	header.modified_by[31] = 0;

	// if we used our own name save them under it
	if (name != NULL)
		strcpy(mapname, name);
	// otherwise use real mapname
	else
		strcpy(mapname, STRING(gpGlobals->mapname));
	strcat(mapname, ".pth");

	if (internals.IsCustomWaypoints())
		UTIL_MarineBotFileName(filename, "customwpts", mapname);
	else
		UTIL_MarineBotFileName(filename, "defaultwpts", mapname);

	FILE *bfp = fopen(filename, "wb");

	// if file wasn't opened
	if (bfp == NULL)
		return FALSE;

	// write the path header to the file
	fwrite(&header, sizeof(header), 1, bfp);

	// save all the paths
	for (path_index = 0; path_index < num_w_paths; path_index++)
	{
		// does this path exist
		if (w_paths[path_index] != NULL)
		{
			// update this path status first
			UpdatePathStatus(path_index);

			// save path_index
			fwrite(&path_index, sizeof(path_index), 1, bfp);

			// get length of this path
			path_length = WaypointGetPathLength(path_index);

			// save path length
			fwrite(&path_length, sizeof(path_length), 1, bfp);

			// save paths flags
			fwrite(&w_paths[path_index]->flags, sizeof(w_paths[path_index]->flags), 1, bfp);

			// now save the whole path

			W_PATH *p = w_paths[path_index];	// set the pointer to the head node

			// save path wpt_index one by one until reaches the end
			while (p != NULL)
			{
				fwrite(&p->wpt_index, sizeof(p->wpt_index), 1, bfp);

				p = p->next;  // go to next node in linked list
			}
		}
		// otherwise store only index and length = 0
		else
		{
			// save path_index
			fwrite(&path_index, sizeof(path_index), 1, bfp);

			// set path length to zero (to know that the path was deleted)
			path_length = 0;

			// save path length
			fwrite(&path_length, sizeof(path_length), 1, bfp);
		}
	}

	fclose(bfp);

	return TRUE;
}


/*
* load only some path data (based on waypoint version) to convert older
* paths to latest (actual) version
*/
bool WaypointPathLoadUnsupported(edict_t *pEntity)
{
	extern bool is_dedicated_server;

	char mapname[64];
	char filename[256];
	//OLD_PATH_HDR old_header;
	PATH_HDR header;		// can be used current header until any change is done
	char msg[256];
	bool show_paths_again = FALSE;		// TRUE if paths were turned on
	W_PATH *p = NULL;
	int path_index, path_length, flags, wpt_index;
	bool known;
	OLD_W_PATH *old_paths = NULL;	// we don't really need it here

	strcpy(mapname, STRING(gpGlobals->mapname));
	strcat(mapname, ".pth");
	
	if (internals.IsCustomWaypoints())
		UTIL_MarineBotFileName(filename, "customwpts", mapname);
	else
		UTIL_MarineBotFileName(filename, "defaultwpts", mapname);

	if (is_dedicated_server)
	{
		sprintf(msg, "loading unsupported paths from: %s\n", filename);
		PrintOutput(NULL, msg, MType::msg_info);
	}

	FILE *bfp = fopen(filename, "rb");

	// if file exists, read the path structure from it
	if (bfp != NULL)
	{
		fread(&header, sizeof(header), 1, bfp);

		header.filetype[7] = 0;
		if (strcmp(header.filetype, "FAM_bot") == 0)
		{
			if (header.waypoint_file_version == WAYPOINT_VERSION)
			{
				sprintf(msg, "This path file is actual. No need to load it this way.\n");
				PrintOutput(pEntity, msg, MType::msg_info);

				fclose(bfp);
				return FALSE;
			}

			if (header.waypoint_file_version != WAYPOINT_VERSION)
			{
				known = FALSE;

				if (header.waypoint_file_version == OLD_WAYPOINT_VERSION)
				{
					known = TRUE;

					sprintf(msg, "found known older MarineBot path file (version %d - MB0.9b) - conversion in progress...\n", OLD_WAYPOINT_VERSION);
					PrintOutput(pEntity, msg, MType::msg_info);
				}
				else
				{
					sprintf(msg, "unknown path file version (probably too old) - conversion failed!\n");
					PrintOutput(pEntity, msg, MType::msg_error);

					fclose(bfp);
					return FALSE;
				}
			}

			if (header.waypoint_flag != num_waypoints)
			{
				sprintf(msg, "Waypoint file doesn't match to path file record! - conversion failed!\n");
				PrintOutput(pEntity, msg, MType::msg_error);

				fclose(bfp);
				return FALSE;
			}

			num_w_paths = header.number_of_paths;

			if (num_w_paths >= MAX_W_PATHS)
			{
				sprintf(msg, "MarineBot - Error amount of path exceeded max limit!\n");

				if (externals.GetIsLogging())
					UTIL_MBLogPrint(msg);
				else
					ALERT(at_error, msg);

				fclose(bfp);
				return FALSE;
			}

			header.mapname[31] = 0;

			if (strcmp(header.mapname, STRING(gpGlobals->mapname)) == 0)
			{
				FreeAllPaths();

				sprintf(msg, "loading converted path file\n");
				PrintOutput(pEntity, msg, MType::msg_info);

				if (wptser.IsShowPaths())
				{
					wptser.ResetShowPaths();
					show_paths_again = TRUE;
				}

				// load and rebuild all paths
				for (int all_paths = 0; all_paths < num_w_paths; all_paths++)
				{
					// create new head node for the new path
					p = (W_PATH *)malloc(sizeof(W_PATH));

					if (p == NULL)
					{
						ALERT(at_error, "MarineBot - Error allocating memory for path!\n");

						return FALSE;
					}

					// store head node of the new path into w_paths array
					w_paths[all_paths] = p;

					// init all new values to default at first
					w_paths[all_paths]->wpt_index = -1;
					w_paths[all_paths]->flags = 0;		// init flags
					w_paths[all_paths]->prev = NULL;
					w_paths[all_paths]->next = NULL;

					if (known)
					{
						// read the stored path_index
						fread(&path_index, sizeof(path_index), 1, bfp);

						if (p == NULL)
						{
							ALERT(at_error, "MarineBot - Error allocating memory for path!\n");

							return FALSE;
						}

						// init pointers to previous and next node
						p->prev = NULL;
						p->next = NULL;

						// store head node of the new path into w_paths array
						w_paths[path_index] = p;

						// init actual path (a must for Continue Curr Path())
						internals.SetPathToContinue(path_index);

						// read the path length
						fread(&path_length, sizeof(path_length), 1, bfp);

						// delete the path and go to next path
						if (path_length == 0)
						{
							DeleteWholePath(path_index);
							continue;
						}

						flags = 0;

						// read path flags to local temp flags
						fread(&flags, sizeof(old_paths->flags), 1, bfp);

						// read the head node wpt_index
						fread(&wpt_index, sizeof(old_paths->wpt_index), 1, bfp);
					
						p->wpt_index = wpt_index;

						// read rest of paths wpt_index one by one from 2nd (ie. 1) up to
						// path_length, the 1st (ie. 0) is already done (in head node)
						for (int index = 1; index < path_length; index++)
						{
							fread(&wpt_index, sizeof(old_paths->wpt_index), 1, bfp);

							// check if wpt was added correctly
							//if (ContinueCurrPath(wpt_index) != 1)										NEW CODE 094 (prev code)
							if (ContinueCurrPath(wpt_index, FALSE) < 1)//								NEW CODE 094
								return FALSE;
						}

						// finally convert old/obsolete things
						
						// !!! There's nothing to be converted now
						//  (i.e. no critical changes between version 6 and 7)
						
						// just take the whole flags value and use it
						w_paths[path_index]->flags = flags;
					}
				}

				g_waypoint_paths = TRUE;	// keep track so path can be freed
			}
			else
			{
				sprintf(msg, "MarineBot paths are not for this map! %s\n", filename);
				PrintOutput(pEntity, msg, MType::msg_warning);

				fclose(bfp);
				return FALSE;
			}

			header.author[31] = 0;
			// if no author so set it to unknown
			if (strcmp(header.author, "") == 0)
				strcpy(wpt_author, "unknown");
			else
				strcpy(wpt_author, header.author);

			header.modified_by[31] = 0;
			// the guy who modified the waypoints is not specified? then set it to unknown
			if (strcmp(header.modified_by, "") == 0)
				strcpy(wpt_modified, "unknown");
			else
				strcpy(wpt_modified, header.modified_by);
		}
		else
		{
			sprintf(msg, "Not MarineBot path file! %s\n", filename);
			PrintOutput(pEntity, msg, MType::msg_error);
			
			fclose(bfp);
			return FALSE;
		}

		fclose(bfp);
	}
	else
	{
		sprintf(msg, "Path file not found! %s\n", filename);
		PrintOutput(pEntity, msg, MType::msg_warning);

		return FALSE;
	}

	internals.ResetPathToContinue();

	if (show_paths_again)
		wptser.SetShowPaths(true);

	return TRUE;
}


/*
* added this method because of the fact that many waypoints are available in this ancient system and
* we can still use them without bigger issues
* convert paths from waypoint system in version 5 and convert them to recent version 7
*/
bool WaypointPathLoadVersion5(edict_t *pEntity)
{
	extern bool is_dedicated_server;

	char mapname[64];
	char filename[256];
	//OLD_PATH_HDR header;
	PATH_HDR header;		// can be used current header until any change is done
	char msg[256];
	bool show_paths_again = FALSE;		// TRUE if paths were turned on
	W_PATH *p = NULL;
	int path_index, path_length, flags, wpt_index;
	bool known;
	OLD_W_PATH *old_paths = NULL;	// we don't really need it here
	int OLD_WAYPOINT_VERSION_5 = 5;		// we have to "override" standard conversion by sending even older version number

	strcpy(mapname, STRING(gpGlobals->mapname));
	strcat(mapname, ".pth");
	
	if (internals.IsCustomWaypoints())
		UTIL_MarineBotFileName(filename, "customwpts", mapname);
	else
		UTIL_MarineBotFileName(filename, "defaultwpts", mapname);

	if (is_dedicated_server)
	{
		sprintf(msg, "loading unsupported paths from: %s\n", filename);
		PrintOutput(NULL, msg, MType::msg_info);
	}

	FILE *bfp = fopen(filename, "rb");

	// if file exists, read the path structure from it
	if (bfp != NULL)
	{
		fread(&header, sizeof(header), 1, bfp);

		header.filetype[7] = 0;
		if (strcmp(header.filetype, "FAM_bot") == 0)
		{
			if (header.waypoint_file_version == WAYPOINT_VERSION)
			{
				sprintf(msg, "This path file is actual. No need to load it this way.\n");
				PrintOutput(pEntity, msg, MType::msg_info);

				fclose(bfp);
				return FALSE;
			}

			if (header.waypoint_file_version != WAYPOINT_VERSION)
			{
				known = FALSE;

				if (header.waypoint_file_version == OLD_WAYPOINT_VERSION_5)
				{
					known = TRUE;

					sprintf(msg, "found known older MarineBot path file (version %d - MB0.8b) - conversion in progress...\n", OLD_WAYPOINT_VERSION_5);
					PrintOutput(pEntity, msg, MType::msg_info);
				}
				else
				{
					sprintf(msg, "unknown path file version (probably too old) - conversion failed!\n");
					PrintOutput(pEntity, msg, MType::msg_error);

					fclose(bfp);
					return FALSE;
				}
			}

			if (header.waypoint_flag != num_waypoints)
			{
				sprintf(msg, "Waypoint file doesn't match to path file record! - conversion failed!\n");
				PrintOutput(pEntity, msg, MType::msg_error);

				fclose(bfp);
				return FALSE;
			}

			num_w_paths = header.number_of_paths;

			if (num_w_paths >= MAX_W_PATHS)
			{
				sprintf(msg, "MarineBot - Error amount of path exceeded max limit!\n");

				if (externals.GetIsLogging())
					UTIL_MBLogPrint(msg);
				else
					ALERT(at_error, msg);

				fclose(bfp);
				return FALSE;
			}

			header.mapname[31] = 0;

			if (strcmp(header.mapname, STRING(gpGlobals->mapname)) == 0)
			{
				// these are needed for the conversion (used in version 5)
				// team flag constants
				int OLD_P_FL_TEAM_NO = 1<<0;
				int OLD_P_FL_TEAM_RED =	1<<1;
				int OLD_P_FL_TEAM_BLUE = 1<<2;
				// way/direction constants
				int OLD_P_FL_WAY_ONE = 1<<10;
				int OLD_P_FL_WAY_TWO = 1<<11;
				int OLD_P_FL_WAY_PATROL = 1<<12;
				// class flag constants
				int OLD_P_FL_CLASS_ALL = 1<<20;
				int OLD_P_FL_CLASS_SNIPER = 1<<21;
				int OLD_P_FL_CLASS_MGUNNER = 1<<22;

				FreeAllPaths();

				sprintf(msg, "loading converted path file\n");
				PrintOutput(pEntity, msg, MType::msg_info);

				if (wptser.IsShowPaths())
				{
					wptser.ResetShowPaths();
					show_paths_again = TRUE;
				}

				// load and rebuild all paths
				for (int all_paths = 0; all_paths < num_w_paths; all_paths++)
				{
					// create new head node
					p = (W_PATH *)malloc(sizeof(W_PATH));

					if (p == NULL)
					{
						ALERT(at_error, "MarineBot - Error allocating memory for path!\n");

						return FALSE;
					}

					// store head node of the new path into w_paths array
					w_paths[all_paths] = p;

					// init all new values to default at first
					w_paths[all_paths]->wpt_index = -1;
					w_paths[all_paths]->flags = 0;		// init flags
					w_paths[all_paths]->prev = NULL;
					w_paths[all_paths]->next = NULL;

					if (known)
					{
						// read the stored path_index
						fread(&path_index, sizeof(path_index), 1, bfp);

						if (p == NULL)
						{
							ALERT(at_error, "MarineBot - Error allocating memory for path!\n");

							return FALSE;
						}

						// init pointers to previous and next node
						p->prev = NULL;
						p->next = NULL;

						// store head node of the new path into w_paths array
						w_paths[path_index] = p;

						// init actual path (a must for Continue Curr Path())
						internals.SetPathToContinue(path_index);

						// read the path length
						fread(&path_length, sizeof(path_length), 1, bfp);

						// delete the path and go to next path
						if (path_length == 0)
						{
							DeleteWholePath(path_index);
							continue;
						}

						flags = 0;

						// read all path flags to local temp flags
						fread(&flags, sizeof(old_paths->flags), 1, bfp);

						// read the head node wpt_index
						fread(&wpt_index, sizeof(old_paths->wpt_index), 1, bfp);
					
						p->wpt_index = wpt_index;

						// read rest of paths wpt_index one by one from 2nd (ie. 1) up to
						// path_length, the 1st (ie. 0) is already done (in head node)
						for (int index = 1; index < path_length; index++)
						{
							fread(&wpt_index, sizeof(old_paths->wpt_index), 1, bfp);

							// check if wpt was added correctly
							//if (ContinueCurrPath(wpt_index) != 1)											NEW CODE 094 (prev code)
							if (ContinueCurrPath(wpt_index, FALSE) < 1)//									NEW CODE 094
								return FALSE;
						}

						// finally convert all path flags
						if (flags & OLD_P_FL_TEAM_NO)
							w_paths[path_index]->flags |= P_FL_TEAM_NO;
						else if (flags & OLD_P_FL_TEAM_RED)
							w_paths[path_index]->flags |= P_FL_TEAM_RED;
						else if (flags & OLD_P_FL_TEAM_BLUE)
							w_paths[path_index]->flags |= P_FL_TEAM_BLUE;
						else
						{
							//@@@@@@@@@@@@@@@@@@
							#ifdef _DEBUG
							ALERT(at_console, "UNKNOWN path TEAM flag!!!\n");
							#endif
						}

						if (flags & OLD_P_FL_CLASS_ALL)
							w_paths[path_index]->flags |= P_FL_CLASS_ALL;
						if (flags & OLD_P_FL_CLASS_SNIPER)
							w_paths[path_index]->flags |= P_FL_CLASS_SNIPER;
						if (flags & OLD_P_FL_CLASS_MGUNNER)
							w_paths[path_index]->flags |= P_FL_CLASS_MGUNNER;
						if (!(flags & (OLD_P_FL_CLASS_ALL | OLD_P_FL_CLASS_SNIPER | OLD_P_FL_CLASS_MGUNNER)))
						{
							//@@@@@@@@@@@@@@@@@@
							#ifdef _DEBUG
							ALERT(at_console, "UNKNOWN path CLASS flag!!!\n");
							#endif
						}

						if (flags & OLD_P_FL_WAY_ONE)
							w_paths[path_index]->flags |= P_FL_WAY_ONE;
						else if (flags & OLD_P_FL_WAY_TWO)
							w_paths[path_index]->flags |= P_FL_WAY_TWO;
						else if (flags & OLD_P_FL_WAY_PATROL)
							w_paths[path_index]->flags |= P_FL_WAY_PATROL;
						else
						{
							//@@@@@@@@@@@@@@@@@@
							#ifdef _DEBUG
							ALERT(at_console, "UNKNOWN path WAY flag!!!\n");
							#endif
						}
					}
				}

				g_waypoint_paths = TRUE;	// keep track so path can be freed
			}
			else
			{
				sprintf(msg, "MarineBot paths are not for this map! %s\n", filename);
				PrintOutput(pEntity, msg, MType::msg_warning);

				fclose(bfp);
				return FALSE;
			}

			header.author[31] = 0;
			// if no author then set it to unknown
			if (strcmp(header.author, "") == 0)
				strcpy(wpt_author, "unknown");
			else
				strcpy(wpt_author, header.author);

			header.modified_by[31] = 0;
			// the guy who modified the waypoints is not specified? then set it to unknown
			if (strcmp(header.modified_by, "") == 0)
				strcpy(wpt_modified, "unknown");
			else
				strcpy(wpt_modified, header.modified_by);
		}
		else
		{
			sprintf(msg, "Not MarineBot path file! %s\n", filename);
			PrintOutput(pEntity, msg, MType::msg_error);
			
			fclose(bfp);
			return FALSE;
		}

		fclose(bfp);
	}
	else
	{
		sprintf(msg, "Path file not found! %s\n", filename);
		PrintOutput(pEntity, msg, MType::msg_warning);

		return FALSE;
	}

	internals.ResetPathToContinue();

	if (show_paths_again)
		wptser.SetShowPaths(true);

	return TRUE;
}


/*
* load waypoint structure from file
* return -1 if old waypoint structure is detected (to allow auto conversion)
* return 0 if something went wrong (like not MarineBot waypoints etc.)
* return 1 if everything is OK
*/
int WaypointLoad(edict_t *pEntity, char *name)
{
	extern bool is_dedicated_server;

	char mapname[64];
	char filename[256];
	WAYPOINT_HDR header;
	char msg[256];
	int i;

	if (name == NULL)
	{
		strcpy(mapname, STRING(gpGlobals->mapname));
		strcat(mapname, ".wpt");
	}
	else
	{
		strcpy(mapname, name);
		strcat(mapname, ".wpt");
	}

	if (internals.IsCustomWaypoints())
		UTIL_MarineBotFileName(filename, "customwpts", mapname);
	else
		UTIL_MarineBotFileName(filename, "defaultwpts", mapname);

	if (is_dedicated_server)
	{
		sprintf(msg, "loading waypoint file: %s\n", filename);

		PrintOutput(NULL, msg, MType::msg_info);
	}

	FILE *bfp = fopen(filename, "rb");

	// if file exists, read the waypoint structure from it
	if (bfp != NULL)
	{
		fread(&header, sizeof(header), 1, bfp);

		header.filetype[7] = 0;
		if (strcmp(header.filetype, "FAM_bot") == 0)
		{
			if (header.waypoint_file_version != WAYPOINT_VERSION)
			{
				sprintf(msg, "Incompatible waypoint file version!\n");
				PrintOutput(pEntity, msg, MType::msg_error);

				sprintf(msg, "Waypoints not loading!\n");
				PrintOutput(pEntity, msg, MType::msg_warning);

				// don't print this on user command "wpt load",
				// because in that case we don't autoconvert them
				if (pEntity == NULL)
				{
					sprintf(msg, "Auto conversion started...\n");
					PrintOutput(pEntity, msg, MType::msg_info);
				}

				fclose(bfp);
				return -1;		// to start auto waypoint conversion
			}

			header.mapname[31] = 0;

			if ((strcmp(header.mapname, STRING(gpGlobals->mapname)) == 0) ||
				(name != NULL))
			{
				// remove any existing waypoints
				WaypointInit();

				for (i=0; i < header.number_of_waypoints; i++)
				{
					fread(&waypoints[i], sizeof(waypoints[0]), 1, bfp);
					num_waypoints++;

					// fix some bugs that might happen

					// fix some problem waypoint tag/flag combinations ...
					WaypointFixInvalidFlags(i);
				}
				
				// read the triggers
				for (int index = 0; index < MAX_TRIGGERS; index++)
				{
					fread(&trigger_events[index], sizeof(trigger_events[0]), 1, bfp);

					// we have to mark this slot as a used
					if (strlen(trigger_events[index].message) > 1)
						trigger_gamestate[index].SetUsed(true);
				}
			}
			else
			{
				sprintf(msg, "Waypoints are not for this map!\n");
				PrintOutput(pEntity, msg, MType::msg_warning);

				sprintf(msg, "Bots will not play well this map!\n");
				PrintOutput(pEntity, msg, MType::msg_info);

				fclose(bfp);
				return 0;
			}

			// get waypoint's signatures
			header.author[31] = 0;
			if (strcmp(header.author, "") == 0)
				strcpy(wpt_author, "unknown");
			else
				strcpy(wpt_author, header.author);

			header.modified_by[31] = 0;
			if (strcmp(header.modified_by, "") == 0)
				strcpy(wpt_modified, "unknown");
			else
				strcpy(wpt_modified, header.modified_by);
		}
		else
		{
			sprintf(msg, "Not MarineBot waypoint file! %s\n", filename);
			PrintOutput(pEntity, msg, MType::msg_error);

			sprintf(msg, "Bots will not play well this map!\n");
			PrintOutput(pEntity, msg, MType::msg_info);
			
			fclose(bfp);
			return 0;
		}

		fclose(bfp);
	}
	else
	{
		sprintf(msg, "Waypoint file not found! %s\n", filename);
		PrintOutput(pEntity, msg, MType::msg_error);

		sprintf(msg, "Bots will not play well this map!\n");
		PrintOutput(pEntity, msg, MType::msg_info);

		return -10;
	}

	return 1;
}

/*
* save waypoint structure into the file
*/
bool WaypointSave(const char *name)
{
	char filename[256];
	char mapname[64];
	WAYPOINT_HDR header;
	int index;

	// init the waypoint file header with all important data

	strcpy(header.filetype, "FAM_bot");

	header.waypoint_file_version = WAYPOINT_VERSION;
	header.waypoint_file_flags = 0;  // not currently used
	header.number_of_waypoints = num_waypoints;

	memset(header.mapname, 0, sizeof(header.mapname));
	strncpy(header.mapname, STRING(gpGlobals->mapname), 31);
	header.mapname[31] = 0;

	// write author's signature
	memset(header.author, 0, sizeof(header.author));
	if (wpt_author[0] == 0)
		strncpy(header.author, "unknown", 31);
	else
		strncpy(header.author, wpt_author, 31);
	header.author[31] = 0;

	// write the signature of the one who modified them
	memset(header.modified_by, 0, sizeof(header.modified_by));
	if (wpt_modified[0] == 0)
		strncpy(header.modified_by, "unknown", 31);
	else
		strncpy(header.modified_by, wpt_modified, 31);
	header.modified_by[31] = 0;

	// if we used our own name save them under it
	if (name != NULL)
		strcpy(mapname, name);
	// otherwise use real mapname
	else
		strcpy(mapname, STRING(gpGlobals->mapname));
	strcat(mapname, ".wpt");

	if (internals.IsCustomWaypoints())
		UTIL_MarineBotFileName(filename, "customwpts", mapname);
	else
		UTIL_MarineBotFileName(filename, "defaultwpts", mapname);

	FILE *bfp = fopen(filename, "wb");

	if (bfp != NULL)
	{
		// write the waypoint header to the file
		fwrite(&header, sizeof(header), 1, bfp);

		// write the waypoint data to the file
		for (index=0; index < num_waypoints; index++)
		{
			fwrite(&waypoints[index], sizeof(waypoints[0]), 1, bfp);
		}

		// write the triggers
		for (index = 0; index < MAX_TRIGGERS; index++)
		{
			fwrite(&trigger_events[index], sizeof(trigger_events[0]), 1, bfp);
		}

		fclose(bfp);

		return TRUE;
	}

	return FALSE;
}

/*
* load only some waypoint data (based on waypoint version) to convert older
* waypoints to latest (actual) version
*/
bool WaypointLoadUnsupported(edict_t *pEntity)
{
	extern bool is_dedicated_server;

	char mapname[64];
	char filename[256];
	//OLD_WPT_HDR header;
	WAYPOINT_HDR header;		// can be used current header until any change is done
	char msg[256];
	int index;
	OLD_WAYPOINT oldwaypoints_ver[1];	// we need just one slot because we are converting them one by one
	TRIGGER_EVENT_OLD old_triggers[1];
	bool known;

	strcpy(mapname, STRING(gpGlobals->mapname));
	strcat(mapname, ".wpt");

	if (internals.IsCustomWaypoints())
		UTIL_MarineBotFileName(filename, "customwpts", mapname);
	else
		UTIL_MarineBotFileName(filename, "defaultwpts", mapname);

	if (is_dedicated_server)
	{
		sprintf(msg, "loading unsupported waypoints from: %s\n", filename);
		
		PrintOutput(NULL, msg, MType::msg_info);
	}

	FILE *bfp = fopen(filename, "rb");

	// if file exists, read the waypoint structure from it
	if (bfp != NULL)
	{
		fread(&header, sizeof(header), 1, bfp);

		header.filetype[7] = 0;
		if (strcmp(header.filetype, "FAM_bot") == 0)
		{
			if (header.waypoint_file_version == WAYPOINT_VERSION)
			{
				sprintf(msg, "This waypoint file is actual. No need to load it this way.\n");
				PrintOutput(pEntity, msg, MType::msg_info);

				fclose(bfp);
				return FALSE;
			}

			if (header.waypoint_file_version != WAYPOINT_VERSION)
			{
				known = FALSE;

				// convert only waypoints that are one version back
				if (header.waypoint_file_version == OLD_WAYPOINT_VERSION)
				{
					// bot know this system
					known = TRUE;

					sprintf(msg, "found known older MarineBot waypoint file (version %d - MB0.9b) - conversion in progress...\n", OLD_WAYPOINT_VERSION);
					PrintOutput(pEntity, msg, MType::msg_info);
				}
				// otherwise ignore all other (older) waypoint versions
				else
				{
					sprintf(msg, "unknown waypoint file version (probably too old) - conversion failed!\n");
					PrintOutput(pEntity, msg, MType::msg_error);

					fclose(bfp);
					return FALSE;
				}
			}

			header.mapname[31] = 0;

			if (strcmp(header.mapname, STRING(gpGlobals->mapname)) == 0)
			{
				// remove any existing waypoints
				WaypointInit();

				sprintf(msg, "loading converted waypoint file\n");
				PrintOutput(pEntity, msg, MType::msg_info);

				for (index = 0; index < header.number_of_waypoints; index++)
				{
					// init all new values to default at first
					// this is being done a few lines above,
					// but we do it again just to be completely sure
					WaypointInit(index);

					// just for sure, we don't want to load too old waypoints, because those
					// might have a lot different structure so then we are unable to read
					// correct data from the file
					if (known)
					{		
						// read all data for this waypoint
						fread(&oldwaypoints_ver[0], sizeof(oldwaypoints_ver[0]), 1, bfp);
						num_waypoints++;

						waypoints[index].origin = oldwaypoints_ver[0].origin;

						// to prevent unknown waypoint flag which cause weird things like
						// aim & cross connections going to map origin etc.
						if (oldwaypoints_ver[0].flags == 0)
							waypoints[index].flags |= W_FL_DELETED;
						// otherwise use stored flag
						else
							waypoints[index].flags = oldwaypoints_ver[0].flags;

						waypoints[index].red_priority = oldwaypoints_ver[0].red_priority;
						waypoints[index].blue_priority = oldwaypoints_ver[0].blue_priority;
						waypoints[index].red_time = oldwaypoints_ver[0].red_time;
						waypoints[index].blue_time = oldwaypoints_ver[0].blue_time;
						waypoints[index].trigger_red_priority = oldwaypoints_ver[0].trigger_red_priority;
						waypoints[index].trigger_blue_priority = oldwaypoints_ver[0].trigger_blue_priority;
						waypoints[index].trigger_event_on = oldwaypoints_ver[0].trigger_event_on;
						waypoints[index].trigger_event_off = oldwaypoints_ver[0].trigger_event_off;
						waypoints[index].range = oldwaypoints_ver[0].range;
					}

					// fix possible problems with special waypoint types
					if ((waypoints[index].flags & W_FL_AIMING) && (waypoints[index].range != 0))
						waypoints[index].range = (float) 0;
					
					if (((waypoints[index].flags & W_FL_LADDER) ||
						(waypoints[index].flags & W_FL_DOOR) ||
						(waypoints[index].flags & W_FL_DOORUSE)) &&
						(waypoints[index].range > 20))
						waypoints[index].range = (float) 20;

					// reset range to default if it is too big
					if (waypoints[index].range > MAX_WPT_DIST)
						waypoints[index].range = WPT_RANGE;

					// fix some problem waypoint tag/flag combinations ...
					WaypointFixInvalidFlags(index);

					// finaly convert old/obsolete things
					
					// !!! There's nothing to be converted now
					//  (i.e. no critical changes between version 6 and 7)
				}

				// read & convert the triggers
				for (int index = 0; index < MAX_TRIGGERS; index++)
				{
					// the version 6 triggers used different structure
					// so we have to convert them to version 7 system

					fread(&old_triggers[0], sizeof(old_triggers[0]), 1, bfp);

					trigger_events[index].name = old_triggers[0].name;
					strcpy(trigger_events[index].message, old_triggers[0].message);

					// we have to mark this slot as a used
					if (strlen(trigger_events[index].message) > 1)
						trigger_gamestate[index].SetUsed(true);
				}
				
				// fix missing signatures
				header.author[31] = 0;
				if (strcmp(header.author, "") == 0)
					strcpy(wpt_author, "unknown");
				else
					strcpy(wpt_author, header.author);

				header.modified_by[31] = 0;
				if (strcmp(header.modified_by, "") == 0)
					strcpy(wpt_modified, "unknown");
				else
					strcpy(wpt_modified, header.modified_by);

			}
			else
			{
				sprintf(msg, "MarineBot waypoints are not for this map! %s\n", filename);
				PrintOutput(pEntity, msg, MType::msg_warning);
				
				sprintf(msg, "Bots will not play well this map!\n");
				PrintOutput(pEntity, msg, MType::msg_info);

				fclose(bfp);
				return FALSE;
			}
		}
		else
		{
			sprintf(msg, "Not MarineBot waypoint file! %s\n", filename);
			PrintOutput(pEntity, msg, MType::msg_warning);
				
			sprintf(msg, "Bots will not play well this map!\n");
			PrintOutput(pEntity, msg, MType::msg_info);

			fclose(bfp);
			return FALSE;
		}

		fclose(bfp);
	}
	else
	{
		sprintf(msg, "Waypoint file not found! %s\n", filename);
		PrintOutput(pEntity, msg, MType::msg_warning);
				
		sprintf(msg, "Bots will not play well this map!\n");
		PrintOutput(pEntity, msg, MType::msg_info);

		return FALSE;
	}

	return TRUE;
}


bool WaypointLoadVersion5(edict_t *pEntity)
{
	extern bool is_dedicated_server;

// version 5 waypoint structure (needed because there was a change in the structure itself)
typedef struct {
	int		flags = W_FL_DELETED;			// jump, crouch, button, lift, flag, ammo, etc.
	int		red_priority = MAX_WPT_PRIOR;	// 0-5 where 1 have highest priority for red team (0 - no priority, bot ingnores this waypoint)
	float	red_time = 0.0;					// time the bot wait at this wpt for red team
	int		blue_priority = MAX_WPT_PRIOR;	// 0-5 where 1 have highest priority for blue team
	float	blue_time = 0.0;				// time the bot wait at this wpt for blue team
	int		class_pref =0;					// -- NOT USED -- higher than priority based on bot class (NONE-all)
	float	range = WPT_RANGE;				// circle around wpt where bot detects this waypoint
	int		misc = 0;						// additional slot for various things
	Vector	origin = g_vecZero;//Vector(0, 0, 0);		// location of this waypoint in 3D space
} OLD_WAYPOINT_STRUCT_VERSION_5;

	char mapname[64];
	char filename[256];
	//OLD_WPT_HDR header;
	WAYPOINT_HDR header;		// can be used current header until any change is done
	char msg[256];
	int index;
	OLD_WAYPOINT_STRUCT_VERSION_5 oldwaypoints_ver[1];	// we need just one slot because we are converting them one by one
	int OLD_WAYPOINT_VERSION_5 = 5;		// we have to "override" standard conversion by sending even older version number
	bool known;

	strcpy(mapname, STRING(gpGlobals->mapname));
	strcat(mapname, ".wpt");

	if (internals.IsCustomWaypoints())
		UTIL_MarineBotFileName(filename, "customwpts", mapname);
	else
		UTIL_MarineBotFileName(filename, "defaultwpts", mapname);

	if (is_dedicated_server)
	{
		sprintf(msg, "loading unsupported waypoints from: %s\n", filename);
		
		PrintOutput(NULL, msg, MType::msg_info);
	}

	FILE *bfp = fopen(filename, "rb");

	// if file exists, read the waypoint structure from it
	if (bfp != NULL)
	{
		fread(&header, sizeof(header), 1, bfp);

		header.filetype[7] = 0;
		if (strcmp(header.filetype, "FAM_bot") == 0)
		{
			if (header.waypoint_file_version == WAYPOINT_VERSION)
			{
				sprintf(msg, "This waypoint file is actual. No need to load it this way.\n");
				PrintOutput(pEntity, msg, MType::msg_info);

				fclose(bfp);
				return FALSE;
			}

			if (header.waypoint_file_version != WAYPOINT_VERSION)
			{
				known = FALSE;

				// convert only waypoints that are one version back
				if (header.waypoint_file_version == OLD_WAYPOINT_VERSION_5)
				{
					// bot know this system
					known = TRUE;

					sprintf(msg, "found known older MarineBot waypoint file (version %d - MB0.8b) - conversion in progress...\n", OLD_WAYPOINT_VERSION_5);
					PrintOutput(pEntity, msg, MType::msg_info);
				}
				// otherwise ignore all other (older) waypoint versions
				else
				{
					sprintf(msg, "unknown waypoint file version (probably too old) - conversion failed!\n");
					PrintOutput(pEntity, msg, MType::msg_error);

					fclose(bfp);
					return FALSE;
				}
			}

			header.mapname[31] = 0;

			if (strcmp(header.mapname, STRING(gpGlobals->mapname)) == 0)
			{
				// remove any existing waypoints
				WaypointInit();

				sprintf(msg, "loading converted waypoint file\n");
				PrintOutput(pEntity, msg, MType::msg_info);

				for (index = 0; index < header.number_of_waypoints; index++)
				{
					// init all new values to default at first
					// this is being done a few lines above,
					// but we do it again just to be completely sure
					WaypointInit(index);

					// just for sure, we don't want to load too old waypoints, because those
					// might have a lot different structure so then we are unable to read
					// correct data from the file
					if (known)
					{		
						// read all data for this waypoint
						fread(&oldwaypoints_ver[0], sizeof(oldwaypoints_ver[0]), 1, bfp);
						num_waypoints++;

						waypoints[index].origin = oldwaypoints_ver[0].origin;

						// to prevent unknown waypoint flag which cause weird things like
						// aim & cross connections going to map origin etc.
						if (oldwaypoints_ver[0].flags == 0)
							waypoints[index].flags |= W_FL_DELETED;
						// otherwise use stored flag
						else
							waypoints[index].flags = oldwaypoints_ver[0].flags;

						waypoints[index].red_priority = oldwaypoints_ver[0].red_priority;
						waypoints[index].blue_priority = oldwaypoints_ver[0].blue_priority;
						waypoints[index].red_time = oldwaypoints_ver[0].red_time;
						waypoints[index].blue_time = oldwaypoints_ver[0].blue_time;
						waypoints[index].range = oldwaypoints_ver[0].range;
					}

					// fix possible problems with special waypoint types
					if ((waypoints[index].flags & W_FL_AIMING) && (waypoints[index].range != 0))
						waypoints[index].range = (float) 0;
					
					if (((waypoints[index].flags & W_FL_LADDER) ||
						(waypoints[index].flags & W_FL_DOOR) ||
						(waypoints[index].flags & W_FL_DOORUSE)) &&
						(waypoints[index].range > 20))
						waypoints[index].range = (float) 20;

					// reset range to default if it is too big
					if (waypoints[index].range > MAX_WPT_DIST)
						waypoints[index].range = WPT_RANGE;

					// fix some problem waypoint tag/flag combinations ...
					// (unsure which of those could have been present in version 5 but the ammobox and goback was for sure)
					WaypointFixInvalidFlags(index);

					// add STD flag to all waypoints except special and normal waypoint
					// otherwise we screw whole waypointing system because now we check for this flag
					if (!(waypoints[index].flags & (W_FL_AIMING | W_FL_CROSS | W_FL_DELETED | W_FL_STD)))
					{
						waypoints[index].flags |= W_FL_STD;
					}
				}
				
				// fix missing signatures
				header.author[31] = 0;
				if (strcmp(header.author, "") == 0)
					strcpy(wpt_author, "unknown");
				else
					strcpy(wpt_author, header.author);

				header.modified_by[31] = 0;
				if (strcmp(header.modified_by, "") == 0)
					strcpy(wpt_modified, "unknown");
				else
					strcpy(wpt_modified, header.modified_by);

			}
			else
			{
				sprintf(msg, "MarineBot waypoints are not for this map! %s\n", filename);
				PrintOutput(pEntity, msg, MType::msg_warning);
				
				sprintf(msg, "Bots will not play well this map!\n");
				PrintOutput(pEntity, msg, MType::msg_info);

				fclose(bfp);
				return FALSE;
			}
		}
		else
		{
			sprintf(msg, "Not MarineBot waypoint file! %s\n", filename);
			PrintOutput(pEntity, msg, MType::msg_warning);
				
			sprintf(msg, "Bots will not play well this map!\n");
			PrintOutput(pEntity, msg, MType::msg_info);

			fclose(bfp);
			return FALSE;
		}

		fclose(bfp);
	}
	else
	{
		sprintf(msg, "Waypoint file not found! %s\n", filename);
		PrintOutput(pEntity, msg, MType::msg_warning);
				
		sprintf(msg, "Bots will not play well this map!\n");
		PrintOutput(pEntity, msg, MType::msg_info);

		return FALSE;
	}

	return TRUE;
}

/*
* autosaves waypoints and paths after given time to special files
*/
bool WaypointAutoSave(void)
{
	// don't save waypoints if we are currently building a path
	// because path save routine automatically stops it
	// so we would break stuff such as auto waypointing for example
	// that would stop creating the path and would only add waypoints
	if (internals.IsPathToContinue())
		return FALSE;

	char custom_name[32];

	// set name for those files
	strcpy(custom_name, "autosave");

	if (WaypointSave(custom_name))
	{
		if (WaypointPathSave(custom_name))
			return TRUE;
	}

	return FALSE;
}


/*
*
* NOTE: NOT USED - PROBABLY USELESS
*
* load raw waypoint data
*/
/*
bool WaypointRawLoad(edict_t *pEntity, bool flags, bool priority, bool time, bool class_preference)
{
	extern bool is_dedicated_server;

	char mapname[64];
	char filename[256];
	RAW_FILE_HDR header;
	char msg[80];
	int index;

	strcpy(mapname, STRING(gpGlobals->mapname));
	strcat(mapname, ".raw");

	if (internals.IsCustomWaypoints())
		UTIL_MarineBotFileName(filename, "customwpts", mapname);
	else
		UTIL_MarineBotFileName(filename, "defaultwpts", mapname);

	if (is_dedicated_server)
		printf("loading raw file: %s\n", filename);

	FILE *bfp = fopen(filename, "rb");

	// if file exists, read the waypoint structure from it
	if (bfp != NULL)
	{
		fread(&header, sizeof(header), 1, bfp);

		header.filetype[7] = 0;
		if (strcmp(header.filetype, "FAM_bot") == 0)
		{
			if (header.waypoint_file_version != WAYPOINT_VERSION)
			{
				if (pEntity)
					ClientPrint(pEntity, HUD_PRINTNOTIFY,
					"Older MarineBot waypoint file version!\n");
			}

			header.mapname[31] = 0;

			if (strcmp(header.mapname, STRING(gpGlobals->mapname)) == 0)
			{
				WaypointInit();  // remove any existing waypoints

				for (index = 0; index < header.number_of_waypoints; index++)
				{
					// at first init all new values to default
					waypoints[index].red_priority = MAX_WPT_PRIOR;
					waypoints[index].red_time = 0.0;
					waypoints[index].blue_priority = MAX_WPT_PRIOR;
					waypoints[index].blue_time = 0.0;
					waypoints[index].class_pref = FA_CLASS_NONE;
					waypoints[index].range = WPT_RANGE;
					waypoints[index].misc = 0;

					// read waypoint data from .raw file
					fread(&waypoints[index].origin, sizeof(waypoints[0].origin), 1, bfp);

					if (flags)
						fread(&waypoints[index].flags, sizeof(waypoints[0].flags), 1, bfp);
					if (priority)
					{
						fread(&waypoints[index].red_priority, sizeof(waypoints[0].red_priority), 1, bfp);
						fread(&waypoints[index].blue_priority, sizeof(waypoints[0].blue_priority), 1, bfp);
					}
					if (time)
					{
						fread(&waypoints[index].red_time, sizeof(waypoints[0].red_time), 1, bfp);
						fread(&waypoints[index].blue_time, sizeof(waypoints[0].blue_time), 1, bfp);
					}
					if (class_preference)
						fread(&waypoints[index].class_pref, sizeof(waypoints[0].class_pref), 1, bfp);
					num_waypoints++;

					/*	Already init at top so keep this only for sure and after successful test delete this
					// init all new values to default
					if ((flags == FALSE) && (priority == FALSE) && (time == FALSE) &&
						(class_preference == FALSE))
					{
						waypoints[index].flags = W_FL_STD;
						waypoints[index].red_priority = MAX_WPT_PRIOR;
						waypoints[index].red_time = 0.0;
						waypoints[index].blue_priority = MAX_WPT_PRIOR;
						waypoints[index].blue_time = 0.0;
						waypoints[index].class_pref = FA_CLASS_NONE;
					}
					// init flags and priority
					if ((flags == FALSE) && (priority == FALSE))
					{
						waypoints[index].flags = W_FL_STD;
						waypoints[index].red_priority = MAX_WPT_PRIOR;
						waypoints[index].blue_priority = MAX_WPT_PRIOR;
					}
					// init flags and time
					if ((flags == FALSE) && (time == FALSE))
					{
						waypoints[index].flags = W_FL_STD;
						waypoints[index].red_time = 0.0;
						waypoints[index].blue_time = 0.0;
					}
					// init flags and class_preference
					if ((flags == FALSE) && (class_preference == FALSE))
					{
						waypoints[index].flags = W_FL_STD;
						waypoints[index].class_pref = FA_CLASS_NONE;
					}
					// init priority and time and class_preference
					if ((priority == FALSE) && (time == FALSE) && (class_preference == FALSE))
					{
						waypoints[index].red_priority = MAX_WPT_PRIOR;
						waypoints[index].red_time = 0.0;
						waypoints[index].blue_priority = MAX_WPT_PRIOR;
						waypoints[index].blue_time = 0.0;
						waypoints[index].class_pref = FA_CLASS_NONE;
					}
					// init priority and time
					if ((priority == FALSE) && (time == FALSE))
					{
						waypoints[index].red_priority = MAX_WPT_PRIOR;
						waypoints[index].red_time = 0.0;
						waypoints[index].blue_priority = MAX_WPT_PRIOR;
						waypoints[index].blue_time = 0.0;
					}
					// init priority and class_preference
					if ((priority == FALSE) && (class_preference == FALSE))
					{
						waypoints[index].red_priority = MAX_WPT_PRIOR;
						waypoints[index].blue_priority = MAX_WPT_PRIOR;
						waypoints[index].class_pref = FA_CLASS_NONE;
					}
					// init time and class_preference
					if ((time == FALSE) && (class_preference))
					{
						waypoints[index].red_time = 0.0;
						waypoints[index].blue_time = 0.0;
						waypoints[index].class_pref = FA_CLASS_NONE;
					}
					// init only flags
					if (flags == FALSE)
					{
						waypoints[index].flags = W_FL_STD;
					}
					// init only priority
					if (priority == FALSE)
					{
						waypoints[index].red_priority = MAX_WPT_PRIOR;
						waypoints[index].blue_priority = MAX_WPT_PRIOR;
					}
					// init only time
					if (time == FALSE)
					{
						waypoints[index].red_time = 0.0;
						waypoints[index].blue_time = 0.0;
					}
					// init only class_preference
					if (class_preference == FALSE)
					{
						waypoints[index].class_pref = FA_CLASS_NONE;
					}*//*
				}

				header.author[31] = 0;
				// if no author so set it to unknown
				if (strcmp(header.author, "") == 0)
					strcpy(wpt_author, "unknown");
				else
					strcpy(wpt_author, header.author);

				header.modified_by[31] = 0;
				// if not specified guy who modify wpts so set it to unknown
				if (strcmp(header.modified_by, "") == 0)
					strcpy(wpt_modified, "unknown");
				else
					strcpy(wpt_modified, header.modified_by);
			}
			else
			{
				if (pEntity)
				{
					sprintf(msg, "%s MarineBot waypoints are not for this map!\n", filename);
					ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
				}

				fclose(bfp);
				return FALSE;
			}
/*
			header.author[31] = 0;
			// if no author so set it to unknown
			if (strcmp(header.author, "") == 0)
				strcpy(wpt_author, "unknown");
			else
				strcpy(wpt_author, header.author);

			header.modified_by[31] = 0;
			// if not specified guy who modify wpts so set it to unknown
			if (strcmp(header.modified_by, "") == 0)
				strcpy(wpt_modified, "unknown");
			else
				strcpy(wpt_modified, header.modified_by);*//*
		}
		else
		{
			if (pEntity)
			{
				sprintf(msg, "%s is not a MarineBot waypoint file!\n", filename);
				ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
			}

			fclose(bfp);
			return FALSE;
		}

		fclose(bfp);
	}
	else
	{
		if (pEntity)
		{
			sprintf(msg, "Waypoint file %s does not exist!\n", filename);
			ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);
		}

		if (is_dedicated_server)
			printf("waypoint file %s not found!\n", filename);

		return FALSE;
	}

	return TRUE;
}
*/

/*
*
* NOTE: NOT USED - PROBABLY USELESS
*
* save raw waypoint data
*/
/*
void WaypointRawSave(bool flags, bool priority, bool time, bool class_preference)
{
	char filename[256];
	char mapname[64];
	RAW_FILE_HDR header;
	int index;

	strcpy(header.filetype, "FAM_bot");

	header.waypoint_file_version = WAYPOINT_VERSION;

	header.transfer_flags = (int)flags;

	header.transfer_priority = (int)priority;

	header.transfer_time = (int)time;

	header.transfer_class = (int)class_preference;	

	header.number_of_waypoints = num_waypoints;

	memset(header.mapname, 0, sizeof(header.mapname));
	strncpy(header.mapname, STRING(gpGlobals->mapname), 31);
	header.mapname[31] = 0;

	// write authors signature
	memset(header.author, 0, sizeof(header.author));
	if (wpt_author[0] == 0)
		strncpy(header.author, "unknown", 31);
	else
		strncpy(header.author, wpt_author, 31);
	header.author[31] = 0;

	// write the one who modified them
	memset(header.modified_by, 0, sizeof(header.modified_by));
	if (wpt_modified[0] == 0)
		strncpy(header.modified_by, "unknown", 31);
	else
		strncpy(header.modified_by, wpt_modified, 31);
	header.modified_by[31] = 0;

	strcpy(mapname, STRING(gpGlobals->mapname));
	strcat(mapname, ".raw");

	if (internals.IsCustomWaypoints())
		UTIL_MarineBotFileName(filename, "customwpts", mapname);
	else
		UTIL_MarineBotFileName(filename, "defaultwpts", mapname);

	FILE *bfp = fopen(filename, "wb");

	// write the waypoint header to the file
	fwrite(&header, sizeof(header), 1, bfp);

	// write the waypoint data to the file...
	for (index=0; index < num_waypoints; index++)
	{
		fwrite(&waypoints[index].origin, sizeof(waypoints[0].origin), 1, bfp);
		if (flags)
			fwrite(&waypoints[index].flags, sizeof(waypoints[0].flags), 1, bfp);
		if (priority)
		{
			fwrite(&waypoints[index].red_priority, sizeof(waypoints[0].red_priority), 1, bfp);
			fwrite(&waypoints[index].blue_priority, sizeof(waypoints[0].blue_priority), 1, bfp);
		}
		if (time)
		{
			fwrite(&waypoints[index].red_time, sizeof(waypoints[0].red_time), 1, bfp);
			fwrite(&waypoints[index].blue_time, sizeof(waypoints[0].blue_time), 1, bfp);
		}
		if (class_preference)
			fwrite(&waypoints[index].class_pref, sizeof(waypoints[0].class_pref), 1, bfp);
	}
	
	fclose(bfp);
}
*/

/*
* returns the wpt system version this file was created in
*/
int WaypointGetSystemVersion(void)
{
	char mapname[64];
	char filename[256];
	WAYPOINT_HDR header;

	strcpy(mapname, STRING(gpGlobals->mapname));
	strcat(mapname, ".wpt");

	if (internals.IsCustomWaypoints())
		UTIL_MarineBotFileName(filename, "customwpts", mapname);
	else
		UTIL_MarineBotFileName(filename, "defaultwpts", mapname);

	FILE *bfp = fopen(filename, "rb");

	// if file exists, read only the header info from it
	if (bfp != NULL)
	{
		fread(&header, sizeof(header), 1, bfp);

		header.filetype[7] = 0;
		if (strcmp(header.filetype, "FAM_bot") == 0)
		{
			int version;

			version = header.waypoint_file_version;

			fclose(bfp);

			return version;
		}
	}

	return -1;
}

/*
* checks if waypoint is reachable
*/
bool WaypointReachable(Vector v_src, Vector v_dest, edict_t *pEntity)
{
   TraceResult tr;
   float curr_height, last_height;

   float distance = (v_dest - v_src).Length();

   // is the destination close enough?
   if (distance < MAX_WPT_DIST)
   {
      // check if this waypoint is "visible"...

      UTIL_TraceLine( v_src, v_dest, ignore_monsters, pEntity->v.pContainingEntity, &tr );

      // if waypoint is visible from current position (even behind head)...
      if (tr.flFraction >= 1.0)
      {
         // check for special case of both waypoints being underwater...
         if ((POINT_CONTENTS( v_src ) == CONTENTS_WATER) && (POINT_CONTENTS( v_dest ) == CONTENTS_WATER))
         {
            return TRUE;
         }

         // check for special case of waypoint being suspended in mid-air...

         // is dest waypoint higher than src? (45 is max jump height)
         if (v_dest.z > (v_src.z + 45.0))
         {
            Vector v_new_src = v_dest;
            Vector v_new_dest = v_dest;

            v_new_dest.z = v_new_dest.z - 50;  // straight down 50 units

            UTIL_TraceLine(v_new_src, v_new_dest, dont_ignore_monsters, pEntity->v.pContainingEntity, &tr);

            // check if we didn't hit anything, if not then it's in mid-air
            if (tr.flFraction >= 1.0)
            {
               return FALSE;  // can't reach this one
            }
         }

         // check if distance to ground increases more than jump height
         // at points between source and destination...

         Vector v_direction = (v_dest - v_src).Normalize();  // 1 unit long
         Vector v_check = v_src;
         Vector v_down = v_src;

         v_down.z = v_down.z - 1000.0;  // straight down 1000 units

         UTIL_TraceLine(v_check, v_down, ignore_monsters, pEntity->v.pContainingEntity, &tr);

         last_height = tr.flFraction * 1000.0;  // height from ground

         distance = (v_dest - v_check).Length();  // distance from goal

         while (distance > 10.0)
         {
            // move 10 units closer to the goal...
            v_check = v_check + (v_direction * 10.0);

            v_down = v_check;
            v_down.z = v_down.z - 1000.0;  // straight down 1000 units

            UTIL_TraceLine(v_check, v_down, ignore_monsters, pEntity->v.pContainingEntity, &tr);

            curr_height = tr.flFraction * 1000.0;  // height from ground

            // is the difference in the last height and the current height
            // higher that the jump height?
            if ((last_height - curr_height) > (float)45.0)
            {
               // can't get there from here...
               return FALSE;
            }

            last_height = curr_height;

            distance = (v_dest - v_check).Length();  // distance from goal
         }

         return TRUE;
      }
   }

   return FALSE;
}

/*
* returns the type of waypoint passed in by wpt_index
* return "unknown" if waypoint is invalid or there is unknown flag
*/
void WptGetType(int wpt_index, char *the_type)
{
	int flags;

	// just in case
	if (wpt_index == -1)
	{
		strcpy(the_type, "unknown");
		return;
	}

	// get the waypoint flag
	flags = waypoints[wpt_index].flags;

	// init this string
	strcpy(the_type, "");

	if (flags & W_FL_SNIPER)
		strcat(the_type, "sniper ");
	if (flags & W_FL_FIRE)
		strcat(the_type, "shoot ");
	if (flags & W_FL_AIMING)
		strcat(the_type, "aim ");
	if (flags & W_FL_AMMOBOX)
		strcat(the_type, "ammobox ");
	if (flags & W_FL_CHUTE)
		strcat(the_type, "parachute ");
	if (flags & W_FL_CROSS)
		strcat(the_type, "cross ");
	if (flags & W_FL_CROUCH)
		strcat(the_type, "crouch ");
	if (flags & W_FL_DOOR)
		strcat(the_type, "door ");
	if (flags & W_FL_DOORUSE)
		strcat(the_type, "usedoor ");
	if (flags & W_FL_GOBACK)
		strcat(the_type, "goback ");
	if (flags & W_FL_JUMP)
		strcat(the_type, "jump ");
	if (flags & W_FL_DUCKJUMP)
		strcat(the_type, "duckjump ");
	if (flags & W_FL_LADDER)
		strcat(the_type, "ladder ");
	if (flags & W_FL_MINE)
		strcat(the_type, "claymore ");
	if (flags & W_FL_PRONE)
		strcat(the_type, "prone ");
	if (flags & W_FL_PUSHPOINT)
		strcat(the_type, "flag ");
	if (flags & W_FL_SPRINT)
		strcat(the_type, "sprint ");
	if (flags & W_FL_TRIGGER)
		strcat(the_type, "trigger ");
	if (flags & W_FL_USE)
		strcat(the_type, "use ");

	if (flags & W_FL_STD)
	{
		// if this is a crouch or prone waypoint don't print this flag
		if (flags & (W_FL_CROUCH | W_FL_PRONE))
			;
		else
			strcat(the_type, "normal ");
	}

	if (flags & W_FL_DELETED)
		strcat(the_type, "deleted/erased ");

	// if there are no flags on this waypoint set unknown flag
	if (Wpt_CountFlags(wpt_index) < 1)
		strcat(the_type, "unknown ");

	// cut the space at the end of the string
	int length = strlen(the_type);
	the_type[length-1] = '\0';

	return;
}

/*
* prints info about waypoint into the console
* if info_level == 0 it prints only important info for that waypoint
* if info_level == 1 it prints all except comment (what the bot does there)
* if info_level == 2 it prints all and also the comment
*/
void WaypointPrintInfo(edict_t *pEntity, const char *arg2, const char *arg3)
{
	char msg[256], special_info_r[80], special_info_b[80], comment[512];
	char steam_msg[512];
	int index, flags, red_prior, blue_prior, info_level;
	float red_time, blue_time, range;

	// we have to initialize these two first
	index = -1;
	info_level = 0;

	// now check the arguments for data
	if ((arg2 != NULL) && (*arg2 != 0))
	{
		if (FStrEq(arg2, "help") || FStrEq(arg2, "?"))
		{
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "wpt info <arg1> <arg2> where arg could be either waypoint index or 'more' or 'full'\n");
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "'wpt info 5 more' will print additional info about waypoint no. 5\n'wpt info more 5' will do exactly the same\n");
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "'wpt info more' will print additional information about any waypoint, but you must stand realy close to it\n");
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "'wpt info full 5' will print all information about waypoint no. 5 including short description of what the bot will do there\n");
		}
		else if (FStrEq(arg2, "full"))
			info_level = 2;
		else if (FStrEq(arg2, "more"))
			info_level = 1;
		else
			index = atoi(arg2) - 1;
	}

	if ((arg3 != NULL) && (*arg3 != 0))
	{
		if (FStrEq(arg3, "full"))
			info_level = 2;
		else if (FStrEq(arg3, "more"))
			info_level = 1;
		else
			index = atoi(arg3) - 1;
	}

	// if neither argument passed the waypoint index
	// try to find a waypoint nearby
	if ((index < 0) || (index >= num_waypoints))
		index = WaypointFindNearest(pEntity, 50.0, -1);

	// we still got no waypoint so we have nothing to print
	if (index == -1)
		return;

	// get all data out of the current waypoint
	flags = waypoints[index].flags;
	red_prior = waypoints[index].red_priority;
	red_time = waypoints[index].red_time;
	blue_prior = waypoints[index].blue_priority;
	blue_time = waypoints[index].blue_time;
	range = waypoints[index].range;

	// init the comment
	strcpy(comment, "");

	if (flags & W_FL_AIMING)
		strcat(comment, "aim to this wpt location");
	else if (flags & W_FL_CROSS)
		strcat(comment, "choose next waypoint to continue");
	else
	{
		// is range small AND NOT aim or cross waypoint
		if (range < RANGE_SMALL)
		{
			strcat(comment, "slow down");
		}
		
		if (flags & W_FL_AMMOBOX)
		{
			if (strlen(comment) > 1)
				strcat(comment, " & ");
			strcat(comment, "take the ammo from ammobox");
		}
		if (flags & W_FL_CHUTE)
		{
			if (strlen(comment) > 1)
				strcat(comment, " & ");
			strcat(comment, "check for parachute gate");
		}
		if (flags & W_FL_CROUCH)
		{
			if (strlen(comment) > 1)
				strcat(comment, " & ");
			strcat(comment, "crouch and continue");
		}
		if (flags & W_FL_DOOR)
		{
			if (strlen(comment) > 1)
				strcat(comment, " & ");
			strcat(comment, "handle the door");
		}
		if (flags & W_FL_DOORUSE)
		{
			if (strlen(comment) > 1)
				strcat(comment, " & ");
			strcat(comment, "use the door");
		}
		if (flags & W_FL_GOBACK)
		{
			if (strlen(comment) > 1)
				strcat(comment, " & ");
			strcat(comment, "turn back and continue in opposite direction");
		}
		if (flags & W_FL_JUMP)
		{
			if (strlen(comment) > 1)
				strcat(comment, " & ");
			strcat(comment, "jump out of hole or up to an object here");
		}
		if (flags & W_FL_DUCKJUMP)
		{
			if (strlen(comment) > 1)
				strcat(comment, " & ");
			strcat(comment, "use duckjump here");
		}
		if (flags & W_FL_LADDER)
		{
			if (strlen(comment) > 1)
				strcat(comment, " & ");
			strcat(comment, "climb the ladder");
		}
		if (flags & W_FL_MINE)
		{
			if (strlen(comment) > 1)
				strcat(comment, " & ");
			strcat(comment, "plant a claymore mine here");
		}
		if (flags & W_FL_PRONE)
		{
			if (strlen(comment) > 1)
				strcat(comment, " & ");
			strcat(comment, "go prone and continue");
		}
		if (flags & W_FL_PUSHPOINT)
		{
			if (strlen(comment) > 1)
				strcat(comment, " & ");
			strcat(comment, "get the flag here");
		}
		if (flags & W_FL_SPRINT)
		{
			if (strlen(comment) > 1)
				strcat(comment, " & ");
			strcat(comment, "sprint from here");
		}
		if (flags & W_FL_TRIGGER)
		{
			if (strlen(comment) > 1)
				strcat(comment, " & ");
			strcat(comment, "check priority");
		}
		if (flags & W_FL_USE)
		{
			if (strlen(comment) > 1)
				strcat(comment, " & ");
			strcat(comment, "use button, mounted gun etc.");
		}
		// these should be last
		if (flags & W_FL_FIRE)
		{
			if (strlen(comment) > 1)
				strcat(comment, " & ");
			strcat(comment, "fire the weapon");
		}
		if (flags & W_FL_SNIPER)
		{
			if (strlen(comment) > 1)
				strcat(comment, " & ");
			strcat(comment, "don't move during combat");
		}
		if (flags & W_FL_DELETED)
		{
			if (strlen(comment) > 1)
				strcat(comment, " & ");
			strcat(comment, "deleted waypoint will be ignored");
		}
		if (flags & W_FL_STD)
		{
			// print this only if there's no other flag on this waypoint
			if (strlen(comment) < 1)
				strcat(comment, "just pass around");
		}
	}

	// finalize the comment string
	strcat(comment, "\n");

	// print the info
	char wpt_flags[256];
	WptGetType(index, wpt_flags);

	sprintf(msg,"Waypoint %d of %d total (flags/tags= %s)\n", index + 1, num_waypoints, wpt_flags);

	// prepare additional steam message
	if (is_steam)
		strcpy(steam_msg, msg);

	ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

	// print this only if info_level == 0
	if ((flags & W_FL_CROSS) && (info_level == 0))
	{
		sprintf(special_info_r, "\tWpt range= %.1f\n", range);

		ClientPrint(pEntity, HUD_PRINTNOTIFY, special_info_r);

		if (is_steam)
			strcat(steam_msg, special_info_r);
	}
	// print this only if info_level == 0
	else if (((flags & W_FL_AIMING) || (flags & W_FL_CHUTE)) && (info_level == 0))
	{
		if (red_prior == 0)
			sprintf(special_info_r,"\tRed Team priority(1-highest)= %d   !NO PRIORITY!\n", red_prior);
		else
			sprintf(special_info_r,"\tRed Team priority(1-highest)= %d\n", red_prior);
		if (blue_prior == 0)
			sprintf(special_info_b,"\tBlue Team priority(1-highest)= %d  !NO PRIORITY!\n", blue_prior);
		else
			sprintf(special_info_b,"\tBlue Team priority(1-highest)= %d\n", blue_prior);

		if (is_steam)
		{
			strcat(steam_msg, special_info_r);
			strcat(steam_msg, special_info_b);
		}

		ClientPrint(pEntity, HUD_PRINTNOTIFY, special_info_r);
		ClientPrint(pEntity, HUD_PRINTNOTIFY, special_info_b);
	}
	else
	{
		if (red_prior == 0)
			sprintf(special_info_r,"\tRed Team priority(1-highest)= %d \"wait/guard\" time=%.1f  !NO PRIORITY!\n", red_prior, red_time);
		else
			sprintf(special_info_r,"\tRed Team priority(1-highest)= %d \"wait/guard\" time=%.1f\n", red_prior, red_time);
		if (blue_prior == 0)
			sprintf(special_info_b,"\tBlue Team priority(1-highest)= %d \"wait/guard\" time=%.1f  !NO PRIORITY!\n", blue_prior, blue_time);
		else
			sprintf(special_info_b,"\tBlue Team priority(1-highest)= %d \"wait/guard\" time=%.1f\n", blue_prior, blue_time);

		if (is_steam)
		{
			strcat(steam_msg, special_info_r);
			strcat(steam_msg, special_info_b);
		}

		ClientPrint(pEntity, HUD_PRINTNOTIFY, special_info_r);
		ClientPrint(pEntity, HUD_PRINTNOTIFY, special_info_b);
	}

	// print this if info_level == 1 or higher
	if ((info_level == 1) || (info_level == 2))
	{
		sprintf(special_info_r,"\tWpt range= %.1f\n", range);

		ClientPrint(pEntity, HUD_PRINTNOTIFY, special_info_r);

		if (is_steam)
			strcat(steam_msg, special_info_r);
	}

	// print this only if info_level == 2 (complete info)
	if (info_level == 2)
		ClientPrint(pEntity, HUD_PRINTNOTIFY, comment);

	if (is_steam)
	{
		// "clean" the console so its text doesn't collide with the HUD message
		ClientPrint(pEntity, HUD_PRINTNOTIFY, "\n");
		// display it also on screen using the HUD message if we are on steam
		DisplayMsg(pEntity, steam_msg);
	}
}

/*
* draws a simple beam connecting player origin and waypoint origin specified in argument
* handles also turning the feature off either by off argument or no argument
*/
bool WaypointCompass(edict_t *pEntity, const char *arg2)
{
	int wpt_index = -1;

	if ((arg2 != NULL) && (*arg2 != 0))
	{
		if (FStrEq(arg2, "off"))
		{
			wptser.ResetCompassIndex();

			ClientPrint(pEntity, HUD_PRINTCONSOLE, "waypoint compass turned off\n");
			return TRUE;
		}

		// due to array based style
		wpt_index = atoi(arg2) - 1;

		if ((wpt_index < 0) || (wpt_index > num_waypoints))
		{
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "invalid waypoint index!\n");
			return FALSE;
		}

		if (waypoints[wpt_index].flags & W_FL_DELETED)
		{
			ClientPrint(pEntity, HUD_PRINTCONSOLE, "cannot guide you to deleted waypoint!\n");
			return FALSE;
		}

		// now everything seems to be valid so we can turn it on
		wptser.SetCompassIndex(wpt_index);
		f_compass_time = 0.0;

		// turn these two on as well so the user can see things right away
		wptser.SetShowWaypoints(true);
		wptser.SetShowPaths(true);

		// but turn these off to prevent overloading game engine which then makes the waypoints and all
		// these beams to flicker
		wptser.ResetCheckAims();
		wptser.ResetCheckCross();
		wptser.ResetCheckRanges();

		return TRUE;
	}
	else
	{
		ClientPrint(pEntity, HUD_PRINTCONSOLE, "invalid or missing argument!\n");
	}

	return FALSE;
}

/*
* lists through all waypoints printing their index and their flags/tags
*/
void  WaypointPrintAllWaypoints(edict_t *pEntity)
{
	char msg[256];
	char wpt_flags[242];

	static int printed = 0;
	int right_now = 0;

	if (printed == 0)
		ClientPrint(pEntity, HUD_PRINTCONSOLE, "Printing all used waypoints ...\n");
	
	for (int index = printed; index < num_waypoints; index++)
	{
		WptGetType(index, wpt_flags);

		sprintf(msg, "Waypoint no. %d and its flags/tags= %s\n", index + 1, wpt_flags);
		ClientPrint(pEntity, HUD_PRINTCONSOLE, msg);

		printed++;
		right_now++;

		// did we print 22 lines of text in the console yet?
		// if so then stop it and wait for next call
		if (right_now == 22)
			return;
	}

	// we must have printed all waypoints so reset the static "memory"
	printed = 0;

	return;
}

/*
* prints info about trigger waypoint into the console
*/
void WaypointTriggerPrintInfo(edict_t *pEntity)
{
	char msg[256], special_info_r[80], special_info_b[80];
	char steam_msg[512];
	int index, flags, red_prior, blue_prior, trigger_red, trigger_blue, trigger_on_id, trigger_off_id;
	char trigger_on[32], trigger_off[32];

	// find the nearest waypoint...
	index = WaypointFindNearestType(pEntity, 50.0, W_FL_TRIGGER);

	if (index == -1)
		return;

	// get the data out of the waypoint
	flags = waypoints[index].flags;
	red_prior = waypoints[index].red_priority;
	blue_prior = waypoints[index].blue_priority;
	trigger_red = waypoints[index].trigger_red_priority;
	trigger_blue = waypoints[index].trigger_blue_priority;
	trigger_on_id = waypoints[index].trigger_event_on;
	trigger_off_id = waypoints[index].trigger_event_off;

	// print the info
	char wpt_flags[256];
	WptGetType(index, wpt_flags);

	sprintf(msg,"Waypoint %d of %d total (flags/tags= %s)\n", index + 1, num_waypoints, wpt_flags);

	// prepare additional steam message
	if (is_steam)
		strcpy(steam_msg, msg);

	ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

	if ((red_prior == 0) || (trigger_red == 0))
		sprintf(special_info_r,"\tRed Team priority(1-highest)= %d trigger priority= %d  !NO PRIORITY!\n", red_prior, trigger_red);
	else
		sprintf(special_info_r,"\tRed Team priority(1-highest)= %d trigger priority= %d\n", red_prior, trigger_red);
	if ((blue_prior == 0) || (trigger_blue == 0))
		sprintf(special_info_b,"\tBlue Team priority(1-highest)= %d trigger priority= %d  !NO PRIORITY!\n", blue_prior, trigger_blue);
	else
		sprintf(special_info_b,"\tBlue Team priority(1-highest)= %d trigger priority= %d\n", blue_prior, trigger_blue);

	TriggerIntToName(trigger_on_id, trigger_on);
	TriggerIntToName(trigger_off_id, trigger_off);
	sprintf(msg, "\tTrigger on event=\"%s\" trigger off event=\"%s\"\n", trigger_on, trigger_off);

	ClientPrint(pEntity, HUD_PRINTNOTIFY, special_info_r);
	ClientPrint(pEntity, HUD_PRINTNOTIFY, special_info_b);
	ClientPrint(pEntity, HUD_PRINTNOTIFY, msg);

	if (is_steam)
	{
		strcat(steam_msg, special_info_r);
		strcat(steam_msg, special_info_b);
		strcat(steam_msg, msg);

		ClientPrint(pEntity, HUD_PRINTNOTIFY, "\n");
		// display the HUD message
		DisplayMsg(pEntity, steam_msg);
	}
}

/*
* changes flag on the nearest waypoint form current to
* the one specified in new_type (ie arg2)
* or if not specified it tooks next type from array of ordered types
* returns "no" if something went wrong (ie no close waypoint etc.)
* returns "false" if waypoint flag is invalid
* returns new waypoint flag if everything is OK
*/
char *WptFlagChange(edict_t *pEntity, const char *new_type)
{
	int index;
	char *output = "no";

	if ((new_type == NULL) || (*new_type == 0))
		return "false";

	index = WaypointFindNearest(pEntity, 50.0, -1);

	if (index == -1)
		return output;

	// handle special waypoints first
	if (FStrEq(new_type, "aim"))
	{
		// if the aiming flag was on this waypoint remove it
		if (waypoints[index].flags & W_FL_AIMING)
		{
			waypoints[index].flags = 0;			// clear all flags
			waypoints[index].flags |= W_FL_STD;	// and set default flag (ie "reset" the waypoint)
			waypoints[index].range = WPT_RANGE;	// set default waypoint range
			output = "removed";
		}
		// otherwise set it
		else
		{
			waypoints[index].flags = 0;		// reset all flags because this waypoint is special
			waypoints[index].flags |= W_FL_AIMING;	// set only the aiming flag
			waypoints[index].red_time = 0;			// reset also both wait times
			waypoints[index].blue_time = 0;
			waypoints[index].range = (float) 0;		// reset also range
			output = (char *)new_type;
		}

		// we need to immediately leave this method when special waypoint is done
		// due to it's speciality (no STD flag)
		return output;
	}
	else if (FStrEq(new_type, "cross"))
	{
		if (waypoints[index].flags & W_FL_CROSS)
		{
			waypoints[index].flags = 0;
			waypoints[index].flags |= W_FL_STD;
			waypoints[index].range = WPT_RANGE;
			output = "removed";
		}
		else
		{
			waypoints[index].flags = 0;
			waypoints[index].flags |= W_FL_CROSS;
			waypoints[index].red_time = 0;
			waypoints[index].blue_time = 0;
			waypoints[index].range = WPT_RANGE * (float) 3;	// set the range to cross default
			output = (char *)new_type;

			//																						NEW CODE 094 END

			// see if this waypoint is part of any path
			int in_path = FindPath(index);
			
			// convert this waypoint index to a string value
			char wpt_index_as_char[6];
			sprintf(wpt_index_as_char, "%d", index + 1);

			while (in_path != -1)
			{
				char path_index_as_char[6];
				sprintf(path_index_as_char, "%d", in_path + 1);

				// first try to split the path on this waypoint
				WaypointSplitPath(pEntity, path_index_as_char, wpt_index_as_char);

				// no matter how the splitting went
				// (even if it was successfully splitted this waypoint is still inside this path as its last waypoint)
				// exclude this waypoint from this path
				ExcludeFromPath(index, in_path);

				// and check if there is yet another path on this waypoint
				in_path = FindPath(index);
			}

			// if we are autowaypointing ...
			if (wptser.IsAutoWaypointing())
			{
				// change its range to match the autowaypointing distance
				// in order to reach (be able to connect to) normal waypoint/s added by the autowaypointing
				waypoints[index].range = wptser.GetAutoWaypointingDistance() + (float) 10;
			}
			//************************************************************************************	NEW CODE 094 END
		}

		return output;
	}

	// if we are changing directly from one of these waypoints do "reset" the waypoint
	if (waypoints[index].flags & (W_FL_AIMING | W_FL_CROSS))
	{
		waypoints[index].flags = 0;
		waypoints[index].flags |= W_FL_STD;
		waypoints[index].range = WPT_RANGE;
	}

	// continue with standard waypoint types
	if (FStrEq(new_type, "normal"))
	{
		// allow this to make the waypointing easier
		// user is able to "reset" the waypoint this way
		if (waypoints[index].flags & W_FL_CROUCH)
			waypoints[index].flags &= ~W_FL_CROUCH;
		else if (waypoints[index].flags & W_FL_PRONE)
			waypoints[index].flags &= ~W_FL_PRONE;

		waypoints[index].flags |= W_FL_STD;

		output = (char *)new_type;
	}

	else if (FStrEq(new_type, "ammobox"))
	{
		if (waypoints[index].flags & W_FL_AMMOBOX)
		{
			waypoints[index].flags &= ~W_FL_AMMOBOX;
			output = "removed";
		}
		else
		{
			waypoints[index].flags |= W_FL_AMMOBOX;
			waypoints[index].red_time = 0;
			waypoints[index].blue_time = 0;
			// set smaller range only if range is quite big
			if (waypoints[index].range > 20)
				waypoints[index].range = (float) 20;
			output = (char *)new_type;
		}
	}

	// NO CODE YET AND IT IS PROBABLY USELESS HERE WE WILL DO IT AUTOMATICALLY IN WptAdd()
	//else if (FStrEq(new_type, "bandages"))
	//{
		//waypoints[index].flags = 0;
		//waypoints[index].flags |= W_FL_BANDAGES;
		//output = (char *)new_type;
	//}

	else if (FStrEq(new_type, "parachute"))
	{
		if (waypoints[index].flags & W_FL_CHUTE)
		{
			waypoints[index].flags &= ~W_FL_CHUTE;
			output = "removed";
		}
		else
		{
			// you can't go prone during parachuting
			if (waypoints[index].flags & W_FL_PRONE)
				waypoints[index].flags &= ~W_FL_PRONE;

			if (waypoints[index].flags & W_FL_DOOR)
				waypoints[index].flags &= ~W_FL_DOOR;

			if (waypoints[index].flags & W_FL_DOORUSE)
				waypoints[index].flags &= ~W_FL_DOORUSE;

			if (waypoints[index].flags & W_FL_USE)
				waypoints[index].flags &= ~W_FL_USE;

			waypoints[index].red_time = 0;
			waypoints[index].blue_time = 0;
			waypoints[index].flags |= W_FL_CHUTE;
			output = (char *)new_type;
		}
	}

	else if (FStrEq(new_type, "crouch"))
	{
		if (waypoints[index].flags & W_FL_CROUCH)
		{
			waypoints[index].flags &= ~W_FL_CROUCH;
			output = "removed";
		}
		else
		{
			if (waypoints[index].flags & W_FL_PRONE)
				waypoints[index].flags &= ~W_FL_PRONE;

			waypoints[index].flags |= W_FL_CROUCH;
			output = (char *)new_type;
		}
	}

	else if (FStrEq(new_type, "door"))
	{
		if (waypoints[index].flags & W_FL_DOOR)
		{
			waypoints[index].flags &= ~W_FL_DOOR;
			output = "removed";
		}
		else
		{
			if (waypoints[index].flags & W_FL_DOORUSE)
				waypoints[index].flags &= ~W_FL_DOORUSE;

			if (waypoints[index].flags & W_FL_CHUTE)
				waypoints[index].flags &= ~W_FL_CHUTE;

			waypoints[index].flags |= W_FL_DOOR;
			if (waypoints[index].range > 20)
				waypoints[index].range = (float) 20;
			output = (char *)new_type;
		}
	}

	else if (FStrEq(new_type, "usedoor"))
	{
		if (waypoints[index].flags & W_FL_DOORUSE)
		{
			waypoints[index].flags &= ~W_FL_DOORUSE;
			output = "removed";
		}
		else
		{
			if (waypoints[index].flags & W_FL_DOOR)
				waypoints[index].flags &= ~W_FL_DOOR;

			if (waypoints[index].flags & W_FL_CHUTE)
				waypoints[index].flags &= ~W_FL_CHUTE;

			waypoints[index].flags |= W_FL_DOORUSE;
			if (waypoints[index].range > 20)
				waypoints[index].range = (float) 20;
			output = (char *)new_type;
		}
	}

	else if (FStrEq(new_type, "shoot"))
	{
		if (waypoints[index].flags & W_FL_FIRE)
		{
			waypoints[index].flags &= ~W_FL_FIRE;
			output = "removed";
		}
		else
		{
			waypoints[index].flags |= W_FL_FIRE;
			output = (char *)new_type;
		}
	}

	else if (FStrEq(new_type, "goback"))
	{
		if (waypoints[index].flags & W_FL_GOBACK)
		{
			waypoints[index].flags &= ~W_FL_GOBACK;
			output = "removed";
		}
		else
		{
			waypoints[index].flags |= W_FL_GOBACK;
			output = (char *)new_type;
		}
	}

	else if (FStrEq(new_type, "jump"))
	{
		if (waypoints[index].flags & W_FL_JUMP)
		{
			waypoints[index].flags &= ~W_FL_JUMP;
			output = "removed";
		}
		else
		{
			if (waypoints[index].flags & W_FL_PRONE)
				waypoints[index].flags &= ~W_FL_PRONE;

			if (waypoints[index].flags & W_FL_DUCKJUMP)
				waypoints[index].flags &= ~W_FL_DUCKJUMP;

			waypoints[index].flags |= W_FL_JUMP;
			waypoints[index].red_time = 0;
			waypoints[index].blue_time = 0;
			if (waypoints[index].range > 20)
				waypoints[index].range = (float) 20;
			output = (char *)new_type;
		}
	}

	else if (FStrEq(new_type, "duckjump"))
	{
		if (waypoints[index].flags & W_FL_DUCKJUMP)
		{
			waypoints[index].flags &= ~W_FL_DUCKJUMP;
			output = "removed";
		}
		else
		{
			if (waypoints[index].flags & W_FL_PRONE)
				waypoints[index].flags &= ~W_FL_PRONE;

			if (waypoints[index].flags & W_FL_JUMP)
				waypoints[index].flags &= ~W_FL_JUMP;

			waypoints[index].flags |= W_FL_DUCKJUMP;
			waypoints[index].red_time = 0;
			waypoints[index].blue_time = 0;
			if (waypoints[index].range > 20)
				waypoints[index].range = (float) 20;
			output = (char *)new_type;
		}
	}

	else if (FStrEq(new_type, "ladder"))
	{
		if (waypoints[index].flags & W_FL_LADDER)
		{
			waypoints[index].flags &= ~W_FL_LADDER;
			output = "removed";
		}
		else
		{
			if (waypoints[index].flags & W_FL_PRONE)
				waypoints[index].flags &= ~W_FL_PRONE;

			waypoints[index].flags |= W_FL_LADDER;
			waypoints[index].red_time = 0;
			waypoints[index].blue_time = 0;
			if (waypoints[index].range > 20)
				waypoints[index].range = (float) 20;
			output = (char *)new_type;
		}
	}

	else if (FStrEq(new_type, "claymore"))
	{
		if (waypoints[index].flags & W_FL_MINE)
		{
			waypoints[index].flags &= ~W_FL_MINE;
			output = "removed";
		}
		else
		{
			// you can't place claymores crouched
			if (waypoints[index].flags & W_FL_CROUCH)
				waypoints[index].flags &= ~W_FL_CROUCH;

			waypoints[index].flags |= W_FL_MINE;
			output = (char *)new_type;
		}
	}

	else if (FStrEq(new_type, "prone"))
	{
		if (waypoints[index].flags & W_FL_PRONE)
		{
			waypoints[index].flags &= ~W_FL_PRONE;
			output = "removed";
		}
		else
		{
			if (waypoints[index].flags & W_FL_CROUCH)
				waypoints[index].flags &= ~W_FL_CROUCH;

			waypoints[index].flags |= W_FL_PRONE;
			output = (char *)new_type;
		}
	}

	else if ((FStrEq(new_type, "flag")) || (FStrEq(new_type, "pushpoint")))
	{
		if (waypoints[index].flags & W_FL_PUSHPOINT)
		{
			waypoints[index].flags &= ~W_FL_PUSHPOINT;
			output = "removed";
		}
		else
		{
			waypoints[index].flags |= W_FL_PUSHPOINT;
			output = (char *)new_type;
		}
	}

	else if (FStrEq(new_type, "sprint"))
	{
		if (waypoints[index].flags & W_FL_SPRINT)
		{
			waypoints[index].flags &= ~W_FL_SPRINT;
			output = "removed";
		}
		else
		{
			if (waypoints[index].flags & W_FL_CROUCH)
				waypoints[index].flags &= ~W_FL_CROUCH;
			else if (waypoints[index].flags & W_FL_PRONE)
				waypoints[index].flags &= ~W_FL_PRONE;

			waypoints[index].flags |= W_FL_SPRINT;
			output = (char *)new_type;
		}
	}

	else if (FStrEq(new_type, "sniper"))
	{
		if (waypoints[index].flags & W_FL_SNIPER)
		{
			waypoints[index].flags &= ~W_FL_SNIPER;
			output = "removed";
		}
		else
		{
			waypoints[index].flags |= W_FL_SNIPER;
			output = (char *)new_type;
		}
	}

	else if (FStrEq(new_type, "trigger"))
	{
		if (waypoints[index].flags & W_FL_TRIGGER)
		{
			waypoints[index].flags &= ~W_FL_TRIGGER;
			output = "removed";
		}
		else
		{
			waypoints[index].flags |= W_FL_TRIGGER;
			output = (char *)new_type;
		}
	}

	else if (FStrEq(new_type, "use"))
	{
		if (waypoints[index].flags & W_FL_USE)
		{
			waypoints[index].flags &= ~W_FL_USE;
			output = "removed";
		}
		else
		{
			waypoints[index].flags |= W_FL_USE;
			waypoints[index].red_time = 0;
			waypoints[index].blue_time = 0;
			if (waypoints[index].range > 20)
				waypoints[index].range = (float) 20;
			output = (char *)new_type;
		}
	}

	else
		output = "false";

	return output;
}

/*
* changes priority on the nearest waypoint from current
* to the one specified in arg2
* if there is specified a team we want to change the priority for it changes only its value
* if team is NOT specified it sets the new value for both teams
* or if there is no new priority specified it checks current priorities and sets the higher one for both teams
* returns -4 if there is no waypoint close
* returns -3 if the new priority is invalid
* returns -2 if the team value is invalid
* returns -1 if something else went wrong
* returns the new priority if everything is OK
*/
int WptPriorityChange(edict_t *pEntity, const char *arg2, const char *arg3)
{
	int index, new_priority, wpt_priority = -1, team = -1;

	// find the nearest waypoint
	index = WaypointFindNearest(pEntity, 50.0, -1);

	if (index == -1)
		return -4;

	// is new priority value
	if ((arg2 != NULL) && (*arg2 != 0))
	{
		int temp = atoi(arg2);

		// is priority valid
		if ((temp >= MIN_WPT_PRIOR) && (temp <= MAX_WPT_PRIOR))
			new_priority = temp;
		else
			return -3;
	}
	// otherwise check wpt_priorities and set the higher priority (lower number) for both teams
	else
	{
		// red is higher so copy it for blue team
		if (waypoints[index].red_priority < waypoints[index].blue_priority)
		{
			if (waypoints[index].red_priority == 0)
				waypoints[index].red_priority = waypoints[index].blue_priority;
			else
				waypoints[index].blue_priority = waypoints[index].red_priority;

			return waypoints[index].red_priority + 10;
		}
		// blue is higher so copy it for red team
		else if (waypoints[index].blue_priority < waypoints[index].red_priority)
		{
			if (waypoints[index].blue_priority == 0)
				waypoints[index].blue_priority = waypoints[index].red_priority;
			else
				waypoints[index].red_priority = waypoints[index].blue_priority;

			return waypoints[index].blue_priority + 10;
		}
		else
			return -1;
	}

	// is team specified
	if ((arg3 != NULL) && (*arg3 != 0))
	{
		if ((FStrEq(arg3, "red")) || (FStrEq(arg3, "1")))
			team = TEAM_RED;
		else if ((FStrEq(arg3, "blue"))  || (FStrEq(arg3, "2")))
			team = TEAM_BLUE;
		else
			return -2;
	}
	// otherwise both priorities are counted
	else
	{
		// does any priority differs from the new one
		if ((waypoints[index].red_priority != new_priority) ||
			(waypoints[index].blue_priority != new_priority))
		{
			waypoints[index].red_priority = new_priority;
			waypoints[index].blue_priority = new_priority;

			if (new_priority != 0)
				return new_priority + 10;
			else
				return new_priority;
		}
	}

	// read red priority for this waypoint
	if (team == TEAM_RED)
		wpt_priority = waypoints[index].red_priority;
	// read blue priority for this waypoint
	if (team == TEAM_BLUE)
		wpt_priority = waypoints[index].blue_priority;

	// is new priority NOT same to waypoint priority so set it
	if ((wpt_priority != -1) && (new_priority != wpt_priority))
	{
		if (team == TEAM_RED)
			waypoints[index].red_priority = new_priority;
		else if (team == TEAM_BLUE)
			waypoints[index].blue_priority = new_priority;

		return new_priority;
	}

	return -1;
}

/*
* changes the trigger priority on the nearest trigger waypoint from current one
* to the one specified in priority argument
* if there is specified a team we want to change the priority for it changes only its value
* if team is NOT specified it sets the new value for both teams
* or if there is no new priority specified it checks current priorities and sets the higher one for both teams
* returns -4 if there is no waypoint close
* returns -3 if the new priority is invalid
* returns -2 if the team value is invalid
* returns -1 if something else went wrong
* returns the new priority if everything is OK
*/
int WptTriggerPriorityChange(edict_t *pEntity, const char *priority, const char *for_team)
{
	int index, new_priority, wpt_priority = -1, team = -1;

	// find the nearest trigger waypoint
	index = WaypointFindNearestType(pEntity, 50.0, W_FL_TRIGGER);

	if (index == -1)
		return -4;

	// is new priority value
	if ((priority != NULL) && (*priority != 0))
	{
		int temp = atoi(priority);

		// is priority valid
		if ((temp >= MIN_WPT_PRIOR) && (temp <= MAX_WPT_PRIOR))
			new_priority = temp;
		else
			return -3;
	}
	// otherwise check priorities and set the higher priority (lower number) for both teams
	else
	{
		// red is higher so copy it for blue team
		if (waypoints[index].trigger_red_priority < waypoints[index].trigger_blue_priority)
		{
			if (waypoints[index].trigger_red_priority == 0)
				waypoints[index].trigger_red_priority = waypoints[index].trigger_blue_priority;
			else
				waypoints[index].trigger_blue_priority = waypoints[index].trigger_red_priority;

			return waypoints[index].trigger_red_priority + 10;
		}
		// blue is higher so copy it for red team
		else if (waypoints[index].trigger_blue_priority < waypoints[index].trigger_red_priority)
		{
			if (waypoints[index].trigger_blue_priority == 0)
				waypoints[index].trigger_blue_priority = waypoints[index].trigger_red_priority;
			else
				waypoints[index].trigger_red_priority = waypoints[index].trigger_blue_priority;

			return waypoints[index].trigger_blue_priority + 10;
		}
		else
			return -1;
	}

	// is team specified
	if ((for_team != NULL) && (*for_team != 0))
	{
		if ((FStrEq(for_team, "red")) || (FStrEq(for_team, "1")))
			team = TEAM_RED;
		else if ((FStrEq(for_team, "blue"))  || (FStrEq(for_team, "2")))
			team = TEAM_BLUE;
		else
			return -2;
	}
	// otherwise both priorities are counted
	else
	{
		// does any priority differs from the new one
		if ((waypoints[index].trigger_red_priority != new_priority) ||
			(waypoints[index].trigger_blue_priority != new_priority))
		{
			waypoints[index].trigger_red_priority = new_priority;
			waypoints[index].trigger_blue_priority = new_priority;

			if (new_priority != 0)
				return new_priority + 10;
			else
				return new_priority;
		}
	}

	// read red priority for this waypoint
	if (team == TEAM_RED)
		wpt_priority = waypoints[index].trigger_red_priority;
	// read blue priority for this waypoint
	if (team == TEAM_BLUE)
		wpt_priority = waypoints[index].trigger_blue_priority;

	// is new priority NOT same to waypoint priority so set it
	if ((wpt_priority != -1) && (new_priority != wpt_priority))
	{
		if (team == TEAM_RED)
			waypoints[index].trigger_red_priority = new_priority;
		else if (team == TEAM_BLUE)
			waypoints[index].trigger_blue_priority = new_priority;

		return new_priority;
	}

	return -1;
}

/*
* changes time on the nearest waypoint
* works completely same to PriorityChange method (also return values are same)
*/
float WptTimeChange(edict_t *pEntity, const char *arg2, const char *arg3)
{
	int index, team = -1;
	float new_time, wpt_time = -1.0;

	// find the nearest waypoint
	index = WaypointFindNearest(pEntity, 50.0, -1);

	if (index == -1)
		return -4.0;

	// is new time value
	if ((arg2 != NULL) && (*arg2 != 0))
	{
		int temp = atoi(arg2);

		// is new_time valid
		if ((temp >= 0) && (temp <= 500))
			new_time = (float) temp;
		else
			return -3.0;
	}
	// otherwise check wpt_times and set the higher time for both teams
	else
	{
		// is red_time higher than blue_time
		if (waypoints[index].red_time > waypoints[index].blue_time)
		{
			waypoints[index].blue_time = waypoints[index].red_time;

			return waypoints[index].red_time + 1000;
		}
		// is blue_time higher than red_time
		else if (waypoints[index].blue_time > waypoints[index].red_time)
		{
			waypoints[index].red_time = waypoints[index].blue_time;
			
			return waypoints[index].blue_time + 1000;
		}
		// otherwise both are same
		else
			return -1.0;
	}

	// if waypoint range is bigger than half of max reachable range
	// set it to the double of the default range
	if (waypoints[index].range > (float) MAX_WPT_DIST / 2)
		waypoints[index].range = WPT_RANGE * (float) 2;

	// is team specified
	if ((arg3 != NULL) && (*arg3 != 0))
	{
		if ((FStrEq(arg3, "red")) || (FStrEq(arg3, "1")))
			team = TEAM_RED;
		else if ((FStrEq(arg3, "blue")) || (FStrEq(arg3, "2")))
			team = TEAM_BLUE;
		else
			return -2.0;
	}
	// otherwise both times are counted
	else
	{
		// does any time differs from the new one
		if ((waypoints[index].red_time != new_time) ||
			(waypoints[index].blue_time != new_time))
		{
			waypoints[index].red_time = new_time;
			waypoints[index].blue_time = new_time;

			if (new_time != 0.0)
				return new_time + 1000;
			else
				return new_time;
		}
	}

	// read red time for this waypoint
	if (team == TEAM_RED)
		wpt_time = waypoints[index].red_time;
	// read blue time for this waypoint
	if (team == TEAM_BLUE)
		wpt_time = waypoints[index].blue_time;

	// is new time NOT same to waypoint time so set it
	if ((wpt_time != -1.0) && (new_time != wpt_time))
	{
		if (team == TEAM_RED)
			waypoints[index].red_time = new_time;
		else if (team == TEAM_BLUE)
			waypoints[index].blue_time = new_time;

		return new_time;
	}

	return -1.0;
}

/*
* changes range on the nearest waypoint to the value specified in arg2
* returns -3.0 if the new range is same to current value
* returns -2.0 if there is no waypoint close
* returns -1.0 if something else went wrong
* returns the new value if everything is OK
*/
float WptRangeChange(edict_t *pEntity, const char *arg2)
{
	int index;
	float range, output = -1.0;

	// find the nearest waypoint...
	index = WaypointFindNearest(pEntity, 50.0, -1);

	if (index == -1)
		return -2.0;

	// read current range for this waypoint
	range = waypoints[index].range;
	
	// is specified agr2
	if ((arg2 != NULL) && (*arg2 != 0))
	{
		int temp = atoi(arg2);

		if ((temp >= 0.0) && (temp <= MAX_WPT_DIST))
		{
			range = (float) temp;

			// is new range NOT same to waypoint range
			if (range != waypoints[index].range)
			{
				// set it
				waypoints[index].range = range;
				output = range;
			}
			else if (range == waypoints[index].range)
				output = -3.0;
		}
	}

	return output;
}

/*
* sets all additional waypoint data (priority, time etc.) to default values (for both teams)
* returns FALSE if there is no waypoint close
* returns TRUE if everything is OK
*/
bool WptResetAdditional(edict_t *pEntity, const char* arg1, const char* arg2, const char* arg3, const char* arg4)
{
	int index;

	// find the nearest waypoint
	index = WaypointFindNearest(pEntity, 50.0, -1);

	if (index == -1)
		return FALSE;

	if ((FStrEq(arg1, "all")) || (arg1 == NULL) || (*arg1 == 0))
	{
		// we must store the waypoint position first
		Vector temp_origin = waypoints[index].origin;
		// than we can reset the waypoint back to defualt values
		WaypointInit(index);
		// return the waypoint back to its position
		waypoints[index].origin = temp_origin;
		// and finally change it to standard waypoint
		waypoints[index].flags = 0;
		waypoints[index].flags |= W_FL_STD;
	}
	// check all possible names/spelling
	if ((FStrEq(arg1, "type")) || (FStrEq(arg2, "type")) ||
		(FStrEq(arg3, "type")) || (FStrEq(arg4, "type")) ||
		(FStrEq(arg1, "flag")) || (FStrEq(arg2, "flag")) ||
		(FStrEq(arg3, "flag")) || (FStrEq(arg4, "flag")) ||
		(FStrEq(arg1, "tag")) || (FStrEq(arg2, "tag")) ||
		(FStrEq(arg3, "tag")) || (FStrEq(arg4, "tag")))
	{
		waypoints[index].flags = 0;
		waypoints[index].flags |= W_FL_STD;
	}
	if ((FStrEq(arg1, "priority")) || (FStrEq(arg2, "priority")) ||
		(FStrEq(arg3, "priority")) || (FStrEq(arg4, "priority")))
	{
		waypoints[index].red_priority = MAX_WPT_PRIOR;
		waypoints[index].blue_priority = MAX_WPT_PRIOR;
	}
	if ((FStrEq(arg1, "time")) || (FStrEq(arg2, "time")) ||
		(FStrEq(arg3, "time")) || (FStrEq(arg4, "time")))
	{
		waypoints[index].red_time = 0.0;
		waypoints[index].blue_time = 0.0;
	}
	if ((FStrEq(arg1, "range")) || (FStrEq(arg2, "range")) ||
		(FStrEq(arg3, "range")) || (FStrEq(arg4, "range")))
	{
		// set the correct range based on waypoint flag
		if (waypoints[index].flags & W_FL_AIMING)
			waypoints[index].range = (float) 0;
		else if (waypoints[index].flags & W_FL_CROSS)
			waypoints[index].range = WPT_RANGE * (float) 3;
		else if (waypoints[index].flags & RANGE_20_WPT)
			waypoints[index].range = (float) 20;
		else
			waypoints[index].range = WPT_RANGE;
	}

	return TRUE;
}

/*
* sets correct size/height of the beam based on waypoint type
* keep this order to ensure that the waypoint will be shown correctly
*/
void SetWaypointSize(int wpt_index, Vector &start, Vector &end)
{
	if (waypoints[wpt_index].flags & (W_FL_CROSS | W_FL_AIMING))
	{
		start = waypoints[wpt_index].origin;
		end = start + Vector(0, 0, 34);
		
		return;
	}
	
	if (waypoints[wpt_index].flags & W_FL_STD)
	{
		start = waypoints[wpt_index].origin - Vector(0, 0, 34);
		end = start + Vector(0, 0, 68);
	}
	// if this is "shoot" waypoint override the setting to prevent
	// confusion when user see bot shooting while standing at normal waypoint
	if (waypoints[wpt_index].flags & (W_FL_FIRE | W_FL_SPRINT))
	{
		start = waypoints[wpt_index].origin;
		end = start + Vector(0, 0, 34);
	}
	if (waypoints[wpt_index].flags & W_FL_SNIPER)
	{
		start = waypoints[wpt_index].origin - Vector(0, 0, 17);
		end = start + Vector(0, 0, 34);
	}
	if (waypoints[wpt_index].flags & W_FL_GOBACK)
	{
		start = waypoints[wpt_index].origin - Vector(0, 0, 34);
		end = start + Vector(0, 0, 68);
	}
	if (waypoints[wpt_index].flags & W_FL_CROUCH)
	{
		start = waypoints[wpt_index].origin - Vector(0, 0, 17);
		end = start + Vector(0, 0, 34);
	}
	else if (waypoints[wpt_index].flags & W_FL_PRONE)
	{
		start = waypoints[wpt_index].origin - Vector(0, 0, 17);
		end = start + Vector(0, 0, 34);
	}
	if (waypoints[wpt_index].flags & W_FL_AMMOBOX)
	{
		start = waypoints[wpt_index].origin;
		end = start + Vector(0, 0, 34);
	}
	else if (waypoints[wpt_index].flags & (W_FL_DOOR | W_FL_DOORUSE))
	{
		start = waypoints[wpt_index].origin - Vector(0, 0, 34);
		end = start + Vector(0, 0, 68);
	}
	if (waypoints[wpt_index].flags & W_FL_MINE)
	{
		start = waypoints[wpt_index].origin - Vector(0, 0, 17);
		end = start + Vector(0, 0, 34);
	}
	else if (waypoints[wpt_index].flags & W_FL_USE)
	{
		start = waypoints[wpt_index].origin - Vector(0, 0, 34);
		end = start + Vector(0, 0, 68);
	}
	if (waypoints[wpt_index].flags & W_FL_JUMP)
	{
		start = waypoints[wpt_index].origin;
		end = start + Vector(0, 0, 34);
	}
	else if (waypoints[wpt_index].flags & W_FL_DUCKJUMP)
	{
		start = waypoints[wpt_index].origin - Vector(0, 0, 34);
		end = start + Vector(0, 0, 68);
	}
	if (waypoints[wpt_index].flags & (W_FL_PUSHPOINT | W_FL_TRIGGER))
	{
		start = waypoints[wpt_index].origin - Vector(0, 0, 34);
		end = start + Vector(0, 0, 68);
	}
	if (waypoints[wpt_index].flags & W_FL_LADDER)
	{
		start = waypoints[wpt_index].origin;
		end = start + Vector(0, 0, 34);
	}
	if (waypoints[wpt_index].flags & W_FL_CHUTE)
	{
		start = waypoints[wpt_index].origin - Vector(0, 0, 17);
		end = start + Vector(0, 0, 34);
	}
}

/*
* sets waypoint color based on waypoint type, the color is returned as vector of RGB values
* we have to keep this order to show correct color due to the fact
* that there can be more flag/tag bits on the waypoint
*/
Vector SetWaypointColor(int wpt_index)
{
	// init waypoint color values
	// (white isn't used for any waypoint so we can easily see if there is any problem)
	int red = 255, green = 255, blue = 255;

	if (waypoints[wpt_index].flags & (W_FL_STD | W_FL_SPRINT))
		{ red = 255; green = 128; blue = 0; }
	if (waypoints[wpt_index].flags & W_FL_FIRE) { red = green = 255; blue = 0; }
	if (waypoints[wpt_index].flags & W_FL_SNIPER) { red = 255;	green = blue = 0; }
	//if (waypoints[wpt_index].flags & (W_FL_JUMP | W_FL_DUCKJUMP))
	//	{ red = 0; green = 255; blue = 0; }
	if (waypoints[wpt_index].flags & W_FL_GOBACK) { red = 0; green = blue = 255; }
	if (waypoints[wpt_index].flags & W_FL_CROUCH) { red = 255; green = 128; blue = 0; }
	if (waypoints[wpt_index].flags & (W_FL_JUMP | W_FL_DUCKJUMP))
		{ red = 0; green = 255; blue = 0; }
	if (waypoints[wpt_index].flags & W_FL_PRONE) { red = 0; green = 255; blue = 0; }
	if (waypoints[wpt_index].flags & W_FL_LADDER) { red = 0; green = blue = 255; }
	if (waypoints[wpt_index].flags & (W_FL_AIMING | W_FL_MINE))
		{ red = 255; green = 0; blue = 255; }
	if (waypoints[wpt_index].flags & W_FL_AMMOBOX) { red = 255; green = blue = 0; }
	if (waypoints[wpt_index].flags & W_FL_CHUTE) { red = green = 255; blue = 0; }
	if (waypoints[wpt_index].flags & W_FL_CROSS) { red = green = 0; blue = 255; }
	if (waypoints[wpt_index].flags & (W_FL_DOOR | W_FL_DOORUSE))
		{ red = 255; green = 0; blue = 255; }
	if (waypoints[wpt_index].flags & W_FL_USE) { red = 255; green = blue = 0; }
	if (waypoints[wpt_index].flags & W_FL_PUSHPOINT) { red = green = 255; blue = 0; }
	if (waypoints[wpt_index].flags & W_FL_TRIGGER) { red = 128; green = 0; blue = 255; }

	return Vector(red, green, blue);
}


/*
* sets path texture based on its direction
* the texture is returned as an int variable that has been precached
* in DispatchSpawn() in dll.cpp
*/
int SetPathTexture(int path_index)
{
	// default path sprite
	int sprite = m_spriteTexturePath2;

	// use different sprite for one-way paths
	// should help to check path direction a bit more easier
	if (w_paths[path_index]->flags & P_FL_WAY_ONE)
		sprite = m_spriteTexturePath1;

	// use different sprite for paths with additional flags
	if (w_paths[path_index]->flags & (P_FL_MISC_AVOID | P_FL_MISC_IGNORE | P_FL_MISC_GITEM))
		sprite = m_spriteTexturePath3;

	return sprite;
}


/*
* sets path color based on its type, the color is returned as vector of RGB values
* we have to keep this order to show correct color due to the fact
* that there is more flag/tag bits on the path
*/
Vector SetPathColor(int path_index)
{
	int red = 255, green = 255, blue = 255;

	// both team - magenta color
	if (w_paths[path_index]->flags & P_FL_TEAM_NO) { red = 128; green = 0; blue = 255; }
	// red team - red color
	else if (w_paths[path_index]->flags & P_FL_TEAM_RED) { red = 255; green = 0; blue = 0; }
	// blue team - blue color
	else if (w_paths[path_index]->flags & P_FL_TEAM_BLUE)
		{ red = 0; green = 0; blue = 255; }
	
	// sniper path - light green color
	if (w_paths[path_index]->flags & P_FL_CLASS_SNIPER) { red = 128; green = 255; blue = 128; }
	// mgunner path - dark green color
	else if (w_paths[path_index]->flags & P_FL_CLASS_MGUNNER)
		{ red = 0; green = 128; blue = 0; }

	// patrot path - yellow color
	if (w_paths[path_index]->flags & P_FL_WAY_PATROL) { red = 255; green = 255; blue = 0; }

	return Vector(red, green, blue);
}


/*
* handle the real-time waypoint data updating, like if we need to change priorities of some
* waypoints because the bots already reached certain map objectives
*/
void UpdateWaypointData(void)
{
	for (int path_index = 0; path_index < num_w_paths; path_index++)
		UpdatePathStatus(path_index);
}


//																										NEW CODE 094
/*
* handle all events with turning auto waypoint on and off
*/
void StartAutoWaypointg(bool switch_on)
{
	extern edict_t *listenserver_edict;

	if (switch_on)
	{
		wptser.SetAutoWaypointing(true);	// turn autowaypointing on
		wptser.SetShowWaypoints(true);		// turn waypoints on too just in case

		// sign the waypoints as autowaypointed ones
		if (WaypointSubscribe("built-in auto waypointing", TRUE, TRUE))
		{
			// we should also clear the 'modified by' signature
			WaypointSubscribe("clear", FALSE, FALSE);
			WaypointSave(NULL);
			WaypointPathSave(NULL);
		}

		wptser.SetAutoAddToPath(true);		// activate also auto adding to path
		wptser.SetShowPaths(true);			// start displaying the paths as well
	}
	else
	{
		wptser.ResetAutoWaypointing();		// stop autowaypointing
		wptser.ResetAutoAddToPath();		// deactivate automatic additions to path

		// correctly end current path
		WaypointFinishPath(listenserver_edict);

		// run waypoint self cleaning routines
		UTIL_RepairWaypointRangeandPosition(listenserver_edict);
		WaypointRepairCrossRange();
		WaypointRepairInvalidPathMerge();
	}

	return;
}//*******************************************************************************************		NEW CODE 094 END


/*
* handle all waypoint and path drawing (redrawing)
* also handle autowaypoint and autoadding to path features
* and some special show features like waypoints connected with cross, drawing team paths only etc.
*/
void WaypointThink(edict_t *pEntity)
{
	float distance, min_distance;
	Vector start, end;
	int i;

	// is auto waypoint on
	if (wptser.IsAutoWaypointing())
	{
		// find the distance from the last used waypoint
		distance = (last_waypoint - pEntity->v.origin).Length();

		// is this wpt far enough to add new wpt
		if (distance > wptser.GetAutoWaypointingDistance())
		{
			min_distance = 9999.0;

			// check that no other reachable waypoints are nearby
			for (i = 0; i < num_waypoints; i++)
			{
				// skip these waypoints
				if (waypoints[i].flags & (W_FL_DELETED | W_FL_AIMING))
					continue;

				if (WaypointReachable(pEntity->v.origin, waypoints[i].origin, pEntity))
				{
					distance = (waypoints[i].origin - pEntity->v.origin).Length();

					//																									NEW CODE 094
					// if the distance to this cross waypoint is at least half of
					// current auto waypointing distance then ignore this cross waypoint
					if ((waypoints[i].flags & W_FL_CROSS) &&
						(distance > (wptser.GetAutoWaypointingDistance() / 2)))
						continue;
					//**********************************************************************************************	NEW CODE 094 END

					if (distance < min_distance)
						min_distance = distance;
				}
			}

			// make sure nearest waypoint is far enough away
			if (min_distance >= wptser.GetAutoWaypointingDistance())
			{
				// is the edict NOT a bot
				if (!(pEntity->v.flags & FL_FAKECLIENT))
				{
					// is the edict in crouch so place crouch wpt
					if (pEntity->v.flags & FL_DUCKING)
						WptAdd(pEntity, "crouch");
					// otherwise place normal/standing
					else
						WptAdd(pEntity, "normal");
				}
			}
		}
	}

	// is path autoadding on
	if (wptser.IsAutoAddToPath())
	{
		// is the edict NOT a bot
		if (!(pEntity->v.flags & FL_FAKECLIENT))
		{
			// search through all waypoints
			for (i = 0; i < num_waypoints; i++)
			{
				// continue current path routine can return even -1, but if 'not enough memory' error
				// happens then things wouldn't work at all so this is a safe value
				int result = -1;

				distance = (waypoints[i].origin - pEntity->v.origin).Length();

				// if player "touch" the waypoint
				if (distance <= AUTOADD_DISTANCE)
				{
					// add the waypoint to actual path
					result = ContinueCurrPath(i);
				}

				// if waypoint was added successfully play snd_done
				if (result == 1)
				{
					// however sound confirmation routine isn't known here so we have to do it manually
					EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "plats/elevbell1.wav", 1.0, ATTN_NORM, 0, 100);
				}
				//																									NEW CODE 094
				// if we are autowaypoing and this is the first waypoint that should be added to a path...
				else if (wptser.IsAutoWaypointing() && (result == -2))
				{
					// then we must start a new path
					if (StartNewPath(i))
					{
						EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "plats/elevbell1.wav", 1.0, ATTN_NORM, 0, 100);
					}
				}
				//**********************************************************************************************	NEW CODE 094 END
			}
		}
	}

	// display the waypoints if turned on
	if (wptser.IsShowWaypoints())
	{
		for (i = 0; i < num_waypoints; i++)
		{
			// skip all deleted waypoint
			if (waypoints[i].flags & W_FL_DELETED)
				continue;

			// needs this waypoint to be refreshed
			if ((wp_display_time[i] + 1.0) < gpGlobals->time)
			{
				// get the distance to this waypoint
				distance = (waypoints[i].origin - pEntity->v.origin).Length();

				// is this waypoint close enough AND is in view cone (in FOV)
				if ((distance < wptser.GetWaypointsDrawDistance()) && (FInViewCone(&waypoints[i].origin, pEntity)))
				{
					// is a path to highlight activated and is it a specific (by index) path to highlight
					if (wptser.IsPathToHighlightAPathIndex())
					{
						// if this waypoint is NOT in the path we want then skip it
						// (ie. display only waypoints that are on path that the user wants to see)
						if (WaypointIsInPath(i, wptser.GetPathToHighlight()) == FALSE)
							continue;
					}

					// get size/hight of the beam based on waypoint type
					SetWaypointSize(i, start, end);

					// use temp vector to set color for this waypoint
					Vector color = SetWaypointColor(i);

					// draw the waypoint
					WaypointBeam(pEntity, start, end, 30, 0, color.x, color.y, color.z, 250, 5);

					wp_display_time[i] = gpGlobals->time;
				}
			}
		}
	}

	// display the waypoint range if turned on
	if (wptser.IsCheckRanges())
	{
		for (i = 0; i < num_waypoints; i++)
		{
			// skip all deleted, aim, cross, ladder and door waypoints
			// (some aren't used for navigation and others don't use range setting)
			if (waypoints[i].flags & (W_FL_DELETED | W_FL_AIMING | W_FL_CROSS | \
										W_FL_LADDER | W_FL_DOOR | W_FL_DOORUSE))
				continue;

			// needs this range to be refreshed
			if ((f_ranges_display_time[i] + 1.0) < gpGlobals->time)
			{
				distance = (waypoints[i].origin - pEntity->v.origin).Length();

				if ((distance < wptser.GetWaypointsDrawDistance()) && (FInViewCone(&waypoints[i].origin, pEntity)))
				{
					if (wptser.IsPathToHighlightAPathIndex())
					{
						if (WaypointIsInPath(i, wptser.GetPathToHighlight()) == FALSE)
							continue;
					}

					distance = (waypoints[i].origin - pEntity->v.origin).Length();

					// draw ranges only for really close waypoints
					if (distance < 100)
					{
						// get this waypoint range
						float the_range = waypoints[i].range;

						// draw x-coord white beam
						start = waypoints[i].origin - Vector(the_range, 0, 0);
						end = waypoints[i].origin + Vector(the_range, 0, 0);
						WaypointBeam(pEntity, start, end, 10, 2, 250, 250, 250, 200, 10);

						// draw y-coord white beam
						start = waypoints[i].origin - Vector(0, the_range, 0);
						end = waypoints[i].origin + Vector(0, the_range, 0);						
						WaypointBeam(pEntity, start, end, 10, 2, 250, 250, 250, 200, 10);

						f_ranges_display_time[i] = gpGlobals->time;
					}
				}
			}
		}
	}

	// display pathbeams (ie connections between waypoints) if turned on
	if (wptser.IsShowPaths())
	{
		int path_index;

		// go through all paths
		for (path_index = 0; path_index < num_w_paths; path_index++)
		{
			// if the user wanted to see just one specific path then
			// we must show only that path and skip all other
			if ((wptser.IsPathToHighlightAPathIndex()) && (path_index != wptser.GetPathToHighlight()))
				continue;

			// exists the path AND is it time to refresh it
			if ((w_paths[path_index] != NULL) && (f_path_time[path_index] + 1.0 <= gpGlobals->time))
			{
				W_PATH *p, *prev;
				float path_wpt_dist;	// distance between path waypoint and player
				
				// do we highlight red team accessible paths AND this path is NOT for red team
				if ((wptser.GetPathToHighlight() == HIGHLIGHT_RED) &&
					!(w_paths[path_index]->flags & (P_FL_TEAM_NO | P_FL_TEAM_RED)))
					continue;	// so skip it
				// do we highlight blue team accessible paths AND this path is NOT for blue team
				else if ((wptser.GetPathToHighlight() == HIGHLIGHT_BLUE) &&
					!(w_paths[path_index]->flags & (P_FL_TEAM_NO | P_FL_TEAM_BLUE)))
					continue;
				// do we highlight one-way paths AND this path is NOT one-way
				else if ((wptser.GetPathToHighlight() == HIGHLIGHT_ONEWAY) &&
					!(w_paths[path_index]->flags & P_FL_WAY_ONE))
					continue;
				// do we want to see just sniper paths AND this one isn't snipers only path
				else if ((wptser.GetPathToHighlight() == HIGHLIGHT_SNIPER) &&
					!(w_paths[path_index]->flags & P_FL_CLASS_SNIPER))
					continue;
				// same as above, but for machine gunner
				else if ((wptser.GetPathToHighlight() == HIGHLIGHT_MGUNNER) &&
					!(w_paths[path_index]->flags & P_FL_CLASS_MGUNNER))
					continue;

				int sprite = SetPathTexture(path_index);
				Vector color = SetPathColor(path_index);

				p = w_paths[path_index];
				prev = NULL;

				// go through this path
				while (p != NULL)
				{
					path_wpt_dist = (waypoints[p->wpt_index].origin - pEntity->v.origin).Length();

					// is player close enough to draw path from this node AND
					// is in view cone
					if ((path_wpt_dist < wptser.GetWaypointsDrawDistance())
						&& (FInViewCone(&waypoints[p->wpt_index].origin, pEntity)))
					{
						// if both exist
						if ((p != NULL) && (prev != NULL))
						{
							Vector v_src = waypoints[prev->wpt_index].origin;
							Vector v_dest = waypoints[p->wpt_index].origin;

							// draw a path line between those waypoints
							PathBeam(pEntity, v_src, v_dest, sprite, 20, 2, color.x, color.y, color.z, 200, 10);

							f_path_time[path_index] = gpGlobals->time;
						}
					}

					prev = p;		// store pointer to previous node (ie waypoint)
					p = p->next;	// go to next node in linked list
				}
			}
		}
	}

	// display connections between waypoint with wait time and its aim waypoints
	if (wptser.IsCheckAims())
	{
		// check all waypoints for any aim waypoint
		for (int aim_wpt = 0; aim_wpt < num_waypoints; aim_wpt++)
		{
			// skip all deleted waypoints
			if (waypoints[aim_wpt].flags & W_FL_DELETED)
				continue;

			// skip all non aim waypoints
			if (!(waypoints[aim_wpt].flags & W_FL_AIMING))
				continue;

			// get the distance of this aim waypoint to the player
			float distance = (waypoints[aim_wpt].origin - pEntity->v.origin).Length();

			// is the player close enough AND is waypoint in his view cone
			if ((distance < wptser.GetWaypointsDrawDistance()) && (FInViewCone(&waypoints[aim_wpt].origin, pEntity)))
			{
				// check all waypoints again for any possible "waittimed" waypoint
				for (int in_range = 0; in_range < num_waypoints; in_range++)
				{
					// skip the current aim waypoint and also any other aim or cross waypoint
					if ((in_range == aim_wpt) ||
						(waypoints[in_range].flags & (W_FL_CROSS | W_FL_AIMING | W_FL_DELETED)))
						continue;

					// draw the connections only for waypoints with time tag on
					// and for claymore waypoints
					if ((waypoints[in_range].red_time != 0.0) ||
						(waypoints[in_range].blue_time != 0.0) ||
						(waypoints[in_range].flags & W_FL_MINE))
					{
						// get the distance of this waypoint from current aim waypoint
						float dist = (waypoints[in_range].origin - waypoints[aim_wpt].origin).Length();

						// is this waypoint "in radius" of the current aim waypoint
						// (ie is the aim waypoint in raduis of 100 units around this waypoint)
						// AND is it time to refresh it
						if ((dist <= 100) &&
							(f_conn_time[aim_wpt][in_range] + 1.0 < gpGlobals->time))
						{
							TraceResult tr;

							start = waypoints[aim_wpt].origin;
							end = waypoints[in_range].origin;

							// check if the waypoint isn't blocked
							UTIL_TraceLine(start, end, ignore_monsters, ignore_glass, NULL, &tr);

							// if not blocked draw the connection
							if (tr.flFraction >= 1.0)
							{
								// draw light purple line
								WaypointBeam(pEntity, start, end, 10, 2, 255, 128, 255, 200, 10);

								f_conn_time[aim_wpt][in_range] = gpGlobals->time;
							}
						}
					}
				}
			}
		}
	}

	// is check_cross
	if (wptser.IsCheckCross())
	{
		// check all wpts
		for (int cross_wpt = 0; cross_wpt < num_waypoints; cross_wpt++)
		{
			// skip all deleted wpts
			if (waypoints[cross_wpt].flags & W_FL_DELETED)
				continue;

			// skip all non cross waypoints
			if (!(waypoints[cross_wpt].flags & W_FL_CROSS))
				continue;

			// get the distance of this cross waypoint to the player
			float distance = (waypoints[cross_wpt].origin - pEntity->v.origin).Length();

			// is the player close enough AND is waypoint in his view cone
			if ((distance < wptser.GetWaypointsDrawDistance()) && (FInViewCone(&waypoints[cross_wpt].origin, pEntity)))
			{
				// check all waypoints again
				for (int in_range = 0; in_range < num_waypoints; in_range++)
				{
					// skip the current cross waypoint and also any other cross or aim waypoint
					if ((in_range == cross_wpt) ||
						(waypoints[in_range].flags & (W_FL_CROSS | W_FL_AIMING | W_FL_DELETED)))
						continue;

					// get the distance of this waypoint from current cross waypoint
					float dist = (waypoints[in_range].origin - waypoints[cross_wpt].origin).Length();

					// is this waypoint in range of the current cross waypoint
					// AND is it time to refresh it
					if ((dist <= waypoints[cross_wpt].range) &&
						(f_conn_time[cross_wpt][in_range] + 1.0 < gpGlobals->time))
					{
						TraceResult tr;

						start = waypoints[cross_wpt].origin;
						end = waypoints[in_range].origin;

						// check if the wpt isn't blocked
						UTIL_TraceLine(start, end, ignore_monsters, NULL, &tr);

						// if not blocked draw the connection
						if (tr.flFraction >= 1.0)
						{
							// draw cyan line
							WaypointBeam(pEntity, start, end, 10, 2, 0, 255, 255, 200, 10);

							f_conn_time[cross_wpt][in_range] = gpGlobals->time;
						}
					}
				}
			}
		}
	}

	// display the waypoint compass if turned on
	if (wptser.GetCompassIndex() != NO_VAL)
	{
		// is it time to refresh the compass
		// the delay is purposely higher than the beam life
		// otherwise the compass beam wouldn't blink
		if ((f_compass_time + 0.5) < gpGlobals->time)
		{
			start = waypoints[wptser.GetCompassIndex()].origin;
			end = pEntity->v.origin;

			// draw red line
			DrawBeam(pEntity, start, end, 2, 255, 5, 5, 125);

			f_compass_time = gpGlobals->time;
		}
	}
}


/*
* runs several tracelines in order to move the waypoint away from obstacles (walls, sandbags etc.)
* also it does reduce its range if reposition alone cannot handle the range intersecting the obstacle
*/
void SelfControlledWaypointReposition(float &the_range, Vector &new_origin, float move_d, float dec_r, bool dont_move, edict_t *pentIgnore)
{
	Vector start, end;
	TraceResult tr, otr;
	bool stop = false;

	// the X-coordinate
	do
	{		
		// we are tracing the range so 'start' is origin - range and the 'end' is origin + range
		start = new_origin - Vector(the_range, 0, 0);
		end = new_origin + Vector(the_range, 0, 0);

		// we have to trace the range to both sides
		// we are going from the waypoint origin (the centre) towards both edges of the range
		// the origin must always be free so the trace will always be valid
		UTIL_TraceLine(new_origin, start, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &tr);
		UTIL_TraceLine(new_origin, end, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &otr);

		// is the range free?
		if (tr.flFraction == 1.0 && otr.flFraction == 1.0)
		{
			stop = true;
		}
		// did we hit anything then see if we can reposition the waypoint or if we can only reduce the range
		else
		{
			// we can ignore the hit only if the entity is not solid or it's a trigger
			if (tr.pHit->v.solid <= SOLID_BBOX || otr.pHit->v.solid < SOLID_BBOX)
				stop = true;
			else
			{
				if (tr.flFraction == 1.0 && otr.flFraction != 1.0 && !dont_move)
				{
					// move the waypoint origin towards start because there was everything okay
					new_origin = new_origin - Vector(move_d, 0, 0);

					// and try a new traceline using the new origins this time
					start = new_origin - Vector(the_range, 0, 0);
					end = new_origin + Vector(the_range, 0, 0);
					
					UTIL_TraceLine(new_origin, start, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &tr);
					UTIL_TraceLine(new_origin, end, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &otr);

					// if all was okay then we can stop checking the x coord
					if (tr.flFraction == 1.0 && otr.flFraction == 1.0)
					{
						stop = true;
					}
					// otherwise reduce the range and try again
					else
					{
						the_range -= dec_r;
					}

					continue;					
				}
				else if (tr.flFraction != 1.0  && otr.flFraction == 1.0 && !dont_move)
				{			
					// move the waypoint origin towards end because there was everything okay
					new_origin = new_origin + Vector(move_d, 0, 0);

					start = new_origin - Vector(the_range, 0, 0);
					end = new_origin + Vector(the_range, 0, 0);
					
					UTIL_TraceLine(new_origin, start, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &tr);
					UTIL_TraceLine(new_origin, end, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &otr);

					if (tr.flFraction == 1.0 && otr.flFraction == 1.0)
					{
						stop = true;
					}
					else
					{
						the_range -= dec_r;
					}

					continue;
				}
				// both sides are blocked ... we can only reduce the range in this case
				else
				{
					the_range -= dec_r;
				}
			}
		}

		// stop it when the range is just too small
		if (the_range <= dec_r)
			stop = true;

	} while (!stop);

	// reset the stop state before startting the other coordinate do-while cycle
	stop = false;
	// the Y-coordinate
	do
	{
		start = new_origin - Vector(0, the_range, 0);
		end = new_origin + Vector(0, the_range, 0);

		UTIL_TraceLine(new_origin, start, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &tr);
		UTIL_TraceLine(new_origin, end, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &otr);

		// is the range free?
		if (tr.flFraction == 1.0 && otr.flFraction == 1.0)
			stop = true;
		// did we hit anything
		else
		{
			// we can ignore the hit only if the entity is not solid or it's a trigger
			if (tr.pHit->v.solid <= SOLID_BBOX || otr.pHit->v.solid < SOLID_BBOX)
				stop = true;
			else
			{
				if (tr.flFraction == 1.0 && otr.flFraction != 1.0 && !dont_move)
				{
					// move the waypoint origin towards start because there was everything okay
					new_origin = new_origin - Vector(0, move_d, 0);

					start = new_origin - Vector(0, the_range, 0);
					end = new_origin + Vector(0, the_range, 0);

					UTIL_TraceLine(new_origin, start, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &tr);
					UTIL_TraceLine(new_origin, end, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &otr);

					if (tr.flFraction == 1.0 && otr.flFraction == 1.0)
					{
						stop = true;
					}
					else
					{
						the_range -= dec_r;
					}

					continue;
				}
				else if (tr.flFraction != 1.0 && otr.flFraction == 1.0 && !dont_move)
				{
					// move the waypoint origin towards end because there was everything okay
					new_origin = new_origin + Vector(0, move_d, 0);

					start = new_origin - Vector(0, the_range, 0);
					end = new_origin + Vector(0, the_range, 0);

					UTIL_TraceLine(new_origin, start, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &tr);
					UTIL_TraceLine(new_origin, end, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &otr);

					if (tr.flFraction == 1.0 && otr.flFraction == 1.0)
					{
						stop = true;
					}
					else
					{
						the_range -= dec_r;
					}
					
					continue;
				}
				else
				{
					the_range -= dec_r;
				}
			}
		}

		// stop it when the range is just too small
		if (the_range <= dec_r)
			stop = true;

	} while (!stop);

	stop = false;
	// the diamond part 1 (bottom right and top left lines)
	do
	{	
		start = new_origin + Vector(the_range, 0, 0);
		end = new_origin + Vector(0, the_range, 0);
		UTIL_TraceLine(start, end, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &tr);
		
		start = new_origin - Vector(the_range, 0, 0);
		end = new_origin - Vector(0, the_range, 0);
		UTIL_TraceLine(start, end, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &otr);

		// is the range free?
		if (tr.flFraction == 1.0 && otr.flFraction == 1.0)
			stop = true;
		// did we hit anything
		else
		{
			// we can ignore the hit only if the entity is not solid or it's a trigger
			if (tr.pHit->v.solid <= SOLID_BBOX || otr.pHit->v.solid < SOLID_BBOX)
				stop = true;
			else
			{
				if (tr.flFraction == 1.0 && otr.flFraction != 1.0 && !dont_move)
				{
					// move the waypoint origin towards start because there was everything okay
					new_origin = new_origin + Vector(move_d, move_d, 0);

					start = new_origin + Vector(the_range, 0, 0);
					end = new_origin + Vector(0, the_range, 0);
					UTIL_TraceLine(start, end, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &tr);

					start = new_origin - Vector(the_range, 0, 0);
					end = new_origin - Vector(0, the_range, 0);
					UTIL_TraceLine(start, end, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &otr);

					if (tr.flFraction == 1.0 && otr.flFraction == 1.0)
					{
						stop = true;
					}
					else
					{
						the_range -= dec_r;
					}
					
					continue;
				}
				else if (tr.flFraction != 1.0 && otr.flFraction == 1.0 && !dont_move)
				{	
					// move the waypoint origin towards end because there was everything okay
					new_origin = new_origin - Vector(move_d, move_d, 0);

					start = new_origin + Vector(the_range, 0, 0);
					end = new_origin + Vector(0, the_range, 0);
					UTIL_TraceLine(start, end, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &tr);
					
					start = new_origin - Vector(the_range, 0, 0);
					end = new_origin - Vector(0, the_range, 0);
					UTIL_TraceLine(start, end, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &otr);

					if (tr.flFraction == 1.0 && otr.flFraction == 1.0)
					{
						stop = true;
					}
					else
					{
						the_range -= dec_r;
					}
					
					continue;
				}
				else
				{
					the_range -= dec_r;
				}
			}
		}

		// stop it when the range is just too small
		if (the_range <= dec_r)
			stop = true;

	} while (!stop);


#ifdef _DEBUG
	// @@@@@@@@@@@@@@@@@@@@@		SHOW ME THE LINES
	if ((pentIgnore->v.origin - new_origin).Length2D() < 200.0 && FInViewCone(&new_origin, pentIgnore))
	{
		start = new_origin + Vector(the_range, 0, 0);
		end = new_origin + Vector(0, the_range, 0);
		DrawBeam(pentIgnore, start, end, 100, 255, 255, 0, 10);
	
		start = new_origin - Vector(the_range, 0, 0);
		end = new_origin - Vector(0, the_range, 0);
		DrawBeam(pentIgnore, start, end, 100, 125, 125, 0, 10);
	}
#endif
					
	
	stop = false;
	// the diamond part 2 (top right and bottom left lines ... if looking from x coordinate)
	do
	{
		start = new_origin + Vector(-the_range, 0, 0);
		end = new_origin + Vector(0, the_range, 0);
		UTIL_TraceLine(start, end, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &tr);

		start = new_origin - Vector(-the_range, 0, 0);
		end = new_origin - Vector(0, the_range, 0);
		UTIL_TraceLine(start, end, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &otr);

		// is the range free?
		if (tr.flFraction == 1.0 && otr.flFraction == 1.0)
			stop = true;
		// did we hit anything
		else
		{
			// we can ignore the hit only if the entity is not solid or it's a trigger
			if (tr.pHit->v.solid <= SOLID_BBOX || otr.pHit->v.solid < SOLID_BBOX)
				stop = true;
			else
			{
				if (tr.flFraction == 1.0 && otr.flFraction != 1.0 && !dont_move)
				{
					// move the waypoint origin towards start because there was everything okay
					new_origin = new_origin + Vector(-move_d, move_d, 0);

					start = new_origin + Vector(-the_range, 0, 0);
					end = new_origin + Vector(0, the_range, 0);
					UTIL_TraceLine(start, end, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &tr);
					
					start = new_origin - Vector(-the_range, 0, 0);
					end = new_origin - Vector(0, the_range, 0);
					UTIL_TraceLine(start, end, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &otr);

					if (tr.flFraction == 1.0 && otr.flFraction == 1.0)
					{
						stop = true;
					}
					else
					{
						the_range -= dec_r;
					}
					
					continue;
				}
				else if (tr.flFraction != 1.0 && otr.flFraction == 1.0 && !dont_move)
				{
					// move the waypoint origin towards end because there was everything okay
					new_origin = new_origin - Vector(-move_d, move_d, 0);

					start = new_origin + Vector(-the_range, 0, 0);
					end = new_origin + Vector(0, the_range, 0);
					UTIL_TraceLine(start, end, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &tr);

					start = new_origin - Vector(-the_range, 0, 0);
					end = new_origin - Vector(0, the_range, 0);
					UTIL_TraceLine(start, end, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &otr);

					if (tr.flFraction == 1.0 && otr.flFraction == 1.0)
					{
						stop = true;
					}
					else
					{
						the_range -= dec_r;
					}
					
					continue;
				}
				else
				{
					the_range -= dec_r;
				}
			}
		}
		
		// stop it when the range is just too small
		if (the_range <= dec_r)
			stop = true;

	} while (!stop);


#ifdef _DEBUG
	// @@@@@@@@@@@@@@@@@@@@@		SHOW ME THE LINES
	if ((pentIgnore->v.origin - new_origin).Length2D() < 200.0 && FInViewCone(&new_origin, pentIgnore))
	{
		start = new_origin + Vector(-the_range, 0, 0);
		end = new_origin + Vector(0, the_range, 0);
		DrawBeam(pentIgnore, start, end, 100, 255, 255, 125, 10);
		
		start = new_origin - Vector(-the_range, 0, 0);
		end = new_origin - Vector(0, the_range, 0);
		DrawBeam(pentIgnore, start, end, 100, 255, 125, 125, 10);
	}
#endif


	stop = false;
	// the diagonal part 1 (top left and bottom right quadrant ... if looking from x coordinate)
	do
	{
		start = new_origin - Vector(the_range, the_range, 0);
		end = new_origin + Vector(the_range, the_range, 0);

		UTIL_TraceLine(new_origin, start, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &tr);
		UTIL_TraceLine(new_origin, end, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &otr);

		// is the range free?
		if (tr.flFraction == 1.0 && otr.flFraction == 1.0)
			stop = true;
		// did we hit anything
		else
		{
			// we can ignore the hit only if the entity is not solid or it's a trigger
			if (tr.pHit->v.solid <= SOLID_BBOX || otr.pHit->v.solid < SOLID_BBOX)
				stop = true;
			else
			{
				if (tr.flFraction == 1.0 && otr.flFraction != 1.0 && !dont_move)
				{	
					// move the waypoint origin towards start because there was everything okay
					new_origin = new_origin - Vector(move_d, move_d, 0);

					start = new_origin - Vector(the_range, the_range, 0);
					end = new_origin + Vector(the_range, the_range, 0);
					
					UTIL_TraceLine(new_origin, start, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &tr);
					UTIL_TraceLine(new_origin, end, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &otr);

					if (tr.flFraction == 1.0 && otr.flFraction == 1.0)
					{
						stop = true;
					}
					else
					{
						the_range -= dec_r;
					}

					continue;
				}
				else if (tr.flFraction != 1.0 && otr.flFraction == 1.0 && !dont_move)
				{
					// move the waypoint origin towards end because there was everything okay
					new_origin = new_origin + Vector(move_d, move_d, 0);

					start = new_origin - Vector(the_range, the_range, 0);
					end = new_origin + Vector(the_range, the_range, 0);
			
					UTIL_TraceLine(new_origin, start, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &tr);
					UTIL_TraceLine(new_origin, end, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &otr);

					if (tr.flFraction == 1.0 && otr.flFraction == 1.0)
					{
						stop = true;
					}
					else
					{
						the_range -= dec_r;
					}
					
					continue;
				}
				else
				{
					the_range -= dec_r;
				}
			}
		}
		
		// stop it when the range is just too small
		if (the_range <= dec_r)
			stop = true;

	} while (!stop);


#ifdef _DEBUG
	// @@@@@@@@@@@@@@@@@@@@@		SHOW ME THE LINES
	if ((pentIgnore->v.origin - new_origin).Length2D() < 200.0 && FInViewCone(&new_origin, pentIgnore))
	{
		start = new_origin - Vector(the_range, the_range, 0);
		end = new_origin + Vector(the_range, the_range, 0);
		DrawBeam(pentIgnore, start, end, 100, 255, 125, 125, 10);
	}
#endif


	stop = false;
	// the diagonal part 2 (top right and bottom left quadrant ... if looking from x coordinate)
	do
	{
		start = new_origin + Vector(the_range, -the_range, 0);
		end = new_origin - Vector(the_range, -the_range, 0);

		UTIL_TraceLine(new_origin, start, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &tr);
		UTIL_TraceLine(new_origin, end, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &otr);

		// is the range free?
		if (tr.flFraction == 1.0 && otr.flFraction == 1.0)
			stop = true;
		// did we hit anything
		else
		{
			// we can ignore the hit only if the entity is not solid or it's a trigger
			if (tr.pHit->v.solid <= SOLID_BBOX || otr.pHit->v.solid < SOLID_BBOX)
				stop = true;
			else
			{
				if (tr.flFraction == 1.0 && otr.flFraction != 1.0 && !dont_move)
				{
					// move the waypoint origin towards start because there was everything okay
					new_origin = new_origin + Vector(move_d, -move_d, 0);

					start = new_origin + Vector(the_range, -the_range, 0);
					end = new_origin - Vector(the_range, -the_range, 0);
					
					UTIL_TraceLine(new_origin, start, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &tr);
					UTIL_TraceLine(new_origin, end, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &otr);

					if (tr.flFraction == 1.0 && otr.flFraction == 1.0)
					{
						stop = true;
					}
					else
					{
						the_range -= dec_r;						
					}

					continue;
				}
				else if (tr.flFraction != 1.0 && otr.flFraction == 1.0 && !dont_move)
				{
					// move the waypoint origin towards end because there was everything okay
					new_origin = new_origin - Vector(move_d, -move_d, 0);

					start = new_origin + Vector(the_range, -the_range, 0);
					end = new_origin - Vector(the_range, -the_range, 0);
					
					UTIL_TraceLine(new_origin, start, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &tr);
					UTIL_TraceLine(new_origin, end, dont_ignore_monsters, dont_ignore_glass, pentIgnore, &otr);

					if (tr.flFraction == 1.0 && otr.flFraction == 1.0)
					{
						stop = true;
					}
					else
					{
						the_range -= dec_r;
					}

					continue;
				}
				else
				{
					the_range -= dec_r;
				}
			}
		}
		
		// stop it when the range is just too small
		if (the_range <= dec_r)
			stop = true;

	} while (!stop);


#ifdef _DEBUG
// @@@@@@@@@@@@@@@@@@@@@		SHOW ME THE LINES
	if ((pentIgnore->v.origin - new_origin).Length2D() < 200.0 && FInViewCone(&new_origin, pentIgnore))
	{
		start = new_origin + Vector(the_range, -the_range, 0);
		end = new_origin - Vector(the_range, -the_range, 0);
		DrawBeam(pentIgnore, start, end, 100, 195, 125, 125, 10);
	}
#endif


	return;
}


#ifdef _DEBUG
/*
* dumps all waypoints with their basic info into debugging file
*/
void Wpt_Dump(void)
{
	extern void DumpVector(FILE *f, Vector vec);

	FILE *f;

	UTIL_DebugDev("***New dump call***", -100, -100);

	f = fopen(debug_fname, "a");

	for (int i = 0; i < num_waypoints; i++)
	{
		fprintf(f, "WPT #%d -- flag  %d", i, waypoints[i].flags);

		if (waypoints[i].flags & (W_FL_STD | W_FL_CROUCH | W_FL_PRONE | W_FL_JUMP | \
			W_FL_DUCKJUMP | W_FL_SPRINT | W_FL_AMMOBOX | W_FL_DOOR | W_FL_DOORUSE | \
			W_FL_LADDER | W_FL_USE | W_FL_CHUTE | W_FL_SNIPER | W_FL_MINE | W_FL_FIRE | \
			W_FL_AIMING | W_FL_CROSS | W_FL_GOBACK | W_FL_PUSHPOINT | W_FL_TRIGGER))
		{
			char flags[256];
			WptGetType(i, flags);
			fprintf(f, "   --->> Known flags (%s)\n", flags);
		}
		else if (waypoints[i].flags & W_FL_DELETED)
			fprintf(f, "   --->> known DELETE flag\n");
		else
			fprintf(f, "   --->> UNKNOWN >>> ERROR\n");

		fprintf(f, "WPT #%d -- origin ", i);
		DumpVector(f, waypoints[i].origin);
	}

	fclose(f);
}

/*
* dumps all waypoints with their basic info into debugging file
*/
void Pth_Dump(void)
{
	extern void DumpVector(FILE *f, Vector vec);

	FILE *f;

	UTIL_DebugDev("***New dump call***", -100, -100);

	f = fopen(debug_fname, "a");

	for (int i = 0; i < num_w_paths; i++)
	{
		fprintf(f, "PTH #%d", i);

		if (w_paths[i] == NULL)
		{
			fprintf(f, " -- NOT USED/DELETED\n");
			continue;
		}

		fprintf(f, " -- flag  %d", w_paths[i]->flags);

		if (w_paths[i]->flags & (P_FL_TEAM_NO | P_FL_TEAM_RED | P_FL_TEAM_BLUE | \
			P_FL_WAY_ONE | P_FL_WAY_TWO | P_FL_WAY_PATROL | \
			P_FL_CLASS_ALL | P_FL_CLASS_SNIPER | P_FL_CLASS_MGUNNER))
		{
			char the_type[256];

			int flags = w_paths[i]->flags;
			strcpy(the_type, "");

			if (flags & P_FL_TEAM_NO)
				strcat(the_type, "both teams ");
			if (flags & P_FL_TEAM_RED)
				strcat(the_type, "team red ");
			if (flags & P_FL_TEAM_BLUE)
				strcat(the_type, "team blue ");
			if (flags & P_FL_WAY_ONE)
				strcat(the_type, "one-way ");
			if (flags & P_FL_WAY_TWO)
				strcat(the_type, "two-way ");
			if (flags & P_FL_WAY_PATROL)
				strcat(the_type, "patrol ");
			if (flags & P_FL_CLASS_ALL)
				strcat(the_type, "all classes ");
			if (flags & P_FL_CLASS_SNIPER)
				strcat(the_type, "snipers ");
			if (flags & P_FL_CLASS_MGUNNER)
				strcat(the_type, "mgunners ");

			//if (Wpt_CountFlags(wpt_index) < 1)
			//	strcat(the_type, "unknown ");

			int length = strlen(the_type);
			the_type[length-1] = '\0';

			fprintf(f, "   --->> Known flags (%s)\n", the_type);
		}
		else
			fprintf(f, "   --->> UNKNOWN >>> ERROR\n");
	}

	fclose(f);

}

/*
* just testing method no real usage
*/
void Wpt_Check(void)
{
	int messed = 0;
	FILE *f = NULL;
	bool init = FALSE;

	for (int i = 0; i < num_waypoints; i++)
	{
		if ((waypoints[i].flags & W_FL_DELETED) && (waypoints[i].flags & W_FL_STD))
		{
			messed++;

			if (init == FALSE)
			{
				UTIL_DebugDev("***New dump call***", -100, -100);
				init = TRUE;
			}

			f = fopen(debug_fname, "a");
			fprintf(f, "WPT #%d -- flag  %d\n", i, waypoints[i].flags);
		}
	}

	if (f)
		fclose(f);

	ALERT(at_console, "There's %d messed wpts\n", messed);
}

// TEMP: at least for now, used to highlight bots aiming location
void ReDrawBeam(edict_t *pEdict, Vector start, Vector end, int team)
{
	if (pEdict == NULL)
		return;

	int duration = 2;

	if (team == TEAM_RED)
		DrawBeam(pEdict, start, end, duration, 250, 0, 0, 250);
	else if (team == TEAM_BLUE)
		DrawBeam(pEdict, start, end, duration, 0, 0, 250, 250);
	else
		DrawBeam(pEdict, start, end, duration, 0, 250, 0, 250);
}

// moves all waypoints by given vector
void ShiftWpts(int x, int y, int z)
{
	for (int i = 0; i < num_waypoints; i++)
	{
		if ((waypoints[i].flags == 0) || (waypoints[i].flags == W_FL_DELETED))
			continue;

		waypoints[i].origin = waypoints[i].origin + Vector(x, y, z);
	}
}

#endif	// _DEBUG