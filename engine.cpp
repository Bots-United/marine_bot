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
// (http://www.marinebot.tk)
//
//
// engine.cpp
// 
////////////////////////////////////////////////////////////////////////////////////////////////

#include "defines.h"

#include "extdll.h"
#include "util.h"

#include "bot.h"
#include "bot_client.h"
#include "bot_func.h"
#include "engine.h"


extern enginefuncs_t g_engfuncs;

int debug_engine = 0;
char debug_fname[256];		// allow debugging into any location within HL folder

void (*botMsgFunction)(void *, int) = NULL;
void (*botMsgEndFunction)(void *, int) = NULL;
int botMsgIndex;

// messages created in RegUserMsg which will be "caught"
int message_VGUI = 0;
int message_ShowMenu = 0;
int message_WeaponList = 0;
int message_CurWeapon = 0;
int message_AmmoX = 0;
int message_WeapPickup = 0;
int message_AmmoPickup = 0;
int message_ItemPickup = 0;
int message_Health = 0;
int message_Bandages = 0;
int message_StatusIcon = 0;			// code for FireArms 2.7+
int message_Parachute = 0;		// code for FA 2.65 and versions below
int message_BrokenLeg = 0;		// code for FA 2.65 and versions below
int message_Battery = 0;  // Armor
int message_Damage = 0;
int message_DeathMsg = 0;
int message_TextMsg = 0;
int message_FOV = 0;				// code for FireArms 2.7+
int message_Stamina = 0;			// code for FireArms 2.7+
int message_Reins = 0;				// code for FireArms 2.7+
int message_Concuss = 0;			// code for FA 2.8 and versions above
int message_ScreenFade = 0;


static FILE *fp;


int pfnPrecacheModel(char* s)
{
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnPrecacheModel: %s\n",s); fclose(fp); }
	return (*g_engfuncs.pfnPrecacheModel)(s);
}
int pfnPrecacheSound(char* s)
{
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnPrecacheSound: %s\n",s); fclose(fp); }
	return (*g_engfuncs.pfnPrecacheSound)(s);
}
void pfnSetModel(edict_t *e, const char *m)
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnSetModel: edict=%x %s\n",e,m); fclose(fp); }
	(*g_engfuncs.pfnSetModel)(e, m);
}
int pfnModelIndex(const char *m)
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnModelIndex: %s\n",m); fclose(fp); }
	return (*g_engfuncs.pfnModelIndex)(m);
}
int pfnModelFrames(int modelIndex)
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnModelFrames: %d\n",modelIndex); fclose(fp); }
	return (*g_engfuncs.pfnModelFrames)(modelIndex);
}
void pfnSetSize(edict_t *e, const float *rgflMin, const float *rgflMax)
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnSetSize: %x rgflMin: %f rgflMax: %f\n",e,*rgflMin,*rgflMax); fclose(fp); }
	(*g_engfuncs.pfnSetSize)(e, rgflMin, rgflMax);
}
void pfnChangeLevel(char* s1, char* s2)
{
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnChangeLevel: s1: %s s2: %s\n",s1,s2); fclose(fp); }
	
	// kick any bot off of the server after time/frag limit...
	for (int index = 0; index < MAX_CLIENTS; index++)
	{
		if (bots[index].is_used)  // is this slot used?
		{
			char cmd[40];
			
			sprintf(cmd, "kick \"%s\"\n", bots[index].name);
			bots[index].respawn_state = RESPAWN_NEED_TO_RESPAWN;
			
			SERVER_COMMAND(cmd);  // kick the bot using (kick "name")
		}
	}
	
	(*g_engfuncs.pfnChangeLevel)(s1, s2);
}
void pfnGetSpawnParms(edict_t *ent)
{
#ifdef _DEBUG
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnGetSpawnParms: edict=%x\n",ent); fclose(fp); }
#endif
	(*g_engfuncs.pfnGetSpawnParms)(ent);
}
void pfnSaveSpawnParms(edict_t *ent)
{
#ifdef _DEBUG
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnSaveSpawnParms: edict=%x\n",ent); fclose(fp); }
#endif
   (*g_engfuncs.pfnSaveSpawnParms)(ent);
}
float pfnVecToYaw(const float *rgflVector)
{
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnVecToYaw: rgflVector: %f\n",*rgflVector); fclose(fp); }
   return (*g_engfuncs.pfnVecToYaw)(rgflVector);
}
void pfnVecToAngles(const float *rgflVectorIn, float *rgflVectorOut)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnVecToAngles: rgflVectorIn: %f rgflVectorOut: %f\n",*rgflVectorIn,*rgflVectorOut); fclose(fp); }
   (*g_engfuncs.pfnVecToAngles)(rgflVectorIn, rgflVectorOut);
}
void pfnMoveToOrigin(edict_t *ent, const float *pflGoal, float dist, int iMoveType)
{
#ifdef _DEBUG
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnMoveToOrigin: edict=%x pflGoal: %f dist: %f iMoveType: %d\n",ent,*pflGoal,dist,iMoveType); fclose(fp); }
#endif
   (*g_engfuncs.pfnMoveToOrigin)(ent, pflGoal, dist, iMoveType);
}
void pfnChangeYaw(edict_t* ent)
{
#ifdef _DEBUG
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnChangeYaw: edict=%x\n",ent); fclose(fp); }
#endif
   (*g_engfuncs.pfnChangeYaw)(ent);
}
void pfnChangePitch(edict_t* ent)
{
#ifdef _DEBUG
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnChangePitch: edict=%x\n",ent); fclose(fp); }
#endif
   (*g_engfuncs.pfnChangePitch)(ent);
}
edict_t* pfnFindEntityByString(edict_t *pEdictStartSearchAfter, const char *pszField, const char *pszValue)
{
	if (debug_engine)
	{
		fp=fopen(debug_fname,"a");
		if (pEdictStartSearchAfter && pEdictStartSearchAfter->v.classname)
			fprintf(fp,"pfnFindEntityByString: (ent name: %s) field=%s value=%s\n",
				STRING(pEdictStartSearchAfter->v.classname), pszField, pszValue);
		else if (strcmp(pszValue, "info_firearms_detect") == 0)
			;	// do nothing ie. don't print it into the debugging file
		else
			fprintf(fp,"pfnFindEntityByString: field=%s value=%s\n", pszField, pszValue);
		fclose(fp);
	}
	return (*g_engfuncs.pfnFindEntityByString)(pEdictStartSearchAfter, pszField, pszValue);
}
int pfnGetEntityIllum(edict_t* pEnt)
{
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnGetEntityIllum:\n"); fclose(fp); }
	return (*g_engfuncs.pfnGetEntityIllum)(pEnt);
}
edict_t* pfnFindEntityInSphere(edict_t *pEdictStartSearchAfter, const float *org, float rad)
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnFindEntityInSphere:\n"); fclose(fp); }
	return (*g_engfuncs.pfnFindEntityInSphere)(pEdictStartSearchAfter, org, rad);
}
edict_t* pfnFindClientInPVS(edict_t *pEdict)
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnFindClientInPVS:\n"); fclose(fp); }
	return (*g_engfuncs.pfnFindClientInPVS)(pEdict);
}
edict_t* pfnEntitiesInPVS(edict_t *pplayer)
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnEntitiesInPVS:\n"); fclose(fp); }
	return (*g_engfuncs.pfnEntitiesInPVS)(pplayer);
}
void pfnMakeVectors(const float *rgflVector)
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnMakeVectors:\n"); fclose(fp); }
	(*g_engfuncs.pfnMakeVectors)(rgflVector);
}
void pfnAngleVectors(const float *rgflVector, float *forward, float *right, float *up)
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnAngleVectors:\n"); fclose(fp); }
	(*g_engfuncs.pfnAngleVectors)(rgflVector, forward, right, up);
}
edict_t* pfnCreateEntity(void)
{
	edict_t *pent = (*g_engfuncs.pfnCreateEntity)();
#ifdef _DEBUG
	if (debug_engine)
	{
		fp=fopen(debug_fname,"a");
		if (pent->v.classname != NULL)
			fprintf(fp,"pfnCreateEntity: %x (classname: %s)\n",pent, STRING(pent->v.classname));
		else
			fprintf(fp,"pfnCreateEntity: %x\n",pent);
		fclose(fp);
	}
#endif
	return pent;
}
void pfnRemoveEntity(edict_t* e)
{
#ifdef _DEBUG
	if (debug_engine)
	{
		fp=fopen(debug_fname,"a");
		if (e->v.classname != NULL)
			fprintf(fp,"pfnRemoveEntity: %x (classname: %s)\n",e, STRING(e->v.classname));
		else
			fprintf(fp,"pfnRemoveEntity: %x\n",e);
		if (e->v.model != 0)
			fprintf(fp," model=%s\n", STRING(e->v.model));
		fclose(fp);
	}
#endif
	
	(*g_engfuncs.pfnRemoveEntity)(e);
}
edict_t* pfnCreateNamedEntity(int className)
{
	edict_t *pent = (*g_engfuncs.pfnCreateNamedEntity)(className);
#ifdef _DEBUG
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnCreateNamedEntity: edict=%x name=%s\n",pent,STRING(className)); fclose(fp); }
#endif
	return pent;
}
void pfnMakeStatic(edict_t *ent)
{
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnMakeStatic:\n"); fclose(fp); }
	(*g_engfuncs.pfnMakeStatic)(ent);
}
int pfnEntIsOnFloor(edict_t *e)
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnEntIsOnFloor:\n"); fclose(fp); }
	return (*g_engfuncs.pfnEntIsOnFloor)(e);
}
int pfnDropToFloor(edict_t* e)
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnDropToFloor:\n"); fclose(fp); }
	return (*g_engfuncs.pfnDropToFloor)(e);
}
int pfnWalkMove(edict_t *ent, float yaw, float dist, int iMode)
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnWalkMove:\n"); fclose(fp); }
	return (*g_engfuncs.pfnWalkMove)(ent, yaw, dist, iMode);
}
void pfnSetOrigin(edict_t *e, const float *rgflOrigin)
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnSetOrigin: client=%x origin=%.1f\n", e, rgflOrigin); fclose(fp); }
	(*g_engfuncs.pfnSetOrigin)(e, rgflOrigin);
}
void pfnEmitSound(edict_t *entity, int channel, const char *sample, /*int*/float volume, float attenuation, int fFlags, int pitch)
{
	int index;
	edict_t *pEdict;

	if (gpGlobals->deathmatch)
	{
		//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnEmitSound: edict=%x sound=%s channel=%d volume=%.2f attenuation=%.2f fFlags=%d, pitch=%d\n",entity,sample,channel,volume,attenuation,fFlags,pitch); fclose(fp); }

#ifdef _DEBUG
		//@@@@@@@@
		extern bool debug_sound;



		//@@@@@@@@@@@@@
		if (debug_sound)
		{
			if (debug_engine)
			{
				fp=fopen(debug_fname,"a");
				fprintf(fp,"pfnEmitSound: edict=%x sound=%s channel=%d volume=%.2f attenuation=%.2f fFlags=%d, pitch=%d\n",entity,sample,channel,volume,attenuation,fFlags,pitch);
				fclose(fp);
			}

			ALERT(at_console, "pfnEmitSound: edict=%x sound=%s volume=%.2f\n",
				entity,sample,volume);
		}
#endif




		// did the bot heard a grenade that has been just thrown
		if (strstr(sample, "fuze.wav") != NULL)
		{
			// muted message holds a pointer to the invoker
			if (volume == 0.0)
			{
				for (index=0; index < MAX_CLIENTS; index++)
				{
					if (bots[index].is_used)  // is this slot used?
					{
						pEdict = bots[index].pEdict;
						
						// is it this bot who threw the grenade
						if (entity == pEdict)
						{
							bots[index].BotSpeak(SAY_GRENADE_OUT);

#ifdef _DEBUG
							//@@@@@@@@@@@
							//ALERT(at_console, "%s used a grenade (engine.cpp)\n", bots[index].name);
#endif
							break;
						}
					}
				}
			}
		}

		// is someone bleeding?
		else if (strstr(sample, "bleed.wav") != NULL)
		{
			for (index=0; index < MAX_CLIENTS; index++)
            {
				// is this slot used ie. is this entity a bot?
				if (bots[index].is_used)
				{
					pEdict = bots[index].pEdict;

					// is it this bot who bleeds
					if (entity == pEdict)
					{
						// the bot doesn't bleed any more
						if (volume == 0.0)
							bots[index].RemoveTask(TASK_BLEEDING);

						// the bot bleeds
						if (volume > 0.0)
							bots[index].SetTask(TASK_BLEEDING);

						break;
					}

					// we can/should ignore non-bleeding for the rest of this code
					if (volume == 0.0)
						break;

					// don't listen other clients if passive healing
					// they will have to call for medic if they need help
					if (externals.GetPassiveHealing())
					{
#ifdef _DEBUG
						//@@@@@@@@
						ALERT(at_console, "%s heard bleeding, but ignores it because of passive healing\n",
							bots[index].name);
#endif

						break;
					}

					// if the bot has NOT the needed skills to help skip him
					if (!(bots[index].bot_fa_skill & (FAID | MEDIC)))
						continue;

					// if this medic is NOT free go for next bot
					if (UTIL_IsAnyMedic(&bots[index], entity, TRUE) == FALSE)
						continue;
				}
				// then this entity must be a human player
				else
				{
					// is it this player who bleeds
					if (entity == clients[index].pEntity)
					{
						if (volume == 0.0)
							clients[index].SetBleeding(FALSE);

						if (volume > 0.0)
							clients[index].SetBleeding(TRUE);

						break;
					}
				}
			}
		}

		// skip all muted sounds for the rest of sound messages
		if (volume == 0.0)
		{
		}

		// is someone yelling for a medic?
		else if (strstr(sample, "voice_medic") != NULL)
		{
			for (index=0; index < MAX_CLIENTS; index++)
            {
				if (bots[index].is_used)  // is this slot used?
				{
					// if the bot isn't medic then skip him
					if (!(bots[index].bot_fa_skill & (FAID | MEDIC)))
						continue;

					// if this medic is NOT free go for next bot
					if (UTIL_IsAnyMedic(&bots[index], entity, FALSE) == FALSE)
						continue;
				}
            }
		}

		// is any medic close AND this bot is bleeding?
		else if ((strstr(sample, "voice_medhere.wav") != NULL) ||
			(strstr(sample, "voice_treatyou.wav") != NULL))
		{
			for (index=0; index < MAX_CLIENTS; index++)
            {
				if (bots[index].is_used)  // is this slot used?
				{
					float distance = (bots[index].pEdict->v.origin - entity->v.origin).Length();

					// don't react if this bot is too far from the medic
					if (distance > (550 - ((bots[index].bot_skill + 1) * 50)))
						continue;

					int player_team = UTIL_GetTeam(entity);
					int bot_team = UTIL_GetTeam(bots[index].pEdict);

					// don't react on enemy medics
					if (bot_team != player_team)
						continue;

					// does this bot bleed and the bot doesn't bandage himself,
					// pause for a while to allow the medic to get close
					if (bots[index].IsTask(TASK_BLEEDING) &&
						(bots[index].bandage_time <= gpGlobals->time))
					{
						bots[index].f_pause_time = gpGlobals->time + RANDOM_LONG(2.5, 5.0);


#ifdef _DEBUG
						//@@@@@@@@
						ALERT(at_console, "%s heard that medic is coming and will react\n",
							bots[index].name);
#endif


					}
				}
			}
		}

		// is there a downed soldier yelling for help?
		else if (strstr(sample, "gr_pain") != NULL)
		{
			for (index=0; index < MAX_CLIENTS; index++)
            {
				// exist this bot and is it time for him to react on this event
				if (bots[index].is_used) //&&
					//(bots[index].f_look_for_ground_items <= gpGlobals->time))
				{
					if (!(bots[index].bot_fa_skill & (FAID | MEDIC)))
						continue;

					// once we find suitable bot break the loop
					if (UTIL_CanMedEvac(&bots[index], entity))
						break;
				}
			}
		}

		// does someone call for cover?
		else if (strstr(sample, "voice_coverme.wav") != NULL)
		{
			for (index=0; index < MAX_CLIENTS; index++)
            {
				if (bots[index].is_used)  // is this slot used?
				{
					pEdict = bots[index].pEdict;

					// skip "yourself"
					if (pEdict == entity)
						continue;

					// is this bot already "used" so ignore the call
					if (bots[index].pBotUser != NULL)
						continue;

					// can this bot hear the call
					if (UTIL_HeardIt(&bots[index], entity, 400))
					{
						int counter = 0;

						// check how many "followers" has this "user"
						for (int i = 0; i < 31; i++)
						{
							if (bots[i].pBotUser == entity)
								counter++;
						}

						// follow this "user" only if doesn't have too many "followers"
						if (counter < 2)
						{
							bots[index].pBotUser = entity;
							// set use time to know that this bot has been just "used"
							bots[index].f_bot_use_time = gpGlobals->time;
							
							UTIL_Radio(pEdict, "yes");
							bots[index].speak_time = gpGlobals->time;


#ifdef _DEBUG
						//@@@@@@@@
						ALERT(at_console, "%s heard that and will react\n", bots[index].name);
#endif


						}
						// say no, this "user" already has enough "followers"
						else
						{
							UTIL_Radio(pEdict, "no");
							bots[index].speak_time = gpGlobals->time;
						}
					}
				}
			}
		}
	}

	(*g_engfuncs.pfnEmitSound)(entity, channel, sample, volume, attenuation, fFlags, pitch);
}
void pfnEmitAmbientSound(edict_t *entity, float *pos, const char *samp, float vol, float attenuation, int fFlags, int pitch)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnEmitAmbientSound:\n"); fclose(fp); }
   (*g_engfuncs.pfnEmitAmbientSound)(entity, pos, samp, vol, attenuation, fFlags, pitch);
}
void pfnTraceLine(const float *v1, const float *v2, int fNoMonsters, edict_t *pentToSkip, TraceResult *ptr)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnTraceLine:\n"); fclose(fp); }
   (*g_engfuncs.pfnTraceLine)(v1, v2, fNoMonsters, pentToSkip, ptr);
}
void pfnTraceToss(edict_t* pent, edict_t* pentToIgnore, TraceResult *ptr)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnTraceToss:\n"); fclose(fp); }
   (*g_engfuncs.pfnTraceToss)(pent, pentToIgnore, ptr);
}
int pfnTraceMonsterHull(edict_t *pEdict, const float *v1, const float *v2, int fNoMonsters, edict_t *pentToSkip, TraceResult *ptr)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnTraceMonsterHull:\n"); fclose(fp); }
   return (*g_engfuncs.pfnTraceMonsterHull)(pEdict, v1, v2, fNoMonsters, pentToSkip, ptr);
}
void pfnTraceHull(const float *v1, const float *v2, int fNoMonsters, int hullNumber, edict_t *pentToSkip, TraceResult *ptr)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnTraceHull:\n"); fclose(fp); }
   (*g_engfuncs.pfnTraceHull)(v1, v2, fNoMonsters, hullNumber, pentToSkip, ptr);
}
void pfnTraceModel(const float *v1, const float *v2, int hullNumber, edict_t *pent, TraceResult *ptr)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnTraceModel:\n"); fclose(fp); }
   (*g_engfuncs.pfnTraceModel)(v1, v2, hullNumber, pent, ptr);
}
const char *pfnTraceTexture(edict_t *pTextureEntity, const float *v1, const float *v2 )
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnTraceTexture:\n"); fclose(fp); }
   return (*g_engfuncs.pfnTraceTexture)(pTextureEntity, v1, v2);
}
void pfnTraceSphere(const float *v1, const float *v2, int fNoMonsters, float radius, edict_t *pentToSkip, TraceResult *ptr)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnTraceSphere:\n"); fclose(fp); }
   (*g_engfuncs.pfnTraceSphere)(v1, v2, fNoMonsters, radius, pentToSkip, ptr);
}
void pfnGetAimVector(edict_t* ent, float speed, float *rgflReturn)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnGetAimVector:\n"); fclose(fp); }
   (*g_engfuncs.pfnGetAimVector)(ent, speed, rgflReturn);
}
void pfnServerCommand(char* str)
{
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnServerCommand: %s\n",str); fclose(fp); }
   (*g_engfuncs.pfnServerCommand)(str);
}
void pfnServerExecute(void)
{
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnServerExecute:\n"); fclose(fp); }
   (*g_engfuncs.pfnServerExecute)();
}
void pfnClientCommand(edict_t* pEdict, char* szFmt, ...)
{
	if (debug_engine)
	{
		// print this also to console
		ALERT(at_console, "pfnClientCommand=%s\n",szFmt);
		
		fp=fopen(debug_fname,"a");
		fprintf(fp,"pfnClientCommand=%s\n",szFmt);
		fclose(fp);
	}
	
	// new code to test if this finally fix the problem with bot using the say and teamsay cmds
	static char tempFmt[1024];
	va_list argp;
	va_start(argp, szFmt);
	vsprintf(tempFmt, szFmt, argp);
	va_end(argp);
	
	if (!(pEdict->v.flags & FL_FAKECLIENT))
		(*g_engfuncs.pfnClientCommand)(pEdict, tempFmt);
	
	return;
}
void pfnParticleEffect(const float *org, const float *dir, float color, float count)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnParticleEffect:\n"); fclose(fp); }
   (*g_engfuncs.pfnParticleEffect)(org, dir, color, count);
}
void pfnLightStyle(int style, char* val)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnLightStyle:\n"); fclose(fp); }
   (*g_engfuncs.pfnLightStyle)(style, val);
}
int pfnDecalIndex(const char *name)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnDecalIndex:\n"); fclose(fp); }
   return (*g_engfuncs.pfnDecalIndex)(name);
}
int pfnPointContents(const float *rgflVector)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnPointContents:\n"); fclose(fp); }
   return (*g_engfuncs.pfnPointContents)(rgflVector);
}
void pfnMessageBegin(int msg_dest, int msg_type, const float *pOrigin, edict_t *ed)
{
	if (gpGlobals->deathmatch)
	{
		int index = -1;

#ifdef _DEBUG
		if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnMessageBegin: edict=%x dest=%d type=%d\n",ed,msg_dest,msg_type); fclose(fp); }
#endif
		
		if (ed)
		{
			index = UTIL_GetBotIndex(ed);

#ifdef _DEBUG
			if (index == -1)
			{
				if (msg_type == message_CurWeapon)
				{
					botMsgFunction = NULL;     // no msg function until known otherwise
					botMsgEndFunction = NULL;  // no msg end function until known otherwise
					botMsgIndex = index;       // index of bot receiving message
					
					if (msg_type == message_CurWeapon)
						botMsgFunction = BotClient_FA_CurrentWeapon;
				}
			}
#endif

			// is this message for a bot?
			if (index != -1)
			{
				botMsgFunction = NULL;     // no msg function until known otherwise
				botMsgEndFunction = NULL;  // no msg end function until known otherwise
				botMsgIndex = index;       // index of bot receiving message
				
				if (mod_id == FIREARMS_DLL)
				{
					if (msg_type == message_VGUI)
						botMsgFunction = BotClient_FA_VGUI;
					else if (msg_type == message_WeaponList)
						botMsgFunction = BotClient_FA_WeaponList;
					else if (msg_type == message_CurWeapon)
						botMsgFunction = BotClient_FA_CurrentWeapon;
					else if (msg_type == message_AmmoX)
						botMsgFunction = BotClient_FA_AmmoX;
					else if (msg_type == message_AmmoPickup)
						botMsgFunction = BotClient_FA_AmmoPickup;
					else if (msg_type == message_WeapPickup)
						botMsgFunction = BotClient_FA_WeaponPickup;
					else if (msg_type == message_ItemPickup)
						botMsgFunction = BotClient_FA_ItemPickup;
					else if (msg_type == message_Health)
						botMsgFunction = BotClient_FA_Health;
					else if (msg_type == message_Bandages)
						botMsgFunction = BotClient_FA_Bandages;
					else if (msg_type == message_StatusIcon)
						botMsgFunction = BotClient_FA_StatusIcon;
					else if (msg_type == message_Parachute)	// code for FA 2.65 and versions below
					{
						botMsgFunction = BotClient_FA_Parachute;

						// ugly technique to handle this message
						if (botMsgFunction)
							(*botMsgFunction)(static_cast<void *>(NULL), botMsgIndex);
					}
					else if (msg_type == message_BrokenLeg)	// code for FA 2.65 and versions below
						botMsgFunction = BotClient_FA_BrokenLeg;
					else if (msg_type == message_Battery)
						botMsgFunction = BotClient_FA_Battery;
					else if (msg_type == message_Damage)
						botMsgFunction = BotClient_FA_Damage;
					else if (msg_type == message_TextMsg)
						botMsgFunction = BotClient_FA_TextMsg;
					else if (msg_type == message_FOV)
						botMsgFunction = BotClient_FA_FOV;
					else if (msg_type == message_Stamina)
						botMsgFunction = BotClient_FA_Stamina;
					else if (msg_type == message_Reins)
						botMsgFunction = BotClient_FA_Reins;
					else if (msg_type == message_Concuss)
						botMsgFunction = BotClient_FA_Concuss;
					else if (msg_type == message_ScreenFade)
						botMsgFunction = BotClient_FA_ScreenFade;
					else if (msg_type == 23)
					{
						botMsgFunction = BotClient_FA_HUDMsg;
					}
				}
			}
		}
		else if (msg_dest == MSG_ALL)
		{
			botMsgFunction = NULL;  // no msg function until known otherwise
			botMsgIndex = -1;       // index of bot receiving message (none)
			
			if (mod_id == FIREARMS_DLL)
			{
				if (msg_type == message_DeathMsg)
					botMsgFunction = BotClient_FA_DeathMsg;
				else if (msg_type == message_TextMsg)
					botMsgFunction = BotClient_FA_TextMsg_ForAll;
			}
		}
	}
	
	(*g_engfuncs.pfnMessageBegin)(msg_dest, msg_type, pOrigin, ed);
}
void pfnMessageEnd(void)
{
	if (gpGlobals->deathmatch)
	{
		if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnMessageEnd:\n"); fclose(fp); }

		if (botMsgEndFunction)
			(*botMsgEndFunction)(NULL, botMsgIndex);  // NULL indicated msg end
		
		// clear out the bot message function pointers...
		botMsgFunction = NULL;
		botMsgEndFunction = NULL;
	}
	
	(*g_engfuncs.pfnMessageEnd)();
}
void pfnWriteByte(int iValue)
{
	if (gpGlobals->deathmatch)
	{
		if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnWriteByte: %d\n",iValue); fclose(fp); }
		
		// if this message is for a bot, call the client message function...
		if (botMsgFunction)
			(*botMsgFunction)(static_cast<void *>(&iValue), botMsgIndex);
	}
	
	(*g_engfuncs.pfnWriteByte)(iValue);
}
void pfnWriteChar(int iValue)
{
	if (gpGlobals->deathmatch)
	{
		if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnWriteChar: %d\n",iValue); fclose(fp); }
		
		// if this message is for a bot, call the client message function...
		if (botMsgFunction)
			(*botMsgFunction)(static_cast<void *>(&iValue), botMsgIndex);
	}
	
	(*g_engfuncs.pfnWriteChar)(iValue);
}
void pfnWriteShort(int iValue)
{
	if (gpGlobals->deathmatch)
	{
		if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnWriteShort: %d\n",iValue); fclose(fp); }

		// if this message is for a bot, call the client message function...
		if (botMsgFunction)
			(*botMsgFunction)(static_cast<void *>(&iValue), botMsgIndex);
	}
	
	(*g_engfuncs.pfnWriteShort)(iValue);
}
void pfnWriteLong(int iValue)
{
	if (gpGlobals->deathmatch)
	{
		if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnWriteLong: %d\n",iValue); fclose(fp); }
		
		// if this message is for a bot, call the client message function...
		if (botMsgFunction)
			(*botMsgFunction)(static_cast<void *>(&iValue), botMsgIndex);
	}
	
	(*g_engfuncs.pfnWriteLong)(iValue);
}
void pfnWriteAngle(float flValue)
{
	if (gpGlobals->deathmatch)
	{
		if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnWriteAngle: %f\n",flValue); fclose(fp); }
		
		// if this message is for a bot, call the client message function...
		if (botMsgFunction)
			(*botMsgFunction)(static_cast<void *>(&flValue), botMsgIndex);
	}
	
	(*g_engfuncs.pfnWriteAngle)(flValue);
}
void pfnWriteCoord(float flValue)
{
	if (gpGlobals->deathmatch)
	{
		if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnWriteCoord: %f\n",flValue); fclose(fp); }
		
		// if this message is for a bot, call the client message function...
		if (botMsgFunction)
			(*botMsgFunction)(static_cast<void *>(&flValue), botMsgIndex);
	}

	(*g_engfuncs.pfnWriteCoord)(flValue);
}
void pfnWriteString(const char *sz)
{
	if (gpGlobals->deathmatch)
	{
		if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnWriteString: %s\n",sz); fclose(fp); }
	
		// if this message is for a bot, call the client message function...
		if (botMsgFunction)
			(*botMsgFunction)((void *)sz, botMsgIndex);	
	}
	
	(*g_engfuncs.pfnWriteString)(sz);
}
void pfnWriteEntity(int iValue)
{
	if (gpGlobals->deathmatch)
	{
		if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnWriteEntity: %d\n",iValue); fclose(fp); }
		
		// if this message is for a bot, call the client message function...
		if (botMsgFunction)
			(*botMsgFunction)(static_cast<void *>(&iValue), botMsgIndex);
	}
	
	(*g_engfuncs.pfnWriteEntity)(iValue);
}
void pfnCVarRegister(cvar_t *pCvar)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnCVarRegister:\n"); fclose(fp); }
   (*g_engfuncs.pfnCVarRegister)(pCvar);
}
float pfnCVarGetFloat(const char *szVarName)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnCVarGetFloat: %s\n",szVarName); fclose(fp); }
   return (*g_engfuncs.pfnCVarGetFloat)(szVarName);
}
const char* pfnCVarGetString(const char *szVarName)
{
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnCVarGetString:\n"); fclose(fp); }
   return (*g_engfuncs.pfnCVarGetString)(szVarName);
}
void pfnCVarSetFloat(const char *szVarName, float flValue)
{
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnCVarSetFloat:\n"); fclose(fp); }
   (*g_engfuncs.pfnCVarSetFloat)(szVarName, flValue);
}
void pfnCVarSetString(const char *szVarName, const char *szValue)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnCVarSetString: varName=%s value=%s\n", szVarName, szValue); fclose(fp); }
   (*g_engfuncs.pfnCVarSetString)(szVarName, szValue);
}

void* pfnPvAllocEntPrivateData(edict_t *pEdict, int32 cb)

{
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnPvAllocEntPrivateData:\n"); fclose(fp); }
   return (*g_engfuncs.pfnPvAllocEntPrivateData)(pEdict, cb);
}
void* pfnPvEntPrivateData(edict_t *pEdict)
{
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnPvEntPrivateData:\n"); fclose(fp); }
	return (*g_engfuncs.pfnPvEntPrivateData)(pEdict);
}
void pfnFreeEntPrivateData(edict_t *pEdict)
{
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnFreeEntPrivateData:\n"); fclose(fp); }
	(*g_engfuncs.pfnFreeEntPrivateData)(pEdict);
}
const char* pfnSzFromIndex(int iString)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnSzFromIndex:\n"); fclose(fp); }
   return (*g_engfuncs.pfnSzFromIndex)(iString);
}
int pfnAllocString(const char *szValue)
{
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnAllocString:\n"); fclose(fp); }
   return (*g_engfuncs.pfnAllocString)(szValue);
}
entvars_t* pfnGetVarsOfEnt(edict_t *pEdict)
{
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnGetVarsOfEnt:\n"); fclose(fp); }
   return (*g_engfuncs.pfnGetVarsOfEnt)(pEdict);
}
edict_t* pfnPEntityOfEntOffset(int iEntOffset)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnPEntityOfEntOffset:\n"); fclose(fp); }
   return (*g_engfuncs.pfnPEntityOfEntOffset)(iEntOffset);
}
int pfnEntOffsetOfPEntity(const edict_t *pEdict)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnEntOffsetOfPEntity: %x\n",pEdict); fclose(fp); }
   return (*g_engfuncs.pfnEntOffsetOfPEntity)(pEdict);
}
int pfnIndexOfEdict(const edict_t *pEdict)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnIndexOfEdict: %x\n",pEdict); fclose(fp); }
   return (*g_engfuncs.pfnIndexOfEdict)(pEdict);
}
edict_t* pfnPEntityOfEntIndex(int iEntIndex)
{
#ifdef _DEBUG
	edict_t* pent = (*g_engfuncs.pfnPEntityOfEntIndex)(iEntIndex);
	
	/*/			// gets called really often
	if (debug_engine)
	{
		fp=fopen(debug_fname,"a");
		if (pent != NULL)
		{
			if (pent->v.classname != NULL)
			{
				if (pent->v.netname != NULL)
					fprintf(fp,"pfnPEntityOfEntIndex: %x (classname: %s) (netname: %s)\n",
						pent, STRING(pent->v.classname), STRING(pent->v.netname));
				else
					fprintf(fp,"pfnPEntityOfEntIndex: %x (classname: %s)\n",pent, STRING(pent->v.classname));
			}
			else
				fprintf(fp,"pfnPEntityOfEntIndex: %x\n",pent);
		}
		else
			// index 32 is the last one and is always NULL 
			fprintf(fp,"pfnPEntityOfEntIndex: pent must be NULL see its iEntIndex: %d\n", iEntIndex);
		fclose(fp);
	}
	/**/
	return pent;
#else
	return (*g_engfuncs.pfnPEntityOfEntIndex)(iEntIndex);
#endif
}
edict_t* pfnFindEntityByVars(entvars_t* pvars)
{
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnFindEntityByVars:\n"); fclose(fp); }
   return (*g_engfuncs.pfnFindEntityByVars)(pvars);
}
void* pfnGetModelPtr(edict_t* pEdict)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnGetModelPtr: %x\n",pEdict); fclose(fp); }
   return (*g_engfuncs.pfnGetModelPtr)(pEdict);
}
int pfnRegUserMsg(const char *pszName, int iSize)
{
   int msg;

   msg = (*g_engfuncs.pfnRegUserMsg)(pszName, iSize);

   if (gpGlobals->deathmatch)
   {
#ifdef _DEBUG
	  if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnRegUserMsg: pszName=%s msg=%d\n",pszName,msg); fclose(fp); }
#endif

      if (mod_id == FIREARMS_DLL)
      {
         if (strcmp(pszName, "VGUIMenu") == 0)
            message_VGUI = msg;
         else if (strcmp(pszName, "WeaponList") == 0)
            message_WeaponList = msg;
         else if (strcmp(pszName, "CurWeapon") == 0)
            message_CurWeapon = msg;
         else if (strcmp(pszName, "AmmoX") == 0)
            message_AmmoX = msg;
         else if (strcmp(pszName, "AmmoPickup") == 0)
            message_AmmoPickup = msg;
         else if (strcmp(pszName, "WeapPickup") == 0)
            message_WeapPickup = msg;
         else if (strcmp(pszName, "ItemPickup") == 0)
            message_ItemPickup = msg;
         else if (strcmp(pszName, "Health") == 0)
            message_Health = msg;
		 else if (strcmp(pszName, "Bandage") == 0)
			 message_Bandages = msg;
		 else if (strcmp(pszName, "StatusIcon") == 0)
			 message_StatusIcon = msg;
		 else if (strcmp(pszName, "Parachute") == 0)	// code for FA 2.65 and versions below
			 message_Parachute = msg;
		 else if (strcmp(pszName, "BrokenLeg") == 0)	// code for FA 2.65 and versions below
			 message_BrokenLeg = msg;
         else if (strcmp(pszName, "Battery") == 0)
            message_Battery = msg;
         else if (strcmp(pszName, "Damage") == 0)
            message_Damage = msg;
         else if (strcmp(pszName, "DeathMsg") == 0)
            message_DeathMsg = msg;
		 else if (strcmp(pszName, "TextMsg") == 0)
			 message_TextMsg = msg;
		 else if (strcmp(pszName, "SetFOV") == 0)
			 message_FOV = msg;
		 else if (strcmp(pszName, "Stamina") == 0)
			 message_Stamina = msg;
		 else if (strcmp(pszName, "Reins") == 0)
			 message_Reins = msg;
		 else if (strcmp(pszName, "Concuss") == 0)		// code for FA 2.8 and versions above
			 message_Concuss = msg;
         else if (strcmp(pszName, "ScreenFade") == 0)
            message_ScreenFade = msg;
      }
   }

   return msg;
}
void pfnAnimationAutomove(const edict_t* pEdict, float flTime)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnAnimationAutomove:\n"); fclose(fp); }
   (*g_engfuncs.pfnAnimationAutomove)(pEdict, flTime);
}
void pfnGetBonePosition(const edict_t* pEdict, int iBone, float *rgflOrigin, float *rgflAngles )
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnGetBonePosition:\n"); fclose(fp); }
   (*g_engfuncs.pfnGetBonePosition)(pEdict, iBone, rgflOrigin, rgflAngles);
}
uint32 pfnFunctionFromName( const char *pName )
{
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnFunctionFromName:\n"); fclose(fp); }

   return (*g_engfuncs.pfnFunctionFromName)(pName);
}
const char *pfnNameForFunction( uint32 function )
{
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnNameForFunction:\n"); fclose(fp); }
   
   return (*g_engfuncs.pfnNameForFunction)(function);
}
void pfnClientPrintf( edict_t* pEdict, PRINT_TYPE ptype, const char *szMsg )
{
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnClientPrintf: %s\n", szMsg); fclose(fp); }

   // don't print it for nonexistent clients
   if (pEdict == NULL)
	   return;

   (*g_engfuncs.pfnClientPrintf)(pEdict, ptype, szMsg);
}
void pfnServerPrint( const char *szMsg )
{
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnServerPrint: %s\n",szMsg); fclose(fp); }
   (*g_engfuncs.pfnServerPrint)(szMsg);
}
void pfnGetAttachment(const edict_t *pEdict, int iAttachment, float *rgflOrigin, float *rgflAngles )
{
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnGetAttachment: %d\n", iAttachment); fclose(fp); }
   (*g_engfuncs.pfnGetAttachment)(pEdict, iAttachment, rgflOrigin, rgflAngles);
}
void pfnCRC32_Init(CRC32_t *pulCRC)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnCRC32_Init:\n"); fclose(fp); }
   (*g_engfuncs.pfnCRC32_Init)(pulCRC);
}
void pfnCRC32_ProcessBuffer(CRC32_t *pulCRC, void *p, int len)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnCRC32_ProcessBuffer:\n"); fclose(fp); }
   (*g_engfuncs.pfnCRC32_ProcessBuffer)(pulCRC, p, len);
}
void pfnCRC32_ProcessByte(CRC32_t *pulCRC, unsigned char ch)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnCRC32_ProcessByte:\n"); fclose(fp); }
   (*g_engfuncs.pfnCRC32_ProcessByte)(pulCRC, ch);
}
CRC32_t pfnCRC32_Final(CRC32_t pulCRC)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnCRC32_Final:\n"); fclose(fp); }
   return (*g_engfuncs.pfnCRC32_Final)(pulCRC);
}
int32 pfnRandomLong(int32 lLow, int32 lHigh)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnRandomLong: lLow=%d lHigh=%d\n",lLow,lHigh); fclose(fp); }
   return (*g_engfuncs.pfnRandomLong)(lLow, lHigh);
}
float pfnRandomFloat(float flLow, float flHigh)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnRandomFloat:\n"); fclose(fp); }
   return (*g_engfuncs.pfnRandomFloat)(flLow, flHigh);
}
void pfnSetView(const edict_t *pClient, const edict_t *pViewent )
{
#ifdef _DEBUG
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnSetView: pClient %x pViewent %x\n", pClient, pViewent); fclose(fp); }
#endif

	// NOTE: This seems to finally FIXED the bloody trigger_camera crash on obj_armory
	//		 Although I'm not sure whether this won't affect things like HLTV.

	if (pClient != NULL)
	{
		if (UTIL_GetBotIndex((edict_t *)pClient) != -1)
		{
			//fp=fopen(debug_fname,"a");
			//fprintf(fp,"pfnSetView(): pClient is NOT a real human player. It could be a BOT -> not passing!\n");
			//fclose(fp);

			return;
		}
	}

	(*g_engfuncs.pfnSetView)(pClient, pViewent);
}
float pfnTime( void )
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnTime:\n"); fclose(fp); }
   return (*g_engfuncs.pfnTime)();
}
void pfnCrosshairAngle(const edict_t *pClient, float pitch, float yaw)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnCrosshairAngle:\n"); fclose(fp); }
   (*g_engfuncs.pfnCrosshairAngle)(pClient, pitch, yaw);
}
byte *pfnLoadFileForMe(char *filename, int *pLength)
{
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnLoadFileForMe: filename=%s\n",filename); fclose(fp); }
   return (*g_engfuncs.pfnLoadFileForMe)(filename, pLength);
}
void pfnFreeFile(void *buffer)
{
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnFreeFile:\n"); fclose(fp); }
   (*g_engfuncs.pfnFreeFile)(buffer);
}
void pfnEndSection(const char *pszSectionName)
{
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnEndSection:\n"); fclose(fp); }
   (*g_engfuncs.pfnEndSection)(pszSectionName);
}
int pfnCompareFileTime(char *filename1, char *filename2, int *iCompare)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnCompareFileTime:\n"); fclose(fp); }
   return (*g_engfuncs.pfnCompareFileTime)(filename1, filename2, iCompare);
}
void pfnGetGameDir(char *szGetGameDir)
{
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnGetGameDir:\n"); fclose(fp); }
   (*g_engfuncs.pfnGetGameDir)(szGetGameDir);
}
void pfnCvar_RegisterVariable(cvar_t *variable)
{
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnCvar_RegisterVariable:\n"); fclose(fp); }
   (*g_engfuncs.pfnCvar_RegisterVariable)(variable);
}
void pfnFadeClientVolume(const edict_t *pEdict, int fadePercent, int fadeOutSeconds, int holdTime, int fadeInSeconds)
{
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnFadeClientVolume:\n"); fclose(fp); }
   (*g_engfuncs.pfnFadeClientVolume)(pEdict, fadePercent, fadeOutSeconds, holdTime, fadeInSeconds);
}
void pfnSetClientMaxspeed(const edict_t *pEdict, float fNewMaxspeed)
{
#ifdef _DEBUG
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnSetClientMaxspeed: edict=%x %f\n",pEdict,fNewMaxspeed); fclose(fp); }


   //@@@@@@@@@@@@@22
   //ALERT(at_console, "pfnSetClientMaxspeed: edict=%x speed=%.1f\n",pEdict,fNewMaxspeed);
#endif

   
   // NOTE: ugly technique to get bot know that the round has ended
   // this needs to be changed because it results in false round end
   // (it sets TRUE also when bot is killed)
   // however there seems to be completely no message or something that would tell us
   // that the round has ended in FA
   if (gpGlobals->deathmatch)
   {
      if (mod_id == FIREARMS_DLL)
      {
		  int index;

		  index = UTIL_GetBotIndex((edict_t *) pEdict);

		  // is this edict a bot?
		  if (index != -1)
		  {
			  // set round end flag for correct respawn
			  bots[index].round_end = TRUE;
		  }

		  index = 0;
		  while ((index < MAX_CLIENTS) && (clients[index].pEntity != pEdict))
			index++;

		  if (index < MAX_CLIENTS)
		  {
			  // store the time when this client was "out of the game"
			  clients[index].SetMaxSpeedTime(gpGlobals->time);
		  }
      }
   }

   (*g_engfuncs.pfnSetClientMaxspeed)(pEdict, fNewMaxspeed);
}
edict_t * pfnCreateFakeClient(const char *netname)
{
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnCreateFakeClient:\n"); fclose(fp); }
   return (*g_engfuncs.pfnCreateFakeClient)(netname);
}
void pfnRunPlayerMove(edict_t *fakeclient, const float *viewangles, float forwardmove, float sidemove, float upmove, unsigned short buttons, byte impulse, byte msec )
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnRunPlayerMove:\n"); fclose(fp); }
   (*g_engfuncs.pfnRunPlayerMove)(fakeclient, viewangles, forwardmove, sidemove, upmove, buttons, impulse, msec);
}
int pfnNumberOfEntities(void)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnNumberOfEntities:\n"); fclose(fp); }
   return (*g_engfuncs.pfnNumberOfEntities)();
}
char* pfnGetInfoKeyBuffer(edict_t *e)
{
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnGetInfoKeyBuffer:\n"); fclose(fp); }
   return (*g_engfuncs.pfnGetInfoKeyBuffer)(e);
}
char* pfnInfoKeyValue(char *infobuffer, char *key)
{
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnInfoKeyValue: %s %s\n",infobuffer,key); fclose(fp); }
   return (*g_engfuncs.pfnInfoKeyValue)(infobuffer, key);
}
void pfnSetKeyValue(char *infobuffer, char *key, char *value)
{
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnSetKeyValue: %s %s\n",key,value); fclose(fp); }
   (*g_engfuncs.pfnSetKeyValue)(infobuffer, key, value);
}
void pfnSetClientKeyValue(int clientIndex, char *infobuffer, char *key, char *value)
{
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnSetClientKeyValue: %s %s\n",key,value); fclose(fp); }
   (*g_engfuncs.pfnSetClientKeyValue)(clientIndex, infobuffer, key, value);
}
int pfnIsMapValid(char *filename)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnIsMapValid:\n"); fclose(fp); }
   return (*g_engfuncs.pfnIsMapValid)(filename);
}
void pfnStaticDecal( const float *origin, int decalIndex, int entityIndex, int modelIndex )
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnStaticDecal:\n"); fclose(fp); }
   (*g_engfuncs.pfnStaticDecal)(origin, decalIndex, entityIndex, modelIndex);
}
int pfnPrecacheGeneric(char* s)
{
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnPrecacheGeneric: %s\n",s); fclose(fp); }
   return (*g_engfuncs.pfnPrecacheGeneric)(s);
}
int pfnGetPlayerUserId(edict_t *e )
{
#ifdef _DEBUG
   if (gpGlobals->deathmatch)
   {
      if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnGetPlayerUserId: %x\n",e); fclose(fp); }
   }
#endif

   return (*g_engfuncs.pfnGetPlayerUserId)(e);
}
const char *pfnGetPlayerAuthId (edict_t *e)
{
#ifdef _DEBUG
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnGetPlayerAuthId: %x\n",e); fclose(fp); }
#endif
   
   if (e->v.flags & FL_FAKECLIENT)
	   return "BOT";

   return (*g_engfuncs.pfnGetPlayerAuthId)(e);
}
void pfnBuildSoundMsg(edict_t *entity, int channel, const char *sample, /*int*/float volume, float attenuation, int fFlags, int pitch, int msg_dest, int msg_type, const float *pOrigin, edict_t *ed)
{
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnBuildSoundMsg:\n"); fclose(fp); }

	// test though this shouldn't cause any harm
	if (entity->v.flags & FL_FAKECLIENT)
		return;

	(*g_engfuncs.pfnBuildSoundMsg)(entity, channel, sample, volume, attenuation, fFlags, pitch, msg_dest, msg_type, pOrigin, ed);
}
int pfnIsDedicatedServer(void)
{
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnIsDedicatedServer:\n"); fclose(fp); }
   return (*g_engfuncs.pfnIsDedicatedServer)();
}
cvar_t* pfnCVarGetPointer(const char *szVarName)
{
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnCVarGetPointer: %s\n",szVarName); fclose(fp); }
   return (*g_engfuncs.pfnCVarGetPointer)(szVarName);
}
unsigned int pfnGetPlayerWONId(edict_t *e)
{
#ifdef _DEBUG
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnGetPlayerWONId: %x\n",e); fclose(fp); }
#endif

   if (e->v.flags & FL_FAKECLIENT)
	   return 0;

   return (*g_engfuncs.pfnGetPlayerWONId)(e);
}


// new stuff for SDK 2.0

void pfnInfo_RemoveKey(char *s, const char *key)
{
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnInfo_RemoveKey:\n"); fclose(fp); }
   (*g_engfuncs.pfnInfo_RemoveKey)(s, key);
}
const char *pfnGetPhysicsKeyValue(const edict_t *pClient, const char *key)
{
#ifdef _DEBUG
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnGetPhysicsKeyValue: %x %s\n", pClient, key); fclose(fp); }
#endif
   return (*g_engfuncs.pfnGetPhysicsKeyValue)(pClient, key);
}
void pfnSetPhysicsKeyValue(const edict_t *pClient, const char *key, const char *value)
{
#ifdef _DEBUG
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnSetPhysicsKeyValue: client=%x key=%s value=%s\n", pClient, key, value); fclose(fp); }
#endif
   (*g_engfuncs.pfnSetPhysicsKeyValue)(pClient, key, value);
}
const char *pfnGetPhysicsInfoString(const edict_t *pClient)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnGetPhysicsInfoString: %x\n", pClient); fclose(fp); }
   return (*g_engfuncs.pfnGetPhysicsInfoString)(pClient);
}
unsigned short pfnPrecacheEvent(int type, const char *psz)
{
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnPrecacheEvent: %s\n", psz); fclose(fp); }
   return (*g_engfuncs.pfnPrecacheEvent)(type, psz);
}
void pfnPlaybackEvent(int flags, const edict_t *pInvoker, unsigned short eventindex, float delay,
   float *origin, float *angles, float fparam1,float fparam2, int iparam1, int iparam2, int bparam1, int bparam2)
{
#ifdef _DEBUG
   if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnPlaybackEvent: flags=%d invoker=%x index=%d\n", flags, pInvoker, eventindex); fclose(fp); }
#endif
   (*g_engfuncs.pfnPlaybackEvent)(flags, pInvoker, eventindex, delay, origin, angles, fparam1, fparam2, iparam1, iparam2, bparam1, bparam2);
}
unsigned char *pfnSetFatPVS(float *org)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnSetFatPVS:\n"); fclose(fp); }
   return (*g_engfuncs.pfnSetFatPVS)(org);
}
unsigned char *pfnSetFatPAS(float *org)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnSetFatPAS:\n"); fclose(fp); }
   return (*g_engfuncs.pfnSetFatPAS)(org);
}
int pfnCheckVisibility(const edict_t *entity, unsigned char *pset)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnCheckVisibility:\n"); fclose(fp); }
   return (*g_engfuncs.pfnCheckVisibility)(entity, pset);
}
void pfnDeltaSetField(struct delta_s *pFields, const char *fieldname)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnDeltaSetField:\n"); fclose(fp); }
   (*g_engfuncs.pfnDeltaSetField)(pFields, fieldname);
}
void pfnDeltaUnsetField(struct delta_s *pFields, const char *fieldname)
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnDeltaUnsetField:\n"); fclose(fp); }
   (*g_engfuncs.pfnDeltaUnsetField)(pFields, fieldname);
}
void pfnDeltaAddEncoder(char *name, void (*conditionalencode)( struct delta_s *pFields, const unsigned char *from, const unsigned char *to))
{
   //if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnDeltaAddEncoder:\n"); fclose(fp); }
   (*g_engfuncs.pfnDeltaAddEncoder)(name, conditionalencode);
}
int pfnGetCurrentPlayer(void)
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnGetCurrentPlayer:\n"); fclose(fp); }
	return (*g_engfuncs.pfnGetCurrentPlayer)();
}
int pfnCanSkipPlayer(const edict_t *player)
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnCanSkipPlayer: player: %x\n", player); fclose(fp); }
	return (*g_engfuncs.pfnCanSkipPlayer)(player);
}
int pfnDeltaFindField(struct delta_s *pFields, const char *fieldname)
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnDeltaFindField:\n"); fclose(fp); }
	return (*g_engfuncs.pfnDeltaFindField)(pFields, fieldname);
}
void pfnDeltaSetFieldByIndex(struct delta_s *pFields, int fieldNumber)
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnDeltaSetFieldByIndex:\n"); fclose(fp); }
	(*g_engfuncs.pfnDeltaSetFieldByIndex)(pFields, fieldNumber);
}
void pfnDeltaUnsetFieldByIndex(struct delta_s *pFields, int fieldNumber)
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnDeltaUnsetFieldByIndex:\n"); fclose(fp); }
	(*g_engfuncs.pfnDeltaUnsetFieldByIndex)(pFields, fieldNumber);
}
void pfnSetGroupMask(int mask, int op)
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnSetGroupMask:\n"); fclose(fp); }
	(*g_engfuncs.pfnSetGroupMask)(mask, op);
}
int pfnCreateInstancedBaseline(int classname, struct entity_state_s *baseline)
{
	//if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnCreateInstancedBaseline:\n"); fclose(fp); }
	return (*g_engfuncs.pfnCreateInstancedBaseline)(classname, baseline);
}
void pfnCvar_DirectSet(struct cvar_s *var, char *value)
{
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnCvar_DirectSet:\n"); fclose(fp); }
	(*g_engfuncs.pfnCvar_DirectSet)(var, value);
}
void pfnForceUnmodified(FORCE_TYPE type, float *mins, float *maxs, const char *filename)
{
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnForceUnmodified:\n"); fclose(fp); }
	(*g_engfuncs.pfnForceUnmodified)(type, mins, maxs, filename);
}
void pfnGetPlayerStats(const edict_t *pClient, int *ping, int *packet_loss)
{
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnGetPlayerStats:\n"); fclose(fp); }
	(*g_engfuncs.pfnGetPlayerStats)(pClient, ping, packet_loss);
}
void pfnAddServerCommand(char *cmd_name, void (*function)(void))
{
#ifdef _DEBUG
	if (debug_engine) { fp=fopen(debug_fname,"a"); fprintf(fp,"pfnAddServerCommand: %s %x\n",cmd_name,function); fclose(fp); }
#endif
	(*g_engfuncs.pfnAddServerCommand)(cmd_name, function);
}

