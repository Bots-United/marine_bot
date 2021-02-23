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
// bot_client.cpp
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
#include "bot_client.h"
#include "bot_weapons.h"

// types of damage to ignore
#define IGNORE_DAMAGE (DMG_CRUSH | DMG_FREEZE | DMG_FALL | DMG_SHOCK | \
                       DMG_NERVEGAS | DMG_RADIATION | DMG_ACID | DMG_SLOWBURN | \
                       DMG_SLOWFREEZE | 0xFF000000)

bot_weapon_t weapon_defs[MAX_WEAPONS]; // array of weapon definitions

/*
* this message is sent when the Firearms VGUI menu is displayed
*/
void BotClient_FA_VGUI(void *p, int bot_index)
{
	static int state = 0;   // current state machine state
	static int iValue1;		// the first byte is the most important value in vgui message

	// we need to process the whole message first
	if (state == 0)
	{
		state++;
		iValue1 = *(int *)p;	// store this away we need it
	}
	// additionals two values, but we don't use/need them now
	else if (state == 1)
	{
		state++;				// just skip it
	}
	else if (state == 2)
	{
		state = 0;				// reset the counter
	}

	// now process the important values...

	// handle Firearms 2.9 MOTD window confirmation
	if ((g_mod_version == FA_29) || (g_mod_version == FA_30))
	{
		if (iValue1 == 19)
			bots[bot_index].start_action = MSG_FA_MOTD_WINDOW;
	}

	// do we spawn a bot in Firearms 2.5 and below?
	if (UTIL_IsOldVersion())
	{
		if (iValue1 == 1)
			bots[bot_index].start_action = MSG_FA25_BASE_ARMOR_SELECT;
		else if (iValue1 == 2)
			bots[bot_index].start_action = MSG_FA25_ARMOR_SELECT;
		else if (iValue1 == 3)
			bots[bot_index].start_action = MSG_FA25_ITEMS_SELECT;
		else if (iValue1 == 4)
			bots[bot_index].start_action = MSG_FA25_SIDEARMS_SELECT;
		else if (iValue1 == 5)
			bots[bot_index].start_action = MSG_FA25_SHOTGUNS_SELECT;
		else if (iValue1 == 6)
			bots[bot_index].start_action = MSG_FA25_SMGS_SELECT;
		else if (iValue1 == 7)
			bots[bot_index].start_action = MSG_FA25_RIFLES_SELECT;
		else if (iValue1 == 8)
			bots[bot_index].start_action = MSG_FA25_SNIPES_SELECT;
		else if (iValue1 == 9)
			bots[bot_index].start_action = MSG_FA25_MGUNS_SELECT;
		else if (iValue1 == 10)
			bots[bot_index].start_action = MSG_FA25_GLS_SELECT;
	}
	
	// is it a join the game confirmation?
	if (iValue1 == 12)
		bots[bot_index].start_action = MSG_FA_INTO_BATTLE;
	//is it team select menu?
	else if (iValue1 == 13)
		bots[bot_index].start_action = MSG_FA_TEAM_SELECT;
	// is it a class selection menu?
	else if (iValue1 == 14)
		bots[bot_index].start_action = MSG_FA_CLASS_SELECT;
	// is it a skill selection menu?
	// (it's also displayed after a round end if we have enough points to take new skill)
	else if (iValue1 == 20)
		bots[bot_index].start_action = MSG_FA_SKILL_SELECT;
		
		
	//NOTE: This one was probably removed or changed to the one above (==12)
	// is it a join the game confirmation?
	else if (iValue1 == 22)
		bots[bot_index].start_action = MSG_FA_INTO_BATTLE;
}

/*
* this message is sent when a client joins the game
* all of the weapons are sent with the weapon ID and information about what ammo is used
*/
void BotClient_FA_WeaponList(void *p, int bot_index)
{
	static int state = 0;   // current state machine state
	static bot_weapon_t bot_weapon;

	if (state == 0)
	{
		state++;
		strcpy(bot_weapon.szClassname, (char *)p);
	}
	else if (state == 1)
	{
		state++;
		bot_weapon.iAmmo1 = *(int *)p;  // ammo index 1
	}
	else if (state == 2)
	{
		state++;
		bot_weapon.iAmmo1Max = *(int *)p;  // max ammo1
	}
	else if (state == 3)
	{
		state++;
		bot_weapon.iAmmo2 = *(int *)p;  // ammo index 2
	}
	else if (state == 4)
	{
		state++;
		bot_weapon.iAmmo2Max = *(int *)p;  // max ammo2
	}
	else if (state == 5)
	{
		state++;
		bot_weapon.iSlot = *(int *)p;  // slot for this weapon
	}
	else if (state == 6)
	{
		state++;
		bot_weapon.iPosition = *(int *)p;  // position in slot
	}
	else if (state == 7)
	{
		state++;
		bot_weapon.iId = *(int *)p;  // weapon ID
	}
	else if (state == 8)
	{
		state = 0;

		bot_weapon.iFlags = *(int *)p;  // flags for weapon (???)

		// store away this weapon with it's ammo information
		weapon_defs[bot_weapon.iId] = bot_weapon;
	}
}

/*
* this message is sent when a weapon is selected (either by the bot choosing
* a weapon or by the server auto assigning the bot a weapon)
*/
void BotClient_FA_CurrentWeapon(void *p, int bot_index)
{
	static int state = 0;   // current state machine state
	static int iState;
	static int iId;
	static int iClip;
	static int iAttach;
	static int iFireMode;

	if (state == 0)
	{
		state++;
		iState = *(int *)p;  // state of the current weapon
	}
	else if (state == 1)
	{
		state++;
		iId = *(int *)p;  // weapon ID of current weapon
	}
	else if (state == 2)
	{
		state++;

		iClip = *(int *)p;  // ammo currently in the clip for this weapon
	}
	// probably secondary fire handler
	// if stealth mount supressor (0-normal, 1-silenced)
	// if arty ammo2 currently in chamber (0-empty 1-loaded)
	else if (state == 3)
	{
		state++;

		iAttach = *(int *)p;
	}
	// fire mode handler code
	else if (state == 4)
	{
		state = 0;

		iFireMode = *(int *)p;	// 4-auto, 2-burst, 1-single, 0-on weapon where no choice

		if (iId <= 31)
		{
			if (bot_index != -1)
			{
				bots[bot_index].bot_weapons |= (1<<iId);  // set this weapon bit
				
				if (iState == 1)
				{
					bots[bot_index].current_weapon.isActive = iState;
					bots[bot_index].current_weapon.iId = iId;
					bots[bot_index].current_weapon.iClip = iClip;
					bots[bot_index].current_weapon.iAttachment = iAttach;
					bots[bot_index].current_weapon.iFireMode = iFireMode;

					// update the ammo counts for this weapon
					bots[bot_index].current_weapon.iAmmo1 =
						bots[bot_index].curr_rgAmmo[weapon_defs[iId].iAmmo1];
					bots[bot_index].current_weapon.iAmmo2 =
						bots[bot_index].curr_rgAmmo[weapon_defs[iId].iAmmo2];
				}
			}
		}
	}
}

/*
* this message is sent whenever ammo ammounts are adjusted (up or down)
*/
void BotClient_FA_AmmoX(void *p, int bot_index)
{
	static int state = 0;   // current state machine state
	static int index;
	static int ammount;

	if (state == 0)
	{
		state++;
		index = *(int *)p;  // ammo index (for type of ammo)
	}
	else if (state == 1)
	{
		state = 0;

		ammount = *(int *)p;  // the ammount of ammo currently available

		bots[bot_index].curr_rgAmmo[index] = ammount;  // store it away

		const int ammo_index = bots[bot_index].current_weapon.iId;

		// update the ammo counts for this weapon
		bots[bot_index].current_weapon.iAmmo1 =
			bots[bot_index].curr_rgAmmo[weapon_defs[ammo_index].iAmmo1];
		bots[bot_index].current_weapon.iAmmo2 =
			bots[bot_index].curr_rgAmmo[weapon_defs[ammo_index].iAmmo2];
	}
}

/*
* this message is sent when the bot picks up some ammo (AmmoX messages are
* also sent so this message is probably not really necessary except it
* allows the HUD to draw pictures of ammo that have been picked up
* the bots don't really need pictures since they don't have any eyes anyway
*/
void BotClient_FA_AmmoPickup(void *p, int bot_index)
{
	static int state = 0;   // current state machine state
	static int index;
	static int ammount;

	if (state == 0)
	{
		state++;
		index = *(int *)p;
	}
	else if (state == 1)
	{
		state = 0;

		ammount = *(int *)p;

		bots[bot_index].curr_rgAmmo[index] = ammount;

		const int ammo_index = bots[bot_index].current_weapon.iId;

		// update the ammo counts for this weapon
		bots[bot_index].current_weapon.iAmmo1 =
			bots[bot_index].curr_rgAmmo[weapon_defs[ammo_index].iAmmo1];
		bots[bot_index].current_weapon.iAmmo2 =
			bots[bot_index].curr_rgAmmo[weapon_defs[ammo_index].iAmmo2];
	}
}

/*
*																			NEW CODE 094
* this message gets sent when the bot picks up a weapon
*/
void BotClient_FA_WeaponPickup(void *p, int bot_index)
{
	const int index = *(int*)p;

#ifdef DEBUG

	//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@	NEW CODE 094 (remove it)
	//char dmsg[256];
	//sprintf(dmsg, "FA weap pickup() -> index =<%d>\n", index);
	//UTIL_DebugInFile(dmsg);
	//ALERT(at_console, dmsg);


#endif // DEBUG

	
	// set this weapon bit to indicate that we are carrying this weapon
	bots[bot_index].bot_weapons |= (1 << index);

	if (bots[bot_index].bot_weapons & (1 << fa_weapon_claymore))
	{
		if (bots[bot_index].claymore_slot == NO_VAL)
			bots[bot_index].claymore_slot = fa_weapon_claymore;

		if (bots[bot_index].f_use_clay_time != 0.0)
			bots[bot_index].f_use_clay_time = 0.0;

		if (bots[bot_index].clay_action != ALTW_NOTUSED)
			bots[bot_index].clay_action = ALTW_NOTUSED;
	}

	if (bots[bot_index].bot_weapons & (1 << fa_weapon_frag))
	{
		if (bots[bot_index].grenade_slot == NO_VAL)
			bots[bot_index].grenade_slot = fa_weapon_frag;

		if (bots[bot_index].grenade_time != 0.0)
			bots[bot_index].grenade_time = 0.0;

		if (bots[bot_index].grenade_action != ALTW_NOTUSED)
			bots[bot_index].grenade_action = ALTW_NOTUSED;

		// if the bot picked this grenade up while already in game then let him check for other grenades he may carry
		if (bots[bot_index].IsSubTask(ST_NOOTHERNADE))
			bots[bot_index].RemoveSubTask(ST_NOOTHERNADE);
	}

	if (bots[bot_index].bot_weapons & (1 << fa_weapon_concussion))
	{
		if (bots[bot_index].grenade_slot == NO_VAL)
			bots[bot_index].grenade_slot = fa_weapon_concussion;

		if (bots[bot_index].grenade_time != 0.0)
			bots[bot_index].grenade_time = 0.0;

		if (bots[bot_index].grenade_action != ALTW_NOTUSED)
			bots[bot_index].grenade_action = ALTW_NOTUSED;

		if (bots[bot_index].IsSubTask(ST_NOOTHERNADE))
			bots[bot_index].RemoveSubTask(ST_NOOTHERNADE);
	}

	if (bots[bot_index].bot_weapons & (1 << fa_weapon_flashbang))
	{
		if (bots[bot_index].grenade_slot == NO_VAL)
			bots[bot_index].grenade_slot = fa_weapon_flashbang;

		if (bots[bot_index].grenade_time != 0.0)
			bots[bot_index].grenade_time = 0.0;

		if (bots[bot_index].grenade_action != ALTW_NOTUSED)
			bots[bot_index].grenade_action = ALTW_NOTUSED;

		if (bots[bot_index].IsSubTask(ST_NOOTHERNADE))
			bots[bot_index].RemoveSubTask(ST_NOOTHERNADE);
	}

	if (bots[bot_index].bot_weapons & (1 << fa_weapon_stg24))
	{
		if (bots[bot_index].grenade_slot == NO_VAL)
			bots[bot_index].grenade_slot = fa_weapon_stg24;

		if (bots[bot_index].grenade_time != 0.0)
			bots[bot_index].grenade_time = 0.0;

		if (bots[bot_index].grenade_action != ALTW_NOTUSED)
			bots[bot_index].grenade_action = ALTW_NOTUSED;

		if (bots[bot_index].IsSubTask(ST_NOOTHERNADE))
			bots[bot_index].RemoveSubTask(ST_NOOTHERNADE);
	}
}

/*
* this message gets sent when the bot picks up an item (like a battery
* or a healthkit)
*/
void BotClient_FA_ItemPickup(void *p, int bot_index)
{
   // do nothing
}

/*
* this message gets sent when bots health changes
*/
void BotClient_FA_Health(void *p, int bot_index)
{
	bots[bot_index].bot_health = *(int *)p;  // health ammount
}

/*
* this message is sent when bot gets, picks up or uses bandages
*/
void BotClient_FA_Bandages(void *p, int bot_index)
{
	bots[bot_index].bot_bandages = *(int *)p;  // bandages ammount
}

/*
* this message is sent when various events happens
* like picking up a parachute, using a bipod on weapons etc.
* (ie. all that are shown on hud as an icon)
*/
void BotClient_FA_StatusIcon(void *p, int bot_index)
{
	static int state = 0;   // current state machine state
	static char event_type[64];

	if (state == 0)
	{
		state++;
	}
	else if (state == 1)
	{
		state++;

		strcpy(event_type, (char *)p);
	}
	else if (state == 2)
	{
		state++;
	}
	else if (state == 3)
	{
		state++;
		
		if (*(int *)p == 255)
		{
			if (strcmp(event_type, "parachute") == 0)
			{
				bots[bot_index].SetTask(TASK_PARACHUTE);		// bot just picked up a parachute
			}
			else if (strcmp(event_type, "bipod") == 0)
			{
				bots[bot_index].SetTask(TASK_BIPOD);			// bipod used
				bots[bot_index].SetBehaviour(BOT_DONTGOPRONE);	// bipod prevents changing stance so we can't go prone



#ifdef DEBUG
				//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@											// NEW CODE 094 (remove it)
				char dm[256];
				sprintf(dm, "(bot_client.cpp) %s got BIPOD DISPLAYED message and SET DONTGOPRONE behaviour!\n", bots[bot_index].name);
				//ALERT(at_console, dm); 
				UTIL_DebugInFile(dm);
				//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#endif // DEBUG



			}
			// NOTE: No use for this yet
			else if (strcmp(event_type, "dmg_conc") == 0)
			{
				;	// concussion grenade stun
			}
		}
		else if (*(int *)p == 0)
		{
			// parachute was thrown off so reset variables to allow bot to use another parachute if needed
			if (strcmp(event_type, "parachute") == 0)
			{
				bots[bot_index].RemoveTask(TASK_PARACHUTE);
				bots[bot_index].RemoveTask(TASK_PARACHUTE_USED);
				bots[bot_index].chute_action_time = 0.0;
			}
			else if (strcmp(event_type, "bipod") == 0)
			{
				bots[bot_index].RemoveTask(TASK_BIPOD);
				bots[bot_index].RemoveBehaviour(BOT_DONTGOPRONE);	// now we are free to go/resume prone again




#ifdef DEBUG
				//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@											// NEW CODE 094 (remove it)
				char dm[256];
				sprintf(dm, "(bot_client.cpp) %s got BIPOD HIDE message and removed DONTGOPRONE behaviour!\n", bots[bot_index].name);
				//ALERT(at_console, dm); 
				UTIL_DebugInFile(dm);
				//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#endif // DEBUG



			}
			else if (strcmp(event_type, "dmg_conc") == 0)
			{
				;
			}
		}
	}
	else if (state == 4)
	{
		state = 0;

		if (*(int *)p == 128)
		{
			if (strcmp(event_type, "gotitem") == 0)
			{
				bots[bot_index].SetTask(TASK_GOALITEM);	// the bot carries a goal item now


#ifdef _DEBUG
				//@@@@@@@@@@@
				//ALERT(at_console,"***bot picked up the goal item\n");
#endif
			}
		}
		else if (*(int *)p == 0)
		{
			if (strcmp(event_type, "gotitem") == 0)
			{
				bots[bot_index].RemoveTask(TASK_GOALITEM);	// the bot got it to goal area


#ifdef _DEBUG
				//@@@@@@@@@@@
				//ALERT(at_console,"***bot finished the goal item action (captured it)\n");
#endif
			}
		}
	}
}

/*
* BACKWARD COMPATIBILITY code for FA 2.65 and versions below
* this message gets send when the bot has a parachute
* it displays that small parachute icon on the screen
*/
void BotClient_FA_Parachute(void *p, int bot_index)
{
	// set this task only if we aren't trying to drop the parachute when back on ground
	if (!bots[bot_index].IsNeed(NEED_RESETPARACHUTE))
		bots[bot_index].SetTask(TASK_PARACHUTE);
}

/*
* BACKWARD COMPATIBILITY code for FA 2.65 and versions below
* this message is being send all the time 
* first two bits are team reinforcements and the 3rd bit is current stamina value
*/
void BotClient_FA_BrokenLeg(void *p, int bot_index)
{
	static int state = 0;   // current state machine state
	
	if (state == 0)
	{
		state++;

		if (*(int *)p == 0)
		{
			botmanager.SetOverrideTeamsBalance(true);
		}
	}
	else if (state == 1)
	{
		state++;

		if (*(int *)p == 0)
		{
			botmanager.SetOverrideTeamsBalance(true);
		}
	}
	else if (state == 2)
	{
		state = 0;

		if (*(int *)p <= 4)
		{
			bots[bot_index].RemoveTask(TASK_SPRINT);	// no sprint is allowed
		}
		else if (*(int *)p <= 15)
		{
			bots[bot_index].SetTask(TASK_NOJUMP);		// no jump is allowed
		}
		else if (*(int *)p >= 35)
		{
			bots[bot_index].RemoveTask(TASK_NOJUMP);	// the bot can jump again
		}
	}
}

/*
* this message gets sent when the bots armor changes
*/
void BotClient_FA_Battery(void *p, int bot_index)
{
	bots[bot_index].bot_armor = *(int *)p;  // armor ammount
}

/*
* this message gets sent when the bots are getting damaged
*/
void BotClient_FA_Damage(void *p, int bot_index)
{
	static int state = 0;   // current state machine state
	static int damage_armor;
	static int damage_taken;
	static int damage_bits;  // type of damage being done
	static Vector damage_origin;

	if (state == 0)
	{
		state++;
		damage_armor = *(int *)p;
	}
	else if (state == 1)
	{
		state++;
		damage_taken = *(int *)p;
	}
	else if (state == 2)
	{
		state++;
		damage_bits = *(int *)p;
	}
	else if (state == 3)
	{
		state++;
		damage_origin.x = *(float *)p;
	}
	else if (state == 4)
	{
		state++;
		damage_origin.y = *(float *)p;
	}
	else if (state == 5)
	{
		state = 0;

		// ignore all types of damage if the bot is in "I'm ignoring all" mode
		if (botdebugger.IsIgnoreAll())
			return;

		damage_origin.z = *(float *)p;

		if ((damage_armor > 0) || (damage_taken > 0))
		{
			// NOTE: In FA 3.0 bleeding seems to have new value (1<<25) so as int value it's 33554432
			//			I'm not using it, because it doesn't seem to cause any unwanted turn around

			// ignore bleeding hurt etc. (ie do not try to face it)
			// we won't facing this hurt, because bot does stupid turn arounds
			if (damage_bits == 2048)	// i.e. (1<<11)
				return;
			
			// if bot fall off something and is hurt store new health value
			// to prevent false bandage treatment and ignore
			if (damage_bits & DMG_FALL)
			{
				bots[bot_index].bot_prev_health = bots[bot_index].bot_health;

				return;
			}

			// set the drowing flag to know that the bot is drowing
			// we have to handle FA2.9 changes
			if ((damage_bits & DMG_DROWN) || ((damage_bits & DMG_ALWAYSGIB) &&
				((g_mod_version == FA_29) || (g_mod_version == FA_30))))
			{
				bots[bot_index].SetTask(TASK_DROWNING);

				// we also have to handle false bahaviour
				// the bot doesn't bleed even if he's losing health and
				// the engine is sending bleeding sound
				bots[bot_index].RemoveTask(TASK_BLEEDING);

				return;
			}

			// reset the drowing flag to know that the bot is no more drowing
			// this works only in FA2.8 and lower versions but not in FA2.9
			// FA2.9 doesn't send this bit
			if (damage_bits & DMG_DROWNRECOVER)
			{
				bots[bot_index].RemoveTask(TASK_DROWNING);

				return;
			}

			// ignore certain types of damage
			if (damage_bits & IGNORE_DAMAGE)
				return;

			edict_t *pDmgEnt = bots[bot_index].pEdict->v.dmg_inflictor;

			// is the bot being hurt by this trigger then ignore it for now
			// prevents the bot to "dance" on such object
			//
			// NOTE: This should be changed to allow the bot to evade such object
			if ((pDmgEnt != nullptr) && (strcmp(STRING(pDmgEnt->v.classname), "trigger_hurt") == 0))
				return;

			// react only on damage taken from another player's gunfire
			// at least for now
			if ((pDmgEnt != nullptr) && (strcmp(STRING(pDmgEnt->v.classname), "player") == 0))
			{
				const int enemy_team = UTIL_GetTeam(bots[bot_index].pEdict->v.dmg_inflictor);
				const int bot_team = UTIL_GetTeam(bots[bot_index].pEdict);
				
				// try to warn the teammate who's shooting at you
				if ((enemy_team == bot_team) && (RANDOM_LONG(1, 100) <= 35) &&		// was 25
					IsAlive(bots[bot_index].pEdict))
				{
					bots[bot_index].SetSubTask(ST_SAY_CEASEFIRE);
				}
			}

			float new_enemy_dist;

			// get the vector to new enemy
			const Vector v_new_enemy = damage_origin - bots[bot_index].pEdict->v.origin;

			// get the distance to new enemy
			new_enemy_dist = v_new_enemy.Length();

			// if the bot already has an enemy
			if (bots[bot_index].pBotEnemy)
			{
				// get the distance for current enemy
				const float curr_enemy_dist = (bots[bot_index].pBotEnemy->v.origin -
					bots[bot_index].pEdict->v.origin).Length();

				// is current enemy closer than the new enemy AND does the bot see him
				// then ignore new enemy
				if ((curr_enemy_dist < new_enemy_dist) &&
					(bots[bot_index].f_bot_wait_for_enemy_time < gpGlobals->time))
					return;

				// the new enemy must be closer than current enemy so...

				// is the bot sniper AND the new enemy is too close keep the current enemy
				// (ie. bot isn't able to change to gun that fast so it's better if he try to do
				// as many damage to the current enemy as he can)
				//if ((bots[bot_index].bot_behaviour & SNIPER) && (enemy_dist < 400))			NEW CODE 094 (prev code)
				if ((bots[bot_index].bot_behaviour & SNIPER) && (new_enemy_dist < 200) &&
					(bots[bot_index].f_bot_wait_for_enemy_time < gpGlobals->time))
					return;

				// if the bot has clear view at current enemy AND
				// the bot probably already shoot at current enemy than
				// in most of the time keep current enemy
				if ((bots[bot_index].f_bot_wait_for_enemy_time < gpGlobals->time) &&
					(bots[bot_index].pBotEnemy->v.health < 50) && (RANDOM_LONG(1, 100) <= 90))
					return;

				// the bot decided to target the new enemy so forget the current one
				bots[bot_index].BotForgetEnemy();
			}
			// it's a completely new enemy or the bot switched on this enemy so...

			if (bots[bot_index].IsTask(TASK_GOALITEM) && (new_enemy_dist > ENEMY_DIST_GOALITEM))
				return;
			
			// face the attacker (ie. the new enemy)
			const Vector bot_angles = UTIL_VecToAngles(v_new_enemy);
			bots[bot_index].pEdict->v.ideal_yaw = bot_angles.y;

			BotFixIdealYaw(bots[bot_index].pEdict);

			bots[bot_index].SetSubTask(ST_FACEENEMY);

			// we need to know if bot turned more than a few degree to a side
			// if so then we have to trace that direction immediatelly to see
			// if there's danger of death fall
			if (bots[bot_index].IsTask(TASK_DEATHFALL) && (bots[bot_index].f_dontmove_time < gpGlobals->time))
			{
				bots[bot_index].move_speed = SPEED_NO;
				bots[bot_index].SetDontMoveTime(1.0);


				
				//@@@@@@@@@@@@@@@@@@@@@@@@@@@
#ifdef _DEBUG
				ALERT(at_console, "BEEN HIT during DEATH FALL danger -->>> STOP\n");
#endif
			}

			// don't look for waypoint while searching the enemy
			bots[bot_index].f_dont_look_for_waypoint_time = gpGlobals->time + 0.4f;			// was 0.2

			// if not already bandaging then prevent starting it
			if (bots[bot_index].bandage_time <= gpGlobals->time)							// NEW CODE 094
				bots[bot_index].bandage_time = -1.0;
			
			// break wpt_action_time
			bots[bot_index].wpt_action_time = 0.0;
		}
	}
}

/*
* this message gets sent when the bots get killed
*/
void BotClient_FA_DeathMsg(void *p, int bot_index)
{
	/*											NOT USED YET

	static int state = 0;   // current state machine state
	static int killer_index;
	static int victim_index;
	static edict_t *victim_edict;
	static int index;

	if (state == 0)
	{
		state++;
		killer_index = *(int *)p;  // ENTINDEX() of killer
	}
	else if (state == 1)
	{
		state++;
		victim_index = *(int *)p;  // ENTINDEX() of victim
	}
	else if (state == 2)
	{
		state = 0;

		victim_edict = INDEXENT(victim_index);

		index = UTIL_GetBotIndex(victim_edict);

		// is this message about a bot being killed?
		if (index != -1)
		{
			if ((killer_index == 0) || (killer_index == victim_index))
			{
				// bot killed by world (worldspawn) or bot killed self
				bots[index].killer_edict = NULL;
			}
			else
			{
				// store edict of player that killed this bot
				bots[index].killer_edict = INDEXENT(killer_index);
			}
		}
	}

	*/
}

/*
* this message gets send when any text is printed on the client's screen
* like changing to gl attachment and back or player's promotion to Sergeant etc.
*/
void BotClient_FA_TextMsg(void *p, int bot_index)
{
	static int state = 0;   // current state machine state

	if (state == 0)
		state++;

	else if (state == 1)
	{
		state = 0;

		char the_text[256];

		// get the message and its length engine sends
		strcpy(the_text, (char *)p);
		int pos = strlen(the_text);

		// remove '\n' sign from its end
		the_text[pos - 1] = 0;
		pos--;


		// prevents going prone for some time
		if ((strcmp(the_text, "Not enough room to go prone here!") == 0) ||
			(strcmp(the_text, "Cannot prone on another player!") == 0))
		{
#ifdef DEBUG

			//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@													// NEW CODE 094 (remove it)
			char dm[256];
			sprintf(dm, "(bot_client.cpp) %s CANNOT PRONE message -> SET CANTPRONE subtask!\n", bots[bot_index].name);
			//ALERT(at_console, dm); 
			UTIL_DebugInFile(dm);
			//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#endif // DEBUG

			bots[bot_index].SetSubTask(ST_CANTPRONE);
		}

		// switches to secondary fire for m16
		else if (strcmp(the_text, "Switched to M203 fire") == 0)
		{
			bots[bot_index].secondary_active = TRUE;


#ifdef DEBUG

			//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@													// NEW CODE 094 (remove it)
			char dm[256];
			sprintf(dm, "(bot_client.cpp) %s got SWITCHED TO M203 message!\n", bots[bot_index].name);
			ALERT(at_console, dm);
			UTIL_DebugInFile(dm);
			//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#endif // DEBUG



		}
		// switches back to primary fire for m16
		else if (strcmp(the_text, "Switched to M16 fire") == 0)
		{
			bots[bot_index].secondary_active = FALSE;


#ifdef DEBUG

			//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@													// NEW CODE 094 (remove it)
			char dm[256];
			sprintf(dm, "(bot_client.cpp) %s got SWITCHED TO M16 FIRE message!\n", bots[bot_index].name);
			ALERT(at_console, dm);
			UTIL_DebugInFile(dm);
			//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#endif // DEBUG



		}
		// switches to secondary fire for ak74
		else if (strcmp(the_text, "Switched to gp25 fire") == 0)
		{
			bots[bot_index].secondary_active = TRUE;



#ifdef DEBUG

			//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@													// NEW CODE 094 (remove it)
			char dm[256];
			sprintf(dm, "(bot_client.cpp) %s got SWITCHED TO GP25 message!\n", bots[bot_index].name);
			ALERT(at_console, dm);
			UTIL_DebugInFile(dm);
			//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#endif // DEBUG




		}
		// switches back to primary fire for ak74
		else if (strcmp(the_text, "Switched to normal fire") == 0)
		{
			bots[bot_index].secondary_active = FALSE;



#ifdef DEBUG

			//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@													// NEW CODE 094 (remove it)
			char dm[256];
			sprintf(dm, "(bot_client.cpp) %s got SWITCHED TO NORMAL(ak74) fire message!\n", bots[bot_index].name);
			ALERT(at_console, dm); 
			UTIL_DebugInFile(dm);
			//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#endif // DEBUG



		}
		// player has been promoted
		else if (strncmp(the_text, "You have been promoted ", 23) == 0)
		{
			bots[bot_index].SetTask(TASK_PROMOTED);
		}
		// the bot is merging magazines
		else if (strncmp(the_text, "Merging magazines...", 20) == 0)
		{
			// so pause him for some time to finish it
			// from the tests I did it seems that the proccess takes roughly 3 seconds
			bots[bot_index].SetPauseTime(RANDOM_FLOAT(3.7, 5.0));

			bots[bot_index].weapon_action = W_INMERGEMAGS;

			// and clear both bits to prevent doing this over and over again
			bots[bot_index].RemoveWeaponStatus(WS_MERGEMAGS1);
			bots[bot_index].RemoveWeaponStatus(WS_MERGEMAGS2);

#ifdef DEBUG

			//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@													// NEW CODE 094 (remove it)
			char dm[256];
			sprintf(dm, "(bot_client.cpp) %s got MERGING MAGAZINES message!\n", bots[bot_index].name);
			UTIL_DebugInFile(dm);
			//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#endif // DEBUG


		}
		// the bot is under medical treatment
		else if ((strncmp(the_text, "You are being bandaged", 22) == 0) ||
			(strncmp(the_text, "You have been treated", 21) == 0) || (strncmp(the_text, "%s applies sulfer", 17) == 0))
		{
			// we must first fix the message system for these messages
			// because these messages have additional string
			// (ie. it's not a message of 2 lines, but a message of 3 lines)
			state = 2;


#ifdef DEBUG
			char dm[256];
			sprintf(dm, "(bot_client.cpp) %s is being healed by a teammate!\n", bots[bot_index].name);
			//ALERT(at_console, dm);
			UTIL_DebugInFile(dm);
#endif // DEBUG






			// TODO:	Need to check exact time of sulfer treatment
			//			It seems to be not as long as healing so the pause could then be shorter
			//			Also sulfer doesn't seem to invalidate the weapon action.
			//			More tests are needed to prove it.







			// based on tests in FA 3.0 healing teammate takes 3.1 seconds
			// we'll pause the bot for a little longer though
			// to give game engine enough time to make the weapon usable,
			// because FA disables weapon during healing and then it takes a while to be able to use it again,
			// if we allowed the bot to manipulate with the gun too soon the engine wouldn't recognize it
			// and the bot would be stuck in endless reloading loop for example
			bots[bot_index].SetPauseTime(RANDOM_FLOAT(4.3, 4.8));

			// if the bot is in process of reloading his weapon then
			// it will be invalidated when someone else started to heal him
			if (bots[bot_index].IsWeaponStatus(WS_PRESSRELOAD))
			{
				bots[bot_index].SetWeaponStatus(WS_INVALID);
				bots[bot_index].f_reload_time = 0.0f;

#ifdef DEBUG
				char dm[256];
				sprintf(dm, "(bot_client.cpp) %s invalidated weapon reloading, because of being healed by a teammate!\n", bots[bot_index].name);
				//ALERT(at_console, dm);
				sprintf(dm, "%s pause time needed to finish the treatment will end at %.3f\n", dm, bots[bot_index].f_pause_time);
				UTIL_DebugInFile(dm);
#endif // DEBUG


			}
			// or is the bot changing current weapon?
			else if ((bots[bot_index].weapon_action == W_TAKEOTHER) || (bots[bot_index].weapon_action == W_INCHANGE))
			{
				// we must invalide this action, because often the engine doesn't recognize it fast enough
				// and the player ends with the original weapon instead of with the selected one
				// so we have to tell the bot to start anew after the treatment is over
				bots[bot_index].SetWeaponStatus(WS_INVALID);


#ifdef DEBUG
				char dm[256];
				sprintf(dm, "(bot_client.cpp) %s invalidated weapon change, because of being healed by a teammate!\n", bots[bot_index].name);
				//ALERT(at_console, dm);
				sprintf(dm, "%s pause time needed to finish the treatment will end at %.3f\n", dm, bots[bot_index].f_pause_time);
				UTIL_DebugInFile(dm);
#endif // DEBUG
			
			
			
			}
		}
		// the bot bandage his wounds
		// NOTE: This message also gets sent when teammate start to bandage this bot/player
		// (this one is sent first and is followed by message 'You're being bandaged by <teammate name here>')
		else if (strncmp(the_text, "You bandage your wounds", 23) == 0)
		{
			// this is sort of safety call, because the bot isn't allowed to start bandaging his wounds and
			// start to reload current weapon at the same time, but if it happens somehow
			// then we must invalidate the reloading so the bot can do it again after he finishes bandaging his wounds
			if (bots[bot_index].IsWeaponStatus(WS_PRESSRELOAD))
			{
				bots[bot_index].SetWeaponStatus(WS_INVALID);
				bots[bot_index].f_reload_time = 0.0f;

#ifdef DEBUG
				char dm[256];
				sprintf(dm, "(bot_client.cpp) %s invalidated weapon reloading, because of doing it at the same time as bandaging wounds!\n", bots[bot_index].name);
				//ALERT(at_console, dm); 
				UTIL_DebugInFile(dm);
#endif // DEBUG
			}
		}
		// the bot successfully finished medevac
		else if ((strcmp(the_text, "Patient life sustained") == 0) || (strcmp(the_text, "Patient will MEDEVAC") == 0))
		{
			bots[bot_index].SetSubTask(ST_MEDEVAC_DONE);
		}
		// the bot is trying to place a claymore inside or too close to some object
		// or in no claymore zone ie. where people aren't allowed to place claymores
		else if ((strncmp(the_text, "Claymore cannot be placed here!", 31) == 0) ||
			(strncmp(the_text, "You are not allowed to place a claymore here", 44) == 0))
		{
			bots[bot_index].SetSubTask(ST_RESETCLAYMORE);
			
#ifdef DEBUG
			char dm[256];
			sprintf(dm, "(bot_client.cpp) %s is placing the claymore too close to a wall/object!\n", bots[bot_index].name);
			//ALERT(at_console, dm); 
			UTIL_DebugInFile(dm);
#endif // DEBUG
		}
		// the bot is doing medical treatment
		else if ((strncmp(the_text, "You apply a bandage to", 22) == 0) ||
			(strncmp(the_text, "You apply sulfer to", 19) == 0))
		{
			// the only thing we must do here is that
			// we must fix the message system for these messages
			// because these messages have additional string
			// (ie. it's not a message of 2 lines, but a message of 3 lines)
			// without this fix the whole message system would get corrupted for this particular bot
			// (ie. he would have not been able to decode the following text messages correctly)
			state = 2;


#ifdef DEBUG
			char dm[256];
			sprintf(dm, "(bot_client.cpp) %s is doing medical treatment to his teammate!\n", bots[bot_index].name);
			//ALERT(at_console, dm);
			UTIL_DebugInFile(dm);
#endif // DEBUG





		}
	}
	// there's nothing important on the 3rd line
	// just the name of the teammate who is doing or receives the treatment
	// so we just reset it back to "start"
	else if (state == 2)
		state = 0;
}

/*
* this message gets send when any text is printed on all client's screen
* like certain push point (ie flag) has been captured by one team etc.
*/
void BotClient_FA_TextMsg_ForAll(void *p, int bot_index)
{
	static int state = 0;   // current state machine state
	static char the_text[256];
	char the_item[126];

	if (state == 0)
	{
		state++;
	}

	else if (state == 1)
	{
		// get the message the engine sends
		strcpy(the_text, (char *)p);

		if (strstr(the_text, "Force occupies") != nullptr)
		{
			UpdateWaypointData();
		}

		// is this message just a generic header (ie. is there going to be inserted an additional string in this message)
		if (strstr(the_text, "%s") != nullptr)
		{
			// then gets us to the additional string that gets send in this case
			state++;
		}
		// it's a full message then so let's ...
		else
		{
			state = 0;

			// check if the message matches any user-defined trigger event
			if (WaypointMatchingTriggerMessage(the_text))
			{		
				// we will update waypoints as well just to "know" about the latest events
				UpdateWaypointData();
			}
		}
	}
	else if (state == 2)
	{
		state = 0;

		strcpy(the_item, (char *)p);

		char temp_text[sizeof(the_text)];

		// get the pointer where we are going to insert the 2nd string
		char *pch = strstr(the_text, "%s");

		// this should not happen, but if it does report it and do NOT continue as it may result in errors
		if (pch == nullptr)
		{
			UTIL_DebugInFile("FATextMsgForAll() -> state 2 -> pch is NULL\n");
			return;
		}
		if (the_item == nullptr)
		{
			UTIL_DebugInFile("FATextMsgForAll() -> state 2 -> the item is NULL\n");
			return;
		}

		// holds the position of the %s marker ... NOT an array based index!
		const int pos = (pch - the_text);
		int len = strlen(the_text);

		// make a copy of the original message past the %s marker
		for (int i = 0; i < len - pos; i++)
		{
			// we need to get past the %s marker so we have to add 2 to actual position in the string
			temp_text[i] = the_text[i+pos+2];
		}
		
		len = strlen(the_item);

		// insert the 2nd string to its position
		if (strncpy(pch, the_item, len) != nullptr)
		{
			// we have to terminate it before the next append
			the_text[pos+len] = '\0';

			// add the trailing part of the original message back
			if (strcat(the_text, temp_text) != nullptr)
			{
				// finally terminate the message ... just in case
				len = strlen(the_text);
				the_text[len] = '\0';
				
				if (WaypointMatchingTriggerMessage(the_text))
				{
					UpdateWaypointData();
				}
			}
		}
	}
}

/*
* this message gets send when any HUD text (the fancy colored and bit transparent one) is printed on the client's screen
* like the messages mappers often send to clients eg. the countdown on obj maps (thanatos uses just these)
*/
void BotClient_FA_HUDMsg(void *p, int bot_index)
{
	static int state = 0;   // current state machine state

	while (state < 18)
	{
		state++;
		
		// the message gets sent as the last thing
		if (state == 17)
		{
			// reset it back to start
			state = 0;
			
			char the_text[256];
			// get the message the engine sends
			strcpy(the_text, (char *)p);
			
			// check if the message matches any user-defined trigger event
			if (WaypointMatchingTriggerMessage(the_text))
			{		
				// we will update waypoints as well just to "know" about the latest events
				UpdateWaypointData();
			}

			// we have to break it here otherwise we would loop forever
			break;
		}
	}
}

/*
* this message gets sent when bots FOV (Field Of View) changes
* for example when the bot switches zoom level on sniper rifle etc.
*/
void BotClient_FA_FOV(void *p, int bot_index)
{
	if (*(int *)p == NO_ZOOM)
	{
		bots[bot_index].secondary_active = FALSE;
	}
	else if (*(int *)p == ZOOM_05X)
	{
		;		// do nothing for now -- no code for those weapons (anaconda, g36e, m14)
	}
	else if (*(int *)p == ZOOM_1X)
	{
		// m82 has better zoom so don't set the flag now for it
		if (bots[bot_index].current_weapon.iId != fa_weapon_m82)
		{
			bots[bot_index].secondary_active = TRUE;
		}
	}
	else if (*(int *)p == ZOOM_2X)
	{
		bots[bot_index].secondary_active = TRUE;
	}
}

/*
* this message gets sent when stamina is used (ie player do sprint or jump)
* NOTE: all stamina values are just taken from some tests - no exact numbers!!!
*/
void BotClient_FA_Stamina(void *p, int bot_index)
{
	if (*(int *)p <= 4)
	{
		bots[bot_index].RemoveTask(TASK_SPRINT);	// no sprint is allowed
	}
	else if (*(int *)p <= 15)
	{
		bots[bot_index].SetTask(TASK_NOJUMP);		// no jump is allowed
	}
	else if (*(int *)p >= 35)
	{
		bots[bot_index].RemoveTask(TASK_NOJUMP);	// the bot can jump again
	}
}




// note by Frank WHY??? why here it has nothing to do with HL engine message decoding
// this file contains only methods that do decode messages nothing else
// so move it to util.cpp or somewhere else
/*
Order harakiri to bots of team at harakiri time.
*/
void SetHarakiri(int team) {
	static char harakiri_msg[]="One team loses the game. SAMURAIS MUST do HARA-KIRI at 60 sec!!!";
	if (bot_t::harakiri_moment <= 0.0)
	{
		bot_t::harakiri_moment = gpGlobals->time + externals.harakiri_time;
		//Vector color1 = Vector(200, 50, 0);
		//Vector color2 = Vector(0, 250, 0);
		//CustHudMessageToAll(harakiri_msg, color1, color2, 2, 8);
	}
	for (int i=0; i<gpGlobals->maxClients; ++i)
	{
		if (bots[i].pEdict!= nullptr && bots[i].pEdict->v.team == team)
		{
			//ALERT(at_console, "@@@@@@ SetHarakiri i=%d team=%d harakiri=%d\n", 
			//	i, bots[i].pEdict->v.team, bots[i].harakiri);
			if (bots[i].harakiri == false)
			{
				bots[i].harakiri = true;
				//ALERT(at_console, "@@@@@@ SetHarakiri ON for team %d bot=%d\n", team, i);
			}
		}
	}	
}
//end of the note




/*
* this message gets sent when reinforcements are changed
* (ie player has been killed and is respawning now)
*/
void BotClient_FA_Reins(void *p, int bot_index)
{
	static int state = 0;   // current state machine state

	if (state == 0)
	{
		state++;
		
		if (*(int *)p == 0)
		{
			botmanager.SetOverrideTeamsBalance(true);
		}
	}
	else if (state == 1)
	{
		state = 0;

		if (*(int *)p == 0)
		{
			botmanager.SetOverrideTeamsBalance(true);
		}
	}


// note by Frank:
//		Next thing is that you need to add it also to BotClient_FA_BrokenLeg() due to
//		differences in FA versions

	if(*(int *)p<=0)
	{
		//reinforcements is ended. 
		const int team_gamers = UTIL_CountPlayers(TEAM_BLUE - state);
		//ALERT(at_console, "REINF change p=%d auto_balance=%d state=%d team_gamers=%d\n", 
			//*(int*)p, override_auto_balance, state, team_gamers);
		if (team_gamers==0)
		{
			SetHarakiri(TEAM_BLUE - state);
		}
	}
	/*
	else {
		team_gamers = UTIL_CountPlayers(TEAM_BLUE - state);
		ALERT(at_console, "REINF change p=%d auto_balance=%d state=%d team_gamers=%d\n", 
			*(int*)p, override_auto_balance, state, team_gamers);
	}
	*/
}

/*
* code for FA 2.8 and versions above
* this message gets sent when player is being affected by conc grenade (blindness & screen shake)
* its send only at the start and at the end of this event
*/
void BotClient_FA_Concuss(void *p, int bot_index)
{
	static int duration;

	duration = *(int *)p;

	if (duration > 0)
	{
		bots[bot_index].blinded_time = gpGlobals->time + duration - 2.0;
	}
	// the concussion grenade stun is over
	else
	{
		bots[bot_index].blinded_time = 0.0;
	}
}

void BotClient_FA_ScreenFade(void *p, int bot_index)
{
	static int state = 0;   // current state machine state
	static int duration;
	static int hold_time;
	static int fade_flags;

	if (state == 0)
	{
		state++;
		duration = *(int *)p;
	}
	else if (state == 1)
	{
		state++;
		hold_time = *(int *)p;
	}
	else if (state == 2)
	{
		state++;
		fade_flags = *(int *)p;
	}
	else if (state == 6)
	{
		state = 0;

		const int length = (duration + hold_time) / 4096;
		bots[bot_index].blinded_time = gpGlobals->time + length - 2.0;
	}
	else
	{
		state++;
	}
}
