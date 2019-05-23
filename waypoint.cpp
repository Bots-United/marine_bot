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
// (http://www.marinebot.tk)
//
//
// waypoint.cpp
// 
////////////////////////////////////////////////////////////////////////////////////////////////

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
#include "waypoint.h"

#define RANGE_20_WPT (W_FL_AMMOBOX | W_FL_DOOR | W_FL_DOORUSE | W_FL_JUMP | W_FL_DUCKJUMP | \
						W_FL_LADDER | W_FL_USE)

extern int m_spriteTexture;
extern int m_spriteTexturePath1;
extern int m_spriteTexturePath2;
extern int m_spriteTexturePath3;

extern bool debug_waypoints;
extern bool check_aims;
extern bool check_cross;
extern bool check_ranges;

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


bool g_waypoint_on = FALSE;		// to show wpts
float g_draw_distance = 800;	// default draw distance (i.e. wpt that is this far from the player will show on screen)
float wp_display_time[MAX_WAYPOINTS];	// time that this waypoint was displayed (while editing)
float f_ranges_display_time[MAX_WAYPOINTS];	// time that this range was displayed (while editing)
float f_conn_time[MAX_WAYPOINTS][MAX_WAYPOINTS];	// time the connection (cross&aim) was displayed while editing

bool g_auto_waypoint = FALSE;
float g_auto_wpt_distance = 200;	// default distance between wpts when autowpting

bool g_path_waypoint_on = FALSE;	// to show paths
bool g_waypoint_paths = FALSE;		// have any paths been allocated?
bool g_auto_path = FALSE;			// auto add "touched" waypoint in actual path
int g_path_to_continue = -1;		// index of path that is currently in edit
int g_path_last_wpt = -1;			// index of last wpt added to current path (for g_auto_path) NOTE: might be useful for tests if the path do not lead through obstacle
float f_path_time[MAX_W_PATHS];		// time this path was diplayed (while editing)

Vector last_waypoint;				// for autowaypointing

bool b_custom_wpts = FALSE;			// allow custom waypoints (read from different folder)
char wpt_author[32];
char wpt_modified[32];

bool debug_cross = FALSE;		// for cross behaviour
bool debug_paths = FALSE;		// for path debug
extern int highlight_this_path;	// to show only chosen paths

static FILE *fp;


// few functions prototypes used in this file
void WaypointDebug(void);
void FreeAllPaths(void);
void FreeAllTriggers(void);
void WaypointInit(int wpt_index);
bool IsMessageValid(char *msg);
int WaypointReturnTriggerWaypointPriority(int wpt_index, int team);
int TriggerNameToIndex(const char *trigger_name);
TRIGGER_NAME TriggerNameToId(const char *trigger_name);
TRIGGER_NAME TriggerIndexToId(int i);
void TriggerIntToName(int id, char *trigger_name);
int WaypointIsPathOkay(int path_index);
int WaypointFixInvalidFlags(int wpt_index);
void UpdatePathStatus(int path_index);
void UpdateGoalPathStatus(int wpt_index, int path_index);
int FindPath(int current_wpt_index);
int GetPathStart(int path_index);
int GetPathEnd(int path_index);
int GetPathPreviousWaypoint(int wpt_index, int path_index);
bool IsPath(int path_index, PATH_TYPES path_type);
bool PathMatchingFlag(int path_index1, int path_index2, PATH_TYPES path_type);
bool PathCompareTeamDownwards(int checked_path, int standard_path);
bool PathCompareClassDownwards(int checked_path, int standard_path);
int ContinueCurrPath(int current_wpt_index);
int ExcludeFromPath(int current_wpt_index, int path_index);
bool DeleteWholePath(int path_index);
W_PATH *GetWaypointPointer(int wpt_index, int path_index);
W_PATH *GetWaypointTypePointer(WPT_TYPES flag_type, int path_index);
bool WaypointTypeIsInPath(WPT_TYPES flag_type, int path_index);
void WaypointSetValue(bot_t *pBot, int wpt_index, WAYPOINT_VALUE *wpt_value);
bool IsWaypointPossible(bot_t *pBot, int wpt_index);
int WaypointFindNearestType(edict_t *pEntity, float range, int type);
int WaypointFindNearestCross(const Vector &source_origin);
int WaypointFindNearestCross(const Vector &source_origin, bool see_through_doors);
int WaypointFindNearestStandard(int end_waypoint, int path_index);
int WaypointFindAimingAround(int source_waypoint);
int Wpt_CountFlags(int wpt_index);
WPT_TYPES WaypointAdd(const Vector position, WPT_TYPES wpt_type);
int PathClassFlagCount(int path_index);
void GetPathClass(int path_index, char *the_class);
bool WaypointReachable(Vector v_srv, Vector v_dest, edict_t *pEntity);
void WptGetType(int wpt_index, char *the_type);
void Wpt_SetSize(int wpt_index, Vector &start, Vector &end);
Vector Wpt_SetColor(int wpt_index);
int Wpt_SetPathTexture(int path_index);
Vector Wpt_SetPathColor(int path_index);


/*
* returns true if the path passed by path_index has an ignore enemy flag
*/
bool bot_t::IsIgnorePath(int path_index)
{
	if (path_index == -1)
		path_index = curr_path_index;

	// this will handle passing through a cross waypoint the bot won't remove the ignore enemy flag in this case,
	// but will wait for next path to decide if the flag should or should not be removed
	if (IsWaypoint(curr_wpt_index, wpt_cross) || IsWaypoint(prev_wpt_index.get(0), wpt_cross))
		return TRUE;

	if (IsPath(path_index, path_ignore))
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
	SetName(trigger_none);
	SetUsed(false);
	SetTriggered(false);
	SetTime(0.0);
}


void trigger_event_gamestate_t::Reset(int i)
{
	if (i == -1)
		SetName(trigger_none);
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
bool WaypointSubscribe(char *signature, bool author)
{
	char mapname[64];
	char filename[256];
	WAYPOINT_HDR header;

	strcpy(mapname, STRING(gpGlobals->mapname));
	strcat(mapname, ".wpt");

	if (b_custom_wpts)
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
			// is wpt file NOT subcribed so access granted to change wpt_author (no file writing)
			if ((strcmp(header.author, "") == 0) || (strcmp(header.author, "unknown") == 0))
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
* if "author" is TRUE return waypoints author
* if "author" is FALSE return signature of the one who modified them
*/
char *WaypointAuthors(bool author)
{
	char mapname[64];
	char filename[256];
	WAYPOINT_HDR header;

	strcpy(mapname, STRING(gpGlobals->mapname));
	strcat(mapname, ".wpt");

	if (b_custom_wpts)
		UTIL_MarineBotFileName(filename, "customwpts", mapname);
	else
		UTIL_MarineBotFileName(filename, "defaultwpts", mapname);

	FILE *bfp = fopen(filename, "rb");

	if (bfp != NULL)
	{
		fread(&header, sizeof(header), 1, bfp);

		// return authors signature if exists
		if (author)
		{
			// is wpt file NOT subscribed
			if ((strcmp(header.author, "") == 0) ||
				(strcmp(header.author, "unknown") == 0))
			{
				fclose(bfp);
				return "noauthor";
			}
			// otherwise return the signature
			else
			{
				fclose(bfp);
				return header.author;
			}
		}
		// return signature of guy who modified them if exists
		else
		{
			// is wpt file NOT subscribed
			if ((strcmp(header.modified_by, "") == 0) ||
				(strcmp(header.modified_by, "unknown") == 0))
			{
				fclose(bfp);
				return "nosig";
			}
			// otherwise return the signature
			else
			{
				fclose(bfp);
				return header.modified_by;
			}
		}
	}
	else
	{
		return "nofile";
	}
}


/*
* initialize just one waypoint
*/
void WaypointInit(int wpt_index)
{
	waypoints[wpt_index].flags = W_FL_DELETED;
	waypoints[wpt_index].red_priority = MAX_WPT_PRIOR;		// lowest priority
	waypoints[wpt_index].red_time = 0.0;					// no "stay here" time
	waypoints[wpt_index].blue_priority = MAX_WPT_PRIOR;
	waypoints[wpt_index].blue_time = 0.0;
	waypoints[wpt_index].trigger_red_priority = MAX_WPT_PRIOR;
	waypoints[wpt_index].trigger_blue_priority = MAX_WPT_PRIOR;
	waypoints[wpt_index].trigger_event_on = trigger_none;	// no trigger event
	waypoints[wpt_index].trigger_event_off = trigger_none;
	waypoints[wpt_index].range = WPT_RANGE;					// default range
	waypoints[wpt_index].origin = Vector(0,0,0);			// located at map origin
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

	last_waypoint = Vector(0,0,0);
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
	if (b_custom_wpts)
		UTIL_MarineBotFileName(filename, "customwpts", mapname);
	else
		UTIL_MarineBotFileName(filename, "defaultwpts", mapname);

	// and then erase it from HDD
	remove(filename);

	strcpy(mapname, STRING(gpGlobals->mapname));
	strcat(mapname, ".pth");
	
	if (b_custom_wpts)
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
	if (debug_waypoints)
		ALERT(at_console, "All triggers have been set back to \"not triggered\"\n");

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
	//if (debug_waypoints)
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
	//if (debug_waypoints)
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
			if (debug_waypoints)
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

	if (IsWaypoint(wpt_index, wpt_trigger))
	{
		// this is ugly conversion
		char name[32];

		TriggerIntToName(waypoints[wpt_index].trigger_event_on, name);
		TRIGGER_NAME trigger_on = TriggerNameToId(name);

		TriggerIntToName(waypoints[wpt_index].trigger_event_off, name);
		TRIGGER_NAME trigger_off = TriggerNameToId(name);

		float trigger_on_time, trigger_off_time;
		trigger_on_time = trigger_off_time = 0.0;

		if (trigger_on != trigger_none)
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
					if (trigger_off != trigger_none &&		// is there any trigger_off event set for this waypoint
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
TRIGGER_NAME TriggerNameToId(const char *trigger_name)
{
	if (FStrEq(trigger_name, "trigger1"))
		return trigger1;
	else if (FStrEq(trigger_name, "trigger2"))
		return trigger2;
	else if (FStrEq(trigger_name, "trigger3"))
		return trigger3;
	else if (FStrEq(trigger_name, "trigger4"))
		return trigger4;
	else if (FStrEq(trigger_name, "trigger5"))
		return trigger5;
	else if (FStrEq(trigger_name, "trigger6"))
		return trigger6;
	else if (FStrEq(trigger_name, "trigger7"))
		return trigger7;
	else if (FStrEq(trigger_name, "trigger8"))
		return trigger8;

	return trigger_none;
}


/*
* convert trigger array index to trigger ID
*/
TRIGGER_NAME TriggerIndexToId(int i)
{
	if (i == 0)
		return trigger1;
	else if (i == 1)
		return trigger2;
	else if (i == 2)
		return trigger3;
	else if (i == 3)
		return trigger4;
	else if (i == 4)
		return trigger5;
	else if (i == 5)
		return trigger6;
	else if (i == 6)
		return trigger7;
	else if (i == 7)
		return trigger8;
	
	return trigger_none;
}


/*
* convert trigger ID to trigger name (string)
*/
void TriggerIntToName(int i, char *trigger_name)
{
	if (i == trigger_none)
		strcpy(trigger_name, "no event");
	else if ((i >= trigger1) && (i <= trigger8))
	{
		sprintf(trigger_name, "trigger%d", i);
	}
	else
		strcpy(trigger_name, "UKNOWN");
}
/*
void TriggerIntToName(int i, char *trigger_name)
{
	if (i == trigger_none)
		strcpy(trigger_name, "no event");
	else if (i == trigger1)
		strcpy(trigger_name, "trigger1");
	else if (i == trigger2)
		strcpy(trigger_name, "trigger2");
	else if (i == trigger3)
		strcpy(trigger_name, "trigger3");
	else if (i == trigger4)
		strcpy(trigger_name, "trigger4");
	else if (i == trigger5)
		strcpy(trigger_name, "trigger5");
	else if (i == trigger6)
		strcpy(trigger_name, "trigger6");
	else if (i == trigger7)
		strcpy(trigger_name, "trigger7");
	else if (i == trigger8)
		strcpy(trigger_name, "trigger8");
	else
		strcpy(trigger_name, "UKNOWN");
}
*/


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
		waypoints[index].trigger_event_on = TriggerNameToId(trigger_name);
	else
		waypoints[index].trigger_event_off = TriggerNameToId(trigger_name);

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
		waypoints[index].trigger_event_on = trigger_none;
	else
		waypoints[index].trigger_event_off = trigger_none;

	return 1;
}


/*
* check path for various problems and fix them if possible
*/
int WaypointIsPathOkay(int path_index)
{
	W_PATH *p;
	int prev_path_wpt = -1;
	int safety_stop = 0;
	bool there_was_error = FALSE;

	p = w_paths[path_index];

	while (p)
	{
#ifdef _DEBUG
		if (safety_stop > 1000)
		{
			ALERT(at_error, "WaypointIsPathOkay() | LLERROR\n***Path no. %d\n", path_index + 1);

			WaypointDebug();
		}
#endif

		// the same waypoint added twice in a row problem
		if (p->wpt_index == prev_path_wpt)
		{
			// return error if we can't fix it
			if (ExcludeFromPath(p->wpt_index, path_index) != 1)
				return -1;

			there_was_error = TRUE;
		}

		// cross or aim waypoint cannot be part of path
		// (this shouldn't happen, because it's handled in other methods, but just in case)
		if (waypoints[p->wpt_index].flags & (W_FL_CROSS | W_FL_AIMING))
		{
			if (ExcludeFromPath(p->wpt_index, path_index) != 1)
				return -1;

			there_was_error = TRUE;
		}
		
		safety_stop++;

		// update prev waypoint record
		prev_path_wpt = p->wpt_index;
		
		// go to/check the next node
		p = p->next;
	}

	// there was some error that had been fixed so we have to return "warning"
	if (there_was_error)
		return 1;

	return 0;
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
	if (path_index == -1 || w_paths[path_index] == NULL)
		return -1;

	int end_waypoint = -1;

	// if it is a one way path then we will check just the path end
	if (IsPath(path_index, path_one))
	{
		end_waypoint = GetPathEnd(path_index);

		// check validity
		if (end_waypoint == -1)
			return -1;

		// check for cases when a one-way path ends with a goback waypoint
		if (IsWaypoint(end_waypoint, wpt_goback))
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
		if (!WaypointTypeIsInPath(wpt_chute, path_index) && !WaypointTypeIsInPath(wpt_duckjump, path_index) &&
			!WaypointTypeIsInPath(wpt_jump, path_index))
		{
			w_paths[path_index]->flags &= ~P_FL_WAY_ONE;
			w_paths[path_index]->flags |= P_FL_WAY_TWO;

			// and make the end waypoint a goback one unless there already is a turn back marker set
			if (waypoints[end_waypoint].flags & (W_FL_GOBACK | W_FL_AMMOBOX | W_FL_USE))
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
	if (waypoints[end_waypoint].flags & (W_FL_GOBACK | W_FL_AMMOBOX | W_FL_USE))
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
	if (waypoints[end_waypoint].flags & (W_FL_GOBACK | W_FL_AMMOBOX | W_FL_USE))
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

	for (int path_index = 0; path_index < num_w_paths; path_index++)
	{
		result = WaypointRepairInvalidPathEnd(path_index);

		switch (result)
		{
			case 1:
				ALERT(at_console, "path #%d was repaired\n", path_index + 1);
				break;
			case 10:
				ALERT(at_console, "unable to repair path #%d\n", path_index + 1);
				break;
		}
	}

	return;
}


/*
* check path end whether there doesn't start or end another path without cross waypoint connection
* (ie. the end waypoint isn't connected to a cross waypoint yet there starts or ends another path)
* we are also checking for either of the turn back markers (goback, ammobox or use)
* return -1 if some error occured, 0 if the path was okay
* return 1 if there was anything repaired, 2 if there was detected suspicious connection
* return 3 if there was anything repaired as well as detected suspicious connection
*/
int WaypointRepairInvalidPathMerge(int path_index)
{
	// first check the validity
	if (path_index == -1 || w_paths[path_index] == NULL)
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
			g_path_to_continue = -1;
			g_path_last_wpt = -1;

			// we should also check our path for errors
			WaypointIsPathOkay(path_index);
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


/*
* go through all paths and check them for invalid path merge
* (ie. the end waypoint isn't connected to a cross waypoint yet there starts another path)
* 
*/
void WaypointRepairInvalidPathMerge(void)
{
	int result;

	for (int path_index = 0; path_index < num_w_paths; path_index++)
	{
		result = WaypointRepairInvalidPathMerge(path_index);

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
		if (!IsWaypoint(end_waypoint, wpt_sniper))
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
	if (!missing_aim_waypoint && (IsPath(path_index, path_sniper) || IsPath(path_index, path_mgunner)))
	{
		// set wait time on this waypoint
		waypoints[end_waypoint].red_time = 60.0;
		waypoints[end_waypoint].blue_time = 60.0;

		if (!IsWaypoint(end_waypoint, wpt_sniper))
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
			if (WaypointAdd(aim_origin, wpt_aim) == wpt_aim)
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

	p = w_paths[path_index];

	while (p)
	{
#ifdef _DEBUG
		if (safety_stop > 1000)
		{
			ALERT(at_error, "UpdatePathStatus() | LLERROR\n***Path no. %d\n", path_index + 1);

			WaypointDebug();
		}
#endif
		if (IsWaypoint(p->wpt_index, wpt_ammobox))
			UTIL_SetBit(P_FL_MISC_AMMO, w_paths[path_index]->flags);

		if (IsWaypoint(p->wpt_index, wpt_flag))
			UpdateGoalPathStatus(p->wpt_index, path_index);

		safety_stop++;
		p = p->next;
	}
}


/*
* will scan the surrounding of the pushpoint/flag waypoints for the pushpoint entity
* if we find it then it we'll set correct goal path flag on all paths going over this waypoint
*/
void UpdateGoalPathStatus(int wpt_index, int path_index)
{
	if ((!IsWaypoint(wpt_index, wpt_flag)) || (path_index == -1))
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
* all return values: index of a path that match, -1 = wpt isn't in any path
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

			safety_stop++;

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
bool IsPath(int path_index, PATH_TYPES path_type)
{
	if ((path_index == -1) || (path_type == default_flagtype))
		return FALSE;

	if (w_paths[path_index] == NULL)
		return FALSE;

	int flag = 0;

	if (path_type == path_both)
		flag = P_FL_TEAM_NO;
	else if (path_type == path_red)
		flag = P_FL_TEAM_RED;
	else if (path_type == path_blue)
		flag = P_FL_TEAM_BLUE;
	else if (path_type == path_one)
		flag = P_FL_WAY_ONE;
	else if (path_type == path_two)
		flag = P_FL_WAY_TWO;
	else if (path_type == path_patrol)
		flag = P_FL_WAY_PATROL;
	else if (path_type == path_all)
		flag = P_FL_CLASS_ALL;
	else if (path_type == path_sniper)
		flag = P_FL_CLASS_SNIPER;
	else if (path_type == path_mgunner)
		flag = P_FL_CLASS_MGUNNER;
	else if (path_type == path_ammo)
		flag = P_FL_MISC_AMMO;
	else if (path_type == path_goal_red)
		flag = P_FL_MISC_GOAL_RED;
	else if (path_type == path_goal_blue)
		flag = P_FL_MISC_GOAL_BLUE;
	else if (path_type == path_avoid)
		flag = P_FL_MISC_AVOID;
	else if (path_type == path_ignore)
		flag = P_FL_MISC_IGNORE;
	else if (path_type == path_gitem)
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
bool PathMatchingFlag(int path_index1, int path_index2, PATH_TYPES path_type)
{
	if ((path_index1 == -1) || (path_index1 == -1) || (path_type == default_flagtype))
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
	if (IsPath(restrictive_path, path_red))
	{
		// we can always access any red team only path or any both team path
		if (PathMatchingFlag(restrictive_path, checked_path, path_red) || IsPath(checked_path, path_both))
			return TRUE;
	}

	if (IsPath(restrictive_path, path_blue))
	{
		if (PathMatchingFlag(restrictive_path, checked_path, path_blue) || IsPath(checked_path, path_both))
			return TRUE;
	}

	// from both team path ...
	if (IsPath(restrictive_path, path_both))
	{
		// we can always access only another both team path
		if (PathMatchingFlag(restrictive_path, checked_path, path_both))
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
	if (IsPath(restrictive_path, path_sniper))
	{
		// access another sniper path or path for all classes
		if (PathMatchingFlag(restrictive_path, checked_path, path_sniper) || IsPath(checked_path, path_all))
			return TRUE;
	}

	if (IsPath(restrictive_path, path_mgunner))
	{
		if (PathMatchingFlag(restrictive_path, checked_path, path_mgunner) || IsPath(checked_path, path_all))
			return TRUE;
	}

	if (IsPath(restrictive_path, path_all))
	{
		if (PathMatchingFlag(restrictive_path, checked_path, path_all))
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

	p = static_cast<W_PATH *>(malloc(sizeof(W_PATH)));	// create new head node

	if (p == NULL)
	{
		ALERT(at_error, "MarineBot - Error allocating memory for path!\n");

		return FALSE;
	}

	p->wpt_index = current_wpt_index;	// save the wpt index to first/head node

	p->prev = NULL;		// no previous not yet (this one it the first - the head node)
	p->next = NULL;		// no next node yet

	w_paths[free_path_index] = p;	// store the pointer to new path

	// init the flags value just for sure
	w_paths[free_path_index]->flags = 0;

	// set all flags to default
	w_paths[free_path_index]->flags |= P_FL_TEAM_NO | P_FL_CLASS_ALL | P_FL_WAY_TWO;

	g_path_to_continue = free_path_index;	// save path index to continue in
	g_path_last_wpt = current_wpt_index;	// save wpt index to obstacle test

	// increase total number of paths if adding at the end of the array
	if (free_path_index == num_w_paths)
		num_w_paths++;

	return TRUE;
}


/*
* continue in current path
* returns -3 if invalid wpt (ie this wpt type can't be in path)
* returns -2 if no path (ie g_path_to_continue is -1)
* returns -1 if not enough memory
* returns 0 if path doesn't exist (not sure if this can happen)
* returns 1 if everything is OK
*/
int ContinueCurrPath(int current_wpt_index)
{
	int path_index;

	// don't add these wpt types into paths
	if ((current_wpt_index == -1) ||
		(waypoints[current_wpt_index].flags & (W_FL_DELETED | W_FL_AIMING | W_FL_CROSS)))
		return -3;

	path_index = g_path_to_continue;

	if (path_index == -1)
		return -2;			// no path to continue

	// if the path is valid add current_wpt_index at the end of that path
	if (w_paths[path_index])
	{
		W_PATH *p = w_paths[path_index];	// set the pointer to the head node
		W_PATH *prev_node = NULL;		// temp pointer to previous node
		int safety_stop = 0;

		while (p)
		{
			prev_node = p;	// save the previous node in linked list

			p = p->next;	// go to next node in linked list

#ifdef _DEBUG
			safety_stop++;
			if (safety_stop > 1000)
				WaypointDebug();
#endif
		}

		p = static_cast<W_PATH *>(malloc(sizeof(W_PATH)));	// create new node

		if (p == NULL)
		{
			ALERT(at_error, "MarineBot - Error allocating memory for path!\n");

			return -1;		// no memory for path
		}
		
		p->wpt_index = current_wpt_index;	// store new wpt index
		
		p->next = NULL;			// NULL next node

		g_path_last_wpt = current_wpt_index;	// save wpt index to obstacle test and g_auto_path

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
*/
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

	// waypoint already in in this path
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

			new_node = static_cast<W_PATH *>(malloc(sizeof(W_PATH)));	// create new node

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

			new_node = static_cast<W_PATH *>(malloc(sizeof(W_PATH)));

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


/*
* remove current_wpt_index waypoint from specified (path_index) path
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
	if ((wpt_index == -1) || (path_index == -1))
		return FALSE;

	if (GetWaypointPointer(wpt_index, path_index) != NULL)
		return TRUE;	

	return FALSE;
}


/*
* return a pointer on a waypoint of given type if it is in the path otherwise return NULL
*/
W_PATH *GetWaypointTypePointer(WPT_TYPES flag_type, int path_index)
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
bool WaypointTypeIsInPath(WPT_TYPES flag_type, int path_index)
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
		if (debug_paths)
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

				if (IsPath(path_index, path_gitem) && pBot->IsTask(TASK_GOALITEM) && (chance < 95))
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

				if (IsPath(path_index, path_ammo) && pBot->IsNeed(NEED_AMMO) && (chance < 95))
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
				
				if (((IsPath(path_index, path_goal_red) && (pBot->bot_team == TEAM_RED)) ||
					(IsPath(path_index, path_goal_blue) && (pBot->bot_team == TEAM_BLUE))) &&
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
				
				if (IsPath(path_index, path_sniper) && pBot->IsBehaviour(SNIPER) && (chance < 65))
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
				
				if (IsPath(path_index, path_mgunner) && pBot->IsBehaviour(MGUNNER) && (chance < 65))
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
				
				if (IsPath(path_index, path_patrol) && pBot->IsBehaviour(DEFENDER) && (chance < 50))
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
					if (IsPath(path_index, path_gitem))
					{
						// then decide its replacing only with another carry goal item path
						if (IsPath(best_path[i], path_gitem) && (chance < 50))
							path_index = best_path[i];
					}
					// do we have a path that runs through pushpoint
					else if (IsPath(path_index, path_goal_red) || IsPath(path_index, path_goal_blue))
					{
						// carry goal item has higher value so always pick that one
						if (IsPath(best_path[i], path_gitem))
							path_index = best_path[i];

						// decide whether to pick the other pushpoint path or not
						if (IsPath(best_path[i], path_goal_red) || IsPath(best_path[i], path_goal_blue))
						{
							if (chance < 50)
								path_index = best_path[i];
						}
					}
					// do we have a path with an ammobox already
					else if (IsPath(path_index, path_ammo))
					{
						// carry item or pushpoint are more important
						if (IsPath(best_path[i], path_gitem) ||
							IsPath(best_path[i], path_goal_red) || IsPath(best_path[i], path_goal_blue))
							path_index = best_path[i];

						// if it is another ammobox path then decide
						if (IsPath(best_path[i], path_ammo) && (chance < 50))
							path_index = best_path[i];
					}
					// ... then there are all the other cases (pretty simple though)
					else
					{
						// these are the most important so always pick them
						if (IsPath(best_path[i], path_gitem) ||
							IsPath(best_path[i], path_goal_red) || IsPath(best_path[i], path_goal_blue))
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

			if (debug_cross)
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

				if (debug_cross)
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

				if (debug_cross)
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
		if (debug_paths)
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
	if (waypoints[wpt_index].flags & W_FL_AIMING)
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


/*														//		THIS ONE ISN'T USED ANYWHERE IN THE CODE
//Fill point_list from current path pointer to the end of path.
//forward -is direction of move trough the path, true-forward
//false - backward. 

int fillPointList(int *pl, int pl_size, W_PATH *w_path_ptr, bool forward)
{
	int i=0;
	if (forward) {
		do { //fill point list with new route in FORWARD way.
			*(pl+i) = w_path_ptr->wpt_index;
			++i;
			//if we see GOBACK wpt, we stop fill path_list.
			//next time when this created path will be ended
			//we will change direction in BotOnPath1 function
			if (waypoints[w_path_ptr->wpt_index].flags & W_FL_GOBACK
				&& i>1)
			{
				break;
			}
		} while ((w_path_ptr = w_path_ptr->next) != NULL && i<ROUTE_LENGTH-1 && i<pl_size);
	}
	else {
		do { //fill point list with new route in REVERSE way.
			*(pl+i) = w_path_ptr->wpt_index;
			++i;
			if (waypoints[w_path_ptr->wpt_index].flags & W_FL_GOBACK
				&& i>1)
			{
				break;
			}
		} while ((w_path_ptr = w_path_ptr->prev) != NULL && i<ROUTE_LENGTH-1 && i<pl_size);
	}
	*(pl+i) = -1; //close new current path.

	// by Frank: I've enclosed this part into debugging because common users don't need this
#ifdef _DEBUG
	FILE *f = fopen("\\debug.txt","a");
	if(f!=NULL) {
		for (int j=0; j<ROUTE_LENGTH-1; ++j) {
			fprintf(f,"_%d", *(pl+j));
			if (*(pl+j)==-1) break;
		}
		fprintf(f,"\n");
		fclose(f);
	}
#endif
	return i;
}

/*										//		THIS ONE ISN'T USED ANYWHERE IN THE CODE
//Set new path to the bot if this path is possible.
//If success return TRUE.
bool SetPossiblePath(bot_t *pBot, int wpt_index)
{
	// is wpt valid and are there any paths
	if ((wpt_index == -1) || (num_w_paths < 1))
		return FALSE;

	//clear current path
	pBot->clear_path();
	// go through all paths
	// first skip paths that doesn't match with bot team and/or class
	// and then check the rest for the wpt (wpt index)
	for (int path_index = 0; path_index < num_w_paths; path_index++)
	{
		// skip free slots
		if (w_paths[path_index] == NULL)
			continue;

		// is this path possible for bot (ie do teams and class restriction match)
		if (IsPathPossible(pBot, path_index))
		{
			W_PATH *w_path_ptr = FindPointInPath(wpt_index, path_index);
			if (w_path_ptr == NULL)
			{
				return FALSE; //cant find wpt in path.
			}
			// if wpt is closer to the path end AND NOT on one-way path
			// use opposite direction path moves
			if ((WaypointIsPathEndCloser(wpt_index, path_index)) &&
				!(w_paths[path_index]->flags & P_FL_WAY_ONE))
			{
				fillPointList(pBot->point_list, ROUTE_LENGTH, w_path_ptr, false);
			}
			else 
			{
				fillPointList(pBot->point_list, ROUTE_LENGTH, w_path_ptr, true);
			}
			return TRUE;
		}
	}

	return FALSE;
}
/**/
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
			if(WaypointReturnPriority(wpt_index, TEAM_RED) != 0) 
			{
				return true;
			}
			break;
		case TEAM_BLUE:
			if (WaypointReturnPriority(wpt_index, TEAM_BLUE) != 0) 
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
int WaypointReturnPriority(int wpt_index, int team)
{	
	if (wpt_index == -1)
		return 0;			// like no priority

	if (IsWaypoint(wpt_index, wpt_trigger))
		return WaypointReturnTriggerWaypointPriority(wpt_index, team);

	if (team == TEAM_RED)
		return waypoints[wpt_index].red_priority;

	if (team == TEAM_BLUE)
		return waypoints[wpt_index].blue_priority;

	return 0;
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
		if (WaypointReturnPriority(the_source, pBot->bot_team) != 0)
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
			distance = (waypoints[i].origin - waypoints[the_source].origin).Length();

			// waypoint must be far enough, but not too far
			// ie. must be in the range of this cross waypoint
			if ((distance > 10) && (distance <= waypoints[the_source].range))
			{
				// get the priority of this waypoint
				priority = WaypointReturnPriority(i, pBot->bot_team);

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

		if (debug_cross)
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
		if ((pBot->prev_path_index != -1) && (RANDOM_LONG(1, 100) < 90))
		{
			for (i = 0; i < num_found_wpt; i++)
			{
				// see if one of the found waypoints is part of the path the bot just followed
				if (WaypointIsInPath(w_index[i], pBot->prev_path_index))
				{
					// if so then set super low priority so this waypoint can barely be taken
					w_prior[i] = MAX_WPT_PRIOR + 1;

					if (debug_cross)
					{
						char msg[256];
						char wpt_flags[128];
						WptGetType(w_index[i], wpt_flags);

						sprintf(msg, "<<CROSS>>Lowering a chance to pick wpt (#%d<%s>) - it has just been used! (Path #%d)\n",
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
					if (last_value == wpt_value[i].wpt_value && choice < 50)
					{
						best_wpt = wpt_value[i].wpt_index;
						last_value = wpt_value[i].wpt_value;
					}
					// otherwise try to always pick the most weighted waypoint
					else if (last_value < wpt_value[i].wpt_value || choice < 5)
					{
						best_wpt = wpt_value[i].wpt_index;
						last_value = wpt_value[i].wpt_value;
					}
				}

				// we found the best waypoint for current situation so return it
				// ie. skip the priority based decision
				if (best_wpt != -1)
				{
					if (debug_cross)
					{
						char msg[256];
						char wpt_flags[128];
						WptGetType(best_wpt, wpt_flags);
						
						sprintf(msg, "<<CROSS>>Taking wpt based on current NEEDS (#%d<%s>)\n", best_wpt + 1, wpt_flags);
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

			// in 85% of time we're acting regular
			if (choice > 14)
			{
				// if there is only one waypoint with the highest priority then take it
				if (high_prior_count == 1)
				{
					the_found = w_index[0];
					
					if (debug_cross)
					{
						char msg[256];
						char wpt_flags[128];
						WptGetType(the_found, wpt_flags);

						sprintf(msg, "<<CROSS>>Taking wpt based on priority -> HIGHEST ONE is #%d<%s>\n", the_found + 1,
							wpt_flags);
						PrintOutput(NULL, msg);
					}
				}
				// there are more waypoints with the same priority 
				else
				{
					// so pick of of them
					choice = RANDOM_LONG(1, high_prior_count);
					the_found = w_index[choice - 1];
					
					if (debug_cross)
					{
						char msg[256];
						char wpt_flags[128];
						WptGetType(the_found, wpt_flags);

						sprintf(msg, "<<CROSS>>Taking wpt based on priority -> MULTIPLE same priority wpts -> picking this one (#%d<%s>)\n",
							the_found + 1, wpt_flags);
						PrintOutput(NULL, msg);
					}
				}
			}
			// otherwise acting irregular (less predictable)
			else
			{
				// see if there is at least one waypoint with higher priority (but not highest) that means ...
				// 1st position cannot be lowest priority because then there would be only lowest priority waypoints there
				// also position right behind the highest priority waypoint/waypoints cannot be default priority because then
				// there would be only highest priority and the rest would be default priority waypoints
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

						if (debug_cross)
						{
							char msg[256];
							char wpt_flags[128];
							WptGetType(the_found, wpt_flags);
							
							sprintf(msg, "<CROSS>>Ignoring ONLY highest priority -> picking waypoint (#%d<%s>)\n",
								the_found + 1, wpt_flags);
							PrintOutput(NULL, msg);
						}
					}
					// there must be more of them
					else
					{
						choice = high_prior_count + RANDOM_LONG(0, priority);
						the_found = w_index[choice];

						if (debug_cross)
						{
							char msg[256];
							char wpt_flags[128];
							WptGetType(the_found, wpt_flags);
							
							sprintf(msg, "<CROSS>>Ignoring ONLY highest priority -> picking waypoint (#%d<%s>) from total of %d like waypoints\n",
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
					
					if (debug_cross)
					{
						char msg[256];
						char wpt_flags[128];
						WptGetType(the_found, wpt_flags);
						
						sprintf(msg, "<<CROSS>>Ignoring all priorities -> entirely RANDOM PICK (#%d<%s>)\n",
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

			// skip the aim waypoints
			if (waypoints[i].flags & W_FL_AIMING)
				continue;

			// skip ladder waypoints if on end ladder
			if ((pBot->waypoint_top_of_ladder) && (waypoints[i].flags & W_FL_LADDER))
				continue;

			distance = (waypoints[i].origin - waypoints[the_source].origin).Length();

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
			if (debug_waypoints)
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

		// is this waypoint origin higher than max jump high then so skip it
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
int FindRightLadderWpt(bot_t *pBot)
{
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

		UTIL_TraceLine(waypoints[curr_index].origin, waypoints[i].origin, ignore_monsters, pBot->pEdict, &tr);

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
int WaypointFindNearestAiming(bot_t *pBot, Vector v_origin)
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

		distance = (v_origin - waypoints[index].origin).Length();

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
						if (IsPath(this_path, path_one) && i == GetPathEnd(this_path))
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
				if (!IsWaypoint(i, wpt_goback))
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
	if (source_waypoint == -1 || waypoints[source_waypoint].flags & W_FL_DELETED)
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
	if ((wpt_index == -1) || (waypoints[wpt_index].flags & W_FL_DELETED))
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
bool IsWaypoint(int wpt_index, WPT_TYPES flag_type)
{
	if ((wpt_index == -1) || (flag_type == default_flagtype))
		return FALSE;

	int flag = 0;
	//bool flag_level2 = FALSE;

	// convert the flag_type to a real waypoint flag
	// the order is set by how much are particular types used
	if (flag_type == wpt_normal)
		flag = W_FL_STD;
	else if (flag_type == wpt_crouch)
		flag = W_FL_CROUCH;
	else if (flag_type == wpt_prone)
		flag = W_FL_PRONE;
	else if (flag_type == wpt_cross)
		flag = W_FL_CROSS;
	else if (flag_type == wpt_aim)
		flag = W_FL_AIMING;
	else if (flag_type == wpt_no)
		flag = W_FL_DELETED;
	else if (flag_type == wpt_goback)
		flag = W_FL_GOBACK;
	else if (flag_type == wpt_sniper)
		flag = W_FL_SNIPER;
	else if (flag_type == wpt_jump)
		flag = W_FL_JUMP;
	else if (flag_type == wpt_duckjump)
		flag = W_FL_DUCKJUMP;
	else if (flag_type == wpt_ammobox)
		flag = W_FL_AMMOBOX;
	else if (flag_type == wpt_ladder)
		flag = W_FL_LADDER;
	else if (flag_type == wpt_sprint)
		flag = W_FL_SPRINT;
	else if (flag_type == wpt_fire)
		flag = W_FL_FIRE;
	else if (flag_type == wpt_mine)
		flag = W_FL_MINE;
	else if (flag_type == wpt_chute)
		flag = W_FL_CHUTE;
	else if (flag_type == wpt_flag)
		flag = W_FL_PUSHPOINT;
	else if (flag_type == wpt_trigger)
		flag = W_FL_TRIGGER;
	else if (flag_type == wpt_cover)
		flag = W_FL_COVER;
	else if (flag_type == wpt_bandages)
		flag = W_FL_BANDAGES;
	else if (flag_type == wpt_use)
		flag = W_FL_USE;
	else if (flag_type == wpt_door)
		flag = W_FL_DOOR;
	else if (flag_type == wpt_dooruse)
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
		;	// we don't need to add anything else, the flag is already set
	}
	else if (FStrEq(wpt_type, "ammobox"))
	{
		waypoints[index].flags |= W_FL_AMMOBOX;	// add another flag that makes this type
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
	}
	else
	{
		// reset the flag back to default ie to deleted waypoint
		waypoints[index].flags = W_FL_DELETED;

		return "unknown";
	}

	// get size/hight of the beam based on waypoint type
	Wpt_SetSize(index, start, end);

	// use temp vector to set color for this waypoint
	Vector color = Wpt_SetColor(index);

	// draw the waypoint
	WaypointBeam(pEntity, start, end, 30, 0, color.x, color.y, color.z, 250, 5);

	// store the origin (location) of this waypoint (use entity origin)
	waypoints[index].origin = pEntity->v.origin;

	// store the last used waypoint for the auto waypoint code...
	last_waypoint = pEntity->v.origin;

	// set the time that this waypoint was originally displayed...
	//wp_display_time[index] = gpGlobals->time;		//PREV CODE
	wp_display_time[index] = 0.0;

	// set the time for possible range highlighting
	//f_ranges_display_time[index] = gpGlobals->time;		//PREV CODE
	f_ranges_display_time[index] = 0.0;

	// set the time for possible in range status of any cross or aim wpt (to draw connection)
	//f_conn_time[index][index] = gpGlobals->time;			//PREV CODE
	f_conn_time[index][index] = 0.0;

	EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "weapons/xbow_hit1.wav", 1.0, ATTN_NORM, 0, 100);

	// increment total number of waypoints if adding at end of array...
	if (index == num_waypoints)
		num_waypoints++;

	return const_cast<char *>(wpt_type);
}

/*
* add new waypoint to given location and specify all waypoint values (priority, time etc.)
* return the type (normal, aim etc.) of that waypoint
*/
WPT_TYPES WaypointAdd(const Vector position, WPT_TYPES wpt_type)
{
	int index;
	//Vector start, end;
	
	if (num_waypoints >= MAX_WAYPOINTS)
		return wpt_no;

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
	if (wpt_type == wpt_no || wpt_type < 0)
	{
		wpt_type = wpt_normal;		
	}

	// reset this waypoint slot before using it
	WaypointInit(index);

	waypoints[index].flags = 0;		// no flag yet, must be to clear the deleted flag

	// set standard flag used as a default
	waypoints[index].flags |= W_FL_STD;

	// now add all other flags based on the waypoint type
	if (wpt_type == wpt_normal)
	{
		;	// we don't need to add anything else, the flag is already set
	}
	else if (wpt_type == wpt_ammobox)
	{
		waypoints[index].flags |= W_FL_AMMOBOX;	// add another flag that makes this type
		waypoints[index].range = (float) 20;
	}
	else if (wpt_type == wpt_chute)
	{
		waypoints[index].flags |= W_FL_CHUTE;
	}
	else if (wpt_type == wpt_crouch)
	{
		waypoints[index].flags |= W_FL_CROUCH;
	}
	else if (wpt_type == wpt_door)
	{
		waypoints[index].flags |= W_FL_DOOR;
		waypoints[index].range = (float) 20;
	}
	else if (wpt_type == wpt_dooruse)
	{
		waypoints[index].flags |= W_FL_DOORUSE;
		waypoints[index].range = (float) 20;
	}
	else if (wpt_type == wpt_fire)
	{
		waypoints[index].flags |= W_FL_FIRE;
	}
	else if (wpt_type == wpt_goback)
	{
		waypoints[index].flags |= W_FL_GOBACK;
	}
	else if (wpt_type == wpt_jump)
	{
		waypoints[index].flags |= W_FL_JUMP;
		waypoints[index].range = (float) 20;
	}
	else if (wpt_type == wpt_duckjump)
	{
		waypoints[index].flags |= W_FL_DUCKJUMP;
		waypoints[index].range = (float) 20;
	}
	else if (wpt_type == wpt_ladder)
	{
		waypoints[index].flags |= W_FL_LADDER;
		waypoints[index].range = (float) 20;
	}
	else if (wpt_type == wpt_mine)
	{
		waypoints[index].flags |= W_FL_MINE;
	}
	else if (wpt_type == wpt_prone)
	{
		waypoints[index].flags |= W_FL_PRONE;
	}
	else if (wpt_type == wpt_flag)
	{
		waypoints[index].flags |= W_FL_PUSHPOINT;
		waypoints[index].range = (float) 20;
	}
	else if (wpt_type == wpt_sprint)
	{
		waypoints[index].flags |= W_FL_SPRINT;
	}
	else if (wpt_type == wpt_sniper)
	{
		waypoints[index].flags |= W_FL_SNIPER;
	}
	else if (wpt_type == wpt_trigger)
	{
		waypoints[index].flags |= W_FL_TRIGGER;
	}
	else if (wpt_type == wpt_use)
	{
		waypoints[index].flags |= W_FL_USE;
		waypoints[index].range = (float) 20;
	}
	else if (wpt_type == wpt_aim)
	{
		waypoints[index].flags = 0;				// we need to clear the STD flag
		waypoints[index].flags |= W_FL_AIMING;
		waypoints[index].range = (float) 0;
	}
	else if (wpt_type == wpt_cross)
	{
		waypoints[index].flags = 0;
		waypoints[index].flags |= W_FL_CROSS;
		waypoints[index].range = WPT_RANGE * (float) 3;
	}
	else
	{
		// reset the flag back to default ie to deleted waypoint
		waypoints[index].flags = W_FL_DELETED;

		return wpt_no;
	}

	// get size/hight of the beam based on waypoint type
	//Wpt_SetSize(index, start, end);

	// use temp vector to set color for this waypoint
	//Vector color = Wpt_SetColor(index);

	// draw the waypoint
	//WaypointBeam(pEntity, start, end, 30, 0, color.x, color.y, color.z, 250, 5);

	// store the origin (location) of this waypoint (use entity origin)
	waypoints[index].origin = position;

	// store the last used waypoint for the auto waypoint code...
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

	// check if exists any paths
	if (num_w_paths >= 1)
	{
		// go through all paths
		for (int path_index = 0; path_index < num_w_paths; path_index++)
		{
			// remove waypoint from this path
			ExcludeFromPath(index, path_index);
		}
	}

	// call the init method which will reset the waypoint
	WaypointInit(index);

	wp_display_time[index] = 0.0;
	f_ranges_display_time[index] = 0.0;

	// clear the connections "sub array" too
	for (int j = 0; j < MAX_WAYPOINTS; j++)
		f_conn_time[index][j] = 0.0;

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
	if (g_path_to_continue != -1)
	{
		int path_status;

		path_status = WaypointIsPathOkay(g_path_to_continue);
		
		// test current path if there are no errors like the same wpt added twice in a row etc.
		if (path_status == 1)
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "there was an error in this path that has been automatically fixed\n");
		else if (path_status == -1)
			ClientPrint(pEntity, HUD_PRINTNOTIFY, "there is an error in this path that couldn't be fixed automatically\n");

		// also update this path status
		UpdatePathStatus(g_path_to_continue);

		g_path_to_continue = -1;	
		g_path_last_wpt = -1;	// clear last path waypoint

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
	if (path_index != -1 && w_paths[path_index] != NULL)
	{
		// update global "pointers"
		g_path_to_continue = path_index;

		g_path_last_wpt = GetPathEnd(path_index);

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

	// if auto_path is on the wpt must be really close
	if (g_auto_path)
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

	if (result > 0)
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
		path_index = g_path_to_continue;

	// if still no path search the first on current wpt
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
			UTIL_DebugDev("ExcludeFromPath()->unk\n", current_waypoint +1, path_index + 1);
			ALERT(at_console, "unknown error in ExcludeFromPath()\n");
#endif
		return FALSE;
	}

	return FALSE;
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

			// do we highlight red team accessible path AND path is NOT for red team
			if ((highlight_this_path == HIGHLIGHT_RED) &&
				!(w_paths[path_index]->flags & (P_FL_TEAM_NO | P_FL_TEAM_RED)))
				continue;	// so skip it
			// do we highlight blue team accessible path AND path is NOT for blue team
			else if ((highlight_this_path == HIGHLIGHT_BLUE) &&
				!(w_paths[path_index]->flags & (P_FL_TEAM_NO | P_FL_TEAM_BLUE)))
				continue;
			// do we highlight one-way path AND path is NOT one-way
			else if ((highlight_this_path == HIGHLIGHT_ONEWAY) &&
				!(w_paths[path_index]->flags & P_FL_WAY_ONE))
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

	// if not close to any wpt AND is any path actual (in edditing) so use it
	if ((path_index == -1) && (g_path_to_continue != -1))
		path_index = g_path_to_continue;

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
* all paths on nearest waypoint if we pass -10
* all used paths (ie. all stored in the path waypoint file for this map) if we pass -1
*/
bool WaypointPrintAllPaths(edict_t *pEntity, int wpt_index = -1)
{
	W_PATH *p;
	char *team_fl, *class_fl, *way_fl;
	char misc[64];
	char msg[128];
	// these 3 allow us to split the output and print 32 paths each time this method gets called
	// (it's used to deal with non steam console limits as well as limited steam console history)
	static int printed = 0;
	static int slot = 0;
	int max_on_screen[] = {32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 480, 512};

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

		// did we already use all lines/rows in this slot (ie. we printed 32 paths)
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

	// if not close to any wpt AND is any path actual (in edditing) so use it
	if ((path_index == -1) && (g_path_to_continue != -1))
		path_index = g_path_to_continue;

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

	// if not close to any waypoint AND is any path actual (in edditing) so use it
	if ((path_index == -1) && (g_path_to_continue != -1))
		path_index = g_path_to_continue;

	// change the team flag only if the path exist
	if (path_index != -1 && w_paths[path_index])
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

	if ((path_index == -1) && (g_path_to_continue != -1))
		path_index = g_path_to_continue;

	if (path_index != -1 && w_paths[path_index])
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

	if ((path_index == -1) && (g_path_to_continue != -1))
		path_index = g_path_to_continue;

	if (path_index != -1 && w_paths[path_index])
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

	if ((path_index == -1) && (g_path_to_continue != -1))
		path_index = g_path_to_continue;

	if (path_index != -1 && w_paths[path_index])
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
	bool show_paths_on = FALSE;		// TRUE if paths were turned on
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

	if (b_custom_wpts)
		UTIL_MarineBotFileName(filename, "customwpts", mapname);
	else
		UTIL_MarineBotFileName(filename, "defaultwpts", mapname);

	if (is_dedicated_server)
	{
		sprintf(msg, "loading paths file: %s\n", filename);
		PrintOutput(NULL, msg, msg_info);
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
				PrintOutput(pEntity, msg, msg_error);

				sprintf(msg, "Paths not loading!\n");
				PrintOutput(pEntity, msg, msg_warning);
				
				// "wpt load" command shouldn't let run this method at all, but just in case
				if (pEntity == NULL)
				{
					sprintf(msg, "Auto conversion started...\n");
					PrintOutput(pEntity, msg, msg_info);
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

				// is path showing on so turn it off
				if (g_path_waypoint_on)
				{
					g_path_waypoint_on = false;

					show_paths_on = TRUE;
				}

				// load and rebuild all paths
				for (int all_paths = 0; all_paths < num_w_paths; all_paths++)
				{
					// read the stored path_index
					fread(&path_index, sizeof(path_index), 1, bfp);

					p = static_cast<W_PATH *>(malloc(sizeof(W_PATH)));	// create new head node

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

					// init actual path (a must for ContinueCurrPath())
					g_path_to_continue = path_index;

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
						if (ContinueCurrPath(wpt_index) != 1)
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
				PrintOutput(pEntity, msg, msg_warning);
				
				sprintf(msg, "Bots will not play well this map!\n");
				PrintOutput(pEntity, msg, msg_info);

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
			PrintOutput(pEntity, msg, msg_warning);

			sprintf(msg, "Bots will not play well this map!\n");
			PrintOutput(pEntity, msg, msg_info);
			
			fclose(bfp);
			return 0;
		}

		fclose(bfp);
	}
	else
	{
		sprintf(msg, "path file not found! %s\n", filename);
		PrintOutput(pEntity, msg, msg_warning);

		sprintf(msg, "Bots will not play well this map!\n");
		PrintOutput(pEntity, msg, msg_info);

		return 0;
	}

	// clear actual path "pointer"
	g_path_to_continue = -1;

	// if the paths were shown then turn the draw paths back on
	if (show_paths_on)
		g_path_waypoint_on = TRUE;

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

	if (b_custom_wpts)
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
	bool show_paths_on = FALSE;		// TRUE if paths were turned on
	W_PATH *p = NULL;
	int path_index, path_length, flags, wpt_index;
	bool known;
	OLD_W_PATH *old_paths = NULL;	// we don't really need it here

	strcpy(mapname, STRING(gpGlobals->mapname));
	strcat(mapname, ".pth");
	
	if (b_custom_wpts)
		UTIL_MarineBotFileName(filename, "customwpts", mapname);
	else
		UTIL_MarineBotFileName(filename, "defaultwpts", mapname);

	if (is_dedicated_server)
	{
		sprintf(msg, "loading unsupported paths from: %s\n", filename);
		PrintOutput(NULL, msg, msg_info);
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
				PrintOutput(pEntity, msg, msg_info);

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
					PrintOutput(pEntity, msg, msg_info);
				}
				else
				{
					sprintf(msg, "unknown path file version (probably too old) - conversion failed!\n");
					PrintOutput(pEntity, msg, msg_error);

					fclose(bfp);
					return FALSE;
				}
			}

			if (header.waypoint_flag != num_waypoints)
			{
				sprintf(msg, "Waypoint file doesn't match to path file record! - conversion failed!\n");
				PrintOutput(pEntity, msg, msg_error);

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
				PrintOutput(pEntity, msg, msg_info);

				if (g_path_waypoint_on)
				{
					g_path_waypoint_on = FALSE;

					show_paths_on = TRUE;
				}

				// load and rebuild all paths
				for (int all_paths = 0; all_paths < num_w_paths; all_paths++)
				{
					// create new head node for the new path
					p = static_cast<W_PATH *>(malloc(sizeof(W_PATH)));

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

						// init actual path (a must for ContinueCurrPath())
						g_path_to_continue = path_index;

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
							if (ContinueCurrPath(wpt_index) != 1)
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
				PrintOutput(pEntity, msg, msg_warning);

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
			PrintOutput(pEntity, msg, msg_error);
			
			fclose(bfp);
			return FALSE;
		}

		fclose(bfp);
	}
	else
	{
		sprintf(msg, "Path file not found! %s\n", filename);
		PrintOutput(pEntity, msg, msg_warning);

		return FALSE;
	}

	// clear actual path "pointer"
	g_path_to_continue = -1;

	if (show_paths_on)
		g_path_waypoint_on = TRUE;

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
	bool show_paths_on = FALSE;		// TRUE if paths were turned on
	W_PATH *p = NULL;
	int path_index, path_length, flags, wpt_index;
	bool known;
	OLD_W_PATH *old_paths = NULL;	// we don't really need it here
	int OLD_WAYPOINT_VERSION_5 = 5;		// we have to "override" standard conversion by sending even older version number

	strcpy(mapname, STRING(gpGlobals->mapname));
	strcat(mapname, ".pth");
	
	if (b_custom_wpts)
		UTIL_MarineBotFileName(filename, "customwpts", mapname);
	else
		UTIL_MarineBotFileName(filename, "defaultwpts", mapname);

	if (is_dedicated_server)
	{
		sprintf(msg, "loading unsupported paths from: %s\n", filename);
		PrintOutput(NULL, msg, msg_info);
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
				PrintOutput(pEntity, msg, msg_info);

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
					PrintOutput(pEntity, msg, msg_info);
				}
				else
				{
					sprintf(msg, "unknown path file version (probably too old) - conversion failed!\n");
					PrintOutput(pEntity, msg, msg_error);

					fclose(bfp);
					return FALSE;
				}
			}

			if (header.waypoint_flag != num_waypoints)
			{
				sprintf(msg, "Waypoint file doesn't match to path file record! - conversion failed!\n");
				PrintOutput(pEntity, msg, msg_error);

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
				PrintOutput(pEntity, msg, msg_info);

				if (g_path_waypoint_on)
				{
					g_path_waypoint_on = FALSE;

					show_paths_on = TRUE;
				}

				// load and rebuild all paths
				for (int all_paths = 0; all_paths < num_w_paths; all_paths++)
				{
					// create new head node
					p = static_cast<W_PATH *>(malloc(sizeof(W_PATH)));

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

						// init actual path (a must for ContinueCurrPath())
						g_path_to_continue = path_index;

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
							if (ContinueCurrPath(wpt_index) != 1)
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
				PrintOutput(pEntity, msg, msg_warning);

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
			PrintOutput(pEntity, msg, msg_error);
			
			fclose(bfp);
			return FALSE;
		}

		fclose(bfp);
	}
	else
	{
		sprintf(msg, "Path file not found! %s\n", filename);
		PrintOutput(pEntity, msg, msg_warning);

		return FALSE;
	}

	// clear actual path "pointer"
	g_path_to_continue = -1;

	if (show_paths_on)
		g_path_waypoint_on = TRUE;

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

	if (b_custom_wpts)
		UTIL_MarineBotFileName(filename, "customwpts", mapname);
	else
		UTIL_MarineBotFileName(filename, "defaultwpts", mapname);

	if (is_dedicated_server)
	{
		sprintf(msg, "loading waypoint file: %s\n", filename);

		PrintOutput(NULL, msg, msg_info);
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
				PrintOutput(pEntity, msg, msg_error);

				sprintf(msg, "Waypoints not loading!\n");
				PrintOutput(pEntity, msg, msg_warning);

				// don't print this on user command "wpt load",
				// because in that case we don't autoconvert them
				if (pEntity == NULL)
				{
					sprintf(msg, "Auto conversion started...\n");
					PrintOutput(pEntity, msg, msg_info);
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
				PrintOutput(pEntity, msg, msg_warning);

				sprintf(msg, "Bots will not play well this map!\n");
				PrintOutput(pEntity, msg, msg_info);

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
			PrintOutput(pEntity, msg, msg_error);

			sprintf(msg, "Bots will not play well this map!\n");
			PrintOutput(pEntity, msg, msg_info);
			
			fclose(bfp);
			return 0;
		}

		fclose(bfp);
	}
	else
	{
		sprintf(msg, "Waypoint file not found! %s\n", filename);
		PrintOutput(pEntity, msg, msg_error);

		sprintf(msg, "Bots will not play well this map!\n");
		PrintOutput(pEntity, msg, msg_info);

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

	if (b_custom_wpts)
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

	if (b_custom_wpts)
		UTIL_MarineBotFileName(filename, "customwpts", mapname);
	else
		UTIL_MarineBotFileName(filename, "defaultwpts", mapname);

	if (is_dedicated_server)
	{
		sprintf(msg, "loading unsupported waypoints from: %s\n", filename);
		
		PrintOutput(NULL, msg, msg_info);
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
				PrintOutput(pEntity, msg, msg_info);

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
					PrintOutput(pEntity, msg, msg_info);
				}
				// otherwise ignore all other (older) waypoint versions
				else
				{
					sprintf(msg, "unknown waypoint file version (probably too old) - conversion failed!\n");
					PrintOutput(pEntity, msg, msg_error);

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
				PrintOutput(pEntity, msg, msg_info);

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
				PrintOutput(pEntity, msg, msg_warning);
				
				sprintf(msg, "Bots will not play well this map!\n");
				PrintOutput(pEntity, msg, msg_info);

				fclose(bfp);
				return FALSE;
			}
		}
		else
		{
			sprintf(msg, "Not MarineBot waypoint file! %s\n", filename);
			PrintOutput(pEntity, msg, msg_warning);
				
			sprintf(msg, "Bots will not play well this map!\n");
			PrintOutput(pEntity, msg, msg_info);

			fclose(bfp);
			return FALSE;
		}

		fclose(bfp);
	}
	else
	{
		sprintf(msg, "Waypoint file not found! %s\n", filename);
		PrintOutput(pEntity, msg, msg_warning);
				
		sprintf(msg, "Bots will not play well this map!\n");
		PrintOutput(pEntity, msg, msg_info);

		return FALSE;
	}

	return TRUE;
}


bool WaypointLoadVersion5(edict_t *pEntity)
{
	extern bool is_dedicated_server;

// version 5 waypoint structure (needed because there was a change in the structure itself)
typedef struct {
	int		flags;			// jump, crouch, button, lift, flag, ammo, etc.
	int		red_priority;	// 0-5 where 1 have highest priority for red team (0 - no priority, bot ingnores this waypoint)
	float	red_time;		// time the bot wait at this wpt for red team
	int		blue_priority;	// 0-5 where 1 have highest priority for blue team
	float	blue_time;		// time the bot wait at this wpt for blue team
	int		class_pref;		// -- NOT USED -- higher than priority based on bot class (NONE-all)
	float	range;			// circle around wpt where bot detects this waypoint
	int		misc;			// additional slot for various things
	Vector	origin;			// location of this waypoint in 3D space
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

	if (b_custom_wpts)
		UTIL_MarineBotFileName(filename, "customwpts", mapname);
	else
		UTIL_MarineBotFileName(filename, "defaultwpts", mapname);

	if (is_dedicated_server)
	{
		sprintf(msg, "loading unsupported waypoints from: %s\n", filename);
		
		PrintOutput(NULL, msg, msg_info);
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
				PrintOutput(pEntity, msg, msg_info);

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
					PrintOutput(pEntity, msg, msg_info);
				}
				// otherwise ignore all other (older) waypoint versions
				else
				{
					sprintf(msg, "unknown waypoint file version (probably too old) - conversion failed!\n");
					PrintOutput(pEntity, msg, msg_error);

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
				PrintOutput(pEntity, msg, msg_info);

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
				PrintOutput(pEntity, msg, msg_warning);
				
				sprintf(msg, "Bots will not play well this map!\n");
				PrintOutput(pEntity, msg, msg_info);

				fclose(bfp);
				return FALSE;
			}
		}
		else
		{
			sprintf(msg, "Not MarineBot waypoint file! %s\n", filename);
			PrintOutput(pEntity, msg, msg_warning);
				
			sprintf(msg, "Bots will not play well this map!\n");
			PrintOutput(pEntity, msg, msg_info);

			fclose(bfp);
			return FALSE;
		}

		fclose(bfp);
	}
	else
	{
		sprintf(msg, "Waypoint file not found! %s\n", filename);
		PrintOutput(pEntity, msg, msg_warning);
				
		sprintf(msg, "Bots will not play well this map!\n");
		PrintOutput(pEntity, msg, msg_info);

		return FALSE;
	}

	return TRUE;
}

/*
* autosaves waypoints and paths after given time to temporary files
*/
bool WaypointAutoSave(void)
{
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

	if (b_custom_wpts)
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

	if (b_custom_wpts)
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

	if (b_custom_wpts)
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
            if ((last_height - curr_height) > 45.0)
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
void WaypointPrintInfo(edict_t *pEntity, int info_level)
{
	char msg[256], special_info_r[80], special_info_b[80], comment[512];
	char steam_msg[512];
	int index, flags, red_prior, blue_prior;
	float red_time, blue_time, range;

	// find the nearest waypoint...
	index = WaypointFindNearest(pEntity, 50.0, -1);

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
			strcat(comment, "slow down & ");
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

	sprintf(msg,"Waypoint %d of %d total (flags\\tags= %s)\n", index + 1, num_waypoints, wpt_flags);

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

	sprintf(msg,"Waypoint %d of %d total (flags\\tags= %s)\n", index + 1, num_waypoints, wpt_flags);

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
	//sprintf(msg, "\tTrigger on event=\"%s\"\n", trigger_on);
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
			output = const_cast<char *>(new_type);
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
			output = const_cast<char *>(new_type);
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

		output = const_cast<char *>(new_type);
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
			output = const_cast<char *>(new_type);
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
			output = const_cast<char *>(new_type);
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
			output = const_cast<char *>(new_type);
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
			output = const_cast<char *>(new_type);
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
			output = const_cast<char *>(new_type);
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
			output = const_cast<char *>(new_type);
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
			output = const_cast<char *>(new_type);
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
			output = const_cast<char *>(new_type);
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
			output = const_cast<char *>(new_type);
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
			output = const_cast<char *>(new_type);
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
			output = const_cast<char *>(new_type);
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
			output = const_cast<char *>(new_type);
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
			output = const_cast<char *>(new_type);
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
			output = const_cast<char *>(new_type);
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
			output = const_cast<char *>(new_type);
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
			output = const_cast<char *>(new_type);
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
			output = const_cast<char *>(new_type);
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
	int index, new_priority, wpt_priority = -1, team;

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
	int index, new_priority, wpt_priority = -1, team;

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
	int index, team;
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
void Wpt_SetSize(int wpt_index, Vector &start, Vector &end)
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
Vector Wpt_SetColor(int wpt_index)
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
int Wpt_SetPathTexture(int path_index)
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
Vector Wpt_SetPathColor(int path_index)
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
	if (g_auto_waypoint)
	{
		// find the distance from the last used waypoint
		distance = (last_waypoint - pEntity->v.origin).Length();

		// is this wpt far enough to add new wpt
		if (distance > g_auto_wpt_distance)
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

					if (distance < min_distance)
						min_distance = distance;
				}
			}

			// make sure nearest waypoint is far enough away
			if (min_distance >= g_auto_wpt_distance)
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
	if (g_auto_path)
	{
		static int older_wpt = -1;
		int backup_wpt = -1;

		// is the edict NOT a bot
		if (!(pEntity->v.flags & FL_FAKECLIENT))
		{
			// search through all waypoints
			for (i = 0; i < num_waypoints; i++)
			{
				int result = -1;

				distance = (waypoints[i].origin - pEntity->v.origin).Length();

				// if player "touch" the waypoint AND the waypoint was NOT already added
				if ((distance <= AUTOADD_DISTANCE) && (i != g_path_last_wpt) && (i != older_wpt))
				{
					// make a copy of last path waypoint
					backup_wpt = g_path_last_wpt;

					result = ContinueCurrPath(i);	// add the waypoint in actual path
				}

				// if waypoint was added successfully play snd_done
				if (result == 1)
				{
					// update additional history of waypoint that has been previously added
					if (backup_wpt != g_path_last_wpt)
						older_wpt = backup_wpt;

					EMIT_SOUND_DYN2(pEntity, CHAN_WEAPON, "plats/elevbell1.wav", 1.0,
						ATTN_NORM, 0, 100);
				}
			}
		}
	}

	// display the waypoints if turned on
	if (g_waypoint_on)
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
				if ((distance < g_draw_distance) && (FInViewCone(&waypoints[i].origin, pEntity)))
				{
					// is path highlighting on and it's a specific (by index) path highlighting
					if ((highlight_this_path != -1) && (highlight_this_path != HIGHLIGHT_RED) &&
						(highlight_this_path != HIGHLIGHT_BLUE) &&
						(highlight_this_path != HIGHLIGHT_ONEWAY))
					{
						// if this waypoint is NOT in the path we want then skip it
						// (ie display only waypoints that are in this path)
						if (WaypointIsInPath(i, highlight_this_path) == FALSE)
							continue;
					}

					// get size/hight of the beam based on waypoint type
					Wpt_SetSize(i, start, end);

					// use temp vector to set color for this waypoint
					Vector color = Wpt_SetColor(i);

					// draw the waypoint
					WaypointBeam(pEntity, start, end, 30, 0, color.x, color.y, color.z, 250, 5);

					wp_display_time[i] = gpGlobals->time;
				}
			}
		}
	}

	// display the waypoint range if turned on
	if (check_ranges)
	{
		for (i = 0; i < num_waypoints; i++)
		{
			// skip all deleted, aim, cross, ladder and door waypoints
			// (some aren't used for navigation and others doesn't use range setting)
			if (waypoints[i].flags & (W_FL_DELETED | W_FL_AIMING | W_FL_CROSS | \
										W_FL_LADDER | W_FL_DOOR | W_FL_DOORUSE))
				continue;

			// needs this range to be refreshed
			if ((f_ranges_display_time[i] + 1.0) < gpGlobals->time)
			{
				distance = (waypoints[i].origin - pEntity->v.origin).Length();

				if ((distance < g_draw_distance) && (FInViewCone(&waypoints[i].origin, pEntity)))
				{
					if ((highlight_this_path != -1) && (highlight_this_path != HIGHLIGHT_RED) &&
						(highlight_this_path != HIGHLIGHT_BLUE) &&
						(highlight_this_path != HIGHLIGHT_ONEWAY))
					{
						if (WaypointIsInPath(i, highlight_this_path) == FALSE)
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
	if (g_path_waypoint_on)
	{
		int path_index;

		// go through all paths
		for (path_index = 0; path_index < num_w_paths; path_index++)
		{
			// if path highlighting is on AND this path is NOT the highlighted path skip it
			if ((highlight_this_path != -1) && (highlight_this_path != HIGHLIGHT_RED) &&
				(highlight_this_path != HIGHLIGHT_BLUE) &&
				(highlight_this_path != HIGHLIGHT_ONEWAY) && (path_index != highlight_this_path))
				continue;

			// exists the path AND is it time to refresh it
			if ((w_paths[path_index] != NULL) &&
				(f_path_time[path_index] + 1.0 <= gpGlobals->time))
			{
				W_PATH *p, *prev;
				float path_wpt_dist;	// distance between path waypoint and player
				
				// do we highlight red team accessible path AND path is NOT for red team
				if ((highlight_this_path == HIGHLIGHT_RED) &&
					!(w_paths[path_index]->flags & (P_FL_TEAM_NO | P_FL_TEAM_RED)))
					continue;	// so skip it
				// do we highlight blue team accessible path AND path is NOT for blue team
				else if ((highlight_this_path == HIGHLIGHT_BLUE) &&
					!(w_paths[path_index]->flags & (P_FL_TEAM_NO | P_FL_TEAM_BLUE)))
					continue;
				// do we highlight one-way path AND path is NOT one-way
				else if ((highlight_this_path == HIGHLIGHT_ONEWAY) && !(w_paths[path_index]->flags & P_FL_WAY_ONE))
					continue;

				int sprite = Wpt_SetPathTexture(path_index);
				Vector color = Wpt_SetPathColor(path_index);

				p = w_paths[path_index];
				prev = NULL;

				// go through this path
				while (p != NULL)
				{
					path_wpt_dist = (waypoints[p->wpt_index].origin - pEntity->v.origin).Length();

					// is player close enough to draw path from this node AND
					// is in view cone
					if ((path_wpt_dist < g_draw_distance)
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
	if (check_aims)
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
			if ((distance < g_draw_distance) && (FInViewCone(&waypoints[aim_wpt].origin, pEntity)))
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
	if (check_cross)
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
			if ((distance < g_draw_distance) &&
				(FInViewCone(&waypoints[cross_wpt].origin, pEntity)))
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

// just for testing, at least for now
void DrawBeam(edict_t *pEntity, Vector start, Vector end, int life,
        int red, int green, int blue, int speed)
{
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
	WRITE_SHORT( m_spriteTexture );
	WRITE_BYTE( 1 ); // framestart
	WRITE_BYTE( 10 ); // framerate
	WRITE_BYTE( life ); // life in 0.1's
	WRITE_BYTE( 10 ); // width
	WRITE_BYTE( 0 );  // noise
	
	WRITE_BYTE( red );   // r, g, b
	WRITE_BYTE( green );   // r, g, b
	WRITE_BYTE( blue );   // r, g, b
	
	WRITE_BYTE( 250 );   // brightness
	WRITE_BYTE( speed );    // speed
	MESSAGE_END();
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

