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
// bot_combat.cpp
// 
////////////////////////////////////////////////////////////////////////////////////////////////

#include "defines.h"

#include "extdll.h"
#include "util.h"
#include "cbase.h"

#include "bot.h"
#include "bot_func.h"
#include "bot_weapons.h"

extern bot_weapon_t weapon_defs[MAX_WEAPONS];
extern bool b_observer_mode;
extern bool b_botdontshoot;
extern bool g_debug_bot_on;
extern float is_team_play;
extern bool checked_teamplay;

float rg_modif = 2.5;		// multiplier that modifies all effective ranges for game purpose

#ifdef _DEBUG

bool in_bot_dev_level1 = FALSE;		// if TRUE print basic info at_console
bool in_bot_dev_level2 = FALSE;		// detailed info - print every action

#endif

// BotFireWeapon(), BotUseKnife() and BotThrowGrenade() return constants
#define RETURN_NOTFIRED		0
#define RETURN_FIRED		1
#define RETURN_RELOADING	2
#define RETURN_NOAMMO		3
#define RETURN_TOOCLOSE		4
#define RETURN_TOOFAR		5
#define RETURN_SECONDARY	6
#define RETURN_TAKING		7
#define RETURN_PRIMING		8

// we need these variables to handle different weapon IDs in FA versions
// we have to init them by -10, because of some checks where -1 would cause bug eg. pBot->main_weapon
int default_ID = -10;
int fa_weapon_knife =		default_ID;
int fa_weapon_coltgov =		default_ID;
int fa_weapon_ber92f =		default_ID;
int fa_weapon_ber93r =		default_ID;
int fa_weapon_anaconda =	default_ID;
int fa_weapon_desert =		default_ID;
int fa_weapon_benelli =		default_ID;		// remington in FA28 and above
int fa_weapon_saiga =		default_ID;
int fa_weapon_mp5k =		default_ID;		// FA24 weapon
int fa_weapon_mp5a5 =		default_ID;
int fa_weapon_mc51 =		default_ID;		// FA25 weapon
int fa_weapon_m11 =			default_ID;		// FA25 weapon
int fa_weapon_bizon =		default_ID;
int fa_weapon_sterling =	default_ID;
int fa_weapon_uzi =			default_ID;
int fa_weapon_m79 =			default_ID;
int fa_weapon_frag =		default_ID;
int fa_weapon_concussion =	default_ID;
int fa_weapon_flashbang =	default_ID;
int fa_weapon_stg24 =		default_ID;		// stielhandgranate
int fa_weapon_claymore =	default_ID;
int fa_weapon_ak47 =		default_ID;
int fa_weapon_famas =		default_ID;
int fa_weapon_g3a3 =		default_ID;
int fa_weapon_g36e =		default_ID;
int fa_weapon_m16 =			default_ID;
int fa_weapon_ak74 =		default_ID;
int fa_weapon_m14 =			default_ID;
int fa_weapon_m4 =			default_ID;
int fa_weapon_aug =			default_ID;		// FA25 weapon
int fa_weapon_psg1 =		default_ID;		// FA25 weapon
int fa_weapon_ssg3000 =		default_ID;
int fa_weapon_m82 =			default_ID;
int fa_weapon_svd =			default_ID;
int fa_weapon_m60 =			default_ID;
int fa_weapon_m249 =		default_ID;
int fa_weapon_pkm =			default_ID;


// bot_combat functions prototypes
bool InitFABaseWeapons(void);
bool InitFASameWeapons(void);
bool InitFA24Weapons(void);
bool InitFA25Weapons(void);
bool InitFA26Weapons(void);
bool InitFA27Weapons(void);
bool InitFA28Weapons(void);
bool InitFA29Weapons(void);
void BotReactions(bot_t *pBot);
inline void DontSeeEnemyActions(bot_t *pBot);
void BotFireMountedGun(Vector v_enemy, bot_t *pBot);
int BotFireWeapon(Vector v_enemy, bot_t *pBot);
int BotUseKnife(Vector v_enemy, bot_t *pBot);
int BotThrowGrenade(Vector v_enemy, bot_t *pBot);
inline void IsChanceToAdvance(bot_t *pBot);
inline void CheckStance(bot_t *pBot);
bool IsEnemyTooClose(bot_t *pBot, float enemy_distance);


/*
* inits a few weapons that have same ID in all FA versions
*/
bool InitFABaseWeapons(void)
{
	fa_weapon_knife =		FAB_WEAPON_KNIFE;
	fa_weapon_coltgov =		FAB_WEAPON_COLTGOV;
	fa_weapon_ber92f =		FAB_WEAPON_BER92F;
	fa_weapon_ber93r =		FAB_WEAPON_BER93R;
	fa_weapon_anaconda =	FAB_WEAPON_ANACONDA;
	fa_weapon_desert =		FAB_WEAPON_DESERT;
	fa_weapon_benelli =		FAB_WEAPON_BENELLI;
	fa_weapon_saiga =		FAB_WEAPON_SAIGA;

	// test if all weapons were successfully initialized
	if ((fa_weapon_knife == default_ID) || (fa_weapon_coltgov == default_ID) ||
		(fa_weapon_ber92f == default_ID) || (fa_weapon_ber93r == default_ID) ||
		(fa_weapon_anaconda == default_ID || (fa_weapon_desert == default_ID) ||
		(fa_weapon_benelli == default_ID) || (fa_weapon_saiga == default_ID)))
	{
		return FALSE;
	}

	return TRUE;
}


/*
* inits a few weapons that have same ID in more than one version
*/
bool InitFASameWeapons(void)
{
	fa_weapon_mp5a5 =		FA25_WEAPON_MP5A5;
	fa_weapon_m79 =			FA25_WEAPON_M79;
	fa_weapon_frag =		FA25_WEAPON_FRAG;
	fa_weapon_claymore =	FA25_WEAPON_CLAYMORE;
	fa_weapon_stg24 =		FA25_WEAPON_STG24;
	fa_weapon_ak47 =		FA25_WEAPON_AK47;
	fa_weapon_famas =		FA25_WEAPON_FAMAS;
	fa_weapon_g3a3 =		FA25_WEAPON_G3A3;
	fa_weapon_g36e =		FA25_WEAPON_G36E;
	fa_weapon_m16 =			FA25_WEAPON_M16;
	fa_weapon_m82 =			FA25_WEAPON_M82;
	fa_weapon_m60 =			FA25_WEAPON_M60;
	fa_weapon_bizon =		FA25_WEAPON_BIZON;
	fa_weapon_sterling =	FA25_WEAPON_STERLING;

	if ((fa_weapon_mp5a5 == default_ID) || (fa_weapon_m79 == default_ID) ||
		(fa_weapon_frag == default_ID) || (fa_weapon_claymore == default_ID) ||
		(fa_weapon_stg24 == default_ID) || (fa_weapon_ak47 == default_ID) ||
		(fa_weapon_famas == default_ID) || (fa_weapon_g3a3 == default_ID) ||
		(fa_weapon_g36e == default_ID) || (fa_weapon_m16 == default_ID) ||
		(fa_weapon_m82 == default_ID) || (fa_weapon_m60 == default_ID) ||
		(fa_weapon_bizon == default_ID) || (fa_weapon_sterling == default_ID))
	{
		return FALSE;
	}

	return TRUE;
}


/*
* inits all FA24 versions, we can't use the same weapons init method because of sterling
*/
bool InitFA24Weapons(void)
{
	fa_weapon_mp5k =		FA24_WEAPON_MP5K;
	fa_weapon_mp5a5 =		FA25_WEAPON_MP5A5;
	fa_weapon_mc51 =		FA25_WEAPON_MC51;
	fa_weapon_m11 =			FA25_WEAPON_M11;
	fa_weapon_m79 =			FA25_WEAPON_M79;
	fa_weapon_frag =		FA25_WEAPON_FRAG;
	fa_weapon_flashbang =	FA25_WEAPON_FLASHBANG;	
	fa_weapon_claymore =	FA25_WEAPON_CLAYMORE;
	fa_weapon_stg24 =		FA25_WEAPON_STG24;
	fa_weapon_ak47 =		FA25_WEAPON_AK47;
	fa_weapon_famas =		FA25_WEAPON_FAMAS;
	fa_weapon_psg1 =		FA25_WEAPON_PSG1;
	fa_weapon_g3a3 =		FA25_WEAPON_G3A3;
	fa_weapon_g36e =		FA25_WEAPON_G36E;
	fa_weapon_m16 =			FA25_WEAPON_M16;
	fa_weapon_aug =			FA25_WEAPON_AUG;
	fa_weapon_m82 =			FA25_WEAPON_M82;
	fa_weapon_m4 =			FA25_WEAPON_M4;
	fa_weapon_m60 =			FA25_WEAPON_M60;
	fa_weapon_bizon =		FA25_WEAPON_BIZON;

	if ((fa_weapon_mp5k == default_ID) || (fa_weapon_mp5a5 == default_ID) ||
		(fa_weapon_mc51 == default_ID) || (fa_weapon_m11 == default_ID) ||
		(fa_weapon_m79 == default_ID) || (fa_weapon_frag == default_ID) ||
		(fa_weapon_flashbang == default_ID) || (fa_weapon_claymore == default_ID) ||
		(fa_weapon_stg24 == default_ID) || (fa_weapon_ak47 == default_ID) ||
		(fa_weapon_famas == default_ID) || (fa_weapon_psg1 == default_ID) ||
		(fa_weapon_g3a3 == default_ID) || (fa_weapon_g36e == default_ID) ||
		(fa_weapon_m16 == default_ID) || (fa_weapon_aug == default_ID) ||
		(fa_weapon_m82 == default_ID) || (fa_weapon_m4 == default_ID) ||
		(fa_weapon_m60 == default_ID) || (fa_weapon_bizon == default_ID))
	{
		return FALSE;
	}

	return TRUE;
}


/*
* inits all FA25 versions
*/
bool InitFA25Weapons(void)
{
	bool prev_inits = FALSE;

	// we are using special initinalization for some weapons
	// because their IDs are same in more versions
	if (InitFASameWeapons())
		prev_inits = TRUE;
	
	fa_weapon_mc51 =		FA25_WEAPON_MC51;
	fa_weapon_m11 =			FA25_WEAPON_M11;	
	fa_weapon_flashbang =	FA25_WEAPON_FLASHBANG;	
	fa_weapon_psg1 =		FA25_WEAPON_PSG1;	
	fa_weapon_aug =			FA25_WEAPON_AUG;	
	fa_weapon_m4 =			FA25_WEAPON_M4;	

	if ((prev_inits == FALSE) || (fa_weapon_mc51 == default_ID) ||
		(fa_weapon_m11 == default_ID) || (fa_weapon_flashbang == default_ID) ||
		(fa_weapon_psg1 == default_ID) || (fa_weapon_aug == default_ID) ||
		(fa_weapon_m4 == default_ID))
	{
		return FALSE;
	}

	return TRUE;
}


/*
* inits all FA26 (2.65 as well) versions
*/
bool InitFA26Weapons(void)
{
	bool prev_inits = FALSE;

	// we are using special initinalization for some weapons
	// because their IDs are same in more versions
	if (InitFASameWeapons())
		prev_inits = TRUE;

	fa_weapon_svd =			FA26_WEAPON_SVD;
	fa_weapon_uzi =			FA26_WEAPON_UZI;
	fa_weapon_concussion =	FA26_WEAPON_CONCUSSION;
	fa_weapon_ssg3000 =		FA26_WEAPON_SSG3000;
	fa_weapon_ak74 =		FA26_WEAPON_AK74;
	fa_weapon_pkm =			FA26_WEAPON_PKM;

	if ((prev_inits == FALSE) || (fa_weapon_svd == default_ID) || (fa_weapon_uzi == default_ID) ||
		(fa_weapon_concussion == default_ID) || (fa_weapon_ssg3000 == default_ID) ||
		(fa_weapon_ak74 == default_ID) || (fa_weapon_pkm == default_ID))
	{
		return FALSE;
	}

	return TRUE;
}


/*
* inits all FA27 versions
*/
bool InitFA27Weapons(void)
{
	bool prev_inits = FALSE;

	// we are using special initinalization for some weapons
	// because their IDs are same in more versions
	if (InitFASameWeapons())
		prev_inits = TRUE;

	fa_weapon_svd =			FA26_WEAPON_SVD;
	fa_weapon_uzi =			FA26_WEAPON_UZI;
	fa_weapon_concussion =	FA26_WEAPON_CONCUSSION;
	fa_weapon_ssg3000 =		FA26_WEAPON_SSG3000;
	fa_weapon_ak74 =		FA26_WEAPON_AK74;
	fa_weapon_m249 =		FA26_WEAPON_PKM;		// the ID is same just different name

	if ((prev_inits == FALSE) || (fa_weapon_svd == default_ID) || (fa_weapon_uzi == default_ID) ||
		(fa_weapon_concussion == default_ID) || (fa_weapon_ssg3000 == default_ID) ||
		(fa_weapon_ak74 == default_ID) || (fa_weapon_m249 == default_ID))
	{
		return FALSE;
	}

	return TRUE;
}


/*
* inits all FA28 versions
*/
bool InitFA28Weapons(void)
{
	fa_weapon_mp5a5 =		FA28_WEAPON_MP5A5;
	fa_weapon_bizon =		FA28_WEAPON_BIZON;
	fa_weapon_sterling =	FA28_WEAPON_STERLING;
	fa_weapon_m79 =			FA28_WEAPON_M79;
	fa_weapon_frag =		FA28_WEAPON_FRAG;
	fa_weapon_concussion =	FA28_WEAPON_CONCUSSION;
	fa_weapon_claymore =	FA28_WEAPON_CLAYMORE;
	fa_weapon_ak47 =		FA28_WEAPON_AK47;
	fa_weapon_famas =		FA28_WEAPON_FAMAS;
	fa_weapon_g3a3 =		FA28_WEAPON_G3A3;
	fa_weapon_g36e =		FA28_WEAPON_G36E;
	fa_weapon_m16 =			FA28_WEAPON_M16;
	fa_weapon_ssg3000 =		FA28_WEAPON_SSG3000;
	fa_weapon_m82 =			FA28_WEAPON_M82;
	fa_weapon_ak74 =		FA28_WEAPON_AK74;
	fa_weapon_m60 =			FA28_WEAPON_M60;
	fa_weapon_m249 =		FA28_WEAPON_M249;
	fa_weapon_uzi =			FA28_WEAPON_UZI;
	fa_weapon_svd =			FA28_WEAPON_SVD;
	fa_weapon_pkm =			FA28_WEAPON_PKM;

	if ((fa_weapon_mp5a5 == default_ID) || (fa_weapon_bizon == default_ID) ||
		(fa_weapon_sterling == default_ID) || (fa_weapon_m79 == default_ID) ||
		(fa_weapon_frag == default_ID) || (fa_weapon_concussion == default_ID) ||
		(fa_weapon_claymore == default_ID) || (fa_weapon_ak47 == default_ID) ||
		(fa_weapon_famas == default_ID) || (fa_weapon_g3a3 == default_ID) ||
		(fa_weapon_g36e == default_ID) || (fa_weapon_m16 == default_ID) ||
		(fa_weapon_ssg3000 == default_ID) || (fa_weapon_m82 == default_ID) ||
		(fa_weapon_ak74 == default_ID) || (fa_weapon_m60 == default_ID) ||
		(fa_weapon_m249 == default_ID) || (fa_weapon_uzi == default_ID) ||
		(fa_weapon_svd == default_ID) || (fa_weapon_pkm == default_ID))
	{
		return FALSE;
	}

	return TRUE;
}


/*
* inits all FA29 versions
*/
bool InitFA29Weapons(void)
{
	bool prev_inits = FALSE;

	// we can use previous version initinalization because weapon IDs are same
	if (InitFA28Weapons())
		prev_inits = TRUE;

	fa_weapon_m14 =		FA29_WEAPON_M14;
	fa_weapon_m4 =		FA29_WEAPON_M4;

	if ((prev_inits == FALSE) || (fa_weapon_m14 == default_ID) || (fa_weapon_m4 == default_ID))
		return FALSE;

	return TRUE;
}


/*
* inits the right weapon set based on mod version
*/
bool InitFAWeapons(void)
{
	bool a_problem = FALSE;

	// these weapon have their IDs same in all version se we can call them here
	if (InitFABaseWeapons() == FALSE)
		a_problem = TRUE;

	if (g_mod_version == FA_24)
	{
		if (InitFA24Weapons())
			ALERT(at_console, "MarineBot firearms 2.4 weapon detection done\n");
		else
		{
			a_problem = TRUE;
			ALERT(at_console, "MarineBot cannot detect firearms 2.4 weapons!\n");
		}
	}
	else if (g_mod_version == FA_25)
	{
		if (InitFA25Weapons())
			ALERT(at_console, "MarineBot firearms 2.5 weapon detection done\n");
		else
		{
			a_problem = TRUE;
			ALERT(at_console, "MarineBot cannot detect firearms 2.5 weapons!\n");
		}
	}
	else if (g_mod_version == FA_26)
	{
		if (InitFA26Weapons())
			ALERT(at_console, "MarineBot firearms 2.6 (2.65) weapon detection done\n");
		else
		{
			a_problem = TRUE;
			ALERT(at_console, "MarineBot cannot detect firearms 2.6 (2.65) weapons!\n");
		}
	}
	else if (g_mod_version == FA_27)
	{
		if (InitFA27Weapons())
			ALERT(at_console, "MarineBot firearms 2.7 weapon detection done\n");
		else
		{
			a_problem = TRUE;
			ALERT(at_console, "MarineBot cannot detect firearms 2.7 weapons!\n");
		}
	}
	else if (g_mod_version == FA_28)
	{
		if (InitFA28Weapons())
			ALERT(at_console, "MarineBot firearms 2.8 weapon detection done\n");
		else
		{
			a_problem = TRUE;
			ALERT(at_console, "MarineBot cannot detect firearms 2.8 weapons!\n");
		}
	}
	else if (g_mod_version == FA_29)
	{
		if (InitFA29Weapons())
			ALERT(at_console, "MarineBot firearms 2.9 weapon detection done\n");
		else
		{
			a_problem = TRUE;
			ALERT(at_console, "MarineBot cannot detect firearms 2.9 weapons!\n");
		}
	}
	else if (g_mod_version == FA_30)
	{
		if (InitFA29Weapons())
			ALERT(at_console, "MarineBot firearms 3.0 weapon detection done\n");
		else
		{
			a_problem = TRUE;
			ALERT(at_console, "MarineBot cannot detect firearms 3.0 weapons!\n");
		}
	}

	// there was some problem during initialization so return error
	if (a_problem)
		return FALSE;

	return TRUE;
}


/*
* inits both weapon stucts (select as well as delay)
* weapons are stored based on their ID
*/
void BotWeaponArraysInit(Section *conf_weapons)
{
	int index, i;
	char weapon_num[16];
	bool modif=true;
	char msg[1024];

	if (conf_weapons == NULL)	//init all weapons to 0
	{
		for (index = 0; index < MAX_WEAPONS; ++index)
		{
			bot_weapon_select[index].iId = index;
			bot_weapon_select[index].max_effective_distance = 0.0;
			bot_weapon_select[index].min_safe_distance = 0.0;

			bot_fire_delay[index].primary_base_delay = 0.0;

			for (i=0; i<BOT_SKILL_LEVELS; ++i)
			{
				bot_fire_delay[index].primary_min_delay[i] = 0.0;
				bot_fire_delay[index].primary_max_delay[i] = 0.0;
			}
		}
		ALERT(at_console, "MarineBot firearms weapons initialisation WASN'T done\n");
		return;
	}
	else {
		for (index = 0; index < MAX_WEAPONS; ++index)
		{
			bot_weapon_select[index].iId = index;
			bot_fire_delay[index].iId = index;

			sprintf(weapon_num, "%d",index);
			CSI si = conf_weapons->sectionList.find(weapon_num);
			if (si==conf_weapons->sectionList.end()) {
				bot_weapon_select[index].max_effective_distance = 0.0;
				bot_weapon_select[index].min_safe_distance = 0.0;
			}
			else {
				SetupVars set_vars;
				set_vars.Add("modif", new SetupBaseType_yesno(modif), false, "no");
				set_vars.Add("min_safe_distance", new SetupBaseType_float(bot_weapon_select[index].min_safe_distance), false, "0.0");
				set_vars.Add("max_effective_distance", new SetupBaseType_float(bot_weapon_select[index].max_effective_distance), false, "9999.0");
				set_vars.Add("primary_base_delay", new SetupBaseType_float(bot_fire_delay[index].primary_base_delay), false, "0.0");
				try
				{
					set_vars.Set(si->second);
				}
				catch (errGetVal &er_val)
				{
					//sprintf(msg, "** missing variable '%s'", er_val.key);
					//
					//PrintOutput(NULL, msg, msg_error);
				}
				if (modif==true)
				{
					bot_weapon_select[index].max_effective_distance *= rg_modif;
				}
				
				for (i=0; i<BOT_SKILL_LEVELS; ++i)
				{
					bot_fire_delay[index].primary_min_delay[i] = 0.0;
					bot_fire_delay[index].primary_max_delay[i] = 0.0;
				}

				CII ii;
				CSI delay_si = si->second->sectionList.find("primary_min_delay");
				if (delay_si != si->second->sectionList.end()) //found
				{
					for(i=0,ii=delay_si->second->item.begin(); ii!=delay_si->second->item.end() && i<BOT_SKILL_LEVELS; ++ii,++i)
					{
						bot_fire_delay[index].primary_min_delay[i] = atof(ii->second.c_str());
					}
				}
				delay_si = si->second->sectionList.find("primary_max_delay");
				if (delay_si != si->second->sectionList.end()) //found
				{
					for(i=0,ii=delay_si->second->item.begin(); ii!=delay_si->second->item.end() && i<BOT_SKILL_LEVELS; ++ii,++i)
					{
						bot_fire_delay[index].primary_max_delay[i] = atof(ii->second.c_str());
					}
				}

			}
		}
	}
/*/
	FILE *ff = fopen("\\kota_debug.txt", "a+");
	if (ff != NULL) {
		for (index = 0; index < MAX_WEAPONS; ++index)
		{
			fprintf(ff,"bot_weapon_select[%d]=%d\n",index, bot_weapon_select[index].iId);
			fprintf(ff,"\tmax_effective_distance=%f\n", bot_weapon_select[index].max_effective_distance);
			fprintf(ff,"\tmin_safe_distance=%f\n",bot_weapon_select[index].min_safe_distance);;
			fprintf(ff,"\tbot_fire_delay=%f\n", bot_fire_delay[index].primary_base_delay);
			for (i=0; i<BOT_SKILL_LEVELS; ++i)
			{
				fprintf(ff,"\t\tprimary_min_delay=%f\n",bot_fire_delay[index].primary_min_delay[i]);
				fprintf(ff,"\t\tprimary_max_delay=%f\n",bot_fire_delay[index].primary_max_delay[i]);
			}
		}
		fclose(ff);
	}
/**/
	ALERT(at_console, "MarineBot firearms weapons initialisation done\n");
}


/*
* returns the max effective range of given weapon
* it takes the current weapon by default
*/
float bot_t::GetEffectiveRange(int weapon_index)
{
	// if the bot has no main weapon than take the current one
	if (weapon_index == -1)
		weapon_index = current_weapon.iId;

	// if there's no weapon at all return zero ... just for sure
	if (weapon_index == -1)
		return 0.0;

	float range;

	range = bot_weapon_select[weapon_index].max_effective_distance;

	// see if we do limit the max distance the bot can see (ie. view distance)
	// if so and the weapon effective range is bigger then the limit
	// we will use the view distance limit instead
	if (internals.IsDistLimit() && (range > internals.GetMaxDistance()))
		range = internals.GetMaxDistance();

	return range;
}



/*
* checks if is team play on
*/
void BotCheckTeamplay(void)
{
	// is this mod Firearms?
	if (mod_id == FIREARMS_DLL)
		is_team_play = 1.0;
	else
		is_team_play = CVAR_GET_FLOAT("mp_teamplay");  // teamplay enabled?

	checked_teamplay = TRUE;
}

/*
* sets correct reaction delay based on bot skill level
* eg. if reaction time is set to 1s then the best bot will use 0.5s as his reaction time
*/
void BotReactions(bot_t *pBot)
{
	// we shouldn't set reaction too often it could cause loop
	if (pBot->f_reaction_time + 2.0 > gpGlobals->time)
		return;

	if (pBot->pBotEnemy)
	{
		Vector vEnemyHead = pBot->pBotEnemy->v.origin + pBot->pBotEnemy->v.view_ofs;
		Vector vEnemyBody = pBot->pBotEnemy->v.origin;
		
		// is the enemy right in front of the bot
		// (ie. the bot don't have to turn to side to face it)?
		if (FInNarrowViewCone(&vEnemyHead, pBot->pEdict, 0.85) ||
			FInNarrowViewCone(&vEnemyBody, pBot->pEdict, 0.85))
		{
			// the bot will fire/attack in next frame
			// (ie. will skip this frame just to simulate some "aiming")
			pBot->f_reaction_time = gpGlobals->time;
			return;
		}
	}
	
	float react_time = externals.GetReactionTime();
	int skill = pBot->bot_skill + 1;	// array based

	// we are using skill level 3 as a default level so its reaction time isn't changed
	// but other skill levels should be modified
	switch (skill)
	{
		case 1:
			react_time -= (float) externals.GetReactionTime() / 2.0;	// use only 50% of it
			break;
		case 2:
			react_time -= (float) externals.GetReactionTime() / 3.0;	// use only 66% of it
			break;
		case 4:
			react_time += (float) externals.GetReactionTime() / 4.0;	// use 150% of it
			break;
		case 5:
			react_time += (float) externals.GetReactionTime();			// use 200% or it
			break;
		default:
			break;
	}

	// if we get under zero reset it back to zero value
	// shouldn't happen but just in case
	if (react_time < 0.0)
		react_time = 0.0;

	// store the reaction time
	pBot->f_reaction_time = gpGlobals->time + react_time;


#ifdef _DEBUG
	//@@@@@@@@@@@@@@@
	//char msg[256];
	//sprintf(msg, "***Reaction time has been set to %.2f for <%s>\n", react_time, pBot->name);
	//HudNotify(msg);
#endif
}

/*
* bot does these actions when do not have enemy at all or
* when do not currently see him (ie lost clear view)
*/
inline void DontSeeEnemyActions(bot_t *pBot)
{
	edict_t *pEdict = pBot->pEdict;

	// is there NO enemy for a while so bot can use bandages
	if ((pBot->bandage_time == -1.0) && (pBot->f_bot_see_enemy_time > 0) &&
		(pBot->f_bot_see_enemy_time + 0.5 <= gpGlobals->time))
	{
		pBot->bandage_time = gpGlobals->time;
	}

	// is the bot holding m3 AND have ammo AND
	// is some time bot seen an enemy so reload OR is bot waiting for enemy
	if ((pBot->current_weapon.iId == fa_weapon_benelli) &&
		(pBot->current_weapon.iAmmo1 != 0) &&
		(((pBot->f_bot_see_enemy_time > 0) &&
		((pBot->f_bot_see_enemy_time + 0.5) < gpGlobals->time)) ||
		(pBot->f_bot_wait_for_enemy_time > gpGlobals->time)))
	{
		// if NOT already reloading start reloading
		if ((pBot->current_weapon.iClip != 7) && (pBot->f_reload_time < gpGlobals->time))
		{
			pEdict->v.button |= IN_RELOAD;
			// set lower time than need to allow bot to shoot as soon as possible even
			// if not fully loaded
			pBot->f_reload_time = gpGlobals->time + 1.0;
		}
		// reset reload time when already fully loaded
		else if (pBot->current_weapon.iClip == 7)
			pBot->f_reload_time = gpGlobals->time - 0.1;
	}
	// is the bot holding colt AND have ammo AND
	// is some time bot seen an enemy OR is bot waiting for enemy
	else if ((pBot->current_weapon.iId == fa_weapon_anaconda) &&
		(pBot->current_weapon.iAmmo1 != 0) &&
		(((pBot->f_bot_see_enemy_time > 0) &&
		((pBot->f_bot_see_enemy_time + 0.5) < gpGlobals->time)) ||
		(pBot->f_bot_wait_for_enemy_time > gpGlobals->time)))
	{
		if ((pBot->current_weapon.iClip != 6) && (pBot->f_reload_time < gpGlobals->time))
		{
			pEdict->v.button |= IN_RELOAD;
			pBot->f_reload_time = gpGlobals->time + 1.0;
		}
		else if (pBot->current_weapon.iClip == 6)
			pBot->f_reload_time = gpGlobals->time - 0.1;
	}
	// is clip empty AND have at least one magazine AND
	// is some time bot seen an enemy so reload
	else if ((pBot->current_weapon.iClip == 0) && (pBot->current_weapon.iAmmo1 != 0) &&
		(((pBot->f_bot_see_enemy_time > 0) &&
		((pBot->f_bot_see_enemy_time + 0.5) < gpGlobals->time)) ||
		(pBot->f_bot_wait_for_enemy_time > gpGlobals->time)))
	{
		pEdict->v.button |= IN_RELOAD;
		pBot->f_reload_time = gpGlobals->time + 0.6; // average time for most of weapons
	}

	// didn't the bot seen ememy in last few seconds AND
	// is chance to speak (time to area clear)?
	if (!externals.GetDontSpeak() && ((pBot->f_bot_see_enemy_time > 0) &&
		((pBot->f_bot_see_enemy_time + 8.0) < gpGlobals->time)) &&
		((pBot->speak_time + RANDOM_FLOAT(25.5, 40.0)) < gpGlobals->time) &&
		(RANDOM_LONG(1, 100) <= 1))
	{
		UTIL_Radio(pEdict, "clear");
		pBot->speak_time = gpGlobals->time;
	}

	// remove ignore enemy task if the bot is not by/on the "crucial" waypoint/path
	if (pBot->IsTask(TASK_IGNORE_ENEMY) && pBot->crowded_wpt_index == -1 && !pBot->IsIgnorePath())
	{
		pBot->RemoveTask(TASK_IGNORE_ENEMY);

#ifdef _DEBUG
		extern bool debug_actions;
		if (debug_actions)
		{
			ALERT(at_console, "***Removed IGNORE ENEMY flag for bot %s\n", pBot->name);
		}
#endif
	}
}


/*
* will make sure the bot "forgets" about his current enemy (ie. clear all important variables)
*/
void bot_t::BotForgetEnemy(void)
{
#ifdef _DEBUG
	ALERT(at_console, "***ForgetEnemy called for bot %s\n", name);
#endif

	// don't have an enemy anymore so null out the pointer
	pBotEnemy = NULL;	
	// clear his backed up position
	v_last_enemy_position = Vector(0, 0, 0);	
	// reset wait & watch time
	f_bot_wait_for_enemy_time = gpGlobals->time - 0.1;	
	// reset prone prevention time
	f_cant_prone = gpGlobals->time - 0.1;
	// no enemy so bot is clear to use bandages
	bandage_time = gpGlobals->time;	
	// reset enemy distance backup
	f_prev_enemy_dist = 0.0;
	// clear medical treatment flag
	RemoveTask(TASK_HEALHIM);
	// clear heavy tracelining
	RemoveTask(TASK_DEATHFALL);
}


/*
* search the world for best enemy
* checks if enemy is still alive and visible and sets right behaviour based on it
*/
edict_t * bot_t::BotFindEnemy()
{
	Vector vecEnd;
	edict_t *pNewEnemy;
	edict_t *pPrevEnemy = NULL;		// previous enemy
	float nearest_distance;
	int clients;

	// does the bot already have an enemy?
	if (pBotEnemy != NULL)
	{
		vecEnd = pBotEnemy->v.origin + pBotEnemy->v.view_ofs;

		// the bot is going to medevac downed teammate so don't look for new enemy
		if (IsTask(TASK_HEALHIM) && IsTask(TASK_MEDEVAC))
		{
			return (pBotEnemy);
		}


		// the enemy is already dead
		// the enemy probably died during previous frame, assume bot or someone else killed it
		else if (IsAlive(pBotEnemy) == FALSE)
		{
			BotForgetEnemy();

			// is still snipe time
			if (sniping_time > gpGlobals->time)
			{
				// break snipe time only if bot is NOT using bipod AND is chance
				if ((UTIL_IsMachinegun(current_weapon.iId)) &&
					!IsTask(TASK_BIPOD) && (RANDOM_LONG(1, 100) < 10))
				{
					sniping_time = gpGlobals->time;	// so stop sniping
#ifdef _DEBUG
					// testing - snipe time
					if (in_bot_dev_level2)
					{
						char msg[80];
						sprintf(msg, "BREAKING sniping time\n");
						ALERT(at_console, msg);
					}
#endif
				}
			}
		}
		// the enemy must still be alive so ...
		else
		{
			// this "enemy" is a wounded teammate
			if (IsTask(TASK_HEALHIM))
			{
				// just return its pointer
				return (pBotEnemy);
			}

			// the bot is carrying a goal item
			// make him less aggresive
			if (IsTask(TASK_GOALITEM))
			{
				// don't attack "healthy" enemy	-- PROBABLY NOT SO GOOD IDEA TO LIMIT BOT THIS MUCH
				//if (pBotEnemy->v.health > 25)
				//{
				//	BotForgetEnemy();
				//	return (pBotEnemy);
				//}

				float enemy_distance = (pBotEnemy->v.origin - pEdict->v.origin).Length();

				// don't attack distant enemy
				if (enemy_distance > ENEMY_DIST_GOALITEM)
				{
					BotForgetEnemy();
					return (pBotEnemy);
				}
			}

			// the enemy probably won't attack this bot so forget about him
			if (IsTask(TASK_AVOID_ENEMY))
			{
				float enemy_distance = (pBotEnemy->v.origin - pEdict->v.origin).Length();
				if (enemy_distance > GetEffectiveRange() && enemy_distance > RANGE_MELEE)
				{
					BotForgetEnemy();
					return (pBotEnemy);
				}
			}
			if (IsTask(TASK_IGNORE_ENEMY))
			{
				float enemy_distance = (pBotEnemy->v.origin - pEdict->v.origin).Length();
				if (enemy_distance > RANGE_MELEE)
				{
					BotForgetEnemy();
					return (pBotEnemy);
				}
			}

			// check for position and visibility first
			bool InFOV = FInViewCone( &vecEnd, pEdict );
			bool IsVisible = false;
			int decide_visibility = FPlayerVisible( vecEnd, pEdict );
			
			// decide visibility based on current weapon
			// we don't want bots trying to shoot an enemy through fence with a weapon that cannot do it (eg. small arms)
			switch (decide_visibility)
			{
				case VIS_NO:
					IsVisible = false;
					break;
				case VIS_YES:
					IsVisible = true;
					break;
				case VIS_FENCE:
					if (UTIL_IsMachinegun(current_weapon.iId) || UTIL_IsSniperRifle(current_weapon.iId) ||
						UTIL_IsAssaultRifle(current_weapon.iId))
						IsVisible = true;
					else
						IsVisible = false;
					break;
			}

			// enemy is alive and fully visible OR was visible in last few seconds
			if ((InFOV && IsVisible) || (f_bot_see_enemy_time + 0.2 > gpGlobals->time))
			{
				// reset wait & watch time if needed
				if (f_bot_wait_for_enemy_time >= gpGlobals->time)
				{
					f_bot_wait_for_enemy_time = gpGlobals->time - 0.1;
					
					// apply reaction_time
					BotReactions(this);
					
					
					
					
					
//@@@@@@@@@@@@@
//#ifdef _DEBUG
					//if (test)
					//ALERT(at_console, "<%s> ***FullyVisible - bot is in W&W ---> applying REACTIONS (%.4f)\n",
					//	name, gpGlobals->time);
//#endif
					
					
					
				}
				
				
//@@@@@@@@@@@@@@@@@@
/*/
#ifdef _DEBUG
				else
					ALERT(at_console, "FullyVisible - enemy is FULLY VISIBLE (%.2f)\n",
						gpGlobals->time);
#endif
/**/




				// should the bot search for different enemy?
				if (IsTask(TASK_FIND_ENEMY))
				{
					pPrevEnemy = pBotEnemy;	// safe pointer to your current enemy
				}
				// otherwise return the current one
				else
				{
					// face the enemy
					Vector v_enemy = pBotEnemy->v.origin - pEdict->v.origin;
					Vector bot_angles = UTIL_VecToAngles( v_enemy );
					
					pEdict->v.ideal_yaw = bot_angles.y;
					BotFixIdealYaw(pEdict);

					SetSubTask(ST_FACEENEMY);

					// keep track of when we last saw an enemy
					if (IsVisible)
					{
						f_bot_see_enemy_time = gpGlobals->time;
						// store enemy last known (ie. visible) origin
						v_last_enemy_position = pBotEnemy->v.origin;
					}
					
					return (pBotEnemy);
				}
			}
			else
			{
				// the enemy is completely lost
				if (!InFOV)
				{
					BotForgetEnemy();
					return (pBotEnemy);
				}
				
				// the enemy must still be somewhere in front of the bot
				// it's just not visible right now ...

				// see if the bot can wait and watch for the enemy to become visible again
				if (f_bot_wait_for_enemy_time < gpGlobals->time)
				{



					//@@@@@@@@@@@@@@@
					//#ifdef _DEBUG
					//ALERT(at_console, "<%s> ***ENEMY ISN'T VISIBLE!\n", name);
					//#endif



					// generate the percentage chance
					int do_watch = RANDOM_LONG(1, 100);
					
					// in 30% should attacking bot keep watching that direction
					// and wait if enemy become visible again
					if (IsBehaviour(ATTACKER) && (do_watch <= 30))
					{
						// set some time to wait & watch
						f_bot_wait_for_enemy_time = gpGlobals->time + RANDOM_FLOAT(3.0, 6.0);					
						return (pBotEnemy);
					}
					// in 65% should defending bot keep watching that direction
					// and wait if enemy become visible again
					else if (IsBehaviour(DEFENDER) && (do_watch <= 65))
					{
						f_bot_wait_for_enemy_time = gpGlobals->time + RANDOM_FLOAT(8.0, 13.0);
						return (pBotEnemy);
					}
					// in 45% should common bot keep watching that direction
					// and wait if enemy become visible again
					else if (IsBehaviour(STANDARD) && (do_watch <= 45))
					{
						f_bot_wait_for_enemy_time = gpGlobals->time + RANDOM_FLOAT(5.0, 7.5);
						return (pBotEnemy);
					}
					
					
					//@@@@@@@@@@@@@@@
					//#ifdef _DEBUG
					//ALERT(at_console, "ENEMY ISN'T VISIBLE --->>> FORGET ABOUT IT!!!!\n");
					//#endif


					BotForgetEnemy();
					return (pBotEnemy);
				}
				// otherwise the bot is already waiting & watching
				else
				{
					// is the bot already w&w for some time
					// then see if we can break the watch and forget about the enemy
					if ((f_bot_wait_for_enemy_time - 2.0 < gpGlobals->time) &&
						IsBehaviour(ATTACKER) && (RANDOM_LONG(1, 100) < 10))
					{
						// is the enemy far and is the right time
						if ((RANDOM_LONG(1, 100) < 35) &&
							((v_last_enemy_position - pEdict->v.origin).Length() > 300))
						{							
#ifdef _DEBUG
							ALERT(at_console, "<%s>***enemy still not visible - breaking the watch\n", name);
#endif
							BotForgetEnemy();
							return (pBotEnemy);
						}
					}
					else if ((f_bot_wait_for_enemy_time - 6.0 < gpGlobals->time) &&
						IsBehaviour(DEFENDER) && (RANDOM_LONG(1, 100) < 2))
					{
						if ((RANDOM_LONG(1, 100) < 5) &&
							((v_last_enemy_position - pEdict->v.origin).Length() > 300))
						{
#ifdef _DEBUG
							ALERT(at_console, "<%s>***enemy still not visible - breaking the watch\n", name);
#endif
							BotForgetEnemy();
							return (pBotEnemy);
						}
					}
					else if ((f_bot_wait_for_enemy_time - 3.0 < gpGlobals->time) &&
						IsBehaviour(STANDARD) && (RANDOM_LONG(1, 100) < 5))
					{
						if ((RANDOM_LONG(1, 100) < 20) &&
							((v_last_enemy_position - pEdict->v.origin).Length() > 300))
						{
#ifdef _DEBUG
							ALERT(at_console, "<%s>***enemy still not visible - breaking the watch\n", name);
#endif
							BotForgetEnemy();
							return (pBotEnemy);
						}
					}

					// the bot decided to continue w&w ...


//@@@@@@@@@@@@@
#ifdef _DEBUG
					if (v_last_enemy_position == Vector(0, 0, 0))
					{
						ALERT(at_console, "***<%s> last known position is (0,0,0)\n", name);
						char smsg[256];
						sprintf(smsg, "<%s> last known position of <%s> is (0,0,0)\n", name,
							STRING(pBotEnemy->v.netname));
						UTIL_DebugDev(smsg);
					}
#endif
					
					// should the bot search for different enemy?
					if (IsTask(TASK_FIND_ENEMY))
					{
						pPrevEnemy = pBotEnemy;	// safe pointer to your current enemy
					}
					// otherwise keep facing the current one
					else
					{
						// face the last known position of this enemy
						Vector v_enemy_last_p = v_last_enemy_position - pEdict->v.origin;
						Vector bot_angles = UTIL_VecToAngles(v_enemy_last_p);
						
						pEdict->v.ideal_yaw = bot_angles.y;
						BotFixIdealYaw(pEdict);
						
						DontSeeEnemyActions(this);
						return (pBotEnemy);
					}
				}				
			}	// END enemy isn't fully visible
		}	// END enemy is alive
	}	// END pBot->pBotEnemy != NULL

	pNewEnemy = NULL;

	if (pNewEnemy == NULL)
	{
		// if the bot already has an enemy try to find someone else who's closer
		if (pPrevEnemy != NULL)
		{
			if (f_bot_wait_for_enemy_time < gpGlobals->time)
				nearest_distance = (pPrevEnemy->v.origin - pEdict->v.origin).Length();
			// the wait & watch case
			else
				nearest_distance = (v_last_enemy_position - pEdict->v.origin).Length();
		}
		// do we limit max enemy distance?
		else if (internals.IsDistLimit())
		{
			nearest_distance = internals.GetMaxDistance();
		}
		else
		{
			// is the bot a sniper bot then there's no range limit to search for an enemy
			if (UTIL_IsSniperRifle(current_weapon.iId))
				nearest_distance = 9999;
			// otherwise search for "close" enemies
			else
				nearest_distance = internals.GetMaxDistance();
		}

		// tweak the distance based on situation
		if (IsTask(TASK_IGNORE_ENEMY))
		{
			nearest_distance = RANGE_MELEE;
		}
		else if (IsTask(TASK_AVOID_ENEMY))
		{
			nearest_distance = GetEffectiveRange();

			if (nearest_distance < RANGE_MELEE)
				nearest_distance = RANGE_MELEE;
		}

		if (IsTask(TASK_GOALITEM))
			nearest_distance = ENEMY_DIST_GOALITEM;

		// search the world for players
		for (clients = 1; clients <= gpGlobals->maxClients; clients++)
		{
			edict_t *pPlayer = INDEXENT(clients);

			// skip invalid players AND skip self (i.e. this bot)
			if ((pPlayer) && (!pPlayer->free) && (pPlayer != pEdict))
			{
				// skip this player if not alive (i.e. dead or dying)
				if (!IsAlive(pPlayer))
					continue;

				if ((b_observer_mode) && !(pPlayer->v.flags & FL_FAKECLIENT))
					continue;

				if (!checked_teamplay)  // check for team play...
					BotCheckTeamplay();

				// is team play enabled?
				if (is_team_play > 0.0)
				{
					int player_team = UTIL_GetTeam(pPlayer);
					int bot_team = UTIL_GetTeam(pEdict);

					// don't target your teammates or players from unknown team
					if ((bot_team == player_team) || (player_team == TEAM_NONE))
						continue;
				}

				// get the distance
				float player_distance = (pPlayer->v.origin - pEdict->v.origin).Length();

				// skip players that are farther than nearest distance limit
				if (player_distance > nearest_distance)
					continue;

				vecEnd = pPlayer->v.origin + pPlayer->v.view_ofs;

				bool IsVisible = false;
				int decide_visibility = FPlayerVisible( vecEnd, pEdict );
				
				switch (decide_visibility)
				{
					case VIS_NO:
						IsVisible = false;
						break;
					case VIS_YES:
						IsVisible = true;
						break;
					case VIS_FENCE:
						if (UTIL_IsMachinegun(current_weapon.iId) || UTIL_IsSniperRifle(current_weapon.iId) ||
							UTIL_IsAssaultRifle(current_weapon.iId))
							IsVisible = true;
						else
							IsVisible = false;
						break;
				}

				// see if bot can see the player
				if (FInViewCone( &vecEnd, pEdict ) && IsVisible)
				{
					// this player must be closer so update the distance limit
					nearest_distance = player_distance;
					pNewEnemy = pPlayer;

					pBotUser = NULL;  // don't follow user when enemy found
				}
			}
		}
	}

	if (pNewEnemy)
	{
		// face the enemy
		Vector v_enemy = pNewEnemy->v.origin - pEdict->v.origin;
		Vector bot_angles = UTIL_VecToAngles( v_enemy );

		pEdict->v.ideal_yaw = bot_angles.y;
		BotFixIdealYaw(pEdict);

		SetSubTask(ST_FACEENEMY);

		// had the bot an enemy even before AND was it exactly this one
		// so break it right now
		if ((pPrevEnemy != NULL) && (pNewEnemy == pPrevEnemy))
			return (pPrevEnemy);

		// had the bot an enemy before then we have to clear all values
		if (pPrevEnemy != NULL)
			BotForgetEnemy();

		// has to be here because the reaction code uses it
		// and if we didn't update it with the new enemy the rections would be set incorrectly
		pBotEnemy = pNewEnemy;

		// apply reaction_time
		BotReactions(this);

		// store enemy last known (visible) origin
		// the enemy must be visible at this moment otherwise the bot wasn't able to target him
		v_last_enemy_position = pBotEnemy->v.origin;

		// keep track of when we last saw an enemy
		f_bot_see_enemy_time = gpGlobals->time;

		// init advance time override ability
		f_overide_advance_time = gpGlobals->time + RANDOM_FLOAT(0.0, 1.5);

		// warn comrades only if had no enemy before
		// (i.e. if completely new enemy) AND is chance to speak
		if (!externals.GetDontSpeak() && (pPrevEnemy == NULL) &&
			((speak_time + RANDOM_FLOAT(25.5, 59.0)) < gpGlobals->time)
			&& (RANDOM_LONG(1, 100) <= 50))
		{
			UTIL_Voice(pEdict, "getdown");
			speak_time = gpGlobals->time;
		}

		// if the bot is on PATROL path set return flag
		if ((patrol_path_wpt != -1) && !IsTask(TASK_BACKTOPATROL))
		{
			SetTask(TASK_BACKTOPATROL);

#ifdef _DEBUG
			//@@@@@
			extern bool debug_paths;
			if (debug_paths)
				ALERT(at_console, "PATROL path return flag set on\n");
#endif
		}

		// we have to clear dontmove flag ie if bot is NOT near any of small range wpts or
		// his waypoint isn't dont move waypoint
		int last_wpt = prev_wpt_index.get();

		if (IsTask(TASK_DONTMOVE) && (crowded_wpt_index == -1) &&
			(UTIL_IsSmallRange(curr_wpt_index) == FALSE) && // the bot is heading towards to it
			(UTIL_IsSmallRange(last_wpt) == FALSE) && // is standing at it
			(UTIL_IsDontMoveWpt(pEdict, curr_wpt_index, FALSE) == FALSE) &&
			(UTIL_IsDontMoveWpt(pEdict, last_wpt, TRUE) == FALSE))
		{
			RemoveTask(TASK_DONTMOVE);


#ifdef _DEBUG
			//@@@@@@@@@@@@@@@@@@@@@
			ALERT(at_console, "DontMove cleared -- enemy is far\n");
#endif
		}

		pPrevEnemy = NULL;	// null out pointer just for sure

		//@@@@@@@@@@@@@@@
		//#ifdef _DEBUG
		//ALERT(at_console, "<%s> *** GOT NEW ENEMY! His name is %s\n", name,
		//	STRING(pBotEnemy->v.netname));
		//#endif
	}

	DontSeeEnemyActions(this);

	// is some time after we saw an enemy so reset it
	// to prevent doing all those things (speaking etc.) again and again
	if ((f_bot_see_enemy_time > 0) && ((f_bot_see_enemy_time + 15.0) < gpGlobals->time))
		f_bot_see_enemy_time = -1;

	return (pNewEnemy);
}

/*
* find best point to target based on visiblity and aim skill level
*/
/*
Vector BotBodyTarget(bot_t *pBot)
{
	Vector target;
	float foe_distance;
	float f_scale = 0.1;
	int d_x, d_y, d_z;
	int hs_percentage;		// precentage chance to do head shot

	edict_t *pEdict = pBot->pEdict;
	edict_t *pBotEnemy = pBot->pBotEnemy;

	foe_distance = (pBotEnemy->v.origin - pEdict->v.origin).Length();

	// random the head shot precentage
	hs_percentage = RANDOM_LONG(1, 100);

	// if bot using sniper rifle make him more accurate
	if (UTIL_IsSniperRifle(pBot->current_weapon.iId))
	{
		if (foe_distance > 2500)
			f_scale = 0.5;
		else if (foe_distance > 100)
			f_scale = foe_distance / (float) 10000.0;

		switch (pBot->aim_skill)
		{
			case 0:
				// VERY GOOD, same as from CBasePlayer::BodyTarget (in player.h)
				target = pBotEnemy->v.origin + pBotEnemy->v.view_ofs * RANDOM_FLOAT( 0.2, 0.5 );
				d_x = 0;  // no offset
				d_y = 0;
				d_z = 0;
				break;
			case 1:
				// GOOD, offset a little for x, y, and z
				if (hs_percentage > 15)
					target = pBotEnemy->v.origin + pBotEnemy->v.view_ofs;  // aim for the head (if you can find it)
				else
					target = pBotEnemy->v.origin;  // aim only for the body
				d_x = RANDOM_FLOAT(-1, 1) * f_scale;
				d_y = RANDOM_FLOAT(-1, 1) * f_scale;
				d_z = RANDOM_FLOAT(-2, 2) * f_scale;
				break;
			case 2:
				// FAIR, offset somewhat for x, y, and z
				if (hs_percentage > 50)
					target = pBotEnemy->v.origin + pBotEnemy->v.view_ofs;
				else
					target = pBotEnemy->v.origin;
				d_x = RANDOM_FLOAT(-5, 5) * f_scale;
				d_y = RANDOM_FLOAT(-5, 5) * f_scale;
				d_z = RANDOM_FLOAT(-9, 9) * f_scale;
				break;
			case 3:
				// POOR, offset for x, y, and z
				if (hs_percentage > 75)
					target = pBotEnemy->v.origin + pBotEnemy->v.view_ofs;
				else
					target = pBotEnemy->v.origin;
				d_x = RANDOM_FLOAT(-10, 10) * f_scale;
				d_y = RANDOM_FLOAT(-10, 10) * f_scale;
				d_z = RANDOM_FLOAT(-15, 15) * f_scale;
				break;
			case 4:
				// BAD, offset lots for x, y, and z
				target = pBotEnemy->v.origin;		// aim only for the body
				d_x = RANDOM_FLOAT(-17, 17) * f_scale;
				d_y = RANDOM_FLOAT(-17, 17) * f_scale;
				d_z = RANDOM_FLOAT(-23, 23) * f_scale;
				break;
		}
	}
	// otherwise use this
	else
	{
		if (foe_distance > 1000)
			f_scale = 1.0;
		else if (foe_distance > 100)
			f_scale = foe_distance / (float) 1000.0;

		// aim at head and use little offset to simulate human player
		// (none can shoot to same point all the time)
		if (foe_distance <= 100)
		{
			target = pBotEnemy->v.origin + pBotEnemy->v.view_ofs;
			d_x = RANDOM_FLOAT(-2, 2) * f_scale;
			d_y = RANDOM_FLOAT(-2, 2) * f_scale;
			d_z = RANDOM_FLOAT(-2, 2) * f_scale;
		}
		// otherwise use standard skill based aiming
		else
		{
			switch (pBot->aim_skill)
			{
				case 0:
					// VERY GOOD, same as from CBasePlayer::BodyTarget (in player.h)
					target = pBotEnemy->v.origin + pBotEnemy->v.view_ofs * RANDOM_FLOAT( 0.4, 1.0 );
					d_x = 0;  // no offset
					d_y = 0;
					d_z = 0;
					break;
				case 1:
					// GOOD, offset a little for x, y, and z
					if (hs_percentage > 30)
						target = pBotEnemy->v.origin + pBotEnemy->v.view_ofs;
					else
						target = pBotEnemy->v.origin;
					d_x = RANDOM_FLOAT(-2.5, 2.5) * f_scale;
					d_y = RANDOM_FLOAT(-2.5, 2.5) * f_scale;
					d_z = RANDOM_FLOAT(-5, 5) * f_scale;
					break;
				case 2:
					// FAIR, offset somewhat for x, y, and z
					if (hs_percentage > 70)
						target = pBotEnemy->v.origin + pBotEnemy->v.view_ofs;
					else
						target = pBotEnemy->v.origin;
					d_x = RANDOM_FLOAT(-7, 7) * f_scale;
					d_y = RANDOM_FLOAT(-7, 7) * f_scale;
					d_z = RANDOM_FLOAT(-13, 13) * f_scale;
					break;
				case 3:
					// POOR, offset for x, y, and z
					if (hs_percentage > 90)
						target = pBotEnemy->v.origin + pBotEnemy->v.view_ofs;
					else
						target = pBotEnemy->v.origin;
					d_x = RANDOM_FLOAT(-15, 15) * f_scale;
					d_y = RANDOM_FLOAT(-15, 15) * f_scale;
					d_z = RANDOM_FLOAT(-20, 20) * f_scale;
					break;
				case 4:
					// BAD, offset lots for x, y, and z
					target = pBotEnemy->v.origin;
					d_x = RANDOM_FLOAT(-24, 24) * f_scale;
					d_y = RANDOM_FLOAT(-24, 24) * f_scale;
					d_z = RANDOM_FLOAT(-30, 30) * f_scale;
					break;
			}
		}
	}

	// add offset to initial aim vector
	target = target + Vector(d_x, d_y, d_z);

	// fix the aim vector when using one of these weapons
	// (ie. aim a bit higher than head or body is)
	if ((pBot->current_weapon.iId == fa_weapon_frag) ||
		(pBot->current_weapon.iId == fa_weapon_concussion))
	{
		// is bot proned
		if (pEdict->v.flags & FL_PRONED)
			target = target + Vector(0, 0, 30);
		else
			target = target + Vector(0, 0, 15);
	}
	// or these
	else if ((pBot->current_weapon.iId == fa_weapon_m79) ||
			((pBot->current_weapon.iId == fa_weapon_m16) && pBot->secondary_active) ||
			((pBot->current_weapon.iId == fa_weapon_ak74) && pBot->secondary_active))
	{
		// check the distance to the enemy
		if (pBot->f_prev_enemy_dist > 1000)
		{
			if (pEdict->v.flags & FL_PRONED)
				target = target + Vector(0, 0, 70);
			else
				target = target + Vector(0, 0, 50);
		}
		else if (pBot->f_prev_enemy_dist > 700)
		{
			if (pEdict->v.flags & FL_PRONED)
				target = target + Vector(0, 0, 55);
			else
				target = target + Vector(0, 0, 45);
		}
		else if (pBot->f_prev_enemy_dist > 500)
		{
			if (pEdict->v.flags & FL_PRONED)
				target = target + Vector(0, 0, 35);
			else
				target = target + Vector(0, 0, 25);
		}
		else
		{
			if (pEdict->v.flags & FL_PRONED)
				target = target + Vector(0, 0, 15);
		}
	}


//@@@@@@@@@@@@@@@@@@
#ifdef _DEBUG
	pBot->curr_aim_location = target;
#endif



	return target;
}
*/


//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// NEW CODE


/*
* returns distance to 0 or 180 degree (whichever is closer) in form of zero based modifier
* negative yaw returns negative modifier
* anything outside -180 to +180 range returns zero
*/
float ModYawAngle(float yaw_angle)
{
	if ((yaw_angle > 0 && yaw_angle < 90) || (yaw_angle < 0 && yaw_angle > -90))
		return (yaw_angle / 100);

	if (yaw_angle >= 90)
		return (180 - yaw_angle) / 100;

	if (yaw_angle <= -90)
		return (180 + yaw_angle) / -100;

	return 0.0;
}


/*
* returns absolute angle distance to 90 degrees
*/
float AngleYaw90(float yaw_angle)
{
	return fabs(90 - fabs(yaw_angle));
}


/*
* returns inverted absolute distance to 90 degree in form of zero based modifier
* so yaw +/-90 returns 1.0
*/
float InvModYaw90(float yaw_angle)
{
	return 1 - (AngleYaw90(yaw_angle) / 100);
}


/*
* return rotated yaw angle where 0 become 180
* the negative angles become positive and vice versa
*/
float RotateYawAngle(float yaw_angle)
{
	float new_angle = 0.0;

	if (yaw_angle >= 0 && yaw_angle <= 180)
		new_angle = -180 + yaw_angle;
	else
		new_angle = 180 - fabs(yaw_angle);

	return new_angle;
}


/*
* return fake origin modifier for given weapon and given exposure
* exposure can be 0 to 3 and represent face, back, right hand, left hand in this order
*/
float FindWeaponModifier(int weapon, int exposure = 0)
{
	// check validity first
	if (exposure < 0 || exposure > 3 || weapon == default_ID)
		return 0.0;

	float w_mod[4] = {0, 0, 0, 0};

	if (weapon == fa_weapon_ak47 || weapon == fa_weapon_ak74 || weapon == fa_weapon_ber93r || weapon == fa_weapon_famas ||
		weapon == fa_weapon_m16 || weapon == fa_weapon_m4 || weapon == fa_weapon_m82 || weapon == fa_weapon_svd)
	{
		w_mod[0] = 7.25; w_mod[1] = 19.25; w_mod[2] = 22.5; w_mod[3] = 3.5;
	}
	else if (weapon == fa_weapon_ber92f || weapon == fa_weapon_desert || weapon == fa_weapon_mp5a5 || weapon == fa_weapon_uzi)
	{
		w_mod[0] = 14; w_mod[1] = 26; w_mod[2] = 29.75; w_mod[3] = 10.5;
	}
	else if (weapon == fa_weapon_coltgov)
	{
		w_mod[0] = 18.75; w_mod[1] = 24.5; w_mod[2] = 26; w_mod[3] = 14.25;
	}
	else if (weapon == fa_weapon_g36e)
	{
		w_mod[0] = 41.25; w_mod[1] = 51.5; w_mod[2] = 54.75; w_mod[3] = 38.5;
	}
	else if (weapon == fa_weapon_g3a3)
	{
		w_mod[0] = 0.25; w_mod[1] = 12.5; w_mod[2] = 16; w_mod[3] = -2.75;
	}
	else if (weapon == fa_weapon_bizon || weapon == fa_weapon_sterling)
	{
		w_mod[0] = 21; w_mod[1] = 32; w_mod[2] = 36; w_mod[3] = 17;
	}
	else if (weapon == fa_weapon_m14)
	{
		w_mod[0] = 27.75; w_mod[1] = 38.5; w_mod[2] = 43.75; w_mod[3] = 23;
	}
	else if (weapon == fa_weapon_m249 || weapon == fa_weapon_m60 || weapon == fa_weapon_pkm)
	{
		w_mod[0] = 47.75; w_mod[1] = 58; w_mod[2] = 59.5; w_mod[3] = 47.5;
	}
	else if (weapon == fa_weapon_ssg3000)
	{
		w_mod[0] = -5.75; w_mod[1] = 6; w_mod[2] = 9.25; w_mod[3] = -4.5;
	}
	// all undone weapons can use these values for now
	else
	{
		w_mod[0] = 7.25; w_mod[1] = 19.25; w_mod[2] = 22.5; w_mod[3] = 3.5;
	}

	return w_mod[exposure];
}


/*
* return modifier based on weapon and stance
*/
float FindStanceMod(int weapon, int stance)
{
	// default stance mod doesn't modify anything
	float s_mod = 1.0;

	if (stance == BOT_CROUCHED)
	{
		if (weapon == fa_weapon_m14)
			s_mod = 0.5045;
		else if (weapon == fa_weapon_g36e)
			s_mod = 0.6727;
		else if (UTIL_IsMachinegun(weapon))
			s_mod = 0.7225;
		else
			s_mod = 0.3778;
	}
	else if (stance == BOT_PRONED)
	{
		if (weapon == fa_weapon_m14)
			s_mod = 0.2703;
		else if (weapon == fa_weapon_g36e)
			s_mod = 0.503;
		else if (UTIL_IsMachinegun(weapon))
			s_mod = 0.1571;
		else
			s_mod = 0.4;
	}

	return s_mod;
}


/*
* return distance modifier based on weapon
*/
float FindDistanceMod(int weapon)
{
	// default mod doesn't modify anything
	float d_mod = 1.0;

	if (weapon == fa_weapon_m14)
		d_mod = 1.0;
	else if (weapon == fa_weapon_g36e)
		d_mod = 1.0;
	else if (UTIL_IsMachinegun(weapon))
		d_mod = 1.0;
	else if (weapon == fa_weapon_bizon || weapon == fa_weapon_sterling)
		d_mod = 1.15;
	else
		d_mod = 0.6;	// tested on m16
	
	return d_mod;
}


/*
* return exposure based on the angle
* return values are 0 = face, 1 = back, 2 = right hand, 3 = left hand
* if player entity spawn angles are rotated (ie. forward is 180) we need to rotate them back to forward == 0 degree
*/
int FindExposure(float hit_angle)
{
	// always keep the angles valid (ie. from -180 to +180)
	if (hit_angle > 180)
		hit_angle -= 360;
	
	if (hit_angle < -180)
		hit_angle += 360;

	if ((hit_angle >= -45 && hit_angle < 0) || (hit_angle >= 0 && hit_angle <= 45))
		return 0;

	if (hit_angle > -135 && hit_angle < -45)
		return 3;
	
	if (hit_angle > 45 && hit_angle < 135)
		return 2;

	if ((hit_angle >= -180 && hit_angle < -135) || (hit_angle > 135 && hit_angle <= 180))
		return 1;

	// if things went wrong return face exposure as the most possible option
	return 0;
}


/*
* make fake origin coordinate based on the angle at which the target is going to be hit
* for Firearms version 2.9 and 3.0 where the hitzones are literally dancing around the player
* if player entity spawn angles are rotated (ie. forward is 180) we need to rotate them back to forward == 0 degree
*/
float MakeFakeOriginCoordinate(float hit_angle, int weapon_ID = default_ID)
{
	float h_face = FindWeaponModifier(weapon_ID, 0);
	float h_back = FindWeaponModifier(weapon_ID, 1);
	float h_right = FindWeaponModifier(weapon_ID, 2);
	float h_left = FindWeaponModifier(weapon_ID, 3);
	float one_degree = 0.0;

	// always keep the angles valid (ie. from -180 to +180)
	if (hit_angle > 180)
		hit_angle -= 360;
	
	if (hit_angle < -180)
		hit_angle += 360;

	// set the default vector for face to face situation (ie. the angle is zero)
	float h_mod = h_face;

	// increase the vector when turning from face to right side
	if (hit_angle > 0 && hit_angle <= 90)
	{
		// find the one degree difference between face and right side
		one_degree = (h_right - h_face) / 90;
		// and increase the vector based on the angle
		h_mod = h_face + hit_angle * one_degree;
	}
	// decrease the vector when turning from right side to back
	else if (hit_angle > 90 && hit_angle <= 180)
	{
		one_degree = (h_right - h_back) / 90;
		h_mod = h_right - (hit_angle - 90) * one_degree;
	}
	// decrease the vector when turning from face to left side
	else if (hit_angle < 0 && hit_angle >= -90)
	{
		one_degree = (h_face - h_left) / 90;
		h_mod = h_face - fabs(hit_angle) * one_degree;
	}
	// increase the vector when turning from left side to back
	else if (hit_angle < -90 && hit_angle >= -180)
	{
		one_degree = (h_back - h_left) / 90;
		h_mod = h_left + fabs(hit_angle + 90) * one_degree;
	}

	return h_mod;
}

#ifdef _DEBUG
// used for testing the aiming system some code is also in dll.cpp
float modif = 0.0;
float x_coord_mod = 0.0;
float y_coord_mod = 0.0;
bool dbl_coords = false;
bool mod_coords = false;
bool inv_coords = false;
bool test_code = false;
//bool ignore_aim_error_code = false;
bool ignore_aim_error_code = true;			// << for tests I need headshots all the time
float modifier = 1;
#endif

/*
* find best point to target based on visiblity and aim skill level
*/
Vector BotBodyTarget(bot_t *pBot)
{
	Vector target, target_origin, target_head;
	float foe_distance;
	float f_scale = 0.1;
	int d_x, d_y, d_z;
	int hs_percentage;		// precentage chance to do head shot
	float x_corr, y_corr;	// origin modifier default value is for distance around 1000 units
	bool use_aim_patch_v1 = false;		// NEW CODE
	bool use_aim_patch_v2 = false;		// NEW CODE

	edict_t *pEdict = pBot->pEdict;
	edict_t *pBotEnemy = pBot->pBotEnemy;

	foe_distance = (pBotEnemy->v.origin - pEdict->v.origin).Length();

	// first decide which weapon works in these versions and which doesn't
	if (g_mod_version == FA_29 || g_mod_version == FA_30)
	{
		int weapon = pBot->current_weapon.iId;

		// these 3 seem to always work in FA2.9 and above
		// also knife, grenades and the grenade launcher don't need any aim patch
		if (UTIL_IsShotgun(weapon) || weapon == fa_weapon_anaconda || weapon == fa_weapon_knife ||
			weapon == fa_weapon_concussion || weapon == fa_weapon_frag || weapon == fa_weapon_m79)
			;
		// sniper rifles don't need aim patch when the bot is proned
		else if (UTIL_IsSniperRifle(weapon) && pEdict->v.flags & FL_PRONED)
			;
		// all the other weapons need to be patched
		else
		{
			use_aim_patch_v1 = true;
		}
	}
	
	// special code to aim correctly ... well at least better than without it cause there are still some bugs in there
	if (use_aim_patch_v1)
	{
		Vector v_fake_origin = pBot->pBotEnemy->v.origin;	
		float hit_angle = 0.0;
		float w_mod = 0.0;

		x_corr = 0;
		y_corr = 0;

		int weapon = pBot->current_weapon.iId;
		float bot_yaw_angle = pBot->pEdict->v.v_angle.y;
		float target_yaw_angle = pBot->pBotEnemy->v.v_angle.y;


#ifdef _DEBUG
		// test stuff
		x_corr = x_coord_mod;
		y_corr = y_coord_mod;

		if (test_code)
		{
#endif


		// unique aiming code for m14
		if (weapon == fa_weapon_m14)
		{
			//		QUADRANT 0 TO -90
			if (bot_yaw_angle < 0 && bot_yaw_angle >= -90)
			{
				hit_angle = RotateYawAngle(target_yaw_angle) - bot_yaw_angle;
				w_mod = FindWeaponModifier(weapon, FindExposure(hit_angle));
				
				x_corr = fabs(InvModYaw90(bot_yaw_angle) * w_mod) * -1.0;
				y_corr = fabs(MakeFakeOriginCoordinate(hit_angle, weapon)) * -1.0;
			}
			
			//		QUADRANT 0 TO +90
			else if (bot_yaw_angle >= 0 && bot_yaw_angle <= 90)
			{
				hit_angle = RotateYawAngle(target_yaw_angle) - bot_yaw_angle;
				w_mod = FindWeaponModifier(weapon, FindExposure(hit_angle));
				
				x_corr = fabs(InvModYaw90(bot_yaw_angle) * w_mod);
				y_corr = fabs(MakeFakeOriginCoordinate(hit_angle, weapon)) * -1.0;
			}
			
			//		QUADRANT +180 TO +90
			if (bot_yaw_angle > 90 && bot_yaw_angle <= 180)
			{
				hit_angle = target_yaw_angle - RotateYawAngle(bot_yaw_angle);
				w_mod = FindWeaponModifier(weapon, FindExposure(hit_angle));
				
				x_corr = fabs(InvModYaw90(bot_yaw_angle) * w_mod);
				y_corr = fabs(MakeFakeOriginCoordinate(hit_angle, weapon));
			}
			
			//		QUADRANT -180 TO -90
			else if (bot_yaw_angle >= -180 && bot_yaw_angle < -90)
			{
				hit_angle = target_yaw_angle - RotateYawAngle(bot_yaw_angle);
				w_mod = FindWeaponModifier(weapon, FindExposure(hit_angle));
				
				x_corr = fabs(InvModYaw90(bot_yaw_angle) * w_mod) * -1.0;
				y_corr = fabs(MakeFakeOriginCoordinate(hit_angle, weapon));
			}
		}	// the end of code for m14

		// unique aiming code for g36e
		else if (weapon == fa_weapon_g36e)
		{
			//		QUADRANT 0 TO -90
			if (bot_yaw_angle < 0 && bot_yaw_angle >= -90)
			{
				hit_angle = RotateYawAngle(target_yaw_angle) - bot_yaw_angle;
				
				if (bot_yaw_angle > -16)
					x_corr = 6.5 * -1.0;
				else if (bot_yaw_angle > -40)
					x_corr = 13.5 * -1.0;
				else if (bot_yaw_angle > -45)
					x_corr = 18.5 * -1.0;
				else
					x_corr = fabs(MakeFakeOriginCoordinate(hit_angle, weapon)) * -1.0;
				
				if (bot_yaw_angle > -45)
					y_corr = fabs(MakeFakeOriginCoordinate(hit_angle, weapon)) * -1.0;
				else if (bot_yaw_angle > -65)
					y_corr = 13.5 * -1.0;
				else
					y_corr = 6.5 * -1.0;
			}
			
			//		QUADRANT 0 TO +90
			else if (bot_yaw_angle >= 0 && bot_yaw_angle <= 90)
			{
				hit_angle = RotateYawAngle(target_yaw_angle) - bot_yaw_angle;
				
				if (bot_yaw_angle < 25)
					x_corr = 6.5;
				else if (bot_yaw_angle < 43)
					x_corr = 13.5;
				else if (bot_yaw_angle < 50)
					x_corr = 18.5;
				else
					x_corr = fabs(MakeFakeOriginCoordinate(hit_angle, weapon));
				
				if (bot_yaw_angle < 50)
					y_corr = fabs(MakeFakeOriginCoordinate(hit_angle, weapon)) * -1.0;
				else
					y_corr = 13.5 * -1.0;
			}
			
			//		QUADRANT +180 TO +90
			if (bot_yaw_angle > 90 && bot_yaw_angle <= 180)
			{
				hit_angle = target_yaw_angle - RotateYawAngle(bot_yaw_angle);

				if (bot_yaw_angle > 140)
					x_corr = 13.5;
				else if (bot_yaw_angle > 135)
					x_corr = 18.5;
				else
					x_corr = fabs(MakeFakeOriginCoordinate(hit_angle, weapon));
				
				if (bot_yaw_angle > 135)
					y_corr = fabs(MakeFakeOriginCoordinate(hit_angle, weapon));
				else if (bot_yaw_angle > 115)
					y_corr = 13.5;
				else
					y_corr = 6.5;
			}
			
			//		QUADRANT -180 TO -90
			else if (bot_yaw_angle >= -180 && bot_yaw_angle < -90)
			{
				hit_angle = target_yaw_angle - RotateYawAngle(bot_yaw_angle);
				
				if (bot_yaw_angle < -155)
					x_corr = 6.5 * -1.0;
				else if (bot_yaw_angle < -140)
					x_corr = 13.5 * -1.0;
				else if (bot_yaw_angle < -129)
					x_corr = 17.5 * -1.0;
				else
					x_corr = fabs(MakeFakeOriginCoordinate(hit_angle, weapon)) * -1.0;
				
				if (bot_yaw_angle < -129)
					y_corr = fabs(MakeFakeOriginCoordinate(hit_angle, weapon));
				else
					y_corr = 13.5;
			}
		}	// the end of code for g36e

		// unigue code for the machineguns
		else if (UTIL_IsMachinegun(weapon))
		{
			//		QUADRANT 0 TO -90
			if (bot_yaw_angle < 0 && bot_yaw_angle >= -90)
			{
				hit_angle = RotateYawAngle(target_yaw_angle) - bot_yaw_angle;
				w_mod = fabs(bot_yaw_angle * 0.6) - 9.5;
				
				if (bot_yaw_angle >= -15)
					x_corr = 1.5 * -1.0;
				else if (bot_yaw_angle >= -20)
					x_corr = 3 * -1.0;
				else
					x_corr = fabs(w_mod) * -1.0;
				
				y_corr = fabs(MakeFakeOriginCoordinate(hit_angle, weapon)) * -1.0;
			}
			
			//		QUADRANT 0 TO +90
			else if (bot_yaw_angle >= 0 && bot_yaw_angle <= 90)
			{
				hit_angle = RotateYawAngle(target_yaw_angle) - bot_yaw_angle;
				w_mod = (bot_yaw_angle * 0.6) - 9.5;
				
				if (bot_yaw_angle <= 15)
					x_corr = 1.5;
				else if (bot_yaw_angle <= 20)
					x_corr = 3;
				else
					x_corr = fabs(w_mod);
				
				y_corr = fabs(MakeFakeOriginCoordinate(hit_angle, weapon)) * -1.0;
			}
			
			//		QUADRANT +180 TO +90
			if (bot_yaw_angle > 90 && bot_yaw_angle <= 180)
			{
				hit_angle = target_yaw_angle - RotateYawAngle(bot_yaw_angle);
				w_mod = fabs(RotateYawAngle(bot_yaw_angle) * 0.6) - 9.5;
				
				if (bot_yaw_angle >= 165)
					x_corr = 1.5;
				else if (bot_yaw_angle >= 160)
					x_corr = 3;
				else
					x_corr = fabs(w_mod);

				y_corr = fabs(MakeFakeOriginCoordinate(hit_angle, weapon));
			}
			
			//		QUADRANT -180 TO -90
			else if (bot_yaw_angle >= -180 && bot_yaw_angle < -90)
			{
				hit_angle = target_yaw_angle - RotateYawAngle(bot_yaw_angle);
				w_mod = fabs(RotateYawAngle(bot_yaw_angle) * 0.6) - 9.5;
				
				if (bot_yaw_angle <= -165)
					x_corr = 1.5 * -1.0;
				else if (bot_yaw_angle <= -160)
					x_corr = 3 * -1.0;
				else
					x_corr = fabs(w_mod) * -1.0;
				
				y_corr = fabs(MakeFakeOriginCoordinate(hit_angle, weapon));
			}
		}	// end of the code for machineguns

		// all other weapons
		else
		{
			// use the default face exposure modifier ... based on current weapon of course
			w_mod = FindWeaponModifier(weapon);

			//		QUADRANT 0 TO -90
			if (bot_yaw_angle < 0 && bot_yaw_angle >= -50)
			{
				hit_angle = RotateYawAngle(target_yaw_angle) - bot_yaw_angle;
				
				x_corr = fabs(InvModYaw90(bot_yaw_angle) * w_mod) * -1.0;
				y_corr = fabs(MakeFakeOriginCoordinate(hit_angle, weapon)) * -1.0;
			}
			else if (bot_yaw_angle < -50 && bot_yaw_angle > -90)
			{
				hit_angle = RotateYawAngle(target_yaw_angle) - bot_yaw_angle;
				
				x_corr = fabs(MakeFakeOriginCoordinate(hit_angle, weapon)) * -1.0;
				y_corr = fabs(MakeFakeOriginCoordinate(hit_angle, weapon)) * -1.0;
			}
			
			//		QUADRANT 0 TO +90
			else if (bot_yaw_angle >= 0 && bot_yaw_angle <= 50)
			{
				hit_angle = RotateYawAngle(target_yaw_angle) - bot_yaw_angle;
				
				x_corr = fabs(InvModYaw90(bot_yaw_angle) * w_mod);
				y_corr = fabs(MakeFakeOriginCoordinate(hit_angle, weapon)) * -1.0;
			}
			else if (bot_yaw_angle > 50 && bot_yaw_angle < 90)
			{
				hit_angle = RotateYawAngle(target_yaw_angle) - bot_yaw_angle;
				
				x_corr = fabs(MakeFakeOriginCoordinate(hit_angle, weapon));
				y_corr = fabs(MakeFakeOriginCoordinate(hit_angle, weapon)) * -1.0;
			}
			
			//		QUADRANT +180 TO +90
			if (bot_yaw_angle >= 130 && bot_yaw_angle <= 180)
			{
				hit_angle = target_yaw_angle - RotateYawAngle(bot_yaw_angle);
				
				x_corr = fabs(InvModYaw90(bot_yaw_angle) * w_mod);
				y_corr = fabs(MakeFakeOriginCoordinate(hit_angle, weapon));
			}
			else if (bot_yaw_angle > 90 && bot_yaw_angle < 130)
			{
				hit_angle = target_yaw_angle - RotateYawAngle(bot_yaw_angle);
				
				x_corr = fabs(MakeFakeOriginCoordinate(hit_angle, weapon));
				y_corr = fabs(MakeFakeOriginCoordinate(hit_angle, weapon));
			}
			
			//		QUADRANT -180 TO -90
			else if (bot_yaw_angle >= -180 && bot_yaw_angle <= -130)
			{
				hit_angle = target_yaw_angle - RotateYawAngle(bot_yaw_angle);
				
				x_corr = fabs(InvModYaw90(bot_yaw_angle) * w_mod) * -1.0;
				y_corr = fabs(MakeFakeOriginCoordinate(hit_angle, weapon));
			}
			else if (bot_yaw_angle > -130 && bot_yaw_angle < -90)
			{
				hit_angle = target_yaw_angle - RotateYawAngle(bot_yaw_angle);
				
				x_corr = fabs(MakeFakeOriginCoordinate(hit_angle, weapon)) * -1.0;
				y_corr = fabs(MakeFakeOriginCoordinate(hit_angle, weapon));
			}
		}	// the end of code for all other weapons


		/*
		// modify it based on distance to the target because the numbers are estimated on distanace == 1000.0

		if (foe_distance < 650 && foe_distance > 300)
		{
			x_corr = t_size.x * (foe_distance / 1000.0) * ModYawAngle(bot_yaw_angle);
			y_corr = t_size.y * (foe_distance / 1000.0);

			if (bot_yaw_angle > 110 && bot_yaw_angle < 125)
				x_corr = t_size.x * (foe_distance / 1000.0) * (1 - ModYawAngle(bot_yaw_angle));

			//@@@@@@@
			ALERT(at_console, "FOE close messing with the corrs! (x=%.2f y=%.2f)\n", x_corr, y_corr);
		}
		*/

		if (foe_distance > 1000)
		{
			x_corr = x_corr * (foe_distance / 1000.0) * FindDistanceMod(weapon);
			y_corr = y_corr * (foe_distance / 1000.0) * FindDistanceMod(weapon);
		}
		else
		{
			x_corr = x_corr * (foe_distance / 1000.0);
			y_corr = y_corr * (foe_distance / 1000.0);
		}

		// when the bot is crouched then modify the corrections by crouch factor
		if (pEdict->v.flags & FL_DUCKING)
		{
			x_corr *= FindStanceMod(weapon, BOT_CROUCHED);
			y_corr *= FindStanceMod(weapon, BOT_CROUCHED);
		}
		
		// when the bot is proned then modify the corrections by prone factor
		if (pEdict->v.flags & FL_PRONED)
		{
			x_corr *= FindStanceMod(weapon, BOT_PRONED);
			y_corr *= FindStanceMod(weapon, BOT_PRONED);
		}

		
#ifdef _DEBUG

		}		// end of test_code

		//@@@@
		if (dbl_coords)
		{
			x_corr *= 2.0;
			y_corr *= 2.0;
		}
		if (mod_coords && modifier != 0)
		{
			x_corr *= modifier;
			y_corr *= modifier;
		}
		if (inv_coords)
		{
			x_corr *= -1.0;
			y_corr *= -1.0;
		}


		//@@@@@@@
		ALERT(at_console, "corrs (x=%.2f y=%.2f) dist %.2f (dist/1000 %.2f) BYaw %.2f (rotated %.2f) TYaw %.1f (rotated %.2f) HitAngle %.2f\n",
			x_corr, y_corr, foe_distance, foe_distance / 1000.0, bot_yaw_angle, RotateYawAngle(bot_yaw_angle), target_yaw_angle,
			RotateYawAngle(target_yaw_angle), hit_angle);
#endif
		
		
		target_origin = v_fake_origin + Vector (x_corr, y_corr, 0);
		target_head = pBotEnemy->v.view_ofs;
	}
	// NOTE: this is only temporary solution, the code is really complicated, ugly and not 100% working
	// special code to aim correctly ... ehm better than without it cause there is still many bugs in there
	else if (use_aim_patch_v2 && !use_aim_patch_v1)
	{
		Vector t_size = pBot->pBotEnemy->v.size;
		Vector v_fake_origin = pBot->pBotEnemy->v.origin;
		float fake_origin_zcoord;

		
		/**/	// NEW CODE END
		


		// if the enemy is proned then aim slightly above the real origin z coord because when
		// the player is proned then the head is below the origin so if we aim a bit down then
		// the bot will hit ground instead of the enemy
		if (pBot->pBotEnemy->v.flags & FL_PRONED)
			//fake_origin_zcoord = (t_size.z * 0.55);	// worked quite fine
			fake_origin_zcoord = (t_size.z * 0.65);
		// otherwise aim slightly below real origin z coord
		else
			fake_origin_zcoord = (t_size.z * 0.4);

		// if the enemy is really close use common aiming (similar to origin based)
		if (foe_distance < 300)
			v_fake_origin = pBotEnemy->v.absmin + Vector((t_size.x * 0.5), (t_size.y * 0.5), fake_origin_zcoord);
		else
		{
			// is it right back side from default turning angle
			if ((pEdict->v.angles.y <= -90) && (pEdict->v.angles.y > -180))
			{
				if (foe_distance < 1000)
				{
					float dist_corr;
					dist_corr = foe_distance / 1000;
					x_corr = 0.5 - fabs(((180 - fabs(pEdict->v.angles.y)) / 200));
					x_corr += (dist_corr / 2.0);

					if ((pEdict->v.flags & FL_PRONED) || (pBot->pBotEnemy->v.flags & FL_PRONED))
					{
						y_corr = 0.5;

						if (UTIL_IsSMG(pBot->current_weapon.iId) ||
							UTIL_IsMachinegun(pBot->current_weapon.iId) ||
							(pBot->current_weapon.iId == fa_weapon_ber92f) ||
							(pBot->current_weapon.iId == fa_weapon_g36e))
							y_corr = 0.8;
					}
					else if (pEdict->v.flags & FL_DUCKING)
					{
						y_corr = 0.5 + dist_corr + fabs(((180 - fabs(pEdict->v.angles.y)) / 200));

						if (UTIL_IsSMG(pBot->current_weapon.iId) ||
							UTIL_IsMachinegun(pBot->current_weapon.iId) ||
							(pBot->current_weapon.iId == fa_weapon_ber92f) ||
							(pBot->current_weapon.iId == fa_weapon_g36e))
							y_corr = 0.8 + dist_corr + fabs(((180 - fabs(pEdict->v.angles.y)) / 200));
					}
					else
					{
						y_corr = 0.5 + dist_corr + fabs(((180 - fabs(pEdict->v.angles.y)) / 200));

						if (UTIL_IsSMG(pBot->current_weapon.iId) ||
							UTIL_IsMachinegun(pBot->current_weapon.iId) ||
							(pBot->current_weapon.iId == fa_weapon_ber92f) ||
							(pBot->current_weapon.iId == fa_weapon_g36e))
							y_corr = 0.8 + dist_corr + fabs(((180 - fabs(pEdict->v.angles.y)) / 200));
					}
				}
				else
				{
					float dist_corr = 0.5 * (foe_distance / 10000);

					if (UTIL_IsSMG(pBot->current_weapon.iId) ||
						UTIL_IsMachinegun(pBot->current_weapon.iId) ||
						(pBot->current_weapon.iId == fa_weapon_ber92f) ||
						(pBot->current_weapon.iId == fa_weapon_g36e) ||
						(pBot->current_weapon.iId == fa_weapon_m14))
						dist_corr = foe_distance / 10000;

					x_corr = 0.6 - fabs(((180 - fabs(pEdict->v.angles.y)) / 200));
					x_corr += dist_corr;

					if (pEdict->v.flags & FL_PRONED)
					{
						y_corr = 0.0 + (dist_corr * 10);

						if (pBot->current_weapon.iId == fa_weapon_pkm)
						{
							x_corr = -0.4 - (0.9 * fabs(((180 - fabs(pEdict->v.angles.y)) / 100)));
							y_corr = 1.7 * (dist_corr * 10);
						}
						if (pBot->current_weapon.iId == fa_weapon_m249)
						{
							x_corr = -0.5 - (1.7 * fabs(((180 - fabs(pEdict->v.angles.y)) / 100)));
							y_corr = 2.1 * (dist_corr * 10);
						}

						if (pBot->current_weapon.iId == fa_weapon_m60)
						{
							x_corr = -0.5 - (1.2 * fabs(((180 - fabs(pEdict->v.angles.y)) / 100)));
							y_corr = 1.6 * (dist_corr * 10);
						}
					}
					else if ((pEdict->v.flags & FL_DUCKING) || (pBot->pBotEnemy->v.flags & FL_PRONED))
					{
						y_corr = 0.45 + (dist_corr * 10);

						if (pBot->current_weapon.iId == fa_weapon_pkm)
						{
							x_corr = -0.5 - (1.1 * fabs(((180 - fabs(pEdict->v.angles.y)) / 100)));
							y_corr = 1.9 * (dist_corr * 10);
						}
						if (pBot->current_weapon.iId == fa_weapon_m249)
						{
							x_corr = -0.5 - (1.7 * fabs(((180 - fabs(pEdict->v.angles.y)) / 100)));
							y_corr = 2.3 * (dist_corr * 10);
						}

						if (pBot->current_weapon.iId == fa_weapon_m60)
						{
							x_corr = -0.5 - (1.2 * fabs(((180 - fabs(pEdict->v.angles.y)) / 100)));
							y_corr = 1.8 * (dist_corr * 10);
						}
					}
					else
					{
						y_corr = 1.05 + (dist_corr * 10);

						if (pBot->current_weapon.iId == fa_weapon_pkm)
						{
							x_corr = -1.5;							
							y_corr = 2.2 * (dist_corr * 10);
						}
						if (pBot->current_weapon.iId == fa_weapon_m249)
						{
							x_corr = -1.0 - (1.6 * fabs(((180 - fabs(pEdict->v.angles.y)) / 100)));
							y_corr = 2.6 * (dist_corr * 10);
						}

						if (pBot->current_weapon.iId == fa_weapon_m60)
						{
							x_corr = -0.5 - (1.2 * fabs(((180 - fabs(pEdict->v.angles.y)) / 100)));
							y_corr = 2.3 * (dist_corr * 10);
						}
					}
				}
			}
			
			// left back side
			if ((pEdict->v.angles.y >= 90) && (pEdict->v.angles.y < 180))
			{
				if (foe_distance < 1000)
				{
					x_corr = 1.15;

					if ((pEdict->v.flags & FL_PRONED) || (pBot->pBotEnemy->v.flags & FL_PRONED))
					{
						y_corr = 0.4;

						if (UTIL_IsSMG(pBot->current_weapon.iId) ||
							UTIL_IsMachinegun(pBot->current_weapon.iId) ||
							(pBot->current_weapon.iId == fa_weapon_ber92f) ||
							(pBot->current_weapon.iId == fa_weapon_g36e))
							y_corr = 0.7;
					}
					else if (pEdict->v.flags & FL_DUCKING)
					{
						y_corr = 0.45;

						if (UTIL_IsSMG(pBot->current_weapon.iId) ||
							UTIL_IsMachinegun(pBot->current_weapon.iId) ||
							(pBot->current_weapon.iId == fa_weapon_ber92f) ||
							(pBot->current_weapon.iId == fa_weapon_g36e))
							y_corr = 1.05;
					}
					else
					{
						y_corr = 1.15;
						
						if (UTIL_IsSMG(pBot->current_weapon.iId) ||
							UTIL_IsMachinegun(pBot->current_weapon.iId) ||
							(pBot->current_weapon.iId == fa_weapon_ber92f) ||
							(pBot->current_weapon.iId == fa_weapon_g36e))
							y_corr = 1.5;
					}
				}
				else
				{
					float dist_corr = 0.5 * (foe_distance / 10000);

					if (UTIL_IsSMG(pBot->current_weapon.iId) ||
						UTIL_IsMachinegun(pBot->current_weapon.iId) ||
						(pBot->current_weapon.iId == fa_weapon_ber92f) ||
						(pBot->current_weapon.iId == fa_weapon_g36e))
						dist_corr = foe_distance / 10000;

					x_corr = 1.15 + dist_corr;

					if ((pEdict->v.flags & FL_PRONED) || (pBot->pBotEnemy->v.flags & FL_PRONED))
					{
						y_corr = 0.05 + (dist_corr * 10);

						if (pBot->current_weapon.iId == fa_weapon_pkm)
							y_corr = 1.5 * (dist_corr * 10);
						if (pBot->current_weapon.iId == fa_weapon_m249)
							y_corr = 2.0 * (dist_corr * 10);
						if (pBot->current_weapon.iId == fa_weapon_m60)
							y_corr = 1.6 * (dist_corr * 10);
					}
					else if (pEdict->v.flags & FL_DUCKING)
					{
						y_corr = 0.2 + (dist_corr * 10);

						if ((pBot->current_weapon.iId == fa_weapon_pkm) ||
							(pBot->current_weapon.iId == fa_weapon_m60))
							y_corr = 1.8 * (dist_corr * 10);
						if (pBot->current_weapon.iId == fa_weapon_m249)
							y_corr = 2.25 * (dist_corr * 10);
					}
					else
					{
						y_corr = 1.15 + (dist_corr * 10);

						if ((pBot->current_weapon.iId == fa_weapon_pkm) ||
							(pBot->current_weapon.iId == fa_weapon_m60))
							y_corr = 2.25 * (dist_corr * 10);
						if (pBot->current_weapon.iId == fa_weapon_m249)
							y_corr = 2.6 * (dist_corr * 10);
					}
				}
			}
			
			
			
			// right front side
			if ((pEdict->v.angles.y < 0) && (pEdict->v.angles.y > -90))
			{
				if (foe_distance < 1000)
				{
					x_corr = 0.25;

					if (pEdict->v.flags & FL_PRONED)
					{
						y_corr = 0.4;

						if (UTIL_IsSMG(pBot->current_weapon.iId) ||
							UTIL_IsMachinegun(pBot->current_weapon.iId) ||
							(pBot->current_weapon.iId == fa_weapon_ber92f) ||
							(pBot->current_weapon.iId == fa_weapon_g36e))
							y_corr = -0.3;
					}
					else if ((pEdict->v.flags & FL_DUCKING) || (pBot->pBotEnemy->v.flags & FL_PRONED))
					{
						y_corr = 0.2;

						if (UTIL_IsSMG(pBot->current_weapon.iId) ||
							UTIL_IsMachinegun(pBot->current_weapon.iId) ||
							(pBot->current_weapon.iId == fa_weapon_ber92f) ||
							(pBot->current_weapon.iId == fa_weapon_g36e))
							y_corr = -0.7;
					}
					else
						y_corr = -0.25;

					if (UTIL_IsSMG(pBot->current_weapon.iId) ||
						UTIL_IsMachinegun(pBot->current_weapon.iId) ||
						(pBot->current_weapon.iId == fa_weapon_ber92f) ||
						(pBot->current_weapon.iId == fa_weapon_g36e))
						y_corr -= 0.25;
				}
				else
				{
					float dist_corr = 0.5 * (foe_distance / 10000);

					if (UTIL_IsSMG(pBot->current_weapon.iId) ||
						UTIL_IsMachinegun(pBot->current_weapon.iId) ||
						(pBot->current_weapon.iId == fa_weapon_ber92f) ||
						(pBot->current_weapon.iId == fa_weapon_g36e))
						dist_corr = foe_distance / 10000;

					x_corr = -0.25 - dist_corr;

					if (pEdict->v.flags & FL_PRONED)
					{
						y_corr = 0.5 - (dist_corr);

						if (UTIL_IsSMG(pBot->current_weapon.iId) ||
							UTIL_IsMachinegun(pBot->current_weapon.iId) ||
							(pBot->current_weapon.iId == fa_weapon_ber92f) ||
							(pBot->current_weapon.iId == fa_weapon_g36e))
							y_corr = -0.3 - dist_corr;
					}
					else if ((pEdict->v.flags & FL_DUCKING) || (pBot->pBotEnemy->v.flags & FL_PRONED))
					{
						y_corr = 1.05 - (dist_corr * 10);

						if (UTIL_IsSMG(pBot->current_weapon.iId) ||
							UTIL_IsMachinegun(pBot->current_weapon.iId) ||
							(pBot->current_weapon.iId == fa_weapon_ber92f) ||
							(pBot->current_weapon.iId == fa_weapon_g36e))
							y_corr = -0.7 - dist_corr;
					}
					else
					{
						y_corr = -0.25 - (dist_corr * 10);
						
						if (UTIL_IsSMG(pBot->current_weapon.iId) ||
							UTIL_IsMachinegun(pBot->current_weapon.iId) ||
							(pBot->current_weapon.iId == fa_weapon_ber92f) ||
							(pBot->current_weapon.iId == fa_weapon_g36e))
							y_corr = 0.0 - (dist_corr * 10);
					}
				}
			}
			
			// left front side
			if ((pEdict->v.angles.y >= 0) && (pEdict->v.angles.y < 90))
			{
				if (foe_distance < 1000)
				{
					x_corr = 0.5 + (pEdict->v.angles.y / 100);

					if (pEdict->v.flags & FL_PRONED)
					{
						y_corr = 0.75;

						if (UTIL_IsSMG(pBot->current_weapon.iId) ||
							UTIL_IsMachinegun(pBot->current_weapon.iId) ||
							(pBot->current_weapon.iId == fa_weapon_ber92f) ||
							(pBot->current_weapon.iId == fa_weapon_g36e))
							y_corr = -0.3;
					}
					else if ((pEdict->v.flags & FL_DUCKING) || (pBot->pBotEnemy->v.flags & FL_PRONED))
					{
						y_corr = 0.75;

						if (UTIL_IsSMG(pBot->current_weapon.iId) ||
							UTIL_IsMachinegun(pBot->current_weapon.iId) ||
							(pBot->current_weapon.iId == fa_weapon_ber92f) ||
							(pBot->current_weapon.iId == fa_weapon_g36e))
							y_corr = -0.2;
					}
					else
						y_corr = -0.45;

					if (UTIL_IsSMG(pBot->current_weapon.iId) ||
						UTIL_IsMachinegun(pBot->current_weapon.iId) ||
						(pBot->current_weapon.iId == fa_weapon_ber92f) ||
						(pBot->current_weapon.iId == fa_weapon_g36e))
						y_corr -= 0.15;
				}
				else
				{
					float dist_corr = 0.5 * (foe_distance / 10000);

					if (UTIL_IsSMG(pBot->current_weapon.iId) ||
						UTIL_IsMachinegun(pBot->current_weapon.iId) ||
						(pBot->current_weapon.iId == fa_weapon_ber92f) ||
						(pBot->current_weapon.iId == fa_weapon_g36e))
						dist_corr = foe_distance / 10000;

					x_corr = 0.5 + (pEdict->v.angles.y / 100) + (dist_corr * 2.0);

					if (pEdict->v.flags & FL_PRONED)
					{
						y_corr = 0.5 - dist_corr;

						if (UTIL_IsSMG(pBot->current_weapon.iId) ||
							UTIL_IsMachinegun(pBot->current_weapon.iId) ||
							(pBot->current_weapon.iId == fa_weapon_ber92f) ||
							(pBot->current_weapon.iId == fa_weapon_g36e))
							y_corr = -0.3 - dist_corr;
					}
					else if ((pEdict->v.flags & FL_DUCKING) || (pBot->pBotEnemy->v.flags & FL_PRONED))
					{
						y_corr = 1.05 - (dist_corr * 10);

						if (UTIL_IsSMG(pBot->current_weapon.iId) ||
							UTIL_IsMachinegun(pBot->current_weapon.iId) ||
							(pBot->current_weapon.iId == fa_weapon_ber92f) ||
							(pBot->current_weapon.iId == fa_weapon_g36e))
							y_corr = -0.7 - dist_corr;
					}
					else
					{
						y_corr = -0.45 - (dist_corr * 10);
						
						if (UTIL_IsSMG(pBot->current_weapon.iId) ||
							UTIL_IsMachinegun(pBot->current_weapon.iId) ||
							(pBot->current_weapon.iId == fa_weapon_ber92f) ||
							(pBot->current_weapon.iId == fa_weapon_g36e))
							y_corr = -0.2 - (dist_corr * 10);
					}
				}
			}

			v_fake_origin = pBot->pBotEnemy->v.absmin +
				Vector((t_size.x * x_corr), (t_size.y * y_corr), fake_origin_zcoord);
		}
		/**/

		target_origin = v_fake_origin;
		target_head = pBotEnemy->v.view_ofs;
	}
	else
	{
		target_origin = pBotEnemy->v.origin;
		target_head = pBotEnemy->v.view_ofs;
	}



#ifdef _DEBUG
	if (ignore_aim_error_code)		//@@@@@@@@@@@@temp to ignore the code that simulates human like errors
	{

	target = target_origin + target_head;
	
	pBot->curr_aim_location = target;
	
	extern edict_t *listenserver_edict;
	Vector color = Vector(255, 50, 50);
	Vector v_source = pBot->pEdict->v.origin + pBot->pEdict->v.view_ofs;
	DrawBeam(listenserver_edict, v_source, target, 20, color.x, color.y, color.z, 50);
	
	return target;

	}
#endif



	// random the head shot precentage
	hs_percentage = RANDOM_LONG(1, 100);

	// if bot using sniper rifle make him more accurate
	if (UTIL_IsSniperRifle(pBot->current_weapon.iId))
	{
		if (foe_distance > 2500)
			f_scale = 0.5;
		else if (foe_distance > 100)
			f_scale = foe_distance / (float) 10000.0;

		switch (pBot->aim_skill)
		{
			case 0:
				//Aim skill for the snipers? [APG]RoboCop[CL]
				// VERY GOOD, same as from CBasePlayer::BodyTarget (in player.h)
				target = target_origin + target_head * RANDOM_FLOAT( 0.2, 0.5 );
				d_x = 0;  // no offset
				d_y = 0;
				d_z = 0;
				// NEW CODE
				// if enemy is proned aim even lower than the edict eyes position refers
				if (pBot->pBotEnemy->v.flags & FL_PRONED)
					d_z = -1.0;
				break;
			case 1:
				// GOOD, offset a little for x, y, and z
				if (hs_percentage > 30)
					target = target_origin + target_head;  // aim for the head (if you can find it)
				else
					target = target_origin;  // aim only for the body
				d_x = RANDOM_FLOAT(-4, 4) * f_scale;
				d_y = RANDOM_FLOAT(-4, 4) * f_scale;
				d_z = RANDOM_FLOAT(-7, 7) * f_scale;
				break;
			case 2:
				// FAIR, offset somewhat for x, y, and z
				if (hs_percentage > 50)
					target = target_origin + target_head;
				else
					target = target_origin;
				d_x = RANDOM_FLOAT(-8, 8) * f_scale;
				d_y = RANDOM_FLOAT(-8, 8) * f_scale;
				d_z = RANDOM_FLOAT(-12, 12) * f_scale;
				break;
			case 3:
				// POOR, offset for x, y, and z
				if (hs_percentage > 75)
					target = target_origin + target_head;
				else
					target = target_origin;
				d_x = RANDOM_FLOAT(-12, 12) * f_scale;
				d_y = RANDOM_FLOAT(-12, 12) * f_scale;
				d_z = RANDOM_FLOAT(-16, 16) * f_scale;
				break;
			case 4:
				// BAD, offset lots for x, y, and z
				target = target_origin;		// aim only for the body
				d_x = RANDOM_FLOAT(-18, 18) * f_scale;
				d_y = RANDOM_FLOAT(-18, 18) * f_scale;
				d_z = RANDOM_FLOAT(-24, 24) * f_scale;
				break;
		}
	}
	// otherwise use this
	else
	{
		if (foe_distance > 1000)
			f_scale = 1.0;
		else if (foe_distance > 100)
			f_scale = foe_distance / (float) 1000.0;

		// aim at head and use little offset to simulate human player
		// (none can shoot to same point all the time)
		if (foe_distance <= 100)
		{
			target = target_origin + target_head;
			d_x = RANDOM_FLOAT(-2, 2) * f_scale;
			d_y = RANDOM_FLOAT(-2, 2) * f_scale;
			d_z = RANDOM_FLOAT(-2, 2) * f_scale;
		}
		// otherwise use standard skill based aiming
		else
		{
			switch (pBot->aim_skill)
			{
				case 0:
					//Aim skill? [APG]RoboCop[CL]
					// VERY GOOD, same as from CBasePlayer::BodyTarget (in player.h)
					target = target_origin + target_head * RANDOM_FLOAT( 0.4, 1.0 );
					d_x = 0;  // no offset
					d_y = 0;
					d_z = 0;
					// NEW CODE
					// if enemy is proned aim even lower than the edict eyes position refers
					if (pBot->pBotEnemy->v.flags & FL_PRONED)
						d_z = -1.0;
					break;
				case 1:
					// GOOD, offset a little for x, y, and z
					if (hs_percentage > 50)
						target = target_origin + target_head;
					else
						target = target_origin;
					d_x = RANDOM_FLOAT(-5, 5) * f_scale;
					d_y = RANDOM_FLOAT(-5, 5) * f_scale;
					d_z = RANDOM_FLOAT(-8, 8) * f_scale;
					break;
				case 2:
					// FAIR, offset somewhat for x, y, and z
					if (hs_percentage > 60)
						target = target_origin + target_head;
					else
						target = target_origin;
					d_x = RANDOM_FLOAT(-10, 10) * f_scale;
					d_y = RANDOM_FLOAT(-10, 10) * f_scale;
					d_z = RANDOM_FLOAT(-14, 14) * f_scale;
					break;
				case 3:
					// POOR, offset for x, y, and z
					if (hs_percentage > 75)
						target = target_origin + target_head;
					else
						target = target_origin;
					d_x = RANDOM_FLOAT(-15, 15) * f_scale;
					d_y = RANDOM_FLOAT(-15, 15) * f_scale;
					d_z = RANDOM_FLOAT(-20, 20) * f_scale;
					break;
				case 4:
					// BAD, offset lots for x, y, and z
					target = target_origin;
					d_x = RANDOM_FLOAT(-20, 20) * f_scale;
					d_y = RANDOM_FLOAT(-20, 20) * f_scale;
					d_z = RANDOM_FLOAT(-25, 25) * f_scale;
					break;
			}
		}
	}

	// add offset to initial aim vector
	target = target + Vector(d_x, d_y, d_z);

	// fix the aim vector when using one of these weapons
	// (ie. aim a bit higher than head or body is)
	if ((pBot->current_weapon.iId == fa_weapon_frag) || (pBot->current_weapon.iId == fa_weapon_concussion) ||
		(pBot->current_weapon.iId == fa_weapon_flashbang) || (pBot->current_weapon.iId == fa_weapon_stg24))
	{
		int aim_fix = 0;
		int base;
		float dist_modif;
		
		// these values are based on few tests in FA29
		if (foe_distance < 900)
			base = 5;
		else if (foe_distance < 1300)
			base = 10;
		else if (foe_distance < 1600)
			base = 15;
		else if (foe_distance < 1900)
			base = 20;
		else
			base = 25;

		// we have to modify it, because in these versions nades didn't fly that straight
		if (g_mod_version == FA_25)
			base *= 2.5;
		else if (g_mod_version == FA_24)
			base *= 3.5;
			

		if (foe_distance < 500)
			dist_modif = 1.5;
		else
			dist_modif = foe_distance / 100;

		aim_fix = base * dist_modif;


		//	TEST --- NEW CODE to fix the proned case ...
		//		the bots are aiming too low and so the grenade is too short and misses
		if (pBot->pEdict->v.flags & FL_PRONED)
			aim_fix += 15;


#ifdef _DEBUG
		//@@@@@@@@@@@@@@@@@@@@@@@
		//ALERT(at_console, "(GRENADES)base %d | dist_mod %.2f | aim_fix %d\n", base, dist_modif, aim_fix);
#endif



		target = target + Vector(0, 0, aim_fix);
	}
	// or these
	else if ((pBot->current_weapon.iId == fa_weapon_m79) ||
		((pBot->current_weapon.iId == fa_weapon_m16) && pBot->secondary_active) ||
		((pBot->current_weapon.iId == fa_weapon_ak74) && pBot->secondary_active))
	{
		int aim_fix = 0;
		int base;
		float dist_modif;
		
		if (foe_distance < 900)
			base = 5;
		else if (foe_distance < 1300)
			base = 10;
		else if (foe_distance < 1600)
			base = 15;
		else if (foe_distance < 1900)
			base = 20;
		else
			base = 25;

		if (UTIL_IsOldVersion())
			base *= 2.5;

		if (foe_distance < 500)
			dist_modif = 1.5;
		else
			dist_modif = foe_distance / 100;
		

		aim_fix = base * dist_modif;


		//	TEST --- NEW CODE to fix the proned case ...
		//		the bots are aiming too low and so the grenade is too short and misses
		if (pBot->pEdict->v.flags & FL_PRONED)
			aim_fix += 15;


#ifdef _DEBUG
		//@@@@@@@@@@@@@@@@@@@@@@@
		//ALERT(at_console, "(GL)base %d | dist_mod %.2f | aim_fix %d\n", base, dist_modif, aim_fix);
#endif




		target = target + Vector(0, 0, aim_fix);
	}

#ifdef _DEBUG
	pBot->curr_aim_location = target;
#endif

	return target;
}


// end test


/*
* using the mounted gun
*/
void BotFireMountedGun(Vector v_enemy, bot_t *pBot)
{
	float distance = v_enemy.Length();  // how far away is the enemy?

	// is enemy close so use full-auto fire
	if (distance < 1000)
	{
		pBot->f_shoot_time = gpGlobals->time;
	}
	// otherwise randomly switch between full-auto and simi fire
	else
	{
		if (RANDOM_LONG(1, 100) < 15)
			pBot->f_shoot_time = gpGlobals->time + RANDOM_FLOAT(0.1, 0.85);
		else
			pBot->f_shoot_time = gpGlobals->time;
	}

	// check if bot can press trigger -> b_botdontshoot
	if (b_botdontshoot == FALSE)
		pBot->pEdict->v.button |= IN_ATTACK;  // press primary attack button

	return;
}

//NOTE: maybe add using scope on m14, g36e and anaconda - more acc
/*
* fire main weapon (assuming enough ammo exists for that weapon and checking effective range)
*/
int BotFireWeapon( Vector v_enemy, bot_t *pBot )
{
	bot_weapon_select_t *pSelect = NULL;
	bot_fire_delay_t *pDelay = NULL;

	pSelect = &bot_weapon_select[0];
	pDelay = &bot_fire_delay[0];

	edict_t *pEdict = pBot->pEdict;
	bool press_trigger;
	float distance = v_enemy.Length2D();  // how far away is the enemy (ignoring his z-coord)?
	int weaponID;//, select_index;

	// these are for handling M203/GP25 attachment
	float min_safe_distance, base_delay, min_delay, max_delay;
	
#ifdef _DEBUG
	// to print everything
	if (in_bot_dev_level2)
		in_bot_dev_level1 = TRUE;
#endif

	// see which weapon is used
	if (pBot->forced_usage == USE_MAIN)
		weaponID = pBot->main_weapon;
	else if (pBot->forced_usage == USE_BACKUP)
		weaponID = pBot->backup_weapon;
	else
	{
#ifdef _DEBUG
		char msg[80];
		sprintf(msg, "!!!!BFW --- not using main or backup weapon (weaponID %d)!!!!\n",
			pBot->current_weapon.iId);

		ALERT(at_console, msg);

		UTIL_DebugDev(msg, -100, -100);
#endif
		return RETURN_NOTFIRED;
	}

	press_trigger = FALSE;		// reset press_trigger flag first

	// is this slot empty
	if (weaponID == -1)
	{
		// change to knife, we know that each player gets knife by default in Firearms
		pBot->forced_usage = USE_KNIFE;

		return RETURN_NOTFIRED;
	}
	
	// if the bot doesn't carry this weapon
	if ((pBot->current_weapon.iId != weaponID) && (pBot->weapon_action == W_READY))
	{
		// set this flag to allow weapon change
		pBot->weapon_action = W_TAKEOTHER;

#ifdef _DEBUG
		// testing - right weapon
		if (in_bot_dev_level2)
			ALERT(at_console, "Taking new weapon (ID=%d)\n", weaponID);
#endif
		return RETURN_TAKING;
	}

	// if the weapon is NOT ready yet break it
	if (pBot->weapon_action != W_READY)
	{
		return RETURN_NOTFIRED;
	}

	// normal fire action statement, most weapons have NO secondary (special) action
	if (pBot->secondary_active == FALSE)
	{
		// check if clip is empty
		if (pBot->current_weapon.iClip == 0)
		{
			// the bot has no magazines
			if (pBot->current_weapon.iAmmo1 == 0)
			{
				// for all weapon with exception of m11 in FA25
				if (pBot->current_weapon.iId != fa_weapon_m11)
				{
#ifdef _DEBUG
					// testing - empty weapon
					if (in_bot_dev_level2)
					{
						ALERT(at_console, "No ammo for weapon ID=%d (inClip=%d Magazines=%d)\n",
							pBot->current_weapon.iId, pBot->current_weapon.iClip,
							pBot->current_weapon.iAmmo1);
					}
#endif
					// no_ammo flag must be set
					if (pBot->forced_usage == USE_MAIN)
						pBot->main_no_ammo = TRUE;
					else
						pBot->backup_no_ammo = TRUE;

					return RETURN_NOAMMO;
				}
				else
				{
					// if also second m11 is completely empty then the bot really has no ammo
					if (pBot->current_weapon.iAmmo2 == 0)
					{
#ifdef _DEBUG
						// testing - empty weapon
						if (in_bot_dev_level2)
						{
							ALERT(at_console, "No ammo for weapon ID=%d (inClip=%d Magazines=%d)\n",
								pBot->current_weapon.iId, pBot->current_weapon.iClip,
								pBot->current_weapon.iAmmo1);
						}
#endif
						// no_ammo flag must be set
						if (pBot->forced_usage == USE_MAIN)
							pBot->main_no_ammo = TRUE;
						else
							pBot->backup_no_ammo = TRUE;
						
						return RETURN_NOAMMO;
					}
				}
			}
			// otherwise the bot has at least one magazine
			else
			{
#ifdef _DEBUG
				// testing - right weapon and iClip info
				if (in_bot_dev_level2)
				{
					ALERT(at_console, "Reloading weapon ID=%d (inClip=%d)\n",
						pBot->current_weapon.iId, pBot->current_weapon.iClip);
				}
#endif
				// press reload button and set some delay to allow proper reload action
				pEdict->v.button |= IN_RELOAD;

				// is current weapon benelli/remington
				if (pBot->current_weapon.iId == fa_weapon_benelli)
					pBot->f_reload_time = gpGlobals->time + 4.5; // estimated min delay for 7 shells
				// is current weapon anaconda
				else if (pBot->current_weapon.iId == fa_weapon_anaconda)
					pBot->f_reload_time = gpGlobals->time + 6.5; // estimated min delay for 6 rounds
				// otherwise all other weapons
				else
					pBot->f_reload_time = gpGlobals->time + 0.6; // estimated delay (was 1.5)

				return RETURN_RELOADING;
			}
		}	// END if clip is empty

		// if the bot uses m11 and the second m11 is empty and there's ammo for it then reload
		if ((pBot->current_weapon.iId == fa_weapon_m11) &&
			(pBot->current_weapon.iAttachment == 0) && (pBot->current_weapon.iAmmo2 > 0))
		{
#ifdef _DEBUG
			// testing - right weapon and iClip info
			if (in_bot_dev_level2)
			{
				ALERT(at_console, "Reloading weapon ID=%d (inClip=%d) (second gun)\n",
					pBot->current_weapon.iId, pBot->current_weapon.iClip);
			}
#endif

			pEdict->v.button |= IN_RELOAD;
			pBot->f_reload_time = gpGlobals->time + 0.6;

			return RETURN_RELOADING;
		}

		// these weapons have to ignore safe distance specified in the struct
		// it's used only for firing the gl attachment
		if ((pBot->current_weapon.iId == fa_weapon_m16) ||
			(pBot->current_weapon.iId == fa_weapon_ak74))
			min_safe_distance = 0.0;		
		else
			min_safe_distance = pSelect[weaponID].min_safe_distance;

		base_delay = pDelay[weaponID].primary_base_delay;
		min_delay = pDelay[weaponID].primary_min_delay[pBot->bot_skill];
		max_delay = pDelay[weaponID].primary_max_delay[pBot->bot_skill];

		// carries the bot this weapon AND is that weapon active (ie. in hands)
		if ((pBot->bot_weapons & (1<<pSelect[weaponID].iId)) &&
			(pBot->current_weapon.isActive == 1))
		{
			// is the bot in effective range of this weapon
			if ((distance <= pSelect[weaponID].max_effective_distance) &&
				(distance >= min_safe_distance))
			{				
#ifdef _DEBUG
				// testing - firing weapon and ammo info
				if (in_bot_dev_level1)
				{					
					ALERT(at_console, "FIRE weapon ID=%d Array ID=%d (iClip=%d Mags=%d Dist=%.2f)\n",
						pBot->current_weapon.iId, pSelect[weaponID].iId,
						pBot->current_weapon.iClip, pBot->current_weapon.iAmmo1, distance);
				}
#endif
				press_trigger = TRUE;	// bot is ready to fire

				// don't move if using one of these weapons
				if (UTIL_IsSniperRifle(pBot->current_weapon.iId) ||
					UTIL_IsMachinegun(pBot->current_weapon.iId))
				{
#ifdef _DEBUG
					// testing - special weapons
					if (in_bot_dev_level2)
						ALERT(at_console, "Must slow down to fire weapon ID=%d\n",
							pBot->current_weapon.iId);
#endif
					// is it time to set new sniping time
					if ((pBot->sniping_time < gpGlobals->time) &&
						(pBot->f_combat_advance_time < gpGlobals->time))
					{
						bool start_sniping = FALSE;

						// is the bipod down then do snipe all the time
						if (pBot->IsTask(TASK_BIPOD))
							start_sniping = TRUE;
						// does the bot use m82 AND is 95% chance
						else if ((pBot->current_weapon.iId == fa_weapon_m82) &&
							(RANDOM_LONG(1, 100) <= 95))
							start_sniping = TRUE;
						// all other weapons need chance 85% to start sniping
						else if (RANDOM_LONG(1, 100) <= 85)
							start_sniping = TRUE;

						if (start_sniping)
						{
							// set sniping time based on behaviour type
							if (pBot->bot_behaviour & DEFENDER)
								pBot->sniping_time = gpGlobals->time + RANDOM_FLOAT(5.0, 13.0);
							else
								pBot->sniping_time = gpGlobals->time + RANDOM_FLOAT(2.5, 10.5);
#ifdef _DEBUG
							// testing - snipe time
							if (in_bot_dev_level2)
								ALERT(at_console, "Sniping time set %.2f\n", pBot->sniping_time);
#endif
							if (g_debug_bot_on)
							{
								char msg[80];
								extern edict_t *pRecipient;
								sprintf(msg, "Sniping time set %.2f\n", pBot->sniping_time);
								ClientPrint(pRecipient, HUD_PRINTNOTIFY, msg);
							}
						}
					}

					// fire m82 and psg1 unscoped in FA25 and below,
					// because there's no value to check if scoped or not
					// actually we don't do anything important in this statement,
					// we only need it to seperate them from the standard sniper behaviour
					if (UTIL_IsOldVersion() && (pBot->sniping_time > gpGlobals->time) &&
						((pBot->current_weapon.iId == fa_weapon_psg1) ||
						(pBot->current_weapon.iId == fa_weapon_m82)))
					{
						if (g_debug_bot_on)
						{
							extern edict_t *pRecipient;
							ClientPrint(pRecipient, HUD_PRINTNOTIFY, "Using unscoped in FA25\n");
						}
					}

					// has the bot this weapon AND scope is NOT active AND is snipe time
					else if ((pBot->current_weapon.iId == fa_weapon_ssg3000) &&
						(pEdict->v.fov == NO_ZOOM) && (pBot->sniping_time > gpGlobals->time))
					{
						pEdict->v.button |= IN_ATTACK2;				// switch to scope
						pBot->f_shoot_time = gpGlobals->time + 0.4;	// time to take effect

#ifdef _DEBUG
						// testing - weapon with scope
						if (in_bot_dev_level2)
							ALERT(at_console, "Trying scope 1X (WeaponID=%d)\n",
								pBot->current_weapon.iId);
#endif
						if (g_debug_bot_on)
						{
							extern edict_t *pRecipient;
							ClientPrint(pRecipient, HUD_PRINTNOTIFY, "Trying scope 1X\n");
						}
						
						return RETURN_SECONDARY;
					}

					else if ((pBot->current_weapon.iId == fa_weapon_svd) &&
						(pEdict->v.fov == NO_ZOOM) && (pBot->sniping_time > gpGlobals->time))
					{
						pEdict->v.button |= IN_ATTACK2;
						pBot->f_shoot_time = gpGlobals->time + 0.4;

#ifdef _DEBUG
						// testing - weapon with scope
						if (in_bot_dev_level2)
							ALERT(at_console, "Trying scope 1X (WeaponID=%d)\n",
								pBot->current_weapon.iId);
#endif
						if (g_debug_bot_on)
						{
							extern edict_t *pRecipient;
							ClientPrint(pRecipient, HUD_PRINTNOTIFY, "Trying scope 1X\n");
						} 

						return RETURN_SECONDARY;
					}

					else if ((pBot->current_weapon.iId == fa_weapon_m82) &&
						(pBot->sniping_time > gpGlobals->time))
					{
						if (pEdict->v.fov == ZOOM_1X)
						{
							pEdict->v.button |= IN_ATTACK2;
							pBot->f_shoot_time = gpGlobals->time + 0.2;
#ifdef _DEBUG
							// testing - weapon with scope
							if (in_bot_dev_level2)
								ALERT(at_console, "Trying scope 2X (WeaponID=%d)\n",
									pBot->current_weapon.iId);
#endif
							if (g_debug_bot_on)
							{
								extern edict_t *pRecipient;
								ClientPrint(pRecipient, HUD_PRINTNOTIFY, "Trying scope 2X\n");
							}
						}
						else if (pEdict->v.fov == NO_ZOOM)
						{
							pEdict->v.button |= IN_ATTACK2;
							pBot->f_shoot_time = gpGlobals->time + 0.4;
#ifdef _DEBUG
							// testing - weapon with scope
							if (in_bot_dev_level2)
								ALERT(at_console, "Trying scope 1X (WeaponID=%d)\n",
									pBot->current_weapon.iId);
#endif
							if (g_debug_bot_on)
							{
								extern edict_t *pRecipient;
								ClientPrint(pRecipient, HUD_PRINTNOTIFY, "Trying Scope 1X\n");
							}
						}

						return RETURN_SECONDARY;
					}

					// if the bot is moving too fast then don't allow shooting
					if (UTIL_IsSniperRifle(pBot->current_weapon.iId) &&
						(pEdict->v.velocity.Length() > 0))
					{
						return RETURN_NOTFIRED;
					}
					// we know that the bot can use only machinegun now
					// don't allow shoting while in move in FA29
					if (((g_mod_version == FA_29) || (g_mod_version == FA_30)) &&
						(pEdict->v.velocity.Length() > 0))
					{
						return RETURN_NOTFIRED;
					}
					// otherwise allow shooting only if crouch or prone movements
					else if ((pEdict->v.velocity.Length() > 0) &&
						!(pEdict->v.flags & FL_PRONED) && !(pEdict->v.flags & FL_DUCKING))
					{
						return RETURN_NOTFIRED;
					}
				}

				// has the bot one of these weapons
				else if ((pBot->current_weapon.iId == fa_weapon_m16) ||
					(pBot->current_weapon.iId == fa_weapon_ak74))
				{
					// is the bot in safe distance AND is chance to use gl AND
					// have ammo2 AND necessary skill (arty1)
					if ((distance > pSelect[weaponID].min_safe_distance) &&
						(RANDOM_LONG(1, 100) < 15) &&
						(pBot->current_weapon.iAttachment > 0) &&
						(pBot->bot_fa_skill & ARTY1))
					{
						// there's no switching to gl attachment in this version
						// it will just fire it on IN_ATTACK2 command
						if (UTIL_IsOldVersion())
						{
							pBot->secondary_active = TRUE;
							// add short delay to all bot to aim a bit up before firing the gl
							pBot->f_shoot_time = gpGlobals->time + 0.3;
						}
						else
						{
							pEdict->v.button |= IN_ATTACK2;			// switch to attached gl
							pBot->f_shoot_time = gpGlobals->time + 1.5;	// time to take effect
						}

#ifdef _DEBUG
						if (in_bot_dev_level2)						
							ALERT(at_console, "Changing to GL (ID=%d)\n",
								pBot->current_weapon.iId);
#endif
						if (g_debug_bot_on)
						{
							extern edict_t *pRecipient;
							ClientPrint(pRecipient, HUD_PRINTNOTIFY, "Changing to GL attachment\n");
						}

						return RETURN_SECONDARY;
					}
				}

				// don't move if using m79
				else if (pBot->current_weapon.iId == fa_weapon_m79)
				{
					if ((pBot->sniping_time < gpGlobals->time) &&
						(pBot->f_combat_advance_time < gpGlobals->time) &&
						(RANDOM_LONG(1, 100) <= 75))
					{
						// set short sniping time for firing just one grenade
						pBot->sniping_time = gpGlobals->time + 1.5;
#ifdef _DEBUG
						// testing - snipe time
						if (in_bot_dev_level2)
						{
							char msg[80];
							sprintf(msg, "Sniping delay=%f\n", pBot->sniping_time);
							ALERT(at_console, msg);
						}
#endif
					}

					if (pEdict->v.velocity.Length() > 125)
					{
						return RETURN_NOTFIRED;
					}
				}
			}// END is in effective range

			// is the bot too close to fire this weapon
			else if (distance < pSelect[weaponID].min_safe_distance)
			{
#ifdef _DEBUG
				// testing - this weapon
				if (in_bot_dev_level2)
				//if (in_bot_dev_level1)
					ALERT(at_console, "Too close to fire weapon ID=%d Array ID=%d (Distance=%.2f)\n",
						pBot->current_weapon.iId, pSelect[weaponID].iId, distance);
#endif
				return RETURN_TOOCLOSE;
			}

			// is bot too far to fire this weapon
			else if (distance > pSelect[weaponID].max_effective_distance)
			{
#ifdef _DEBUG
				// testing - this weapon
				//if (in_bot_dev_level1)
				if (in_bot_dev_level2)
					ALERT(at_console, "Too far to fire weapon ID=%d Array ID=%d (Distance=%.2f)\n",
						pBot->current_weapon.iId, pSelect[weaponID].iId, distance);
#endif

				return RETURN_TOOFAR;
			}
		}// END is bot carrying this weapon

#ifdef _DEBUG
		extern int debug_engine;
		// testing - check if weapons in array pSelect and in array pDelay are in same order
		if (debug_engine == 1)
		{
			if (pSelect[weaponID].iId != pDelay[weaponID].iId)
			{
				ALERT(at_console, "Weapon order in pSelect(weapon ID=%d) isn't same as in pDelay(weapon ID=%d)\n",
					pSelect[weaponID].iId, pDelay[weaponID].iId);

				UTIL_DebugInFile("BotCombat|BotFireWeapon()|pSelect != pDelay\n");

				return RETURN_NOTFIRED;
			}
		}
#endif
	}// END is secondary_active == FALSE

	// otherwise if is secondary_active then the bot already has the enemy in sight
	// so now we only need to fire the weapon if the bot is outside min_safe_distance
	else if (pBot->secondary_active)
	{
#ifdef _DEBUG
		//if ((pEdict->v.fov == NO_ZOOM) && UTIL_IsSniperRifle(pBot->current_weapon.iId))
		//{
		//	char msg[80];
		//	sprintf(msg, "<<<BUG>>> --- Sniper rifle zoom bug (weaponID %d)\n",
		//		pBot->current_weapon.iId);

		//	ALERT(at_console, msg);

		//	UTIL_DebugDev(msg, -100, -100);
		//}
#endif


#ifdef _DEBUG
		// testing - right weapon
		if (in_bot_dev_level1)
			ALERT(at_console, "Secondary fire ID=%d (iClip=%d | Distance=%.2f)\n",
				pBot->current_weapon.iId, pBot->current_weapon.iClip, distance);
#endif

		// check if clip is empty AND using sniper rifle
		if ((pBot->current_weapon.iClip == 0) && UTIL_IsSniperRifle(pBot->current_weapon.iId))
		{
			// the bot has no magazines
			if (pBot->current_weapon.iAmmo1 == 0)
			{
				// no_ammo flag must be set
				if (pBot->forced_usage == USE_MAIN)
					pBot->main_no_ammo = TRUE;
				else
					pBot->backup_no_ammo = TRUE;

				return RETURN_NOAMMO;
			}
			// otherwise the bot has at least one magazine
			else
			{
				// press reload button and set some delay to allow proper reload action
				pEdict->v.button |= IN_RELOAD;

				pBot->f_reload_time = gpGlobals->time + 0.6; // estimated delay (was 1.5)

				return RETURN_RELOADING;
			}
		}

		// is it time to set new sniping time
		if (UTIL_IsSniperRifle(pBot->current_weapon.iId) &&
			(pBot->sniping_time < gpGlobals->time) &&
			(pBot->f_combat_advance_time < gpGlobals->time))
		{
			bool start_sniping = FALSE;

			if (pBot->IsTask(TASK_BIPOD))
				start_sniping = TRUE;
			else if ((pBot->current_weapon.iId == fa_weapon_m82) && (RANDOM_LONG(1, 100) <= 95))
				start_sniping = TRUE;
			else if (RANDOM_LONG(1, 100) <= 85)
				start_sniping = TRUE;

			if (start_sniping)
			{
				if (pBot->bot_behaviour & DEFENDER)
					pBot->sniping_time = gpGlobals->time + RANDOM_FLOAT(5.0, 13.0);
				else
					pBot->sniping_time = gpGlobals->time + RANDOM_FLOAT(2.5, 10.5);
#ifdef _DEBUG
				// testing - snipe time
				if (in_bot_dev_level2)
					ALERT(at_console, "Sniping delay=%.2f\n", pBot->sniping_time);
#endif
			}
		}

		if (UTIL_IsSniperRifle(pBot->current_weapon.iId) && (pEdict->v.velocity.Length() > 0))
		{
			return RETURN_NOTFIRED;
		}

		// is the weapon m16+m203 OR ak74+gp25
		if ((pBot->current_weapon.iId == fa_weapon_m16) ||
			(pBot->current_weapon.iId == fa_weapon_ak74))
		{
			float use_delay[BOT_SKILL_LEVELS] = {1.0, 1.2, 1.4, 1.6, 2.0};

			//min_safe_distance = pSelect[select_index].min_safe_distance;
			min_safe_distance = pSelect[weaponID].min_safe_distance;
			base_delay = use_delay[pBot->bot_skill];
			min_delay = 0.0;
			max_delay = 0.0;

			// has the bot NO ammo in attached gl chamber
			if (pBot->current_weapon.iAttachment < 1)
			{
				if (UTIL_IsOldVersion())
					pBot->secondary_active = FALSE;

				pEdict->v.button |= IN_ATTACK2;	// get back primary fire
				pBot->f_shoot_time = gpGlobals->time + base_delay; // time to take effect

				return RETURN_FIRED; // like it was fired
			}
		}
		// all other weapons
		else
		{
			int skill = pBot->bot_skill;

			min_safe_distance = pSelect[weaponID].min_safe_distance;
			base_delay = pDelay[weaponID].primary_base_delay;
			min_delay = pDelay[weaponID].primary_min_delay[skill];
			max_delay = pDelay[weaponID].primary_max_delay[skill];
		}

		// if still in safe distance
		if (distance >= min_safe_distance)
		{
			press_trigger = TRUE;

#ifdef _DEBUG
			if ((in_bot_dev_level2) &&
				((pBot->current_weapon.iId == fa_weapon_m16) ||
				(pBot->current_weapon.iId == fa_weapon_ak74)))
			{
				ALERT(at_console, "SECONADRY_active -- gl is going to be fired (ID=%d)\n",
					pBot->current_weapon.iId);
			}
			else if (in_bot_dev_level2)
			{
				ALERT(at_console, "SECONADRY_active -- firing weapon (ID=%d)\n",
					pBot->current_weapon.iId);
			}
#endif
			if (g_debug_bot_on)
			{
				extern edict_t *pRecipient;

				if ((pBot->current_weapon.iId == fa_weapon_m16) ||
					(pBot->current_weapon.iId == fa_weapon_ak74))
				{
					ClientPrint(pRecipient, HUD_PRINTNOTIFY, "SECONADRY_active -- gl is going to be fired\n");
				}
				else
				{
					ClientPrint(pRecipient, HUD_PRINTNOTIFY, "SECONADRY_active -- firing weapon\n");
				}
			}
		}
		// otherwise don't fire - risk your life enemy is too close
		else
		{
			// if using m16 or ak74 gl attachment get back to primary fire mode
			if ((pBot->current_weapon.iId == fa_weapon_m16) ||
				(pBot->current_weapon.iId == fa_weapon_ak74))
			{
				if (UTIL_IsOldVersion())
					pBot->secondary_active = FALSE;

				pEdict->v.button |= IN_ATTACK2;
				pBot->f_shoot_time = gpGlobals->time + base_delay;

#ifdef _DEBUG
				if (in_bot_dev_level2)
					ALERT(at_console, "SECONADRY_active -- too close for gl --> returning to normal fire (ID=%d)\n",
						pBot->current_weapon.iId);
#endif
				if (g_debug_bot_on)
				{
					extern edict_t *pRecipient;
					ClientPrint(pRecipient, HUD_PRINTNOTIFY, "SECONADRY_active -- too close for gl --> returning to normal fire\n");
				}

				return RETURN_FIRED; // like it was fired
			}

			return RETURN_TOOCLOSE;
		}
	}// END is secondary_active == TRUE

	// everything is done so fire the weapon and set correct fire delay - based on skill
	if (press_trigger)
	{
		// check if bot can press trigger ie b_botdontshoot
		if (b_botdontshoot == FALSE)
		{
			// do we need to fire m16 attached gl in FA25
			if (((pBot->current_weapon.iId == fa_weapon_m16) && pBot->secondary_active &&
				UTIL_IsOldVersion()) || UTIL_IsBitSet(WA_USEAKIMBO, pBot->weapon_status))
			{
				pEdict->v.button |= IN_ATTACK2;  // press secondary attack button
			}
			else
				pEdict->v.button |= IN_ATTACK;  // press primary attack button
		}

		// has the bot NOT machinegun or saiga (in FA25 saiga has full auto as its firemode)
		// and should use full auto OR
		// is the right time for auto fire if bot is holding machinegun
		if (((UTIL_IsMachinegun(pBot->current_weapon.iId) == FALSE) &&
			(pBot->current_weapon.iId != fa_weapon_saiga) &&
			(pBot->current_weapon.iFireMode == FM_AUTO)) ||
			((pBot->f_fullauto_time > 0.0) && UTIL_IsMachinegun(pBot->current_weapon.iId)))
		{
			pBot->f_shoot_time = gpGlobals->time;
		}
		else
		{
			pBot->f_shoot_time = gpGlobals->time + base_delay + RANDOM_FLOAT(min_delay, max_delay);
		}

		return RETURN_FIRED;
	}

	return RETURN_NOTFIRED; // something went wrong
}

/*
* use knife - checking max usable distance
*/
int BotUseKnife( Vector v_enemy, bot_t *pBot )
{
	bot_weapon_select_t *pSelect = NULL;
	bot_fire_delay_t *pDelay = NULL;

	pSelect = &bot_weapon_select[0];
	pDelay = &bot_fire_delay[0];

	edict_t *pEdict = pBot->pEdict;
	float distance = v_enemy.Length2D();

#ifdef _DEBUG
	if (in_bot_dev_level2)
		in_bot_dev_level1 = TRUE;
#endif

	if ((pBot->current_weapon.iId != fa_weapon_knife) && (pBot->weapon_action == W_READY))
	{
		pBot->weapon_action = W_TAKEOTHER;

#ifdef _DEBUG
		// testing - is it really knife
		if (in_bot_dev_level2)
			ALERT(at_console, "Taking knife (prev weaponID=%d)\n", pBot->current_weapon.iId);
#endif
		return RETURN_TAKING;
	}

	if (pBot->weapon_action != W_READY)
	{
		return RETURN_NOTFIRED;
	}

#ifdef _DEBUG
	// testing - is it knife?
	if (in_bot_dev_level1)
		ALERT(at_console, "Weapon id=%d Array id=%d\n",
			pBot->current_weapon.iId, pSelect[pBot->current_weapon.iId].iId);
#endif

	//if (distance <= pSelect[select_index].max_effective_distance)
	if (distance <= pSelect[pBot->current_weapon.iId].max_effective_distance)
	{
		int skill = pBot->bot_skill;
		float base_delay, min_delay, max_delay; 

		// 2/3 of the time use primary attack
		if (RANDOM_LONG(1, 100) <= 66)
		{
			if (b_botdontshoot == FALSE)
				pEdict->v.button |= IN_ATTACK;  // press primary attack button

			base_delay = pDelay[pBot->current_weapon.iId].primary_base_delay;
			min_delay = pDelay[pBot->current_weapon.iId].primary_min_delay[skill];
			max_delay = pDelay[pBot->current_weapon.iId].primary_max_delay[skill];
	
#ifdef _DEBUG
			// testing - knife attack and distance
			if (in_bot_dev_level1)
				ALERT(at_console, "Primary attack with knife (distance=%.2f)\n", distance);
#endif
		}
		// otherwise use secondary attack
		else
		{
			float min_d_array[BOT_SKILL_LEVELS] = {0.0, 0.1, 0.2, 0.4, 0.6};
			float max_d_array[BOT_SKILL_LEVELS] = {0.1, 0.2, 0.3, 0.7, 1.0};

			if (b_botdontshoot == FALSE)
				pEdict->v.button |= IN_ATTACK2;  // press secondary attack button

			base_delay = 0.5;
			min_delay = min_d_array[skill];
			max_delay = max_d_array[skill];

#ifdef _DEBUG
			// testing - knife attack and distance
			if (in_bot_dev_level1)
				ALERT(at_console, "Secondary attack with knife (distance=%.2f)\n", distance);
#endif
		}

		pBot->f_shoot_time = gpGlobals->time + base_delay + RANDOM_FLOAT(min_delay, max_delay);

		return RETURN_FIRED;
	}

	//else if (distance > pSelect[select_index].max_effective_distance)
	else if (distance > pSelect[pBot->current_weapon.iId].max_effective_distance)
	{
#ifdef _DEBUG
		// testing - can't reached with knife
		if (in_bot_dev_level2)
		//if (in_bot_dev_level1)
			ALERT(at_console, "Too far to use knife (distance=%.2f)\n", distance);
#endif

		return RETURN_TOOFAR;
	}

	return RETURN_NOTFIRED;
}

/*
* use grenade - checking min(safe) and max distance, setting grenades_used flag
* handles changes between varios FA versions
*/
int BotThrowGrenade( Vector v_enemy, bot_t *pBot )
{
	bot_weapon_select_t *pSelect = NULL;
	bot_fire_delay_t *pDelay = NULL;
	
	pSelect = &bot_weapon_select[0];
	pDelay = &bot_fire_delay[0];

	edict_t *pEdict = pBot->pEdict;
	float distance = v_enemy.Length2D();

#ifdef _DEBUG
	if (in_bot_dev_level2)
		in_bot_dev_level1 = TRUE;
#endif

	// if something went wrong
	if (pBot->grenade_slot == -1)
	{
		// set grenade action to correct flag,
		// grenades weren't spawned so set this flag like it were used
		// to prevent running this method
		pBot->grenade_action = ALTW_USED;

#ifdef _DEBUG
		char msg[256];
		sprintf(msg, "BotCombat|Bot Throw Grenade()|Invalid run of this method -> gr_slot==-1 | bot class %d\n",
			pBot->bot_class);
		UTIL_DebugDev(msg, -100, -100);
#endif

		return RETURN_NOTFIRED;
	}

	// check if something went wrong, if so change to different weapon
	if (pBot->grenade_action == ALTW_USED)
	{
#ifdef _DEBUG

		UTIL_DebugInFile("<<BUG>>Invalid run - no grenades - already used before\n");

		//@@@@@@@@@@@@@@@
		ALERT(at_console, "<<BUG>>Invalid run - no grenades - already used before\n");

#endif
		return RETURN_NOTFIRED;
	}

	if ((pBot->current_weapon.iId != pBot->grenade_slot) && (pBot->weapon_action == W_READY))
	{
		if (pBot->bot_weapons & (1<<pBot->grenade_slot))
		{

#ifdef _DEBUG
			// testing
			if (in_bot_dev_level2)
				ALERT(at_console, "<<GRENADE>>Not in hands yet (currW is %d)\n", pBot->current_weapon.iId);
#endif


			pBot->weapon_action = W_TAKEOTHER;
			return RETURN_TAKING;
		}
		// this case should not happen if it does then we have to increase the base time in pDelay for this particular
		// grenade ... that will give the engine more time to finish the entity removal task
		else
		{

#ifdef _DEBUG
			// testing
			if (in_bot_dev_level2)
				ALERT(at_console, "<<GRENADE>>ALREADY USED THEM ALL (currW is %d)\n", pBot->current_weapon.iId);
#endif
			char error_msg[256];

			if (pBot->grenade_slot != -1)
				sprintf(error_msg, "<<BUG>>ThrowGrenade() called after using the last grenade. Try inceasing the primary_base_delay value in marine_bot\\weapons\\modversion file for weapon %s\n", STRING(weapon_defs[pBot->grenade_slot].szClassname));
			else
				sprintf(error_msg, "<<BUG>>ThrowGrenade() called after using the last grenade. Try inceasing the primary_base_delay value in marine_bot\\weapons\\modversion file for all grenades\n");
			UTIL_DebugInFile(error_msg);

			pBot->grenade_action = ALTW_USED;
			return RETURN_NOTFIRED;
		}
	}

	if (pBot->weapon_action != W_READY)
	{
		return RETURN_TAKING;
	}

#ifdef _DEBUG
	// testing - is it really any grenade
	if (in_bot_dev_level2)
		ALERT(at_console, "<<GRENADE>> %d has been SELECTED now (currT=%.3f | gren_act=%d)\n",
		pSelect[pBot->current_weapon.iId].iId, gpGlobals->time, pBot->grenade_action);
#endif

	if ((pBot->bot_weapons & (1<<pSelect[pBot->current_weapon.iId].iId)) &&
		(pBot->current_weapon.isActive == 1))
	{
		if ((distance <= pSelect[pBot->current_weapon.iId].max_effective_distance) &&
			(distance >= pSelect[pBot->current_weapon.iId].min_safe_distance))
		{
			// let the engine finish the model animation before we press fire to remove the fuse
			if (pBot->grenade_action == ALTW_NOTUSED)
			{
				pBot->grenade_action = ALTW_TAKING;
				
				// assures enough time for the animation
				pBot->f_shoot_time = gpGlobals->time + 1.0;

				// we need grenade time even bigger else this method won't be called after the animation was finished
				pBot->grenade_time = pBot->f_shoot_time + 0.5;

#ifdef _DEBUG
					// testing
					if (in_bot_dev_level1)
						ALERT(at_console, "<<GRENADE>>Model animation wait time grID=%d (currT=%.3f | gren_act=%d)\n",
						pSelect[pBot->current_weapon.iId].iId, gpGlobals->time, pBot->grenade_action);
#endif

				return RETURN_NOTFIRED;
			}
#ifdef _DEBUG
			// testing - amount of grenades
			if (in_bot_dev_level1)
				ALERT(at_console, "This grenade can be thrown now -> ID=%d Array ID=%d (Reserves=%d Dist=%.2f currT=%.3f gren_act=%d)\n",
					pBot->current_weapon.iId, pSelect[pBot->current_weapon.iId].iId,
					pBot->current_weapon.iAmmo1, distance, gpGlobals->time, pBot->grenade_action);
#endif

			// this will throw the grenade
			if (b_botdontshoot == FALSE)
			{
				// when the grenade is ready then press fire button to remove the fuse
				if (pBot->grenade_action == ALTW_TAKING)
				{
#ifdef _DEBUG
					// testing
					if (in_bot_dev_level1)
						ALERT(at_console, "<<GRENADE>>Priming the grenade %d (currT=%.3f | gren_act=%d)\n",
						pSelect[pBot->current_weapon.iId].iId, gpGlobals->time, pBot->grenade_action);
#endif
					if (UTIL_IsNewerVersion())
					{
						pEdict->v.button |= IN_ATTACK;
						pBot->grenade_action = ALTW_PLACED;
						
						return RETURN_PRIMING;
					}
					// in Firearms 2.7 and below the grenades don't have the 'remove fuse first behaviour'
					// so we can throw them right away
					else
					{
						pBot->grenade_action = ALTW_PLACED;
					}
				}

				// the fuse must have been removed by now so throw the grenade finally
				if (pBot->grenade_action == ALTW_PLACED)	// or goalplaced for secondary grenade priming ... UNDONE
				{
					pEdict->v.button |= IN_ATTACK;

					// in Firearms 2.4 when you want to throw the standard fragmentation grenade you need to keep the 
					// fire button pressed for a while else you drop the grenade right under your feet
					if (g_mod_version == FA_24 && pBot->current_weapon.iId == fa_weapon_frag && RANDOM_LONG(1, 500) > 1)
					{
						// so this will assure the method to be called couple frames in row
						// and the fire button will stay pressed
						pBot->grenade_time = gpGlobals->time + 0.1;
						return RETURN_NOTFIRED;
					}

					pBot->grenade_action = ALTW_PREPARED;

#ifdef _DEBUG
					// testing
					if (in_bot_dev_level1)
						ALERT(at_console, "<<GRENADE>>The grenade %d is PRIMED & READY!!! (currT=%.3f | gren_act=%d)\n",
						pSelect[pBot->current_weapon.iId].iId, gpGlobals->time, pBot->grenade_action);
#endif
				}
			}
			
			// is everything done so the grenade can be thrown in this frame
			// then set the correct shoot time for standard weapon
			if (pBot->grenade_action == ALTW_PREPARED)
			{
				int skill = pBot->bot_skill;
				
				// set shoot time to prevent firing weapon immediatelly after the bot threw the grenade
				float base = pDelay[pBot->current_weapon.iId].primary_base_delay;
				float min = pDelay[pBot->current_weapon.iId].primary_min_delay[skill];
				float max = pDelay[pBot->current_weapon.iId].primary_max_delay[skill];
				
				pBot->f_shoot_time = gpGlobals->time + base + RANDOM_FLOAT(min, max);
				pBot->grenade_action = ALTW_NOTUSED;

#ifdef _DEBUG
			// testing - should really throw it now
			if (in_bot_dev_level1)
				ALERT(at_console, "*****************     THROWING NOW >> ID=%d Array ID=%d (Reserves=%d Dist=%.2f gren_act=%d) | shootT=%.2f currT=%.3f (deltaT=%.2f)\n",
					pBot->current_weapon.iId, pSelect[pBot->current_weapon.iId].iId,
					pBot->current_weapon.iAmmo1, distance, pBot->grenade_action, pBot->f_shoot_time, gpGlobals->time,
					pBot->f_shoot_time - gpGlobals->time);
#endif
				
				return RETURN_FIRED;
			}
		}

		else if (distance < pSelect[pBot->current_weapon.iId].min_safe_distance)
		{
#ifdef _DEBUG
			// testing - not in safe distance
			if (in_bot_dev_level1)
				ALERT(at_console, "Too close to throw ID=%d Array ID=%d (Distance=%.2f | gren_act=%d)\n",
					pBot->current_weapon.iId, pSelect[pBot->current_weapon.iId].iId, distance, pBot->grenade_action);
#endif
			return RETURN_TOOCLOSE;
		}
		
		else if (distance > pSelect[pBot->current_weapon.iId].max_effective_distance)
		{
#ifdef _DEBUG
			// testing - grenade
			if (in_bot_dev_level1)
				ALERT(at_console, "Too far to throw there ID=%d Array ID=%d (Distance=%.2f | gren_act=%d)\n",
					pBot->current_weapon.iId, pSelect[pBot->current_weapon.iId].iId, distance, pBot->grenade_action);
#endif
			return RETURN_TOOFAR;
		}
	}// END is bot carrying this grenade

#ifdef _DEBUG
	extern int debug_engine;
	// testing - check if weapons in array pSelect and in array pDelay are in same order
	if (debug_engine == 1 || in_bot_dev_level1)
	{
		if (pSelect[pBot->current_weapon.iId].iId != pDelay[pBot->current_weapon.iId].iId)
		{
			ALERT(at_console, "Weapon order in pSelect isn't same as in pDelay (weapon ID=%d)\n",
				pSelect[pBot->current_weapon.iId].iId);

			if (debug_engine)
				UTIL_DebugInFile("BotCombat|BotThrowGrenade()|pSelect != pDelay\n");

			return RETURN_NOTFIRED;
		}
	}
#endif

	return RETURN_NOTFIRED;
}

// NOTE: advance should check for some special weapons like knife and then set special
//		 longer time to allow bot to get faster to enemy
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
/*
* set correct advance time based on behaviour, botskill etc.
*/
inline void IsChanceToAdvance(bot_t *pBot)
{
	//NOTE: Move this array to be global (no need to set it for each bot)
	// time delays based on bot_skill
	// the lower bot skill level (better bots) the longer delays
	// Reaction time delay? [APG]RoboCop[CL]
	float delay[BOT_SKILL_LEVELS] = {3.0, 2.5, 2.0, 1.5, 1.0};
	//float delay[BOT_SKILL_LEVELS] = {7.0, 5.5, 4.0, 2.5, 1.0};

	// NOTE: these delays should be arrays based on bot_skill value
	float min_delay, max_delay;

	// generate basic chance
	int chance = RANDOM_LONG(1, 100);
	// get bot skill level
	int skill = pBot->bot_skill;

	// is bot attacker AND did NOT advanced in last few seconds
	if ((pBot->bot_behaviour & ATTACKER) && (chance < (45 + skill * 5)) && // best bot attacks in 50% and worst in 95% 
		((gpGlobals->time - (1.0 * delay[skill])) > pBot->f_combat_advance_time))
	{
		min_delay = 4.0;
		max_delay = 7.0;

		// is bot using m79 do shorter advance runs
		if (pBot->current_weapon.iId == fa_weapon_m79)
			pBot->f_combat_advance_time = gpGlobals->time +
					RANDOM_FLOAT(min_delay / 2, max_delay / 2);
		else
			pBot->f_combat_advance_time = gpGlobals->time + RANDOM_FLOAT(min_delay, max_delay);

		// if bot not in proned position
		if (!(pBot->pEdict->v.flags & FL_PRONED))
		{
			if (RANDOM_LONG(1, 100) < 10)
				SetStanceByte(pBot, BOT_CROUCHED);	// advance in crouched position
			else
				SetStanceByte(pBot, BOT_STANDING);
		}
	}
	// or is bot defender
	else if ((pBot->bot_behaviour & DEFENDER) && (chance < (5 + skill * 5))	&& // best bot attacks in 10% and worst in 55% 
		((gpGlobals->time - (2.0 * delay[skill])) > pBot->f_combat_advance_time))
	{
		min_delay = 2.0;
		max_delay = 5.0;

		pBot->f_combat_advance_time = gpGlobals->time + RANDOM_FLOAT(min_delay, max_delay);

		// if bot not in proned position
		if (!(pBot->pEdict->v.flags & FL_PRONED))
		{
			if (RANDOM_LONG(1, 100) < 25)
				SetStanceByte(pBot, BOT_CROUCHED);
			else
				SetStanceByte(pBot, BOT_STANDING);
		}
	}
	// standard behaviour type
	else if ((pBot->bot_behaviour & STANDARD) && (chance < (20 + skill * 5)) && // best bot attacks in 25% and worst in 70% 
		((gpGlobals->time - (1.5 * delay[skill])) > pBot->f_combat_advance_time))
	{
		min_delay = 3.0;
		max_delay = 6.0;

		pBot->f_combat_advance_time = gpGlobals->time + RANDOM_FLOAT(min_delay, max_delay);

		// if bot not in proned position
		if (!(pBot->pEdict->v.flags & FL_PRONED))
		{
			if (RANDOM_LONG(1, 100) < 20)
				SetStanceByte(pBot, BOT_CROUCHED);
			else
				SetStanceByte(pBot, BOT_STANDING);
		}
	}

	// leave the advance time untouched ie bot will stay in last Stance
	return;
}

/*
* do traceline in all three positions (proned, crouched & standing) to set the best one
* so that the bot still see & can shoot at enemy while having the best possible cover
*/
inline void CheckStance(bot_t *pBot)
{
	edict_t *pEdict = pBot->pEdict;
	edict_t *pEnemy = pBot->pBotEnemy;
	Vector v_botshead, v_enemy;
	float enemy_dist;
	TraceResult tr;

	// look through bots eyes
	v_botshead = pEdict->v.origin + pEdict->v.view_ofs;
	// at enemy head
	v_enemy = pEnemy->v.origin + pEnemy->v.view_ofs;
	
	enemy_dist = (pEdict->v.origin - pEnemy->v.origin).Length();

	// don't try to go prone if enemy is close
	if (enemy_dist < 800)		// was 500
	{
		// if using knife get up from prone
		if ((pBot->forced_usage == USE_KNIFE) || (pBot->current_weapon.iId == fa_weapon_knife))
		{
			// prevents go prone
			pBot->f_cant_prone = gpGlobals->time + 2.5;

			if (enemy_dist < 200)
				SetStanceByte(pBot, BOT_STANDING);
			else
			{
				if (RANDOM_LONG(1, 100) <= 35)
					SetStanceByte(pBot, BOT_CROUCHED);
				else
					SetStanceByte(pBot, BOT_STANDING);
			}
		}
		else
		{
			// if not already proned don't go prone
			if (!(pEdict->v.flags & FL_PRONED))
				pBot->f_cant_prone = gpGlobals->time + 2.5;
		}
			
		return;
	}

	// don't change Stance if enemy is still in the same distance
	// do it only in 20% of the time
	if ((pBot->f_prev_enemy_dist == enemy_dist) && (RANDOM_LONG(1, 100) < 80))
		return;

	/*
	UTIL_TraceLine(v_botshead, v_enemy, dont_ignore_monsters, ignore_glass,
				pEdict->v.pContainingEntity, &tr);

		//@@@@@vecEndPos
		ALERT(at_console, "duck try (%.2f) (hitgr %d) (flPlaneDist %.2f)\n",
			tr.flFraction, tr.iHitgroup, tr.flPlaneDist);
		ALERT(at_console, "duck try hitent (class %s) (net %s) (glob %s)\n",
			STRING(tr.pHit->v.classname), STRING(tr.pHit->v.netname),
			STRING(tr.pHit->v.globalname));
		ALERT(at_console, "duck try hitent(edict %x)\n", tr.pHit);
		ALERT(at_console, "(trEnd: x%.1f y%.1f z%.1f) (v_enemy: x%.1f y%.1f z%.1f) (pEnemy: x%.1f y%.1f z%.1f)\n",
			tr.vecEndPos.x, tr.vecEndPos.y, tr.vecEndPos.z,
			v_enemy.x, v_enemy.y, v_enemy.z,
			pEnemy->v.origin.x,	pEnemy->v.origin.y, pEnemy->v.origin.z);
		*/


	// is bot proned
	if (pEdict->v.flags & FL_PRONED)
	{
		// did it hit something?
		if (FPlayerVisible(v_enemy, pEdict) == VIS_NO)
		{
			// to try the crouched position we have to fake bot's head position
			v_botshead = pEdict->v.origin + pEdict->v.view_ofs + Vector(0, 0, 8);

			// didn't hit anything so change to crouched position
			if (FPlayerVisible(v_enemy, v_botshead, pEdict) == VIS_YES)
			{
				SetStanceByte(pBot, BOT_CROUCHED);

				return;
			}
			else
			{
				// try standing position
				v_botshead  = pEdict->v.origin + pEdict->v.view_ofs + Vector(0, 0, 42);

				// change to standing position
				if (FPlayerVisible(v_enemy, v_botshead, pEdict) == VIS_YES)
				{
					SetStanceByte(pBot, BOT_STANDING);

					return;
				}


				// NOTE: here should be some code that test strafe to side
			}
		}

		// keep proned position bot has clear view to enemy
		if (!(pBot->bot_behaviour & BOT_PRONED))
		{
			SetStanceByte(pBot, BOT_PRONED);
		}

		return;
	}

	// is bot crouched
	else if (pEdict->v.flags & FL_DUCKING)
	{
		// try proned position from time to time
		if ((RANDOM_LONG(1, 100) < 15) && (pBot->f_cant_prone < gpGlobals->time))
		{
			v_botshead = pEdict->v.origin + pEdict->v.view_ofs - Vector(0, 0, 8);

			// change to proned position
			if (FPlayerVisible(v_enemy, v_botshead, pEdict) == VIS_YES)
			{
				SetStanceByte(pBot, BOT_PRONED);
				
				return;
			}
		}
		// or try standing position from time to time
		else if (RANDOM_LONG(1, 100) < 25)
		{
			v_botshead = pEdict->v.origin + pEdict->v.view_ofs + Vector(0, 0, 34);

			// change to standing position
			if (FPlayerVisible(v_enemy, v_botshead, pEdict) == VIS_YES)
			{
				SetStanceByte(pBot, BOT_STANDING);
				
				return;
			}
		}

		// look through bots eyes
		// did it hit something?
		if (FPlayerVisible(v_enemy, pEdict) == VIS_NO)
		{
			// try standing position
			v_botshead = pEdict->v.origin + pEdict->v.view_ofs + Vector(0, 0, 34);

			// change to standing position
			if (FPlayerVisible(v_enemy, v_botshead, pEdict) == VIS_YES)
			{
				SetStanceByte(pBot, BOT_STANDING);

				return;
			}


			// NOTE: here should be some code that test strafe to side
		}

		// keep crouched position bot has clear view to enemy
		if (!(pBot->bot_behaviour & BOT_CROUCHED))
		{
			SetStanceByte(pBot, BOT_CROUCHED);
		}

		return;
	}

	// or bot must stand still ie NOT going to crouch or prone
	else if ((pEdict->v.bInDuck != 1) && (pEdict->v.flags & FL_ONGROUND) && (pBot->f_go_prone_time < gpGlobals->time))
	{
		// try proned position
		v_botshead = pEdict->v.origin + pEdict->v.view_ofs - Vector(0, 0, 42);

		// change to proned position
		if ((FPlayerVisible(v_enemy, v_botshead, pEdict) == VIS_YES) && (pBot->f_cant_prone < gpGlobals->time))
		{
			SetStanceByte(pBot, BOT_PRONED);

			return;
		}

		// try crouched position
		v_botshead = pEdict->v.origin + pEdict->v.view_ofs - Vector(0, 0, 34);

		// change to crouched position
		if (FPlayerVisible(v_enemy, v_botshead, pEdict) == VIS_YES)
		{
			SetStanceByte(pBot, BOT_CROUCHED);
			
			return;
		}

		// keep standing position bot has clear view to enemy
		if (!(pBot->bot_behaviour & BOT_STANDING))
		{
			SetStanceByte(pBot, BOT_STANDING);
		}

		return;
	}
}

/*
* returns TRUE if enemy is close enough based on current weapon
* returns FALSE if enemy is still quite far (ie bot can still move towards it)
* also sets best firemode for current weapon at this distance to enemy
*/
bool IsEnemyTooClose(bot_t *pBot, float enemy_distance)
{
	// sniper rifles
	if (UTIL_IsSniperRifle(pBot->current_weapon.iId))
	{
		if (enemy_distance < 1000 ||
			(pBot->IsTask(TASK_AVOID_ENEMY) && enemy_distance <= pBot->GetEffectiveRange()))
			return TRUE;
	}
	// machineguns
	else if (UTIL_IsMachinegun(pBot->current_weapon.iId))
	{
		if (enemy_distance < 600 ||
			(pBot->IsTask(TASK_AVOID_ENEMY) && enemy_distance <= pBot->GetEffectiveRange()))
			return TRUE;
	}
	// SMGs
	else if (UTIL_IsSMG(pBot->current_weapon.iId))
	{
		if (enemy_distance < 300)
		{
			// set full auto if not already set
			if (pBot->current_weapon.iFireMode != FM_AUTO)
				UTIL_ChangeFireMode(pBot, FM_AUTO, dont_test_weapons);

			// if bot uses m11 allow akimbo fire when the bot is this close
			if (pBot->current_weapon.iId == fa_weapon_m11)
				UTIL_SetBit(WA_USEAKIMBO, pBot->weapon_status);

			return TRUE;
		}
		else if (enemy_distance < 400)
		{
			// set full auto if not already set
			if (pBot->current_weapon.iFireMode != FM_AUTO)
				UTIL_ChangeFireMode(pBot, FM_AUTO, dont_test_weapons);
		}
		// does bot uses mp5 and enemy isn't that far so change to burst
		else if ((enemy_distance < 550) && (pBot->current_weapon.iId == fa_weapon_mp5a5))
		{
			// set full auto if not already set
			if (pBot->current_weapon.iFireMode != FM_BURST)
				UTIL_ChangeFireMode(pBot, FM_BURST, dont_test_weapons);
		}
		// enemy is quite far use single shot
		else
		{
			if (pBot->current_weapon.iFireMode != FM_SEMI)
				UTIL_ChangeFireMode(pBot, FM_SEMI, dont_test_weapons);

			if (UTIL_IsBitSet(WA_USEAKIMBO, pBot->weapon_status))
				UTIL_ClearBit(WA_USEAKIMBO, pBot->weapon_status);
		}

		// will ensure that the bot won't move during combat in this case
		if (pBot->IsTask(TASK_AVOID_ENEMY) && enemy_distance <= pBot->GetEffectiveRange())
			return TRUE;
	}
	// handguns
	else if (UTIL_IsHandgun(pBot->current_weapon.iId))
	{
		if (enemy_distance < 150)
		{
			// set 3r burst if not already set
			if (pBot->current_weapon.iFireMode != FM_BURST)
				UTIL_ChangeFireMode(pBot, FM_BURST, test_weapons);

			return TRUE;
		}
		// does bot uses ber93r and enemy isn't that far so change to burst
		else if ((enemy_distance < 200) && (pBot->current_weapon.iId == fa_weapon_ber93r))
		{
			// set 3r burst if not already set
			if (pBot->current_weapon.iFireMode != FM_BURST)
				UTIL_ChangeFireMode(pBot, FM_BURST, dont_test_weapons);
		}
		// enemy is quite far use single shot
		else
		{
			if (pBot->current_weapon.iFireMode != FM_SEMI)
				UTIL_ChangeFireMode(pBot, FM_SEMI, dont_test_weapons);
		}

		if (pBot->IsTask(TASK_AVOID_ENEMY) && enemy_distance <= pBot->GetEffectiveRange())
			return TRUE;
	}
	// no firearm
	else if (pBot->current_weapon.iId == fa_weapon_knife)
	{
		if (enemy_distance < 35)
			return TRUE;
	}
	// grenade launcher M79 in FA28
	else if (pBot->current_weapon.iId == fa_weapon_m79)
	{
		if (enemy_distance < 550 ||
			(pBot->IsTask(TASK_AVOID_ENEMY) && enemy_distance <= pBot->GetEffectiveRange()))
			return TRUE;
	}
	// shotguns
	else if (UTIL_IsShotgun(pBot->current_weapon.iId))
	{
		if (enemy_distance < 250 ||
			(pBot->IsTask(TASK_AVOID_ENEMY) && enemy_distance <= pBot->GetEffectiveRange()))
			return TRUE;
	}
	// it must be one of ARs
	else
	{
		if (enemy_distance < 300)
		{
			// set 3r burst for m16 if not already set or not in FA25 and below
			if ((pBot->current_weapon.iId == fa_weapon_m16) && (UTIL_IsOldVersion() == FALSE))
			{
				if (pBot->current_weapon.iFireMode != FM_BURST)
					UTIL_ChangeFireMode(pBot, FM_BURST, dont_test_weapons);
			}
			else
			{
				// set full auto if not already set
				if (pBot->current_weapon.iFireMode != FM_AUTO)
					UTIL_ChangeFireMode(pBot, FM_AUTO, test_weapons);
			}

			return TRUE;
		}
		// enemy isn't that far so change to full auto
		else if (enemy_distance < 450)
		{
			// set full auto if not already set
			if (pBot->current_weapon.iFireMode != FM_AUTO)
				UTIL_ChangeFireMode(pBot, FM_AUTO, test_weapons);
		}
		// enemy isn't that far so change to burst
		else if (enemy_distance < 650)
		{
			// set 3r burst if not already set
			if (pBot->current_weapon.iFireMode != FM_BURST)
				UTIL_ChangeFireMode(pBot, FM_BURST, test_weapons);
		}
		// enemy is quite far use single shot
		else
		{
			if (pBot->current_weapon.iFireMode != FM_SEMI)
				UTIL_ChangeFireMode(pBot, FM_SEMI, test_weapons);
		}

		if (pBot->IsTask(TASK_AVOID_ENEMY) && enemy_distance <= pBot->GetEffectiveRange())
			return TRUE;
	}

	return FALSE;
}

/*
* directs all in combat actions and behaviour
*/
void BotShootAtEnemy( bot_t *pBot )
{
	extern bool b_freeze_mode;

	float foe_distance;
	edict_t *pEdict = pBot->pEdict;
	Vector v_enemy;
	bool out_of_bipod_limit = FALSE;

	// bot lost his enemy and now can only watch that direction
	if (pBot->f_bot_wait_for_enemy_time >= gpGlobals->time)
	{
		v_enemy = pBot->v_last_enemy_position - pEdict->v.origin;

		pBot->f_prev_enemy_dist = v_enemy.Length();

		// don't move if can't see enemy
		pBot->move_speed = SPEED_NO;
		pBot->f_dont_check_stuck = gpGlobals->time + 1.0;

		// is chance to stand up to scan horizon
		if (RANDOM_LONG(1, 100) < 2)
		{
			// standing up is based on behaviour and chance
			if (((pBot->bot_behaviour & ATTACKER) && (RANDOM_LONG(1, 100) < 35)) ||
				((pBot->bot_behaviour & DEFENDER) && (RANDOM_LONG(1, 100) < 5)) ||
				((pBot->bot_behaviour & STANDARD) && (RANDOM_LONG(1, 100) < 20)))
			{
				if (pEdict->v.flags & FL_PRONED)
				{
					UTIL_GoProne(pBot);
					SetStanceByte(pBot, BOT_STANDING);
				}

				if (pBot->bot_behaviour & BOT_CROUCHED)
					SetStanceByte(pBot, BOT_STANDING);
			}
		}

		// keep crouched position
		if (pBot->bot_behaviour & BOT_CROUCHED)
			pEdict->v.button |= IN_DUCK;

		// we can't shoot at the enemy because we can't see him
		return;
	}

	// don't apply bandages if going to shoot
	pBot->bandage_time = -1.0;

	// the bot is under water but hasn't switched to knife yet
	if ((pEdict->v.waterlevel == 3) && (pBot->forced_usage != USE_KNIFE))
	{
		pBot->forced_usage = USE_KNIFE;
	}

	// aim for the head and/or body
	v_enemy = BotBodyTarget(pBot) - GetGunPosition(pEdict);

	pEdict->v.v_angle = UTIL_VecToAngles( v_enemy );

	if (pEdict->v.v_angle.y > 180)
		pEdict->v.v_angle.y -=360;

	// is bot using bipod
	if (pBot->IsTask(TASK_BIPOD))
	{
		// bot does turn normally when using bipod so we must somehow prevent that manually
		// we need to know in which direction (exact angle) bot looked to count correct limits
		// luckily FireArms stores these angles in pEdict->v.vuser1 and because
		// pEdict->v.vuser1 is already an angle value all we need to do, when bot exceed
		// the yaw limit, is just put the pEdict->v.vuser1.y value into pEdict->v.v_angle.y
		// (yaw variable) so bot will look again to initial bipod angle

		float bipod_limit = 45.0;

		// is bot trying to turn more than bipod allows so limit his yaw
		if (fabs(pEdict->v.vuser1.y - pEdict->v.v_angle.y) > bipod_limit)
		{
			pEdict->v.v_angle.y = pEdict->v.vuser1.y;

			// prevents shoot weapon
			out_of_bipod_limit = TRUE;
		}
	}

	// Paulo-La-Frite - START bot aiming bug fix
	if (pEdict->v.v_angle.x > 180)
		pEdict->v.v_angle.x -=360;

	// set the body angles to point the gun correctly
	pEdict->v.angles.x = pEdict->v.v_angle.x / 3;
	pEdict->v.angles.y = pEdict->v.v_angle.y;
	pEdict->v.angles.z = 0;

	// adjust the view angle pitch to aim correctly (MUST be after body v.angles stuff)
	pEdict->v.v_angle.x = -pEdict->v.v_angle.x;
	// Paulo-La-Frite - END

	BotFixIdealYaw(pEdict);

	v_enemy.z = 0;  // ignore z component (up & down)

	foe_distance = v_enemy.Length();  // how far away is the enemy scum?

	// if current enemy is quite far AND is right time AND NOT using snipe check for closer one
	if ((foe_distance > 500) && (pBot->search_closest_time <= gpGlobals->time) &&
		(UTIL_IsSniperRifle(pBot->current_weapon.iId) == FALSE))
	{
		int skill = pBot->bot_skill;
		float time_delay[BOT_SKILL_LEVELS] = {1.5, 2.0, 4.0, 7.0, 12.0};

		pBot->SetTask(TASK_FIND_ENEMY);
		pBot->search_closest_time = gpGlobals->time + time_delay[skill];
	}
	else
		pBot->RemoveTask(TASK_FIND_ENEMY);

	// is time for decision to move toward the enemy
	if ((pBot->f_combat_advance_time < gpGlobals->time) &&
		IsEnemyTooClose(pBot, foe_distance) == FALSE &&		// enemy is still quite far (has to be first because it includes firemode change code and we need to run this code)
		b_freeze_mode == FALSE &&							// bot is NOT freezed (mrfreeze)
		!pBot->IsTask(TASK_BIPOD) &&						// NOT using a bipod
		(pBot->sniping_time < gpGlobals->time))				// is NOT sniping
	{
		if (!pBot->IsTask(TASK_DONTMOVE) &&						// can move in combat
			!pBot->IsTask(TASK_IGNORE_ENEMY) &&					// NOT ignoring enemy (the bot must already be close enough)
			!pBot->IsTask(TASK_AVOID_ENEMY))					// NOT avoiding enemy (the bot must already be close enough)
			IsChanceToAdvance(pBot);
		// this is a special case (knife attack) when we need to allow the movement even if the
		// tests from above return "dont move", but the bot has to trace the forward
		if (pBot->IsTask(TASK_DEATHFALL))
			IsChanceToAdvance(pBot);
	}

	// is time to trace enemy to set best position/stance
	if ((pBot->f_check_stance_time < gpGlobals->time) &&
		(pBot->f_combat_advance_time < gpGlobals->time) &&		// NOT in advace
		(pBot->hide_time + 5.0 < gpGlobals->time) &&			// NOT in hiding
		!(pBot->IsTask(TASK_USETANK)) &&						// NOT using tank
		(pBot->f_reload_time < gpGlobals->time))				// NOT reloading
	{
		CheckStance(pBot);

		// NOTE: these delays should be based on bot_skill value
		float min_delay = 2.5;
		float max_delay = 5.0;
		pBot->f_check_stance_time = gpGlobals->time + RANDOM_FLOAT(min_delay, max_delay);
	}

	// is the bot paused OR doing medical treatment so just don't move and wait
	if ((pBot->f_pause_time > gpGlobals->time) ||
		(pBot->f_medic_treat_time > gpGlobals->time))
	{
		pBot->move_speed = SPEED_NO;
		pBot->f_dont_check_stuck = gpGlobals->time + 1.0;

		// if NOT in cover position try to go prone or at least crouch
		if (!(pEdict->v.flags & FL_PRONED) && !(pBot->bot_behaviour & BOT_CROUCHED) &&
			(pBot->f_stance_changed_time < gpGlobals->time))
		{
			// go prone only if paused
			if ((RANDOM_LONG(1, 100) > 50) && (pBot->f_medic_treat_time < gpGlobals->time))
				SetStanceByte(pBot, BOT_PRONED);
			else
				SetStanceByte(pBot, BOT_CROUCHED);
		}
	}

	// is the bot reloading the weapon?
	else if (pBot->f_reload_time > gpGlobals->time)
	{
		pBot->move_speed = SPEED_NO;
		pBot->f_dont_check_stuck = gpGlobals->time + 1.0;

		// if NOT in proned position do crouch for cover
		if (!(pEdict->v.flags & FL_PRONED) && (pBot->f_stance_changed_time < gpGlobals->time))
			SetStanceByte(pBot, BOT_CROUCHED);
	}

	// or is still sniping time?
	else if (pBot->sniping_time > gpGlobals->time)
	{
		// try bipod weapon if NOT already used AND bot is using one of these AND can handle it
		if (!(pBot->IsTask(TASK_BIPOD)) && IsBipodWeapon(pBot->current_weapon.iId) &&
			(pBot->f_bipod_try_time < gpGlobals->time) && (pBot->f_reload_time < gpGlobals->time))
		{
				FakeClientCommand(pEdict, "bipod", NULL, NULL);

				// set some time for next try
				pBot->f_bipod_try_time = gpGlobals->time + 3.0;
		}

		pBot->move_speed = SPEED_NO;
		pBot->f_dont_check_stuck = gpGlobals->time + 1.0;
	}

	// or is still wait time AND NOT dontmove flag AND NOT behind mounted gun AND NOT avoid enemy?
	else if ((pBot->wpt_wait_time >= gpGlobals->time) && !pBot->IsTask(TASK_DONTMOVE) &&
		!pBot->IsTask(TASK_USETANK) && !pBot->IsTask(TASK_AVOID_ENEMY))
	{
		pBot->f_dont_check_stuck = gpGlobals->time + 1.0;

		// is the bot sniping so hold position
		if (UTIL_IsSniperRifle(pBot->current_weapon.iId))
			pBot->move_speed = SPEED_NO;
		// is the bot holding MG
		else if (UTIL_IsMachinegun(pBot->current_weapon.iId))
		{
			// is enemy too far so break wpt_wait time
			if (foe_distance > 1400 * rg_modif)
				pBot->wpt_wait_time = gpGlobals->time - 0.1;
			// otherwise hold position
			else
				pBot->move_speed = SPEED_NO;
		}
		// otherwise break wpt_wait time
		else
			pBot->wpt_wait_time = gpGlobals->time - 0.1;
	}

	else if (pBot->crowded_wpt_index != -1 || UTIL_IsSmallRange(pBot->curr_wpt_index))
	{
		// will cause that the bot forgets about distant enemy and
		// will continue in navigation
		pBot->SetTask(TASK_AVOID_ENEMY);
	}

	// or is bot forced to stay at one place (ie don't move)
	else if (pBot->IsTask(TASK_DONTMOVE))
	{
		bot_weapon_select_t *pSelect = NULL;
		
		pSelect = &bot_weapon_select[0];

		pBot->move_speed = SPEED_NO;
		pBot->f_dont_check_stuck = gpGlobals->time + 1.0;

		// find the correct weapon in the array
		int select_index = 0;
		while (pSelect[select_index].iId != pBot->current_weapon.iId)
		{
			select_index++;
		}

		// allow changing weapon firemodes
		IsEnemyTooClose(pBot, foe_distance);

		// check if enemy is too far AND bot wasn't in hiding in last 5 seconds
		if ((foe_distance > pSelect[select_index].max_effective_distance) &&
			(pBot->hide_time + 5.0 < gpGlobals->time))
		{
			// if NOT in prone AND NOT fully crouched
			if (!(pEdict->v.flags & FL_PRONED) && !(pBot->bot_behaviour & BOT_CROUCHED) &&
				(pBot->f_stance_changed_time < gpGlobals->time))
			{
				// change to crouched position
				SetStanceByte(pBot, BOT_CROUCHED);
			}

			// set some time to stay away
			pBot->hide_time = gpGlobals->time + RANDOM_FLOAT(3.0, 10.0);




			// NOTE: Handle SMG's, Shotguns etc, bots stop and die when out of range


			//@@@@@@@@@@@@@@@
			//ALERT(at_console, "DontMove--Out of range (%.3f) (weap %d)\n",
				//pBot->hide_time, pSelect[select_index].iId);
		}
		// is hide time over but is still some time to next hide scan horizon for enemies
		else if ((pBot->hide_time < gpGlobals->time) && (pBot->hide_time + 5.0 > gpGlobals->time))
		{
			// change to standing position
			if ((!(pBot->bot_behaviour & BOT_STANDING)) && (RANDOM_LONG(1, 100) <= 50))
				SetStanceByte(pBot, BOT_STANDING);
		}
	}

	// otherwise if behind mounted gun 
	else if (pBot->IsTask(TASK_USETANK))
	{
		pBot->move_speed = SPEED_NO;
		pBot->f_dont_check_stuck = gpGlobals->time + 1.0;

		// set & keep standing position
		if (!(pBot->bot_behaviour & BOT_STANDING))
			SetStanceByte(pBot, BOT_STANDING);
	}

	// is it time to move towards the enemy (moves are based on carried weapon)
	//else if (pBot->f_combat_advance_time > gpGlobals->time)		// was ie PREV CODE
	if (pBot->f_combat_advance_time > gpGlobals->time)
	{
		// is the bot close enough to the enemy then don't move
		if (IsEnemyTooClose(pBot, foe_distance))
		{
			pBot->move_speed = SPEED_NO;
			pBot->f_dont_check_stuck = gpGlobals->time + 1.0;

			// reset advance time
			pBot->f_combat_advance_time = gpGlobals->time;
		}
		// otherwise move at full speed
		else
			pBot->move_speed = SPEED_MAX;
	}
	// otherwise don't move
	else
	{
		pBot->move_speed = SPEED_NO;
		pBot->f_dont_check_stuck = gpGlobals->time + 1.0;
	}

	if (pBot->f_strafe_time < gpGlobals->time)
		pBot->f_strafe_direction = 0.0;	// no strafe just for sure

	// is it time to shoot yet
	if ((out_of_bipod_limit == FALSE) && (pBot->f_shoot_time <= gpGlobals->time) &&
		(pBot->f_reload_time < gpGlobals->time) && (pBot->f_reaction_time < gpGlobals->time))
	{
		int result = -1;

		// should bot use a grenade
		if (pBot->forced_usage == USE_GRENADE)
		{
			if (pBot->grenade_time >= gpGlobals->time)
			{
				int result = BotThrowGrenade(v_enemy, pBot);

				// are we removing the fuse
				if (result == RETURN_PRIMING)
				{
					// give the engine some time to finish all things before calling the method again
					if (g_mod_version == FA_28)
						pBot->f_shoot_time = gpGlobals->time + 2.5;
					else
						pBot->f_shoot_time = gpGlobals->time + 1.5;

					// to prevent switching back to firearms or knife
					pBot->grenade_time = pBot->f_shoot_time + 1.0;
				}
				// if the bot can't throw grenade due to unsafe distance OR
				// already used all grenades
				// get back to the best weapon the bot can use
				else if ((result == RETURN_TOOCLOSE) || (result == RETURN_TOOFAR) ||
					((result == RETURN_NOTFIRED) && (pBot->grenade_action == ALTW_USED)))
				{
					//pBot->grenade_time = gpGlobals->time + 1.0;	// ORIGINAL CODE
					pBot->grenade_time = gpGlobals->time;

					if (pBot->main_no_ammo == FALSE)
						pBot->forced_usage = USE_MAIN;
					else if ((pBot->main_no_ammo == TRUE) && (pBot->backup_no_ammo == FALSE))
						pBot->forced_usage = USE_BACKUP;
					else
						pBot->forced_usage = USE_KNIFE;

#ifdef _DEBUG
					// testing
					if (in_bot_dev_level1)
						ALERT(at_console, "<<GRENADE>>Returned GREATER THAN currT (forcedW is %d | gren_act is %d | currT is %.2f)\n",
						pBot->forced_usage, pBot->grenade_action, gpGlobals->time);
#endif
				}
			}
			
			// after the bot thrown the grenade get back to the best weapon the bot can use
			if (pBot->grenade_time < gpGlobals->time)
			{
				//pBot->grenade_time = gpGlobals->time + 1.0;		//ORIGINAL CODE
				pBot->grenade_time = gpGlobals->time;

				if (pBot->main_no_ammo == FALSE)
					pBot->forced_usage = USE_MAIN;
				else if ((pBot->main_no_ammo == TRUE) && (pBot->backup_no_ammo == FALSE))
					pBot->forced_usage = USE_BACKUP;
				else
					pBot->forced_usage = USE_KNIFE;


#ifdef _DEBUG
				// testing
				if (in_bot_dev_level1)
					ALERT(at_console, "<<GRENADE>>Returned LESS THAN currT (forcedW is %d | gren_act is %d | currT is %.2f)\n",
					pBot->forced_usage, pBot->grenade_action, gpGlobals->time);
#endif
			}
		}
		// have grenades AND NOT used them both AND out of water AND time to use grenade AND
		// bot is NOT behind mounted gun AND some time after reload
		// in highest min_dist and lowest max_dist for grenades
		else if ((pBot->grenade_slot != -1) && (pBot->grenade_action != ALTW_USED) &&
			(pEdict->v.waterlevel != 3) &&
			(pBot->grenade_time + (5.0 * (pBot->bot_skill + 1)) < gpGlobals->time) &&
			!pBot->IsTask(TASK_USETANK) && (pBot->f_reload_time + 2.0 < gpGlobals->time) &&
			((foe_distance >= 800) && (foe_distance <= 1400)))
		{
			int chance = RANDOM_LONG(1, 100);

			// if main weapon is out of ammo use grenade 2/3 of the time
			if ((pBot->main_no_ammo) && (chance <= 66))
			{
				// set longer delay for grenades in FA28
				if (g_mod_version == FA_28)
					pBot->grenade_time = gpGlobals->time + 5.0;
				else
					pBot->grenade_time = gpGlobals->time + 3.0;

				// the bot must use some grenade now
				pBot->forced_usage = USE_GRENADE;

#ifdef _DEBUG
				// testing - it's time for grenades
				if (g_debug_bot_on)
					ALERT(at_console, "GRENADE when main weapon out of ammo\n");
#endif
			}
			// if have enough ammo for weapons use grenades only in 25% of the time
			else if (chance <= 25)
			{
				if (g_mod_version == FA_28)
					pBot->grenade_time = gpGlobals->time + 5.0;
				else
					pBot->grenade_time = gpGlobals->time + 3.0;

				pBot->forced_usage = USE_GRENADE;

#ifdef _DEBUG
				//@@@@@@@@@@@@@@@
				if (UTIL_IsSniperRifle(pBot->current_weapon.iId))
				{
					ALERT(at_console, "Grenade use auto Scope off\n");
				}
#endif

			}
			// otherwise check if can use grenades again after some time (based on bot skill)
			else
			{
				pBot->grenade_time = gpGlobals->time - RANDOM_FLOAT(0.0, 0.7);
			}
		}

		// check if bot can use full-auto fire AND holding machinegun AND have ammo AND
		// NOT using grenades AND (just for sure) bot is NOT behind mounted gun AND
		// weapon is set to full-auto
		if ((pBot->f_fullauto_time < gpGlobals->time) &&
			UTIL_IsMachinegun(pBot->current_weapon.iId) && (pBot->current_weapon.iClip != 0) &&
			(pBot->forced_usage != USE_GRENADE) && (pBot->grenade_time < gpGlobals->time) &&
			!pBot->IsTask(TASK_USETANK) && (pBot->current_weapon.iFireMode == 4))
		{
			// reset the full-auto time first
			// we don't know if bot use it again (ie is allowed to use full auto fire)
			pBot->f_fullauto_time = -1.0;

			// based on skill - Fire Rate Delay? [APG]RoboCop[CL]
			float min_delay[BOT_SKILL_LEVELS] = {0.1, 0.2, 0.3, 0.4, 0.5};
			float max_delay[BOT_SKILL_LEVELS] = {0.2, 0.3, 0.5, 0.8, 1.0};

			int skill = pBot->bot_skill;
			bool can_use = FALSE;

			// full auto usage is based on bot skill
			switch (pBot->bot_skill)
			{
				case 0:
					// if enemy is close use full auto often
					if ((foe_distance < 500) && (RANDOM_LONG(1, 100) > 25))
						can_use = TRUE;
					// if enemy is farther, didn't use it in last seconds and is 50% chance
					else if ((foe_distance < 1000) &&
						(pBot->f_fullauto_time + 0.75 < gpGlobals->time) &&
						(RANDOM_LONG(1, 100) > 50))
						can_use = TRUE;
					// use full auto only seldom if enemy is far and some time after last usage
					else if ((foe_distance > 1000) &&
						(pBot->f_fullauto_time + 1.5 < gpGlobals->time) &&
						(RANDOM_LONG(1, 100) > 95))
						can_use = TRUE;
					break;

				case 1:
					if ((foe_distance < 500) && (RANDOM_LONG(1, 100) > 20))
						can_use = TRUE;
					else if ((foe_distance < 1000) &&
						(pBot->f_fullauto_time + 0.65 < gpGlobals->time) &&
						(RANDOM_LONG(1, 100) > 45))
						can_use = TRUE;
					else if ((foe_distance > 1000) &&
						(pBot->f_fullauto_time + 1.25 < gpGlobals->time) &&
						(RANDOM_LONG(1, 100) > 90))
						can_use = TRUE;
					break;

				case 2:
					if ((foe_distance < 500) && (RANDOM_LONG(1, 100) > 15))
						can_use = TRUE;
					else if ((foe_distance < 1000) &&
						(pBot->f_fullauto_time + 0.5 < gpGlobals->time) &&
						(RANDOM_LONG(1, 100) > 40))
						can_use = TRUE;
					else if ((foe_distance > 1000) &&
						(pBot->f_fullauto_time + 1.0 < gpGlobals->time) &&
						(RANDOM_LONG(1, 100) > 85))
						can_use = TRUE;
					break;

				case 3:
					if ((foe_distance < 500) && (RANDOM_LONG(1, 100) > 10))
						can_use = TRUE;
					else if ((foe_distance < 1000) &&
						(pBot->f_fullauto_time + 0.4 < gpGlobals->time) &&
						(RANDOM_LONG(1, 100) > 35))
						can_use = TRUE;
					else if ((foe_distance > 1000) &&
						(pBot->f_fullauto_time + 0.75 < gpGlobals->time) &&
						(RANDOM_LONG(1, 100) > 75))
						can_use = TRUE;
					break;

				case 4:
					// if enemy is close use full auto very often
					if ((foe_distance < 500) && (RANDOM_LONG(1, 100) > 5))
						can_use = TRUE;
					else if ((foe_distance < 1000) &&
						(pBot->f_fullauto_time + 0.25 < gpGlobals->time) &&
						(RANDOM_LONG(1, 100) > 30))
						can_use = TRUE;
					// use full auto quite often if enemy is far
					else if ((foe_distance > 1000) &&
						(pBot->f_fullauto_time + 0.5 < gpGlobals->time) &&
						(RANDOM_LONG(1, 100) > 65))
						can_use = TRUE;
					break;
			}

			// if bot can use the full auto fire set its next use time
			if (can_use)
				pBot->f_fullauto_time = gpGlobals->time +
						RANDOM_FLOAT(min_delay[skill], max_delay[skill]);
		}

		// is the bot behing mounted gun
		if (pBot->IsTask(TASK_USETANK))
		{
			BotFireMountedGun(v_enemy, pBot);
		}

		// NOT using grenade AND out of water
		// is bot forced to use main weapon and have any main weapon OR
		// is forced to use backup weapon and have any backup weapon
		else if ((pBot->grenade_time < gpGlobals->time) && (pEdict->v.waterlevel != 3) &&
			(((pBot->forced_usage == USE_MAIN) && (pBot->main_weapon != -1)) ||
			((pBot->forced_usage == USE_BACKUP) && (pBot->backup_weapon != -1))))
		{
			result = BotFireWeapon(v_enemy, pBot);

			if (result == RETURN_RELOADING)
			{
				pBot->f_fullauto_time = -1.0;	// clear full-auto fire time
			}
			else if (result == RETURN_NOAMMO)
			{
				// try take main weapon if enough ammo
				if ((pBot->main_weapon != -1) && (pBot->main_no_ammo == FALSE))
					pBot->forced_usage = USE_MAIN;
				// try take backup weapon if enough ammo
				else if ((pBot->backup_weapon != -1) && (pBot->backup_no_ammo == FALSE))
					pBot->forced_usage = USE_BACKUP;
				else
					pBot->forced_usage = USE_KNIFE;

				// stop using bipod weapon if was used AND is time for handling it
				if (pBot->IsTask(TASK_BIPOD) && (pBot->f_bipod_try_time < gpGlobals->time))
				{
					FakeClientCommand(pEdict, "bipod", NULL, NULL);

					// set some time for next try
					pBot->f_bipod_try_time = gpGlobals->time + 3.0;
				}

				pBot->sniping_time = gpGlobals->time; // clear sniping time
				pBot->f_fullauto_time = -1.0;
			}
			else if (result == RETURN_TOOCLOSE)
			{
				if (pBot->forced_usage == USE_BACKUP)
				{
					// if main weapon exists AND have ammo for it AND NOT one of these
					if ((pBot->main_weapon != -1) &&
						(pBot->main_no_ammo == FALSE) &&
						((UTIL_IsSniperRifle(pBot->main_weapon) == FALSE) ||
						(pBot->main_weapon != fa_weapon_m79)))
					{
						pBot->forced_usage = USE_MAIN;
					}
					else
						pBot->forced_usage = USE_KNIFE;
				}
				else if (UTIL_IsSniperRifle(pBot->current_weapon.iId))
				{
					// have backup weapon AND have enough ammo AND NOT m79
					if ((pBot->backup_weapon != -1) && (pBot->backup_no_ammo == FALSE) &&
						(pBot->backup_weapon != fa_weapon_m79))
					{
						pBot->forced_usage = USE_BACKUP;
					}
					else
						pBot->forced_usage = USE_KNIFE;
				}
				// if using m79 as a main weapon force the bot to use backup if there's some
				else if (pBot->main_weapon == fa_weapon_m79)
				{
					if ((pBot->backup_weapon != -1) && (pBot->backup_no_ammo == FALSE))
					{
						pBot->forced_usage = USE_BACKUP;
					}
					else
						pBot->forced_usage = USE_KNIFE;
				}
				else
					pBot->forced_usage = USE_KNIFE;

				// stop using bipod weapon if was used AND is time for handling it
				if ((pBot->IsTask(TASK_BIPOD)) && (pBot->f_bipod_try_time < gpGlobals->time))
				{
					FakeClientCommand(pEdict, "bipod", NULL, NULL);

					// set some time for next try
					pBot->f_bipod_try_time = gpGlobals->time + 3.0;
				}

				pBot->sniping_time = gpGlobals->time;
			}
			else if (result == RETURN_TOOFAR)
			{
				// is it the time to override advance and allow the bot to move towards enemy?
				if ((pBot->forced_usage == USE_MAIN) &&
					(pBot->f_combat_advance_time < gpGlobals->time) &&
					(RANDOM_LONG(1, 100) <= 25))
				{
					// can we reset the advance time yet?
					if (pBot->f_overide_advance_time <= gpGlobals->time)
					{
						pBot->f_combat_advance_time = 0.0;

						// set the next time to try override the advance time
						pBot->f_overide_advance_time = gpGlobals->time + RANDOM_FLOAT(3.5, 5.0);


						//@@@@@@@@@@@@@@@@
						#ifdef _DEBUG
						ALERT(at_console, "***HEY Advance overriden (MAIN)\n");
						#endif



					}
				}

				else if (pBot->forced_usage == USE_BACKUP)
				{
					if ((pBot->main_weapon != -1) && (pBot->main_no_ammo == FALSE))
						pBot->forced_usage = USE_MAIN;
					else
					{
						if ((pBot->f_combat_advance_time < gpGlobals->time) &&
							(RANDOM_LONG(1, 100) <= 45))
						{
							if (pBot->f_overide_advance_time <= gpGlobals->time)
							{
								pBot->f_combat_advance_time = 0.0;

								pBot->f_overide_advance_time = gpGlobals->time +
									RANDOM_FLOAT(2.5, 4.0);



								//@@@@@@@@@@@@@@@@
								#ifdef _DEBUG
								ALERT(at_console, "***HEY Advance overriden (BACKUP)\n");
								#endif



							}
						}
					}
				}
			}
		}
		
		// NOT using grenades AND is bot forced to use knife
		else if ((pBot->grenade_time < gpGlobals->time) && (pBot->forced_usage == USE_KNIFE))
		{
			result = BotUseKnife(v_enemy, pBot);

			if ((result == RETURN_TOOFAR) && (pEdict->v.waterlevel != 3))
			{
				// try take back main weapon if enough ammo
				if (pBot->main_no_ammo == FALSE)
				{
					// using snipe or GL so take it back only if out of minimal range
					if (UTIL_IsSniperRifle(pBot->main_weapon) ||
						(pBot->main_weapon == fa_weapon_m79))
					{
						if (foe_distance > 500)
						{
							pBot->forced_usage = USE_MAIN;
						}
					}
					// otherwise take back main weapon
					else
					{
						pBot->forced_usage = USE_MAIN;
					}
				}
				// or try take back backup weapon if enough ammo
				else if (pBot->backup_no_ammo == FALSE)
				{
					// using m79 so take it back only if in safe distance
					if (pBot->backup_weapon == fa_weapon_m79)
					{
						if (foe_distance > 350)
						{
							pBot->forced_usage = USE_BACKUP;
						}
					}
					// otherwise take back backup weapon
					else
					{
						pBot->forced_usage = USE_BACKUP;
					}
				}
				else
				{
					if ((pBot->f_combat_advance_time < gpGlobals->time) &&
						(RANDOM_LONG(1, 100) > 35))
					{
						if (pBot->f_overide_advance_time <= gpGlobals->time)
						{
							pBot->f_combat_advance_time = 0.0;
							
							// if the bot is on "limited movement" path then
							// we have to scan the forward direction for danger
							if ((pBot->IsTask(TASK_AVOID_ENEMY) ||
								pBot->IsTask(TASK_IGNORE_ENEMY)) &&
								foe_distance < RANGE_MELEE)
								pBot->SetTask(TASK_DEATHFALL);

							pBot->f_overide_advance_time = gpGlobals->time + RANDOM_FLOAT(1.0, 3.0);


							//@@@@@@@@@@@@@@@@
							#ifdef _DEBUG
							ALERT(at_console, "***HEY Advance overriden (KNIFE)\n");
							#endif



						}
					}
				}
			}
			// selecting it
			else if (result == RETURN_TAKING)
				pBot->forced_usage = USE_KNIFE;
		}

#ifdef _DEBUG
		if (in_bot_dev_level2)
		{
			char msg[80];
			sprintf(msg, "bot skill level=%d (1 best)\n", pBot->bot_skill + 1);
			//ALERT(at_console, msg);
		}
#endif
	}

	// is it time to change Stance yet
	if ((pBot->f_stance_changed_time < gpGlobals->time) &&
		(pBot->f_go_prone_time < gpGlobals->time))
	{
		bool is_proned = FALSE;

		// should the bot go prone
		if (!(pEdict->v.flags & FL_PRONED) && (pBot->bot_behaviour & BOT_PRONED))
		{
			is_proned = TRUE;

			if (UTIL_GoProne(pBot))
			{		
#ifdef _DEBUG
				//@@@@@
				//ALERT(at_console, "GO Prone\n");
#endif

			}
		}
		// should bot be in crouched position
		else if (pBot->bot_behaviour & BOT_CROUCHED)
		{
			pEdict->v.button |= IN_DUCK;
		}
		// get bot to standing position from previous proned position
		else if ((pEdict->v.flags & FL_PRONED) && (pBot->bot_behaviour & BOT_STANDING))
		{
			is_proned = TRUE;

			if (UTIL_GoProne(pBot))
			{		
#ifdef _DEBUG
				//@@@@@
				//ALERT(at_console, "Stand UP from Prone\n");
#endif

			}

		}

		// if bot was proned or going prone set some time for next change
		if (is_proned)
			pBot->f_stance_changed_time = gpGlobals->time + RANDOM_FLOAT(4.0, 7.0);
	}

	// backup the enemy distance value
	pBot->f_prev_enemy_dist = (pBot->pBotEnemy->v.origin - pEdict->v.origin).Length();
}

/*
* directs bot actions to plant a claymore mine
*/
void BotPlantClaymore(bot_t *pBot)
{
	// has the bot a claymore
	if ((pBot->claymore_slot == -1) || ((pBot->bot_weapons & (1<<fa_weapon_claymore)) == FALSE))
		return;

	// take claymore if NOT currently in hands
	if ((pBot->f_use_clay_time < gpGlobals->time) && (pBot->clay_action == ALTW_NOTUSED) &&
		(pBot->current_weapon.iId != fa_weapon_claymore))
	{
		// get info about current weapon "handling" time
		float prev_weapon_delay = 1.0;

		// change weapon to claymore mine
		FakeClientCommand(pBot->pEdict, "weapon_claymore", NULL, NULL);

		// set the current action flag
		pBot->clay_action = ALTW_TAKING;

		// set time to finish this process
		pBot->f_use_clay_time = gpGlobals->time + 1.0 + prev_weapon_delay;

		// update wait time with prev weapon handling delay
		pBot->wpt_wait_time = pBot->f_use_clay_time + 1.0;
	}

	// has the bot already the mine in hands
	else if ((pBot->f_use_clay_time < gpGlobals->time) && (pBot->clay_action == ALTW_TAKING) &&
		(pBot->current_weapon.iId == fa_weapon_claymore))
	{
		// some safety time
		pBot->f_shoot_time = gpGlobals->time + RANDOM_FLOAT(0.3, 0.6);

		// update the action flag
		pBot->clay_action = ALTW_PREPARED;
	}

	return;
}
