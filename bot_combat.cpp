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
// bot_combat.cpp
// 
////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(WIN32)
#pragma warning(disable: 4005 91 4477)
#endif

#include "defines.h"

#include "extdll.h"
#include "util.h"
#include "cbase.h"

#include "bot.h"
#include "bot_func.h"
#include "bot_manager.h"
#include "bot_weapons.h"

extern bot_weapon_t weapon_defs[MAX_WEAPONS];

float rg_modif = 2.5;		// multiplier that modifies all effective ranges for game purpose

// better bot = longer delay between two sprints towards enemy (i.e. better bots hold their positions more often)
float g_combat_advance_delay[BOT_SKILL_LEVELS] = { 7.0, 5.5, 4.0, 2.5, 1.0 };

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
int default_ID = 0;
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
bool CanUseBackupInsteadofReload(bot_t *pBot, float enemy_distance = 0.0);
inline void IsChanceToAdvance(bot_t *pBot);
inline void CheckStance(bot_t *pBot, float enemy_distance);
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
					sprintf(msg, "** missing variable '%s'", er_val.key.c_str());

					PrintOutput(NULL, msg, MType::msg_error);
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

	ALERT(at_console, "MarineBot firearms weapons initialisation done\n");
}


/*
* checks the amount of magazines for main weapon
* and sets out of ammo if there are no magazines left
* and the weapon clip is also empty
*/
bool bot_t::CheckMainWeaponOutOfAmmo(const char* loc)
{
	if ((current_weapon.iClip == 0) && (current_weapon.iAmmo1 == 0) && (main_no_ammo == false) &&
		(weapon_action == W_READY) && (current_weapon.iId == main_weapon))
	{
		main_no_ammo = true;


#ifdef DEBUG
		if (loc != NULL)
		{
			char dbgmsg[256];
			sprintf(dbgmsg, "MainWeaponOutOfAmmo() called @ %s)\n", loc);
			HudNotify(dbgmsg, this);
		}
#else
		if (botdebugger.IsDebugActions())
			HudNotify("MAIN weapon completely out of ammo (no magazines)\n", this);
#endif // DEBUG

	}

	return main_no_ammo;
}


/*
* checks the amount of magazines for backup weapon
* and sets out of ammo if there are no magazines left
* and the weapon clip is also empty
*/
bool bot_t::CheckBackupWeaponOutOfAmmo(const char* loc)
{
	if ((current_weapon.iClip == 0) && (current_weapon.iAmmo1 == 0) && (backup_no_ammo == false) &&
		(weapon_action == W_READY) && (current_weapon.iId == backup_weapon))
	{
		backup_no_ammo = true;


#ifdef DEBUG
		if (loc != NULL)
		{
			char dbgmsg[256];
			sprintf(dbgmsg, "BackupWeaponOutOfAmmo() called @ %s)\n", loc);
			HudNotify(dbgmsg, this);
		}
#else
		if (botdebugger.IsDebugActions())
			HudNotify("BACKUP weapon completely out of ammo (no magazines)\n", this);
#endif // DEBUG

	}

	return backup_no_ammo;
}


/*
* returns the max effective range of given weapon
* it takes the current weapon by default
*/
float bot_t::GetEffectiveRange(int weapon_index)
{
	//																					NEW CODE 094
	if (weapon_index == NO_VAL)
	{
		// always try main weapon first
		// the bot may have backup weapon in hands at the moment, but can still switch back to main (unless it's empty)
		if ((main_weapon != NO_VAL) && !main_no_ammo)
			weapon_index = main_weapon;
		else
			// if the bot has no main weapon or is out of ammo for it then take the current one
			weapon_index = current_weapon.iId;
	}

	// if there's no weapon at all return zero ... just for sure
	if (weapon_index == NO_VAL)
		return 0.0;

	float range;

	range = bot_weapon_select[weapon_index].max_effective_distance;

	// see if we do limit the max distance the bot can see (ie. view distance)
	// if so and the weapon effective range is bigger then the limit
	// we will use the view distance limit instead
	if (internals.IsEnemyDistanceLimit() && (range > internals.GetEnemyDistanceLimit()))
		range = internals.GetEnemyDistanceLimit();

	return range;

	/*///																				NEW CODE 094 (prev code)
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
	/**/
}



/*
* checks if is team play on
*/
void BotCheckTeamplay(void)
{
	// is this mod Firearms?
	if (mod_id == FIREARMS_DLL)
		internals.SetTeamPlay(1.0);
	else
		internals.SetTeamPlay(CVAR_GET_FLOAT("mp_teamplay"));  // teamplay enabled?

	internals.SetTeamPlayChecked(true);
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
			react_time += (float) externals.GetReactionTime() / 2.0;	// use 150% of it
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
}

/*
* bot does these actions when do not have enemy at all or
* when do not currently see him (ie lost clear view)
*/
inline void DontSeeEnemyActions(bot_t *pBot)
{
	edict_t *pEdict = pBot->pEdict;

	// is there NO enemy for a while AND NOT doing any weapon beased action ...
	if ((pBot->bandage_time == -1.0) && pBot->NotSeenEnemyfor(0.5) && (pBot->weapon_action == W_READY))
	{
		// then bot is allowed to use bandages again
		pBot->bandage_time = gpGlobals->time;


#ifdef _DEBUG
		if (botdebugger.IsDebugActions())
		{
			HudNotify("bot_combat.cpp|DontSeeEnemy Actions() -> bandaging ALLOWED again\n", pBot);
		}
#endif

	}

	// if the bot doesn't use main weapon and can use it then try to switch back to it
	pBot->BotSelectMainWeapon("DontSeeEnemy Actions() -> time to switch back to MAIN WEAPON");

	// bot is waiting if the enemy become visible again AND current weapon is ready AND not going to/resume from prone
	if ((pBot->f_bot_wait_for_enemy_time > gpGlobals->time) && (pBot->weapon_action == W_READY) && !pBot->IsGoingProne())
	{
		//																		PREVIOUS CODE
		/*/


		// is the bot holding partly loaded m3/remington AND have ammo AND can manipulate with gun AND
		// not doing any waypoint action AND is some time bot seen an enemy OR is bot waiting for enemy
		if ((pBot->current_weapon.iId == fa_weapon_benelli) && (pBot->current_weapon.iClip < 8) &&
			(pBot->current_weapon.iAmmo1 != 0))
		{
			pBot->ReloadWeapon("bot_combat.cpp|DontSeeEnemy Actions() -> remington-benelli clip < 8");
		}
		// is the bot holding colt with less than half rounds in it AND have ammo AND
		// is some time bot seen an enemy OR is bot waiting for enemy
		else if ((pBot->current_weapon.iId == fa_weapon_anaconda) && (pBot->current_weapon.iClip < 3) &&
			(pBot->current_weapon.iAmmo1 != 0))
		{
			pBot->ReloadWeapon("bot_combat.cpp|DontSeeEnemy Actions() -> anaconda clip < 3");
		}
		// is clip empty AND have at least one magazine AND
		// is some time bot seen an enemy so reload
		else if ((pBot->current_weapon.iClip == 0) && (pBot->current_weapon.iAmmo1 != 0))
		{
			pBot->ReloadWeapon("bot_combat.cpp|DontSeeEnemy Actions() -> weapon clip is empty");
		}
		/**/

		if (UTIL_ShouldReload(pBot, "bot_combat.cpp|DontSeeEnemy Actions()"))
			pBot->ReloadWeapon("bot_combat.cpp|DontSeeEnemy Actions()");
	}

	// did the bot lost his enemy while switched to GL attachement on m16 or ak74
	if (pBot->secondary_active && pBot->NotSeenEnemyfor(1.5) && (pBot->weapon_action == W_READY) &&
		(pBot->f_bot_wait_for_enemy_time < gpGlobals->time) && (pBot->f_pause_time < gpGlobals->time) &&
		((pBot->current_weapon.iId == fa_weapon_m16) || (pBot->current_weapon.iId == fa_weapon_ak74)))
	{
		// then switch back to normal fire mode
		pBot->pEdict->v.button |= IN_ATTACK2;
		pBot->SetPauseTime(1.0);


#ifdef DEBUG
		//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@											NEW CODE 094 (remove it)
		if (botdebugger.IsDebugActions())
			HudNotify("Lost enemy while m203/gp25 ready -> switching back to normal fire mode on m16/ak74\n", pBot);
#endif // DEBUG



	}

	// didn't the bot seen ememy in last few seconds AND
	// is chance to speak (time to area clear)?
	if (!externals.GetDontSpeak() && pBot->NotSeenEnemyfor(8.0) &&
		((pBot->speak_time + RANDOM_FLOAT(25.5, 40.0)) < gpGlobals->time) && (RANDOM_LONG(1, 100) <= 1))
	{
		UTIL_Radio(pEdict, "clear");
		pBot->speak_time = gpGlobals->time;
	}

	// remove ignore enemy task if the bot is not by/on the "crucial" waypoint/path
	if (pBot->IsTask(TASK_IGNORE_ENEMY) && (pBot->crowded_wpt_index == -1) && !pBot->IsIgnorePath())
	{
		pBot->RemoveTask(TASK_IGNORE_ENEMY);

#ifdef _DEBUG
		if (botdebugger.IsDebugActions())
		{
			HudNotify("bot_combat.cpp|DontSeeEnemy Actions() -> Removed IGNORE ENEMY flag\n", pBot);
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
	extern edict_t* g_debug_bot;
	extern bool g_debug_bot_on;

	// can't use hudnotify() here, because it can crash hl engine if the bot gets hit
	// then the code in botclient.cpp -> fa dmg message can call this method
	// (clientprint in hudnotify() starts new engine msg before the fa_dmg one was finished and hl crashes)
	if (g_debug_bot_on && (g_debug_bot == pEdict) || !g_debug_bot_on)
	{
		char femsg[128];
		sprintf(femsg, "%s called ForgetEnemy()\n", name);
		ALERT(at_console, femsg);


		// so if there is a need to log things in file we have to do it manually right here
		UTIL_DebugInFile(femsg);

		//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

		// @@@@@@@@@@@@		^^^^ (uncomment logging in file if it is needed for tests) ^^^^					NEW CODE 094

		//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

	}

#endif

	// don't have an enemy anymore so null out the pointer
	pBotEnemy = NULL;	
	// clear his backed up position
	v_last_enemy_position = g_vecZero;//Vector(0, 0, 0);	
	// reset wait & watch time
	f_bot_wait_for_enemy_time = gpGlobals->time - 0.1;
	
	// if the bot is doing any weapon action like reloading for example we can't clear these two, because
	// bot can start one of these in the next frame and weapon action would be invalidated
	// (e.g. bot would end up with empty weapon)
	//if (weapon_action == W_READY)																	// NEW CODE 094
	//{
		// reset prone prevention time
		//f_cant_prone = gpGlobals->time - 0.1;														NEW CODE 094

		// no enemy so bot is clear to use bandages													<<< BUGS THINGS
		//bandage_time = gpGlobals->time;
	//}

	// reset enemy distance backup
	f_prev_enemy_dist = 0.0;
	// clear medical treatment flag
	RemoveTask(TASK_HEALHIM);
	// clear heavy tracelining
	RemoveTask(TASK_DEATHFALL);
	// bot most probably fired couple rounds at this enemy so let him check his weapon clip
	SetSubTask(ST_W_CLIP);
	// allow the bot to try deploying bipod later on when he finds new enemy
	RemoveWeaponStatus(WS_CANTBIPOD);

#ifdef _DEBUG
	curr_aim_offset = g_vecZero;//Vector(0, 0, 0);
	target_aim_offset = g_vecZero;//Vector(0, 0, 0);
#endif
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
				if ((UTIL_IsMachinegun(current_weapon.iId)) && !IsTask(TASK_BIPOD) && (RANDOM_LONG(1, 100) < 10))
				{
					sniping_time = gpGlobals->time;	// so stop sniping
#ifdef _DEBUG
					// testing - snipe time
					if (in_bot_dev_level2)
					{
						char msg[80];
						sprintf(msg, "BREAKING sniping time\n");
						HudNotify(msg);
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
				if (enemy_distance > GetEffectiveRange() && (enemy_distance > RANGE_MELEE))
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
				}

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
					//if (v_last_enemy_position == Vector(0, 0, 0))
					if (v_last_enemy_position == g_vecZero)
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
		else if (internals.IsEnemyDistanceLimit())
		{
			nearest_distance = internals.GetEnemyDistanceLimit();
		}
		else
		{
			// is the bot a sniper bot then there's no range limit to search for an enemy
			if (UTIL_IsSniperRifle(current_weapon.iId))
				nearest_distance = 9999;
			// otherwise search for "close" enemies
			else
				nearest_distance = internals.GetEnemyDistanceLimit();
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

				// ignore observerving player
				if (botdebugger.IsObserverMode() && !(pPlayer->v.flags & FL_FAKECLIENT))
					continue;

				if (internals.IsTeamPlayChecked() == false)  // check for team play...
					BotCheckTeamplay();

				// is team play enabled?
				if (internals.GetTeamPlay() > 0.0)
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

#ifdef DEBUG
		//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		if (botdebugger.IsDebugActions())
			HudNotify("***FindEnemy() -> Got NEW ENEMY!!!\n", this);
#endif // DEBUG

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
			((speak_time + RANDOM_FLOAT(25.5, 59.0)) < gpGlobals->time) && (RANDOM_LONG(1, 100) <= 50))
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
			if (botdebugger.IsDebugPaths())
				HudNotify("FindEnemy() -> PATROL path return flag SET on\n", this);
#endif
		}

		// we have to clear dontmove flag ie if bot is NOT near any of small range wpts or
		// his waypoint isn't dont move waypoint
		int last_wpt = prev_wpt_index.get();

		if (IsTask(TASK_DONTMOVEINCOMBAT) && (crowded_wpt_index == -1) &&
			(UTIL_IsSmallRange(curr_wpt_index) == FALSE) && // the bot is heading towards to it
			(UTIL_IsSmallRange(last_wpt) == FALSE) && // is standing at it
			(UTIL_IsDontMoveWpt(pEdict, curr_wpt_index, FALSE) == FALSE) &&
			(UTIL_IsDontMoveWpt(pEdict, last_wpt, TRUE) == FALSE))
		{
			RemoveTask(TASK_DONTMOVEINCOMBAT);


			//@@@@@@@@@@@@@22
#ifdef _DEBUG
			if (botdebugger.IsDebugActions() || botdebugger.IsDebugStuck())
			{
				HudNotify("FindEnemy() -> removing DONT MOVE IN COMBAT TASK\n", this);
			}
#endif


		}

		pPrevEnemy = NULL;	// null out pointer just for sure
	}

	DontSeeEnemyActions(this);

	// is some time after we saw an enemy so reset it
	// to prevent doing all those things (speaking etc.) again and again
	if (NotSeenEnemyfor(15.0))
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
	pBot->curr_aim_offset = target;
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

bool ignore_aim_error_code = false;
//bool ignore_aim_error_code = true;			// << for tests I need headshots all the time
float some_modifier = 1;
#endif

bool g_test_aim_code = false;		// global switch to allow the new aim patch even in release compilation

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
	bool use_aim_patch_v1 = false;		// current aiming patch implemented in version 092
	bool use_aim_patch_v2 = false;		// needs NEW CODE

	edict_t *pEdict = pBot->pEdict;
	edict_t *pBotEnemy = pBot->pBotEnemy;

	foe_distance = (pBotEnemy->v.origin - pEdict->v.origin).Length();

	// apply aim patch only in these lastest FA version
	if (g_mod_version == FA_29 || g_mod_version == FA_30)
	{
		// see if the user turned the test aim code under release compilation
		if (g_test_aim_code)
		{
			use_aim_patch_v2 = true;
		}
		// default setting is to use aim patch implemented in mb version 092
		else
		{
			int weapon = pBot->current_weapon.iId;

			// all these weapons seem to always work in FA2.9 and above (i.e. don't need any aim patch)
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
	}
	
	// NOTE: this is only temporary solution, the code is really complicated, ugly and not 100% working
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

		//@@@@
		if (dbl_coords)
		{
			x_corr *= 2.0;
			y_corr *= 2.0;
		}
		if (mod_coords && some_modifier != 0)
		{
			x_corr *= some_modifier;
			y_corr *= some_modifier;
		}
		if (inv_coords)
		{
			x_corr *= -1.0;
			y_corr *= -1.0;
		}


		//@@@@@@@
		//ALERT(at_console, "corrs (x=%.2f y=%.2f) dist %.2f (dist/1000 %.2f) BYaw %.2f (rotated %.2f) TYaw %.1f (rotated %.2f) HitAngle %.2f\n",
		//	x_corr, y_corr, foe_distance, foe_distance / 1000.0, bot_yaw_angle, RotateYawAngle(bot_yaw_angle), target_yaw_angle,
		//	RotateYawAngle(target_yaw_angle), hit_angle);
#endif
		
		
		target_origin = v_fake_origin + Vector (x_corr, y_corr, 0);
		target_head = pBotEnemy->v.view_ofs;
	}
#ifdef DEBUG
	// brand new aiming code should be here
	else if (use_aim_patch_v2 && !use_aim_patch_v1)
	{
		// TODO: write something that would really work
	}
#endif
	// standard aiming ... used in older FA versions need just this
	else
	{
		target_origin = pBotEnemy->v.origin;
		target_head = pBotEnemy->v.view_ofs;
	}



#ifdef _DEBUG
	/*/
	if (ignore_aim_error_code)		//@@@@@@@@@@@@temp to ignore the code that simulates human like errors
	{

	target = target_origin + target_head;
	
	extern edict_t *listenserver_edict;
	Vector color = Vector(255, 50, 50);
	Vector v_source = pBot->pEdict->v.origin + pBot->pEdict->v.view_ofs;
	DrawBeam(listenserver_edict, v_source, target, 20, color.x, color.y, color.z, 50);
	
	return target;

	}/**/
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
				if (hs_percentage > 33)
					target = target_origin + target_head;  // aim for the head (if you can find it)
				else
					target = target_origin;  // aim only for the body
				d_x = RANDOM_FLOAT(-6, 6) * f_scale;
				d_y = RANDOM_FLOAT(-6, 6) * f_scale;
				d_z = RANDOM_FLOAT(-10, 10) * f_scale;
				break;
			case 2:
				// FAIR, offset somewhat for x, y, and z
				if (hs_percentage > 40)
					target = target_origin + target_head;
				else
					target = target_origin;
				d_x = RANDOM_FLOAT(-10, 10) * f_scale;
				d_y = RANDOM_FLOAT(-10, 10) * f_scale;
				d_z = RANDOM_FLOAT(-14, 14) * f_scale;
				break;
			case 3:
				// POOR, offset for x, y, and z
				if (hs_percentage > 50)
					target = target_origin + target_head;
				else
					target = target_origin;
				d_x = RANDOM_FLOAT(-14, 14) * f_scale;
				d_y = RANDOM_FLOAT(-14, 14) * f_scale;
				d_z = RANDOM_FLOAT(-18, 18) * f_scale;
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
					d_x = RANDOM_FLOAT(-6, 6) * f_scale;
					d_y = RANDOM_FLOAT(-6, 6) * f_scale;
					d_z = RANDOM_FLOAT(-10, 10) * f_scale;
					break;
				case 2:
					// FAIR, offset somewhat for x, y, and z
					if (hs_percentage > 66)
						target = target_origin + target_head;
					else
						target = target_origin;
					d_x = RANDOM_FLOAT(-10, 10) * f_scale;
					d_y = RANDOM_FLOAT(-10, 10) * f_scale;
					d_z = RANDOM_FLOAT(-15, 15) * f_scale;
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

	return target;
}


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

	// check if bot can press the trigger
	if (botdebugger.IsDontShoot() == FALSE)
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

	// this is for handling M203/GP25 attachment, the time is based on in game tests in FA 3.0,
	// best time is what the engine allows us (delay between the switch command and firing the nade)
	// and all the other times are increased a little to represent less skilled bots
	float switch_delay[BOT_SKILL_LEVELS] = { 2.3, 2.5, 2.8, 3.2, 3.7 };

	float min_safe_distance, base_delay, min_delay, max_delay;
	
#ifdef _DEBUG
	// to print everything
	if (in_bot_dev_level2)
		in_bot_dev_level1 = TRUE;
#endif

	// if the weapon is NOT ready yet break it
	if (pBot->weapon_action != W_READY)
	{
		return RETURN_NOTFIRED;
	}

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

	// is this slot empty
	if (weaponID == -1)
	{
		// change to knife, we know that each player gets knife by default in Firearms
		pBot->UseWeapon(USE_KNIFE);

		return RETURN_NOTFIRED;
	}
	
	// if the bot doesn't carry this weapon
	if (pBot->current_weapon.iId != weaponID)
	{
		// set this flag to allow weapon change
		pBot->weapon_action = W_TAKEOTHER;

		if (botdebugger.IsDebugActions())
			HudNotify("FireWeapon() -> going to change current weapon\n", pBot);
		
		return RETURN_TAKING;
	}

	press_trigger = FALSE;		// reset press_trigger flag first

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
							pBot->current_weapon.iId, pBot->current_weapon.iClip, pBot->current_weapon.iAmmo1);
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
								pBot->current_weapon.iId, pBot->current_weapon.iClip, pBot->current_weapon.iAmmo1);
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
				// see if we can fast switch to backup weapon instead of realoding
				if (CanUseBackupInsteadofReload(pBot, distance))
				{
					return RETURN_NOTFIRED;
				}

				pBot->ReloadWeapon("bot_combat.cpp|FireWeapon()|weapon clip is empty");

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
#endif // DEBUG

			pBot->ReloadWeapon("bot_combat.cpp|FireWeapon()|2nd m11 is empty");

			return RETURN_RELOADING;
		}

		// these weapons have to ignore safe distance specified in the struct
		// it's used only for firing the gl attachment
		if ((pBot->current_weapon.iId == fa_weapon_m16) || (pBot->current_weapon.iId == fa_weapon_ak74))
			min_safe_distance = 0.0;		
		else
			min_safe_distance = pSelect[weaponID].min_safe_distance;

		base_delay = pDelay[weaponID].primary_base_delay;
		min_delay = pDelay[weaponID].primary_min_delay[pBot->bot_skill];
		max_delay = pDelay[weaponID].primary_max_delay[pBot->bot_skill];

		// carries the bot this weapon AND is that weapon active (ie. in hands)
		if ((pBot->bot_weapons & (1<<pSelect[weaponID].iId)) && (pBot->current_weapon.isActive == 1))
		{
			// is the bot in effective range of this weapon
			if ((distance <= pSelect[weaponID].max_effective_distance) && (distance >= min_safe_distance))
			{				
#ifdef _DEBUG	// testing - firing weapon and ammo info
				if (in_bot_dev_level1)
				{					
					ALERT(at_console, "FIRE weapon ID=%d Array ID=%d (iClip=%d Mags=%d Dist=%.2f)\n",
						pBot->current_weapon.iId, pSelect[weaponID].iId,
						pBot->current_weapon.iClip, pBot->current_weapon.iAmmo1, distance);
				}
#endif
				press_trigger = TRUE;	// bot is ready to fire

				// don't move if using one of these weapons
				if (UTIL_IsSniperRifle(pBot->current_weapon.iId) || UTIL_IsMachinegun(pBot->current_weapon.iId))
				{
#ifdef _DEBUG		// testing - special weapons slow down
					if (in_bot_dev_level2)
						ALERT(at_console, "Must slow down to fire weapon ID=%d\n", pBot->current_weapon.iId);
#endif
					// is it time to set new sniping time
					if ((pBot->sniping_time < gpGlobals->time) && (pBot->f_combat_advance_time < gpGlobals->time))
					{
						bool start_sniping = FALSE;

						// is the bipod down then do snipe all the time
						if (pBot->IsTask(TASK_BIPOD))
							start_sniping = TRUE;
						// does the bot use m82 AND is 95% chance
						else if ((pBot->current_weapon.iId == fa_weapon_m82) && (RANDOM_LONG(1, 100) <= 95))
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
							{
								/*/
								char msg[80];
								sprintf(msg, "Sniping time set %.2f\n", pBot->sniping_time);
								HudNotify(msg, pBot);
								/**/
							}
#endif
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
#ifdef _DEBUG
						if (in_bot_dev_level2)
							HudNotify("Using unscoped in FA25\n", pBot);
#endif
					}

					// has the bot this weapon AND scope is NOT active AND is snipe time
					else if (((pBot->current_weapon.iId == fa_weapon_ssg3000) ||
						(pBot->current_weapon.iId == fa_weapon_svd)) &&
						(pEdict->v.fov == NO_ZOOM) && (pBot->sniping_time > gpGlobals->time))
					{
						pEdict->v.button |= IN_ATTACK2;				// switch to scope
						pBot->f_shoot_time = gpGlobals->time + 0.4;	// time to take effect

#ifdef _DEBUG
						// testing - weapon with scope
						if (in_bot_dev_level2)
							ALERT(at_console, "Trying scope 1X (WeaponID=%d)\n", pBot->current_weapon.iId);
#endif
						
						return RETURN_SECONDARY;
					}

					else if ((pBot->current_weapon.iId == fa_weapon_m82) && (pBot->sniping_time > gpGlobals->time))
					{
						if (pEdict->v.fov == ZOOM_1X)
						{
							pEdict->v.button |= IN_ATTACK2;
							pBot->f_shoot_time = gpGlobals->time + 0.2;
#ifdef _DEBUG
							// testing - weapon with scope
							if (in_bot_dev_level2)
								ALERT(at_console, "Trying scope 2X (WeaponID=%d)\n", pBot->current_weapon.iId);
#endif
						}
						else if (pEdict->v.fov == NO_ZOOM)
						{
							pEdict->v.button |= IN_ATTACK2;
							pBot->f_shoot_time = gpGlobals->time + 0.4;
#ifdef _DEBUG
							// testing - weapon with scope
							if (in_bot_dev_level2)
								ALERT(at_console, "Trying scope 1X (WeaponID=%d)\n", pBot->current_weapon.iId);
#endif
						}

						return RETURN_SECONDARY;
					}

					// if the bot is moving too fast then don't allow shooting
					if (UTIL_IsSniperRifle(pBot->current_weapon.iId) && (pEdict->v.velocity.Length() > 0))
					{
						return RETURN_NOTFIRED;
					}
					// we know that the bot can use only machinegun now
					// don't allow shoting while in move in FA29
					if (((g_mod_version == FA_29) || (g_mod_version == FA_30)) && (pEdict->v.velocity.Length() > 0))
					{
						return RETURN_NOTFIRED;
					}
					// otherwise allow shooting only if crouch or prone movements
					else if ((pEdict->v.velocity.Length() > 0) &&
						(pBot->IsBehaviour(BOT_PRONED) == FALSE) && (pBot->IsBehaviour(BOT_CROUCHED) == FALSE))
					{
						return RETURN_NOTFIRED;
					}
				}

				// has the bot one of these weapons? then try to switch to the attached grenade launcher
				else if ((pBot->current_weapon.iId == fa_weapon_m16) || (pBot->current_weapon.iId == fa_weapon_ak74))
				{
					// is the bot in safe distance AND is chance to use gl AND
					// have ammo2 AND necessary skill (arty1)
					if ((distance > pSelect[weaponID].min_safe_distance) && (RANDOM_LONG(1, 100) < 15) &&
						(pBot->current_weapon.iAttachment > 0) && (pBot->bot_fa_skill & ARTY1))
					{
						// there's no switching to gl attachment in this version
						// it will just fire it on IN_ATTACK2 command
						if (UTIL_IsOldVersion())
						{
							pBot->secondary_active = TRUE;
							// add short delay to allow the bot to aim a bit up before firing the gl
							pBot->f_shoot_time = gpGlobals->time + base_delay;
						}
						else
						{
							pEdict->v.button |= IN_ATTACK2;			// switch to attached gl
							//pBot->f_shoot_time = gpGlobals->time + 1.5;	// time to take effect		NEW CODE 094 (prev code)
							
							// time to take effect
							pBot->f_shoot_time = gpGlobals->time + switch_delay[pBot->bot_skill];
						}

#ifdef _DEBUG
						if (in_bot_dev_level2)						
							ALERT(at_console, "Changing to GL (ID=%d)\n", pBot->current_weapon.iId);
#endif

						return RETURN_SECONDARY;
					}
				}

				// don't move if using m79
				else if (pBot->current_weapon.iId == fa_weapon_m79)
				{
					if ((pBot->sniping_time < gpGlobals->time) && (pBot->f_combat_advance_time < gpGlobals->time) &&
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
							HudNotify(msg);
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

#ifdef _DEBUG // testing - check if weapons in array pSelect and in array pDelay are in same order
		extern int debug_engine;
		if (debug_engine == 1)
		{
			if (pSelect[weaponID].iId != pDelay[weaponID].iId)
			{
				ALERT(at_console, "Weapon order in pSelect(weapon ID=%d) isn't same as in pDelay(weapon ID=%d)\n",
					pSelect[weaponID].iId, pDelay[weaponID].iId);

				UTIL_DebugInFile("BotCombat|FireWeapon()|pSelect != pDelay\n");

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
				// first try to fast switch to backup weapon
				if (CanUseBackupInsteadofReload(pBot, distance))
				{
					return RETURN_NOTFIRED;
				}

				pBot->ReloadWeapon("bot_combat.cpp|FireWeapon()|secondary_active -> zoomed sniper rifle is empty");

				return RETURN_RELOADING;
			}
		}

		// is it time to set new sniping time
		if (UTIL_IsSniperRifle(pBot->current_weapon.iId) && (pBot->sniping_time < gpGlobals->time) &&
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

		// is the weapon m16+m203 OR ak74+gp25 AND has the bot NO ammo in attached gl chamber
		if (((pBot->current_weapon.iId == fa_weapon_m16) || (pBot->current_weapon.iId == fa_weapon_ak74)) &&
			(pBot->current_weapon.iAttachment < 1))
		{
			if (UTIL_IsOldVersion())
				pBot->secondary_active = FALSE;

			pEdict->v.button |= IN_ATTACK2;	// get back primary fire
			pBot->f_shoot_time = gpGlobals->time + switch_delay[pBot->bot_skill]; // time to take effect

			return RETURN_FIRED; // like it was fired
		}

		min_safe_distance = pSelect[weaponID].min_safe_distance;
		base_delay = pDelay[weaponID].primary_base_delay;
		min_delay = pDelay[weaponID].primary_min_delay[pBot->bot_skill];
		max_delay = pDelay[weaponID].primary_max_delay[pBot->bot_skill];

		// if still in safe distance
		if (distance >= min_safe_distance)
		{
			press_trigger = TRUE;

#ifdef _DEBUG
			if ((in_bot_dev_level2) &&
				((pBot->current_weapon.iId == fa_weapon_m16) || (pBot->current_weapon.iId == fa_weapon_ak74)))
			{
				ALERT(at_console, "SECONADRY_active -- gl is going to be fired (ID=%d)\n", pBot->current_weapon.iId);
			}
			else if (in_bot_dev_level2)
			{
				ALERT(at_console, "SECONADRY_active -- firing weapon (ID=%d)\n", pBot->current_weapon.iId);
			}
#endif
		}
		// otherwise don't fire - risk your life enemy is too close
		else
		{
			// if using m16 or ak74 gl attachment get back to primary fire mode
			if ((pBot->current_weapon.iId == fa_weapon_m16) || (pBot->current_weapon.iId == fa_weapon_ak74))
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

				return RETURN_FIRED; // like it was fired
			}

			return RETURN_TOOCLOSE;
		}
	}// END is secondary_active == TRUE

	// everything is done so fire the weapon and set correct fire delay - based on skill
	if (press_trigger)
	{
		// check if bot can press trigger
		if (botdebugger.IsDontShoot() == FALSE)
		{
			// do we need to fire m16 attached gl in FA25 OR akimbo m11 in FA 2.5
			if (((pBot->current_weapon.iId == fa_weapon_m16) && pBot->secondary_active && UTIL_IsOldVersion()) ||
				pBot->IsWeaponStatus(WS_USEAKIMBO))
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
			(pBot->current_weapon.iId != fa_weapon_saiga) && (pBot->current_weapon.iFireMode == FM_AUTO)) ||
			((pBot->f_fullauto_time > 0.0) && UTIL_IsMachinegun(pBot->current_weapon.iId)))
		{
			pBot->f_shoot_time = gpGlobals->time;
		}
		else
		{
			pBot->f_shoot_time = gpGlobals->time + base_delay + RANDOM_FLOAT(min_delay, max_delay);
		}

		//																				NEW CODE 094
		// so we can reload weapon after the battle if the bot ends it with almost empty clip
		pBot->SetSubTask(ST_W_CLIP);

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

	if (pBot->weapon_action != W_READY)
	{
		return RETURN_NOTFIRED;
	}

	if (pBot->current_weapon.iId != fa_weapon_knife)
	{
		pBot->weapon_action = W_TAKEOTHER;

		if (botdebugger.IsDebugActions())
			HudNotify("UseKnife() -> going to change current weapon\n", pBot);

		return RETURN_TAKING;
	}

#ifdef _DEBUG
	// testing - is it knife?
	if (in_bot_dev_level1)
		ALERT(at_console, "Weapon id=%d Array id=%d\n",
			pBot->current_weapon.iId, pSelect[pBot->current_weapon.iId].iId);
#endif

	if (distance <= pSelect[pBot->current_weapon.iId].max_effective_distance)
	{
		int skill = pBot->bot_skill;
		float base_delay, min_delay, max_delay; 

		// 2/3 of the time use primary attack
		if (RANDOM_LONG(1, 100) <= 66)
		{
			if (botdebugger.IsDontShoot() == FALSE)
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
			float min_d_array[BOT_SKILL_LEVELS] = {0.0, 0.1, 0.2, 0.45, 0.65};
			float max_d_array[BOT_SKILL_LEVELS] = {0.1, 0.2, 0.3, 0.7, 1.0};

			if (botdebugger.IsDontShoot() == FALSE)
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

	if (pBot->weapon_action != W_READY)
	{
		return RETURN_TAKING;
	}

	// if something went wrong
	if (pBot->grenade_slot == NO_VAL)
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
		//@@@@@@@@@@@@@@@
		HudNotify("<<BUG>>Invalid run - no grenades - already used before\n", true, pBot);
#endif
		return RETURN_NOTFIRED;
	}

	if (pBot->current_weapon.iId != pBot->grenade_slot)
	{
		if (pBot->bot_weapons & (1<<pBot->grenade_slot))
		{

#ifdef _DEBUG
			// testing
			if (in_bot_dev_level2)
				ALERT(at_console, "<<GRENADE>>Not in hands yet (currW is %d)\n", pBot->current_weapon.iId);
#endif

			if (botdebugger.IsDebugActions())
				HudNotify("ThrowGrenade() -> going to change current weapon\n", pBot);

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

			if (pBot->grenade_slot != NO_VAL)
				sprintf(error_msg, "<<BUG>>ThrowGrenade() called after using the last grenade. Try inceasing the primary_base_delay value in marine_bot\\weapons\\modversion file for weapon %s\n", STRING(weapon_defs[pBot->grenade_slot].szClassname));
			else
				sprintf(error_msg, "<<BUG>>ThrowGrenade() called after using the last grenade. Try inceasing the primary_base_delay value in marine_bot\\weapons\\modversion file for all grenades\n");
			UTIL_DebugInFile(error_msg);

			pBot->grenade_action = ALTW_USED;
			return RETURN_NOTFIRED;
		}
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
			if (botdebugger.IsDontShoot() == FALSE)
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
					if ((g_mod_version == FA_24) && (pBot->current_weapon.iId == fa_weapon_frag) &&
						(RANDOM_LONG(1, 500) > 1))
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
					(double)pBot->f_shoot_time - gpGlobals->time);
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

			//@@@@@@@@@@@@@@@
			HudNotify("ThrowGrenade() -> returning TOO CLOSE!\n", pBot);
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
				UTIL_DebugInFile("BotCombat|ThrowGrenade()|pSelect != pDelay\n");

			return RETURN_NOTFIRED;
		}
	}
#endif

	return RETURN_NOTFIRED;
}

/*
* checks if the bot can switch to backup weapon instead of trying to reload in the middle of battle
*/
bool CanUseBackupInsteadofReload(bot_t *pBot, float enemy_distance)
{
	// see if the bot doesn't use bipod at the moment and if he has usable backup weapon
	// and the enemy must be in range of backup weapon
	if ((pBot->forced_usage == USE_MAIN) && !pBot->IsTask(TASK_BIPOD) &&
		(UTIL_IsHandgun(pBot->backup_weapon) || UTIL_IsSMG(pBot->backup_weapon)) &&
		(pBot->backup_no_ammo == FALSE) && (enemy_distance <= pBot->GetEffectiveRange(pBot->backup_weapon)))
	{
#ifdef DEBUG
		// testing - switch to sidearm instead of releading main weapon
		if (in_bot_dev_level2)
		{
			ALERT(at_console, "Going to switch to sidearm instead of reloading ID=%d (inClip=%d) (backupW=%d)\n",
				pBot->current_weapon.iId, pBot->current_weapon.iClip, pBot->backup_weapon);
		}
#endif // DEBUG


		// switch to backup weapon
		pBot->UseWeapon(USE_BACKUP);

		return TRUE;
	}

	return FALSE;
}

/*
* set correct advance time based on behaviour, botskill and used weapon
*/
inline void IsChanceToAdvance(bot_t *pBot)
{
	float since_last_advance, min_delay, max_delay;

	// generate basic chance
	int chance = RANDOM_LONG(1, 100);
	int skill_modifier = 0;

	// get bot skill level
	int skill = pBot->bot_skill;

	if (pBot->current_weapon.iId == fa_weapon_knife)
	{
		// time delay since last advance is based on bot_skill where better bots don't move forward that much, but
		// if the bot is using knife then the logic is inverted, which means better bots do advance often
		since_last_advance = g_combat_advance_delay[BOT_SKILL_LEVELS - 1 - skill];

		// the chance to advance follows the same logic as the time delay so if the bot is using knife
		// then the best bots have highest chance to move towards the enemy
		skill_modifier = (BOT_SKILL_LEVELS - skill) * 5;
	}
	else
	{
		since_last_advance = g_combat_advance_delay[skill];
		skill_modifier = (skill + 1) * 5;
	}

	// is bot attacker AND did NOT advanced in last few seconds
	if (pBot->IsBehaviour(ATTACKER) &&
		(chance < (45 + skill_modifier)) &&	// best bot attacks in 50% of the time and worst in 70% of the time
											// unless it's the knife case where it is vice versa
		((gpGlobals->time - (1.0 * since_last_advance)) > pBot->f_combat_advance_time))
	{
		// NOTE: perhaps these delays could be arrays based on bot_skill value
		min_delay = 4.0;
		max_delay = 7.0;

		// is bot using m79 do shorter advance sprints
		if (pBot->current_weapon.iId == fa_weapon_m79)
			pBot->f_combat_advance_time = gpGlobals->time + RANDOM_FLOAT(min_delay / 2, max_delay / 2);
		else
			pBot->f_combat_advance_time = gpGlobals->time + RANDOM_FLOAT(min_delay, max_delay);

		// if bot is NOT in proned position
		if (pBot->IsBehaviour(BOT_PRONED) == FALSE)
		{
			// try to advance in crouched position
			if (RANDOM_LONG(1, 100) < 10)
				SetStance(pBot, GOTO_CROUCH, TRUE, "ChanceToAdvance() -> forced GOTO crouch");
			else
				SetStance(pBot, GOTO_STANDING, TRUE, "ChanceToAdvance() -> forced GOTO standing");
		}
	}
	// or is bot defender
	else if (pBot->IsBehaviour(DEFENDER) &&
		(chance < (5 + skill_modifier)) &&		// best bot attacks in 10% of the time and worst in 30% of the time
		((gpGlobals->time - (2.0 * since_last_advance)) > pBot->f_combat_advance_time))
	{
		min_delay = 2.0;
		max_delay = 5.0;

		pBot->f_combat_advance_time = gpGlobals->time + RANDOM_FLOAT(min_delay, max_delay);

		// if bot not in proned position
		if (pBot->IsBehaviour(BOT_PRONED) == FALSE)
		{
			if (RANDOM_LONG(1, 100) < 25)
				SetStance(pBot, GOTO_CROUCH, TRUE, "ChanceToAdvance() -> forced GOTO crouch");
			else
				SetStance(pBot, GOTO_STANDING, TRUE, "ChanceToAdvance() -> forced GOTO standing");
		}
	}
	// standard behaviour type
	else if (pBot->IsBehaviour(STANDARD) &&
		(chance < (20 + skill_modifier)) &&		// best bot attacks in 25% of the time and worst in 45% of the time
		((gpGlobals->time - (1.5 * since_last_advance)) > pBot->f_combat_advance_time))
	{
		min_delay = 3.0;
		max_delay = 6.0;

		pBot->f_combat_advance_time = gpGlobals->time + RANDOM_FLOAT(min_delay, max_delay);

		// if bot not in proned position
		if (pBot->IsBehaviour(BOT_PRONED) == FALSE)
		{
			if (RANDOM_LONG(1, 100) < 20)
				SetStance(pBot, GOTO_CROUCH, TRUE, "ChanceToAdvance() -> forced GOTO crouch");
			else
				SetStance(pBot, GOTO_STANDING, TRUE, "ChanceToAdvance() -> forced GOTO standing");
		}
	}

	// leave the advance time untouched ie bot will stay in last Stance
	return;
}

/*
* do traceline in all three positions (proned, crouched & standing) to set the best one
* so that the bot still see & can shoot at enemy while having the best possible cover
*/
inline void CheckStance(bot_t *pBot, float enemy_distance)
{
	edict_t *pEdict = pBot->pEdict;
	edict_t *pEnemy = pBot->pBotEnemy;
	Vector v_botshead, v_enemy;
	TraceResult tr;

	// look through bots eyes
	v_botshead = pEdict->v.origin + pEdict->v.view_ofs;
	// at enemy head
	v_enemy = pEnemy->v.origin + pEnemy->v.view_ofs;

	// don't try to go prone if enemy is close
	if (enemy_distance < 800)		// was 500
	{
		// if using knife get up from prone
		if ((pBot->forced_usage == USE_KNIFE) || (pBot->current_weapon.iId == fa_weapon_knife))
		{
			// prevents going prone
			//pBot->f_cant_prone = gpGlobals->time + 2.5;							NEW CODE 094 (bug fix - sure it prevents going prone, but also prevents standing up if bot is already proned)

			if (enemy_distance < 200)
				SetStance(pBot, GOTO_STANDING, TRUE, "Check Stance()|UseKNIFE -> forced GOTO standing when foe < 200");
			else
			{
				if (RANDOM_LONG(1, 100) <= 35)
					SetStance(pBot, GOTO_CROUCH, "Check Stance()|UseKNIFE -> GOTO crouch when foe < 800");
				else
					SetStance(pBot, GOTO_STANDING, "Check Stance()|UseKNIFE -> GOTO standing when foe < 800");
			}
		}
		else
		{
			// if not already proned don't go prone
			if (pBot->IsBehaviour(BOT_PRONED) == FALSE)
			{
				//pBot->f_cant_prone = gpGlobals->time + 2.5;
				pBot->SetBehaviour(BOT_DONTGOPRONE);



				//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@			NEW CODE 094 (remove it)
#ifdef DEBUG
				char dp[256];
				sprintf(dp, "%s SET DONTGOPRONE behaviour @ Check Stance()|enemydistance < 800 && bot NOT proned\n", pBot->name);
				HudNotify(dp, pBot);
#endif // DEBUG



			}
		}
			
		return;
	}

	// don't change Stance if enemy is still in the same distance
	// do it only in 20% of the time
	if ((pBot->f_prev_enemy_dist == enemy_distance) && (RANDOM_LONG(1, 100) < 80))
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
	if (pBot->IsBehaviour(BOT_PRONED))
	{
		// did it hit something?
		if (FPlayerVisible(v_enemy, pEdict) == VIS_NO)
		{
			// to try the crouched position we have to fake bot's head position
			v_botshead = pEdict->v.origin + pEdict->v.view_ofs + Vector(0, 0, 8);

			// didn't hit anything so change to crouched position
			if (FPlayerVisible(v_enemy, v_botshead, pEdict) == VIS_YES)
			{
				SetStance(pBot, GOTO_CROUCH, "Check Stance()|PRONED -> foe visible in crouch");
				return;
			}
			else
			{
				// try standing position
				v_botshead  = pEdict->v.origin + pEdict->v.view_ofs + Vector(0, 0, 42);

				// change to standing position
				if (FPlayerVisible(v_enemy, v_botshead, pEdict) == VIS_YES)
				{
					SetStance(pBot, GOTO_STANDING, "Check Stance()|PRONED -> foe visible in standing");
					return;
				}


				// NOTE: here should be some code that test strafe to side
			}
		}

		return;
	}

	// is bot crouched
	else if (pBot->IsBehaviour(BOT_CROUCHED))
	{
		// try proned position from time to time
		if (!pBot->IsSubTask(ST_CANTPRONE) && !pBot->IsBehaviour(BOT_DONTGOPRONE)
			&& (RANDOM_LONG(1, 100) < 15))
		{
			v_botshead = pEdict->v.origin + pEdict->v.view_ofs - Vector(0, 0, 8);

			// change to proned position
			if (FPlayerVisible(v_enemy, v_botshead, pEdict) == VIS_YES)
			{
				SetStance(pBot, GOTO_PRONE, "Check Stance()|CROUCHED -> foe visible in prone");
				return;
			}
		}
		// or try standing position from time to time
		else if (RANDOM_LONG(1, 100) < 20)//									was 25
		{
			v_botshead = pEdict->v.origin + pEdict->v.view_ofs + Vector(0, 0, 34);

			// change to standing position
			if (FPlayerVisible(v_enemy, v_botshead, pEdict) == VIS_YES)
			{
				SetStance(pBot, GOTO_STANDING, "Check Stance()|CROUCHED -> foe visible in standing");
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
				SetStance(pBot, GOTO_STANDING, "Check Stance()|CROUCHED -> foe visible in standing");
				return;
			}


			// NOTE: here should be some code that test strafe to side
		}
		return;
	}

	// or bot must stand still AND NOT going to/resume from prone
	else if (pBot->IsBehaviour(BOT_STANDING) && !pBot->IsGoingProne())
	{
		// try proned position
		v_botshead = pEdict->v.origin + pEdict->v.view_ofs - Vector(0, 0, 42);

		// change to proned position
		if ((FPlayerVisible(v_enemy, v_botshead, pEdict) == VIS_YES) &&
			!pBot->IsSubTask(ST_CANTPRONE) && !pBot->IsBehaviour(BOT_DONTGOPRONE))
		{
			SetStance(pBot, GOTO_PRONE, "Check Stance()|STANDING -> foe visible in prone");
			return;
		}

		// try crouched position
		v_botshead = pEdict->v.origin + pEdict->v.view_ofs - Vector(0, 0, 34);

		// change to crouched position
		if (FPlayerVisible(v_enemy, v_botshead, pEdict) == VIS_YES)
		{
			SetStance(pBot, GOTO_CROUCH, "Check Stance()|STANDING -> foe visible in crouch");
			return;
		}
		return;
	}
	else
	{
#ifdef DEBUG
		//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@														// NEW CODE 094 (remove it)
		char dm[256];
		sprintf(dm, "%s @ Check Stance() -> must be changing stance right now (go/resume prone) so skipping it\n",
			pBot->name);
		HudNotify(dm, true, pBot);
		//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#endif // DEBUG
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
			(pBot->IsTask(TASK_AVOID_ENEMY) && (enemy_distance <= pBot->GetEffectiveRange())))
			return TRUE;
	}
	// machineguns
	else if (UTIL_IsMachinegun(pBot->current_weapon.iId))
	{
		if (enemy_distance < 600 ||
			(pBot->IsTask(TASK_AVOID_ENEMY) && (enemy_distance <= pBot->GetEffectiveRange())))
			return TRUE;
	}
	// SMGs
	else if (UTIL_IsSMG(pBot->current_weapon.iId))
	{
		if (enemy_distance < 300)
		{
			// set full auto if not already set
			if (pBot->current_weapon.iFireMode != FM_AUTO)
				UTIL_ChangeFireMode(pBot, FM_AUTO, FireMode_WTest::dont_test_weapons);

			// if bot uses m11 allow akimbo fire when the bot is this close
			if (pBot->current_weapon.iId == fa_weapon_m11)
				pBot->SetWeaponStatus(WS_USEAKIMBO);

			return TRUE;
		}
		else if (enemy_distance < 400)
		{
			// set full auto if not already set
			if (pBot->current_weapon.iFireMode != FM_AUTO)
				UTIL_ChangeFireMode(pBot, FM_AUTO, FireMode_WTest::dont_test_weapons);
		}
		// does bot uses mp5 and enemy isn't that far so change to burst
		else if ((enemy_distance < 550) && (pBot->current_weapon.iId == fa_weapon_mp5a5))
		{
			// set full auto if not already set
			if (pBot->current_weapon.iFireMode != FM_BURST)
				UTIL_ChangeFireMode(pBot, FM_BURST, FireMode_WTest::dont_test_weapons);
		}
		// enemy is quite far use single shot
		else
		{
			if (pBot->current_weapon.iFireMode != FM_SEMI)
				UTIL_ChangeFireMode(pBot, FM_SEMI, FireMode_WTest::dont_test_weapons);

			if (pBot->IsWeaponStatus(WS_USEAKIMBO))
				pBot->RemoveWeaponStatus(WS_USEAKIMBO);
		}

		// will ensure that the bot won't move during combat in this case
		if (pBot->IsTask(TASK_AVOID_ENEMY) && (enemy_distance <= pBot->GetEffectiveRange()))
			return TRUE;
	}
	// handguns
	else if (UTIL_IsHandgun(pBot->current_weapon.iId))
	{
		if (enemy_distance < 150)
		{
			// set 3r burst if not already set
			if (pBot->current_weapon.iFireMode != FM_BURST)
				UTIL_ChangeFireMode(pBot, FM_BURST, FireMode_WTest::test_weapons);

			return TRUE;
		}
		// does bot uses ber93r and enemy isn't that far so change to burst
		else if ((enemy_distance < 200) && (pBot->current_weapon.iId == fa_weapon_ber93r))
		{
			// set 3r burst if not already set
			if (pBot->current_weapon.iFireMode != FM_BURST)
				UTIL_ChangeFireMode(pBot, FM_BURST, FireMode_WTest::dont_test_weapons);
		}
		// enemy is quite far use single shot
		else
		{
			if (pBot->current_weapon.iFireMode != FM_SEMI)
				UTIL_ChangeFireMode(pBot, FM_SEMI, FireMode_WTest::dont_test_weapons);
		}

		if (pBot->IsTask(TASK_AVOID_ENEMY) && (enemy_distance <= pBot->GetEffectiveRange()))
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
		if (enemy_distance < 550 || (pBot->IsTask(TASK_AVOID_ENEMY) && (enemy_distance <= pBot->GetEffectiveRange())))
			return TRUE;
	}
	// shotguns
	else if (UTIL_IsShotgun(pBot->current_weapon.iId))
	{
		if (enemy_distance < 250 || (pBot->IsTask(TASK_AVOID_ENEMY) && (enemy_distance <= pBot->GetEffectiveRange())))
			return TRUE;
	}
	// grenades
	else if (UTIL_IsGrenade(pBot->current_weapon.iId))
	{
		if (enemy_distance < 1400 || (pBot->IsTask(TASK_AVOID_ENEMY) && (enemy_distance <= pBot->GetEffectiveRange())))
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
					UTIL_ChangeFireMode(pBot, FM_BURST, FireMode_WTest::dont_test_weapons);
			}
			else
			{
				// set full auto if not already set
				if (pBot->current_weapon.iFireMode != FM_AUTO)
					UTIL_ChangeFireMode(pBot, FM_AUTO, FireMode_WTest::test_weapons);
			}

			return TRUE;
		}
		// enemy isn't that far so change to full auto
		else if (enemy_distance < 450)
		{
			// set full auto if not already set
			if (pBot->current_weapon.iFireMode != FM_AUTO)
				UTIL_ChangeFireMode(pBot, FM_AUTO, FireMode_WTest::test_weapons);
		}
		// enemy isn't that far so change to burst
		else if (enemy_distance < 650)
		{
			// set 3r burst if not already set
			if (pBot->current_weapon.iFireMode != FM_BURST)
				UTIL_ChangeFireMode(pBot, FM_BURST, FireMode_WTest::test_weapons);
		}
		// enemy is quite far use single shot
		else
		{
			if (pBot->current_weapon.iFireMode != FM_SEMI)
				UTIL_ChangeFireMode(pBot, FM_SEMI, FireMode_WTest::test_weapons);
		}

		if (pBot->IsTask(TASK_AVOID_ENEMY) && (enemy_distance <= pBot->GetEffectiveRange()))
			return TRUE;
	}

	return FALSE;
}

/*
* directs all in combat actions and behaviour
*/
void BotShootAtEnemy( bot_t *pBot )
{
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
		pBot->SetDontCheckStuck();

		// is chance to stand up to scan horizon
		// don't do it if going/resume from prone or the weapon isn't ready for action
		if ((pBot->IsGoingProne() == FALSE) && (pBot->weapon_action == W_READY) &&
			(RANDOM_LONG(1, 100) < 2))
		{
			// standing up is based on behaviour and chance
			if ((pBot->IsBehaviour(ATTACKER) && (RANDOM_LONG(1, 100) < 35)) ||
				(pBot->IsBehaviour(DEFENDER) && (RANDOM_LONG(1, 100) < 5)) ||
				(pBot->IsBehaviour(STANDARD) && (RANDOM_LONG(1, 100) < 20)))
			{
				if (pBot->IsBehaviour(BOT_PRONED))
				{
					UTIL_GoProne(pBot, "ShootAtEnemy()|WaitForEnemyTime -> StandUp to Scan Horizion");
					SetStance(pBot, GOTO_STANDING, "ShootAtEnemy()|WaitForEnemyTime -> GOTO standing to Scan Horizon");
				}
				else if (pBot->IsBehaviour(BOT_CROUCHED))
					SetStance(pBot, GOTO_STANDING, "ShootAtEnemy()|WaitForEnemyTime -> GOTO standing to Scan Horizon");
			}
		}

		// we can't shoot at the enemy because we can't see him
		return;
	}

	// FA hides weapon when the player is bandaging so there's no point to continue
	if (pBot->bandage_time >= gpGlobals->time)
	{


		//@@@@@@@@@@@@@22
#ifdef _DEBUG
		if (botdebugger.IsDebugActions() || botdebugger.IsDebugStuck())
		{
			char bmsg[128];
			sprintf(bmsg, "ShootatEnemy -> bandage time >= globT -> Can't fight -> skipping battle (SHOULDN'T HAPPEN)\n");
			HudNotify(bmsg, true, pBot);

			if (pBot->bandage_time == gpGlobals->time)
				HudNotify("ShootatEnemy -> Bandaging ends in this frame!\n", true, pBot);

		}
#endif

		return;
	}

	// don't apply bandages if going to shoot
	pBot->bandage_time = -1.0;

	// the bot is under water but hasn't switched to knife yet
	if ((pEdict->v.waterlevel == 3) && (pBot->forced_usage != USE_KNIFE))
	{
		pBot->UseWeapon(USE_KNIFE);
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
		if (fabs((double)pEdict->v.vuser1.y - pEdict->v.v_angle.y) > bipod_limit)
		{
			pEdict->v.v_angle.y = pEdict->v.vuser1.y;

			// prevents shoot weapon
			out_of_bipod_limit = TRUE;

			//																				NEW CODE 094
			// try to fold it in order to face the enemy
			BotUseBipod(pBot, FALSE, "BotShootAtEnemy()|TaskBipod prevents facing enemy -> FOLD IT");
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

	// if current enemy is quite far AND is right time AND NOT using sniper rifle check for closer one
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
		(pBot->weapon_action == W_READY) &&						// weapon must be ready because the chance to advance is based on it
		(pBot->IsGoingProne() == FALSE) &&						// NOT going to/resume from prone
		(IsEnemyTooClose(pBot, foe_distance) == FALSE) &&		// enemy is still quite far (has to be first because it includes firemode change code and we need to run this code)
		!pBot->IsTask(TASK_BIPOD) &&						// NOT using a bipod
		(pBot->sniping_time < gpGlobals->time))				// is NOT sniping
	{
		if (!pBot->IsTask(TASK_DONTMOVEINCOMBAT) &&				// can move in combat
			!pBot->IsTask(TASK_IGNORE_ENEMY) &&					// NOT ignoring enemy (the bot must already be close enough)
			!pBot->IsTask(TASK_AVOID_ENEMY))					// NOT avoiding enemy (the bot must already be close enough)
			IsChanceToAdvance(pBot);
		// this is a special case (knife attack) when we need to allow the movement even if the
		// tests from above return "dont move", but the bot has to trace the forward
		if (pBot->IsTask(TASK_DEATHFALL))
			IsChanceToAdvance(pBot);
	}

	// is time to trace enemy to set best position/stance
	else if ((pBot->f_check_stance_time < gpGlobals->time) &&
		(pBot->weapon_action == W_READY) &&						// weapon must be ready
		(pBot->f_combat_advance_time < gpGlobals->time) &&		// NOT in advace
		(pBot->hide_time + 5.0 < gpGlobals->time) &&			// NOT in hiding
		!(pBot->IsTask(TASK_USETANK)) &&						// NOT using tank
		!pBot->IsTask(TASK_BIPOD) &&							// NOT using a bipod
		(pBot->f_reload_time < gpGlobals->time))				// NOT reloading
	{
		CheckStance(pBot, foe_distance);

		// NOTE: these delays should be based on bot_skill value
		// FIX ME Fix me FIXME
		float min_delay = 2.5;
		float max_delay = 5.0;
		pBot->f_check_stance_time = gpGlobals->time + RANDOM_FLOAT(min_delay, max_delay);
	}

	// is the bot paused OR doing medical treatment so just don't move and wait
	else if ((pBot->f_pause_time > gpGlobals->time) || (pBot->f_medic_treat_time > gpGlobals->time))
	{
		pBot->move_speed = SPEED_NO;
		pBot->SetDontCheckStuck();

		// if NOT in cover position try to go prone or at least crouch
		if (!pBot->IsBehaviour(BOT_PRONED) && !pBot->IsBehaviour(BOT_CROUCHED) &&
			(pBot->f_stance_changed_time < gpGlobals->time))
		{
			// go prone only if paused
			if ((RANDOM_LONG(1, 100) > 50) && (pBot->f_medic_treat_time < gpGlobals->time))
				SetStance(pBot, GOTO_PRONE, "ShootAtEnemy()|Paused -> GOTO prone");
			else
				SetStance(pBot, GOTO_CROUCH, "ShootAtEnemy()|PausedORHealingTeammate -> GOTO crouch");
		}
	}

	// or is still sniping time?
	else if (pBot->sniping_time > gpGlobals->time)
	{
		// if the enemy is far then try to deploy bipod if NOT already using it AND if is able to use it
		if ((foe_distance > 1000) && !pBot->IsTask(TASK_BIPOD) &&
			!pBot->IsWeaponStatus(WS_CANTBIPOD) && IsBipodWeapon(pBot->current_weapon.iId))
			BotUseBipod(pBot, FALSE, "BotShootAtEnemy()|SnipingTime -> DeployIt");

		pBot->move_speed = SPEED_NO;
		pBot->SetDontCheckStuck();
	}

	// or is still wait time AND NOT dontmove flag AND NOT behind mounted gun AND NOT avoid enemy?
	else if ((pBot->wpt_wait_time > gpGlobals->time) && !pBot->IsTask(TASK_DONTMOVEINCOMBAT) &&
		!pBot->IsTask(TASK_USETANK) && !pBot->IsTask(TASK_AVOID_ENEMY))
	{
		pBot->SetDontCheckStuck();

		// is the bot sniper so hold position
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

	else if ((pBot->crowded_wpt_index != -1) || UTIL_IsSmallRange(pBot->curr_wpt_index))
	{
		// will cause that the bot forgets about distant enemy and
		// will continue in navigation
		pBot->SetTask(TASK_AVOID_ENEMY);
	}

	// or is bot forced to stay at one place (ie don't move)
	else if (pBot->IsTask(TASK_DONTMOVEINCOMBAT))
	{
		bot_weapon_select_t *pSelect = NULL;
		
		pSelect = &bot_weapon_select[0];

		pBot->move_speed = SPEED_NO;
		pBot->SetDontCheckStuck();

		// find the correct weapon in the array
		int select_index = 0;
		while (pSelect[select_index].iId != pBot->current_weapon.iId)
		{
			select_index++;
		}

		// allow changing weapon firemodes
		IsEnemyTooClose(pBot, foe_distance);

		// check if enemy is too far AND bot wasn't in hiding in last 5 seconds
		if ((foe_distance > pSelect[select_index].max_effective_distance) && (pBot->hide_time + 5.0 < gpGlobals->time))
		{
			// if NOT in prone AND NOT crouched AND NOT going to/resume prone
			if (!pBot->IsBehaviour(BOT_PRONED) && !pBot->IsBehaviour(BOT_CROUCHED) && !pBot->IsGoingProne())
			{
				// change to crouched position
				SetStance(pBot, GOTO_CROUCH, "ShootAtEnemy()|GonnaHide -> GOTO crouch");
			}

			// set some time to stay away
			pBot->hide_time = gpGlobals->time + RANDOM_FLOAT(3.0, 10.0);




			// NOTE: Handle SMG's, Shotguns etc, bots stop and die when out of range


			//@@@@@@@@@@@@@@@
			//ALERT(at_console, "DontMove--Out of range (%.3f) (weap %d)\n",
				//pBot->hide_time, pSelect[select_index].iId);



		}
		// is hide time over but is still some time to next hiding then scan horizon for enemies
		else if ((pBot->hide_time < gpGlobals->time) && (pBot->hide_time + 5.0 > gpGlobals->time))
		{
			// change to standing position
			if (!pBot->IsBehaviour(BOT_STANDING) && (RANDOM_LONG(1, 100) <= 50))
				SetStance(pBot, GOTO_STANDING, "ShootAtEnemy()|inHiding -> GOTO standing to scan horizon");
		}
	}

	// otherwise if behind mounted gun 
	else if (pBot->IsTask(TASK_USETANK))
	{
		pBot->move_speed = SPEED_NO;
		pBot->SetDontCheckStuck();

		// set & keep standing position
		if (!pBot->IsBehaviour(BOT_STANDING))
			SetStance(pBot, GOTO_STANDING);
	}

	// is it time to move towards the enemy (moves are based on carried weapon)
	if (pBot->f_combat_advance_time > gpGlobals->time)
	{
		// is the bot close enough to the enemy then don't move
		if (IsEnemyTooClose(pBot, foe_distance))
		{
			pBot->move_speed = SPEED_NO;
			pBot->SetDontCheckStuck();

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
		pBot->SetDontCheckStuck();
	}

	if (pBot->f_strafe_time < gpGlobals->time)
		pBot->f_strafe_direction = 0.0;	// no strafe just for sure

	// is it time to shoot yet
	if ((out_of_bipod_limit == FALSE) && (pBot->f_shoot_time <= gpGlobals->time) &&
		(pBot->f_reload_time < gpGlobals->time) && (pBot->f_reaction_time < gpGlobals->time) &&
		(pBot->f_bipod_time < gpGlobals->time))
	{
		int result = -1;

		// should bot use a grenade
		if (pBot->forced_usage == USE_GRENADE)
		{
			if (pBot->grenade_time >= gpGlobals->time)
			{
				result = BotThrowGrenade(v_enemy, pBot);

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

					pBot->DecideNextWeapon("ShootAtEnemy() -> ThrowGrenade() returned failure");

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

				pBot->DecideNextWeapon("ShootAtEnemy() -> ThrowGrenade() returned success");


#ifdef _DEBUG
				// testing
				if (in_bot_dev_level1)
					ALERT(at_console, "<<GRENADE>>Returned LESS THAN currT (forcedW is %d | gren_act is %d | currT is %.2f)\n",
					pBot->forced_usage, pBot->grenade_action, gpGlobals->time);
#endif
			}
		}
		// see if we can use a grenade...
		// do we have grenades AND NOT used them both AND out of water AND NOT doing any weapon action AND
		// NOT using bipod AND is time to use grenade AND bot is NOT behind mounted gun AND
		// is some time after reload AND enemy is in highest min_dist and lowest max_dist for grenades
		else if ((pBot->grenade_slot != NO_VAL) && (pBot->grenade_action != ALTW_USED) &&
			(pEdict->v.waterlevel != 3) && (pBot->weapon_action == W_READY) && !pBot->IsTask(TASK_BIPOD) &&
			(pBot->grenade_time + (5.0f * (pBot->bot_skill + 1)) < gpGlobals->time) &&
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
				pBot->UseWeapon(USE_GRENADE);

#ifdef _DEBUG
				// testing - it's time for grenades
				if (in_bot_dev_level1)
					HudNotify("GRENADE when main weapon out of ammo\n", pBot);
#endif
			}
			// if have enough ammo for weapons use grenades only in 25% of the time
			else if (chance <= 25)
			{
				if (g_mod_version == FA_28)
					pBot->grenade_time = gpGlobals->time + 5.0;
				else
					pBot->grenade_time = gpGlobals->time + 3.0;

				pBot->UseWeapon(USE_GRENADE);

#ifdef _DEBUG
				//@@@@@@@@@@@@@@@
				if (UTIL_IsSniperRifle(pBot->current_weapon.iId))
				{
					HudNotify("ShootatEnemy -> Grenade use auto Scope off\n", pBot);
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
			!pBot->IsTask(TASK_USETANK) && (pBot->current_weapon.iFireMode == FM_AUTO))
		{
			// reset the full-auto time first
			// we don't know if bot use it again (ie is allowed to use full auto fire)
			pBot->f_fullauto_time = -1.0;

			// based on skill - Fire Rate Delay? [APG]RoboCop[CL]
			float min_delay[BOT_SKILL_LEVELS] = {0.1, 0.2, 0.3, 0.4, 0.5};
			float max_delay[BOT_SKILL_LEVELS] = {0.2, 0.3, 0.6, 0.8, 1.0};

			int skill = pBot->bot_skill;
			bool can_use = FALSE;

			// full auto usage is based on bot skill
			switch (pBot->bot_skill)
			{
				case 0:
					// if enemy is close use full auto often
					if ((foe_distance < 450) && (RANDOM_LONG(1, 100) > 25))
						can_use = TRUE;
					// if enemy is farther, didn't use it in last seconds and is 50% chance
					else if ((foe_distance < 900) &&
						(pBot->f_fullauto_time + 0.75 < gpGlobals->time) &&
						(RANDOM_LONG(1, 100) > 50))
						can_use = TRUE;
					// use full auto only seldom if enemy is far and some time after last usage
					else if ((foe_distance > 900) &&
						(pBot->f_fullauto_time + 1.5 < gpGlobals->time) &&
						(RANDOM_LONG(1, 100) > 95))
						can_use = TRUE;
					break;

				case 1:
					if ((foe_distance < 450) && (RANDOM_LONG(1, 100) > 20))
						can_use = TRUE;
					else if ((foe_distance < 900) &&
						(pBot->f_fullauto_time + 0.65 < gpGlobals->time) &&
						(RANDOM_LONG(1, 100) > 45))
						can_use = TRUE;
					else if ((foe_distance > 900) &&
						(pBot->f_fullauto_time + 1.25 < gpGlobals->time) &&
						(RANDOM_LONG(1, 100) > 90))
						can_use = TRUE;
					break;

				case 2:
					if ((foe_distance < 450) && (RANDOM_LONG(1, 100) > 15))
						can_use = TRUE;
					else if ((foe_distance < 900) &&
						(pBot->f_fullauto_time + 0.5 < gpGlobals->time) &&
						(RANDOM_LONG(1, 100) > 40))
						can_use = TRUE;
					else if ((foe_distance > 900) &&
						(pBot->f_fullauto_time + 1.0 < gpGlobals->time) &&
						(RANDOM_LONG(1, 100) > 85))
						can_use = TRUE;
					break;

				case 3:
					if ((foe_distance < 450) && (RANDOM_LONG(1, 100) > 10))
						can_use = TRUE;
					else if ((foe_distance < 900) &&
						(pBot->f_fullauto_time + 0.4 < gpGlobals->time) &&
						(RANDOM_LONG(1, 100) > 35))
						can_use = TRUE;
					else if ((foe_distance > 900) &&
						(pBot->f_fullauto_time + 0.75 < gpGlobals->time) &&
						(RANDOM_LONG(1, 100) > 75))
						can_use = TRUE;
					break;

				case 4:
					// if enemy is close use full auto very often
					if ((foe_distance < 450) && (RANDOM_LONG(1, 100) > 5))
						can_use = TRUE;
					else if ((foe_distance < 900) &&
						(pBot->f_fullauto_time + 0.25 < gpGlobals->time) &&
						(RANDOM_LONG(1, 100) > 30))
						can_use = TRUE;
					// use full auto quite often if enemy is far
					else if ((foe_distance > 900) &&
						(pBot->f_fullauto_time + 0.5 < gpGlobals->time) &&
						(RANDOM_LONG(1, 100) > 65))
						can_use = TRUE;
					break;
			}

			// if bot can use the full auto fire set its next use time
			if (can_use)
				pBot->f_fullauto_time = gpGlobals->time + RANDOM_FLOAT(min_delay[skill], max_delay[skill]);
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
			(((pBot->forced_usage == USE_MAIN) && (pBot->main_weapon != NO_VAL)) ||
			((pBot->forced_usage == USE_BACKUP) && (pBot->backup_weapon != NO_VAL))))
		{
			result = BotFireWeapon(v_enemy, pBot);

			if (result == RETURN_RELOADING)
			{
				pBot->f_fullauto_time = -1.0;	// clear full-auto fire time
			}
			else if (result == RETURN_NOAMMO)
			{
				pBot->DecideNextWeapon("ShootAtEnemy() -> FireWeapon() returned NO AMMO");

				// stop using bipod
				if (pBot->IsTask(TASK_BIPOD))
					BotUseBipod(pBot, TRUE, "ShootAtEnemy() -> FireWeapon() returned NO AMMO");

				pBot->sniping_time = gpGlobals->time; // clear sniping time
				pBot->f_fullauto_time = -1.0;
			}
			else if (result == RETURN_TOOCLOSE)
			{
				if (pBot->forced_usage == USE_BACKUP)
				{
					// if main weapon exists AND have ammo for it AND NOT one of these
					if ((pBot->main_weapon != NO_VAL) && (pBot->main_no_ammo == FALSE) &&
						((UTIL_IsSniperRifle(pBot->main_weapon) == FALSE) || (pBot->main_weapon != fa_weapon_m79)))
					{
						pBot->UseWeapon(USE_MAIN);
					}
					else
						pBot->UseWeapon(USE_KNIFE);
				}
				else if (UTIL_IsSniperRifle(pBot->current_weapon.iId))
				{
					// have backup weapon AND have enough ammo AND NOT m79
					if ((pBot->backup_weapon != NO_VAL) && (pBot->backup_no_ammo == FALSE) &&
						(pBot->backup_weapon != fa_weapon_m79))
					{
						pBot->UseWeapon(USE_BACKUP);
					}
					else
						pBot->UseWeapon(USE_KNIFE);
				}
				// if using m79 as a main weapon force the bot to use backup if there's some
				else if (pBot->main_weapon == fa_weapon_m79)
				{
					if ((pBot->backup_weapon != NO_VAL) && (pBot->backup_no_ammo == FALSE))
					{
						pBot->UseWeapon(USE_BACKUP);
					}
					else
						pBot->UseWeapon(USE_KNIFE);
				}
				else
					pBot->UseWeapon(USE_KNIFE);

				// stop using bipod
				if (pBot->IsTask(TASK_BIPOD))
					BotUseBipod(pBot, TRUE, "ShootAtEnemy()->FireWeapon returned TOO CLOSE");

				pBot->sniping_time = gpGlobals->time;
			}
			else if (result == RETURN_TOOFAR)
			{
				// is it the time to override advance and allow the bot to move towards enemy?
				if ((pBot->forced_usage == USE_MAIN) && (pBot->f_combat_advance_time < gpGlobals->time) &&
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
						//HudNotify("***HEY Advance overriden (MAIN)\n");
						#endif



					}
				}

				else if (pBot->forced_usage == USE_BACKUP)
				{
					if ((pBot->main_weapon != NO_VAL) && (pBot->main_no_ammo == FALSE))
					{
						pBot->UseWeapon(USE_MAIN);


						//@@@@@@@@@@@@@@@@
						#ifdef _DEBUG
						HudNotify("FireWeapon()->returned TOOFAR ***Going to switch back to main weapon\n");
						#endif


					}
					else
					{
						if ((pBot->f_combat_advance_time < gpGlobals->time) && (RANDOM_LONG(1, 100) <= 45))
						{
							if (pBot->f_overide_advance_time <= gpGlobals->time)
							{
								pBot->f_combat_advance_time = 0.0;

								pBot->f_overide_advance_time = gpGlobals->time +
									RANDOM_FLOAT(2.5, 4.0);



								//@@@@@@@@@@@@@@@@
								#ifdef _DEBUG
								//HudNotify("***HEY Advance overriden (BACKUP)\n");
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
					if (UTIL_IsSniperRifle(pBot->main_weapon) || (pBot->main_weapon == fa_weapon_m79))
					{
						if (foe_distance > 450)
						{
							pBot->UseWeapon(USE_MAIN);
						}
					}
					// otherwise take back main weapon
					else
					{
						pBot->UseWeapon(USE_MAIN);
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
							pBot->UseWeapon(USE_BACKUP);
						}
					}
					// otherwise take back backup weapon
					else
					{
						pBot->UseWeapon(USE_BACKUP);
					}
				}
				else
				{
					if ((pBot->f_combat_advance_time < gpGlobals->time) && (RANDOM_LONG(1, 100) > 35))
					{
						if (pBot->f_overide_advance_time <= gpGlobals->time)
						{
							pBot->f_combat_advance_time = 0.0;
							
							// if the bot is on "limited movement" path then
							// we have to scan the forward direction for danger
							if ((pBot->IsTask(TASK_AVOID_ENEMY) || pBot->IsTask(TASK_IGNORE_ENEMY)) &&
								(foe_distance < RANGE_MELEE))
								pBot->SetTask(TASK_DEATHFALL);

							pBot->f_overide_advance_time = gpGlobals->time + RANDOM_FLOAT(1.0, 3.0);


							//@@@@@@@@@@@@@@@@
							#ifdef _DEBUG
							char msg[256];
							sprintf(msg, "*** %s Advance overriden using KNIFE! ***\n", pBot->name);
							HudNotify(msg);
							#endif



						}
					}
				}
			}
			// selecting it
			else if (result == RETURN_TAKING)
				pBot->UseWeapon(USE_KNIFE);
		}

#ifdef _DEBUG
		if (in_bot_dev_level2)
		{
			//char msg[80];
			//sprintf(msg, "bot skill level=%d (1 best)\n", pBot->bot_skill + 1);
			//HudNotify(msg);
		}
#endif
	}

	//																									NEW CODE 094 (prev code)
	/*/
																these changes should be done only in botthink()
	// is it time to change Stance yet
	if ((pBot->f_stance_changed_time < gpGlobals->time))// && (pBot->f_go_prone_time < gpGlobals->time))<< This is a BUG due to crouching statement below
	{
		bool is_proned = FALSE;

		// should the bot go prone
		if (!(pEdict->v.flags & FL_PRONED) && pBot->IsBehaviour(BOT_PRONED))
		{
			is_proned = TRUE;

			UTIL_GoProne(pBot, "ShootAtEnemy()|Time to change stance -> Bot should go prone");
		}
		
		// get bot to standing position from previous proned position
		else if ((pEdict->v.flags & FL_PRONED) && pBot->IsBehaviour(BOT_STANDING))
		{
			is_proned = TRUE;

			UTIL_GoProne(pBot,"ShootAtEnemy()|Time to change stance -> Bot should stand up");
		}

		// if bot was proned or going prone set some time for next change
		if (is_proned)
			pBot->f_stance_changed_time = gpGlobals->time + RANDOM_FLOAT(4.0, 7.0);		@@@@@@@<<<<<  NEEDS TESTS if going
	}																							to implement it in new
																								stance management or not
	/**/

	// backup the enemy distance value
	pBot->f_prev_enemy_dist = (pBot->pBotEnemy->v.origin - pEdict->v.origin).Length();
}

/*
* directs bot actions to plant a claymore mine
*/
void BotPlantClaymore(bot_t *pBot)
{
	// if the bot doesn't have claymore mine then reset the whole action
	if ((pBot->claymore_slot == NO_VAL) || ((pBot->bot_weapons & (1 << fa_weapon_claymore)) == FALSE))
	{
		pBot->SetSubTask(ST_RESETCLAYMORE);
		return;
	}

	// take claymore if NOT currently in hands
	if ((pBot->f_use_clay_time < gpGlobals->time) && (pBot->clay_action == ALTW_NOTUSED) &&
		(pBot->current_weapon.iId != fa_weapon_claymore))
	{
		// change weapon to claymore mine
		FakeClientCommand(pBot->pEdict, "weapon_claymore", NULL, NULL);

		// set the current action flag
		pBot->clay_action = ALTW_TAKING;

		// set time to finish this process
		pBot->f_use_clay_time = gpGlobals->time + 2.0;

		// update wait time
		pBot->wpt_wait_time = pBot->f_use_clay_time + 1.0;

		if (botdebugger.IsDebugActions())
			HudNotify("Switching weapons to the claymore mine\n", pBot);
	}

	// has the bot already the mine in hands
	else if ((pBot->f_use_clay_time < gpGlobals->time) && (pBot->clay_action == ALTW_TAKING) &&
		(pBot->current_weapon.iId == fa_weapon_claymore))
	{
		// some safety time
		pBot->f_shoot_time = gpGlobals->time + RANDOM_FLOAT(0.3, 0.6);

		// update the action flag
		pBot->clay_action = ALTW_PREPARED;

		if (botdebugger.IsDebugActions())
			HudNotify("Claymore mine is ready to be placed\n", pBot);
	}

	return;
}

/*
* handles merging magazines
*/
void BotMergeClips(bot_t *pBot, const char* loc)
{
	extern bot_weapon_t weapon_defs[MAX_WEAPONS];

	// check for available magazines first, because if the bot didn't have time to merge magazines and
	// got into some firefights where he has done standard empty magazine reloading
	// then he may not have any partly used magazines anymore
	// so if there's less than 2 magazines left then there's no merging possible
	if (pBot->curr_rgAmmo[weapon_defs[pBot->current_weapon.iId].iAmmo1] < 2)
	{
		// and we have to reset the "counters" to prevent getting the bot
		// be stuck in endless try for merging
		pBot->RemoveWeaponStatus(WS_MERGEMAGS1);
		pBot->RemoveWeaponStatus(WS_MERGEMAGS2);


#ifdef DEBUG

		//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@														// NEW CODE 094 (remove it)
		if (loc != NULL)
		{
			char dm[256];
			sprintf(dm, " resetting MERGECLIPS @ %s (MAGS < 2)\n", loc);
			HudNotify(dm, true, pBot);
		}
		//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#endif // DEBUG

		return;
	}



#ifdef DEBUG
	//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@														// NEW CODE 094 (remove it)
	if (loc != NULL)
	{
		char dm[256];
		sprintf(dm, " called MERGECLIPS command @ %s (iclip=%d | availableMags=%d | botclass=%d)\n",
			loc, pBot->current_weapon.iClip, pBot->curr_rgAmmo[weapon_defs[pBot->current_weapon.iId].iAmmo1],
			pBot->bot_class);
		HudNotify(dm, true, pBot);
	}
	//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#endif // DEBUG




	
	pBot->f_dont_look_for_waypoint_time = gpGlobals->time;
	//pBot->f_cant_prone = gpGlobals->time;
	pBot->SetBehaviour(BOT_DONTGOPRONE);




	//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@			NEW CODE 094 (remove it)
#ifdef DEBUG
	char dp[256];
	sprintf(dp, "%s SET DONTGOPRONE behaviour @ BotMergeClips()\n", pBot->name);
	UTIL_DebugInFile(dp);
#endif // DEBUG




	// stop the bot for a while
	pBot->SetPauseTime(1.5);
	// to see the result of merge clips command
	FakeClientCommand(pBot->pEdict, "mergeclips", NULL, NULL);

	return;
}


/*
* handles using weapon bipod, forced_call == false means the bot will call bipod command at random chance
*/
void BotUseBipod(bot_t *pBot, bool forced_call, const char* loc)
{
	// first check if the bot can start the action ...
	if ((pBot->weapon_action == W_READY) && !pBot->IsGoingProne() &&
		(pBot->f_reload_time < gpGlobals->time) && (pBot->bandage_time < gpGlobals->time))
	{
		// see if NOT already handling the bipod AND is either forced to use it or is chance to use it
		if ((pBot->f_bipod_time < gpGlobals->time) && (forced_call || (RANDOM_LONG(1, 100) < 33)))
		{
			// now the bot is free to use the bipod so call the command ...
			FakeClientCommand(pBot->pEdict, "bipod", NULL, NULL);

			// and set correct time to finish this action
			pBot->f_bipod_time = gpGlobals->time + SetBipodHandlingTime(pBot->current_weapon.iId, pBot->IsTask(TASK_BIPOD));

			// we must also clear this bit to allow the test for failure
			pBot->RemoveWeaponStatus(WS_CANTBIPOD);




#ifdef DEBUG
			//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@														// NEW CODE 094 (remove it)
			if (loc != NULL)
			{
				char dm[256];
				sprintf(dm, "%s called BIPOD command @ %s\n", pBot->name, loc);
				HudNotify(dm, true, pBot);
			}
			//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#endif // DEBUG



			return;
		}

		// this is point where we handle bipod command call failure
		// seeing we didn't pass through previous if statement we know that the bot:
		// 1) decided not to deploy or fold bipod
		//    (deploying - bot decided not to use bipod on current enemy so we must lock it)
		//	  (folding - this next statement won't pass either and bot will try again in next game frame)
		// 2) that the engine didn't allow deploying bipod there and
		//    the method is now being called again in next game frame so...
		if (!pBot->IsTask(TASK_BIPOD))
		{
			// set this bit to prevent trying to deploy bipod over and over again
			pBot->SetWeaponStatus(WS_CANTBIPOD);

			// and reset bipod time that is preventing the bot to open fire at his enemy
			pBot->f_bipod_time = gpGlobals->time;



#ifdef DEBUG
			//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@														// NEW CODE 094 (remove it)
			if (loc != NULL)
			{
				char dm[256];
				sprintf(dm, "%s tried BIPOD command @ %s, but FAILED !!!\n", pBot->name, loc);
				HudNotify(dm, true, pBot);
			}
			//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
#endif // DEBUG
		}

	}

	return;
}


/*
* returns correct time value based on given weapon to either deploying its bipod or folding it
*/
float SetBipodHandlingTime(int weapon, bool folding)
{
	// for most of the weapons there is no difference between deploying or folding the bipod
	// so we can return just one value per weapon type...
	if (weapon == fa_weapon_m249)
		return (float) 2.7;

	else if (weapon == fa_weapon_m60)
		return (float) 2.2;

	else if (weapon == fa_weapon_m82)
	{
		// only m82 has folding time a little faster
		if (folding)
			return (float) 2.2;
		else
			return (float) 2.8;
	}

	else if (weapon == fa_weapon_pkm)
		return (float) 3.3;

	else
	{
		char dm[256];
		sprintf(dm, "BUG - SetBipodHandlingTime() was called for weaponID=%d (FAver=%d)\n", weapon, g_mod_version);
		UTIL_DebugInFile(dm);
	}

	return (float) 0.0;
}