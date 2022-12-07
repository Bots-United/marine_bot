/***
*
*  Copyright (c) 1999, Valve LLC. All rights reserved.
*
*  This product contains software technology licensed from Id 
*  Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*  All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
///////////////////////////////////////////////////////////////////////////////////////////////
//
// This file contains both GNU as VALVE licenced material.
//
// This source file contains VALVE LCC licence material.
// Read and Agree to the above stated header
// before distibuting or editing this source code.
//
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
// util.cpp
// 
////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(WIN32)
#pragma warning(disable: 4005 91 4996)
#endif

#include "defines.h"

#include "extdll.h"
#include "util.h"
#include "engine.h"

#include "bot.h"
#include "bot_func.h"
#include "bot_manager.h"
#include "bot_weapons.h"
#include "waypoint.h"


int gmsgTextMsg = 0;
int gmsgSayText = 0;
int gmsgShowMenu = 0;

// util functions prototypes
edict_t *UTIL_FindEntityByString( edict_t *pentStart, const char *szKeyword, const char *szValue );
void UTIL_BuildFileName(char *filename, char *arg1, char *arg2, bool separator);
inline void SelectMainWeapon(bot_t *pBot);
inline void SelectBackupWeapon(bot_t *pBot);
inline void SelectGrenade(bot_t *pBot);
bool IgnoreFireMode(int weapon);
bool BurstFireMode(int weapon);
bool FullAutoFireMode(int weapon);
bool IsStringValid(char *str);
void ProcessTheName(char *name);
void ShortenIt(char *name);
void DumpVector(FILE *f, Vector vec);


Vector UTIL_VecToAngles( const Vector &vec )
{
   float rgflVecOut[3];
   VEC_TO_ANGLES(vec, rgflVecOut);
   return Vector(rgflVecOut);
}


// Overloaded to add IGNORE_GLASS
void UTIL_TraceLine( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, IGNORE_GLASS ignoreGlass, edict_t *pentIgnore, TraceResult *ptr )
{
   TRACE_LINE( vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE) | (ignoreGlass?0x100:0), pentIgnore, ptr );
}


void UTIL_TraceLine( const Vector &vecStart, const Vector &vecEnd, IGNORE_MONSTERS igmon, edict_t *pentIgnore, TraceResult *ptr )
{
   TRACE_LINE( vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE), pentIgnore, ptr );
}


void UTIL_MakeVectors( const Vector &vecAngles )
{
   MAKE_VECTORS( vecAngles );
}


edict_t *UTIL_FindEntityInSphere( edict_t *pentStart, const Vector &vecCenter, float flRadius )
{
	edict_t* pentEntity = FIND_ENTITY_IN_SPHERE(pentStart, vecCenter, flRadius);

   if (!FNullEnt(pentEntity))
      return pentEntity;

   return nullptr;
}


// search for entity passed by its classname
edict_t *UTIL_FindEntityInSphere(edict_t *pCallerEdict, const char *ent_name)
{
	edict_t *pEntity = nullptr;
	
	while ((pEntity = UTIL_FindEntityInSphere(pEntity, pCallerEdict->v.origin, PLAYER_SEARCH_RADIUS)) != nullptr)
	{
		if (strcmp(STRING(pEntity->v.classname), ent_name) == 0)
			return pEntity;
	}

	return nullptr;
}


edict_t *UTIL_FindEntityByString( edict_t *pentStart, const char *szKeyword, const char *szValue )
{
	edict_t* pentEntity = FIND_ENTITY_BY_STRING(pentStart, szKeyword, szValue);

   if (!FNullEnt(pentEntity))
      return pentEntity;
   return nullptr;
}


edict_t *UTIL_FindEntityByClassname( edict_t *pentStart, const char *szName )
{
   return UTIL_FindEntityByString( pentStart, "classname", szName );
}


/*		NEVER USED
edict_t *UTIL_FindEntityByTargetname( edict_t *pentStart, const char *szName )
{
   return UTIL_FindEntityByString( pentStart, "targetname", szName );
}
*/


// Defined in util.h
int UTIL_PointContents( const Vector &vec )
{
   return POINT_CONTENTS(vec);
}


// Defined in util.h	- NEVER USED
void UTIL_SetSize( entvars_t *pev, const Vector &vecMin, const Vector &vecMax )
{
   SET_SIZE( ENT(pev), vecMin, vecMax );
}


// Defined in util.h	- NEVER USED
void UTIL_SetOrigin( entvars_t *pev, const Vector &vecOrigin )
{
   SET_ORIGIN(ENT(pev), vecOrigin );
}


void ClientPrint( edict_t *pEntity, int msg_dest, const char *msg_name)
{
   if (gmsgTextMsg == 0)
      gmsgTextMsg = REG_USER_MSG( "TextMsg", -1 );

   pfnMessageBegin( MSG_ONE, gmsgTextMsg, nullptr, pEntity );
   pfnWriteByte( msg_dest );
   pfnWriteString( msg_name );
   pfnMessageEnd();
}


/*		NEVER USED
void UTIL_SayText( const char *pText, edict_t *pEdict )
{
   if (gmsgSayText == 0)
      gmsgSayText = REG_USER_MSG( "SayText", -1 );

   pfnMessageBegin( MSG_ONE, gmsgSayText, NULL, pEdict );
   pfnWriteByte( ENTINDEX(pEdict) );
   pfnWriteString( pText );
   pfnMessageEnd();
}


void UTIL_HostSay( edict_t *pEntity, int teamonly, char *message )
{
   int   j;
   char  text[128];
   char *pc;
   int   sender_team, player_team;
   edict_t *client;

   // make sure the text has content
   for ( pc = message; pc != NULL && *pc != 0; pc++ )
   {
      if ( isprint( *pc ) && !isspace( *pc ) )
      {
         pc = NULL;   // we've found an alphanumeric character,  so text is valid
         break;
      }
   }

   if ( pc != NULL )
      return;  // no character found, so say nothing

   // turn on color set 2  (color on,  no sound)
   if ( teamonly )
      sprintf( text, "%c(TEAM) %s: ", 2, STRING( pEntity->v.netname ) );
   else
      sprintf( text, "%c%s: ", 2, STRING( pEntity->v.netname ) );

   j = sizeof(text) - 2 - strlen(text);  // -2 for /n and null terminator
   if ( (int)strlen(message) > j )
      message[j] = 0;

   strcat( text, message );
   strcat( text, "\n" );

   // loop through all players
   // Start with the first player.
   // This may return the world in single player if the client types something between levels or during spawn
   // so check it, or it will infinite loop

   if (gmsgSayText == 0)
      gmsgSayText = REG_USER_MSG( "SayText", -1 );

   sender_team = UTIL_GetTeam(pEntity);

   client = NULL;
   while ( ((client = UTIL_FindEntityByClassname( client, "player" )) != NULL) &&
           (!FNullEnt(client)) ) 
   {
      if ( client == pEntity )  // skip sender of message
         continue;

      player_team = UTIL_GetTeam(client);

      if ( teamonly && (sender_team != player_team) )
         continue;

      pfnMessageBegin( MSG_ONE, gmsgSayText, NULL, client );
         pfnWriteByte( ENTINDEX(pEntity) );
         pfnWriteString( text );
      pfnMessageEnd();
   }

   // print to the sending client
   pfnMessageBegin( MSG_ONE, gmsgSayText, NULL, pEntity );
      pfnWriteByte( ENTINDEX(pEntity) );
      pfnWriteString( text );
   pfnMessageEnd();
   
   // echo to server console
   g_engfuncs.pfnServerPrint( text );
}
*/


#ifdef   DEBUG
edict_t *DBG_EntOfVars( const entvars_t *pev )
{
   if (pev->pContainingEntity != NULL)
      return pev->pContainingEntity;
   ALERT(at_console, "entvars_t pContainingEntity is NULL, calling into engine");
   edict_t* pent = (*g_engfuncs.pfnFindEntityByVars)((entvars_t*)pev);
   if (pent == NULL)
      ALERT(at_console, "DAMN!  Even the engine couldn't FindEntityByVars!");
   ((entvars_t *)pev)->pContainingEntity = pent;
   return pent;
}
#endif //DEBUG


/*
* return the team based either on team variable or on the model "name" the Edict uses
*/
int UTIL_GetTeam(edict_t *pEntity)
{
	if (pEntity == nullptr)
	{
#ifdef _DEBUG
		char msg[256];
		sprintf(msg, "<FATAL ERROR>(util.cpp|GetTeam()) pEntity is NULL\n");
		UTIL_DebugDev(msg, -100, -100);
		HudNotify(msg);
#endif

		return TEAM_NONE;
	}

	if (mod_id == FIREARMS_DLL)
	{
		// try the team variable first
		// this works in most of situations so it should be on top to save some CPU time
		if (pEntity->v.team == TEAM_RED)
			return TEAM_RED;
		if (pEntity->v.team == TEAM_BLUE)
			return TEAM_BLUE;

		// works for getting team info from thrown grenades in all supported versions
		if (pEntity->v.owner != nullptr)
			return UTIL_GetTeam(pEntity->v.owner);

		/*		PREV CODE - works well in FA 29 and so on, but not in 25 etc.
		if ((pEntity->v.owner != NULL) && (pEntity->v.owner->v.team == TEAM_RED))
			return TEAM_RED;
		else if ((pEntity->v.owner != NULL) && (pEntity->v.owner->v.team == TEAM_BLUE))
			return TEAM_BLUE;
		*/
		
		// try netname is the owner pointer failed
		// happens when the owner just left the game or something
		if (strcmp(STRING(pEntity->v.netname), "Red Force") == 0)
			return TEAM_RED;
		else if (strcmp(STRING(pEntity->v.netname), "Blue Force") == 0)
			return TEAM_BLUE;

		/*/
#ifdef _DEBUG
		if (UTIL_IsOldVersion() == FALSE)
			HudNotify("***Can't get team from edict.team using edict.model\n");
#endif
		/**/

		// if previous checks failed then try to get the team info from the model
		// (works in older FA versions)
		// Red team - sand(brown) camouflage
		if (strstr(STRING(pEntity->v.model), "red1") != nullptr)
			return TEAM_RED;
		// Blue team - jungle(green) camouflage
		else if (strstr(STRING(pEntity->v.model), "blue1") != nullptr)
			return TEAM_BLUE;
	}


#ifdef _DEBUG

	// don't print an error message in these situations
	// (ie. they are quite common and don't cause problems)
	//
	// 1)test if the client didn't get in game yet (currently picking a team, class etc.)
	//   FA doesn't seem to set spectator flag or this combination when someone spectates
	//   so this cannot handle players who do spectate
	// 2)the game world has no team so we must break it here
	//   happens when the bot get hurt after falling from something higher
	bool okay = TRUE;

	if (UTIL_IsOldVersion())
	{
		// comment it out if needed
		//okay = FALSE;
	}

	if (!(pEntity->v.flags & (FL_CLIENT & FL_NOTARGET & FL_SPECTATOR)) && okay)
	{
		char msg[256];
		if ((pEntity->v.netname != NULL) && (pEntity->v.classname != NULL))
			sprintf(msg, "<BUG>Can't get team even from edict.model for edict <%s> (class <%s>)\n",
			STRING(pEntity->v.netname), STRING(pEntity->v.classname));
		else if (pEntity->v.classname != NULL)
			sprintf(msg, "<BUG>Can't get team even from edict.model for edict <no netname> (class <%s>)\n",
			STRING(pEntity->v.classname));
		else if (pEntity->v.netname != NULL)
			sprintf(msg, "<BUG>Can't get team even from edict.model for edict <%s> (class <no classname>)\n",
			STRING(pEntity->v.netname));
		else
			sprintf(msg, "<BUG>Can't get team even from edict.model for edict <no netname> (class <no classname>)\n");
		
		// don't dump grenades placed in the map as item (ie you can pick them up)
		if ((pEntity->v.solid == SOLID_TRIGGER) && (pEntity->v.movetype == MOVETYPE_TOSS))
			return TEAM_NONE;

		HudNotify(msg, true);
	}
#endif

	return TEAM_NONE;  // return this flag if team is unknown
}


int UTIL_GetBotIndex(edict_t *pEdict)
{
	for (int index = 0; index < MAX_CLIENTS; index++)
   {
      if (bots[index].pEdict == pEdict)
      {
         return index;
      }
   }

   return -1;  // return -1 if edict is not a bot
}


/*		NEVER USED
bot_t *UTIL_GetBotPointer(edict_t *pEdict)
{
   int index;

   for (index=0; index < MAX_CLIENTS; index++)
   {
      if (bots[index].pEdict == pEdict)
      {
         break;
      }
   }

   if (index < MAX_CLIENTS)
      return (&bots[index]);

   return NULL;  // return NULL if edict is not a bot
}
*/


bool IsAlive(edict_t *pEdict)
{
	return ((pEdict->v.deadflag == DEAD_NO) && !(pEdict->v.effects & EF_NODRAW) &&
		(pEdict->v.health > 0) && !(pEdict->v.flags & FL_NOTARGET) &&
		(pEdict->v.solid == SOLID_SLIDEBOX));
}


bool FInViewCone(Vector *pOrigin, edict_t *pEdict)
{
	UTIL_MakeVectors ( pEdict->v.angles );
	
	Vector2D vec2LOS = (*pOrigin - pEdict->v.origin).Make2D();
	vec2LOS = vec2LOS.Normalize();

	const float flDot = DotProduct(vec2LOS, gpGlobals->v_forward.Make2D());
	
	if ( flDot > 0.50 )  // 60 degree field of view
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


// for more accurate checks
bool FInNarrowViewCone(Vector *pOrigin, edict_t *pEdict, float cone)
{
	UTIL_MakeVectors ( pEdict->v.angles );
	
	Vector2D vec2LOS = (*pOrigin - pEdict->v.origin).Make2D();
	vec2LOS = vec2LOS.Normalize();

	const float flDot = DotProduct(vec2LOS, gpGlobals->v_forward.Make2D());




	//@@@@@@@@@@@@@@@@@@@@@@@
	/*/
	#ifdef _DEBUG
	char m[256];
	sprintf(m, "(util.cpp) NarrowViewCone() - dot result %.2f\n", flDot);
	HudNotify(m);
	#endif
	/**/




	
	if ( flDot >= cone )  // by default it is something like 35 and less degree field of view
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


// ehm "overloaded" to ignore water restriction by Frank McNeil
bool FVisible(const Vector &vecOrigin, edict_t *pEdict, bool ignore_water)
{
	TraceResult tr;

	// look through caller's eyes
	const Vector vecLookerOrigin = pEdict->v.origin + pEdict->v.view_ofs;

	const int bInWater = (UTIL_PointContents (vecOrigin) == CONTENTS_WATER);
	const int bLookerInWater = (UTIL_PointContents (vecLookerOrigin) == CONTENTS_WATER);
	
	// don't look through water 
	if ((bInWater != bLookerInWater) && (ignore_water == FALSE))
		return FALSE;
	
	UTIL_TraceLine(vecLookerOrigin, vecOrigin, ignore_monsters, ignore_glass, pEdict, &tr);
	
	if (tr.flFraction != 1.0)
	{
		return FALSE;  // Line of sight is not established
	}
	else
	{
		return TRUE;  // line of sight is valid.
	}
}


bool FVisible( const Vector &vecOrigin, edict_t *pEdict )
{
	TraceResult tr;

	// look through caller's eyes
	const Vector vecLookerOrigin = pEdict->v.origin + pEdict->v.view_ofs;

	const int bInWater = (UTIL_PointContents (vecOrigin) == CONTENTS_WATER);
	const int bLookerInWater = (UTIL_PointContents (vecLookerOrigin) == CONTENTS_WATER);
	
	// don't look through water
	if (bInWater != bLookerInWater)
		return FALSE;
	
	UTIL_TraceLine(vecLookerOrigin, vecOrigin, ignore_monsters, ignore_glass, pEdict, &tr);



   //@@@@@@@@@@@@@
	/*/
#ifdef _DEBUG
	char str[126];
	sprintf(str, "(Original VisibleTest)Result=%.2f | entity=%s\n", tr.flFraction, STRING(tr.pHit->v.classname));
	HudNotify(str);
#endif
	/**/
	
	
	if (tr.flFraction != 1.0)
	{
		return FALSE;  // Line of sight is not established
	}
	else
	{
		return TRUE;  // line of sight is valid.
	}
}


/*
* tests direct visibility between the bot and the target
* includes tests for teammate, fence etc.
* returns TRUE if line of sight can be established
*/
int FPlayerVisible(const Vector &vecOrigin, const Vector &vecLookerOrigin, edict_t *pEdict)
{
	TraceResult tr;

	const int bInWater = (UTIL_PointContents(vecOrigin) == CONTENTS_WATER);
	const int bLookerInWater = (UTIL_PointContents(vecLookerOrigin) == CONTENTS_WATER);

	// don't look through water
	if (bInWater != bLookerInWater)
		return VIS_NO;

	// trace to see if there is a direct visibility at the enemy
	// we can't ignore monters, because if we did we would not see teammates and we would do team kills all the time
	UTIL_TraceLine(vecLookerOrigin, vecOrigin, dont_ignore_monsters, dont_ignore_glass, pEdict, &tr);


	//@@@@@@@@@@@@@
#ifdef _DEBUG
	char strs[128];
	sprintf(strs, "(PlayerVisible - FIRST test)Result=%.2f | entity=%s(name=%s)\n",
		tr.flFraction, STRING(tr.pHit->v.classname), STRING(tr.pHit->v.netname));
	//HudNotify(strs);
#endif

	// do we have clear view?
	// when we don't ignore monsters then this won't happen offen, because the player entity has a size
	// and if the bot and his enemy are face to face then enemy origin is "hidden behind" the player entity
	if (tr.flFraction == 1.0)
		return VIS_YES;

	// we don't have another entity in the way (i.e. it's a solid world that blocks the direct visibility)
	// therefore we don't need any other tests
	if (strcmp(STRING(tr.pHit->v.classname), "worldspawn") == 0)
		return VIS_NO;

	// first backup the entity that blocks the direct visibility for later use
	edict_t *obstacle = tr.pHit;

	// now we have to ignore the previous obstacle in order to see if the trace line can finally reach the target
	// i.e. virtually remove the obstacle and check direct visibility
	UTIL_TraceLine(vecLookerOrigin, vecOrigin, ignore_monsters, dont_ignore_glass, obstacle, &tr);


	//@@@@@@@@@@@@@
#ifdef _DEBUG
	char str[256];
	sprintf(str, "(PlayerVisible - 2nd test)Result=%.2f | entity=%s(name=%s)\n",
		tr.flFraction, STRING(tr.pHit->v.classname), STRING(tr.pHit->v.netname));
	//HudNotify(str);
#endif

	// if we can hit worldspawn now i.e. flFraction == 1.0 && pHit->v.classname is worldspawn
	// then we can establish a line of sight through the entity we ignored in the 2nd traceline
	// so now just decide if we can really see through the ignored entity and if we can open fire (i.e. don't do a TK)

	// the traceline reached the enemy without hitting anything else -> bot may eventually open fire
	if ((strcmp(STRING(tr.pHit->v.classname), "worldspawn") == 0) && (tr.flFraction == 1.0))
	{
		// there was a player entity so we need to check if it isn't our teammate
		// (i.e. teammate standing between the bot and the enemy)
		if (strcmp(STRING(obstacle->v.classname), "player") == 0)
		{
			const int pEdict_team = UTIL_GetTeam(pEdict);
			const int obstacle_team = UTIL_GetTeam(obstacle);

			// we would hit a teammate so don't shoot and risk a team kill
			if (pEdict_team == obstacle_team)
			{

				//@@@@@
#ifdef DEBUG
				//HudNotify("*** BREAKING MY TEAMMATE IS IN THE WAY!!!\n");
#endif // DEBUG

				return VIS_NO;
			}

			// it must be enemy so the bot can open fire
			return VIS_YES;
		}

		// there was a breakable entity, all we need to do is check if the
		// entity is done in a way players can see through
		// (ie. some transparency and not rendered as normal or solid)
		if ((strcmp(STRING(obstacle->v.classname), "func_breakable") == 0) &&
			((obstacle->v.rendermode != kRenderNormal) ||
			(obstacle->v.rendermode != kRenderTransAlpha)) && (obstacle->v.renderamt <= 150))	// NEEDS more tests
		{

			//@@@@@
#ifdef DEBUG
			//HudNotify("*** I see through func breakable!\n");
#endif // DEBUG


			return VIS_YES;
		}

		// there was an entity that is meant to be part of the world, but it's done so that you can see through it
		// you can't pass through it of course ... a fence for example
		if (strcmp(STRING(obstacle->v.classname), "func_wall") == 0)
		{

			//@@@@@
#ifdef DEBUG
			char str[256];
			sprintf(str, "(%s)*** I see func wall!\n", STRING(pEdict->v.netname));
			//HudNotify(str);
#endif // DEBUG

			// this is ugly solution, but the central alley causes real problems
			// as long as there isn't better way how to deal with it we have to use texture names
			// to figure out if it's a see through object or not
			if (strcmp(STRING(gpGlobals->mapname), "obj_armory") == 0)
			{
				const char* texture = g_engfuncs.pfnTraceTexture(obstacle, vecLookerOrigin, vecOrigin);

				if ((texture != nullptr) && ((strcmp(texture, "{nm_leaves3") == 0) || (strcmp(texture, "{artrellis") == 0)))
				{

#ifdef DEBUG
					//HudNotify("*** I see enemy! (func wall -> fence/bush on obj_armory)\n");
#endif // DEBUG


					return VIS_FENCE;
				}

				return VIS_NO;
			}

			// a transparent alpha texture case (ie. a fence, barbed wire etc.)
			if ((obstacle->v.rendermode == kRenderTransAlpha) && (obstacle->v.renderamt == 255) &&
				(obstacle->v.flags & FL_WORLDBRUSH))
			{
				//@@@@@
#ifdef DEBUG
				//HudNotify("*** I see enemy! (func wall -> fence/barbed wire)\n");
#endif // DEBUG
				return VIS_FENCE;
			}
			// a transparent texture case (ie. a window, glass wall etc.)
			else if ((obstacle->v.rendermode == kRenderTransTexture) && (obstacle->v.renderamt <= 150) &&
				(obstacle->v.flags & FL_WORLDBRUSH))
			{
				//@@@@@
#ifdef DEBUG
				//HudNotify("*** I see enemy! (func wall -> window/glass)\n");
#endif // DEBUG
				return VIS_YES;
			}
		}
	}

	// line of sight is not established
	// (the enemy must be behind something like a truck or tank and like models)
	// basically something that we cannot and should not see through
	return VIS_NO;
}

// same as above, but we use caller's eyes by default
int FPlayerVisible( const Vector &vecOrigin, edict_t *pEdict )
{
	TraceResult tr;

	// look through caller's eyes
	const Vector vecLookerOrigin = pEdict->v.origin + pEdict->v.view_ofs;

	return FPlayerVisible(vecOrigin, vecLookerOrigin, pEdict);
}


Vector GetGunPosition(edict_t *pEdict)
{
   return (pEdict->v.origin + pEdict->v.view_ofs);
}


Vector VecBModelOrigin(edict_t *pEdict)
{
   return pEdict->v.absmin + (pEdict->v.size * 0.5);
}


/*
* works very similar to FInViewCone(), but it returns exact angle value instead of TRUE or FALSE
*/
int InFieldOfView(edict_t *pEdict, const Vector &dest)
{
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
	int angle = abs(static_cast<int>(view_angle) - static_cast<int>(entity_angles.y));
	
	if (angle > 180)
		angle = 360 - angle;
	
	return angle;
	// rsm - END
}


/*
* checks the mod version to see if it's one of new versions ie. versions that include
* a lot of differences that are imcompatible with previous version
*/
bool UTIL_IsNewerVersion()
{
	if ((g_mod_version == FA_28) || (g_mod_version == FA_29) || (g_mod_version == FA_30))
		return TRUE;

	return FALSE;
}


/*
* checks the mod version to see if it's one of pre 2.6 versions ie. versions that don't
* support edict->v.team and gl attachments are fired right when you press secondary fire etc.
*/
bool UTIL_IsOldVersion()
{
	if ((g_mod_version == FA_25) || (g_mod_version == FA_24))
		return TRUE;

	return FALSE;
}


/*
* adds the bit to the bit map
*/
void UTIL_SetBit(int the_bit, int &bit_map)
{
	bit_map = bit_map | the_bit;
}


/*
* checks if the bit is in the map and if so then it removes it
*/
void UTIL_ClearBit(int the_bit, int &bit_map)
{
	if (bit_map & the_bit)
		bit_map = bit_map & ~the_bit;
}


/*
* returns if the bit is in the bit map or if it isn't
*/
bool UTIL_IsBitSet(int the_bit, int bit_map)
{
	if (bit_map & the_bit)
		return TRUE;

	return FALSE;
}


void UTIL_ShowMenu( edict_t *pEdict, int slots, int displaytime, bool needmore, char *pText )
{
	// don't run anything when in Firearms28, this menu was completely removed from game since FA28
	if (UTIL_IsNewerVersion())
	{
		const Vector color1 = Vector(250, 50, 0);
		const Vector color2 = Vector(255, 0, 20);

		CustHudMessage(pEdict, "Warning: In-game HUD menu was removed in FA2.8 and above\nThis command isn't available", color1, color2, 2, 5);
		ClientPrint(pEdict, HUD_PRINTNOTIFY, "In-game HUD menu was removed in FA2.8 and above. This command isn't available!\n");

		// reset also menu variable
		extern int g_menu_state;
		g_menu_state = 0;	// 0 == MENU_NONE

		return;
	}
	
	if (gmsgShowMenu == 0)
		gmsgShowMenu = REG_USER_MSG( "ShowMenu", -1 );

	pfnMessageBegin( MSG_ONE, gmsgShowMenu, nullptr, pEdict );
	
	pfnWriteShort( slots );
	pfnWriteChar( displaytime );
	pfnWriteByte( needmore );
	pfnWriteString( pText );

	pfnMessageEnd();
}


/*
* builds virtual path inside Half-Life
* separator == TRUE means that we need an additional directory seperator at the end of the path
* (eg. Half-Life\firearms\)
* separator == FALSE means no dir. separator will be added in the end of the path
* (eg. Half-Life\firearms)
*/
void UTIL_BuildFileName(char *filename, char *arg1, char *arg2, bool separator)
{
	if ((arg1) && (*arg1))
		strcpy(filename, arg1);

	if ((arg2) && (*arg2))
	{
#ifndef __linux__
		strcat(filename, "\\");
#else
		strcat(filename, "/");
#endif

		strcat(filename, arg2);
	}
	
	if (separator)
	{
#ifndef __linux__
		strcat(filename, "\\");
#else
		strcat(filename, "/");
#endif
	}


#ifdef _DEBUG
	extern int debug_engine;

	if (debug_engine)
	{
		FILE *fp;
		fp=fopen("!mb_engine_debug.txt","a");
		fprintf(fp, "\nVirtual path build by BuildFileName(): \"%s\"\n", filename);
		fclose(fp);
	}
#endif
}


/*
* builds virtual path inside "marine_bot" folder
*/
void UTIL_MarineBotFileName(char *filename, char *arg1, char *arg2)
{
	// build initial point first (ie. point to MB's folder)
	// we are using char variable that holds mod directory name to allow using MB
	// also in non-standard mod installations (eg. "Half-Life/FA" instead of
	// default "Half-Life/firearms")
	if (mod_id == FIREARMS_DLL)
		UTIL_BuildFileName(filename, mod_dir_name, "marine_bot", TRUE);
	else
	{
		filename[0] = 0;
		return;
	}

	// then add additional parameters such as subdirectory name etc.
	if ((arg1) && (*arg1) && (arg2) && (*arg2))
	{
		strcat(filename, arg1);

#ifndef __linux__
		strcat(filename, "\\");
#else
		strcat(filename, "/");
#endif

		strcat(filename, arg2);
	}
	else if ((arg1) && (*arg1))
	{
		strcat(filename, arg1);
	}
}


//=========================================================
// UTIL_LogPrintf - Prints a logged message to console.
// Preceded by LOG: ( timestamp ) < message >
//=========================================================
void UTIL_LogPrintf( char *fmt, ... )
{
   va_list        argptr;
   static char    string[1024];
   
   va_start ( argptr, fmt );
   vsprintf ( string, fmt, argptr );
   va_end   ( argptr );

   // Print to server console
   ALERT( at_logged, "%s", string );
}


/*
* works same as UTIL_LogPrintf only adds [MARINE_BOT] header before the logged message
*/
void UTIL_MBLogPrint(char *fmt, ...)
{
	va_list        argptr;
	static char    string[1024];
	
	va_start ( argptr, fmt );
	vsprintf ( string, fmt, argptr );
	va_end   ( argptr );
	
	// Print to server console with MB header
	ALERT( at_logged, "[MARINE_BOT] %s", string );
}


/*
//		NOT USED & WILL PROBABLY NEVER BE USED
// trace tree lines forward - standing position eyes, duck position, prone position
int ForwardTrace(const Vector &vecOrigin, edict_t *pEdict)
{
	TraceResult tr;
	Vector      vecLookerOrigin;
	Vector		fake_origin;
	bool		duck_trace_failed;

	fake_origin = pEdict->v.origin;
	duck_trace_failed = FALSE;

	vecLookerOrigin = pEdict->v.origin;

	UTIL_TraceLine(vecLookerOrigin, vecOrigin, ignore_monsters, ignore_glass, pEdict, &tr);

	if (tr.flFraction != 1.0)
		duck_trace_failed = TRUE;

	// look through caller's eyes
	vecLookerOrigin = pEdict->v.origin + pEdict->v.view_ofs;

	int bInWater = (UTIL_PointContents (vecOrigin) == CONTENTS_WATER);
	int bLookerInWater = (UTIL_PointContents (vecLookerOrigin) == CONTENTS_WATER);

	// don't look through water
	if (bInWater != bLookerInWater)
		return -1;

	UTIL_TraceLine(vecLookerOrigin, vecOrigin, ignore_monsters, ignore_glass, pEdict, &tr);

	if (tr.flFraction != 1.0)
	{
		return 0;  // Line of sight is not established
	}
	else
	{
		if (duck_trace_failed == FALSE)
			return 2;	// line from eyes & duck is valid
		else
			return 1;	// only line from eyes is valid.
	}
}
*/


/*
* checks if there is a free space (for a body) on specified side of the object
* passed in by an origin
*/
bool UTIL_TraceObjectsSides(edict_t *pEdict, int side, const Vector &obj_origin)
{
	//@@@@@@@@@@@@@@@@@@@@@@@@@@
	// NO CODE YET


	return FALSE;
}


/*
* bot checks if the sound is made by his teammate if not then turn to that direction
*/
bool bot_t::UpdateSounds(edict_t *pPlayer)
{
	float distance;
	static bool check_footstep_sounds = TRUE;
	static float footstep_sounds_on;
	float sensitivity[BOT_SKILL_LEVELS] = {2.0, 1.8, 1.6, 1.4, 1.0}; // based on bot skill level
	Vector v_sound;

	// ignore sounds when on ladder (just for sure)
	if (pEdict->v.movetype == MOVETYPE_FLY)
		return FALSE;

	// update sounds made by this player, alert bots if they are nearby
	if (check_footstep_sounds)
	{
		check_footstep_sounds = FALSE;
		footstep_sounds_on = CVAR_GET_FLOAT("mp_footsteps");
	}

	if (footstep_sounds_on > 0.0)
	{
		// check if this player is moving fast enough to make sounds
		if (pPlayer->v.velocity.Length2D() > 220.0)
		{
			const float volume = 500.0;  // volume of sound being made (just pick something)

			const int bot_team = UTIL_GetTeam(pEdict);

			const int foe_team = UTIL_GetTeam(pPlayer);
			
			// is possible enemy really enemy (ie not in same team)
			if ((foe_team != TEAM_NONE) && (bot_team != foe_team))
			{
				v_sound = pPlayer->v.origin - pEdict->v.origin;

				distance = v_sound.Length();
			}
			// otherwise it's your teammate so set max distance which will prevent facing him
			else
			{
				distance = 9999.0;
			}

			// is the bot close enough to hear this sound?
			if (distance < (volume * sensitivity[bot_skill]))
			{
				// limit facing the source of the noice while the bot is moving
				// we don't want the bot to turn back and/or sides while he's advancing forward
				if (pEdict->v.velocity.Length2D() > 50.0)
				{
					const bool HeadVisible = FVisible(pPlayer->v.origin + pPlayer->v.view_ofs, pEdict);
					const bool BodyVisible = FVisible(pPlayer->v.origin, pEdict);

					if (!HeadVisible && !BodyVisible)
					{
						//@@@@@@@@@@@@@@@@
						//#ifdef _DEBUG
						//ALERT(at_console, "%s can hear enemy, but can't see it\n", name);
						//#endif

						return FALSE;
					}

				}

				// if bot is using bipod AND the sound is NOT in direction bot is looking
				if (IsTask(TASK_BIPOD) && (FInViewCone(&v_sound, pEdict) == FALSE))
					return FALSE;

				const Vector bot_angles = UTIL_VecToAngles( v_sound );

				pEdict->v.ideal_yaw = bot_angles.y;

				BotFixIdealYaw(pEdict);

				return TRUE;
			}
		}
	}

	return FALSE;
}


/*
* use radio command specified in "word"
*/
void UTIL_Radio(edict_t *pEdict, char *word)
{
	if ((pEdict == nullptr) || (word == nullptr))
		return;

	FakeClientCommand(pEdict, "radio", nullptr, nullptr);	// activate the voice/radio gui
	FakeClientCommand(pEdict, "radio", word, nullptr);	// the message
}


/*
* use voice command specified in "word"
*/
void UTIL_Voice(edict_t *pEdict, char *word)
{
	if ((pEdict == nullptr) || (word == nullptr))
		return;

	FakeClientCommand(pEdict, "radio", nullptr, nullptr);	// activate the voice/radio gui -- It's probably useless, works well also without it
	FakeClientCommand(pEdict, "voice", word, nullptr);	// the message
}


/*
* use say command to send the message
*/
void UTIL_Say(edict_t *pEdict, char *message)
{
	if ((pEdict != nullptr) && (message != nullptr))
		FakeClientCommand(pEdict, "say", message, nullptr);
}


/*
* use team say command to send the message
*/
void UTIL_TeamSay(edict_t *pEdict, char *message)
{
	if ((pEdict != nullptr) && (message != nullptr))
		FakeClientCommand(pEdict, "say_team", message, nullptr);
}


/*
* checks if the weapon is one of assault weapons
*/
bool UTIL_IsAssaultRifle(int weapon)
{
	if ((weapon == fa_weapon_ak47) || (weapon == fa_weapon_famas) ||
		(weapon == fa_weapon_g3a3) || (weapon == fa_weapon_g36e) ||
		(weapon == fa_weapon_m16) || (weapon == fa_weapon_ak74) ||
		(weapon == fa_weapon_m14) || (weapon == fa_weapon_m4) ||
		(weapon == fa_weapon_aug))
		return TRUE;
	
	return FALSE;
}


/*
* checks if the weapon is one of sniper rifles
*/
bool UTIL_IsSniperRifle(int weapon)
{
	if ((weapon == fa_weapon_ssg3000) || (weapon == fa_weapon_m82) ||
		(weapon == fa_weapon_svd) || (weapon == fa_weapon_psg1))
		return TRUE;
	
	return FALSE;
}

/*
* checks if the weapon is one of machineguns
*/
bool UTIL_IsMachinegun(int weapon)
{
	if ((weapon == fa_weapon_m60) || (weapon == fa_weapon_m249) || (weapon == fa_weapon_pkm))
		return TRUE;
	
	return FALSE;
}


/*
* checks if the weapon is one of submachineguns
*/
bool UTIL_IsSMG(int weapon)
{
	if ((weapon == fa_weapon_sterling) || (weapon == fa_weapon_mp5a5) ||
		(weapon == fa_weapon_bizon) || (weapon == fa_weapon_uzi) ||
		(weapon == fa_weapon_mc51) || (weapon == fa_weapon_m11) ||
		(weapon == fa_weapon_mp5k))
		return TRUE;
	
	return FALSE;
}


/*
* checks if the weapon is one of shotguns
*/
bool UTIL_IsShotgun(int weapon)
{
	if ((weapon == fa_weapon_benelli) || (weapon == fa_weapon_saiga))
		return TRUE;
	
	return FALSE;
}


/*
* checks if the weapon is one of handguns/pistols
*/
bool UTIL_IsHandgun(int weapon)
{
	if ((weapon == fa_weapon_coltgov) || (weapon == fa_weapon_anaconda) ||
		(weapon == fa_weapon_ber93r) || (weapon == fa_weapon_ber92f) ||
		(weapon == fa_weapon_desert))
		return TRUE;
	
	return FALSE;
}


/*
* checks if the weapon is a grenade
*/
bool UTIL_IsGrenade(int weapon)
{
	if ((weapon == fa_weapon_concussion) || (weapon == fa_weapon_frag) ||
		(weapon == fa_weapon_flashbang) || (weapon == fa_weapon_stg24))
		return TRUE;

	return FALSE;
}


/*
* checks if the weapon has bipod
*/
bool IsBipodWeapon(int weapon)
{
	if ((weapon == fa_weapon_m82) || (weapon == fa_weapon_m60) ||
		(weapon == fa_weapon_m249) || (weapon == fa_weapon_pkm))
		return TRUE;

	return FALSE;
}


/*
* do select main weapon
*/
inline void SelectMainWeapon(bot_t *pBot)
{
	extern bot_weapon_t weapon_defs[MAX_WEAPONS];
	char this_one[64];
	//float extra_delay = 0.0;
	float extra_delay = 1.7f;	// default value that works with most weapons in FA 3.0											NEW CODE 094

	strcpy(this_one, "weapon_knife");

	// we need to access weapon data in order to...
	bot_weapon_t bot_weapon = weapon_defs[pBot->main_weapon];

	// get weapon name for the client command
	strcpy(this_one, bot_weapon.szClassname);

	if (strcmp(this_one, "weapon_knife") == 0)
	{
		pBot->UseWeapon(USE_KNIFE);
		extra_delay = 1.0f;

#ifdef _DEBUG
		UTIL_DebugDev("Utils|SelectMainWeapon()|no weapon found -- switched to knife\n",
			-100, -100);
#endif
	}


	//@@@@@@@@@@@@@@@
	#ifdef _DEBUG
	//char swmsg[128];
	//sprintf(swmsg, "SelectMainWeapon() - Changing to weapon %s\n", this_one);
	//HudNotify(swmsg, pBot);
	#endif



	// calling the weapon name will select it
	FakeClientCommand(pBot->pEdict, this_one, nullptr, nullptr);
	
	// we must update shoot time to allow the engine to process it
	// and prevent the bot from trying to use the weapon, because it isn't ready yet
	pBot->f_shoot_time = gpGlobals->time + extra_delay;
}


/*
* do select backup weapon and specify extra delay that some weapons needs to properly work
*/
inline void SelectBackupWeapon(bot_t *pBot)
{
	extern bot_weapon_t weapon_defs[MAX_WEAPONS];
	char this_one[64];
	float extra_delay = 1.7f;	// (was 0.0)

	strcpy(this_one, "weapon_knife");

	bot_weapon_t bot_weapon = weapon_defs[pBot->backup_weapon];

	strcpy(this_one, bot_weapon.szClassname);

	//																							NEW CODE 094
	// handguns have a little faster selection
	if (UTIL_IsHandgun(pBot->backup_weapon))
		extra_delay = 1.4;

	if (strcmp(this_one, "weapon_knife") == 0)
	{
		pBot->UseWeapon(USE_KNIFE);
		extra_delay = 1.0f;

#ifdef _DEBUG
		UTIL_DebugDev("Utils|SelectBackupWeapon()|no weapon found -- switched to knife\n",
			-100, -100);
#endif
	}




	//@@@@@@@@@@@@@@@
	#ifdef _DEBUG
	//char swmsg[128];
	//sprintf(swmsg, "SelectBackupWeapon() - Changing to weapon %s\n", this_one);
	//HudNotify(swmsg, pBot);
	#endif



	FakeClientCommand(pBot->pEdict, this_one, nullptr, nullptr);
	pBot->f_shoot_time = gpGlobals->time + extra_delay;
}


/*
* do select grenade and set some time to properly take it
*/
inline void SelectGrenade(bot_t *pBot)
{
	extern bot_weapon_t weapon_defs[MAX_WEAPONS];
	char this_one[64];
	float changing_delay = 1.2;

	strcpy(this_one, "weapon_knife");

	bot_weapon_t bot_weapon = weapon_defs[pBot->grenade_slot];

	strcpy(this_one, bot_weapon.szClassname);

	// these weapons need an extra delay
	if ((pBot->current_weapon.iId == fa_weapon_m79) ||
		(pBot->current_weapon.iId == fa_weapon_m16) ||
		(pBot->current_weapon.iId == fa_weapon_svd) ||
		(pBot->current_weapon.iId == fa_weapon_sterling))
		changing_delay += 0.3;
	else if (pBot->current_weapon.iId == fa_weapon_ak74)
		changing_delay += 0.6;
	else if (pBot->current_weapon.iId == fa_weapon_ak47)
		changing_delay += 1.0;

	if (strcmp(this_one, "weapon_knife") == 0)
	{
		pBot->UseWeapon(USE_KNIFE);

#ifdef _DEBUG
		UTIL_DebugDev("Utils|SelectGrenade()|no grenade found -- switched to knife\n",
			-100, -100);
#endif
	}



	//@@@@@@@@@@@@@@@
	#ifdef _DEBUG
	//char swmsg[128];
	//sprintf(swmsg, "SelectGrenade() - Changing to weapon %s\n", this_one);
	//HudNotify(swmsg, pBot);
	#endif



	FakeClientCommand(pBot->pEdict, this_one, nullptr, nullptr);
	pBot->f_shoot_time = gpGlobals->time + changing_delay;
}


/*
* does all necessary things to change between any two weapons
*/
void UTIL_ChangeWeapon(bot_t *pBot)
{
	//																		NEW CODE 094
	// has the weapon change been invalidated?
	if (pBot->IsWeaponStatus(WS_INVALID))
	{
		// then select knife, because everyone has it by default...
		pBot->UseWeapon(USE_KNIFE);

		// clear the "resetting" bit...
		pBot->RemoveWeaponStatus(WS_INVALID);

		// and set weapon action back to ready to start anew in next frame
		pBot->weapon_action = W_READY;

		if (botdebugger.IsDebugActions())
			HudNotify("ChangeWeapon() -> weapon change has been invalidated -> resetting the action!\n", pBot);

		return;
	}
	// *******************************************************************	NEW CODE 094 END

	// do we need to change weapon? Then wait until going to prone or resume from prone is finished
	if ((pBot->weapon_action == W_TAKEOTHER) && !pBot->IsGoingProne())
	{
		// select the correct weapon
		if (pBot->forced_usage == USE_MAIN)
		{
			SelectMainWeapon(pBot);
		}
		else if (pBot->forced_usage == USE_BACKUP)
		{
			SelectBackupWeapon(pBot);
		}
		else if (pBot->forced_usage == USE_GRENADE)
		{
			SelectGrenade(pBot);
		}
		else if (pBot->forced_usage == USE_KNIFE)
		{
			FakeClientCommand(pBot->pEdict, "weapon_knife", nullptr, nullptr);
			pBot->f_shoot_time = gpGlobals->time + 1.0f;
		}

		// we have to clear this bit if we are changing weapons in FA25 and below
		if (UTIL_IsOldVersion())
			pBot->RemoveWeaponStatus(WS_USEAKIMBO);

		// set appropriate weapon flag
		pBot->weapon_action = W_INCHANGE;

		// reset grenade action if we have any grenade left
		if (pBot->grenade_action != ALTW_USED)
			pBot->grenade_action = ALTW_NOTUSED;

		return;
	}
	
	// are we still changing the weapon
	if (pBot->weapon_action == W_INCHANGE)
	{
		// stop using bipod
		if (pBot->IsTask(TASK_BIPOD))
		{
			// it didn't react on the first weapon change, because with bipod down the weapon cannot be changed
			// so return back to initial flag
			pBot->weapon_action = W_READY;

			BotUseBipod(pBot, TRUE, "UtilChangeWeapon()");

			if (botdebugger.IsDebugActions())
				HudNotify("ChangeWeapon() -> removing BIPOD that is preventing a weapon change\n", pBot);

			return;
		}

		// wait for the engine to finish the animations
		if (pBot->f_shoot_time < gpGlobals->time)
		{
			// has the bot changed to desired weapon yet
			if ((pBot->forced_usage == USE_MAIN) && (pBot->current_weapon.iId == pBot->main_weapon))
			{
				// set the right weapon flag
				pBot->weapon_action = W_INHANDS;
				
				// update shoot time to prevent the bot to fire from the weapon
				pBot->f_shoot_time = gpGlobals->time;

				// bot should also check if this weapons isn't almost empty after the switch
				pBot->SetSubTask(ST_W_CLIP);

				if (botdebugger.IsDebugActions())
					HudNotify("ChangeWeapon() -> changed to MAIN weapon\n", pBot);

				return;
			}

			else if ((pBot->forced_usage == USE_BACKUP) && (pBot->current_weapon.iId == pBot->backup_weapon))
			{
				pBot->weapon_action = W_INHANDS;
				pBot->f_shoot_time = gpGlobals->time;
				pBot->SetSubTask(ST_W_CLIP);

				if (botdebugger.IsDebugActions())
					HudNotify("ChangeWeapon() -> changed to BACKUP weapon\n", pBot);

				return;
			}

			else if ((pBot->forced_usage == USE_GRENADE) && (pBot->current_weapon.iId == pBot->grenade_slot))
			{
				pBot->weapon_action = W_INHANDS;
				pBot->f_shoot_time = gpGlobals->time;

				if (botdebugger.IsDebugActions())
					HudNotify("ChangeWeapon() -> changed to GRENADE\n", pBot);

				return;
			}

			else if ((pBot->forced_usage == USE_KNIFE) && (pBot->current_weapon.iId == fa_weapon_knife))
			{
				pBot->weapon_action = W_INHANDS;
				pBot->f_shoot_time = gpGlobals->time;

				if (botdebugger.IsDebugActions())
					HudNotify("ChangeWeapon() -> changed to KNIFE\n", pBot);

				return;
			}
			else
				// keep increasing the shoot time to prevent the bot in trying to use the weapon
				pBot->f_shoot_time = gpGlobals->time + 0.1f;
		}
		
		if (botdebugger.IsDebugActions())
		{
#ifdef DEBUG
			extern edict_t* g_debug_bot;
			extern bool g_debug_bot_on;

			if (g_debug_bot_on && (g_debug_bot == pBot->pEdict))
				ALERT(at_console, "Trying to switch weapons (currT is %.2f)\n", gpGlobals->time);
#else
			ALERT(at_console, "Trying to switch weapons (currT is %.2f)\n", gpGlobals->time);
#endif // DEBUG
		}
	}

	// weapon change is finished and we can set the weapon action to ready again
	if (pBot->weapon_action == W_INHANDS)
	{
		if (pBot->f_shoot_time + 0.2f < gpGlobals->time)
		{
			pBot->weapon_action = W_READY;

			if (botdebugger.IsDebugActions())
				HudNotify("ChangeWeapon() -> WEAPON is READY now!\n", pBot);
		}
	}
}


/*
* these weapons do NOT allow changing the fire mode (ie have only one mode)
*/
bool IgnoreFireMode(int weapon)
{
	if ((weapon == fa_weapon_knife) || (weapon == fa_weapon_coltgov) ||
		(weapon == fa_weapon_ber92f) || (weapon == fa_weapon_anaconda) ||
		(weapon == fa_weapon_desert) || UTIL_IsShotgun(weapon) || UTIL_IsSniperRifle(weapon) ||
		UTIL_IsMachinegun(weapon) || (weapon == fa_weapon_m79) || (weapon == fa_weapon_frag) ||
		(weapon == fa_weapon_concussion) || (weapon == fa_weapon_flashbang) ||
		(weapon == fa_weapon_stg24) || (weapon == fa_weapon_claymore) ||
		(weapon == fa_weapon_m14))
	{
		return TRUE;
	}

	return FALSE;
}


/*
* these weapons allow us to change the fire mode to 3rburst
*/
bool BurstFireMode(int weapon)
{
	if ((weapon == fa_weapon_ber93r) || (weapon == fa_weapon_mp5a5) ||
		(((UTIL_IsOldVersion() == FALSE) && (weapon == fa_weapon_famas))) ||
		(weapon == fa_weapon_g36e) || (weapon == fa_weapon_m16) ||
		(weapon == fa_weapon_mc51) || (weapon == fa_weapon_mp5k))
	{
		return TRUE;	
	}

	return FALSE;
}


/*
* these weapons allow us to change the fire mode to full auto
*/
bool FullAutoFireMode(int weapon)
{
	if ((weapon == fa_weapon_mp5a5) || (weapon == fa_weapon_uzi) || (weapon == fa_weapon_ak47) ||
		(weapon == fa_weapon_famas) || (weapon == fa_weapon_g3a3) || (weapon == fa_weapon_g36e) ||
		(weapon == fa_weapon_ak74) || (weapon == fa_weapon_m4) || (weapon == fa_weapon_bizon) ||
		(weapon == fa_weapon_sterling) || (weapon == fa_weapon_mc51) ||
		(weapon == fa_weapon_m11) || (weapon == fa_weapon_aug) || (weapon == fa_weapon_mp5k) ||
		(UTIL_IsOldVersion() && (weapon == fa_weapon_m16)))
	{
		return TRUE;
	}

	return FALSE;
}


/*
* checks weapon if we can use specified (in new_mode) fire mode
* if so it changes weapon fire mode to that mode
* wtest value can bypass the weapon testing (don't test weapons) and immediately change
* the fire mode (this should save some CPU time in cases we already know which weapon bot has)
*/
void UTIL_ChangeFireMode(bot_t *pBot, int new_mode, FireMode_WTest wtest)
{
	// don't change fire mode if attached gl is used (based on FA version)
	if ((pBot->secondary_active) &&
		((pBot->current_weapon.iId == fa_weapon_m16) || (pBot->current_weapon.iId == fa_weapon_ak74)))
		return;

	// does current weapon supports full auto mode
	if ((wtest == FireMode_WTest::test_weapons) && (new_mode == FM_AUTO))
	{
		if (FullAutoFireMode(pBot->current_weapon.iId))
		{
			// don't test from now we already know that bot has the right weapon
			wtest = FireMode_WTest::dont_test_weapons;
		}
		// bot doesn't carry weapon with full auto fire mode so end it
		else
			return;
	}

	// does current weapon supports burst mode
	if ((wtest == FireMode_WTest::test_weapons) && (new_mode == FM_BURST))
	{
		if (BurstFireMode(pBot->current_weapon.iId))
		{			
			// don't test from now we already know that bot has the right weapon
			wtest = FireMode_WTest::dont_test_weapons;
		}
		// bot doesn't carry weapon with burst fire so end it
		else
			return;
	}

	// don't try to change firemode on those weapons
	if ((wtest == FireMode_WTest::test_weapons) && (IgnoreFireMode(pBot->current_weapon.iId)))
	{
		return;
	}

	// if current fire mode is NOT the one we want AND
	// this weapon has other fire modes so change it
	if ((pBot->current_weapon.iFireMode != new_mode) &&
		(pBot->current_weapon.iFireMode != FM_NONE))
	{
		FakeClientCommand(pBot->pEdict, "firemode", nullptr, nullptr);

		/*/
#ifdef _DEBUG
		char mode[16];
		if (new_mode == FM_SEMI)
			strcpy(mode, "SEMI");
		else if (new_mode == FM_BURST)
			strcpy(mode, "BURST");
		else if (new_mode == FM_AUTO)
			strcpy(mode, "FULL AUTO");

		ALERT(at_console, "Weapon FireMode changed to %s (weapon ID %d)\n", mode,
			pBot->current_weapon.iId);
#endif
		/**/
	}
}


/*
* do print weapon name based on its ID at console
*/
void UTIL_ShowWeapon(bot_t *pBot)
{
	extern edict_t *pRecipient;
	extern bot_weapon_t weapon_defs[MAX_WEAPONS];
	bot_weapon_t bot_weapon;
	char prim[64], backup[64], grenade[64], claymore[64], msg[256];
	char *use;

	strcpy(prim, "n/a");
	strcpy(backup, "n/a");
	strcpy(grenade, "n/a");
	strcpy(claymore, "n/a");

	if (pBot->main_weapon != -1)
	{
		bot_weapon = weapon_defs[pBot->main_weapon];
		strcpy(prim, bot_weapon.szClassname);
	}
	
	if (pBot->backup_weapon != -1)
	{
		bot_weapon = weapon_defs[pBot->backup_weapon];
		strcpy(backup, bot_weapon.szClassname);
	}

	if (pBot->grenade_slot != -1)
	{
		bot_weapon = weapon_defs[pBot->grenade_slot];
		strcpy(grenade, bot_weapon.szClassname);
	}

	if (pBot->claymore_slot != -1)
	{
		bot_weapon = weapon_defs[pBot->claymore_slot];
		strcpy(claymore, bot_weapon.szClassname);
	}
	

	if (pBot->forced_usage == USE_MAIN) use = "primary";
	else if (pBot->forced_usage == USE_BACKUP) use = "backup";
	else if (pBot->forced_usage == USE_KNIFE) use = "knife";
	else if (pBot->forced_usage == USE_GRENADE) use = "grenade";

	sprintf(msg, "prim %s backup %s grenade %s claymore %s (using %s)\n",
		prim, backup, grenade, claymore, use);
	ClientPrint(pRecipient, HUD_PRINTNOTIFY, msg);
}


/*
* checks whether the bot should reload his current weapon (based on iClip value and the type of the weapon)
*/
bool UTIL_ShouldReload(bot_t *pBot, const char* loc)
{
	const bot_current_weapon_t w = pBot->current_weapon;

	// don't check things if the weapon isn't ready
	if (pBot->weapon_action != W_READY)
		return FALSE;

	// always reload if the weapon is empty and have ammo for it
	if ((w.iClip == 0) && (w.iAmmo1 > 0))
		return TRUE;

	//  keep benelli/remington always fully loaded
	if ((w.iClip < 8) && (w.iAmmo1 > 0) && (w.iId == fa_weapon_benelli))
		return TRUE;

	// if NOT tasked to check weapon clip for remaining ammo then there is no need to reload weapon either
	// (i.e. we don't want to check ammo every frame)
	if (pBot->IsSubTask(ST_W_CLIP) == FALSE)
		return FALSE;

	// if we have at least two magazines then ...
	if (w.iAmmo1 > 1)
	{


#ifdef DEBUG

		//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@	 													// NEW CODE 094 (remove it)
		if (loc != NULL)
		{
			char dm[256];
			sprintf(dm, "ShouldReload() called @ (%s)-> going to check for almost empty clip (=%d)\n", loc, w.iClip);
			//HudNotify(dm, pBot);
		}
		//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#endif // DEBUG



		// see if the magazine is almost empty and ...
		if ((w.iClip < 3) && ((w.iId == fa_weapon_coltgov) || (w.iId == fa_weapon_desert) ||
			(w.iId == fa_weapon_ssg3000) || (w.iId == fa_weapon_m82)))
		{
			// let the bot know about partly used magazines ...
			pBot->SetWeaponStatus(WS_NOTEMPTYMAG);

			// before making him reload the weapon
			return TRUE;
		}

		// anaconda doesn't allow merging magazines so we can't set partly used magazine flag here
		// but we'll allow reloading it if it is almost empty
		if ((w.iClip < 3) && (w.iId == fa_weapon_anaconda))
		{
			return TRUE;
		}

		if ((w.iClip < 5) && ((w.iId == fa_weapon_ber92f) || (w.iId == fa_weapon_ber93r) || (w.iId == fa_weapon_svd) ||
			(w.iId == fa_weapon_saiga)))
		{
			pBot->SetWeaponStatus(WS_NOTEMPTYMAG);

			return TRUE;
		}

		if ((w.iClip < 10) && UTIL_IsAssaultRifle(w.iId))
		{
			pBot->SetWeaponStatus(WS_NOTEMPTYMAG);

			return TRUE;
		}

		if ((w.iClip < 15) && UTIL_IsSMG(w.iId))
		{
			pBot->SetWeaponStatus(WS_NOTEMPTYMAG);

			return TRUE;
		}
	}

	// ammo left in current weapon isn't at critical level so we don't have to check it again
	pBot->RemoveSubTask(ST_W_CLIP);

	// and we also won't reload
	return FALSE;
}


/*
* checks amount of magazines the bot should take from ammobox
*/
void UTIL_CheckAmmoReserves(bot_t *pBot, const char* loc)
{
	const int main_weap_id = pBot->main_weapon;
	const int backup_weap_id = pBot->backup_weapon;
	extern bot_weapon_t weapon_defs[MAX_WEAPONS];

	// don't check ammo when the weapon is NOT ready OR
	// the bot fired the weapon recently (or changed weapons where it's set as well) OR
	// is paused right now where the bot could be merging magazines
	if ((pBot->weapon_action != W_READY) ||
		(pBot->f_shoot_time + 1.0f > gpGlobals->time) ||
		(pBot->f_pause_time >= gpGlobals->time))
		return;


#ifdef DEBUG
	//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	if (loc != NULL)
	{
		char dbgmsg[256];
		sprintf(dbgmsg, "CheckAmmoReserves() called @ %s\n", loc);
		HudNotify(dbgmsg, pBot);
	}
#endif // DEBUG


	// is main weapon completely empty AND have main weapon
	if ((pBot->main_no_ammo) && (pBot->main_weapon != NO_VAL))
		pBot->take_main_mags = weapon_defs[main_weap_id].iAmmo1Max;
	// otherwise count how many mags have
	else if (pBot->main_weapon != NO_VAL)
		pBot->take_main_mags = weapon_defs[main_weap_id].iAmmo1Max - pBot->curr_rgAmmo[weapon_defs[main_weap_id].iAmmo1];
	else
		pBot->take_main_mags = 0;

	//																			NEW CODE 094
	// check for special cases and handle them
	if (pBot->take_main_mags > 0)
	{
		if (main_weap_id == fa_weapon_benelli)
			pBot->take_main_mags /= 8;			// gets 8 slugs at once

		if (main_weap_id == fa_weapon_m11)
			pBot->take_main_mags /= 32;			// gets 32 rounds at once
	}

	// is backup weapon completely empty AND have backup weapon
	if ((pBot->backup_no_ammo) && (pBot->backup_weapon != NO_VAL))
		pBot->take_backup_mags = weapon_defs[backup_weap_id].iAmmo1Max;
	// otherwise count how many mags have
	else if (pBot->backup_weapon != NO_VAL)
		pBot->take_backup_mags = weapon_defs[backup_weap_id].iAmmo1Max -
			pBot->curr_rgAmmo[weapon_defs[backup_weap_id].iAmmo1];
	else
		pBot->take_backup_mags = 0;

	//																			NEW CODE 094
	if (pBot->take_backup_mags > 0)
	{
		if (backup_weap_id == fa_weapon_benelli)
			pBot->take_backup_mags /= 8;

		if (backup_weap_id == fa_weapon_anaconda)
			pBot->take_backup_mags /= 6;		// gets 6 rounds at once

		if (backup_weap_id == fa_weapon_m11)
			pBot->take_backup_mags /= 32;
	}

	// update this time
	//if (pBot->IsTask(TASK_CHECKAMMO) && (pBot->check_ammunition_time < gpGlobals->time))	// NEW CODE 094 (prev code)
	pBot->check_ammunition_time = gpGlobals->time;

	// and reset the task, because bot already checked ammunition
	pBot->RemoveTask(TASK_CHECKAMMO);

	// bot should also check current weapon magazine for remaining ammo
	pBot->SetSubTask(ST_W_CLIP);

	if (botdebugger.IsDebugActions())
		HudNotify("CheckAmmoReserves() -> ammunition was checked ...\n", pBot);

	// has the bot any reserve ammo for main weapon AND main weapon no ammo flag is set
	// (ie we think that main weapon is completely empty, but we have some ammo for it)
	if ((main_weap_id != NO_VAL) && (pBot->curr_rgAmmo[weapon_defs[main_weap_id].iAmmo1] > 0) && (pBot->main_no_ammo))
	{
		pBot->main_no_ammo = FALSE;		// main weapon isn't completely empty now

		if (botdebugger.IsDebugActions())
			HudNotify("CheckAmmoReserves() -> main weapon is usable again\n", pBot);
	}

	// has the bot any reserve ammo for backup weapon AND backup weapon no ammo flag is set
	// (ie we think that backup weapon is completely empty, but we have some ammo for it)
	if ((backup_weap_id != NO_VAL) && (pBot->curr_rgAmmo[weapon_defs[backup_weap_id].iAmmo1] > 0) && (pBot->backup_no_ammo))
	{
		pBot->backup_no_ammo = FALSE;		// backup weapon isn't completely empty now

		if (botdebugger.IsDebugActions())
			HudNotify("CheckAmmoReserves() -> backup weapon is usable again\n", pBot);
	}

	// has the bot some unused grenades yet AND the grenade flag is already set to used
	// (ie we thing that we don't have any grenade, but there is still at least one available)
	if ((pBot->grenade_action == ALTW_USED) && (pBot->grenade_slot != NO_VAL) &&
		(pBot->curr_rgAmmo[weapon_defs[pBot->grenade_slot].iAmmo1] > 0))
	{
		pBot->grenade_action = ALTW_NOTUSED;	// grenades aren't completely used yet

		if (botdebugger.IsDebugActions())
			HudNotify("CheckAmmoReserves() -> grenades are usable again\n", pBot);
	}

	// we must handle situation when the bot spawned more than one grenade type
	// (i.e. some custom classes give the bot both grenades yet the system is done so the bot can handle just one)
	// so look for the moment when the bot depleted his first grenade type
	if ((pBot->grenade_action == ALTW_USED) && !pBot->IsSubTask(ST_NOOTHERNADE))
	{
		int grenade_count = 0;


#ifdef DEBUG
		//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		//HudNotify("checking for other grenades\n");
#endif // DEBUG

		if (botdebugger.IsDebugActions())
			HudNotify("CheckAmmoReserves() -> checking for other grenades ...\n", pBot);

		// now check if he has also the other grenade
		if (pBot->bot_weapons & (1 << fa_weapon_concussion))
		{
			grenade_count++;

			// NOT already used?
			if (pBot->curr_rgAmmo[weapon_defs[fa_weapon_concussion].iAmmo1] > 0)
			{
				// then make it useable
				pBot->grenade_slot = fa_weapon_concussion;
				pBot->grenade_action = ALTW_NOTUSED;

				if (botdebugger.IsDebugActions())
					HudNotify("CheckAmmoReserves() -> concussion grenade has been set as usable\n", pBot);



#ifdef DEBUG
				//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
				char dbgmsg[256];
				sprintf(dbgmsg, "CheckAmmoReserves() -> concussion grenade has been set as usable (class=%d)\n", pBot->bot_class);
				HudNotify(dbgmsg, pBot);
#endif // DEBUG


			}
		}
		
		if (pBot->bot_weapons & (1 << fa_weapon_frag))
		{
			grenade_count++;

			if (pBot->curr_rgAmmo[weapon_defs[fa_weapon_frag].iAmmo1] > 0)
			{
				pBot->grenade_slot = fa_weapon_frag;
				pBot->grenade_action = ALTW_NOTUSED;

				if (botdebugger.IsDebugActions())
					HudNotify("CheckAmmoReserves() -> fragmentation grenade has been set as usable\n", pBot);




#ifdef DEBUG
				//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
				char dbgmsg[256];
				sprintf(dbgmsg, "CheckAmmoReserves() -> frag grenade has been set as usable (class=%d)\n", pBot->bot_class);
				HudNotify(dbgmsg, pBot);
#endif // DEBUG




			}
		}

		if (pBot->bot_weapons & (1 << fa_weapon_flashbang))
		{
			grenade_count++;

			if (pBot->curr_rgAmmo[weapon_defs[fa_weapon_flashbang].iAmmo1] > 0)
			{
				pBot->grenade_slot = fa_weapon_flashbang;
				pBot->grenade_action = ALTW_NOTUSED;

				if (botdebugger.IsDebugActions())
					HudNotify("CheckAmmoReserves() -> flashbang has been set as usable\n", pBot);
			}
		}

		if (pBot->bot_weapons & (1 << fa_weapon_stg24))
		{
			grenade_count++;

			if (pBot->curr_rgAmmo[weapon_defs[fa_weapon_stg24].iAmmo1] > 0)
			{
				pBot->grenade_slot = fa_weapon_stg24;
				pBot->grenade_action = ALTW_NOTUSED;

				if (botdebugger.IsDebugActions())
					HudNotify("CheckAmmoReserves() -> stielhandgranate has been set as usable\n", pBot);
			}
		}

		// if all checks failed then don't do this again because bot already used them or
		// this custom class doesn't come with multiple grenades
		if (grenade_count < 2)
		{
			pBot->SetSubTask(ST_NOOTHERNADE);

			if (botdebugger.IsDebugActions())
				HudNotify("CheckAmmoReserves() -> no other grenade was found\n", pBot);

#ifdef DEBUG
			//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
			//HudNotify("no other grenade was found\n");
#endif // DEBUG


		}
	}

	// fix the case when the bot picked up a grenade (as an item in a map)
	// the grenades were not picked up again after respawn and the bot still has a grenade slot marked as USED
	// (ie. the bot thinks he has a grenade)
	if ((pBot->grenade_action == ALTW_NOTUSED) && (pBot->grenade_slot != NO_VAL) &&
		!(pBot->bot_weapons & (1<<pBot->grenade_slot)))
	{
		pBot->grenade_action = ALTW_USED;
#ifdef _DEBUG
		//@@@@@@@@@@@@@@@@@@@@
		if (botdebugger.IsDebugActions())
			HudNotify("CheckAmmoReserves() -> ONGROUND ITEM explosives -> don't have them anymore -> USED flag SET!\n", pBot);
#endif
	}

	// if the bot is low on ammo set the need ammo flag
	if ((pBot->take_main_mags > 1) || (pBot->take_backup_mags > 1))
	{
		if (botdebugger.IsDebugActions())
			HudNotify("CheckAmmoReserves() -> need ammo flag has been set\n", pBot);

		pBot->SetNeed(NEED_AMMO);
	}
	// otherwise the bot must have enough ammo so clear it
	else
	{
		if (botdebugger.IsDebugActions())
			HudNotify("CheckAmmoReserves() -> need ammo flag has been removed\n", pBot);

		pBot->RemoveNeed(NEED_AMMO);
	}
}


/*
* returns index of the bot matching the name passed by the agrument
*/
int UTIL_FindBotByName(const char *name_string)
{
	char search_name[32];

	if ((name_string != nullptr) && (*name_string != 0))
		strcpy(search_name, name_string);
	else
		return -1;

	char bot_name[32];
	
	for (int index = 0; index < MAX_CLIENTS; index++)
	{
		if (bots[index].is_used == FALSE)
			continue;
		
		// get this bot name rather from edict struct
		strcpy(bot_name, STRING(bots[index].pEdict->v.netname));
		
		// match this bot name (whole/part of it) with that we are looking for
		if ((strstr(bot_name, search_name)) != nullptr)
		{
			return index;
		}
	}

	return -1;
}


/*
* kicks random bot in specified team or all bots on server
*/
bool UTIL_KickBot(int which_one)
{
	// specific bot
	if ((which_one >= 0) && (which_one < MAX_CLIENTS))
	{
		if (bots[which_one].is_used)  // is this slot used?
		{
			char cmd[80];
			
			sprintf(cmd, "kick \"%s\"\n", bots[which_one].name);
			
			SERVER_COMMAND(cmd);  // kick the bot using (kick "name")

			return TRUE;
		}
	}

	// all
	if (which_one == 100)
	{
		int temp = 0;

		for (int i=0; i < MAX_CLIENTS; i++)
		{
			if (bots[i].is_used)
			{
				temp++;

				char cmd[80];

				sprintf(cmd, "kick \"%s\"\n", bots[i].name);

				SERVER_COMMAND(cmd);
			}
		}

		if (temp > 0)
			return TRUE;
	}

	// random red or random blue
	if ((which_one == 100 + TEAM_RED) || (which_one == 100 + TEAM_BLUE))
	{
		int adequate[32];

		int j = 0;

		// set back current team number (only 1 or 2 are valid team numbers)
		which_one -= 100;

		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if ((bots[i].bot_team == which_one) && (bots[i].is_used))
			{
				adequate[j] = i;
				j++;
			}
		}

		if (j == 0)
			return FALSE;

		const int this_one = RANDOM_LONG(1, j) - 1;

		char cmd[80];

		sprintf(cmd, "kick \"%s\"\n", bots[adequate[this_one]].name);

		SERVER_COMMAND(cmd);

		return TRUE;
	}

	// random bot from any team
	if (which_one == -100)
	{
		int in_game;
		int adequate[32];
		
		int i = in_game = 0;

		// count how many bots is in game
		for (i = 0; i < MAX_CLIENTS; i++)
		{
			if (bots[i].is_used)
			{
				adequate[in_game] = i;
				in_game++;
			}
		}

		if (in_game < 1)
			return FALSE;

		i = RANDOM_LONG(1, in_game) - 1;

		char cmd[80];

		sprintf(cmd, "kick \"%s\"\n", bots[adequate[i]].name);

		SERVER_COMMAND(cmd);

		return TRUE;

	}

	return FALSE;
}


/*
* works same as kickbot only kills them
*/
bool UTIL_KillBot(int which_one)
{
	extern DLL_FUNCTIONS other_gFunctionTable;
	int temp = 0;

	// specific bot
	if ((which_one >= 0) && (which_one < MAX_CLIENTS))
	{
		if (bots[which_one].is_used)
		{
			(*other_gFunctionTable.pfnClientKill)(bots[which_one].pEdict);

			return TRUE;
		}
	}

	// all
	if (which_one == 100)
	{
		for (int i=0; i < MAX_CLIENTS; i++)
		{
			if (bots[i].is_used)
			{
				temp++;
				(*other_gFunctionTable.pfnClientKill)(bots[i].pEdict);
			}
		}

		if (temp > 0)
			return TRUE;
	}

	// random red or random blue
	if ((which_one == 100 + TEAM_RED) || (which_one == 100 + TEAM_BLUE))
	{
		which_one -= 100;

		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (bots[i].bot_team != which_one)
				continue;

			if (bots[i].is_used)
			{
				temp++;
				(*other_gFunctionTable.pfnClientKill)(bots[i].pEdict);
			}
		}

		if (temp > 0)
			return TRUE;
	}

	return FALSE;
}


/*
* counts all real clients on server from one team
* return this number.
*/
int UTIL_CountPlayers(int team)
{
	int num=0;

	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		// skip non existent players
		if (clients[i].pEntity == nullptr)
		{
			continue;
		}

		// skip all bots
		if (clients[i].pEntity->v.flags & FL_FAKECLIENT)
		{
			continue;
		}

		if (UTIL_GetTeam(clients[i].pEntity) == team)
		{
			++num;
		}
	}

	return num;
}


/*
* SECTION rewrote & fixed by kota@
* counts all clients on server and returns difference based on team that should be lowered
* red team diff is returned with value + 100
*/
int UTIL_BalanceTeams()
{
	int bot_count, reds, blues;

	int actual_pl = bot_count = reds = blues = 0;

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (clients[i].pEntity == nullptr)
			continue;
		else
		{
			++actual_pl;

			if (clients[i].pEntity->v.flags & FL_FAKECLIENT)
				++bot_count;

			// this is better way then just test v.team, because v.team doesn't always work
			if (UTIL_GetTeam(clients[i].pEntity) == TEAM_RED)
				reds++;
			if (UTIL_GetTeam(clients[i].pEntity) == TEAM_BLUE)
				blues++;
		}
	}

	// are there NO clients (i.e. server is empty)
	if (actual_pl == 0)
		return -2;
	// there are no bots so can't do the balance
	else if (bot_count == 0)
		return -1;
	// is there odd number of actual players
	// get the difference
	const int diff = reds - blues;

//#ifdef _DEBUG
//	ALERT(at_console, "<DEV DEBUG> UTIL_BalanceTeams() - clients %d | bots %d | reds %d | blues %d | diff %d\n",
//		actual_pl, bot_count, reds, blues, diff);
//#endif

	// nothing to do, team already balanced as possible
	if (diff > -2 && diff < 2)
	{
		return 0;
	}
	// we need to decrease red team
	else if (diff > 0)
	{
		return diff/2 + 100;
	}

	// we need to increase red team
	// because diff<0
	return diff/(-2);
}


/*
* gets team diff from bot_manager and calls "changeteam" to change team if needed
*/
int UTIL_DoTheBalance()
{
	extern bool is_dedicated_server;
 
	// if teams are already balanced
	if (botmanager.GetTeamsBalanceValue() <= 0)
	{
#ifdef NDEBUG
		UTIL_DebugInFile("UTIL | D_T_B | T_B_V\n");
#endif
#ifdef _DEBUG
		UTIL_DebugDev("UTIL | DoTheBalance()->teams balance value <= 0\n", -100, -100);
#endif
		return -128;
	}

	static int state = 0;		// current state of state machine
	static int bots_to_go = -1;
	static int team = -1;
	static int bot_index = -1;

	// do the first run init
	switch (state)
	{
		case 0:
			if (botmanager.GetTeamsBalanceValue() > 100)
			{
				team = TEAM_RED;
				int temp = botmanager.GetTeamsBalanceValue();
				temp -= 100;
				bots_to_go = temp;
			}
			else
			{
				team = TEAM_BLUE;
				bots_to_go = botmanager.GetTeamsBalanceValue();
			}

			state++;

		case 1:
			{
				bot_index = -1;

				for (int index = 31; index >= 0; index--)
				{
					// skip unused slots
					if (bots[index].is_used == FALSE)
						continue;

					// stop when have right team
					if (bots[index].bot_team == team)
					{
						bot_index = index;
						break;
					}
				}

				// only if is valid index (due to this: static int bot_index = -1;)
				//fatality, break balance!!! kota@
				if (bot_index < 0)
				{
					bots_to_go = 0;
					break;
				}

				FakeClientCommand(bots[bot_index].pEdict, "changeteam", nullptr, nullptr);

				state++;

				break;
			}

		case 2: 
			//sanity check if this bot exists kota@
			if (bots[bot_index].is_used == FALSE)
			{
				bots_to_go = 0;
				break;
			}

			if (team == TEAM_RED)
			{
				if ((g_mod_version == FA_29) || (g_mod_version == FA_30))
					FakeClientCommand(bots[bot_index].pEdict, "vguimenuoption", "2", nullptr);
				else
					FakeClientCommand(bots[bot_index].pEdict, "vguimenuoption", "3", nullptr);
			}
			else if (team == TEAM_BLUE)
			{
				if ((g_mod_version == FA_29) || (g_mod_version == FA_30))
					FakeClientCommand(bots[bot_index].pEdict, "vguimenuoption", "1", nullptr);
				else
					FakeClientCommand(bots[bot_index].pEdict, "vguimenuoption", "2", nullptr);
			}

			state++;

			break;

		case 3:
			{
				int i;

				// find this bot in client array
				for (i=31; (i>=0) && (clients[i].pEntity != bots[bot_index].pEdict); --i)
					;

				//this is fatality
				if (i < 0)
				{
					bots_to_go = 0;
					break;
				}

				// and update bot_team with new value
				if (clients[i].pEntity->v.team != bots[bot_index].bot_team)
					bots[bot_index].bot_team = clients[i].pEntity->v.team;

				// look if another bot have targeted this one ie is his pBotEnemy
				edict_t *pTarget = bots[bot_index].pEdict;

				for (i = 0; i < MAX_CLIENTS; ++i)
				{
					if (bots[i].is_used == FALSE)
						continue;

					// so NULL it
					if (bots[i].pBotEnemy == pTarget)
						bots[i].BotForgetEnemy();
				}

				state++;
			}

		case 4:
			bots_to_go--;
			// reset these before next round
			state = 1;		// must be reset to 1 becase state 0 is full init
			bot_index = -1;
	}
	
	// are teams balanced now
	if (bots_to_go == 0)
	{
		if (is_dedicated_server)
			PrintOutput(nullptr, "teams balancing finished\n", MType::msg_info);
		else
			PrintOutput(nullptr, "Teams balancing finished\n", MType::msg_info);

		// set those on last run
		botmanager.ResetTeamsBalanceValue();
		state = 0;
		bots_to_go = -1;

		return 1;
	}

	if (state == 0)
		return 0;
	else
		return -1;
}
// SECTION BY kota@ END


/*
* changes bot skill level to all bots
* by_one == true means increase/decrease by one level
* skill_level == -1 means decreasing and == 1 is increasing
*/
extern int UTIL_ChangeBotSkillLevel(bool by_one, int skill_level)
{
	// increase/decrease by one level
	if (by_one)
	{
		bool no_bots = TRUE;

		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			// skip unused slots
			if (bots[i].is_used == FALSE)
				continue;

			// skip bots with highest skill while increasing
			if ((bots[i].bot_skill == 0) && (skill_level == 1))
				continue;

			// skip bots with lowest skill while decreasing
			if ((bots[i].bot_skill == BOT_SKILL_LEVELS - 1) && (skill_level == -1))
				continue;

			// found atleast one bot to process
			no_bots = FALSE;

			// store current skill level
			int prev_level = bots[i].bot_skill;
			
			// increase skill
			if (skill_level == 1)
				prev_level--;
			// decrease skill
			else if (skill_level == -1)
				prev_level++;

			// set new skill level
			bots[i].bot_skill = prev_level;
		}

		if (no_bots)
			return 0;
		else
			return 1;
	}
	// otherwise set to skill_level
	else if (by_one == FALSE)
	{
		skill_level -= 1;		// due array based style

		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			// skip unused slots
			if (bots[i].is_used == FALSE)
				continue;

			bots[i].bot_skill = skill_level;
		}

		return skill_level + 1;
	}

	// some error occured
	return -1;
}


/*
* changes aim skill level to all bots
*/
int UTIL_ChangeAimSkillLevel(int skill_level)
{
	skill_level -= 1;		// due array based style

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		// skip unused slots
		if (bots[i].is_used == FALSE)
			continue;

		bots[i].aim_skill = skill_level;
	}

	return skill_level + 1;
}


/*
* translates weapon name to weapon ID value and returns the ID value
*/
// This should handle all the if (g_mod_version ==) however this would also increase CPU load
// because it would have to go throuh the array with each if (pBot->current_weapon.iId == )
/*/
int UTIL_GetIDFromName(const char *weapon_name)
{
	extern bot_weapon_t weapon_defs[MAX_WEAPONS];

	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		if (strcmp(weapon_defs[i].szClassname, weapon_name) == 0)
		{
			return weapon_defs[i].iId;
		}
	}

#ifdef _DEBUG
	char msg[256];
	sprintf(msg, "\"%s\" isn't valid weapon name or this weapon doesn't exist in currect mod",
		weapon_name);
	UTIL_DebugInFile(msg);
#endif

	// ensures false evaluation
	return -1;
}
/**/



/*
* checks if the bot has stealth skill
*/
bool UTIL_IsStealth(edict_t *pEdict)
{
	if ((g_mod_version == FA_29) || (g_mod_version == FA_30))
	{
		if (pEdict->v.iuser3 & USR3_STEALTH)
			return TRUE;
	}

	//if ((g_mod_version != FA_29) || (g_mod_version != FA_30))		// ORIGINAL CODE
	if ((g_mod_version != FA_29) && (g_mod_version != FA_30))
	{
		if (pEdict->v.iuser3 & USR3_STEALTH_GE)
			return TRUE;
	}

	return FALSE;
}


/*
* checks weapon if we can mount a silencer to it
*/
bool UTIL_CanMountSilencer(bot_t *pBot)
{
	if ((pBot->current_weapon.iId == fa_weapon_m4) ||
		(pBot->current_weapon.iId == fa_weapon_mp5a5) ||
		(pBot->current_weapon.iId == fa_weapon_uzi) ||
		(pBot->current_weapon.iId == fa_weapon_ber92f))
	{
		return TRUE;
	}

	return FALSE;
}


/*
* checks if bot can go/resume prone
* if so then set the appropriate task
*/
bool UTIL_GoProne(bot_t *pBot, const char* loc)
{
	if (!pBot->IsGoingProne() && !pBot->IsSubTask(ST_CANTPRONE) && !pBot->IsTask(TASK_BIPOD) &&
		!pBot->IsBehaviour(BOT_DONTGOPRONE) && (pBot->weapon_action == W_READY))
	{
		pBot->SetTask(TASK_GOPRONE);

		
#ifdef DEBUG
		//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@														// NEW CODE 094 (remove it)
		if (loc != NULL)
		{
			char dm[256];
			sprintf(dm, "%s Called UTIL_GoProne() @ %s\n", pBot->name, loc);
			HudNotify(dm, true, pBot);
		}
		//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#endif // DEBUG


		return true;
	}

	return false;
}


/*
* clear all Stance flags first and then set the new one
*/
void SetStance(bot_t *pBot, int flag, const char* loc)
{
	return SetStance(pBot, flag, false, loc);
}


/*
* overloaded to allow forcing stance right after we just set one
*/
void SetStance(bot_t *pBot, int flag, bool is_forced_stance, const char* loc)
{
	// if the bot already is in this Stance OR using bipod OR still changing the Stance, don't set new one
	if ((pBot->bot_behaviour & flag) || pBot->IsTask(TASK_BIPOD) || (pBot->f_bipod_time > gpGlobals->time) ||
		((pBot->f_stance_changed_time > gpGlobals->time) && !is_forced_stance))
		return;

	if (flag & (BOT_STANDING | BOT_CROUCHED | BOT_PRONED))
	{
#ifdef DEBUG
		UTIL_DebugInFile("util.cpp|SetStance() -> INVALID stance bit was sent !!!\n");
#endif // DEBUG

		return;
	}


#ifdef DEBUG
	//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@														// NEW CODE 094 (remove it)
	if (loc != NULL)
	{
		char dm[256];
		sprintf(dm, "%s Called SetStance() @ %s\n", pBot->name, loc);
		HudNotify(dm, true, pBot);
	}
	//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#endif // DEBUG




	// if we want the bot to go prone, but he cannot go prone at his current location ...
	if ((flag & GOTO_PRONE) && pBot->IsSubTask(ST_CANTPRONE))
	{



#ifdef DEBUG
		// @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@							NEW CODE 094 (remove it)
		char dm[256];
		sprintf(dm, "%s - util.cpp|SetStance() GOTO PRONE && SubTask CANTPRONE -> Stance set to GOTO STANDING !!!\n", pBot->name);
		HudNotify(dm, true, pBot);
#endif // DEBUG




		// then reset the stance back to standing
		flag = GOTO_STANDING;
	}


	if (pBot->IsGoingProne())
	{



#ifdef DEBUG
		char dm[256];
		sprintf(dm, "%s - util.cpp|SetStance() -> called while going to/resume from prone !!!\n", pBot->name);
		UTIL_DebugInFile(dm);
#endif // DEBUG



	}

	// clear all flags
	UTIL_ClearBit(GOTO_PRONE, pBot->bot_behaviour);
	UTIL_ClearBit(GOTO_CROUCH, pBot->bot_behaviour);
	UTIL_ClearBit(GOTO_STANDING, pBot->bot_behaviour);

#ifdef _DEBUG
	if (pBot->is_forced)
		flag = pBot->forced_stance;
#endif

	// and set new flag
	pBot->bot_behaviour |= flag;

	//																				NEW CODE 094
	// we need to store the time when we changed stance and also adding short delay there
	pBot->f_stance_changed_time = gpGlobals->time + 0.5f;

	return;
}


/*
* returns true if the bot is close enough to hear that noise/call
* also checks the team of the invoker (ie don't react on enemies)
*/
bool UTIL_HeardIt(bot_t *pBot, edict_t *pInvoker, float range)
{
	edict_t *pEdict = pBot->pEdict;

	const int player_team = UTIL_GetTeam(pInvoker);
	const int bot_team = UTIL_GetTeam(pEdict);

	// teams doesn't match so break it
	if (bot_team != player_team)
		return FALSE;

	// get the distance from the noise
	const float distance = (pEdict->v.origin - pInvoker->v.origin).Length();

	// set the ability of the bot to hear the noise, based on bot skill level
	// (ie better bots can hear sounds from bigger distance)
	const float sensitivity = range - ((pBot->bot_skill + 1) * (range / 10));

	// is the bot close enough to hear it
	if (distance < sensitivity)
		return TRUE;

	return FALSE;
}


/*
* removes all invalid paths (ie path length = 1) as well as tries to fix those which include invalid waypoints
* returns total number of paths that has been either fixed or deleted
* if print_details is TRUE it also does print index of each removed or fixed path
*/
int UTIL_RemoveFalsePaths(bool print_details)
{
	int num_of_removed_paths = 0;
	char msg[64];

	for (int path_index = 0; path_index < num_w_paths; path_index++)
	{
		//																								NEW CODE 094
		// first remove all invalid waypoints from this path
		int result = WaypointValidatePath(path_index);
		//******************************************************************************************	NEW CODE 094 END

		// is path length equal 1 (then the whole path is invalid)
		if (WaypointGetPathLength(path_index) == 1)
		{
			// delete that path
			// I'm using WaypointDeletePath() because DeleteWholePath() isn't known outside waypoint.cpp
			if (WaypointDeletePath(nullptr, path_index))
			{
				num_of_removed_paths++;

				// if removing actual path then clear also "pointer" on it
				if (path_index == internals.GetPathToContinue())
				{
					internals.ResetPathToContinue();
				}

				if (print_details)
				{
					sprintf(msg, "Invalid path (path no. %d) was removed\n", path_index + 1);
					HudNotify(msg);
				}

				// we must reset it in order to prevent printing wrong messages
				// eg. "path no. 5 was removed" followed by "path no. 5 was repaired"
				result = 0;																					// NEW CODE 094
			}
		}

		//																								NEW CODE 094
		// finally set correct return value based on the result
		if (result == 1)
		{
			num_of_removed_paths++;

			if (print_details)
			{
				sprintf(msg, "Invalid path (path no. %d) was repaired\n", path_index + 1);
				HudNotify(msg);
			}
		}
		else if (result == -1)
		{
			num_of_removed_paths += 600;		// there's max of 512 paths so this value is safe

			if (print_details)
			{
				sprintf(msg, "Unable to repair invalid path (path no. %d)\n", path_index + 1);
				HudNotify(msg);
			}
		}
		//******************************************************************************************	NEW CODE 094 END
	}

	return num_of_removed_paths;
}


/*
* checks all paths for possible problems like ...
* if goback waypoint is found in one-way path we report it as a bug
* if parachute waypoint is found in one-way path we print a warning to make sure the bot already has a parachute when
* approaching this waypoint
* returns total count of problem paths or zero if there was no problem found
*/
int UTIL_CheckPathsForProblems(bool log_in_file)
{
	int num_of_problem_paths = 0;
	const int max_problems = 10;		// prevents crashing the server due to overloading on a netchan
	char msg[256];

	// go through all paths ...
	for (int path_index = 0; path_index < num_w_paths; path_index++)
	{
		// look for a goback waypoint in one-way path
		if (WaypointScanPathForBadCombinationOfFlags(path_index, PathT::path_one, WptT::wpt_goback))
		{
			num_of_problem_paths++;

			sprintf(msg, "BUG: There's a goback waypoint in one-way path (path no. %d)\n", path_index + 1);
			HudNotify(msg, log_in_file);
		}

		// if there are more problems than the given maximum we simply stop checking for more
		if (num_of_problem_paths >= max_problems)
			return num_of_problem_paths;

		// look for a parachute waypoint in one-way path
		if (WaypointScanPathForBadCombinationOfFlags(path_index, PathT::path_one, WptT::wpt_chute))
		{
			num_of_problem_paths++;

			sprintf(msg, "WARNING: There's a parachute waypoint in one-way path (path no. %d)\nMake sure the bot already has the parachute pack there.\n", path_index + 1);
			HudNotify(msg, log_in_file);
		}

		if (num_of_problem_paths >= max_problems)
			return num_of_problem_paths;

		// look for invalid path ends ie. paths not ending at a cross waypoint or not ending at ammobox etc.
		if (WaypointCheckInvalidPathEnd(path_index, log_in_file) > 0)
			num_of_problem_paths++;

		if (num_of_problem_paths >= max_problems)
			return num_of_problem_paths;

		// also report the invalid merge of paths 
		//if (WaypointCheckInvalidPathMerge(path_index, log_in_file) > 0)								NEW CODE 094 (prev code)
		//	num_of_problem_paths++;
		
		//																								NEW CODE 094
		if (WaypointRepairInvalidPathMerge(path_index, false, log_in_file) > 0)
			num_of_problem_paths++;

		if (num_of_problem_paths >= max_problems)
			return num_of_problem_paths;

		// look for multiple addition of the same waypoint into this path
		W_PATH *p = w_paths[path_index];

		while (p)
		{
			W_PATH *rp = p->next;				// start on the next waypoint from this path ...
			int stop = 0;

			while (rp)
			{
				// go through the rest of the path and look for repeated addition of the same waypoint
				if (rp->wpt_index == p->wpt_index)
				{
					num_of_problem_paths++;

					sprintf(msg, "BUG: Waypoint no. %d has been added more than once to path no. %d.\n",
						p->wpt_index + 1, path_index + 1);
					HudNotify(msg, log_in_file);

					if (num_of_problem_paths >= max_problems)
						return num_of_problem_paths;
				}

				rp = rp->next;
			}

			p = p->next;
		}
	}

	// now go through all waypoints and check for solitary waypoints (i.e. waypoints that aren't part of any path)
	for (int wpt_index = 0; wpt_index < num_waypoints; wpt_index++)
	{
		// skip all waypoints that aren't allowed in paths
		if (IsWaypoint(wpt_index, WptT::wpt_no) || IsWaypoint(wpt_index, WptT::wpt_aim) || IsWaypoint(wpt_index, WptT::wpt_cross))
			continue;

		// if we can't find any path on this waypoint then it must be solitary
		if (FindPath(wpt_index) == -1)
		{
			num_of_problem_paths++;

			sprintf(msg, "WARNING: Waypoint no. %d isn't part of any path. There should NOT be lone waypoints except for 'aim' and 'cross'.\n", wpt_index + 1);
			HudNotify(msg, log_in_file);
		}

		if (num_of_problem_paths >= max_problems)
			return num_of_problem_paths;
	}
	//**********************************************************************************************	NEW CODE 094 END

	return num_of_problem_paths;
}


/*
* goes through all waypoints
*/
void UTIL_RepairWaypointRangeandPosition(edict_t *pEdict)
{
	for (int index = 0; index < num_waypoints; index++)
		UTIL_RepairWaypointRangeandPosition(index, pEdict);

	return;
}


/*
* Overloaded to force just range changes without reposition
*/
void UTIL_RepairWaypointRangeandPosition(edict_t *pEdict, bool dont_move)
{
	for (int index = 0; index < num_waypoints; index++)
		UTIL_RepairWaypointRangeandPosition(index, pEdict, dont_move);

	return;
}


/*
* tries to repair bad waypoint placement and range setting (ie. when the waypoint range does intersect walls etc.)
*/
bool UTIL_RepairWaypointRangeandPosition(int wpt_index, edict_t *pEdict)
{
	return UTIL_RepairWaypointRangeandPosition(wpt_index, pEdict, false);
}


/*
* Overloaded to force just range changes without reposition
*/
bool UTIL_RepairWaypointRangeandPosition(int wpt_index, edict_t *pEdict, bool dont_move)
{
	if (wpt_index != -1 && wpt_index < num_waypoints)
	{
		bool cant_move = false;			// to disable default reposition if we detect that the waypoint has a purpose
										// (eg. when there are bandages right next to it)


		// TODO: We should handle aiming and cross waypoints getting out of range
		// when the master/nearby waypoint got repositioned


		// for now do not work with these waypoints
		if (waypoints[wpt_index].flags & (W_FL_DELETED | W_FL_AIMING | W_FL_CROSS | W_FL_LADDER))
			return FALSE;
		
		// get this waypoint range
		float the_range = waypoints[wpt_index].range;
		
		// we need more space for the whole body ... without this the bot would still hit the obstacle
		// (basically add the body size to the range)
		the_range += static_cast<float>(15);

		edict_t *pent = nullptr;
		// the distance we will move the waypoint origin
		const float move_d = static_cast<float>(10);
		// value used to decrease the range
		const float dec_r = static_cast<float>(10);
		
		// first check for some important entities around the waypoint
		while ((pent = UTIL_FindEntityInSphere(pent, waypoints[wpt_index].origin, static_cast<float>(30))) != nullptr)
		{
			// if there are bandages right next to the waypoint then do not reposition it
			if ((strcmp(STRING(pent->v.classname), "item_bandage") == 0))
			{
				cant_move = true;
			}
			
			// check for door entity and make the waypoint a door waypoint if it is right in the doorway
			if ((strcmp(STRING(pent->v.classname), "func_door") == 0) ||
				(strcmp(STRING(pent->v.classname), "func_door_rotating") == 0) ||
				(strcmp(STRING(pent->v.classname), "momentary_door") == 0))
			{
				waypoints[wpt_index].flags |= W_FL_DOOR;
				waypoints[wpt_index].range = static_cast<float>(20);
				
				cant_move = true;
			}
		}

		// do not reposition jump waypoints else the bots may not be able to move towards enemy/goal at all
		if (waypoints[wpt_index].flags & (W_FL_DUCKJUMP | W_FL_JUMP))
			cant_move = true;
		
		if (cant_move)
			ALERT(at_console, "CANNOT REPOSITION WAYPOINT #%d IT COULD LOSE ITS PURPOSE!!!\n", wpt_index + 1);

		// we've found that this waypoint has a purpose at its current position
		// so we must prevent its reposition
		if (cant_move && !dont_move)
			dont_move = true;
		
		// init the new origin with the original waypoint position for the first run
		Vector new_origin = waypoints[wpt_index].origin;
		
		// first check for obstacles using standard waypoint origin
		SelfControlledWaypointReposition(the_range, new_origin, move_d, dec_r, dont_move, pEdict);
		
		// drop the waypoint origin a few units lower to catch sandbags, ledges and like ...
		new_origin = new_origin - Vector (0, 0, 15);
		
		// check for obstacles there ...
		SelfControlledWaypointReposition(the_range, new_origin, move_d, dec_r, dont_move, pEdict);
		
		// and return the origin back where it should be
		new_origin = new_origin + Vector (0, 0, 15);
		
		// remove the body size addition, but don't go under range == 5
		if (the_range >= 20)
			the_range -= static_cast<float>(15);
		
		// and set the tweaked range now
		waypoints[wpt_index].range = the_range;
		
		// if we moved the waypoint then overwrite the original waypoint position with the new one
		// (ie. now we really do move the waypoint itself)
		if (new_origin != waypoints[wpt_index].origin)
		{
			waypoints[wpt_index].origin = new_origin;

			ALERT(at_console, "Waypoint #%d was moved to NEW position (%.1f, %.1f, %.1f)\n", wpt_index + 1,
				waypoints[wpt_index].origin.x, waypoints[wpt_index].origin.y, waypoints[wpt_index].origin.z);
		}
		
		// check for door entity and make the waypoint a door waypoint if it is right in the doorway
		while ((pent = UTIL_FindEntityInSphere(pent, waypoints[wpt_index].origin, static_cast<float>(30))) != nullptr)
		{
			if ((strcmp(STRING(pent->v.classname), "func_door") == 0) ||
				(strcmp(STRING(pent->v.classname), "func_door_rotating") == 0) ||
				(strcmp(STRING(pent->v.classname), "momentary_door") == 0))
			{
				waypoints[wpt_index].flags |= W_FL_DOOR;
				waypoints[wpt_index].range = static_cast<float>(20);
			}
		}
		
		while ((pent = UTIL_FindEntityInSphere(pent, waypoints[wpt_index].origin, static_cast<float>(50))) != nullptr)
		{
			if (strcmp(STRING(pent->v.classname), "ammobox") == 0)
			{
				waypoints[wpt_index].flags |= W_FL_AMMOBOX;
				if (the_range > static_cast<float>(20))
					waypoints[wpt_index].range = static_cast<float>(20);
			}
		}

		return TRUE;
	}

	return FALSE;
}


// check if bot is in wpt_index wpt range		CURRENTLY NOT USED
// if bot is in range return wpt_index range otherwise return 9999.0
/*float UTIL_IsInRange(edict_t *pEdict, int wpt_index)
{
	// is it valid
	if (wpt_index != -1)
	{
		float dist = (pEdict->v.origin - waypoints[wpt_index].origin).Length();

		// is in range or really close to it
		if (dist < waypoints[wpt_index].range + 10.0)
			return waypoints[wpt_index].range;
	}

	return (float) 9999.0;
}*/


/*
* is this waypoint range small ie < 20
*/
bool UTIL_IsSmallRange(int wpt_index)
{
	// is it valid wpt
	if ((wpt_index != -1) && (waypoints[wpt_index].range < RANGE_SMALL))
		return TRUE;

	return FALSE;
}


/*
* should the bot stop and don't move at this waypoint
*/
bool UTIL_IsDontMoveWpt(edict_t *pEdict, int wpt_index, bool passed)
{
	// is it valid wpt
	if (wpt_index != -1)
	{
		// the bot is heading toward it
		if ((waypoints[wpt_index].flags & W_FL_SNIPER) && (passed == FALSE))
			return TRUE;

		// the bot just passed it (probably stading at it, but already have next wpt)
		if ((waypoints[wpt_index].flags & W_FL_SNIPER) && (passed))
		{
			const float dist = (pEdict->v.origin - waypoints[wpt_index].origin).Length();

			// is still in wpt range
			if (dist <= waypoints[wpt_index].range)
				return TRUE;
		}
	}

	return FALSE;
}


/*
* returns the right direction to climb this ladder based on ladder vs bot origin z-coord
*/
int UTIL_GetLadderDir(bot_t *pBot)
{
	edict_t *pent = nullptr;

	while ((pent = UTIL_FindEntityInSphere(pent, pBot->pEdict->v.origin, PLAYER_SEARCH_RADIUS)) != nullptr)
	{
		if (strcmp("func_ladder", STRING(pent->v.classname)) == 0)
		{
			const Vector ladder_origin = VecBModelOrigin(pent);

#ifdef _DEBUG
			if (botdebugger.IsDebugPaths() || botdebugger.IsDebugWaypoints())
			{
				ALERT(at_console, "LADDER - bot z-coord=%.1f | ladder z-coord=%.1f)\n",
					(float) pBot->pEdict->v.origin.z, (float) ladder_origin.z);
			}
#endif

			// the bot has to climb down
			if (ladder_origin.z < pBot->pEdict->v.origin.z)
				return LADDER_DOWN;
			// the bot has to climb up
			else if (ladder_origin.z >= pBot->pEdict->v.origin.z)
				return LADDER_UP;
		}
	}

	return LADDER_UNKNOWN;
}


/*
* returns true if there's a breakable object in front of the bot
*/
bool UTIL_CheckForwardForBreakable(edict_t *pEdict)
{
	TraceResult tr;
				
	UTIL_MakeVectors(pEdict->v.v_angle);
	// we're using just origin here, because if the brekable is a ventilation cover the head of the bot
	// can be higher then the breakable itself
	const Vector v_src = pEdict->v.origin;
	const Vector v_dest = v_src + gpGlobals->v_forward * 250;
				
	UTIL_TraceLine(v_src, v_dest, dont_ignore_monsters, pEdict, &tr);

	// spawnflags & takedamage will tell us if it can be destroyed or not
	if ((strcmp(STRING(tr.pHit->v.classname), "func_breakable") != 0) ||
		((strcmp(STRING(tr.pHit->v.classname), "func_breakable") == 0) &&
		((tr.pHit->v.spawnflags & SF_BREAK_TRIGGER_ONLY) ||
		(tr.pHit->v.takedamage == DAMAGE_NO))))
	{
		// sd object with impulse == 1 can be killed only by claymore
		if ((strcmp(STRING(tr.pHit->v.classname), "fa_sd_object") != 0) ||
			((strcmp(STRING(tr.pHit->v.classname), "fa_sd_object") == 0) &&
			(tr.pHit->v.impulse == 1)))
			return FALSE;	// no func_breakable as well as no sd object
		else
		{
			return TRUE;	// there's some sd object
		}
	}
	else
	{
		return TRUE;	// there's some func_breakable
	}
}


/*
* returns true if there's a breakable object somewhere around the bot
*/
bool UTIL_CheckForBreakableAround(bot_t *pBot)
{
	edict_t *pent = nullptr;
	edict_t *pEdict = pBot->pEdict;

	// search the surrounding for entities
	while ((pent = UTIL_FindEntityInSphere(pent, pEdict->v.origin, EXTENDED_SEARCH_RADIUS)) != nullptr)
	{
		// handle only these two entities
		if ((strcmp(STRING(pent->v.classname), "func_breakable") != 0) &&
			(strcmp(STRING(pent->v.classname), "fa_sd_object") != 0))
			continue;

		// not breakable by gunfire
		if ((pent->v.spawnflags & SF_BREAK_TRIGGER_ONLY) || (pent->v.takedamage == DAMAGE_NO) ||
			(pent->v.impulse == 1))
			continue;
		
		// don't target own objects, this will also allow us to target neutral objects
		// we have to check it this way, because fa sd object has team values swapped,
		// ie. if it is a red team object (reds own it) it has the team value set to blue team
		// and vice versa (blue object has red team value)
		if (((pent->v.team == TEAM_RED) && (pEdict->v.team == TEAM_BLUE)) ||
			((pent->v.team == TEAM_BLUE) && (pEdict->v.team == TEAM_RED)))
			continue;

		// will handle entities vith origin 0,0,0
		Vector entity_origin = VecBModelOrigin(pent);

		Vector entity = pent->v.origin - pEdict->v.origin;

		TraceResult tr;
		Vector v_src = pEdict->v.origin + pEdict->v.view_ofs;

		// do we see the entity from here
		UTIL_TraceLine(v_src, entity_origin, dont_ignore_monsters, pEdict, &tr);

		if ((strcmp(STRING(tr.pHit->v.classname), "func_breakable") == 0) ||
			(strcmp(STRING(tr.pHit->v.classname), "fa_sd_object") == 0) ||
			((tr.flFraction == 1.0) && (pent->v.solid == SOLID_BSP)))
		{
			if (strcmp(STRING(tr.pHit->v.classname), "worldspawn") == 0)
				pBot->SetSubTask(ST_RANDOMCENTRE);
			
			// store the entity we found
			pBot->pItem = pent;

			return TRUE;
		}
	}

	return FALSE;
}


/*
* looks for any usable entity in the vicinity (e.g. button)
* returns true if bot should do any waypoint action that needs pressing the 'use' key
* returns fale if the bot didn't find anything, but we will hit the 'use' key anyway
* (in case there's special entity set up on some custom map that works like a button and requires pressing 'use' key)
*/
bool UTIL_CheckForUsablesAround(bot_t *pBot)
{
	edict_t *pent = nullptr;
	char item_name[64];
	Vector entity_origin;
	float search_radius;//																			NEW CODE 094


#ifdef DEBUG
		//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		char dbgmsg[256];
		sprintf(dbgmsg, "%s - UTIL_CheckForUsables() called\n", pBot->name);
		HudNotify(dbgmsg, pBot);
#endif // DEBUG



	edict_t *pEdict = pBot->pEdict;

	// if we are at ammobox waypoint then we can extend the search radius, because ammoboxes can be used from
	// greater distance than other entities such as a button for example
	if (IsWaypoint(pBot->curr_wpt_index, WptT::wpt_ammobox))
		search_radius = 96.0;
	else
		search_radius = PLAYER_SEARCH_RADIUS;

	// search the surrounding for entities
	while ((pent = UTIL_FindEntityInSphere(pent, pEdict->v.origin, search_radius)) != nullptr)
	{
		strcpy(item_name, STRING(pent->v.classname));

		if ((strcmp("ammobox", item_name) == 0) || (strcmp("momentary_rot_button", item_name) == 0))
		{
			// make vector to entity
			const Vector entity = pent->v.origin - pEdict->v.origin;

			// make angles from vector to entity
			const Vector bot_angles = UTIL_VecToAngles(entity);

			// look at the entity at correct angle
			pEdict->v.idealpitch = -bot_angles.x;
			BotFixIdealPitch(pEdict);

			// face the entity
			pEdict->v.ideal_yaw = bot_angles.y;
			BotFixIdealYaw(pEdict);

			// print what we just found if needed
			if (botdebugger.IsDebugActions())
			{
				if (strcmp("ammobox", item_name) == 0)
					HudNotify("Found ammobox\n", pBot);
				else if (strcmp("momentary_rot_button", item_name) == 0)
					HudNotify("Found m_rot_button (wheel)\n", pBot);
			}

			// store the entity we found
			pBot->pItem = pent;

			// NOTE: this should be changed to more complex time setting
			//		based on the difference of current view angle and angle to item

			// set some time to face the item
			pBot->f_face_item_time = gpGlobals->time + 0.5f;

			// no need to continue in searching
			return true;
		}

		// use BModel conversion for entities that are BModels
		// (ie. those entities have 0,0,0 as an origin value)
		entity_origin = VecBModelOrigin(pent);

		if ((strcmp("func_tankcontrols", item_name) == 0) || (strcmp("func_button", item_name) == 0) ||
			(strcmp("button_target", item_name) == 0))
		{
			const Vector entity = entity_origin - pEdict->v.origin;

			const Vector bot_angles = UTIL_VecToAngles(entity);

			pEdict->v.idealpitch = -bot_angles.x;
			BotFixIdealPitch(pEdict);

			pEdict->v.ideal_yaw = bot_angles.y;
			BotFixIdealYaw(pEdict);

			if (botdebugger.IsDebugActions())
			{
				if (strcmp("func_tankcontrols", item_name) == 0)
					HudNotify("Found TANK (mounted gun)\n", pBot);
				else if (strcmp("func_button", item_name) == 0)
					HudNotify("Found button\n", pBot);
				else if (strcmp("button_target", item_name) == 0)
					HudNotify("Found button_target\n", pBot);
			}

			pBot->pItem = pent;

			// NOTE: Same as above, it should be changed to more complex time setting
			//		based on the difference of current view angle and angle to item
			pBot->f_face_item_time = gpGlobals->time + 0.5f;

			return true;
		}
	}

	// didn't find anything but "press" the use key anyway
	return false;
}


/*
* check if there is any free medic
* passive == TRUE means that the bot is medic and listens if someone around is bleeding
*/
bool UTIL_IsAnyMedic(bot_t *pBot, edict_t *pWounded, bool passive)
{
	edict_t *pEdict = pBot->pEdict;
	float sensitivity;

	// the bot is already going to heal someone else
	if (pBot->IsTask(TASK_HEALHIM))
		return FALSE;

	// the wounded edict died
	if (IsAlive(pWounded) == FALSE)
		return FALSE;

	// the wounded edict is fit
	if (pWounded->v.health >= 100)
		return FALSE;

	// don't react on observers
	if (botdebugger.IsObserverMode() && !(pWounded->v.flags & FL_FAKECLIENT))
		return FALSE;

	// if the bot is being used AND his user isn't the wounded one
	if ((pBot->pBotUser != nullptr) && (pBot->pBotUser != pWounded))
	{
		// in 25% of time stay with your current "user" (ie ignore the wouded teammate)
		if (RANDOM_LONG(1, 100) <= 25)
			return FALSE;
	}

	const int player_team = UTIL_GetTeam(pWounded);
	const int bot_team = UTIL_GetTeam(pEdict);

	// don't heal your enemies
	if (bot_team != player_team)
		return FALSE;

	const float distance = (pEdict->v.origin - pWounded->v.origin).Length();

	if (passive)
		sensitivity = 350 - ((pBot->bot_skill + 1) * 50);		// was 550
	else
		sensitivity = 1100 - ((pBot->bot_skill + 1) * 100);

	if (distance < sensitivity)
	{
		// has the bot only last 'keep it for yourself' bandage? or is there another can't do a
		// medical treatment situation
		// also if the bot has enemy AND the enemy is heavily hurt AND not a chance
		// then ignore the call
		if ((pBot->bot_bandages <= 1) || (pBot->sniping_time > gpGlobals->time) ||
			pBot->IsTask(TASK_BIPOD) || pBot->IsTask(TASK_USETANK) ||
			((pBot->pBotEnemy != nullptr) && (pBot->pBotEnemy->v.health < 50) &&
			(RANDOM_LONG(1, 100) > 50)))
		{
			// don't say this that much
			if (RANDOM_LONG(1, 100) <= 50)
				pBot->BotSpeak(SAY_MEDIC_CANT_HELP, STRING(pWounded->v.netname));

			return FALSE;
		}

		// check if another bot is going to heal this player
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (bots[i].is_used)
			{
				// another bot has this player as an enemy AND is healing flag
				if ((pWounded == bots[i].pBotEnemy) && bots[i].IsTask(TASK_HEALHIM))
				{
					return FALSE;
				}
			}
		}

		const Vector v_bothead = pEdict->v.origin + pEdict->v.view_ofs;
		const Vector v_patient = pWounded->v.origin + pWounded->v.view_ofs;
		TraceResult tr;

		UTIL_TraceLine(v_bothead, v_patient, ignore_monsters, pEdict, &tr);

		// sees the bot the wounded soldier?
		if ((tr.flFraction >= 1.0) ||
			((tr.flFraction >= 0.95) && (strcmp("player", STRING(tr.pHit->v.classname)) == 0)))
		{
			// the bot has a patient from now
			pBot->pBotEnemy = pWounded;
			pBot->f_bot_see_enemy_time = gpGlobals->time;

			const Vector v_wounded = (v_patient - v_bothead);

			const Vector wounded_angles = UTIL_VecToAngles(v_wounded);

			// face the wounded soldier
			pEdict->v.ideal_yaw = wounded_angles.y;
			BotFixIdealYaw(pEdict);

			// don't look for new waypoint and keep track of current waypoint
			pBot->f_dont_look_for_waypoint_time = gpGlobals->time + 2.0f;
			pBot->f_waypoint_time = gpGlobals->time + 2.5f;

			// set the healing flag
			pBot->SetTask(TASK_HEALHIM);

			// set treat flag to start the medical treatment
			pBot->SetSubTask(ST_TREAT);

			// reset wait time
			pBot->f_bot_wait_for_enemy_time = gpGlobals->time - 0.1f;

			// use text message to notify your patient
			pBot->BotSpeak(SAY_MEDIC_HELP_YOU, STRING(pWounded->v.netname));

			return TRUE;
		}
	}

	return FALSE;
}


/*
* check if there is any free medic
*/
bool UTIL_CanMedEvac(bot_t *pBot, edict_t *pWounded)
{
	edict_t *pEdict = pBot->pEdict;

	if (pBot->IsTask(TASK_HEALHIM) || (RANDOM_LONG(1, 100) <= 20) ||
		((pBot->pBotUser != nullptr) && (RANDOM_LONG(1, 100) <= 25)))
		return FALSE;

	const int player_team = UTIL_GetTeam(pWounded);
	const int bot_team = UTIL_GetTeam(pEdict);

	if (bot_team != player_team)
		return FALSE;

	// has the bot only last 'keep it for yourself' bandage? AND is the bot able to help a teammate
	if ((pBot->bot_bandages <= 1) || (pBot->sniping_time > gpGlobals->time) ||
		pBot->IsTask(TASK_BIPOD) || pBot->IsTask(TASK_USETANK))
	{
		return FALSE;
	}

	// this bot already knows about him and is trying to treat him
	// there's so no need to search for another one
	if (pBot->pBotEnemy == pWounded)
		return TRUE;

	const float distance = (pEdict->v.origin - pWounded->v.origin).Length();

	// 300 for best bots and goes down with worse skill
	const float sensitivity = 325 - ((pBot->bot_skill + 1) * 25);

	if (distance < sensitivity)
	{
		if ((pBot->pBotEnemy != nullptr) && (pBot->pBotEnemy->v.health < 50) &&
			(RANDOM_LONG(1, 100) > 50))
		{
			return FALSE;
		}

		// check if another bot is going to treat this corpse
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if ((bots[i].is_used) &&
				(pWounded == bots[i].pBotEnemy) && bots[i].IsTask(TASK_HEALHIM))
				return FALSE;
		}

		const Vector v_bothead = pEdict->v.origin + pEdict->v.view_ofs;
		const Vector v_patient = pWounded->v.origin;
		TraceResult tr;

		UTIL_TraceLine(v_bothead, v_patient, ignore_monsters, pEdict, &tr);

		if ((tr.flFraction >= 1.0) ||
			((tr.flFraction >= 0.95) && (strcmp("bodyque", STRING(tr.pHit->v.classname)) == 0)))
		{
			const Vector v_wounded = (v_patient - v_bothead);
			const Vector wounded_angles = UTIL_VecToAngles(v_wounded);

			pEdict->v.ideal_yaw = wounded_angles.y;
			BotFixIdealYaw(pEdict);

			pBot->f_dont_look_for_waypoint_time = gpGlobals->time + 1.0f;
			pBot->f_waypoint_time = gpGlobals->time + 1.0f;

			pBot->pBotEnemy = pWounded;
			pBot->f_bot_see_enemy_time = gpGlobals->time;

			pBot->SetTask(TASK_HEALHIM);
			pBot->SetTask(TASK_MEDEVAC);

			pBot->f_bot_wait_for_enemy_time = gpGlobals->time - 0.1f;

			return TRUE;
		}
	}

	return FALSE;
}


/*
* check if the patient still needs a medical treatment
*/
bool UTIL_PatientNeedsTreatment(edict_t *pPatient)
{
	const int bot_index = UTIL_GetBotIndex(pPatient);

	// ignore the patient if he's under water
	if (pPatient->v.waterlevel == 3)
	{
		return FALSE;
	}

	// this edict is a bot AND is already bandaging self so there's no need to help him
	if ((bot_index != -1) && (bots[bot_index].bandage_time >= gpGlobals->time))
	{
		return FALSE;
	}

	return TRUE;
}


/*
* we must add these methods when we are under linux
* you'll also have to add them when you use different compiler than one of MS VisualC compilers,
* because these methods are MSVC methods
*/
#ifdef __unix__
char *strrev(char *str);
char *strlwr(char *str);

/*
* this method will reverse the string
* ie. string "abc" will become "cba"
*/
char *strrev(char *str) 
{
	int i, j=strlen(str);
	char t;
	if (j==0) { return str; }
	for (i=0, --j; i<j; ++i,--j) 
	{
		t = str[i];
		str[i] = str[j];
		str[j] = t;
	}
	return str;
}


/*
* this method will change the string to all lower case
* ie. "HI There" will be "hi there"
*/
char *strlwr(char *str) 
{
	int len = strlen(str);
	for (int i=0; i<len; ++i)
	{
		str[i] = tolower(str[i]);
	}
	return str;
}
#endif


/*
* returns true if the string has at least 3 chars and doesn't include any of unwanted chars
*/
bool IsStringValid(char *str)
{
	if ((str != nullptr) && (strlen(str) > 3) && (strchr(str, '<') == nullptr) &&
		(strchr(str, '>') == nullptr) && (strchr(str, '{') == nullptr) && (strchr(str, '}') == nullptr) &&
		(strchr(str, '[') == nullptr) && (strchr(str, ']') == nullptr) && (strchr(str, '(') == nullptr) &&
		(strchr(str, ')') == nullptr) && (strchr(str, '+') == nullptr) && (strchr(str, '=') == nullptr) &&
		(strchr(str, '_') == nullptr) && (strchr(str, '|') == nullptr) && (strchr(str, '~') == nullptr) &&
		(strchr(str, '*') == nullptr) && (strchr(str, '/') == nullptr) && (strchr(str, '\\') == nullptr))
		return TRUE;

	return FALSE;
}


/*
* returns a name without various clantags and similar stuff
* (it is usually a small part from the original nick)
*/
void ProcessTheName(char *name)
{
	// check name length and if it is too long try to cut it to something useful
	if ((name != nullptr) && (strlen(name) > 6))
	{
		char temp_name[BOT_NAME_LEN];
		char *seperators = " <>)([]}{+=_~*|/\\";

		// reverse the name first so we can work with it from behind,
		// it will handle the problem with 3+ chars long clantags before names
		// (doesn't work on 100%, however this way has much better results then
		// if we read it from beginning),
		strrev(name);
		
		// cut the first part based on seperators
		char* useful_part = strtok(name, seperators);

		// check if this part is valid name
		if (IsStringValid(useful_part))
		{
			const int len = strlen(useful_part);

			// check name ends and remove anything that isn't an alphabetical letter or digit
			if ((isalnum(useful_part[len-1])) == FALSE)
				useful_part[len-1] = '\0';

			strcpy(temp_name, useful_part);

			if (isalnum(temp_name[0]))
			{
				// finally reverse it back to original reading before returning it
				strcpy(name, strrev(temp_name));
				return;
			}
			else
			{
				strcpy(name, &temp_name[1]);
				strrev(name);
				return;
			}
		}
		// it wasn't valid so try to find something better in the rest of the name
		else
		{
			int safety_stop = 0;
			while ((useful_part = strtok(nullptr, seperators)) != nullptr)
			{
				if (IsStringValid(useful_part))
				{
					const int len = strlen(useful_part);
					
					if ((isalnum(useful_part[len-1])) == FALSE)
						useful_part[len-1] = '\0';
					
					strcpy(temp_name, useful_part);
					
					if (isalnum(temp_name[0]))
					{
						strcpy(name, strrev(temp_name));
						return;
					}
					else
					{
						strcpy(name, &temp_name[1]);
						strrev(name);
						return;
					}
				}

				// if we can't find anything useful then use "you" instead of the name
				if (safety_stop > 10)
				{
					strcpy(name, "you");
					return;
				}

				safety_stop++;
			}

			// there wasn't anything else to cut after the first one
			// so reverse the name back to original reading
			strrev(name);
		}
	}
}


/*
* returns only a few chars from the original name, handles nicks that passed previous
* processing and are still too long (prev. method coudln't handle them much)
*/
void ShortenIt(char *name)
{
	if (((name != nullptr) && (strlen(name) <= 12)) || (name == nullptr))
		return;

	// the name is still too long so cut it to only a few chars
	// and make it low case
	char temp_name[BOT_NAME_LEN];

	strncpy(temp_name, name, 4);
	temp_name[4] = '\0';//																	NEW CODE 094
	strcpy(name, temp_name);
	name[4] = '\0';
	strcat(name, "...");
	strlwr(name);
}


/*
* returns a name that has been shortened and processed in a way so it looks more
* like if human wrote it (ie. without various clantags and similar stuff)
*/
void UTIL_HumanizeTheName(const char* original_name, char *name)
{
	if (original_name != nullptr)
		strncpy(name, original_name, BOT_NAME_LEN);
	
	// just in case something went wrong
	//if (name == NULL)//																NEW CODE 094 (prev code)
	//	strcpy(name, "you");
	if (name[0] == '\0')//																NEW CODE 094
		strcpy(name, "man");

	// remove clantags and pick only a part of the original nick
	ProcessTheName(name);
	// additional check to catch nicks that weren't shortened enough
	ShortenIt(name);
}


/*
* returns true if the waypointer is a member of MB team
*/
bool IsOfficialWpt(const char *waypointer)
{
	if ((strstr(waypointer, "Drek")) || (strstr(waypointer, "Frank McNeil")) ||
		(strstr(waypointer, "Modest_Genius")) || (strstr(waypointer, "Modest Genius")) ||
		(strstr(waypointer, "Seeker")) )
		return TRUE;

	return FALSE;
}


/*
* prints info message to appropriate output using given format
* length of the message is limited
*/
void PrintOutput(edict_t *pEdict, char *message, MType msg_type)
{
	char output[1024];
	extern bool is_dedicated_server;

	// print it into console of this user
	if (pEdict)
	{
		ClientPrint(pEdict, HUD_PRINTNOTIFY, message);
	}
	// print it into dedicated console
	else if (is_dedicated_server)
	{
		switch (msg_type)
		{
			case MType::msg_null:
				sprintf(output, "%s", message);
				break;
			case MType::msg_info:
				sprintf(output, "MARINE_BOT INFO - %s", message);
				break;
			case MType::msg_warning:
				sprintf(output, "MARINE_BOT WARNING - %s", message);
				break;
			case MType::msg_error:
				sprintf(output, "MARINE_BOT ERROR - %s", message);
				break;
			default:
				sprintf(output, "MARINE_BOT - %s", message);
				break;
		}
		
		if (externals.GetIsLogging())
			UTIL_MBLogPrint(output);
		else
		{
			printf(output);
			EchoConsole(output);
		}
	}
	// print it into console while the map loads
	else
	{
		if (externals.GetIsLogging())
			UTIL_MBLogPrint(message);
		else
			ALERT(at_console, message);
	}
}


/*
* prints the message to steam dedicated server console
*/
void EchoConsole(const char *message)
{
	char msg[1024];
	sprintf(msg, "echo %s\n", message);
	SERVER_COMMAND(msg);
}


/*
* works even if the developer is off
*/
void HudNotify(char *msg)
{
#ifdef DEBUG																			// NEW CODE 094 (remove it)

	HudNotify(msg, true);

#else

	
	HudNotify(msg, false);
#endif // DEBUG
}


void HudNotify(char* msg, bot_t* pBot)
{
#ifdef DEBUG																			// NEW CODE 094 (remove it)

	HudNotify(msg, true, pBot);

#else


	HudNotify(msg, false, pBot);
#endif // DEBUG
}


/*
* Overloaded to allow logging in file
*/
void HudNotify(char *msg, bool islogging)
{
	extern edict_t *listenserver_edict;

	if (msg == nullptr)
		return;

#ifdef DEBUG																	// NEW CODE 094 (remove it)

	if (islogging && (gpGlobals->time > 10.0))
	{
		char extram[256];
		//sprintf(extram, "%s\n forcedusage=%d | weaponaction=%d | currWid=%d | currWclip=%d | currWisactive=%d\n",
		//	msg, bots[0].forced_usage, bots[0].weapon_action, bots[0].current_weapon.iId, bots[0].current_weapon.iClip, bots[0].current_weapon.isActive);
		sprintf(extram, "%s", msg);

		UTIL_DebugInFile(extram);
	}

#else


	// write it into file first, because this way if something goes wrong we should have the last event logged in file
	if (islogging)
		UTIL_DebugInFile(msg);


#endif // DEBUG

	// engine printing to hud notify area can only accept 128 characters including the terminating one
	// so we must cut the message here to prevent the overflow
	// otherwise the console won't format the text correctly (new line character is missing)

	const int length = strlen(msg);
	if (length > 127)
	{
		msg[126] = '\n';
		msg[127] = '\0';
	}

	if (listenserver_edict)
		ClientPrint(listenserver_edict, HUD_PRINTNOTIFY, msg);
}


void HudNotify(char *msg, bool islogging, bot_t *pBot)
{
	extern edict_t *listenserver_edict;

	if (msg == nullptr)
		return;

#ifdef DEBUG																	// NEW CODE 094

	extern edict_t* g_debug_bot;
	extern bool g_debug_bot_on;

	// if we are debugging one specific bot and this isn't him
	// then quit and don't print anything
	if (g_debug_bot_on && (g_debug_bot != pBot->pEdict))
		return;


	if (msg != NULL)
	{
		static char last_msg[256] = "";

		if (strcmp(msg, last_msg) != 0)
		{
			char extram[256];
			//sprintf(extram, "%s\n%s\n forcedusage=%d | weaponaction=%d | currWid=%d | currWclip=%d | health=%d | botclass=%d\n",
			//	pBot->name, msg, pBot->forced_usage, pBot->weapon_action, pBot->current_weapon.iId,
			//	pBot->current_weapon.iClip, pBot->bot_health, pBot->bot_class);
			sprintf(extram, "(%s)%s", pBot->name, msg);

			UTIL_DebugInFile(extram);

			strcpy(last_msg, msg);

			int length = strlen(msg);
			if (length > 127)
			{
				msg[126] = '\n';
				msg[127] = '\0';
			}

			if (listenserver_edict)
				ClientPrint(listenserver_edict, HUD_PRINTNOTIFY, msg);
		}
	}

#else


	// write it into file first, because this way if something goes wrong we should have the last event logged in file
	if (islogging)
		UTIL_DebugInFile(msg);

	const int length = strlen(msg);
	if (length > 127)
	{
		msg[126] = '\n';
		msg[127] = '\0';
	}

	if (listenserver_edict)
		ClientPrint(listenserver_edict, HUD_PRINTNOTIFY, msg);

#endif // DEBUG

}



/*
* makes tracelines visible
*/
void UTIL_HighlightTrace(Vector v_source, Vector v_dest, edict_t *pEdict)
{
#ifdef _DEBUG
	extern edict_t *listenserver_edict;

	if (listenserver_edict)
	{
		Vector color;

		// if we have some player then use team colors
		if (pEdict)
		{
			if (UTIL_GetTeam(pEdict) == TEAM_RED)
				color = Vector(255, 50, 50);
			else if (UTIL_GetTeam(pEdict) == TEAM_BLUE)
				color = Vector(50, 50, 255);
		}
		// otherwise use green
		else
			color = Vector(50, 255, 50);

		DrawBeam(listenserver_edict, v_source, v_dest, 200, color.x, color.y, color.z, 150);
	}
#endif
}


/*
* logging in public debug file
*/
void UTIL_DebugInFile(char *msg)
{
	char filename[256];

	UTIL_MarineBotFileName(filename, PUBLIC_DEBUG_FILE, nullptr);

	FILE* f = fopen(filename, "a");
	
	fprintf(f, "***new record(time:%f)(map:%s)***\n", gpGlobals->time, STRING(gpGlobals->mapname));
	fprintf(f, msg);
	fprintf(f, "\n");
#ifndef DEBUG
	fprintf(f, "***record end***\n\n");
#else
	fprintf(f, "\n");
#endif // !DEBUG

	fclose(f);
}


/*
* logging in development debug file
*/
void UTIL_DebugDev(char *msg, int wpt, int path)
{
#ifdef DEBUG

	FILE *f;

	// doesn't the debug file exist yet?
	if (debug_fname[0] == '\0')
	{
		// so build it
		UTIL_MarineBotFileName(debug_fname, "!mb_devdebug.txt", NULL);

		ALERT(at_console, "DevelopmentDebug file was created !!!\n");
	}

	f = fopen(debug_fname, "a");
	
	fprintf(f, "***new record(time:%f)(map:%s)***\n", gpGlobals->time, STRING(gpGlobals->mapname));

	// print only valid messages
	if (msg != NULL)
	{
		fprintf(f, msg);
		fprintf(f, "\n");
	}
	
	if (wpt == -100)
		;
	else if (wpt < -1)
		fprintf(f, " wpt no. (no value)\n");
	else if (wpt == -1)
		fprintf(f, " wpt no. %d (nothing/false)\n", wpt);
	else
		fprintf(f, " wpt no. %d\n", wpt + 1);
	
	if (path == -100)
		;
	else if (path < -1)
		fprintf(f, " path no. (no value)\n");
	else if (path == -1)
		fprintf(f, " path no. %d (nothing/false)\n", path);
	else
		fprintf(f, " path no. %d\n", path + 1);
	
	fprintf(f, "***record end***\n\n");

	fclose(f);

	ALERT(at_console, "\n***EVENT SEND TO DEBUG FILE***\n");

#endif // DEBUG
}


void DumpVector(FILE *f, Vector vec)
{
	fprintf(f, "Vector - x %.3f | y %.3f | z %.3f || Length %.3f\n",
		vec.x, vec.y, vec.z, vec.Length());
}


#ifdef _DEBUG
/*
* dumps all pEdict variables into the development debug file
* variables are in exact order as in progdefs.h
*/
void UTIL_DumpEdictToFile(edict_t *pEdict)
{
	// don't compile it if it isn't needed
	/**/

	FILE *f;

	UTIL_DebugDev("***New dump call***", -100, -100);

	f = fopen(debug_fname, "a");
	
	fprintf(f, "***dump of pEdict(time:%f)***\n", gpGlobals->time);
	fprintf(f, "\n");
	
	if (pEdict->v.classname)
		fprintf(f, "classname %s\n", STRING(pEdict->v.classname));
	if (pEdict->v.globalname)
		fprintf(f, "globalname %s\n", STRING(pEdict->v.globalname));
	fprintf(f, "\n");

	fprintf(f, "origin ");
	DumpVector(f, pEdict->v.origin);
	fprintf(f, "oldorigin ");
	DumpVector(f, pEdict->v.oldorigin);
	fprintf(f, "velocity ");
	DumpVector(f, pEdict->v.velocity);
	fprintf(f, "basevelocity ");
	DumpVector(f, pEdict->v.basevelocity);
	fprintf(f, "clbasevelocity ");
	DumpVector(f, pEdict->v.clbasevelocity);
	fprintf(f, "\n");

	fprintf(f, "movedir ");
	DumpVector(f, pEdict->v.movedir);
	fprintf(f, "\n");

	fprintf(f, "angles ");
	DumpVector(f, pEdict->v.angles);
	fprintf(f, "avelocity ");
	DumpVector(f, pEdict->v.avelocity);
	fprintf(f, "punchangle ");
	DumpVector(f, pEdict->v.punchangle);
	fprintf(f, "v_angle ");
	DumpVector(f, pEdict->v.v_angle);
	fprintf(f, "\n");

	fprintf(f, "endpos ");
	DumpVector(f, pEdict->v.endpos);
	fprintf(f, "startpos ");
	DumpVector(f, pEdict->v.startpos);
	fprintf(f, "impacttime %.3f\n", pEdict->v.impacttime);
	fprintf(f, "starttime %.3f\n", pEdict->v.starttime);
	fprintf(f, "\n");

	fprintf(f, "fixangle %d\n", pEdict->v.fixangle);
	fprintf(f, "idealpitch %.3f\n", pEdict->v.idealpitch);
	fprintf(f, "pitch_speed %.3f\n", pEdict->v.pitch_speed);
	fprintf(f, "ideal_yaw %.3f\n", pEdict->v.ideal_yaw);
	fprintf(f, "yaw_speed %.3f\n", pEdict->v.yaw_speed);
	fprintf(f, "\n");

	fprintf(f, "modelindex %d\n", pEdict->v.modelindex);
	fprintf(f, "model %s\n", STRING(pEdict->v.model));
	fprintf(f, "\n");

	fprintf(f, "viewmodel %d\n", pEdict->v.viewmodel);
	fprintf(f, "weaponmodel %d\n", pEdict->v.weaponmodel);
	fprintf(f, "\n");

	fprintf(f, "absmin ");
	DumpVector(f, pEdict->v.absmin);
	fprintf(f, "absmax ");
	DumpVector(f, pEdict->v.absmax);
	fprintf(f, "mins ");
	DumpVector(f, pEdict->v.mins);
	fprintf(f, "maxs ");
	DumpVector(f, pEdict->v.maxs);
	fprintf(f, "size ");
	DumpVector(f, pEdict->v.size);
	fprintf(f, "\n");

	fprintf(f, "ltime %.3f\n", pEdict->v.ltime);
	fprintf(f, "nextthink %.3f\n", pEdict->v.nextthink);
	fprintf(f, "\n");

	fprintf(f, "movetype %d\n", pEdict->v.movetype);
	fprintf(f, "solid %d\n", pEdict->v.solid);
	fprintf(f, "\n");

	fprintf(f, "skin %d\n", pEdict->v.skin);
	fprintf(f, "body %d\n", pEdict->v.body);
	fprintf(f, "effects %d\n", pEdict->v.effects);
	fprintf(f, "\n");

	fprintf(f, "gravity %.3f\n", pEdict->v.gravity);
	fprintf(f, "friction %.3f\n", pEdict->v.friction);
	fprintf(f, "\n");

	fprintf(f, "light_level %d\n", pEdict->v.light_level);
	fprintf(f, "\n");

	fprintf(f, "sequence %d\n", pEdict->v.sequence);
	fprintf(f, "gaitsequence %d\n", pEdict->v.gaitsequence);
	fprintf(f, "frame %.3f\n", pEdict->v.frame);
	fprintf(f, "animtime %.3f\n", pEdict->v.animtime);
	fprintf(f, "framerate %.3f\n", pEdict->v.framerate);
	fprintf(f, "controller[0] %d\n", pEdict->v.controller[0]);
	fprintf(f, "controller[1] %d\n", pEdict->v.controller[1]);
	fprintf(f, "controller[2] %d\n", pEdict->v.controller[2]);
	fprintf(f, "controller[3] %d\n", pEdict->v.controller[3]);
	fprintf(f, "blending[0] %d\n", pEdict->v.blending[0]);
	fprintf(f, "blending[1] %d\n", pEdict->v.blending[1]);
	fprintf(f, "\n");
	
	fprintf(f, "scale %.3f\n", pEdict->v.scale);
	fprintf(f, "\n");

	fprintf(f, "rendermode %d\n", pEdict->v.rendermode);
	fprintf(f, "renderamt %.3f\n", pEdict->v.renderamt);
	fprintf(f, "rendercolor ");
	DumpVector(f, pEdict->v.rendercolor);
	fprintf(f, "renderfx %d\n", pEdict->v.renderfx);
	fprintf(f, "\n");


	fprintf(f, "health %.3f\n", pEdict->v.health);
	fprintf(f, "frags %.3f\n", pEdict->v.frags);
	fprintf(f, "weapons %d\n", pEdict->v.weapons);
	fprintf(f, "takedamage %.3f\n", pEdict->v.takedamage);
	fprintf(f, "\n");


	fprintf(f, "deadflag %d\n", pEdict->v.deadflag);
	fprintf(f, "view_ofs ");
	DumpVector(f, pEdict->v.view_ofs);
	fprintf(f, "\n");

	fprintf(f, "button %d\n", pEdict->v.button);
	fprintf(f, "impulse %d\n", pEdict->v.impulse);
	fprintf(f, "\n");

	// is exists
	if (pEdict->v.chain)
		fprintf(f, "chain (exists this is its name) %s\n", STRING(pEdict->v.chain->v.classname));
	else
		fprintf(f, "chain does not exist - no data\n");
	if (pEdict->v.dmg_inflictor)
		fprintf(f, "dmg_inflictor (exists this is its name) %s\n", STRING(pEdict->v.dmg_inflictor->v.classname));
	else
		fprintf(f, "dmg_inflictor does not exist - no data\n");
	if (pEdict->v.enemy)
		fprintf(f, "enemy (exists this is its name) %s\n", STRING(pEdict->v.enemy->v.classname));
	else
		fprintf(f, "enemy does not exist - no data\n");
	if (pEdict->v.aiment)
		fprintf(f, "aiment (exists this is its name) %s\n", STRING(pEdict->v.aiment->v.classname));
	else
		fprintf(f, "aiment does not exist - no data\n");
	if (pEdict->v.owner)
		fprintf(f, "owner (exists this is its name) %s\n", STRING(pEdict->v.owner->v.classname));
	else
		fprintf(f, "owner does not exist - no data\n");
	if (pEdict->v.groundentity)
		fprintf(f, "groundentity (exists this is its name) %s\n", STRING(pEdict->v.groundentity->v.classname));
	else
		fprintf(f, "groundentity does not exist - no data\n");
	fprintf(f, "\n");

	fprintf(f, "spawnflags %d\n", pEdict->v.spawnflags);
	fprintf(f, "flags %d\n", pEdict->v.flags);
	fprintf(f, "\n");

	fprintf(f, "colormap %d\n", pEdict->v.colormap);
	fprintf(f, "team %d\n", pEdict->v.team);
	fprintf(f, "\n");

	fprintf(f, "max_health %.3f\n", pEdict->v.max_health);
	fprintf(f, "teleport_time %.3f\n", pEdict->v.teleport_time);
	fprintf(f, "armortype %.3f\n", pEdict->v.armortype);
	fprintf(f, "armorvalue %.3f\n", pEdict->v.armorvalue);
	fprintf(f, "waterlevel %d\n", pEdict->v.waterlevel);
	fprintf(f, "watertype %d\n", pEdict->v.watertype);
	fprintf(f, "\n");

	if (pEdict->v.target)
		fprintf(f, "target %s\n", STRING(pEdict->v.target));
	else
		fprintf(f, "target does not exist - no data\n");
	if (pEdict->v.targetname)
		fprintf(f, "targetname %s\n", STRING(pEdict->v.targetname));
	else
		fprintf(f, "targetname does not exist - no data\n");
	if (pEdict->v.netname)
		fprintf(f, "netname %s\n", STRING(pEdict->v.netname));
	else
		fprintf(f, "netname does not exist - no data\n");
	if (pEdict->v.message)
		fprintf(f, "message %s\n", STRING(pEdict->v.message));
	else
		fprintf(f, "message does not exist - no data\n");
	fprintf(f, "\n");

	fprintf(f, "dmg_take %.3f\n", pEdict->v.dmg_take);
	fprintf(f, "dmg_save %.3f\n", pEdict->v.dmg_save);
	fprintf(f, "dmg %.3f\n", pEdict->v.dmg);
	fprintf(f, "dmgtime %.3f\n", pEdict->v.dmgtime);
	fprintf(f, "\n");

	if (pEdict->v.noise)
		fprintf(f, "noise %s\n", STRING(pEdict->v.noise));
	else
		fprintf(f, "noise does not exist - no data\n");
	if (pEdict->v.noise1)
		fprintf(f, "noise1 %s\n", STRING(pEdict->v.noise1));
	else
		fprintf(f, "noise1 does not exist - no data\n");
	if (pEdict->v.noise2)
		fprintf(f, "noise2 %s\n", STRING(pEdict->v.noise2));
	else
		fprintf(f, "noise2 does not exist - no data\n");
	if (pEdict->v.noise3)
		fprintf(f, "noise3 %s\n", STRING(pEdict->v.noise3));
	else
		fprintf(f, "noise3 does not exist - no data\n");
	fprintf(f, "\n");

	fprintf(f, "speed %.3f\n", pEdict->v.speed);
	fprintf(f, "air_finished %.3f\n", pEdict->v.air_finished);
	fprintf(f, "pain_finished %.3f\n", pEdict->v.pain_finished);
	fprintf(f, "radsuit_finished %.3f\n", pEdict->v.radsuit_finished);
	fprintf(f, "\n");

	if (pEdict->v.pContainingEntity)
		fprintf(f, "pContainingEntity (exists this is its name) %s\n", STRING(pEdict->v.pContainingEntity->v.classname));
	else
		fprintf(f, "pContainingEntity does not exist - no data\n");
	fprintf(f, "\n");

	fprintf(f, "playerclass %d\n", pEdict->v.playerclass);
	fprintf(f, "maxspeed %.3f\n", pEdict->v.maxspeed);
	fprintf(f, "\n");

	fprintf(f, "fov %.3f\n", pEdict->v.fov);
	fprintf(f, "weaponanim %d\n", pEdict->v.weaponanim);
	fprintf(f, "\n");

	fprintf(f, "pushmsec %d\n", pEdict->v.pushmsec);
	fprintf(f, "\n");

	fprintf(f, "bInDuck %d\n", pEdict->v.bInDuck);
	fprintf(f, "flTimeStepSound %d\n", pEdict->v.flTimeStepSound);
	fprintf(f, "flSwimTime %d\n", pEdict->v.flSwimTime);
	fprintf(f, "flDuckTime %d\n", pEdict->v.flDuckTime);
	fprintf(f, "iStepLeft %d\n", pEdict->v.iStepLeft);
	fprintf(f, "flFallVelocity %.3f\n", pEdict->v.flFallVelocity);
	fprintf(f, "\n");

	fprintf(f, "gamestate %d\n", pEdict->v.gamestate);
	fprintf(f, "\n");

	fprintf(f, "oldbuttons %d\n", pEdict->v.oldbuttons);
	fprintf(f, "\n");

	fprintf(f, "groupinfo %d\n", pEdict->v.groupinfo);
	fprintf(f, "\n");

	fprintf(f, "iuser1 %d\n", pEdict->v.iuser1);
	fprintf(f, "iuser2 %d\n", pEdict->v.iuser2);
	fprintf(f, "iuser3 %d\n", pEdict->v.iuser3);
	fprintf(f, "iuser4 %d\n", pEdict->v.iuser4);
	fprintf(f, "fuser1 %.3f\n", pEdict->v.fuser1);
	fprintf(f, "fuser2 %.3f\n", pEdict->v.fuser2);
	fprintf(f, "fuser3 %.3f\n", pEdict->v.fuser3);
	fprintf(f, "fuser4 %.3f\n", pEdict->v.fuser4);
	fprintf(f, "vuser1 ");
	DumpVector(f, pEdict->v.vuser1);
	fprintf(f, "vuser2 ");
	DumpVector(f, pEdict->v.vuser2);
	fprintf(f, "vuser3 ");
	DumpVector(f, pEdict->v.vuser3);
	fprintf(f, "vuser4 ");
	DumpVector(f, pEdict->v.vuser4);
	if (pEdict->v.euser1)
		fprintf(f, "euser1 (exists this is its name) %s\n", STRING(pEdict->v.euser1->v.classname));
	else
		fprintf(f, "euser1 does not exist - no data\n");
	if (pEdict->v.euser2)
		fprintf(f, "euser2 (exists this is its name) %s\n", STRING(pEdict->v.euser2->v.classname));
	else
		fprintf(f, "euser2 does not exist - no data\n");
	if (pEdict->v.euser3)
		fprintf(f, "euser3 (exists this is its name) %s\n", STRING(pEdict->v.euser3->v.classname));
	else
		fprintf(f, "euser3 does not exist - no data\n");
	if (pEdict->v.euser4)
		fprintf(f, "euser4 (exists this is its name) %s\n", STRING(pEdict->v.euser4->v.classname));
	else
		fprintf(f, "euser4 does not exist - no data\n");

	fprintf(f, "***record end***\n\n");
	fclose(f);

	ALERT(at_console, "\n***DONE***\n");

	/**/
}
#endif	// _DEBUG
