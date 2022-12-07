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
// bot_navigate.cpp
// 
////////////////////////////////////////////////////////////////////////////////////////////////

// added by kota@
#if defined(WIN32)
#pragma warning(disable:4786)
#endif

#if defined(WIN32)
#pragma warning(disable: 4005 91)
#endif

#include <cstring>

#include "extdll.h"
#include "util.h"
#include "cbase.h"

#include <cstdio>
#include <sys/types.h>
#include <cmath>

#include "bot.h"
#include "bot_func.h"
#include "bot_manager.h"
#include "waypoint.h"
#include "bot_weapons.h"


// few function prototypes used in this file
void UpdateWptHistory(bot_t *pBot);
Vector GenerateOrigin(int wpt_index);
int ForwardPathMove(bot_t *pBot, W_PATH *p, int &path_next_wpt, bool &path_end);
int BackwardPathMove(bot_t *pBot, W_PATH *p, int &path_next_wpt, bool &path_end);
void AssignPathBasedBehaviour(bot_t *pBot, int wpt_index);
//int getNextWptInPath(bot_t *pBot);		// not used
bool TraceForward(bot_t *pBot, bool check_head, bool check_feet, int dist, TraceResult *tr);
bool TraceJumpUp(bot_t *pBot, int height);

void BotFixIdealPitch(edict_t *pEdict)
{
	// check for wrap around of angle
	if (pEdict->v.idealpitch > 180)
		pEdict->v.idealpitch -= 360;

	if (pEdict->v.idealpitch < -180)
		pEdict->v.idealpitch += 360;
}


float BotChangePitch( bot_t *pBot, float speed )
{
	edict_t *pEdict = pBot->pEdict;
	float current_180;  // current +/- 180 degrees

	const float bipod_top_limit = 20.0;
	const float bipod_bottom_limit = 45.0;

	// turn from the current v_angle pitch to the idealpitch by selecting
	// the quickest way to turn to face that direction

	float current = pEdict->v.v_angle.x;

	const float ideal = pEdict->v.idealpitch;

	// is bot using bipod so limit his turn angles
	if (pBot->IsTask(TASK_BIPOD))
	{
		// in pEdict->v.vuser1 is stored v_view angle when player started using bipod
		// (ie the direction player faced when he used "bipod" command)

		// is current angle bigger than top limit angle (while the bot is looking upwards)
		// OR is current angle bigger than bottom limit angle (while the bot is looking downwards)
		if (((std::fabs(current) > bipod_top_limit) && (current < 0)) ||
			((std::fabs(current) > bipod_bottom_limit) && (current > 0)))
			current = pEdict->v.vuser1.x;	// reset to original value
	}

	// find the difference in the current and ideal angle
	const float diff = fabs(static_cast<double>(current) - ideal);

	// check if the bot is already facing the idealpitch direction
	if (diff <= 1)
		return diff;  // return number of degrees turned

	// check if difference is less than the max degrees per turn
	if (diff < speed)
		speed = diff;  // just need to turn a little bit (less than max)

	// we should make some difference between bots if the bot is trying to face an enemy
	// ie. we need to slow down worse bots more than better bots
	if (pBot->IsSubTask(ST_FACEENEMY))
	{
		// so we just take some constant and multiply it by the skill
		// which means no difference for best bots because skill is zero based variable
		const float speed_tweak = 4 * pBot->bot_skill;

		// and then we can modify the turning speed
		speed = speed - speed_tweak;
	}

	// here we have four cases, both angle positive, one positive and
	// the other negative, one negative and the other positive, or
	// both negative.  handle each case separately

	if ((current >= 0) && (ideal >= 0))  // both positive
	{
		if (current > ideal)
			current -= speed;
		else
			current += speed;
	}
	else if ((current >= 0) && (ideal < 0))
	{
		current_180 = current - 180;

		if (current_180 > ideal)
			current += speed;
		else
			current -= speed;
	}
	else if ((current < 0) && (ideal >= 0))
	{
		current_180 = current + 180;
		if (current_180 > ideal)
			current += speed;
		else
			current -= speed;
	}
	else  // (current < 0) && (ideal < 0)  both negative
	{
		if (current > ideal)
			current -= speed;
		else
			current += speed;
	}

	// check for wrap around of angle
	if (current > 180)
		current -= 360;
	if (current < -180)
		current += 360;

	pEdict->v.v_angle.x = current;

	return speed;  // return number of degrees turned
}


void BotFixIdealYaw(edict_t *pEdict)
{
	// check for wrap around of angle
	if (pEdict->v.ideal_yaw > 180)
		pEdict->v.ideal_yaw -= 360;

	if (pEdict->v.ideal_yaw < -180)
		pEdict->v.ideal_yaw += 360;
}


float BotChangeYaw( bot_t *pBot, float speed )
{
	edict_t *pEdict = pBot->pEdict;
	float current_180;  // current +/- 180 degrees

	// turn from the current v_angle yaw to the ideal_yaw by selecting
	// the quickest way to turn to face that direction

	float current = pEdict->v.v_angle.y;

	const float ideal = pEdict->v.ideal_yaw;

	if (pBot->IsTask(TASK_BIPOD))
	{
		const float bipod_limit = 45.0;

		// in pEdict->v.vuser1 is stored v_view angle when player started using bipod
		// (ie the direction player looked when he used "bipod" command)

		// is bot trying to turn more than bipod allows
		if (fabs(static_cast<double>(pEdict->v.vuser1.y) - current) > bipod_limit)
		{
			current = pEdict->v.vuser1.y;
		}
	}

	if (pBot->IsSubTask(ST_FACEENEMY))
	{
		const float speed_tweak = 4 * pBot->bot_skill;

		speed = speed - speed_tweak;
	}

	const float diff = fabs(static_cast<double>(current) - ideal);

	// the bot is basically facing the direction we wanted so we can remove the flag
	if (diff < 4)
	{
		// we do it only here, because yaw is the more important part
		// in making the bot reactions human like
		pBot->RemoveSubTask(ST_FACEENEMY);
	}

	if (diff <= 1)
	{
		return diff;
	}

	if (diff < speed)
		speed = diff;

	if ((current >= 0) && (ideal >= 0))
	{
		if (current > ideal)
			current -= speed;
		else
			current += speed;
	}
	else if ((current >= 0) && (ideal < 0))
	{
		current_180 = current - 180;

		if (current_180 > ideal)
			current += speed;
		else
			current -= speed;
	}
	else if ((current < 0) && (ideal >= 0))
	{
		current_180 = current + 180;
		if (current_180 > ideal)
			current += speed;
		else
			current -= speed;
	}
	else  // (current < 0) && (ideal < 0)  both negative
	{
		if (current > ideal)
			current -= speed;
		else
			current += speed;
	}
	
	if (current > 180)
		current -= 360;
	if (current < -180)
		current += 360;

	pEdict->v.v_angle.y = current;

	return speed;  // return number of degrees turned
}


/*
* remove oldest history record and move all others by one (to make free slot for new wpt)
*/
void UpdateWptHistory(bot_t *pBot)
{
	// don't update history if nothing changed
	if (pBot->prev_wpt_index.get() == pBot->curr_wpt_index)
	{
		return;
	}

	pBot->prev_wpt_index.push(pBot->curr_wpt_index);
	pBot->prev_wpt_origin = pBot->wpt_origin;
}


/*
* creates fake waypoint origin somewhere inside its range
* to make bots moves more random (human like)
* real waypoint origin stays untouched
* we are working only in planar dimension (ie ingoring z-coord)
*/
Vector GenerateOrigin(int wpt_index)
{
	// if really small range OR the waypoint is cross OR ladder OR door waypoint use real origin
	if ((waypoints[wpt_index].range <= 15) ||
		(waypoints[wpt_index].flags & (W_FL_CROSS | W_FL_LADDER | W_FL_DOOR | W_FL_DOORUSE)))
	{
		return waypoints[wpt_index].origin;
	}

	Vector new_origin = waypoints[wpt_index].origin;

	// get the range out of this wpt
	const float wpt_range = waypoints[wpt_index].range;

	// make the new x,y position
	float d_x = RANDOM_FLOAT(1, wpt_range - 1);
	float d_y = RANDOM_FLOAT(1, wpt_range - 1);

	// in 50% of time use also negative values for fake position
	if (RANDOM_LONG(1, 100) < 50)
		d_x *= (float) -1.0;

	if (RANDOM_LONG(1, 100) < 50)
		d_y *= (float) -1.0;

	// change waypoint x and y value
	new_origin.x += d_x;
	new_origin.y += d_y;

	// return the new fake origin
	return new_origin;
}


/*
* goes through given path and returns pointer to given waypoint node
* returns NULL if the waypoint isn't in this path
*/
/*
		CAN'T USE IT NOW - NEED A NEW HEADER FILE THAT WILL HOLD PATH STRUCTURE

W_PATH *GetWaypointFromPath (bot_t *pBot, int wpt_index, int path_index)
{
	// set it to head node
	W_PATH *pcur_path_node = w_paths[path_index];

	// go through the path node by node until the waypoint stored in one of those nodes do
	// match given waypoint
	while (pcur_path_node)
	{
		// we need to test if the bot already used this path node or not
		// to handle situations when there is the same waypoint used more than once in one path

		if ((pcur_path_node->wpt_index == wpt_index) &&
			(pcur_path_node != pBot->curr_path_node))
		{
			// return a pointer to a path node that holds the waypoint we've searched
			return pcur_path_node;
		}

		pcur_path_node = pcur_path_node->next;
	}

	return NULL;
}
*/


/* kota@
* this is the standard path move from Bot OnPath()
* I move it into separate function for easy understanding and manipulation
* Function set path_next_wpt, path_end.
* Function return status like Bot OnPath().
* 0 - need continue run Bot OnPath()
* 1 - return Bot OnPath() with the same status.
*/
int ForwardPathMove(bot_t *pBot, W_PATH *p, int &path_next_wpt, bool &path_end)
{
	// is standard path move (start -> end)
	

	// if the path ends AND is only one way path
	if ((p->next == nullptr) && (w_paths[pBot->curr_path_index]->flags & P_FL_WAY_ONE))
	{
		// search for waypoints around
		const int next_wpt = WaypointSearchNewInRange(pBot, p->wpt_index, pBot->curr_path_index);

		// is any new waypoint in range
		if (next_wpt != -1)
		{
			pBot->curr_wpt_index = next_wpt;
			pBot->wpt_origin = waypoints[next_wpt].origin;

			pBot->f_waypoint_time = gpGlobals->time;

			if (botdebugger.IsDebugPaths())
			{
				char fpmsg[128];
				sprintf(fpmsg, "\n***Reached the end of one-way path! - path #%d | curr_wpt #%d | dir is (start->end) | next wpt #%d\n\n",
					pBot->curr_path_index + 1, p->wpt_index + 1, pBot->curr_wpt_index + 1);
				HudNotify(fpmsg);
			}

			return 1;
		}
		// otherwise bot reached end of one-way path and didn't find any waypoint
		else
		{
			// set some wait time
			pBot->wpt_wait_time = gpGlobals->time + RANDOM_FLOAT(30.0, 120.0);

			// bot still has a waypoint
			pBot->f_waypoint_time = gpGlobals->time;

			if (botdebugger.IsDebugPaths())
			{
				char fpemsg[128];
				sprintf(fpemsg, "\n\n<<BUG IN WAYPOINTS>>Reached the end of one-way path, but didn't find wpt! - path #%d | last_wpt #%d\n\n\n",
					pBot->curr_path_index + 1, p->wpt_index + 1);
				HudNotify(fpemsg);
			}

			return -1;
		}
	}
	// if the path ends AND it's two way path use the path in opposite direction
	else if ((p->next == nullptr) && (w_paths[pBot->curr_path_index]->flags & P_FL_WAY_TWO))
	{
		// if path ends with goback and similar "turnback" waypoints
		if (waypoints[pBot->curr_wpt_index].flags & PATH_TURNBACK &&
			WaypointGetPriority(pBot->curr_wpt_index, pBot->bot_team) != 0)
		{
			if (p->prev != nullptr)
			{
				// change the direction bot had
				pBot->opposite_path_dir = TRUE;
					
				// set next waypoint - the previous one in llist
				path_next_wpt = p->prev->wpt_index;
			}
// REMOVE THIS - ONLY TESTING
#ifdef _DEBUG
			else
			{
				ALERT(at_console, "(<<OBSERVE INFO>>)What's the bot doing when this happens?\n");
				UTIL_DebugDev("start->end | two-way | wpt==goback/ammobox/use | prev == NULL",
					pBot->curr_wpt_index, pBot->curr_path_index);
			}
#endif
		}
		// otherwise try to find any new waypoint and path
		else
		{
			const int next_wpt = WaypointSearchNewInRange(pBot, p->wpt_index, pBot->curr_path_index);

			if (next_wpt != -1)
			{
				pBot->curr_wpt_index = next_wpt;
				pBot->wpt_origin = waypoints[next_wpt].origin;

				pBot->f_waypoint_time = gpGlobals->time;

				if (botdebugger.IsDebugPaths())
				{
					char fpmsg[128];
					sprintf(fpmsg, "\n***Reached the end of path! - path #%d | curr_wpt #%d | dir is (start->end) | next wpt #%d\n\n",
						pBot->curr_path_index + 1, p->wpt_index + 1, pBot->curr_wpt_index + 1);
					HudNotify(fpmsg);
				}

				return 1;
			}
			// otherwise wait there for a while and then turn back
			else
			{
				pBot->wpt_wait_time = gpGlobals->time + RANDOM_FLOAT(15.0, 60.0);

				if (p->prev != nullptr)
				{
					pBot->opposite_path_dir = TRUE;

					path_next_wpt = p->prev->wpt_index;
				}

				if (botdebugger.IsDebugPaths())
				{
					char fpemsg[128];
					sprintf(fpemsg, "\n\n<<BUG IN WAYPOINTS>>Reached the end of this path, but didn't find next wpt! - path #%d | curr_wpt #%d\n\n\n",
						pBot->curr_path_index + 1, p->wpt_index + 1);
					HudNotify(fpemsg);
				}

				return -1;
			}
		}	
	}
	// if the path ends AND it's a patrol path use the path in opposite direction
	else if ((p->next == nullptr) && (w_paths[pBot->curr_path_index]->flags & P_FL_WAY_PATROL))
	{
		// if path ends with these waypoints do nothing (further code does all we need)
		if (waypoints[pBot->curr_wpt_index].flags & PATH_TURNBACK &&
			WaypointGetPriority(pBot->curr_wpt_index, pBot->bot_team) != 0)
			;
		// otherwise if it does NOT end with "turnbacks" check
		// if there is a chance to leave this path
		else if (RANDOM_LONG(1, 100) < 15)
		{
			const int next_wpt = WaypointSearchNewInRange(pBot, p->wpt_index, pBot->curr_path_index);

			if (next_wpt != -1)
			{
				pBot->curr_wpt_index = next_wpt;
				pBot->wpt_origin = waypoints[next_wpt].origin;

				pBot->f_waypoint_time = gpGlobals->time;

				if (botdebugger.IsDebugPaths())
				{
					char ppmsg[128];
					sprintf(ppmsg, "\n***Reached patrol path end! - path #%d | curr_wpt #%d | dir is (start->end) | next wpt #%d\n\n",
						pBot->curr_path_index + 1, p->wpt_index + 1, pBot->curr_wpt_index + 1);
					HudNotify(ppmsg);
				}

				return 1;
			}
		}

		pBot->opposite_path_dir = TRUE;

		path_next_wpt = p->prev->wpt_index;
	}
	// otherwise get next waypoint to continue to
	else
	{
		if (p->next != nullptr)
		{
			path_next_wpt = p->next->wpt_index;

			// we didn't reached end yet
			path_end = FALSE;
		}
	}

	return 0;
}


/* kota@
* this is the standart path move from Bot OnPath().
* I move it into separate function for easy understanding and manipulation
* Function set path_next_wpt, path_end.
* Function return status like Bot OnPath().
* 0 - need continue run Bot OnPath()
* 1 - return Bot OnPath() with the same status.
* -1 - error , return Bot OnPath() with the same status.
*/
int BackwardPathMove(bot_t *pBot, W_PATH *p, int &path_next_wpt, bool &path_end)
{
	// if bot gets to one way path
	if (w_paths[pBot->curr_path_index]->flags & P_FL_WAY_ONE)
	{
#ifdef _DEBUG
		ALERT(at_console, "***DAMN there is a problem: Get on One-way path #%d in opposite direction (RESET)\n", pBot->curr_path_index + 1);

		UTIL_DebugDev("ERROR (Bad wpts): Bot entered One-way path from opposite direction!!!",
			pBot->curr_wpt_index, pBot->curr_path_index);
#endif

		pBot->opposite_path_dir = FALSE;

		return -1;
	}

	// if reached path start change back to standard path moves
	if ((p->prev == nullptr) && (w_paths[pBot->curr_path_index]->flags & P_FL_WAY_TWO))
	{
		// if path ends with goback and similar "turnback" waypoints
		if (waypoints[pBot->curr_wpt_index].flags & PATH_TURNBACK &&
			WaypointGetPriority(pBot->curr_wpt_index, pBot->bot_team) != 0)
		{
			if (p->next != nullptr)
			{
				// change the direction bot had
				pBot->opposite_path_dir = FALSE;

				// set next waypoint - the new one in llist
				path_next_wpt = p->next->wpt_index;
			}
		}
		// otherwise try to find any new waypoint and path
		else
		{
			const int next_wpt = WaypointSearchNewInRange(pBot, p->wpt_index, pBot->curr_path_index);

			if (next_wpt != -1)
			{
				pBot->curr_wpt_index = next_wpt;
				pBot->wpt_origin = waypoints[next_wpt].origin;

				pBot->f_waypoint_time = gpGlobals->time;

				if (botdebugger.IsDebugPaths())
				{
					char bpmsg[128];
					sprintf(bpmsg, "\n***Reached path start! - path #%d | curr_wpt #%d | dir is (end->start) | next wpt #%d\n\n",
						pBot->curr_path_index + 1, p->wpt_index + 1, pBot->curr_wpt_index + 1);
					HudNotify(bpmsg);
				}

				// clear this flag before return
				pBot->opposite_path_dir = FALSE;

				return 1;
			}
			// otherwise wait there for a while and then turn back
			else
			{
				pBot->wpt_wait_time = gpGlobals->time + RANDOM_FLOAT(15.0, 60.0);

				if (p->next != nullptr)
				{
					pBot->opposite_path_dir = FALSE;

					path_next_wpt = p->next->wpt_index;
				}

				if (botdebugger.IsDebugPaths())
				{
					char bpemsg[128];
					sprintf(bpemsg, "\n\n<<BUG IN WAYPOINTS>>Reached the end of this path, but didn't find next wpt! - path #%d | curr_wpt #%d\n\n\n",
						pBot->curr_path_index + 1, p->wpt_index + 1);
					HudNotify(bpemsg);
				}

				return -1;
			}
		}
	}
	// if reached path start change back to standard path moves
	else if ((p->prev == nullptr) && (w_paths[pBot->curr_path_index]->flags & P_FL_WAY_PATROL))
	{
		// if path ends with a turback do nothing (further code does all we need)
		if (waypoints[pBot->curr_wpt_index].flags & PATH_TURNBACK &&
			WaypointGetPriority(pBot->curr_wpt_index, pBot->bot_team) != 0)
			;
		// otherwise if does NOT end with "turnbacks" check
		// if there is a chance to leave this path
		else if (RANDOM_LONG(1, 100) < 25)
		{
			const int next_wpt = WaypointSearchNewInRange(pBot, p->wpt_index, pBot->curr_path_index);

			if (next_wpt != -1)
			{
				pBot->curr_wpt_index = next_wpt;
				pBot->wpt_origin = waypoints[next_wpt].origin;

				pBot->f_waypoint_time = gpGlobals->time;

				if (botdebugger.IsDebugPaths())
				{
					char ppmsg[128];
					sprintf(ppmsg, "\n***Reached patrol path start! - path #%d | curr_wpt #%d | dir is (end->start) | next wpt #%d\n\n",
						pBot->prev_path_index + 1, p->wpt_index + 1, pBot->curr_wpt_index + 1);
					HudNotify(ppmsg);
				}

				// clear this flag before return
				pBot->opposite_path_dir = FALSE;

				return 1;
			}
		}
			
		// change the direction bot had
		pBot->opposite_path_dir = FALSE;

		// set next waypoint - the new one in llist
		path_next_wpt = p->next->wpt_index;
	}
	// otherwise get next (this time from previous node in llist) waypoint to head to
	else
	{
		if (p->prev != nullptr)
		{
			path_next_wpt = p->prev->wpt_index;

			// we didn't reached end yet
			path_end = FALSE;
		}
	}

	return 0;
}


/*
* checks current path flags and sets correct behaviour based on it
*/
void AssignPathBasedBehaviour(bot_t *pBot, int wpt_index = -1)
{
	if (pBot->curr_path_index == -1 || w_paths[pBot->curr_path_index] == nullptr)
		return;

	// if actual path is PATROL path store last waypoint index (to return to it after combat)
	if (w_paths[pBot->curr_path_index]->flags & P_FL_WAY_PATROL)
		pBot->patrol_path_wpt = wpt_index;
	// if actual path isn't PATROL AND the index was set clear it
	else if ((pBot->patrol_path_wpt != -1) && !(w_paths[pBot->curr_path_index]->flags & P_FL_WAY_PATROL))
		pBot->patrol_path_wpt = -1;

	// set avoid enemy task if the path forces the bot to do so
	if (!pBot->IsTask(TASK_AVOID_ENEMY) && (w_paths[pBot->curr_path_index]->flags & P_FL_MISC_AVOID))
	{
		pBot->SetTask(TASK_AVOID_ENEMY);
		
		// keep these messages as alert at console
		// so the user can turn them on/off using the developer 1 command
		// these are just additional info
		if (botdebugger.IsDebugPaths())
			ALERT(at_console, "<<PATHS>>Bot %s will AVOID all enemies that aren't in current weapon range (path #%d)\n",
			pBot->name, pBot->curr_path_index + 1);
	}

	// remove avoid enemy task if the path is a "common path"
	if (pBot->IsTask(TASK_AVOID_ENEMY) && !(w_paths[pBot->curr_path_index]->flags & P_FL_MISC_AVOID))
	{
		pBot->RemoveTask(TASK_AVOID_ENEMY);

		if (botdebugger.IsDebugPaths())
			ALERT(at_console, "<<PATHS>>Bot %s will no longer AVOID enemies! Any visible foe can be a target again (path #%d)\n",
			pBot->name, pBot->curr_path_index + 1);
	}
	
	// set ignore enemy task if the path forces the bot to do so
	if (!pBot->IsTask(TASK_IGNORE_ENEMY) && (w_paths[pBot->curr_path_index]->flags & P_FL_MISC_IGNORE))
	{
		pBot->SetTask(TASK_IGNORE_ENEMY);
		
		if (botdebugger.IsDebugPaths())
			ALERT(at_console, "<<PATHS>>Bot %s will IGNORE most enemies now (path %d)\n", pBot->name, pBot->curr_path_index + 1);
	}
	
	// remove ignore enemy task if the path is a "common path"
	if (pBot->IsTask(TASK_IGNORE_ENEMY) && !(w_paths[pBot->curr_path_index]->flags & P_FL_MISC_IGNORE))
	{
		pBot->RemoveTask(TASK_IGNORE_ENEMY);

		if (botdebugger.IsDebugPaths())
			ALERT(at_console, "<<PATHS>>Bot %s will no longer IGNORE enemies! Any visible foe can be a target again (path #%d)\n",
			pBot->name, pBot->curr_path_index + 1);
	}

	return;
}


/*
* path navigation (also handles a path change)
* returns 1 = successful path based navigation,
* 0 = something went wrong but keep path navigation (ie don't try standard wpt-to-wpt navigation),
* -1 = error (try standard waypoint-to-waypoint navigation)
*/
int BotOnPath(bot_t *pBot)
{
	int path_next_wpt = -1;
	bool found = FALSE;
	bool found2 = FALSE;
	bool path_end = TRUE;
	W_PATH *p = nullptr;

	// get pointer to path node that holds current waypoint (ie. find current waypoint in path)
	if ((p = FindPointInPath (pBot->curr_wpt_index, pBot->curr_path_index)) != nullptr)
	// CAN'T USE NOW
	//if ((p = GetWaypointFromPath (pBot, pBot->curr_wpt_index, pBot->curr_path_index)) != NULL)
	{
		found = TRUE;
	}
	// current waypoint isn't in current path (ie bot must have changed path)
	else
	{
		// check all paths for current waypoint
		if (CheckPossiblePath(pBot, pBot->curr_wpt_index))
		{
			if ((p = FindPointInPath (pBot->curr_wpt_index, pBot->curr_path_index)) != nullptr )
			// CAN'T USE NOW
			// if ((p = GetWaypointFromPath (pBot, pBot->curr_wpt_index, pBot->curr_path_index)) != NULL )
			{
				found2 = TRUE;
			}
		}
	}

	// we found nothing ie current waypoint isn't in any path
	if (p == nullptr)
	{
		// update path history
		pBot->prev_path_index = pBot->curr_path_index;
		pBot->curr_path_index = -1;	// clear current path index

		return -1;
	}

	// is standard path move (start -> end)
	if ((pBot->opposite_path_dir == FALSE) && (path_next_wpt == -1))
	{
		const int forward_status = ForwardPathMove(pBot, p, path_next_wpt, path_end);
		
		if (forward_status != 0)
		{
			return forward_status;
		}
	}

	// is opposite direction path move (end -> start)
	if ((pBot->opposite_path_dir) && (path_next_wpt == -1))
	{
		const int backward_status = BackwardPathMove(pBot, p, path_next_wpt, path_end);

		if (backward_status != 0)
		{
			return backward_status;
		}
	}

	// current waypoint has been found in one path AND we also found next waypoint
	// in this path to continue to
	if (((found) || (found2)) && (path_next_wpt != -1))
	{
		// is the current waypoint a goback wpt and NOT end of the path
		if (IsFlagSet(W_FL_GOBACK, pBot->curr_wpt_index, pBot->bot_team) && !path_end)
		{
			// clear opposite_dir flag and set next waypoint in llist
			if ((pBot->opposite_path_dir) && (p->next != nullptr))
			{
				pBot->opposite_path_dir = FALSE;
				path_next_wpt = p->next->wpt_index;
			}
			// otherwise set opposite direction flag and use prev node waypoint
			else if ((pBot->opposite_path_dir == FALSE) && (p->prev != nullptr))
			{
				pBot->opposite_path_dir = TRUE;
				path_next_wpt = p->prev->wpt_index;
			}
		}

		// set next path waypoint as a waypoint to head towards
		pBot->curr_wpt_index = path_next_wpt;
		
		// generate next waypoint fake origin
		pBot->wpt_origin = GenerateOrigin(path_next_wpt);

		pBot->f_waypoint_time = gpGlobals->time;
		
		// set this time to prevent unwanted search for different waypoint
		// while bot needs to do bigger turning at current wpt
		// (ie if new waypoint is NOT in his view cone)
		if (FInViewCone(&pBot->wpt_origin, pBot->pEdict) == FALSE)
			pBot->f_face_wpt = gpGlobals->time + 0.6f;

		// set correct behaviour based on the path flags
		AssignPathBasedBehaviour(pBot, path_next_wpt);

		if (botdebugger.IsDebugPaths())
		{
			char pmsg[128];
			if (pBot->opposite_path_dir)
				sprintf(pmsg, "<<PATHS>>current path is #%d | current waypoint is #%d | path direction is (end->start)\n",
					pBot->curr_path_index + 1, pBot->curr_wpt_index + 1);
			else
				sprintf(pmsg, "<<PATHS>>current path is #%d | current waypoint is #%d | path direction is (start->end)\n",
					pBot->curr_path_index + 1, pBot->curr_wpt_index + 1);
			HudNotify(pmsg);
		}

		return 1;
	}

#ifdef _DEBUG
	char msg[80];
	sprintf(msg, "BotOnPath()->unknown event | bot.opp_dir %d\n", pBot->opposite_path_dir);
	UTIL_DebugDev(msg, pBot->curr_wpt_index, pBot->curr_path_index);
#endif

	return -1;
}


/*
* find the nearest next waypoint from the current waypoint
*/
bool BotFindWaypoint(bot_t *pBot, bool ladder)
{
	int next_waypoint = -1;

	//																						NOTE: As of now this cannot happen!!!
	//					It should probably be changed to check for movetype == MOVETYPE_FLY
	//					and scrap the bool ladder variable altogether.
	if (ladder)
	{
		next_waypoint = FindRightLadderWpt(pBot);
	}
	else
	{
		// look for nearby waypoints
		next_waypoint = WaypointFindAvailable(pBot);
	}

	// is there at least one
	if (next_waypoint != -1)
	{		
		// update visited wpts history; prevents the loop effect
		UpdateWptHistory(pBot);

		pBot->curr_wpt_index = next_waypoint;

		// generate next waypoint fake origin
		pBot->wpt_origin = GenerateOrigin(next_waypoint);

		pBot->f_waypoint_time = gpGlobals->time;

		// is new wpt NOT in view cone so set some time to allow bigger turn
		if (FInViewCone(&pBot->wpt_origin, pBot->pEdict) == FALSE)
			pBot->f_face_wpt = gpGlobals->time + 0.6f;

		return TRUE;
	}

	return FALSE;
}


/*
* head towards to your waypoint if close enough find next one
*/
bool BotHeadTowardWaypoint( bot_t *pBot )
{
	float wpt_distance, wpt_range;
	bool in_wpt_range;		// when bot reaches the wpt range
	bool past_the_wpt;		// if bot run past current wpt set this and then double check if really reach its range
	bool is_next_wpt;		// do we have next wpt to head to
	int unreachable_wpt_index = -1;	// prevents the bot to take the same wpt where he got stuck before
	bool use_only_planar;	// in cases when we need to test just the 2D distance to waypoint (eg. tiny wpt range)			NEW CODE 094
	char dbgmsg[128];		// allows printing feedback for waypointer who's testing his work

	edict_t *pEdict = pBot->pEdict;

	// forget on any waypoint that is unreachable for longer time															NEW CODE 094 (prev code)
	// don't apply this when waiting or doing any action
	//if ((pBot->f_waypoint_time + 5.0 < gpGlobals->time) &&
	//	(pBot->wpt_wait_time > 0.0) && (pBot->wpt_wait_time < gpGlobals->time) &&
	//	(pBot->wpt_action_time > 0.0) && (pBot->wpt_action_time < gpGlobals->time))
	
	// forget on any waypoint that is unreachable for longer time
	if (pBot->f_waypoint_time + 5.0f < gpGlobals->time)//																	NEW CODE 094
	{
#ifdef _DEBUG
		//@@@@@@@@@@@@@@@@
		if (botdebugger.IsDebugStuck())
		{
			//sprintf(dbgmsg, "(nav.cpp) f_wpt_time=%3.f < globTIME=%3.f for wpt #%d ---- !!GENERAL!! !!GENERAL!! !!GENERAL!!\n",
			//	pBot->f_waypoint_time, gpGlobals->time, pBot->curr_wpt_index+1);
			//HudNotify(dbgmsg);
		}
#endif

		//																													NEW CODE 094
		// if the bot was stuck in last 5 seconds AND
		// it's NOT soon after battle, because the bot can move far away from
		// his waypoint while in battle so we must allow him to reset navigation
		// if he isn't able to reach his waypoint anymore
		// for example if he fell from roof during battle
		if ((pBot->f_stuck_time + 5.0f >= gpGlobals->time) &&
			(pBot->f_bot_see_enemy_time + 8.0f < gpGlobals->time) &&
			(pBot->f_bot_wait_for_enemy_time + 8.0f < gpGlobals->time))
		{
			if (pBot->IsNeed(NEED_RESETNAVIG) == false)
			{
				// then update the waypoint time to compensate the time he lost trying to free self
				pBot->f_waypoint_time = gpGlobals->time;
				// and prepare the navigation reset
				pBot->SetNeed(NEED_RESETNAVIG);

				if (botdebugger.IsDebugPaths() || botdebugger.IsDebugStuck() || botdebugger.IsDebugWaypoints())
				{
					sprintf(dbgmsg, "<<WARNING>>%s was stuck while heading to waypoint #%d -> setting extra time to reach it!\n",
						pBot->name, pBot->curr_wpt_index + 1);
					HudNotify(dbgmsg, pBot);
				}
			}
			else
			{
				// the bot already had 10 seconds to reach current waypoint
				// but give him another 10 seconds...
				if (pBot->f_waypoint_time + 10.0f < gpGlobals->time)
				{
					// if the bot is still stuck while trying to reach this waypoint
					// then mark current waypoint as unreachable waypoint to allow resetting navigation
					unreachable_wpt_index = pBot->curr_wpt_index;

					if (botdebugger.IsDebugPaths() || botdebugger.IsDebugStuck() || botdebugger.IsDebugWaypoints())
					{
						sprintf(dbgmsg, "<<WARNING>>%s's extra stuck time is over & still can't reach the wpt -> going to RESET NAVIG!\n",
							pBot->name);
						HudNotify(dbgmsg, pBot);
					}
				}
			}
		}
		//	**************************************************************************************************************	NEW CODE 094 END

		// if bot is near door ie handling door or on ladder or in crouch or in prone
		// of moving at slow or even slower speeds (e.g. going down the hill or stairs)
		else if ((waypoints[pBot->curr_wpt_index].flags & (W_FL_DOOR | W_FL_DOORUSE)) ||
			(waypoints[pBot->prev_wpt_index.get()].flags & (W_FL_DOOR | W_FL_DOORUSE)) ||
			(pBot->pEdict->v.movetype == MOVETYPE_FLY) ||
			pBot->IsBehaviour(BOT_CROUCHED) || pBot->IsBehaviour(BOT_PRONED) ||//											NEW CODE 094
			(pBot->move_speed == SPEED_SLOW) || (pBot->move_speed == SPEED_SLOWEST))//										NEW CODE 094
		{

			//			vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

			// TO DO - Need to test somehow if the bot is moving forward
			//		(maybe something like moved_distance in bot.cpp)
			//		before doing any WPT & PATH resetting

			//			^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^



			// give the bot 15 seconds instead of default 5 seconds
			// to reach this waypoint
			if (pBot->f_waypoint_time + 15.0f < gpGlobals->time)
			{
				unreachable_wpt_index = pBot->curr_wpt_index;

				if (botdebugger.IsDebugPaths() || botdebugger.IsDebugStuck() || botdebugger.IsDebugWaypoints())
				{
					sprintf(dbgmsg, "<<WARNING>>%s's extra time to reach wpt #%d is over -> going to RESET NAVIG!\n",
						pBot->name, pBot->curr_wpt_index + 1);
					HudNotify(dbgmsg, pBot);
				}
			}
			else
			{
				;
#ifdef _DEBUG
				//@@@@@@@@@@@@@@@@
				//if (botdebugger.IsDebugStuck() || botdebugger.IsDebugPaths() || botdebugger.IsDebugWaypoints())
					//HudNotify("(nav.cpp) wpt_time(+5.0) < globTIME -> door OR ladder OR crouch OR prone OR slow/slower movement speed\n");
#endif
			}
		}
		// otherwise the bot must be stuck at this waypoint (unable to reach it)
		else
		{
			unreachable_wpt_index = pBot->curr_wpt_index;//																			NEW CODE 094

#ifdef _DEBUG
//@@@@@@@@@@@@@@@@
			//if (botdebugger.IsDebugStuck())
			//	HudNotify("(nav.cpp)DEFAULT wptTime(+5.0) is over - can't reach this wpt in TIME -> going to RESET NAVIG!\n");
#endif

		}

		// is this waypoint unreachable?
		if (unreachable_wpt_index != -1)//																							NEW CODE 094
		{
			if (botdebugger.IsDebugPaths() || botdebugger.IsDebugStuck() || botdebugger.IsDebugWaypoints())
			{				
				sprintf(dbgmsg, "<<POSSIBLE BUG IN WAYPOINTS>>Can't get to wpt #%d in time -> Navigation RESET (wpts may be too far from each other)\n",
					pBot->curr_wpt_index + 1);
				HudNotify(dbgmsg, pBot);
			}

			// backup current wpt before clearing its record																		NEW CODE 094 (prev code)
			//unreachable_wpt_index = pBot->curr_wpt_index;

			// bot can't reach this wpt so clear it
			pBot->curr_wpt_index = -1;

			// update path history
			pBot->prev_path_index = pBot->curr_path_index;
			pBot->curr_path_index = -1;
		}
	}

	// no waypoint so search for some
	if (pBot->curr_wpt_index == -1)
	{
		int found = WaypointFindFirst(pBot, MAX_WPT_DIST, unreachable_wpt_index);

		// didn't found any possible waypoint
		if (found == -1)
		{
			// clear all previously visited waypoints
			pBot->prev_wpt_index.clear();

			found = WaypointFindFirst(pBot, MAX_WPT_DIST, unreachable_wpt_index);

			// still no waypoint so clear current waypoint
			if (found == -1)
			{
				pBot->curr_wpt_index = -1;
				return FALSE;
			}
		}

		if (botdebugger.IsDebugWaypoints() || botdebugger.IsDebugStuck())
		{
			sprintf(dbgmsg, "***Have no waypoint! Found this one (index=%d) as a new waypoint\n", found + 1);
			HudNotify(dbgmsg, pBot);
		}

		pBot->curr_wpt_index = found;

		// generate new waypoint fake origin
		pBot->wpt_origin = GenerateOrigin(found);

		pBot->f_waypoint_time = gpGlobals->time;
	}

	// the bot already reached his current waypoint where he already finished given action (e.g. waiting)
	// so we must skip standard position checks in order to prevent doing unwanted turn backs in case
	// when he actually ran slightly past the waypoint (for example when he went prone, because going prone
	// isn't instant stop, but the body still has some forward motion, then it looks really weird if the bot
	// does 180 degree turn to crawl back to his waypoint followed by another 180 degree turn
	// to move forward to his next waypoint)
	if (pBot->IsNeed(NEED_NEXTWPT))
	{
		// we just tell him he is exactly in waypoint range
		in_wpt_range = TRUE;
		past_the_wpt = FALSE;

		// and that he is facing his current waypoint by resetting the time
		pBot->f_face_wpt = gpGlobals->time - 1.0f;
	}
	// otherwise the bot is still heading towards his waypoint so he must keep checking his current position and
	// compare it to waypoint position
	else
	{
		// 3D distance (space - ie we check also z-coord)
		wpt_distance = (pEdict->v.origin - pBot->wpt_origin).Length();

		// 2D distance (planar - ie we ignore z-coord)
		const float wpt_distance2D = (pEdict->v.origin - pBot->wpt_origin).Length2D();

		// get this waypoint range
		wpt_range = waypoints[pBot->curr_wpt_index].range;

		//																																	NEW CODE 094
		// if the waypoint range is really small then make it little bigger for the test
		// and use only planar distance to test whether the bot is in range or not
		// this trick handles the unwanted dancing around waypoints with range of 5 units
		if (wpt_range <= 10.0f)
		{
			wpt_range = 10.0f;
			use_only_planar = true;
		}
		else
			use_only_planar = false;

		// if the bot is jumping over the claymore mine then use only planar distance to waypoint
		// to prevent returning to the waypoints the bot may have passed while in jump
		if ((pBot->f_duckjump_time >= gpGlobals->time) && pBot->IsTask(TASK_CLAY_EVADE))
			use_only_planar = true;
		//	***************************************************************************************************************************		NEW CODE 094 END

		in_wpt_range = FALSE;
		past_the_wpt = FALSE;

		// are we close enough to a target waypoint (ie are we in current waypoint range)
		if (wpt_distance < wpt_range)//																										NEW CODE 094
		{
			in_wpt_range = TRUE;



#ifdef _DEBUG
			// this isn't meant to be public message
			if (botdebugger.IsDebugPaths() || botdebugger.IsDebugWaypoints() || botdebugger.IsDebugStuck())
			{
				sprintf(dbgmsg, "***SET bot IS IN RANGE for wpt #%d disttowpt=%.2f ?<=? (wptrange=%.2f)@@@ CORRECT RANGE @@@\n",
					pBot->curr_wpt_index + 1, wpt_distance, wpt_range);
				//HudNotify(dbgmsg);
			}
#endif


		}

		// did the bot run past the waypoint? (prevent the loop-the-loop problem)
		//if ((pBot->prev_wpt_distance > 1.0) && (wpt_distance > pBot->prev_wpt_distance))														NEW CODE 094 (prev code)
		else if ((pBot->prev_wpt_distance > 1.0f) && (wpt_distance > pBot->prev_wpt_distance)
			&& (pBot->move_speed == SPEED_MAX))//																								NEW CODE 094
		{
			in_wpt_range = TRUE;

			past_the_wpt = TRUE;	// bot just run past his current waypoint


#ifdef _DEBUG
			// this isn't meant to be public message
			if (botdebugger.IsDebugPaths() || botdebugger.IsDebugWaypoints() || botdebugger.IsDebugStuck())
			{
				sprintf(dbgmsg, "***SET bot run PAST THE WPT for wpt #%d disttowpt=%.2f ?>? (prevWptDist=%.2f)$$$$$$$$$$$$$$$$$$$$\n",
					pBot->curr_wpt_index + 1, wpt_distance, pBot->prev_wpt_distance);
				//HudNotify(dbgmsg);
			}
#endif



		}

		/*																																		NEW CODE 094 (prev code)
		// are we close enough to a target waypoint (ie are we in current waypoint range)
		else if (wpt_distance < wpt_range)
		{
			in_wpt_range = TRUE;



#ifdef _DEBUG
			// this isn't meant to be public message
			if (botdebugger.IsDebugPaths() || botdebugger.IsDebugWaypoints() || botdebugger.IsDebugStuck())
			{
				sprintf(dbgmsg, "***SET bot IS IN RANGE for wpt #%d disttowpt=%.2f ?<=? (wptrange=%.2f)@@@ CORRECT RANGE @@@\n",
					pBot->curr_wpt_index + 1, wpt_distance, wpt_range);
				HudNotify(dbgmsg);
			}
#endif


		}
		*/

		// we are NOT in 3D range, but only in 2D/planar range
		else if ((wpt_distance > wpt_range) && (wpt_distance2D < wpt_range))
		{
			// if it is a ladder waypoint we are NOT in range yet
			if (waypoints[pBot->curr_wpt_index].flags & W_FL_LADDER)
				in_wpt_range = FALSE;

			//NOTE: This code below may cause problems with ladders
			//		Because it probably tells the bot he reached the ladder already even if it didn't
			//		(our ladder range is 20 units the code below checks for 45 units)
			//		It needs to be tested and eventually put it in an else statement
			//		if ladder
			//		{}
			//		else
			//		{ this whole section from below }

			// check if waypoint z origin (up/down) is in some limits (45 is jump limit)

			// is waypoint higher than jump limit (bot is under waypoint and can't jump to it) OR
			// lower (bot is above the waypoint and must fall to it - a ditch for example)
			if (((waypoints[pBot->curr_wpt_index].origin.z > (pEdict->v.origin.z + 45.0)) ||
				(waypoints[pBot->curr_wpt_index].origin.z < (pEdict->v.origin.z - 45.0))) &&
				use_only_planar == false)//																									NEW CODE 094
			{
				in_wpt_range = FALSE;


#ifdef _DEBUG
				// this isn't meant to be public message
				if (botdebugger.IsDebugPaths() || botdebugger.IsDebugWaypoints() || botdebugger.IsDebugStuck())
				{
					sprintf(dbgmsg, "***IN PLANAR RANGE -> RESET IN RANGE for wpt #%d disttowpt=%.2f ?<=? (wptrange=%.2f)@@@@@@\n",
						pBot->curr_wpt_index + 1, wpt_distance, wpt_range);
					//HudNotify(dbgmsg);
				}
#endif



			}
			// otherwise bot already is inside those limits
			else
			{
				in_wpt_range = TRUE;



#ifdef _DEBUG
				// this isn't meant to be public message
				if (botdebugger.IsDebugPaths() || botdebugger.IsDebugWaypoints() || botdebugger.IsDebugStuck())
				{
					sprintf(dbgmsg, "***IN PLANAR RANGE -> SET IN RANGE for wpt #%d disttowpt=%.2f ?<=? (wptrange=%.2f)@@@ PLANAR RANGE @@@\n",
						pBot->curr_wpt_index + 1, wpt_distance, wpt_range);
					//HudNotify(dbgmsg);
				}
#endif


			}

		}

		// save current distance as previous unless it's a cross waypoint
		// for cross waypoint we must reset it, because that one has no true range
		if (waypoints[pBot->curr_wpt_index].flags & W_FL_CROSS)//															NEW CODE 094
			pBot->prev_wpt_distance = 0.0;
		else
			pBot->prev_wpt_distance = wpt_distance;

		// bot reached his wpt but still have time to turn
		if ((pBot->f_face_wpt > gpGlobals->time) && (wpt_distance2D <= wpt_range))
		{
			// "reset" it
			pBot->f_face_wpt = gpGlobals->time - 1.0f;

#ifdef _DEBUG
			// this isn't meant to be public message
			if (botdebugger.IsDebugPaths() || botdebugger.IsDebugWaypoints() || botdebugger.IsDebugStuck())
			{
				sprintf(dbgmsg, "***Reached the wpt & Turn time cleared on waypoint %d\n", pBot->curr_wpt_index + 1);
				HudNotify(dbgmsg, pBot);
			}
#endif

		}
	}

	if (in_wpt_range && (pBot->f_face_wpt < gpGlobals->time))
	{
		bool waypoint_found = FALSE;

		pBot->prev_wpt_distance = 0.0;

		if (past_the_wpt)
		{
			//if ((pEdict->v.origin - pBot->wpt_origin).Length() > waypoints[pBot->curr_wpt_index].range)																NEW CODE 094 (prev code)
			
			// if this waypoint range is quite small and the bot was moving too fast and
			// ran through the range and is past it now then give him next game frame so he can try it again
			// sadly this will increase the number of cases when the bot is "dancing" around the waypoint
			// but small range has a purpose so we must ensure the bot is careful there
			if ((wpt_distance > wpt_range) && (wpt_range < WPT_RANGE))//																								NEW CODE 094
			{
				/*
				if (pBot->f_waypoint_time + 5.0 < gpGlobals->time)
				{
					//																													NEW CODE 094
					if ((pBot->f_stuck_time + 5.0f >= gpGlobals->time) &&
						(pBot->f_bot_see_enemy_time + 8.0f < gpGlobals->time) &&
						(pBot->f_bot_wait_for_enemy_time + 8.0f < gpGlobals->time))
					{
						pBot->f_waypoint_time = gpGlobals->time;

						if (botdebugger.IsDebugPaths() || botdebugger.IsDebugStuck() || botdebugger.IsDebugWaypoints())
						{
							sprintf(dbgmsg, "<<WARNING>>%s was stuck while heading to waypoint #%d -> setting new time to reach it!\n",
								pBot->name, pBot->curr_wpt_index + 1);
							HudNotify(dbgmsg);
						}
					}
					else
					{
						//																												NEW CODE 094 END
						// this must go first before we reset the waypoint index
						if (botdebugger.IsDebugPaths() || botdebugger.IsDebugStuck() || botdebugger.IsDebugWaypoints())
						{
							sprintf(dbgmsg, "<<WARNING>>Navigation reset for %s cause is stuck on waypoint #%d -> can't reach its range!\n",
								pBot->name, pBot->curr_wpt_index + 1);
							HudNotify(dbgmsg);
						}

						pBot->curr_wpt_index = -1;

						// update path history
						pBot->prev_path_index = pBot->curr_path_index;
						pBot->curr_path_index = -1;
					}
				}
				
				*/
				// TRUE, because bot has waypoint, but he isn't in wpt range yet
				return TRUE;
			}
		}

// ###################### WAYPOINT BEHAVIOR #####################################################

		// does the bot reached an ammobox waypoint
		if (waypoints[pBot->curr_wpt_index].flags & W_FL_AMMOBOX)
		{
			// first time the bot got to this waypoint?
			if (!pBot->IsTask(TASK_WPTACTION) && (pBot->wpt_action_time < gpGlobals->time))
			{
				// check if we need any additional ammo/mags
				UTIL_CheckAmmoReserves(pBot, "HeadTowardWpt() -> current wpt is AMMOBOX");

				// estimated delay to take both mags (main+backup)
				// is multiplied by the amount of needed mags
				//total_time = 1.7 * (pBot->take_main_mags + pBot->take_backup_mags);
				float total_time = (float)pBot->take_main_mags + (float)pBot->take_backup_mags;
				total_time *= 1.7;

				// if total_time is 0 then the bot is carrying all mags he can
				if (total_time == 0.0)
				{
					pBot->wpt_action_time = 0.0;	// prevent opening the box
				}
				// otherwise take needed mags, also set some time to do so
				else
				{
					pBot->SetTask(TASK_WPTACTION);
					pBot->wpt_action_time = gpGlobals->time + total_time;

					// here we won't set the need for next waypoint, because if we did, the bot won't be able to
					// use the ammobox again after he reloaded weapon or merged magazines while standing
					// next to it (wptaction task gets reset pretty fast ... while doing either of those actions) so
					// we just prevent finding new waypoint in this game frame
					waypoint_found = TRUE;
				}
			}
		}

		// check if the bot is at parachute waypoint
		if (waypoints[pBot->curr_wpt_index].flags & W_FL_CHUTE)
		{
			if (pBot->IsTask(TASK_PARACHUTE))
			{
				// prevent running wpt search code for next few seconds
				pBot->f_dont_look_for_waypoint_time = gpGlobals->time + 2.5f;

				if (pBot->chute_action_time < gpGlobals->time)
					pBot->chute_action_time = gpGlobals->time + 0.7f;	// some time to jump out
			}
			else if (!pBot->IsTask(TASK_PARACHUTE) && (pBot->chute_action_time < gpGlobals->time))
			{
				// if the bot is on two-way path turnback
				if ((pBot->curr_path_index != -1) && (w_paths[pBot->curr_path_index]->flags & P_FL_WAY_TWO))
				{
					// change the direction the bot follows this path
					if (pBot->opposite_path_dir)
						pBot->opposite_path_dir = FALSE;
					else
						pBot->opposite_path_dir = TRUE;
				}
				// otherwise wait untill the parachute spawns again
				else
				{
					// the bot can be stuck so try to get the path from current waypoint
					if ((pBot->curr_path_index == -1) && (num_w_paths > 0))
						CheckPossiblePath(pBot, pBot->curr_wpt_index);
					// there are no paths or current path isn't two-way path (or doesn't match)
					// so wait at this waypoint
					else
						pBot->wpt_wait_time = gpGlobals->time + RANDOM_FLOAT(2.0, 5.0);
					
					// don't look for next waypoint
					waypoint_found = TRUE;
						
					pBot->f_waypoint_time = gpGlobals->time;
				}
			}
#ifdef _DEBUG
			else
			{
				//@@@@@@@@@@@
				HudNotify("HEY >>>>>>>>>>>>>>>>>>> PARACHUTE flag Else\n");
			}
#endif
		}

		// is bot at door/dooruse waypoint
		if ((pBot->IsNeed(NEED_NEXTWPT) == FALSE) && (waypoints[pBot->curr_wpt_index].flags & (W_FL_DOOR | W_FL_DOORUSE)))
		{
			// get the end vector to allow bot facing correct direction
			//if (pBot->v_door == Vector(0, 0, 0))
			if (pBot->v_door == g_vecZero)
			{
				edict_t *pent = nullptr;
				char door_types[64];
				
				//@@@@@@@@@@@@@@@
#ifdef _DEBUG
				char dm[256];
				sprintf(dm, "HeadTowardWpt() -> DOOR wpt #%d called | NextWpt is #%d\n",pBot->curr_wpt_index + 1,
					pBot->GetNextWaypoint() + 1);
				HudNotify(dm);
#endif
				
				pBot->f_dont_avoid_wall_time = gpGlobals->time + 2.0f;// 5.0; <- prev code
				pBot->RemoveSubTask(ST_DOOR_OPEN);
				
				// is the bot at first door wpt (i.e. not past door yet)?
				// it's done this way because of the possibility of a sequence of two 'door' waypoints
				// (way_two + team_no paths a.k.a. default paths will probably have 'door' wpts on both sides of the doors)
				// and we don't want to do anything while the bot is passing the second 'door' wpt
				// (i.e. when he's already behind the doors)
				// therefore we are checking if previous wpt was NOT a 'door' wpt
				if ((waypoints[pBot->prev_wpt_index.get()].flags & (W_FL_DOOR | W_FL_DOORUSE)) == FALSE)
				{
					while ((pent = UTIL_FindEntityInSphere(pent, pEdict->v.origin, PLAYER_SEARCH_RADIUS)) != nullptr)
					{
						strcpy(door_types, STRING(pent->v.classname));
						
						if ((strcmp("func_door", door_types) == 0) ||
							(strcmp("func_door_rotating", door_types) == 0) ||
							(strcmp("momentary_door", door_types) == 0))
						{
							Vector door_entity;

							// if the bot is able to get his next waypoint then
							// use the next waypoint as the door origin otherwise use the real door origin
							// this system allows the bot to successfully pass even doors
							// that are partially open or are closing at the moment
							// (using the real door origin in this case would cause that
							// the bot will try facing the doors and will get stuck there)
							const int next_waypoint = pBot->GetNextWaypoint();
							
							if (next_waypoint != -1)
								door_entity = waypoints[next_waypoint].origin;
							else
								// BModels have 0,0,0 for origin so must use VecBModelOrigin
								door_entity = VecBModelOrigin(pent);
							
							// save door coords
							pBot->v_door = door_entity;

							const Vector bot_angles = UTIL_VecToAngles(door_entity);
							pEdict->v.ideal_yaw = bot_angles.y;
							BotFixIdealYaw(pEdict);

							// don't look for next waypoint now
							waypoint_found = TRUE;
							pBot->SetNeed(NEED_NEXTWPT);

							pBot->wpt_wait_time = gpGlobals->time + 1.0f;
							pBot->SetSubTask(ST_DOOR_OPEN);
							
							break;
						}
					}
				}
#ifdef DEBUG
				else
				{
					char dm[256];
					sprintf(dm, "HeadTowardWpt() -> DOOR wpt #%d called, BUT PREV wpt #%d was also door wpt so IGNORING (because already past the door)\n",
						pBot->curr_wpt_index + 1, pBot->prev_wpt_index.get() + 1);
					HudNotify(dm);
				}
#endif // DEBUG
			}
		}

		// is current waypoint a shoot waypoint AND hasn't been "used" yet?
		else if ((pBot->IsNeed(NEED_NEXTWPT) == FALSE) && (waypoints[pBot->curr_wpt_index].flags & W_FL_FIRE))
		{
			UTIL_CheckAmmoReserves(pBot, "HeadTowardWpt() -> current wpt is SHOOT");
			
			pBot->ResetAims("HeadTowardWpt() -> SHOOT wpt");

			// if this waypoint has some priority then "use" it
			if (WaypointGetPriority(pBot->curr_wpt_index, pBot->bot_team) != 0)
			{
				// force the bot get the aim waypoint "exactly" in the middle of FOV
				// (i.e. in very tight view cone)
				pBot->SetTask(TASK_PRECISEAIM);

				// run the fire task only if there's no wait time on this waypoint
				// otherwise we have different code for that case ... the wait time code will catch it
				if (WaypointGetWaitTime(pBot->curr_wpt_index, pBot->bot_team) == 0.0)
				{
					pBot->SetTask(TASK_FIRE);
					pBot->SetNeed(NEED_NEXTWPT);


					//@@@@@@@@@@@@@@@@@@@@@@@												NEW CODE 094 Test
#ifdef _DEBUG
					//HudNotify("navigate.cpp -> SHOOT wpt -> Setting TASK FIRE!\n");
#endif



				}

				waypoint_found = TRUE;		// prevent finding new waypoint
			}


			//@@@@@@@@@@@@@@@@@@@@@@@
#ifdef _DEBUG
			if (botdebugger.IsDebugActions() || botdebugger.IsDebugWaypoints())
				HudNotify("navigate.cpp -> SHOOT wpt called\n");
#endif



		}

		// check if bot is at jump waypoint
		if (waypoints[pBot->curr_wpt_index].flags & (W_FL_JUMP | W_FL_DUCKJUMP))
		{
			// first we must check for prone because sending IN_JUMP would break it
			if ((pBot->IsBehaviour(BOT_PRONED) == FALSE) && (pBot->IsGoingProne() == FALSE))	// NEW CODE 094
			{
				pEdict->v.button |= IN_JUMP;  // jump here
				
				if (waypoints[pBot->curr_wpt_index].flags & W_FL_DUCKJUMP)
				{
					// also press crouch
					//pEdict->v.button |= IN_DUCK;												NEW CODE 094 (prev code)
					SetStance(pBot, GOTO_CROUCH, "bot_navig.cpp|HeadToWpt() -> GOTO crouch @ DUCKJUMP WPT");//	NEW CODE 094

					pBot->f_duckjump_time = gpGlobals->time + 1.0f;
				}
			}
		}

		// did the bot reach ladder waypoint
		if (waypoints[pBot->curr_wpt_index].flags & W_FL_LADDER)
		{
			// prevent unwanted actions (like turn away from wall etc.)
			pBot->f_dont_avoid_wall_time = gpGlobals->time + 2.0f;
			pBot->SetDontCheckStuck("Bot HeadTowardsWpt() -> LADDER waypoint", 0.5f);
		}

		// is this wpt a claymore waypoint AND have NOT been "used" yet AND
		// has the bot a claymore mine AND NOT no priority for the team bot is in
		else if ((pBot->IsNeed(NEED_NEXTWPT) == FALSE) && (waypoints[pBot->curr_wpt_index].flags & W_FL_MINE) &&
			(pBot->bot_weapons & (1<<fa_weapon_claymore)) && (pBot->clay_action == ALTW_NOTUSED) &&
			// (pBot->wpt_wait_time + 2.5 < gpGlobals->time) &&							NEW CODE 094 (prev code)
			(WaypointGetPriority(pBot->curr_wpt_index, pBot->bot_team) != 0))
		{
			// if the bot is trying to evade a claymoree then we shouldn't force him to
			// set his own claymore (i.e. we'll do nothing here -> bot will ignore this waypoint)
			if (pBot->IsTask(TASK_CLAY_EVADE))
			{
				//@@@@@@@@@@@@@22
				#ifdef _DEBUG
				if (botdebugger.IsDebugWaypoints())
					HudNotify("CLAYMORE WPT called -> there's already a mine -> evading -> not placing my clay\n", pBot);
				#endif
			}
			// if the bot is crouched (or doing duck jump) then ignore this waypoint
			// because FA doesn't allow placing claymore in such case
			// duckjump doesn't set crouch behaviour (it's a temporary action)
			// so we must check even for the ducking flag here
			else if (pBot->IsBehaviour(BOT_CROUCHED) || (pEdict->v.flags & FL_DUCKING))
			{
				if (botdebugger.IsDebugWaypoints() || botdebugger.IsDebugActions())
					HudNotify("***Can't place my claymore mine due to being in crouch stance -> ignoring this waypoint action!\n", pBot);
			}
			else
			{
				// set things only if the bot is allowed to use the claymore
				// (the waypoint is a goal claymore on sd_ maps or the bot decided to place the mine here)
				if ((WaypointGetPriority(pBot->curr_wpt_index, pBot->bot_team) == 1) || (RANDOM_LONG(1, 100) > 50))
				{
					// set some wait time
					pBot->wpt_wait_time = gpGlobals->time +	5.0f;

					// make the bot place the mine in correct direction 'Front Toward Enemy' :)
					pBot->ResetAims("HeadTowardWpt() -> CLAYMORE wpt");
					pBot->SetTask(TASK_PRECISEAIM);

					// prevent finding new waypoint
					pBot->SetNeed(NEED_NEXTWPT);
					waypoint_found = TRUE;
				}
			}
		}

		// is bot at sprint waypoint AND can he use it
		else if ((waypoints[pBot->curr_wpt_index].flags & W_FL_SPRINT) &&
			(WaypointGetPriority(pBot->curr_wpt_index, pBot->bot_team) != 0))
		{
			// the bot is already sprinting so stop it (ie. he got to 2nd sprint waypoint)
			if (pBot->IsTask(TASK_SPRINT))
				pBot->RemoveTask(TASK_SPRINT);
			// not sprinting yet so start it (ie. he got to 1st/the only sprint waypoint)
			else
			{
				pBot->SetTask(TASK_SPRINT);
				// so the bot won't stop and shoot while we need sprint from him
				// the bot will attack only close enemy in this case
				pBot->SetTask(TASK_IGNORE_ENEMY);
			}
		}

		// check if bot reached sniper waypoint AND don't move flag is NOT set yet
		if ((waypoints[pBot->curr_wpt_index].flags & W_FL_SNIPER) &&
			!pBot->IsTask(TASK_DONTMOVEINCOMBAT))
		{
			pBot->SetTask(TASK_DONTMOVEINCOMBAT);		// don't move in combat
		}

		// is the bot at use waypoint?
		if (waypoints[pBot->curr_wpt_index].flags & W_FL_USE)
		{
			// first time the bot got to this waypoint?
			if (!pBot->IsTask(TASK_WPTACTION) && (pBot->IsNeed(NEED_NEXTWPT) == FALSE))
				//(pBot->wpt_action_time < gpGlobals->time) &&								NEW CODE 094 (prev code)
				//(gpGlobals->time - 0.5 > pBot->wpt_action_time))
			{
				// use wait time from this waypoint as the action time
				const float total_time = WaypointGetWaitTime(pBot->curr_wpt_index, pBot->bot_team);

				// if total_time is 0 then generate some time randomly
				if (total_time == 0.0)
					pBot->wpt_action_time = gpGlobals->time + RANDOM_FLOAT(1.0, 2.2);
				// otherwise use it
				else
				{
					pBot->wpt_action_time = gpGlobals->time + total_time;
					pBot->SetSubTask(ST_TANK_SHORT);
				}

				pBot->SetTask(TASK_WPTACTION);
				waypoint_found = TRUE;		// prevent finding new waypoint
				pBot->SetNeed(NEED_NEXTWPT);



				//@@@@@@@@@@@@@@@
#ifdef _DEBUG
				char dm[256];
				sprintf(dm, "HeadTowardWpt() -> USE wpt #%d called\n", pBot->curr_wpt_index + 1);
				HudNotify(dm);
#endif


			}
		}

		// check if this waypoint is a prone waypoint AND bot is NOT in prone so go prone
		if ((waypoints[pBot->curr_wpt_index].flags & W_FL_PRONE) && !pBot->IsBehaviour(BOT_PRONED))
		{
			UTIL_GoProne(pBot, "bot_navigate.cpp|HeadTowardWpt() -> At PRONE WPT but not in prone");
		}

		// is there some wait_time on this waypoint AND safety statement (we won't keep waiting)
		if ((WaypointGetWaitTime(pBot->curr_wpt_index, pBot->bot_team, "waitT check") > 0.0) &&
			(pBot->IsNeed(NEED_NEXTWPT) == FALSE))
		{
			// handles the cases when the 'wait-timed' waypoint is the first waypoint in a path
			// (eg. there is a cross waypoint and the next waypoint is this current waypoint)
			// we do not have a path yet because the cross waypoint kills it but we already need the info from the path here
			// so we call the 'give me my path' right here
			// (standard path navigation routine would have done it anyways ... just too late for this particular case)
			if (pBot->curr_path_index == -1)
			{
				if (CheckPossiblePath(pBot, pBot->curr_wpt_index))
					AssignPathBasedBehaviour(pBot, pBot->curr_wpt_index);
			}

			// set wait time only if path class restriction and bot "class" do match
			// otherwise even the sniper/mgunner chasing bots would camp there, but they should just
			// scout the paths and kill the sniper/mgunner
			if ((pBot->curr_path_index != -1) &&
				(w_paths[pBot->curr_path_index]->flags & (P_FL_CLASS_SNIPER | P_FL_CLASS_MGUNNER)))
			{
				if ((w_paths[pBot->curr_path_index]->flags & P_FL_CLASS_SNIPER) && (pBot->bot_behaviour & SNIPER))
				{
					pBot->wpt_wait_time = gpGlobals->time + WaypointGetWaitTime(pBot->curr_wpt_index, pBot->bot_team);
					waypoint_found = TRUE;

					if (botdebugger.IsDebugWaypoints())
						ALERT(at_console, "Bot SNIPER - on sniper path -> going to camp here\n");
				}

				if ((w_paths[pBot->curr_path_index]->flags & P_FL_CLASS_MGUNNER) &&	(pBot->bot_behaviour & MGUNNER))
				{
					pBot->wpt_wait_time = gpGlobals->time + WaypointGetWaitTime(pBot->curr_wpt_index, pBot->bot_team);
					waypoint_found = TRUE;

					if (botdebugger.IsDebugWaypoints())
						ALERT(at_console, "Bot MGUNNER - on mgunner path -> going to camp here\n");
				}
			}
			// if no path class restriction or no path at all set wait time
			else
			{
				pBot->wpt_wait_time = gpGlobals->time + WaypointGetWaitTime(pBot->curr_wpt_index, pBot->bot_team);
				waypoint_found = TRUE;

				if (botdebugger.IsDebugWaypoints())
					ALERT(at_console, "Reached 'wait' time wpt -> going to camp here\n");
			}

			// this waypoint is usable for this bot so ...
			if (waypoint_found)
			{
				// clear the aim waypoint array and stuff
				pBot->ResetAims("HeadTowardWpt() -> Reached wpt with WAIT TIME");

				// tweak the time based on bot behaviour
				// unless it's a priority == 1 waypoint
				// (i.e. a goal waypoint where we must use the exact time that is set on the waypoint,
				// otherwise things may not work correctly - for example the explosives on obj_bocage
				// that need specific time value to appear and become active)
				if (WaypointGetPriority(pBot->curr_wpt_index, pBot->bot_team) != 1)//														NEW CODE 094
				{
					if (pBot->bot_behaviour & ATTACKER)
					{
						pBot->wpt_wait_time -= gpGlobals->time;
						pBot->wpt_wait_time *= (float)0.65;
						pBot->wpt_wait_time += gpGlobals->time;
					}
					else if (pBot->bot_behaviour & DEFENDER)
					{
						pBot->wpt_wait_time -= gpGlobals->time;
						pBot->wpt_wait_time *= (float)1.35;
						pBot->wpt_wait_time += gpGlobals->time;
					}
				}

				// and set the need for next waypoint to prevent repeating this action again
				pBot->SetNeed(NEED_NEXTWPT);
			}
		}

		// is time to look for a new waypoint
		if (waypoint_found == FALSE)
		{
			// we are going to call for next waypoint so we must reset these needs
			pBot->RemoveNeed(NEED_NEXTWPT);
			pBot->RemoveNeed(NEED_RESETNAVIG);


#ifdef DEBUG
			//HudNotify("Need for NEXTWPT & RESETNAVIG have been removed -> going to find next waypoint NOW!!!!!!!\n");
#endif // DEBUG




			//kota@ the bot reaches the next wpt
            //It is the best place update wpt history
			UpdateWptHistory(pBot);

			// init it first
			is_next_wpt = FALSE;
		
			// are there any paths for this map
			if (num_w_paths > 0)
			{
				const int result = BotOnPath(pBot);

				if (result != -1)
				{
					is_next_wpt = TRUE;
				}
				else
					is_next_wpt = FALSE;
			}


			// if the bot isn't on any path or is at the end of his current path
			// (is heading towards to cross waypoint)
			// then find a new waypoint
			if (is_next_wpt == FALSE)
			{
				// clear current path index, because the bot isn't on any path now
				if (pBot->curr_path_index != -1)
					pBot->curr_path_index = -1;

				is_next_wpt = BotFindWaypoint(pBot, FALSE);
			}

			// is the next waypoint a goback waypoint?
			if (waypoints[pBot->prev_wpt_index.get()].flags & W_FL_GOBACK)
			{
				// to allow 180 degree turning
				pBot->f_face_wpt = gpGlobals->time + 0.6f;
			}

			// NOTE: DON'T DELETE THIS
			// check wpt time and if there is some set it
			//if (waypoints[pBot->prev_wpt_index[0]].flags & W_FL_USE)
			//{
				/*create global array for use wpts and check last time the use wpt was used,
				special slot in wpt struct could hold this use time*/

			//	;	// don't set wpt_wait_time
			//}

			// no waypoint to continue to
			if (is_next_wpt == FALSE)
			{
				pBot->curr_wpt_index = -1;  // indicate no waypoint found

				// clear all previous waypoints
				pBot->prev_wpt_index.clear();

				// clear also previous waypoint origin
				pBot->prev_wpt_origin = g_vecZero;//Vector(0, 0, 0);

				if (botdebugger.IsDebugWaypoints() || botdebugger.IsDebugStuck())
					HudNotify("<<BUG IN WAYPOINTS>>***No waypoint to head to - clearing previous waypoints history!\n", pBot);

				return FALSE;
			}
		}
	}

	// is cross wpt near wpt/wpts with very small range so slow down too
	if ((pBot->crowded_wpt_index != -1) && (waypoints[pBot->curr_wpt_index].flags & (W_FL_CROSS | W_FL_GOBACK)))
		pBot->crowded_wpt_index = pBot->curr_wpt_index;

	// has current waypoint very small range remember it (to slow down when close to it)
	if ((waypoints[pBot->curr_wpt_index].range < 20) && (pBot->crowded_wpt_index == -1))
		pBot->crowded_wpt_index = pBot->curr_wpt_index;

	// check if the bot is proned AND current waypoint is NOT prone waypoint
	// prevent false prone (the only exception to this is claymore evasion)
	if (pBot->IsBehaviour(BOT_PRONED) && !pBot->IsTask(TASK_CLAY_EVADE) &&
		!(waypoints[pBot->curr_wpt_index].flags & (W_FL_PRONE | W_FL_CROSS)))
	{
		UTIL_GoProne(pBot, "bot_navigate.cpp|HeadTowardWpt() -> Bot in prone but NOT at PRONE WPT");
	}
	// check if the bot is crouched AND current waypoint is NOT crouch waypoint
	if (pBot->IsBehaviour(BOT_CROUCHED) && !(waypoints[pBot->curr_wpt_index].flags & (W_FL_CROUCH | W_FL_CROSS)))
	{
		SetStance(pBot, GOTO_STANDING, "bot_navigate.cpp|HeadTowardWpt() -> GOTO standing because NOT at crouch wpt");
	}
	// is bot heading towards the ammobox and have NOT all mags
	if ((waypoints[pBot->curr_wpt_index].flags & W_FL_AMMOBOX) &&
		((pBot->take_main_mags > 0) || (pBot->take_backup_mags > 0)))
	{
		// is the bot quite close so slow down
		if ((pBot->move_speed > SPEED_SLOW) &&
			((pEdict->v.origin - waypoints[pBot->curr_wpt_index].origin).Length() < (WPT_RANGE * (float) 2)))
		   pBot->move_speed = SPEED_SLOW;
	}

	// is bot heading towards one of these AND is quite close so slow down
	if (((waypoints[pBot->curr_wpt_index].flags & (W_FL_USE | W_FL_DOOR | W_FL_DOORUSE | W_FL_FIRE)) ||
		((waypoints[pBot->curr_wpt_index].flags & W_FL_LADDER) && (pEdict->v.movetype != MOVETYPE_FLY))) &&
		(pBot->move_speed > SPEED_SLOW) &&
		((pEdict->v.origin - waypoints[pBot->curr_wpt_index].origin).Length() < (WPT_RANGE * (float) 2)))
	{
		pBot->move_speed = SPEED_SLOW;
	}

	// is bot heading to mine wpt AND did this bot spawned with any claymore AND
	// didn't use it yet AND isn't evading another claymore AND is quite close to the wpt so slow down a bit
	if ((waypoints[pBot->curr_wpt_index].flags & W_FL_MINE) && (pBot->claymore_slot != NO_VAL) &&
		(pBot->clay_action == ALTW_NOTUSED) && !pBot->IsTask(TASK_CLAY_EVADE) && (pBot->move_speed > SPEED_SLOW) &&
		((pEdict->v.origin - waypoints[pBot->curr_wpt_index].origin).Length() < (WPT_RANGE * (float) 2)))
		pBot->move_speed = SPEED_SLOW;

	// if the bot is NOT on a ladder, change the yaw...
	if (pEdict->v.movetype != MOVETYPE_FLY)
	{
		// keep turning towards the waypoint
		const Vector v_direction = pBot->wpt_origin - pEdict->v.origin;

		const Vector v_angles = UTIL_VecToAngles(v_direction);
			
		pEdict->v.idealpitch = -v_angles.x;
		BotFixIdealPitch(pEdict);

		pEdict->v.ideal_yaw = v_angles.y;
		BotFixIdealYaw(pEdict);
	}
	// the bot is still on the ladder but already reached its end
	// so allow him to face next waypoint
	else if ((pEdict->v.movetype == MOVETYPE_FLY) && pBot->waypoint_top_of_ladder)
	{
		const Vector v_direction = pBot->wpt_origin - pEdict->v.origin;
		const Vector v_angles = UTIL_VecToAngles(v_direction);

		pEdict->v.ideal_yaw = v_angles.y;
		BotFixIdealYaw(pEdict);
	}

	return TRUE;
}


/*
* bots ladder behaviour, all from facing it to leaving it on the other end
*/
bool BotHandleLadder(bot_t *pBot, float moved_distance)
{
	edict_t *pent = nullptr;
	edict_t *pEdict = pBot->pEdict;
	float end_wpt_distance;
	bool bot_stuck_on_ladder;

	// did the bot just touch the ladder
	if ((pBot->ladder_dir == LADDER_UNKNOWN) && (pEdict->v.movetype == MOVETYPE_FLY))
		pBot->f_start_ladder_time = gpGlobals->time;

	// has the bot no ladder end yet?
	if (pBot->end_wpt_index == -1)
		pBot->end_wpt_index = pBot->curr_wpt_index;

	// the bot doesn't know which way to go yet
	if (pBot->ladder_dir == LADDER_UNKNOWN)
	{
		Vector v_src, v_dest, view_angles;
		TraceResult tr;
		float angle = 0.0;
		bool done = FALSE;

		// do we have an end point of this ladder?
		if (pBot->end_wpt_index != -1)
		{
			if (waypoints[pBot->end_wpt_index].origin.z < pEdict->v.origin.z)
				pBot->ladder_dir = LADDER_DOWN;		// the bot will climb down
			else if (waypoints[pBot->end_wpt_index].origin.z > pEdict->v.origin.z)
				pBot->ladder_dir = LADDER_UP;		// the bot will climb up
			else
				pBot->ladder_dir = LADDER_UNKNOWN;
		}
		// otherwise use the ladder origin to tell the bot the right direction
		else
		{
			while ((pent = UTIL_FindEntityInSphere(pent, pEdict->v.origin, PLAYER_SEARCH_RADIUS)) != nullptr)
			{
				if (strcmp("func_ladder", STRING(pent->v.classname)) == 0)
				{
					Vector ladder_origin = g_vecZero;//Vector(0, 0, 0);
					ladder_origin = VecBModelOrigin(pent);
					
					if (ladder_origin.z < pEdict->v.origin.z)
					{
						pBot->ladder_dir = LADDER_DOWN;
					}
					else if (ladder_origin.z > pEdict->v.origin.z)
					{
						pBot->ladder_dir = LADDER_UP;
					}
					else
					{
						pBot->ladder_dir = LADDER_UNKNOWN;
					}
					
					break;
				}
			}
		}

/*/
#ifdef _DEBUG
		if (pBot->ladder_dir == LADDER_UNKNOWN)
			ALERT(at_console, "***unknown ladder direction\n");
		else if (pBot->ladder_dir == LADDER_DOWN)
			ALERT(at_console, "***climb it down\n");
		else if (pBot->ladder_dir == LADDER_UP)
			ALERT(at_console, "***climb it up\n");
		#endif
/**/
		
		// also try to square up the bot on the ladder
		while ((!done) && (angle < 180.0))
		{
			// try looking in one direction (forward + angle)
			view_angles = pEdict->v.v_angle;
			view_angles.y = pEdict->v.v_angle.y + angle;

			if (view_angles.y < 0.0)
				view_angles.y += 360.0;
			if (view_angles.y > 360.0)
				view_angles.y -= 360.0;

			UTIL_MakeVectors(view_angles);

			v_src = pEdict->v.origin + pEdict->v.view_ofs;
			v_dest = v_src + gpGlobals->v_forward * 30;

			UTIL_TraceLine( v_src, v_dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);

			if (tr.flFraction < 1.0)  // hit something?
			{
				if (strcmp("func_ladder", STRING(tr.pHit->v.classname)) == 0)
				{
					done = TRUE;

					//@@@@@@@
					//ALERT(at_console, "FACE THE LADDER\n");
				}
				//else if (strcmp("func_wall", STRING(tr.pHit->v.classname)) == 0)
				if (strcmp("func_wall", STRING(tr.pHit->v.classname)) == 0)
				{
					// square up to the wall...
					view_angles = UTIL_VecToAngles(tr.vecPlaneNormal);

					// Normal comes OUT from wall, so flip it around...
					view_angles.y += 180;

					if (view_angles.y > 180)
						view_angles.y -= 360;

					pEdict->v.ideal_yaw = view_angles.y;
					//BotFixIdealYaw(pEdict);
					
					done = TRUE;

					//@@@@@@@
					//ALERT(at_console, "HIT SOMETHING - Flip it around\n");
				}
			}
			else
			{
				// try looking in the other direction (forward - angle)
				view_angles = pEdict->v.v_angle;
				view_angles.y = pEdict->v.v_angle.y - angle;

				if (view_angles.y < 0.0)
					view_angles.y += 360.0;
				if (view_angles.y > 360.0)
					view_angles.y -= 360.0;

				UTIL_MakeVectors( view_angles );

				v_src = pEdict->v.origin + pEdict->v.view_ofs;
				v_dest = v_src + gpGlobals->v_forward * 30;
				
				UTIL_TraceLine( v_src, v_dest, dont_ignore_monsters,
					pEdict->v.pContainingEntity, &tr);

				if (tr.flFraction < 1.0)  // hit something?
				{
					if (strcmp("func_wall", STRING(tr.pHit->v.classname)) == 0)
					{
						// square up to the wall...
						view_angles = UTIL_VecToAngles(tr.vecPlaneNormal);

						// Normal comes OUT from wall, so flip it around...
						view_angles.y += 180;

						if (view_angles.y > 180)
							view_angles.y -= 360;

						pEdict->v.ideal_yaw = view_angles.y;
						BotFixIdealYaw(pEdict);

						//@@@@@@@
						//ALERT(at_console, "DIDN'T HIT - Flip it around\n");

						done = TRUE;
					}
				}
			}

			angle += 10;
		}

		if (!done)  // if didn't find a wall, just reset ideal_yaw
		{
			// set ideal_yaw to current yaw (so bot won't keep turning)
			pEdict->v.ideal_yaw = pEdict->v.v_angle.y;
			BotFixIdealYaw(pEdict);
		}
	}

	end_wpt_distance = 0.0;
	bot_stuck_on_ladder = FALSE;

	// the bot must be stuck on ladder
	if ((pBot->f_start_ladder_time + 2.0 < gpGlobals->time) &&
		(moved_distance <= 1.0) && (pBot->prev_speed != SPEED_NO) &&
		((pBot->ladder_dir == LADDER_UP) || (pBot->ladder_dir == LADDER_DOWN)))
	{
		// is the bot stuck on this ladder for quite some time?
		// then get of it no matter if that would mean breaking a leg or even death
		// we basically need to free this ladder
		if (pBot->f_start_ladder_time + 4.0 < gpGlobals->time)		// was 10.0
		{
			pEdict->v.button |= IN_JUMP;


			//@@@@@@@@@
			#ifdef _DEBUG
			ALERT(at_console, "***was stuck on ladder --> jumping off it!\n");
			#endif
		}

		Vector vecStart, vecEnd;
		TraceResult tr;

		int dir = pBot->ladder_dir;
		
		UTIL_MakeVectors(pEdict->v.v_angle);
		vecStart = pEdict->v.origin + pEdict->v.view_ofs;

		// centre and up (ie. above the bot's head)
		if (dir == LADDER_UP)
			vecEnd = vecStart + gpGlobals->v_up * 50;
		// centre and down (ie. below bot's feet)
		else
			vecEnd = pEdict->v.origin + gpGlobals->v_up * -70;
		
		UTIL_TraceLine(vecStart, vecEnd, dont_ignore_monsters, pEdict, &tr);



		//@@@@@@@@@!!!!!!!!!@@@@@@@@
		/*
		#ifdef _DEBUG
		extern edict_t *listenserver_edict;
		DrawBeam(listenserver_edict, vecStart, vecEnd, 5, 10, 250, 10, 15);
		#endif
		/**/




		// check if the centre trace didn't hit anything
		// then we need to check both shoulders/feet to know which side is blocked
		if (tr.flFraction >= 1.0)
		{
			// right side and up
			if (dir == LADDER_UP)
			{
				vecStart = pEdict->v.origin + pEdict->v.view_ofs + gpGlobals->v_right * 30;
				vecEnd = vecStart + gpGlobals->v_up * 50;
			}
			// right side and down
			else
			{
				vecStart = pEdict->v.origin + gpGlobals->v_right * 30;
				vecEnd = vecStart + gpGlobals->v_up * -70;
			}

			UTIL_TraceLine(vecStart, vecEnd, dont_ignore_monsters, pEdict, &tr);



			//@@@@@@@@@!!!!!!!!!@@@@@@@@
			/*
			#ifdef _DEBUG
			extern edict_t *listenserver_edict;
			DrawBeam(listenserver_edict, vecStart, vecEnd, 5, 10, 250, 10, 15);
			#endif
			/**/

			// didn't the right trace hit anything?
			// then try the other side
			if (tr.flFraction >= 1.0)
			{
				// left side and up
				if (dir == LADDER_UP)
				{
					vecStart = pEdict->v.origin + pEdict->v.view_ofs + gpGlobals->v_right * -30;
					vecEnd = vecStart + gpGlobals->v_up * 50;
				}
				// left side and down
				else
				{
					vecStart = pEdict->v.origin + gpGlobals->v_right * -30;
					vecEnd = vecStart + gpGlobals->v_up * -70;
				}

				UTIL_TraceLine(vecStart, vecEnd, dont_ignore_monsters, pEdict, &tr);



				//@@@@@@@@@!!!!!!!!!@@@@@@@@
				/*
				#ifdef _DEBUG
				extern edict_t *listenserver_edict;
				DrawBeam(listenserver_edict, vecStart, vecEnd, 5, 10, 250, 10, 15);
				#endif
				/**/


				// didn't the left trace hit anything? ----->>>>> PROBLEM 
				// (maybe we can try to check backs ie. v_forward * -30 and up/down)
				//
				// NOTE: this doesn't seems to be a big problem, bots were able to unstuck
				// in those few tests I ran
				if (tr.flFraction >= 1.0)
				{


					//@@@@@@@@@
					#ifdef _DEBUG
					char msg[256];
					sprintf(msg, "bot_navigate.cpp|BotHandleLadder() - stuck on ladder -> PROBLEM -> all 3 tracers did hit something!!! -> the bot will probably jump off the ladder\n");
					ALERT(at_console, msg);
					UTIL_DebugDev(msg, pBot->curr_wpt_index, pBot->curr_path_index);
					#endif


				}
				// the left trace did hit something
				else
				{
					// do strafe to the right then
					pBot->f_strafe_direction = 1.0f;
					pBot->f_strafe_time = gpGlobals->time + 0.1f;
					
					
					//@@@@@@@@@
					//#ifdef _DEBUG
					//ALERT(at_console, "stuck on ladder --- left trace hit something!!!\n");
					//#endif
				}

			}
			// the right trace did hit something
			else
			{
				// do strafe to the left then
				pBot->f_strafe_direction = -1.0f;
				pBot->f_strafe_time = gpGlobals->time + 0.1f;


				//@@@@@@@@@
				//#ifdef _DEBUG
				//ALERT(at_console, "stuck on ladder --- right trace hit something!!!\n");
				//#endif
			}
		}
		// the centre trace did hit something
		else
		{


			//@@@@@@@@@
			//#ifdef _DEBUG
			//ALERT(at_console, "stuck on ladder --- centre trace hit something!!!\n");
			//ALERT(at_console, "stuck on ladder - current ladder direction %d!!!\n", dir);
			//#endif

			// for now it seems that this isn't needed ie. the bot is able to
			// unstuck even without "messing" with path navigation


			// do the same as if the bot was by goback wpt ie.
			// change path direction
			// set most recent wpt as current wpt
			// clear all prev. wpts
			// and also change the ladder dir ie.
			// set curr wpt to end ladder wpt
			// set opposite direction (from up to down and vice versa)


		}

		bot_stuck_on_ladder = TRUE;
	}

	// did the bot find the end of ladder if so get the distance to the end
	if (pBot->end_wpt_index != -1)
	{
		end_wpt_distance = fabs(static_cast<double>(pEdict->v.origin.z) - waypoints[pBot->end_wpt_index].origin.z);
	}
	// otherwise try get back to prev wpt
	else
	{
		pBot->end_wpt_index = pBot->prev_wpt_index.get();

		// if no previous wpt use current wpt
		if (pBot->end_wpt_index == -1)
			pBot->end_wpt_index = pBot->curr_wpt_index;
	}

	// if bot is close to the end of ladder
	if ((end_wpt_distance > 0.0) && (end_wpt_distance <= 10.0f))
		pBot->waypoint_top_of_ladder = TRUE;
	// otherwise bot didn't reached ladder end yet
	else
		pBot->waypoint_top_of_ladder = FALSE;

	// climb the ladder up
	if (pBot->ladder_dir == LADDER_UP)
	{
		pEdict->v.v_angle.x = -80;
		pBot->move_speed = SPEED_SLOW;
	}
	// climb the ladder down
	if (pBot->ladder_dir == LADDER_DOWN)
	{
		pEdict->v.v_angle.x = 80;
		pBot->move_speed = SPEED_SLOW;
	}

	// don't move forward when stuck
	if (bot_stuck_on_ladder == FALSE)
		pEdict->v.button |= IN_FORWARD;

	return TRUE;
}


/*
* returns TRUE if bot can't do side steps to the left
* we are checking in which Stance the bot is and then
* we are sending tracelines at important body parts for that Stance
*/
bool BotCantStrafeLeft(edict_t *pEdict)
{
	Vector v_src, v_left;
	TraceResult tr;

	UTIL_MakeVectors(pEdict->v.v_angle);

	bool trace_head = FALSE;
	bool trace_waist = FALSE;

	// is bot fully proned send only one traceline
	if (pEdict->v.flags & FL_PRONED)
	{
		trace_head = TRUE;
	}
	// otherwise traceline all
	else
	{
		trace_head = TRUE;
		trace_waist = TRUE;
	}

	// do a trace to the left at head level
	if (trace_head)
	{
		v_src = pEdict->v.origin + pEdict->v.view_ofs;
		v_left = v_src + gpGlobals->v_right * -100;  // 100 units to the left

		UTIL_TraceLine(v_src, v_left, dont_ignore_monsters,	pEdict->v.pContainingEntity, &tr);

		// bot's head will something return can't strafe there
		if (tr.flFraction < 1.0)
			return TRUE;
	}

	// do a trace to the left at waist level
	if (trace_waist)
	{
		v_src = pEdict->v.origin;
		v_left = v_src + gpGlobals->v_right * -100;  // 100 units to the left

		UTIL_TraceLine(v_src, v_left, dont_ignore_monsters,	pEdict->v.pContainingEntity, &tr);

		if (tr.flFraction < 1.0)
			return TRUE;
	}

	return FALSE;	// the way is clear return can strafe there
}


/*
* returns TRUE if bot can't do side steps to the right
* we are checking in which Stance the bot is and then
* we are sending tracelines at important body parts for that Stance
*/
bool BotCantStrafeRight(edict_t *pEdict)
{
	Vector v_src, v_left;
	TraceResult tr;

	UTIL_MakeVectors(pEdict->v.v_angle);

	bool trace_head = FALSE;
	bool trace_waist = FALSE;

	if (pEdict->v.flags & FL_PRONED)
	{
		trace_head = TRUE;
	}
	else
	{
		trace_head = TRUE;
		trace_waist = TRUE;
	}

	if (trace_head)
	{
		v_src = pEdict->v.origin + pEdict->v.view_ofs;
		v_left = v_src + gpGlobals->v_right * 100;

		UTIL_TraceLine(v_src, v_left, dont_ignore_monsters,	pEdict->v.pContainingEntity, &tr);

		if (tr.flFraction < 1.0)
			return TRUE;
	}

	if (trace_waist)
	{
		v_src = pEdict->v.origin;
		v_left = v_src + gpGlobals->v_right * 100;

		UTIL_TraceLine(v_src, v_left, dont_ignore_monsters,	pEdict->v.pContainingEntity, &tr);

		if (tr.flFraction < 1.0)
			return TRUE;
	}

	return FALSE;
}


/*
* returns TRUE if the bot can follow his teammate (the one who "uses" him)
*/
bool BotFollowTeammate(bot_t *pBot)
{
	edict_t *pEdict = pBot->pEdict;
	Vector vec_end;

	// is bot NOT under water?
	if ((pEdict->v.waterlevel != 2) && (pEdict->v.waterlevel != 3))
	{
		// reset pitch to 0 (level horizontally)
		pEdict->v.idealpitch = 0;
		pEdict->v.v_angle.x = 0;
	}
	
	// is the user dead?
	if (IsAlive(pBot->pBotUser) == FALSE)
	{
		// no one to follow
		pBot->pBotUser = nullptr;
		
		return FALSE;
	}

	vec_end = pBot->pBotUser->v.origin + pBot->pBotUser->v.view_ofs;

	bool user_visible = FInViewCone(&vec_end, pEdict) && FVisible(vec_end, pEdict);

	// not in FOV and visible so check if user is quite close at least
	if (user_visible == FALSE)
	{
		const float dist = (pEdict->v.origin - pBot->pBotUser->v.origin).Length();

		// if the user is close set that he is still visible
		if (dist < 300)
			user_visible = TRUE;
	}
	
	// check if the "user" is still visible
	if ((user_visible) || (pBot->f_bot_use_time + 2.0 > gpGlobals->time))
	{
		Vector aim_vec;

		// update use time if "user" is visible
		if (user_visible)
			pBot->f_bot_use_time = gpGlobals->time;

		// how far away is the "user"?
		const float f_distance = (pBot->pBotUser->v.origin - pEdict->v.origin).Length();
		
		// don't move if close enough
		if (f_distance < 115)
		{
			pBot->move_speed = SPEED_NO;
			pBot->SetDontCheckStuck("bot_navigate.cpp|Bot FollowTeammate() -> close enough to him so stop moving");
		}
		// walk if not too far from user
		else if (f_distance < 150)
			pBot->move_speed = SPEED_SLOW;
		// run to catch the user
		else
			pBot->move_speed = SPEED_MAX;

		// does the bot just stand still
		if ((pBot->move_speed == SPEED_NO) && (pBot->f_aim_at_target_time < gpGlobals->time))
		{
			const int direction = RANDOM_LONG(1, 100);
			UTIL_MakeVectors(pBot->pEdict->v.v_angle);

			// watch the right
			if (direction <= 35)
			{
				vec_end = pBot->pEdict->v.origin + gpGlobals->v_right * 40;
				aim_vec = pBot->pEdict->v.origin - vec_end;
			}
			// watch the left
			else if (direction <= 70)
			{
				vec_end = pBot->pEdict->v.origin - gpGlobals->v_right * 40;
				aim_vec = pBot->pEdict->v.origin - vec_end;
			}
			// otherwise watch the back
			else
			{
				vec_end = pBot->pEdict->v.origin - gpGlobals->v_forward * 40;
				aim_vec = pBot->pEdict->v.origin - vec_end;
			}

			// set some time to watch new direction
			pBot->f_aim_at_target_time = gpGlobals->time + RANDOM_FLOAT(1.0, 2.5);

			// store current aiming vector
			// we can use waypoint origin variable, because we don't use
			// waypoints during follow teammate feature
			pBot->wpt_origin = aim_vec;
		}
		// otherwise face your user
		else if (pBot->move_speed > SPEED_NO)
		{
			aim_vec = pBot->pBotUser->v.origin - pEdict->v.origin;
			pBot->wpt_origin = aim_vec;
		}

		const Vector bot_angles = UTIL_VecToAngles(pBot->wpt_origin);
		pEdict->v.ideal_yaw = bot_angles.y;
		BotFixIdealYaw(pEdict);

		return TRUE;
	}
	else
	{
		// person to follow has gone out of sight
		pBot->pBotUser = nullptr;
	}
	
	return FALSE;
}


/*
* returns TRUE if bot medic can reach his patient
*/
bool BotMedicGetToPatient(bot_t *pBot)
{
	bool patient_visible = FALSE;
	edict_t *pEdict = pBot->pEdict;

	// is bot NOT under water?
	if ((pEdict->v.waterlevel != 2) && (pEdict->v.waterlevel != 3))
	{
		// reset pitch to 0 (level horizontally)
		pEdict->v.idealpitch = 0;
		pEdict->v.v_angle.x = 0;
	}
	
	// is the patient dead?
	if (IsAlive(pBot->pBotEnemy) == FALSE)
	{

		#ifdef _DEBUG
		//@@@@@@@@@@@@@
		//ALERT(at_console, "***BotMedicGetToPatient() - PATIENT is dead\n");
		#endif



		return FALSE;
	}

	const float distance = (pEdict->v.origin - pBot->pBotEnemy->v.origin).Length();

	// check if the patient is visible
	const Vector v_bothead = pEdict->v.origin + pEdict->v.view_ofs;
	Vector v_patient = pBot->pBotEnemy->v.origin + pBot->pBotEnemy->v.view_ofs;
	TraceResult tr;

	UTIL_TraceLine(v_bothead, v_patient, ignore_monsters, pEdict, &tr);

	// sees the bot the wounded soldier?
	if ((tr.flFraction >= 1.0) ||
		((tr.flFraction >= 0.95) && (strcmp("player", STRING(tr.pHit->v.classname)) == 0)))
		patient_visible = TRUE;

	if (patient_visible == FALSE)
		patient_visible = FInViewCone(&v_patient, pEdict);

	// if the patient isn't visible, but still quite close
	if ((patient_visible == FALSE) && (distance < 300))
	{
		// set that he is still visible
		patient_visible = TRUE;
	}

	// patient is visible OR has been just found
	if ((patient_visible) || (pBot->f_bot_see_enemy_time + 2.0 > gpGlobals->time))
	{
		// update the time bot last saw his patient if he is visible right now
		if (patient_visible)
			pBot->f_bot_see_enemy_time = gpGlobals->time;

		// face him
		const Vector v_patient = pBot->pBotEnemy->v.origin - pEdict->v.origin;
		const Vector bot_angles = UTIL_VecToAngles(v_patient);
		
		pEdict->v.ideal_yaw = bot_angles.y;
		BotFixIdealYaw(pEdict);

		// don't move if already in "heal" distance
		if (distance < 70)
		{
			// we have to be closer in FA2.5 and below
			if (UTIL_IsOldVersion())
			{
				if (distance < 50)
					pBot->move_speed = SPEED_NO;
				else
					pBot->move_speed = SPEED_SLOW;
			}
			else
				pBot->move_speed = SPEED_NO;
		}
		// if we are quite close slow down
		else if (distance < 120)
			pBot->move_speed = SPEED_SLOW;
		// otherwise run
		else
			pBot->move_speed = SPEED_MAX;

		return TRUE;
	}



	#ifdef _DEBUG
	//@@@@@@@@@@@@@
	//ALERT(at_console, "***BotMedicGetToPatient() - PATIENT probably isn't visible\n");
	#endif



	return FALSE;
}


/*
* returns TRUE if bot medic can reach the corpse
* we also check if the corpse is still "alive"
*/
bool BotMedicGetToCorpse(bot_t *pBot, float &distance)
{
	edict_t *pEdict = pBot->pEdict;
	edict_t *pent = nullptr;
	bool found_it = FALSE;

	if ((pEdict->v.waterlevel != 2) && (pEdict->v.waterlevel != 3))
	{
		pEdict->v.idealpitch = 0;
		pEdict->v.v_angle.x = 0;
	}

	// the search radius is based on bot skill
	const float radius = 325 - ((pBot->bot_skill + 1) * 25);

	// check if the bodyque is still there
	while ((pent = UTIL_FindEntityInSphere(pent, pEdict->v.origin, radius )) != nullptr)
	{
		char item_name[64];
		strcpy(item_name, STRING(pent->v.classname));

		// is the entity a bodyque AND is it the one we are looking for?
		if ((strcmp("bodyque", item_name) == 0) && (pent == pBot->pBotEnemy))
		{
			found_it = TRUE;
			break;
		}
	}

	// we didn't find it (probably disappeared) so break the whole thing
	if (found_it == FALSE)
		return FALSE;

	distance = (pEdict->v.origin - pBot->pBotEnemy->v.origin).Length();
	const Vector v_corpse = pBot->pBotEnemy->v.origin - pEdict->v.origin;
	const Vector bot_angles = UTIL_VecToAngles(v_corpse);
	
	pEdict->v.ideal_yaw = bot_angles.y;
	BotFixIdealYaw(pEdict);
	
	if (distance < 70)
	{
		if (UTIL_IsOldVersion())
		{
			if (distance < 60)
				pBot->move_speed = SPEED_NO;
			else
				pBot->move_speed = SPEED_SLOW;
		}
		else
			pBot->move_speed = SPEED_NO;
	}
	else if (distance < 140)
		pBot->move_speed = SPEED_SLOW;
	else
		pBot->move_speed = SPEED_MAX;

	pBot->f_dont_look_for_waypoint_time = gpGlobals->time;

	return TRUE;
}


/*
* guides the bot to successfully evade any claymore he sees
*/
void BotEvadeClaymore(bot_t *pBot)
{
	edict_t *pEdict = pBot->pEdict;

	const Vector vecClay = pBot->v_ground_item - pEdict->v.origin;

	// get the distance to claymore
	const float distance_to_mine = vecClay.Length();

	// the bot is quite far from the claymore so we can use standard navigation
	// needs to be a bit bigger than botfinditem() search radius to prevent 
	// immediate canceling
	if (distance_to_mine > (FIND_ITEM_RADIUS + 20.0))
	{
		if (botdebugger.IsDebugActions())
			ALERT(at_console, "***EvadeClaymore() - in safe distance now -> evasion behaviour canceled!\n");

		// the bot can't blow this mine so reset all evasion behaviour
		pBot->RemoveTask(TASK_CLAY_EVADE);
		pBot->RemoveTask(TASK_CLAY_IGNORE);
		pBot->RemoveSubTask(ST_EVASIONSTARTED);//																	NEW CODE 094
		pBot->v_ground_item = g_vecZero;//Vector(0, 0, 0);

		return;
	}

	// the bot has to pretend he never saw this claymore (ie. no evasion actions can be started)
	if (pBot->IsTask(TASK_CLAY_IGNORE))
		return;

	// we can crawl through claymores in FA so let the bot continue
	if (pBot->IsBehaviour(BOT_PRONED))
		return;

	// if the bot is in safe distance or not heading directly to claymore
	// break it (ie. bot doesn't need to evade this claymore)
	//if ((distance_to_mine > 120) || (InFieldOfView(pEdict, vecClay) > 45))//										NEW CODE 094 (prev code)
	if (distance_to_mine > 120)
	{

		#ifdef _DEBUG
		//@@@@@@@@@@@@@
		//ALERT(at_console, "***evadeclaymore() - bot ISN'T that CLOSE to the claymore -> do nothing (gTime=%0.3f)\n", gpGlobals->time);
		#endif



		return;
	}
	//																												NEW CODE 094
	// the bot cannot cheat so if the mine is out of field of view then do nothing
	else if (InFieldOfView(pEdict, vecClay) > 45)
	{


#ifdef _DEBUG
		//@@@@@@@@@@@@@
		//ALERT(at_console, "***evadeclaymore() - claymore ISN'T in FOV -> do nothing (gTime=%0.3f)\n", gpGlobals->time);
#endif


		return;
	}
	// *********************************************************************************************************	NEW CODE 094 END

	// the bot must really head directly to it so do some action so he won't blow it
	else
	{
		
		//TODO: add a code to allow bot to go around this mine
		//		ie. check both sides for enough space to slip around it in safe distance
		//		once this is done add a randomly generated action ie. some percentages for
		//		jumping over it, evading it from left and evading it from right



		

		// TEMP: only now, it'll go to .h file once the code is finished
		extern bool UTIL_TraceObjectsSides(edict_t *pEdict, int side, const Vector &obj_origin);


		// try to run around its left side if possible
		if (!pBot->IsSubTask(ST_EVASIONSTARTED) && UTIL_TraceObjectsSides(pEdict, SIDE_LEFT, pBot->v_ground_item))
		{
			#ifdef _DEBUG
			//@@@@@@@@@@@@@
			//ALERT(at_console, "***BotEvadeClaymore() - bot CAN evade this claymore from LEFT!!!\n");
			#endif

			pBot->SetSubTask(ST_EVASIONSTARTED);
		}
		// try the right side if possible
		else if (!pBot->IsSubTask(ST_EVASIONSTARTED) && UTIL_TraceObjectsSides(pEdict, SIDE_RIGHT, pBot->v_ground_item))
		{

			#ifdef _DEBUG
			//@@@@@@@@@@@@@
			//ALERT(at_console, "***BotEvadeClaymore() - bot CAN evade this claymore from RIGHT!!!\n");
			#endif

			pBot->SetSubTask(ST_EVASIONSTARTED);
		}
		//																											NEW CODE 094
		// if NOT moving at full speed then crawl through it
		// move speed will be accurate at the end of current botthink() frame, but it isn't now
		// so we must use previous speed, because that one holds accurate value
		// (ie. how fast was the bot moving in last frame)
		else if (!pBot->IsSubTask(ST_EVASIONSTARTED) && (pBot->prev_speed < SPEED_MAX) &&
			!pBot->IsSubTask(ST_CANTPRONE))
		{

#ifdef _DEBUG
			//@@@@@@@@@@@@@
			if (botdebugger.IsDebugActions())
				ALERT(at_console, "***EvadeClaymore() -> move speed (curr speed=%d) < MAX SPEED (=%d) so have to prone through it (curr_wpt=%d) gTime=%0.3f\n",
					pBot->prev_speed, SPEED_MAX, pBot->curr_wpt_index + 1, gpGlobals->time);
#endif


			if (UTIL_GoProne(pBot, "bot_navigate.cpp|EvadeClaymore() -> move speed < MAX SPEED so have to prone through it"))
				pBot->SetSubTask(ST_EVASIONSTARTED);
		}
		else
		{

			// TODO: There needs to be test if the bot is moving towards steep hill or stairway
			//		because that case often ends up landing right on the claymore and detonating it
			//		(perhaps testing current wpt z-coor and the next wpt z-coor would work)		<<<<---------------------------------<<<<<--------------------------<<<<<



			// don't jump until quite close to the claymore AND is the bot on ground AND
			// has enough stamina (otherwise the jump won't be allowed)
			if (!pBot->IsSubTask(ST_EVASIONSTARTED) && (distance_to_mine < 100) &&
				(pEdict->v.flags & FL_ONGROUND) && !pBot->IsTask(TASK_NOJUMP))
			{
#ifdef _DEBUG
				//@@@@@@@@@@@@@
				if (botdebugger.IsDebugActions())
					ALERT(at_console, "***EvadeClaymore() -> going to duckjump over the claymore (curr_wpt=%d gTime=%0.3f)\n",
						pBot->curr_wpt_index + 1, gpGlobals->time);
#endif

				pEdict->v.button |= IN_JUMP;
				pBot->f_duckjump_time = gpGlobals->time + 0.5f;	// duckjump is the safer way over it
				pBot->SetSubTask(ST_EVASIONSTARTED);

				float curr_wpt_range = 9999.0f;

				if (pBot->curr_wpt_index != -1)
					curr_wpt_range = waypoints[pBot->curr_wpt_index].range;

				// see if current waypoint range is quite small
				if (curr_wpt_range < 50)
				{

#ifdef _DEBUG
					//@@@@@@@@@@@@@
					//if (botdebugger.IsDebugActions())

					char cemsg[128];
					sprintf(cemsg, "***EvadeClaymore() - small wpt range (#%d<%0.2f>) -> targetting next wpt\n",
							pBot->curr_wpt_index + 1, curr_wpt_range);
					HudNotify(cemsg, pBot);
#endif


					// in this case we should target the next waypoint to prevent the bot
					// to blow the mine when he's trying to reach the waypoint
					// (ie. be in its range)
					// basically we will just skip current waypoint and we'll go for
					// the next waypoint
					pBot->SetNeed(NEED_NEXTWPT);
				}
			}
		}

		// *****************************************************************************************************	NEW CODE 094 END

	}
}


/*			NOT USING
void BotOnLadder( bot_t *pBot, float moved_distance )
{
   Vector v_src, v_dest, view_angles;
   TraceResult tr;
   float angle = 0.0;
   bool done = FALSE;

   edict_t *pEdict = pBot->pEdict;

   // check if the bot has JUST touched this ladder...
   if (pBot->ladder_dir == LADDER_UNKNOWN)
   {
      // try to square up the bot on the ladder...
      while ((!done) && (angle < 180.0))
      {
         // try looking in one direction (forward + angle)
         view_angles = pEdict->v.v_angle;
         view_angles.y = pEdict->v.v_angle.y + angle;

         if (view_angles.y < 0.0)
            view_angles.y += 360.0;
         if (view_angles.y > 360.0)
            view_angles.y -= 360.0;

         UTIL_MakeVectors( view_angles );

         v_src = pEdict->v.origin + pEdict->v.view_ofs;
         v_dest = v_src + gpGlobals->v_forward * 30;

         UTIL_TraceLine( v_src, v_dest, dont_ignore_monsters,
                         pEdict->v.pContainingEntity, &tr);

         if (tr.flFraction < 1.0)  // hit something?
         {
            if (strcmp("func_wall", STRING(tr.pHit->v.classname)) == 0)
            {
               // square up to the wall...
               view_angles = UTIL_VecToAngles(tr.vecPlaneNormal);

               // Normal comes OUT from wall, so flip it around...
               view_angles.y += 180;

               if (view_angles.y > 180)
                  view_angles.y -= 360;

               pEdict->v.ideal_yaw = view_angles.y;

               BotFixIdealYaw(pEdict);

               done = TRUE;
            }
         }
         else
         {
            // try looking in the other direction (forward - angle)
            view_angles = pEdict->v.v_angle;
            view_angles.y = pEdict->v.v_angle.y - angle;

            if (view_angles.y < 0.0)
               view_angles.y += 360.0;
            if (view_angles.y > 360.0)
               view_angles.y -= 360.0;

            UTIL_MakeVectors( view_angles );

            v_src = pEdict->v.origin + pEdict->v.view_ofs;
            v_dest = v_src + gpGlobals->v_forward * 30;

            UTIL_TraceLine( v_src, v_dest, dont_ignore_monsters,
                            pEdict->v.pContainingEntity, &tr);

            if (tr.flFraction < 1.0)  // hit something?
            {
               if (strcmp("func_wall", STRING(tr.pHit->v.classname)) == 0)
               {
                  // square up to the wall...
                  view_angles = UTIL_VecToAngles(tr.vecPlaneNormal);

                  // Normal comes OUT from wall, so flip it around...
                  view_angles.y += 180;

                  if (view_angles.y > 180)
                     view_angles.y -= 360;

                  pEdict->v.ideal_yaw = view_angles.y;

                  BotFixIdealYaw(pEdict);

                  done = TRUE;
               }
            }
         }

         angle += 10;
      }

      if (!done)  // if didn't find a wall, just reset ideal_yaw...
      {
         // set ideal_yaw to current yaw (so bot won't keep turning)
         pEdict->v.ideal_yaw = pEdict->v.v_angle.y;

         BotFixIdealYaw(pEdict);
      }
   }

   // moves the bot up or down a ladder.  if the bot can't move
   // (i.e. get's stuck with someone else on ladder), the bot will
   // change directions and go the other way on the ladder.

   if (pBot->ladder_dir == LADDER_UP)  // is the bot currently going up?
   {
      pEdict->v.v_angle.x = -60;  // look upwards

      // check if the bot hasn't moved much since the last location...
      if ((moved_distance <= 1) && (pBot->prev_speed >= 1.0))
      {
         // the bot must be stuck, change directions...

         pEdict->v.v_angle.x = 60;  // look downwards
         pBot->ladder_dir = LADDER_DOWN;
      }
   }
   else if (pBot->ladder_dir == LADDER_DOWN)  // is the bot currently going down?
   {
      pEdict->v.v_angle.x = 60;  // look downwards

      // check if the bot hasn't moved much since the last location...
      if ((moved_distance <= 1) && (pBot->prev_speed >= 1.0))
      {
         // the bot must be stuck, change directions...

         pEdict->v.v_angle.x = -60;  // look upwards
         pBot->ladder_dir = LADDER_UP;
      }
   }
   else  // the bot hasn't picked a direction yet, try going up...
   {
      pEdict->v.v_angle.x = -60;  // look upwards
      pBot->ladder_dir = LADDER_UP;
   }

   // move forward (i.e. in the direction the bot is looking, up or down)
   pEdict->v.button |= IN_FORWARD;
}
*/


void BotUnderWater( bot_t *pBot )
{
   bool found_waypoint = FALSE;

   edict_t *pEdict = pBot->pEdict;

   // are there waypoints in this level
   if (num_waypoints > 0)
   {
      // head towards a waypoint
      found_waypoint = BotHeadTowardWaypoint(pBot);
   }

   if (found_waypoint == FALSE)
   {
      // handle movements under water.  right now, just try to keep from
      // drowning by swimming up towards the surface and look to see if
      // there is a surface the bot can jump up onto to get out of the
      // water.  bots DON'T like water!

      TraceResult tr;

      // swim up towards the surface
      pEdict->v.v_angle.x = -60;  // look upwards
   
      // move forward (i.e. in the direction the bot is looking, up or down)
      pEdict->v.button |= IN_FORWARD;
   
      // set gpGlobals angles based on current view angle (for TraceLine)
      UTIL_MakeVectors( pEdict->v.v_angle );
   
      // look from eye position straight forward (remember: the bot is looking
      // upwards at a 60 degree angle so TraceLine will go out and up...
   
      Vector v_src = pEdict->v.origin + pEdict->v.view_ofs;  // EyePosition()
      Vector v_forward = v_src + gpGlobals->v_forward * 90;
   
      // trace from the bot's eyes straight forward...
      UTIL_TraceLine( v_src, v_forward, dont_ignore_monsters,
                      pEdict->v.pContainingEntity, &tr);
   
      // check if the trace didn't hit anything (i.e. nothing in the way)...
      if (tr.flFraction >= 1.0)
      {
         // find out what the contents is of the end of the trace...
         int contents = UTIL_PointContents(tr.vecEndPos);
   
         // check if the trace endpoint is in open space...
         if (contents == CONTENTS_EMPTY)
         {
            // ok so far, we are at the surface of the water, continue...
   
            v_src = tr.vecEndPos;
            v_forward = v_src;
            v_forward.z -= 90;
   
            // trace from the previous end point straight down...
            UTIL_TraceLine( v_src, v_forward, dont_ignore_monsters,
                            pEdict->v.pContainingEntity, &tr);
   
            // check if the trace hit something...
            if (tr.flFraction < 1.0)
            {
               contents = UTIL_PointContents( tr.vecEndPos );
   
               // if contents isn't water then assume it's land, jump!
               if (contents != CONTENTS_WATER)
               {
                  pEdict->v.button |= IN_JUMP;
               }
            }
         }
      }
   }
}


/*
void BotUseLift( bot_t *pBot, float moved_distance )
{
   edict_t *pEdict = pBot->pEdict;

   // just need to press the button once, when the flag gets set...
   if (pBot->f_use_button_time == gpGlobals->time)
   {
      pEdict->v.button = IN_USE;

      // face opposite from the button
      pEdict->v.ideal_yaw += 180;  // rotate 180 degrees

      BotFixIdealYaw(pEdict);
   }

   // check if the bot has waited too long for the lift to move...
   if (((pBot->f_use_button_time + 2.0) < gpGlobals->time) &&
       (!pBot->b_lift_moving))
   {
      // clear use button flag
      pBot->b_use_button = FALSE;

      // bot doesn't have to set f_find_item since the bot
      // should already be facing away from the button

      pBot->move_speed = SPEED_MAX;
   }

   // check if lift has started moving...
   if ((moved_distance > 1) && (!pBot->b_lift_moving))
   {
      pBot->b_lift_moving = TRUE;
   }

   // check if lift has stopped moving...
   if ((moved_distance <= 1) && (pBot->b_lift_moving))
   {
      TraceResult tr1, tr2;
      Vector v_src, v_forward, v_right, v_left;
      Vector v_down, v_forward_down, v_right_down, v_left_down;

      pBot->b_use_button = FALSE;

      // TraceLines in 4 directions to find which way to go...

      UTIL_MakeVectors( pEdict->v.v_angle );

      v_src = pEdict->v.origin + pEdict->v.view_ofs;
      v_forward = v_src + gpGlobals->v_forward * 90;
      v_right = v_src + gpGlobals->v_right * 90;
      v_left = v_src + gpGlobals->v_right * -90;

      v_down = pEdict->v.v_angle;
      v_down.x = v_down.x + 45;  // look down at 45 degree angle

      UTIL_MakeVectors( v_down );

      v_forward_down = v_src + gpGlobals->v_forward * 100;
      v_right_down = v_src + gpGlobals->v_right * 100;
      v_left_down = v_src + gpGlobals->v_right * -100;

      // try tracing forward first...
      UTIL_TraceLine( v_src, v_forward, dont_ignore_monsters,
                      pEdict->v.pContainingEntity, &tr1);
      UTIL_TraceLine( v_src, v_forward_down, dont_ignore_monsters,
                      pEdict->v.pContainingEntity, &tr2);

      // check if we hit a wall or didn't find a floor...
      if ((tr1.flFraction < 1.0) || (tr2.flFraction >= 1.0))
      {
         // try tracing to the RIGHT side next...
         UTIL_TraceLine( v_src, v_right, dont_ignore_monsters,
                         pEdict->v.pContainingEntity, &tr1);
         UTIL_TraceLine( v_src, v_right_down, dont_ignore_monsters,
                         pEdict->v.pContainingEntity, &tr2);

         // check if we hit a wall or didn't find a floor...
         if ((tr1.flFraction < 1.0) || (tr2.flFraction >= 1.0))
         {
            // try tracing to the LEFT side next...
            UTIL_TraceLine( v_src, v_left, dont_ignore_monsters,
                            pEdict->v.pContainingEntity, &tr1);
            UTIL_TraceLine( v_src, v_left_down, dont_ignore_monsters,
                            pEdict->v.pContainingEntity, &tr2);

            // check if we hit a wall or didn't find a floor...
            if ((tr1.flFraction < 1.0) || (tr2.flFraction >= 1.0))
            {
               // only thing to do is turn around...
               pEdict->v.ideal_yaw += 180;  // turn all the way around
            }
            else
            {
               pEdict->v.ideal_yaw += 90;  // turn to the LEFT
            }
         }
         else
         {
            pEdict->v.ideal_yaw -= 90;  // turn to the RIGHT
         }

         BotFixIdealYaw(pEdict);
      }

      BotChangeYaw( pBot, pEdict->v.yaw_speed );

      pBot->move_speed = SPEED_MAX;
   }
}
*/


bool BotStuckInCorner( bot_t *pBot )
{
   TraceResult tr;
   edict_t *pEdict = pBot->pEdict;
   
   UTIL_MakeVectors( pEdict->v.v_angle );

   // trace 45 degrees to the right...
   Vector v_src = pEdict->v.origin;
   Vector v_dest = v_src + gpGlobals->v_forward * 20 + gpGlobals->v_right * 20;

   UTIL_TraceLine( v_src, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   if (tr.flFraction >= 1.0)
      return FALSE;  // no wall, so not in a corner

   // trace 45 degrees to the left...
   v_src = pEdict->v.origin;
   v_dest = v_src + gpGlobals->v_forward*20 - gpGlobals->v_right*20;

   UTIL_TraceLine( v_src, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   if (tr.flFraction >= 1.0)
      return FALSE;  // no wall, so not in a corner

   return TRUE;  // bot is in a corner
}


void BotTurnAtWall( bot_t *pBot, TraceResult *tr )
{
   edict_t *pEdict = pBot->pEdict;
   float Z;

   // Find the normal vector from the trace result.  The normal vector will
   // be a vector that is perpendicular to the surface from the TraceResult.

   Vector Normal = UTIL_VecToAngles(tr->vecPlaneNormal);

   // Since the bot keeps it's view angle in -180 < x < 180 degrees format,
   // and since TraceResults are 0 < x < 360, we convert the bot's view
   // angle (yaw) to the same format at TraceResult.

   float Y = pEdict->v.v_angle.y;
   Y = Y + 180;
   if (Y > 359) Y -= 360;

   // Turn the normal vector around 180 degrees (i.e. make it point towards
   // the wall not away from it.  That makes finding the angles that the
   // bot needs to turn a little easier.

   Normal.y = Normal.y - 180;
   if (Normal.y < 0)
   Normal.y += 360;

   // Here we compare the bots view angle (Y) to the Normal - 90 degrees (Y1)
   // and the Normal + 90 degrees (Y2).  These two angles (Y1 & Y2) represent
   // angles that are parallel to the wall surface, but heading in opposite
   // directions.  We want the bot to choose the one that will require the
   // least amount of turning (saves time) and have the bot head off in that
   // direction.

   float Y1 = Normal.y - 90;
   if (RANDOM_LONG(1, 100) <= 50)
   {
      Y1 = Y1 - RANDOM_FLOAT(5.0, 20.0);
   }
   if (Y1 < 0) Y1 += 360;

   float Y2 = Normal.y + 90;
   if (RANDOM_LONG(1, 100) <= 50)
   {
      Y2 = Y2 + RANDOM_FLOAT(5.0, 20.0);
   }
   if (Y2 > 359) Y2 -= 360;

   // D1 and D2 are the difference (in degrees) between the bot's current
   // angle and Y1 or Y2 (respectively).

   float D1 = fabs(static_cast<double>(Y) - Y1);
   if (D1 > 179) D1 = fabs(static_cast<double>(D1) - 360);
   float D2 = fabs(static_cast<double>(Y) - Y2);
   if (D2 > 179) D2 = fabs(static_cast<double>(D2) - 360);

   // If difference 1 (D1) is more than difference 2 (D2) then the bot will
   // have to turn LESS if it heads in direction Y1 otherwise, head in
   // direction Y2.  I know this seems backwards, but try some sample angles
   // out on some graph paper and go through these equations using a
   // calculator, you'll see what I mean.

   if (D1 > D2)
      Z = Y1;
   else
      Z = Y2;

   // convert from TraceResult 0 to 360 degree format back to bot's
   // -180 to 180 degree format.

   if (Z > 180)
      Z -= 360;

   // set the direction to head off into...
   pEdict->v.ideal_yaw = Z;

   BotFixIdealYaw(pEdict);
}


/*
* uses some TraceLines to determine if anything is blocking the current path of the bot
* returns true if at least one traceline hit something (ie. bot can't move forward)
*/
bool TraceForward(bot_t *pBot, bool check_head, bool check_feet, int dist, TraceResult *tr)
{
	edict_t *pEdict = pBot->pEdict;	
	Vector v_src, v_forward;
	
	UTIL_MakeVectors( pEdict->v.v_angle );
	
	// first do a trace from the bot's eyes forward (if we need it)
	if (check_head)
	{
		v_src = pEdict->v.origin + pEdict->v.view_ofs;  // EyePosition()
		v_forward = v_src + gpGlobals->v_forward * dist;
		
		// trace from the bot's eyes straight forward...
		UTIL_TraceLine( v_src, v_forward, dont_ignore_monsters, pEdict->v.pContainingEntity, tr);



		//@@@@@@@@@@@@@@@@@@
		//#ifdef _DEBUG
		//UTIL_HighlightTrace(v_src, v_forward, pEdict);
		//#endif


		
		// check if the trace hit something...
		if (tr->flFraction < 1.0)
		{
			return TRUE;  // bot's head will hit something
		}
	}
	
	// bot's head is clear (if we checked it), check at waist level...
	
	v_src = pEdict->v.origin;
	v_forward = v_src + gpGlobals->v_forward * dist;
	
	// trace from the bot's waist straight forward...
	UTIL_TraceLine( v_src, v_forward, dont_ignore_monsters, pEdict->v.pContainingEntity, tr);
	


	//@@@@@@@@@@@@@@@@@@
//	#ifdef _DEBUG
//	UTIL_HighlightTrace(v_src, v_forward, pEdict);
//	#endif



	// check if the trace hit something...
	if (tr->flFraction < 1.0)
	{
		return TRUE;  // bot's body will hit something
	}

	// do a trace from the bot's feet forward (if we need it)
	if (check_feet)
	{
		v_src = pEdict->v.origin - pEdict->v.view_ofs;  // FeetPosition
		v_forward = v_src + gpGlobals->v_forward * dist;
		
		// trace from the bot's feet straight forward...
		UTIL_TraceLine( v_src, v_forward, dont_ignore_monsters, pEdict->v.pContainingEntity, tr);



		//@@@@@@@@@@@@@@@@@@
//		#ifdef _DEBUG
//		UTIL_HighlightTrace(v_src, v_forward, pEdict);
//		#endif


		
		// check if the trace hit something...
		if (tr->flFraction < 1.0)
		{
			return TRUE;  // bot's feet will hit something
		}
	}
	
	return FALSE;  // bot can move forward, return false
}


bool BotCantMoveForward( bot_t *pBot, TraceResult *tr )
{
	return TraceForward(pBot, TRUE, FALSE, 40, tr);
}


bool IsForwardBlocked(bot_t *pBot)
{
	TraceResult tr;	// we don't need it here

	return TraceForward(pBot, TRUE, TRUE, 40, &tr);
}


bool IsDeathFall(edict_t *pEdict)
{
	// We need to trace an area before the bot to detect if there isn't a danger
	// of death fall. We send one trace line a few units in front of the bot.

	TraceResult tr;

	// NOTE: Try to use MakeVectors when the bot goes up the hill
	// because now usually that farther trace ends with deathfall danger warning which is bug
	//UTIL_MakeVectors(pEdict->v.v_angle);

	// we have to trace in front of the bot to allow him to react
	const Vector v_source = pEdict->v.origin + pEdict->v.view_ofs + gpGlobals->v_forward * 75;
	
	// 250 units long fall is right about not to break the leg
	const Vector v_dest = v_source - pEdict->v.view_ofs + Vector(0, 0, -250);

	UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);

#ifdef _DEBUG
	//@@@@@@@@@@@@@@@@@@
	//UTIL_HighlightTrace(v_source, v_dest, pEdict);

	/*
	char msg[256];
	sprintf(msg, "tr %.2f (hitgr %d) (flPlaneDist %.2f)\n",
			tr.flFraction, tr.iHitgroup, tr.flPlaneDist);
	HudNotify(msg);
	if (tr.pHit)
	{
		sprintf(msg, "pHit (class %s) (net %s) (glob %s)\n", STRING(tr.pHit->v.classname),
			STRING(tr.pHit->v.netname), STRING(tr.pHit->v.globalname));
		HudNotify(msg);
	}
	*/
#endif

	// if trace hit something, return FALSE ie. no danger of deathfall
	if (tr.flFraction < 1.0)
		return FALSE;

	// there isn't safe ground in front of the bot (ie. the bot shouldn't continue forward)
	return TRUE;
}


bool TraceJumpUp(bot_t *pBot, int height)
{
   // What I do here is trace 3 lines straight out, one unit higher than
   // the highest normal jumping distance.  I trace once at the center of
   // the body, once at the right side, and once at the left side.  If all
   // three of these TraceLines don't hit an obstruction then I know the
   // area to jump to is clear.  I then need to trace from head level,
   // above where the bot will jump to, downward to see if there is anything
   // blocking the jump.  There could be a narrow opening that the body
   // will not fit into.  These horizontal and vertical TraceLines seem
   // to catch most of the problems with falsely trying to jump on something
   // that the bot can not get onto.

   TraceResult tr;
   edict_t *pEdict = pBot->pEdict;

   // convert current view angle to vectors for TraceLine math...

   Vector v_jump = pEdict->v.v_angle;
   v_jump.x = 0;  // reset pitch to 0 (level horizontally)
   v_jump.z = 0;  // reset roll to 0 (straight up and down)

   UTIL_MakeVectors( v_jump );

   // use center of the body first...

   // check one unit above the height
   Vector v_source = pEdict->v.origin + Vector(0, 0, -36 + height + 1);
   Vector v_dest = v_source + gpGlobals->v_forward * 24;

   // trace a line forward at maximum jump height...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0)
      return FALSE;

   // now check same height to one side of the bot...
   v_source = pEdict->v.origin + gpGlobals->v_right * 16 + Vector(0, 0, -36 + height + 1);
   v_dest = v_source + gpGlobals->v_forward * 24;

   // trace a line forward at maximum jump height...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0)
      return FALSE;

   // now check same height on the other side of the bot...
   v_source = pEdict->v.origin + gpGlobals->v_right * -16 + Vector(0, 0, -36 + height + 1);
   v_dest = v_source + gpGlobals->v_forward * 24;

   // trace a line forward at maximum jump height...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0)
      return FALSE;

   // now trace from head level downward to check for obstructions...

   // start of trace is 24 units in front of bot, 72 units above head...
   v_source = pEdict->v.origin + gpGlobals->v_forward * 24;

   // offset 72 units from top of head (72 + 36)
   v_source.z = v_source.z + 108;

   // (108 minus the jump limit height which is height - 36)
   // * -1 because we need negative value
   const int bottom = (108 - (height - 36)) * -1;

   // end point of trace is <bottom> units straight down from start...
   v_dest = v_source + Vector(0, 0, bottom);

   // trace a line straight down toward the ground...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0)
      return FALSE;

   // now check same height to one side of the bot...
   v_source = pEdict->v.origin + gpGlobals->v_right * 16 + gpGlobals->v_forward * 24;
   v_source.z = v_source.z + 108;
   v_dest = v_source + Vector(0, 0, bottom);

   // trace a line straight down toward the ground...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0)
      return FALSE;

   // now check same height on the other side of the bot...
   v_source = pEdict->v.origin + gpGlobals->v_right * -16 + gpGlobals->v_forward * 24;
   v_source.z = v_source.z + 108;
   v_dest = v_source + Vector(0, 0, bottom);

   // trace a line straight down toward the ground...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0)
      return FALSE;

   return TRUE;
}


bool BotCanJumpUp( bot_t *pBot )
{
	// maximum jump height is 45
	return TraceJumpUp(pBot, 45);
}


bool BotCanDuckJumpUp( bot_t *pBot )
{
	// maximum duck jump height is 62
	return TraceJumpUp(pBot, 62);
}


bool BotCanDuckUnder( bot_t *pBot )
{
   // What I do here is trace 3 lines straight out, one unit higher than
   // the ducking height.  I trace once at the center of the body, once
   // at the right side, and once at the left side.  If all three of these
   // TraceLines don't hit an obstruction then I know the area to duck to
   // is clear.  I then need to trace from the ground up, 72 units, to make
   // sure that there is something blocking the TraceLine.  Then we know
   // we can duck under it.

   TraceResult tr;
   edict_t *pEdict = pBot->pEdict;

   // convert current view angle to vectors for TraceLine math...

   Vector v_duck = pEdict->v.v_angle;
   v_duck.x = 0;  // reset pitch to 0 (level horizontally)
   v_duck.z = 0;  // reset roll to 0 (straight up and down)

   UTIL_MakeVectors( v_duck );

   // use center of the body first...

   // duck height is 36, so check one unit above that (37)
   Vector v_source = pEdict->v.origin + Vector(0, 0, -36 + 37);
   Vector v_dest = v_source + gpGlobals->v_forward * 24;

   // trace a line forward at duck height...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0)
      return FALSE;

   // now check same height to one side of the bot...
   v_source = pEdict->v.origin + gpGlobals->v_right * 16 + Vector(0, 0, -36 + 37);
   v_dest = v_source + gpGlobals->v_forward * 24;

   // trace a line forward at duck height...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0)
      return FALSE;

   // now check same height on the other side of the bot...
   v_source = pEdict->v.origin + gpGlobals->v_right * -16 + Vector(0, 0, -36 + 37);
   v_dest = v_source + gpGlobals->v_forward * 24;

   // trace a line forward at duck height...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // if trace hit something, return FALSE
   if (tr.flFraction < 1.0)
      return FALSE;

   // now trace from the ground up to check for object to duck under...

   // start of trace is 24 units in front of bot near ground...
   v_source = pEdict->v.origin + gpGlobals->v_forward * 24;
   v_source.z = v_source.z - 35;  // offset to feet + 1 unit up

   // end point of trace is 72 units straight up from start...
   v_dest = v_source + Vector(0, 0, 72);

   // trace a line straight up in the air...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // if trace didn't hit something, return FALSE
   if (tr.flFraction >= 1.0)
      return FALSE;

   // now check same height to one side of the bot...
   v_source = pEdict->v.origin + gpGlobals->v_right * 16 + gpGlobals->v_forward * 24;
   v_source.z = v_source.z - 35;  // offset to feet + 1 unit up
   v_dest = v_source + Vector(0, 0, 72);

   // trace a line straight up in the air...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // if trace didn't hit something, return FALSE
   if (tr.flFraction >= 1.0)
      return FALSE;

   // now check same height on the other side of the bot...
   v_source = pEdict->v.origin + gpGlobals->v_right * -16 + gpGlobals->v_forward * 24;
   v_source.z = v_source.z - 35;  // offset to feet + 1 unit up
   v_dest = v_source + Vector(0, 0, 72);

   // trace a line straight up in the air...
   UTIL_TraceLine( v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // if trace didn't hit something, return FALSE
   if (tr.flFraction >= 1.0)
      return FALSE;

   return TRUE;
}


void BotRandomTurn( bot_t *pBot )
{
   pBot->move_speed = SPEED_NO;  // don't move while turning
            
   if (RANDOM_LONG(1, 100) <= 10)
   {
      // 10 percent of the time turn completely around...
      pBot->pEdict->v.ideal_yaw += 180;
   }
   else
   {
      // turn randomly between 30 and 60 degress
      if (pBot->wander_dir == SIDE_LEFT)
         pBot->pEdict->v.ideal_yaw += RANDOM_LONG(30, 60);
      else
         pBot->pEdict->v.ideal_yaw -= RANDOM_LONG(30, 60);
   }
            
   BotFixIdealYaw(pBot->pEdict);
}


bool BotCheckWallOnLeft( bot_t *pBot )
{
   edict_t *pEdict = pBot->pEdict;
   TraceResult tr;

   UTIL_MakeVectors( pEdict->v.v_angle );

   // do a trace to the left...

   const Vector v_src = pEdict->v.origin;
   const Vector v_left = v_src + gpGlobals->v_right * -40;  // 40 units to the left

   UTIL_TraceLine( v_src, v_left, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // check if the trace hit something...
   if (tr.flFraction < 1.0)
   {
      if (pBot->f_wall_on_left < 1.0)
         pBot->f_wall_on_left = gpGlobals->time;

      return TRUE;
   }

   return FALSE;
}


bool BotCheckWallOnRight( bot_t *pBot )
{
   edict_t *pEdict = pBot->pEdict;
   TraceResult tr;

   UTIL_MakeVectors( pEdict->v.v_angle );

   // do a trace to the right...

   const Vector v_src = pEdict->v.origin;
   const Vector v_right = v_src + gpGlobals->v_right * 40;  // 40 units to the right

   UTIL_TraceLine( v_src, v_right, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

   // check if the trace hit something...
   if (tr.flFraction < 1.0)
   {
      if (pBot->f_wall_on_right < 1.0)
         pBot->f_wall_on_right = gpGlobals->time;

      return TRUE;
   }

   return FALSE;
}