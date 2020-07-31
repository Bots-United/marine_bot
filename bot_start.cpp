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
// bot_start.cpp
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
#include "bot_weapons.h"

// for new config system by Andrey Kotrekhov
#pragma warning(disable:4786)
#include "Config.h"
extern Section *conf;

extern int debug_engine;

// bot_start functions prototypes
inline bool isnt(const char *cc1, const char *cc2)	{ return (strcmp(cc1, cc2) != 0); }
void BotFinishWeaponSpawning(bot_t *pBot);
void BotFinishSkillsSpawning(bot_t *pBot);
int BotGenerateSkill(bot_t *pBot);
inline void BotSetBehaviour(bot_t *pBot);
inline void BotSetWeaponStatus(bot_t *pBot);
int SetMainWeapon(const char *bank, const char *slot);
inline int SetBackupWeapon(bot_t *pBot, const char *bank, const char *slot);
inline int SetGrenade(const char *bank, const char *slot);
inline int SetMine(const char *bank, const char *slot);
int NumberOfFASkills(bot_t *pBot);
void ProcessCustomConfig( bot_t *pBot, const char *line_buffer );
bool CanSpawnIt(bot_t *pBot, int iID);
void RearrangeWeapons(bot_t *pBot);
bool RearrangeWeapons(bot_t *pBot, int weapon);
void FA25SpawnBaseArmor(bot_t *pBot, const char *line_buffer);
void FA25SpawnAdditionalArmor(bot_t *pBot, const char *line_buffer);
void FA25SpawnItems(bot_t *pBot, const char *line_buffer);
void FA25SpawnSidearms(bot_t *pBot, const char *line_buffer);
void FA25SpawnShotguns(bot_t *pBot, const char *line_buffer);
void FA25SpawnSMGs(bot_t *pBot, const char *line_buffer);
void FA25SpawnRifles(bot_t *pBot, const char *line_buffer);
void FA25SpawnSnipes(bot_t *pBot, const char *line_buffer);
void FA25SpawnMGuns(bot_t *pBot, const char *line_buffer);
void FA25SpawnGLs(bot_t *pBot, const char *line_buffer);
bool BotSpawnSkills(bot_t *pBot, const char *line_buffer);

/*
* sets some important bot spawn/respawn variables and lead bot through VGUI menus
*/
void bot_t::BotStartGame()
{
	if (mod_id == FIREARMS_DLL)
	{
		// handle MODT selection menu
		if (start_action == MSG_FA_MOTD_WINDOW)
		{
			start_action = MSG_FA_IDLE;  // switch back to idle

			FakeClientCommand(pEdict, "vguimenuoption", "0", NULL);	// just press "okay"

			return;
		}

		// handle team selection menu
		if (start_action == MSG_FA_TEAM_SELECT)
		{
			start_action = MSG_FA_IDLE;  // switch back to idle

			// select the skin the bot wishes to use
			if ((bot_skin < 1) || (bot_skin > 5))
				bot_skin = -1;

			if (bot_skin == -1)
			{
				bot_skin = RANDOM_LONG(1, 5);
			}

			if (bot_skin == 1)
				FakeClientCommand(pEdict, "changeskin", "0", NULL);
			else if (bot_skin == 2)
				FakeClientCommand(pEdict, "changeskin", "1", NULL);
			else if (bot_skin == 3)
				FakeClientCommand(pEdict, "changeskin", "2", NULL);
			else if (bot_skin == 4)
				FakeClientCommand(pEdict, "changeskin", "3", NULL);
			else if (bot_skin == 5)
				FakeClientCommand(pEdict, "changeskin", "4", NULL);

			// was the team NOT specified so generate it
			if ((bot_team != 1) && (bot_team != 2))
				bot_team = -1;

			if (bot_team == -1)
				bot_team = RANDOM_LONG(1, 2);

			// select the team the bot wishes to join
			if (bot_team == 1)
			{
				// handle Firearms 2.9 and 3.0
				if ((g_mod_version == FA_29) || (g_mod_version == FA_30))
					FakeClientCommand(pEdict, "vguimenuoption", "1", NULL);	// join red team
				else
					FakeClientCommand(pEdict, "vguimenuoption", "2", NULL);
			}
			else
			{
				if ((g_mod_version == FA_29) || (g_mod_version == FA_30))
					FakeClientCommand(pEdict, "vguimenuoption", "2", NULL);	// join blue team
				else
					FakeClientCommand(pEdict, "vguimenuoption", "3", NULL);
			}

			return;
		}

		// handle class selection menu
		if (start_action == MSG_FA_CLASS_SELECT)
		{
			start_action = MSG_FA_IDLE;  // switch back to idle

			// force custom classes in FA25 and below
			if (UTIL_IsOldVersion())
			{
				externals.SetCustomClasses(TRUE);
			}

			// do we use custom classes
			if (externals.GetCustomClasses())
			{
				// SECTION - for new config system by Andrey Kotrekhov
				// ADDED - debug also for Listen server and system debug by Frank McNeil
				char temp[64];

				if (conf != NULL)
				{
					//process custom config kota@
					SI custom_section_i = conf->sectionList.find("custom_config");
#ifdef _DEBUG
					//@@@@@@@
					//PrintOutput(NULL, "*** BotStartGame() - find custom_config\n", msg_null);
#endif
					if (custom_section_i != conf->sectionList.end())
					{
						Section *cust_sect_p = custom_section_i->second;
						
						// to deal with compiler warning signed/unsigned mismatch
						int max_classes = cust_sect_p->sectionList.size();
						
						// check for invalid class value ...
						if ((bot_class < 1) || (bot_class > max_classes))
						{
							// and generate valid class
							bot_class = RANDOM_LONG(1, max_classes);
						}

						// convert bot class to string number
						sprintf(temp, "class%d", bot_class);

						FakeClientCommand(pEdict, "usingpreconfig", NULL, NULL);

						SI class_sect_i = cust_sect_p->sectionList.find(temp);
						if (class_sect_i != cust_sect_p->sectionList.end())
						{
#ifdef _DEBUG
							//@@@@@@@@@@@@
							//char dmsg[256];
							//sprintf(dmsg, "*** BotStartGame() - class %s found\n", temp);
							//PrintOutput(NULL, dmsg, msg_null);
#endif
							//class is found
							Section *class_p = class_sect_i->second;

							pcust_class = class_p;

							// if is it FA25 and below then just enter the menu based spawning
							if (UTIL_IsOldVersion())
							{
#ifdef _DEBUG
								//@@@@@@@@@@@@@
								//PrintOutput(NULL, "BotStartGame() - FA25 menu based spawning entered\n", msg_null);
#endif
								FakeClientCommand(pEdict, "vguimenuoption", "1", NULL);

								return;
							}

							for (II item_i = class_p->item.begin(); item_i != class_p->item.end(); ++item_i)
							{
#ifdef _DEBUG
								//@@@@@@@@@@@@@@@@
								char dmsg[256];
								sprintf(dmsg, "*** %s ", item_i->second.c_str());
								PrintOutput(NULL, dmsg, MType::msg_null);
#endif
								ProcessCustomConfig(this, item_i->second.c_str());
							}
#ifdef _DEBUG
							//@@@@@
							PrintOutput(NULL, "***\n", MType::msg_null);
#endif
						}
					}
				}
				// SECTION BY Andrey Kotrekhov END
			}// END custom classes

			// otherwise use hard coded classes
			// default FA2.7 classes (fixed for FA2.7 credit system, because FA2.7 classes
			// aren't compatible with FA2.7 credit system - it's a bug in FA2.7)
			else
			{
				if ((bot_class < 1) || (bot_class > 8))
					bot_class = -1;

				if (bot_class == -1)
					bot_class = RANDOM_LONG(1, 8);

				// select the class the bot wishes to use
				if (bot_class == 1)
				{
					//this will spawn assault
					FakeClientCommand(pEdict, "usingpreconfig", NULL, NULL);
					FakeClientCommand(pEdict, "loadpreconfig", "1", "2");
					FakeClientCommand(pEdict, "loadpreconfig", "2", "2");
					FakeClientCommand(pEdict, "loadpreconfig", "7", "3");
					FakeClientCommand(pEdict, "loadpreconfig", "4", "3");
					FakeClientCommand(pEdict, "loadpreconfig", "3", "2");
					// if we are using skill system spawn this skill
					if (internals.GetFASkillSystem() != 0.0)
					{
						FakeClientCommand(pEdict, "loadpreconfig", "23", "1");
						// also set this byte
						bot_fa_skill |= MARKS1;
					}
#ifndef NOFAMAS
					main_weapon = fa_weapon_famas;
#endif
					backup_weapon = fa_weapon_anaconda;
					grenade_slot = fa_weapon_frag;
				}
				else if (bot_class == 2)
				{
					//this will spawn machinegunner
					FakeClientCommand(pEdict, "usingpreconfig", NULL, NULL);
					FakeClientCommand(pEdict, "loadpreconfig", "1", "2");
					FakeClientCommand(pEdict, "loadpreconfig", "2", "2");
					FakeClientCommand(pEdict, "loadpreconfig", "9", "2");
					FakeClientCommand(pEdict, "loadpreconfig", "4", "2");
					if (internals.GetFASkillSystem() != 0.0)
					{
						FakeClientCommand(pEdict, "loadpreconfig", "23", "1");
						bot_fa_skill |= MARKS1;
						// spawn this skill only if allowed (ie more skills to start)
						if ((internals.GetFASkillSystem() == 2.0) || (internals.GetFASkillSystem() == 3.0))
						{
							FakeClientCommand(pEdict, "loadpreconfig", "23", "2");
							bot_fa_skill |= MARKS2;
						}
					}

					main_weapon = fa_weapon_m60;
					backup_weapon = fa_weapon_coltgov;
				}
				else if (bot_class == 3)
				{
					//this will spawn grenadier
					FakeClientCommand(pEdict, "usingpreconfig", NULL, NULL);
					FakeClientCommand(pEdict, "loadpreconfig", "1", "2");
					FakeClientCommand(pEdict, "loadpreconfig", "2", "2");
					FakeClientCommand(pEdict, "loadpreconfig", "2", "3");
					FakeClientCommand(pEdict, "loadpreconfig", "7", "6");
					FakeClientCommand(pEdict, "loadpreconfig", "4", "5");
					FakeClientCommand(pEdict, "loadpreconfig", "3", "2");
					FakeClientCommand(pEdict, "loadpreconfig", "3", "6");
					if (internals.GetFASkillSystem() != 0.0)
					{
						FakeClientCommand(pEdict, "loadpreconfig", "23", "8");
						bot_fa_skill |= ARTY1;
					}

					main_weapon = fa_weapon_m16;
					backup_weapon = fa_weapon_ber92f;
					grenade_slot = fa_weapon_frag;
				}
				else if (bot_class == 4)
				{
					//this will spawn specialist
					FakeClientCommand(pEdict, "usingpreconfig", NULL, NULL);
					FakeClientCommand(pEdict, "loadpreconfig", "1", "1");
					FakeClientCommand(pEdict, "loadpreconfig", "2", "2");
					FakeClientCommand(pEdict, "loadpreconfig", "2", "4");
					FakeClientCommand(pEdict, "loadpreconfig", "2", "3");
					FakeClientCommand(pEdict, "loadpreconfig", "6", "3");
					FakeClientCommand(pEdict, "loadpreconfig", "4", "5");
					FakeClientCommand(pEdict, "loadpreconfig", "3", "2");
					FakeClientCommand(pEdict, "loadpreconfig", "3", "5");
					FakeClientCommand(pEdict, "loadpreconfig", "3", "6");
					if (internals.GetFASkillSystem() != 0.0)
					{
						FakeClientCommand(pEdict, "loadpreconfig", "23", "1");
						bot_fa_skill |= MARKS1;
						if ((internals.GetFASkillSystem() == 2.0) || (internals.GetFASkillSystem() == 3.0))
						{
							FakeClientCommand(pEdict, "loadpreconfig", "23", "2");
							bot_fa_skill |= MARKS2;
						}
					}

					main_weapon = fa_weapon_mp5a5;
					backup_weapon = fa_weapon_ber92f;
					grenade_slot = fa_weapon_frag;
					claymore_slot = fa_weapon_claymore;
				}
				else if (bot_class == 5)
				{
					//this will spawn combat sniper
					FakeClientCommand(pEdict, "usingpreconfig", NULL, NULL);
					FakeClientCommand(pEdict, "loadpreconfig", "1", "1");
					FakeClientCommand(pEdict, "loadpreconfig", "2", "3");
					FakeClientCommand(pEdict, "loadpreconfig", "8", "2");
					FakeClientCommand(pEdict, "loadpreconfig", "4", "3");
					FakeClientCommand(pEdict, "loadpreconfig", "3", "5");
					FakeClientCommand(pEdict, "loadpreconfig", "3", "6");
					if (internals.GetFASkillSystem() != 0.0)
					{
						FakeClientCommand(pEdict, "loadpreconfig", "23", "1");
						bot_fa_skill |= MARKS1;
						if ((internals.GetFASkillSystem() == 2.0) || (internals.GetFASkillSystem() == 3.0))
						{
							FakeClientCommand(pEdict, "loadpreconfig", "23", "2");
							bot_fa_skill |= MARKS2;
							FakeClientCommand(pEdict, "loadpreconfig", "23", "10");
							bot_fa_skill |= STEALTH;
						}
					}

					main_weapon = fa_weapon_ssg3000;
					backup_weapon = fa_weapon_anaconda;
					claymore_slot = fa_weapon_claymore;
				}
				else if (bot_class == 6)
				{
					//this will spawn medic
					FakeClientCommand(pEdict, "usingpreconfig", NULL, NULL);
					FakeClientCommand(pEdict, "loadpreconfig", "1", "3");
					FakeClientCommand(pEdict, "loadpreconfig", "2", "2");
					FakeClientCommand(pEdict, "loadpreconfig", "2", "3");
					FakeClientCommand(pEdict, "loadpreconfig", "2", "4");
					FakeClientCommand(pEdict, "loadpreconfig", "5", "2");
					FakeClientCommand(pEdict, "loadpreconfig", "4", "3");
					FakeClientCommand(pEdict, "loadpreconfig", "3", "6");
					if (internals.GetFASkillSystem() != 0.0)
					{
						FakeClientCommand(pEdict, "loadpreconfig", "23", "6");
						bot_fa_skill |= FAID;
						if ((internals.GetFASkillSystem() == 2.0) || (internals.GetFASkillSystem() == 3.0))
						{
							FakeClientCommand(pEdict, "loadpreconfig", "23", "7");
							bot_fa_skill |= MEDIC;
							FakeClientCommand(pEdict, "loadpreconfig", "23", "1");
							bot_fa_skill |= MARKS1;
						}
					}

					main_weapon = fa_weapon_benelli;
					backup_weapon = fa_weapon_anaconda;
				}
				else if (bot_class == 7)
				{
					//this will spawn scout
					FakeClientCommand(pEdict, "usingpreconfig", NULL, NULL);
					FakeClientCommand(pEdict, "loadpreconfig", "1", "2");
					FakeClientCommand(pEdict, "loadpreconfig", "2", "2");
					FakeClientCommand(pEdict, "loadpreconfig", "2", "3");
					FakeClientCommand(pEdict, "loadpreconfig", "7", "5");
					FakeClientCommand(pEdict, "loadpreconfig", "4", "6");
					FakeClientCommand(pEdict, "loadpreconfig", "3", "3");
					FakeClientCommand(pEdict, "loadpreconfig", "3", "6");
					if (internals.GetFASkillSystem() != 0.0)
					{
						FakeClientCommand(pEdict, "loadpreconfig", "23", "8");
						bot_fa_skill |= ARTY1;
					}

					main_weapon = fa_weapon_g36e;
					backup_weapon = fa_weapon_desert;
					grenade_slot = fa_weapon_concussion;
				}
				else
				{
					//this will spawn veteran
					FakeClientCommand(pEdict, "usingpreconfig", NULL, NULL);
					FakeClientCommand(pEdict, "loadpreconfig", "1", "3");
					FakeClientCommand(pEdict, "loadpreconfig", "2", "2");
					FakeClientCommand(pEdict, "loadpreconfig", "7", "4");
					FakeClientCommand(pEdict, "loadpreconfig", "4", "2");
					if (internals.GetFASkillSystem() != 0.0)
					{
						FakeClientCommand(pEdict, "loadpreconfig", "23", "1");
						bot_fa_skill |= MARKS1;
					}

					main_weapon = fa_weapon_g3a3;
					backup_weapon = fa_weapon_coltgov;
				}
			}// END hard coded classes

			// init weapon & stuff settings
			BotFinishWeaponSpawning(this);

			// we need to check skills and spawn all posible skills based on game skill settings
			BotFinishSkillsSpawning(this);

			// finish spawning
			FakeClientCommand(pEdict, "finishedpreconfig", NULL, NULL);

			// bot has now joined the game (doesn't need to be started)
			not_started = 0;

#ifdef _DEBUG
			//@@@@@@@@@@@@@@@@
			char dmsg[256];
			sprintf(dmsg, "Bot reached end of class selection menu (mainW=%d backupW=%d)\n", main_weapon, backup_weapon);
			PrintOutput(NULL, dmsg, MType::msg_null);
#endif

			return;
		}

		// handle FA25 base armor menu (ie. light, medium etc.)
		// we don't need to confirm this menu
		if (start_action == MSG_FA25_BASE_ARMOR_SELECT)
		{
			start_action = MSG_FA_IDLE;

			// use the custom class pointer only if exists
			if (pcust_class != NULL)
			{
				// init class pointer
				Section *class_p = pcust_class;
				
				// go through whole class
				for (II item_i = class_p->item.begin(); item_i != class_p->item.end(); ++item_i)
				{
					FA25SpawnBaseArmor(this, item_i->second.c_str());
				}
			}
			// otherwise we need to spawn some armor so why not to take the first one
			else
				FakeClientCommand(pEdict, "vguimenuoption", "1", NULL);

			return;
		}

		// handle FA25 additional armor menu (ie. helmet etc.)
		if (start_action == MSG_FA25_ARMOR_SELECT)
		{
			start_action = MSG_FA_IDLE;

			if (pcust_class != NULL)
			{
				Section *class_p = pcust_class;
				
				for (II item_i = class_p->item.begin(); item_i != class_p->item.end(); ++item_i)
				{
					FA25SpawnAdditionalArmor(this, item_i->second.c_str());
				}
			}

			// we have to confirm this menu to continue to next section
			FakeClientCommand(pEdict, "vguimenuoption", "1", NULL);

			return;
		}

		// handle FA25 items menu (ie. grenades etc.)
		if (start_action == MSG_FA25_ITEMS_SELECT)
		{
			start_action = MSG_FA_IDLE;

			if (pcust_class != NULL)
			{
				Section *class_p = pcust_class;
				
				for (II item_i = class_p->item.begin(); item_i != class_p->item.end(); ++item_i)
				{
					FA25SpawnItems(this, item_i->second.c_str());
				}
			}

			FakeClientCommand(pEdict, "vguimenuoption", "1", NULL);

			return;
		}

		// handle FA25 sidearms menu
		if (start_action == MSG_FA25_SIDEARMS_SELECT)
		{
			start_action = MSG_FA_IDLE;

			if (pcust_class != NULL)
			{
				Section *class_p = pcust_class;
				
				for (II item_i = class_p->item.begin(); item_i != class_p->item.end(); ++item_i)
				{
					FA25SpawnSidearms(this, item_i->second.c_str());
				}
			}

			FakeClientCommand(pEdict, "vguimenuoption", "9", NULL);

			return;
		}

		// handle FA25 shotguns menu
		if (start_action == MSG_FA25_SHOTGUNS_SELECT)
		{
			start_action = MSG_FA_IDLE;

			if (pcust_class != NULL)
			{
				Section *class_p = pcust_class;
				
				for (II item_i = class_p->item.begin(); item_i != class_p->item.end(); ++item_i)
				{
					FA25SpawnShotguns(this, item_i->second.c_str());
				}
			}

			FakeClientCommand(pEdict, "vguimenuoption", "9", NULL);

			return;
		}

		// handle FA25 submachineguns menu
		if (start_action == MSG_FA25_SMGS_SELECT)
		{
			start_action = MSG_FA_IDLE;

			if (pcust_class != NULL)
			{
				Section *class_p = pcust_class;
				
				for (II item_i = class_p->item.begin(); item_i != class_p->item.end(); ++item_i)
				{
					FA25SpawnSMGs(this, item_i->second.c_str());
				}
			}

			FakeClientCommand(pEdict, "vguimenuoption", "9", NULL);

			return;
		}

		// handle FA25 rifles menu
		if (start_action == MSG_FA25_RIFLES_SELECT)
		{
			start_action = MSG_FA_IDLE;

			if (pcust_class != NULL)
			{
				Section *class_p = pcust_class;
				
				for (II item_i = class_p->item.begin(); item_i != class_p->item.end(); ++item_i)
				{
					FA25SpawnRifles(this, item_i->second.c_str());
				}
			}

			FakeClientCommand(pEdict, "vguimenuoption", "9", NULL);

			return;
		}

		// handle FA25 sniper rifles menu
		if (start_action == MSG_FA25_SNIPES_SELECT)
		{
			start_action = MSG_FA_IDLE;

			if (pcust_class != NULL)
			{
				Section *class_p = pcust_class;
				
				for (II item_i = class_p->item.begin(); item_i != class_p->item.end(); ++item_i)
				{
					FA25SpawnSnipes(this, item_i->second.c_str());
				}
			}

			FakeClientCommand(pEdict, "vguimenuoption", "9", NULL);

			return;
		}

		// handle FA25 machineguns menu
		if (start_action == MSG_FA25_MGUNS_SELECT)
		{
			start_action = MSG_FA_IDLE;

			if (pcust_class != NULL)
			{
				Section *class_p = pcust_class;
				
				for (II item_i = class_p->item.begin(); item_i != class_p->item.end(); ++item_i)
				{
					FA25SpawnMGuns(this, item_i->second.c_str());
				}
			}

			FakeClientCommand(pEdict, "vguimenuoption", "9", NULL);

			return;
		}

		// handle FA25 grenade launchers menu
		if (start_action == MSG_FA25_GLS_SELECT)
		{
			start_action = MSG_FA_IDLE;

			if (pcust_class != NULL)
			{
				Section *class_p = pcust_class;
				
				for (II item_i = class_p->item.begin(); item_i != class_p->item.end(); ++item_i)
				{
					FA25SpawnGLs(this, item_i->second.c_str());
				}
			}

			FakeClientCommand(pEdict, "vguimenuoption", "1", NULL);

			return;
		}

		// handle new skill select
		if (start_action == MSG_FA_SKILL_SELECT)
		{
			bool skill_spawned = FALSE;

			start_action = MSG_FA_IDLE;

			// if the pointer to custom class exist use it
			// otherwise the code logic will spawn one skill randomly
			if (pcust_class != NULL)
			{
				Section *class_p = pcust_class;
				
				for (II item_i = class_p->item.begin(); item_i != class_p->item.end(); ++item_i)
				{
					skill_spawned = BotSpawnSkills(this, item_i->second.c_str());
				}
			}

			// there were no or not enough skills in .cfg for this class
			if (skill_spawned == FALSE)
			{
				// generate some skills randomly
				char the_skill[4];
				sprintf(the_skill, "%d", BotGenerateSkill(this) -1);
				
				FakeClientCommand(pEdict, "setskill", the_skill, NULL);

#ifdef _DEBUG
				//@@@@@@@@@@@@@@@
				char dmsg[126];
				sprintf(dmsg, "***no skills in .cfg file -> generating skill no. %s\n", the_skill);
				PrintOutput(NULL, dmsg, MType::msg_null);
#endif
			}

			return;
		}

		// in newer FA versions and use of custom classes this menu isn't called, because
		// the bot enters the game at the end of class selection menu, just like a real player who
		// used one of the predefined classes
		if (start_action == MSG_FA_INTO_BATTLE)
		{
			start_action = MSG_FA_IDLE;	// switch back to idle

			// arrange weapons first ie. try to use main weapon slot all the time
			RearrangeWeapons(this);

			// init weapon based settings
			BotFinishWeaponSpawning(this);

			// with system 3.0 we automatically get all skills without any skill menu
			// so we have to handle that here
			if (internals.GetFASkillSystem() == 3.0)
			{
				// just set all the bytes to ensure correct behaviour
				bot_fa_skill |= MARKS1;
				bot_fa_skill |= MARKS2;
				bot_fa_skill |= NOMEN;
				bot_fa_skill |= BATTAG;
				bot_fa_skill |= LEADER;
				bot_fa_skill |= FAID;
				bot_fa_skill |= MEDIC;
				bot_fa_skill |= ARTY1;
				bot_fa_skill |= ARTY2;
				bot_fa_skill |= STEALTH;
			}

			FakeClientCommand(pEdict, "vguimenuoption", "1", NULL);

			// bot has now joined the game
			not_started = 0;

#ifdef _DEBUG
			char dmsg[256];
			sprintf(dmsg, "***FA join the battle menu confirmed -> now the bot enters the game (mainW=%d backupW=%d)\n",
				main_weapon, backup_weapon);
			PrintOutput(NULL, dmsg, MType::msg_null);
#endif
			return;
		}
	}
	else
	{
		// otherwise, don't need to do anything to start game
		not_started = 0;
	}
}

/*
* inits weapon slots and calls other methods to finalize the spawn
*/
void BotFinishWeaponSpawning(bot_t *pBot)
{
#ifdef _DEBUG
	ALERT(at_console, "BotFinishWeaponSpawning() called\n");
#endif

	// have both weapons (main and backup) so use main
	if ((pBot->main_weapon != NO_VAL) && (pBot->backup_weapon != NO_VAL))
	{
		pBot->UseWeapon(USE_MAIN);
		pBot->main_no_ammo = FALSE;
		pBot->backup_no_ammo = FALSE;
	}
	// have main weapon AND no backup weapon so use main
	else if ((pBot->main_weapon != NO_VAL) && (pBot->backup_weapon == NO_VAL))
	{
		pBot->UseWeapon(USE_MAIN);
		pBot->main_no_ammo = FALSE;
		pBot->backup_no_ammo = TRUE;
	}
	// if no main weapon AND already have backup weapon so use it
	else if ((pBot->main_weapon == NO_VAL) && (pBot->backup_weapon != NO_VAL))
	{
		pBot->UseWeapon(USE_BACKUP);
		pBot->main_no_ammo = TRUE;
		pBot->backup_no_ammo = FALSE;
	}
	// otherwise if both aren't available use knife
	else if ((pBot->main_weapon == NO_VAL) && (pBot->backup_weapon == NO_VAL))
	{
		pBot->UseWeapon(USE_KNIFE);
		pBot->main_no_ammo = TRUE;
		pBot->backup_no_ammo = TRUE;
	}
	// is bot equipped with grenades use them
	if (pBot->grenade_slot != NO_VAL)
		pBot->grenade_action = ALTW_NOTUSED;
	// otherwise prevent running BotThrowGrenade
	else
		pBot->grenade_action = ALTW_USED;

	// set the behaviour now (bot already have weapon)
	if (pBot->bot_behaviour == 0)
		BotSetBehaviour(pBot);

	// check if the bot has a weapon that supports silencer
	if (pBot->weapon_status == 0)
		BotSetWeaponStatus(pBot);
}

/*
* checks the game skill system and do sets & spawn all skills the bot needs to join the game
*/
void BotFinishSkillsSpawning(bot_t *pBot)
{
#ifdef _DEBUG
	ALERT(at_console, "BotFinishSkillsSpawning() called\n");
#endif

	// spawn one or more skills to finish spawning if bot do NOT have any
	int number_of_skills = NumberOfFASkills(pBot);
	
	if ((pBot->bot_fa_skill == 0) || ((internals.GetFASkillSystem() == 1.0) && (number_of_skills < 1)) ||
		((internals.GetFASkillSystem() == 2.0) && (number_of_skills < 4)) || (internals.GetFASkillSystem() == 3.0))
	{
		// is system without skills so do nothing
		if (internals.GetFASkillSystem() == 0.0)
			return;

		if (internals.GetFASkillSystem() == 1.0)
		{
			char the_skill[4];
			
			// convert skill value to char
			sprintf(the_skill, "%d", BotGenerateSkill(pBot) - 1);
			
			FakeClientCommand(pBot->pEdict, "loadpreconfig", "23", the_skill);
		}
		else if (internals.GetFASkillSystem() == 2.0)
		{
			char the_skill[4];

			// with system 2.0 we can spawn 4 skills
			// so check how many skills bot already have
			int still_need = 4 - number_of_skills;
			
			// and spawn the rest we need
			for (int i = 0; i < still_need; i++)
			{
				sprintf(the_skill, "%d", BotGenerateSkill(pBot));

				FakeClientCommand(pBot->pEdict, "loadpreconfig", "23", the_skill);
			}
		}
		else if (internals.GetFASkillSystem() == 3.0)
		{
			// with system 3.0 we get all skills
			// so just set all these bytes for correct behaviour
			pBot->bot_fa_skill |= MARKS1;
			pBot->bot_fa_skill |= MARKS2;
			pBot->bot_fa_skill |= NOMEN;
			pBot->bot_fa_skill |= BATTAG;
			pBot->bot_fa_skill |= LEADER;
			pBot->bot_fa_skill |= FAID;
			pBot->bot_fa_skill |= MEDIC;
			pBot->bot_fa_skill |= ARTY1;
			pBot->bot_fa_skill |= ARTY2;
			pBot->bot_fa_skill |= STEALTH;
		}
	}
}

/*
* generates and returns new FA skill bot can choose
* checking which of skills bot already has
*/
int BotGenerateSkill(bot_t *pBot)
{
	int new_skill;
	bool spawn_different = TRUE;

#ifdef _DEBUG
	ALERT(at_console, "BotGenerateSkill() called\n");
#endif

	new_skill = RANDOM_LONG(1, 10);

	// do generate different skill until one is valid (i.e. bot doesn't have it yet)
	for(; ;)
	{
		if (((new_skill == 1) && !(pBot->bot_fa_skill & MARKS1)) ||
			((new_skill == 2) && !(pBot->bot_fa_skill & MARKS2)) ||
			((new_skill == 3) && !(pBot->bot_fa_skill & NOMEN)) ||
			((new_skill == 4) && !(pBot->bot_fa_skill & BATTAG)) ||
			((new_skill == 5) && !(pBot->bot_fa_skill & LEADER)) ||
			((new_skill == 6) && !(pBot->bot_fa_skill & FAID)) ||
			((new_skill == 7) && !(pBot->bot_fa_skill & MEDIC)) ||
			((new_skill == 8) && !(pBot->bot_fa_skill & ARTY1)) ||
			((new_skill == 9) && !(pBot->bot_fa_skill & ARTY2)) ||
			((new_skill == 10) && !(pBot->bot_fa_skill & STEALTH)))
		{
			spawn_different = FALSE;
			break;
		}

		new_skill = RANDOM_LONG(1, 10);
	}

	if (spawn_different == FALSE)
	{
		if (new_skill == 1)
			pBot->bot_fa_skill |= MARKS1;
		else if (new_skill == 2)
		{
			// does bot already have marksmanship1
			if (pBot->bot_fa_skill & MARKS1)
				pBot->bot_fa_skill |= MARKS2;
			// otherwise take the marksmanship1 first
			else
			{
				pBot->bot_fa_skill |= MARKS1;
				// lower it to return correct value
				new_skill--;
			}
		}
		else if (new_skill == 3)
			pBot->bot_fa_skill |= NOMEN;
		else if (new_skill == 4)
			pBot->bot_fa_skill |= BATTAG;
		else if (new_skill == 5)
			pBot->bot_fa_skill |= LEADER;
		else if (new_skill == 6)
			pBot->bot_fa_skill |= FAID;
		else if (new_skill == 7)
		{
			if (pBot->bot_fa_skill & FAID)
				pBot->bot_fa_skill |= MEDIC;
			else
			{
				pBot->bot_fa_skill |= FAID;
				new_skill--;
			}
		}
		else if (new_skill == 8)
			pBot->bot_fa_skill |= ARTY1;
		else if (new_skill == 9)
		{
			if (pBot->bot_fa_skill & ARTY1)
				pBot->bot_fa_skill |= ARTY2;
			else
			{
				pBot->bot_fa_skill |= ARTY1;
				new_skill--;
			}
		}
		else if (new_skill == 10)
			pBot->bot_fa_skill |= STEALTH;

#ifdef _DEBUG
		ALERT(at_console, "***bot picked skill no. %d\n", new_skill);
#endif

		return new_skill;
	}

	// log possible error
	char ermsg[126];
	sprintf(ermsg, "***BotGenerateSkill() FATAL ERROR (trying to spawn skill no. %d)\n", new_skill);
	UTIL_DebugInFile(ermsg);
	ALERT(at_console, ermsg);

	return -1;
}

/*
* sets the best behaviour type based on current weapon
*/
inline void BotSetBehaviour(bot_t *pBot)
{
	char behaviour_byte1[32], behaviour_byte2[32];

	// set additional behaviour (for path system)
	// based on version of Fireamrs
	if (UTIL_IsSniperRifle(pBot->main_weapon))
		pBot->bot_behaviour |= SNIPER;

	else if (UTIL_IsMachinegun(pBot->main_weapon))
		pBot->bot_behaviour |= MGUNNER;

	else if (UTIL_IsSMG(pBot->main_weapon) || UTIL_IsShotgun(pBot->main_weapon))
		pBot->bot_behaviour |= CQUARTER;

	else
		pBot->bot_behaviour |= COMMON;

	// set main behaviour
	// SNIPER RIFLES
	if (pBot->bot_behaviour & SNIPER)
	{
		strcpy(behaviour_byte2, "sniper");

		// in 25% of the time baheave normally
		if (RANDOM_LONG(1, 100) > 75)
			pBot->bot_behaviour |= STANDARD;
		// otherwise behave defensively
		else
			pBot->bot_behaviour |= DEFENDER;
	}
	// MACHINEGUNS
	else if (pBot->bot_behaviour & MGUNNER)
	{
		strcpy(behaviour_byte2,"mgunner");

		// in 25% of the time be agessive
		if (RANDOM_LONG(1, 100) > 75)
			pBot->bot_behaviour |= ATTACKER;
		else
		{
			// in another 25% of the time behave normally
			if (RANDOM_LONG(1, 100) > 75)
				pBot->bot_behaviour |= STANDARD;
			// otherwise behave defensively
			else
				pBot->bot_behaviour |= DEFENDER;
		}
	}
	// CLOSE QUARTER WEAPONS
	else if (pBot->bot_behaviour & CQUARTER)
	{
		strcpy(behaviour_byte2,"close_quatrer");

		// in 75% of the time be agessive
		if (RANDOM_LONG(1, 100) > 25)
			pBot->bot_behaviour |= ATTACKER;
		// otherwise behave normally
		else
			pBot->bot_behaviour |= STANDARD;
	}
	// ALL OTHER WEAPONS
	else if (pBot->bot_behaviour & COMMON)
	{
		strcpy(behaviour_byte2,"common_soldier");

		int chance = RANDOM_LONG(1, 100);

		// common soldiers behaves 33 on 33 on 33
		if (chance > 66)
			pBot->bot_behaviour |= ATTACKER;
		else if (chance > 33)
			pBot->bot_behaviour |= STANDARD;
		else
			pBot->bot_behaviour |= DEFENDER;
	}

	if (pBot->bot_behaviour & ATTACKER)
		strcpy(behaviour_byte1,"attacker");
	else if (pBot->bot_behaviour & DEFENDER)
		strcpy(behaviour_byte1,"defender");
	else if (pBot->bot_behaviour & STANDARD)
		strcpy(behaviour_byte1,"standard");
	else
		strcpy(behaviour_byte1,"unknown");

#ifdef _DEBUG
	ALERT(at_console, "***behaviour: %s - <%s>\n", behaviour_byte1, behaviour_byte2);
#endif

	return;
}

/*
* sets correct flag if the bot has a weapon that supports a silencer
*/
inline void BotSetWeaponStatus(bot_t *pBot)
{
	if ((pBot->main_weapon == fa_weapon_m4) || (pBot->main_weapon == fa_weapon_mp5a5) ||
		(pBot->main_weapon == fa_weapon_uzi) || (pBot->backup_weapon == fa_weapon_mp5a5) ||
		(pBot->backup_weapon == fa_weapon_uzi) || (pBot->backup_weapon == fa_weapon_ber92f))
	{
		pBot->SetWeaponStatus(WS_ALLOWSILENCER);
	}

	return;
}

/*
* returns the right weapon id based on bank & slot otherwise return -1
*/
int SetMainWeapon(const char *bank, const char *slot)
{
	int weaponID = -1;

	// SHOTGUNs
	if (FStrEq(bank, "5"))
	{
		if (FStrEq(slot, "2")) { weaponID = fa_weapon_benelli; }
		else if (FStrEq(slot, "3")) { weaponID = fa_weapon_saiga; }
	}
	// SMGs
	else if (FStrEq(bank, "6"))
	{
		if (FStrEq(slot, "2")) { weaponID = fa_weapon_sterling; }
		else if (FStrEq(slot, "3")) { weaponID = fa_weapon_mp5a5; }
		else if (FStrEq(slot, "4")) { weaponID = fa_weapon_uzi; }
		else if (FStrEq(slot, "5")) { weaponID = fa_weapon_bizon; }
	}
	// ASSAULT RIFLEs
	else if (FStrEq(bank, "7"))
	{
		if (FStrEq(slot, "2")) { weaponID = fa_weapon_ak47; }
#ifndef NOFAMAS
		else if (FStrEq(slot, "3")) { weaponID = fa_weapon_famas; }
#endif
		else if (FStrEq(slot, "4")) { weaponID = fa_weapon_g3a3; }
		else if (FStrEq(slot, "5")) { weaponID = fa_weapon_g36e; }
		else if (FStrEq(slot, "6")) { weaponID = fa_weapon_m16; }
		else if (FStrEq(slot, "7")) { weaponID = fa_weapon_ak74; }
		else if (FStrEq(slot, "8"))
		{
			if ((g_mod_version == FA_29) || (g_mod_version == FA_30))
				weaponID = fa_weapon_m14;
		}
		else if (FStrEq(slot, "9"))
		{
			if ((g_mod_version == FA_29) || (g_mod_version == FA_30))
				weaponID = fa_weapon_m4;
		}
	}
	// SNIPER RIFLEs
	else if (FStrEq(bank, "8"))
	{
		if (FStrEq(slot, "2")) { weaponID = fa_weapon_ssg3000; }
		else if (FStrEq(slot, "3")) { weaponID = fa_weapon_m82; }
		else if (FStrEq(slot, "4")) { weaponID = fa_weapon_svd; }
	}
	// MACHINEGUNs
	else if (FStrEq(bank, "9"))
	{
		if (FStrEq(slot, "2")) { weaponID = fa_weapon_m60; }
		else if (FStrEq(slot, "3"))
		{
			if (UTIL_IsNewerVersion() || (g_mod_version == FA_27))
				weaponID = fa_weapon_m249;
			// Firearms 2.6 and 2.65 had pkm under this slot
			else if (g_mod_version == FA_26)
				weaponID = fa_weapon_pkm;
		}
		else if (FStrEq(slot, "4")) { weaponID = fa_weapon_pkm; }
	}
	// GRENADE LAUNCHER
	else if (FStrEq(bank, "10")) { weaponID = fa_weapon_m79; }

	return weaponID;
}

/*
* returns the right weapon id based on bank & slot otherwise return -1
*/
inline int SetBackupWeapon(bot_t *pBot, const char *bank, const char *slot)
{
	int weaponID = -1;

	if (FStrEq(bank, "4"))	// pistols
	{
		if (FStrEq(slot, "2"))
			weaponID = fa_weapon_coltgov;
		else if (FStrEq(slot, "3"))
			weaponID = fa_weapon_anaconda;
		else if (FStrEq(slot, "4"))
			weaponID = fa_weapon_ber93r;
		else if (FStrEq(slot, "5"))
			weaponID = fa_weapon_ber92f;
		else if (FStrEq(slot, "6"))
			weaponID = fa_weapon_desert;
	}
	else if (FStrEq(bank, "5"))	// shotguns already have code so use it
	{
		weaponID = SetMainWeapon(bank, slot);
		// if bot already have this weapon as a main weapon set backup free
		if (weaponID == pBot->main_weapon)
			weaponID = -1;
	}
	else if (FStrEq(bank, "6"))	// smgs
	{
		weaponID = SetMainWeapon(bank, slot);
		
		if (weaponID == pBot->main_weapon)
			weaponID = -1;
	}
	else if (FStrEq(bank, "10"))	// gl
	{
		weaponID = SetMainWeapon(bank, slot);
		
		if (weaponID == pBot->main_weapon)
			weaponID = -1;
	}

	return weaponID;
}

/*
* returns the right grenade id based on bank & slot otherwise return -1
* handles incompatibilities between various versions of the mod
*/
inline int SetGrenade(const char *bank, const char *slot)
{
	int grenadeID = -1;

	if (FStrEq(bank, "3"))
	{
		if (FStrEq(slot, "2")) { grenadeID = fa_weapon_frag; }
		else if (FStrEq(slot, "3")) { grenadeID = fa_weapon_concussion; }
		else if (FStrEq(slot, "4"))
		{
			// Stielhandgrenate was removed in FA28
			if (UTIL_IsNewerVersion() == FALSE)
				grenadeID = fa_weapon_stg24;
		}
	}

	return grenadeID;
}

/*
* returns the right mine id based on bank & slot otherwise return -1
* handles incompatibilities between various versions of the mod
*/
inline int SetMine(const char *bank, const char *slot)
{
	int mineID = -1;

	if (FStrEq(bank, "3"))
	{
		// claymore mine slot depends on mod version
		if (FStrEq(slot, "4") && UTIL_IsNewerVersion())
			mineID = fa_weapon_claymore;
		else if (FStrEq(slot, "5") && (UTIL_IsNewerVersion() == FALSE))
			mineID = fa_weapon_claymore;
	}

	return mineID;
}

/*
* counts bots FA_skills for correct spawn
*/
int NumberOfFASkills(bot_t *pBot)
{
	int the_count = 0;

	if (pBot->bot_fa_skill == 0)
		return 0;

	if (pBot->bot_fa_skill & MARKS1)
		the_count++;

	if (pBot->bot_fa_skill & MARKS2)
		the_count++;

	if (pBot->bot_fa_skill & NOMEN)
		the_count++;

	if (pBot->bot_fa_skill & BATTAG)
		the_count++;

	if (pBot->bot_fa_skill & LEADER)
		the_count++;

	if (pBot->bot_fa_skill & FAID)
		the_count++;

	if (pBot->bot_fa_skill & MEDIC)
		the_count++;

	if (pBot->bot_fa_skill & ARTY1)
		the_count++;

	if (pBot->bot_fa_skill & ARTY2)
		the_count++;

	if (pBot->bot_fa_skill & STEALTH)
		the_count++;

	return the_count;
}

/*
* do check the buffer for the right equipment
* it also handles incompatibilities between mod versions
* reads both MB as well as FA tokens
* this method doesn't handle FA 2.5 due to different spawning system
* changed to accept the new config system by Andrey Kotrekhov
*/
void ProcessCustomConfig(bot_t *pBot, const char *line_buffer)
{
	edict_t *pEdict = pBot->pEdict;
	char bank[4], slot[4];	// FakeClientCmd last two args
	int number_of_skills;

	// init them first
	sprintf(bank,"%d", -1);
	sprintf(slot,"%d", -1);

	if (strcmp(line_buffer, "light_armor") == 0)
	{
		sprintf(bank, "%d", 1);
		sprintf(slot, "%d", 1);
	}
	else if (strcmp(line_buffer, "medium_armor") == 0)
	{
		sprintf(bank, "%d", 1);
		sprintf(slot, "%d", 2);
	}
	else if (strcmp(line_buffer, "heavy_armor") == 0)
	{
		sprintf(bank, "%d", 1);
		sprintf(slot, "%d", 3);
	}
	else if ((strcmp(line_buffer, "armor_helmet") == 0) || (strcmp(line_buffer, "helmet") == 0))
	{
		sprintf(bank, "%d", 2);
		sprintf(slot, "%d", 2);
	}
	else if ((strcmp(line_buffer, "armor_arms") == 0) || (strcmp(line_buffer, "arms") == 0))
	{
		sprintf(bank, "%d", 2);
		sprintf(slot, "%d", 3);
	}
	else if ((strcmp(line_buffer, "armor_legs") == 0) || (strcmp(line_buffer, "legs") == 0))
	{
		sprintf(bank, "%d", 2);
		sprintf(slot, "%d", 4);
	}
	// MISC
	else if ((strcmp(line_buffer, "frag_grenade") == 0) || (strcmp(line_buffer, "frag") == 0))
	{
		sprintf(bank, "%d", 3);
		sprintf(slot, "%d", 2);
	}
	else if ((strcmp(line_buffer, "concussion_grenade") == 0) || (strcmp(line_buffer, "conc") == 0))
	{
		sprintf(bank, "%d", 3);
		sprintf(slot, "%d", 3);
	}
	else if ((strcmp(line_buffer, "stiel_grenade") == 0) || (strcmp(line_buffer, "stg24") == 0))
	{
		// these versions don't support Stielhandgrenate so do NOT spawn it
		if (UTIL_IsNewerVersion())
		{
			return;
		}
		else
		{
			sprintf(bank, "%d", 3);
			sprintf(slot, "%d", 4);
		}
	}
	else if ((strcmp(line_buffer, "claymore_mine") == 0) || (strcmp(line_buffer, "claymore") == 0))
	{
		// steilhandgrenate was removed and nightvisiongoggles were added
		if (UTIL_IsNewerVersion())
		{
			sprintf(bank, "%d", 3);
			sprintf(slot, "%d", 4);
		}
		else
		{
			sprintf(bank, "%d", 3);
			sprintf(slot, "%d", 5);
		}
	}
	else if (strcmp(line_buffer, "bandages") == 0)
	{
		if (UTIL_IsNewerVersion())
		{
			sprintf(bank, "%d", 3);
			sprintf(slot, "%d", 5);
		}
		else
		{
			sprintf(bank, "%d", 3);
			sprintf(slot, "%d", 6);
		}
	}
	else if ((strcmp(line_buffer, "nv_goggles") == 0) || (strcmp(line_buffer, "nvg") == 0))
	{
		if (UTIL_IsNewerVersion())
		{
			sprintf(bank, "%d", 3);
			sprintf(slot, "%d", 6);
		}
		// these versions don't support Night Vision Goggles so do NOT spawn it
		else
		{
			return;
		}
	}
	// WEAPONS - PISTOLS
	else if ((strcmp(line_buffer, "colt_gov") == 0) || (strcmp(line_buffer, "1911a1") == 0))
	{
		sprintf(bank, "%d", 4);
		sprintf(slot, "%d", 2);
	}
	else if ((strcmp(line_buffer, "colt_anaconda") == 0) || (strcmp(line_buffer, "anaconda") == 0))
	{
		sprintf(bank, "%d", 4);
		sprintf(slot, "%d", 3);
	}
	else if ((strcmp(line_buffer, "ber_93r") == 0) || (strcmp(line_buffer, "93r") == 0))
	{
		sprintf(bank, "%d", 4);
		sprintf(slot, "%d", 4);
	}
	else if ((strcmp(line_buffer, "ber_92f") == 0) || (strcmp(line_buffer, "92f") == 0))
	{
		sprintf(bank, "%d", 4);
		sprintf(slot, "%d", 5);
	}
	else if ((strcmp(line_buffer, "desert_eagle") == 0) || (strcmp(line_buffer, "deagle") == 0))
	{
		sprintf(bank, "%d", 4);
		sprintf(slot, "%d", 6);
	}
	// WEAPONS - SHOTGUNS
	else if ((strcmp(line_buffer, "benelli_m3") == 0) || (strcmp(line_buffer, "m3") == 0) ||
		(strcmp(line_buffer, "remington") == 0))
	{
		sprintf(bank, "%d", 5);
		sprintf(slot, "%d", 2);
	}
	else if ((strcmp(line_buffer, "saiga_12k") == 0) || (strcmp(line_buffer, "12k") == 0))
	{
		sprintf(bank, "%d", 5);
		sprintf(slot, "%d", 3);
	}
	// WEAPONS - SMGS
	else if (strcmp(line_buffer, "sterling") == 0)
	{
		sprintf(bank, "%d", 6);
		sprintf(slot, "%d", 2);
	}
	else if ((strcmp(line_buffer, "hk_mp5") == 0) || (strcmp(line_buffer, "mp5") == 0))
	{
		sprintf(bank, "%d", 6);
		sprintf(slot, "%d", 3);
	}
	else if (strcmp(line_buffer, "uzi") == 0)
	{
		sprintf(bank, "%d", 6);
		sprintf(slot, "%d", 4);
	}
	else if (strcmp(line_buffer, "bizon") == 0)
	{
		sprintf(bank, "%d", 6);
		sprintf(slot, "%d", 5);
	}
	// WEAPONS - AR
	else if (strcmp(line_buffer, "ak47") == 0)
	{
		sprintf(bank, "%d", 7);
		sprintf(slot, "%d", 2);
	}
	else if (strcmp(line_buffer, "famas") == 0)
	{
		sprintf(bank, "%d", 7);
		sprintf(slot, "%d", 3);
	}
	else if ((strcmp(line_buffer, "hk_g3a3") == 0) || (strcmp(line_buffer, "g3a3") == 0))
	{
		sprintf(bank, "%d", 7);
		sprintf(slot, "%d", 4);
	}
	else if ((strcmp(line_buffer, "hk_g36e") == 0) || (strcmp(line_buffer, "g36e") == 0))
	{
		sprintf(bank, "%d", 7);
		sprintf(slot, "%d", 5);
	}
	else if ((strcmp(line_buffer, "colt_m16") == 0) || (strcmp(line_buffer, "m16") == 0))
	{
		sprintf(bank, "%d", 7);
		sprintf(slot, "%d", 6);
	}
	else if (strcmp(line_buffer, "ak74") == 0)
	{
		sprintf(bank, "%d", 7);
		sprintf(slot, "%d", 7);
	}
	else if (strcmp(line_buffer, "m14") == 0)
	{
		sprintf(bank, "%d", 7);
		sprintf(slot, "%d", 8);
	}
	else if (strcmp(line_buffer, "m4") == 0)
	{
		sprintf(bank, "%d", 7);
		sprintf(slot, "%d", 9);
	}
	// WEAPONS - SNIPES
	else if ((strcmp(line_buffer, "ssg_3000") == 0) || (strcmp(line_buffer, "ssg3000") == 0))
	{
		sprintf(bank, "%d", 8);
		sprintf(slot, "%d", 2);
	}
	else if ((strcmp(line_buffer, "barrett_m82") == 0) || (strcmp(line_buffer, "m82") == 0))
	{
		sprintf(bank, "%d", 8);
		sprintf(slot, "%d", 3);
	}
	else if (strcmp(line_buffer, "dragunov") == 0)
	{
		sprintf(bank, "%d", 8);
		sprintf(slot, "%d", 4);
	}
	// WEAPONS - MGS
	else if (strcmp(line_buffer, "m60") == 0)
	{
		sprintf(bank, "%d", 9);
		sprintf(slot, "%d", 2);
	}
	else if (strcmp(line_buffer, "m249") == 0)
	{
		sprintf(bank, "%d", 9);
		sprintf(slot, "%d", 3);
	}
	else if (strcmp(line_buffer, "pkm") == 0)
	{
		if (UTIL_IsNewerVersion())
		{
			sprintf(bank, "%d", 9);
			sprintf(slot, "%d", 4);
		}
		// spawn any mgun that is on this place
		else
		{
			sprintf(bank, "%d", 9);
			sprintf(slot, "%d", 3);
		}
	}
	// WEAPONS - GL
	else if ((strcmp(line_buffer, "gl_m79") == 0) || (strcmp(line_buffer, "m79") == 0))
	{
		sprintf(bank, "%d", 10);
		sprintf(slot, "%d", 2);
	}
	// SKILLS
	else if (strcmp(line_buffer, "marksmanship1") == 0)
	{
		number_of_skills = NumberOfFASkills(pBot);

		// handle only systems 1 and 2
		// system 3 (all skills) is handled above in BotStartGame() method
		if (((internals.GetFASkillSystem() == 1.0) && (number_of_skills < 1)) ||
			((internals.GetFASkillSystem() == 2.0) && (number_of_skills < 4)))
		{
			sprintf(bank, "%d", 23);
			sprintf(slot, "%d", 1);
			pBot->bot_fa_skill |= MARKS1;		// set also FA_skill that are spawned
		}
	}
	else if (strcmp(line_buffer, "marksmanship2") == 0)
	{
		number_of_skills = NumberOfFASkills(pBot);

		if (((internals.GetFASkillSystem() == 1.0) && (number_of_skills < 1)) ||
			((internals.GetFASkillSystem() == 2.0) && (number_of_skills < 4)))
		{
			sprintf(bank, "%d", 23);
			sprintf(slot, "%d", 2);

			// does bot already have marksmanship1
			if (pBot->bot_fa_skill & MARKS1)
				pBot->bot_fa_skill |= MARKS2;
			// otherwise take the marksmanship1 first
			else
				pBot->bot_fa_skill |= MARKS1;
		}
	}
	else if (strcmp(line_buffer, "nomenclature") == 0)
	{
		number_of_skills = NumberOfFASkills(pBot);

		if (((internals.GetFASkillSystem() == 1.0) && (number_of_skills < 1)) ||
			((internals.GetFASkillSystem() == 2.0) && (number_of_skills < 4)))
		{
			sprintf(bank, "%d", 23);
			sprintf(slot, "%d", 3);
			pBot->bot_fa_skill |= NOMEN;
		}
	}
	else if (strcmp(line_buffer, "battlefield_agility") == 0)
	{
		number_of_skills = NumberOfFASkills(pBot);

		if (((internals.GetFASkillSystem() == 1.0) && (number_of_skills < 1)) ||
			((internals.GetFASkillSystem() == 2.0) && (number_of_skills < 4)))
		{
			sprintf(bank, "%d", 23);
			sprintf(slot, "%d", 4);
			pBot->bot_fa_skill |= BATTAG;
		}
	}
	else if (strcmp(line_buffer, "leadership") == 0)
	{
		number_of_skills = NumberOfFASkills(pBot);

		if (((internals.GetFASkillSystem() == 1.0) && (number_of_skills < 1)) ||
			((internals.GetFASkillSystem() == 2.0) && (number_of_skills < 4)))
		{
			sprintf(bank, "%d", 23);
			sprintf(slot, "%d", 5);
			pBot->bot_fa_skill |= LEADER;
		}
	}
	else if (strcmp(line_buffer, "first_aid") == 0)
	{
		number_of_skills = NumberOfFASkills(pBot);

		if (((internals.GetFASkillSystem() == 1.0) && (number_of_skills < 1)) ||
			((internals.GetFASkillSystem() == 2.0) && (number_of_skills < 4)))
		{
			sprintf(bank, "%d", 23);
			sprintf(slot, "%d", 6);
			pBot->bot_fa_skill |= FAID;
		}
	}
	else if (strcmp(line_buffer, "field_medicine") == 0)
	{
		number_of_skills = NumberOfFASkills(pBot);

		if (((internals.GetFASkillSystem() == 1.0) && (number_of_skills < 1)) ||
			((internals.GetFASkillSystem() == 2.0) && (number_of_skills < 4)))
		{
			sprintf(bank, "%d", 23);
			sprintf(slot, "%d", 7);

			if (pBot->bot_fa_skill & FAID)
				pBot->bot_fa_skill |= MEDIC;
			else
				pBot->bot_fa_skill |= FAID;
		}
	}
	else if (strcmp(line_buffer, "artillery1") == 0)
	{
		number_of_skills = NumberOfFASkills(pBot);

		if (((internals.GetFASkillSystem() == 1.0) && (number_of_skills < 1)) ||
			((internals.GetFASkillSystem() == 2.0) && (number_of_skills < 4)))
		{
			sprintf(bank, "%d", 23);
			sprintf(slot, "%d", 8);
			pBot->bot_fa_skill |= ARTY1;
		}
	}
	else if (strcmp(line_buffer, "artillery2") == 0)
	{
		number_of_skills = NumberOfFASkills(pBot);

		if (((internals.GetFASkillSystem() == 1.0) && (number_of_skills < 1)) ||
			((internals.GetFASkillSystem() == 2.0) && (number_of_skills < 4)))
		{
			sprintf(bank, "%d", 23);
			sprintf(slot, "%d", 9);

			if (pBot->bot_fa_skill & ARTY1)
				pBot->bot_fa_skill |= ARTY2;
			else
				pBot->bot_fa_skill |= ARTY1;
		}
	}
	else if (strcmp(line_buffer, "stealth") == 0)
	{
		number_of_skills = NumberOfFASkills(pBot);

		if (((internals.GetFASkillSystem() == 1.0) && (number_of_skills < 1)) ||
			((internals.GetFASkillSystem() == 2.0) && (number_of_skills < 4)))
		{
			sprintf(bank, "%d", 23);
			sprintf(slot, "%d", 10);
			pBot->bot_fa_skill |= STEALTH;
		}
	}

	// set main weapon
	if (isnt(bank, "-1") && isnt(bank, "1") && isnt(bank, "2") && isnt(bank, "3") && isnt(bank, "4") &&
		isnt(bank, "23"))
	{
		int new_weapon = SetMainWeapon(bank, slot);

		// we should try to rearrange weapons before we set this weapon as a main weapon
		// without this method we can lose m82, for example, because if there's already
		// mp5, for example, as a main weapon then the m82 cannot be used
		// (ie. the order of weapons in class doesn't matter with this method)
		if (RearrangeWeapons(pBot, new_weapon))
		{
			pBot->main_weapon = new_weapon;
		}
	}
	// set backup weapon
	if (isnt(bank, "-1") && (FStrEq(bank, "4") || FStrEq(bank, "5") || FStrEq(bank, "6") || FStrEq(bank, "10"))
		&& (pBot->backup_weapon == -1))
		pBot->backup_weapon = SetBackupWeapon(pBot, bank, slot);
	// set grenades
	if (isnt(bank, "-1") && FStrEq(bank, "3") && (pBot->grenade_slot == -1))
		pBot->grenade_slot = SetGrenade(bank, slot);
	// set claymore
	if (isnt(bank, "-1") && FStrEq(bank, "3") && (pBot->claymore_slot == -1))
		pBot->claymore_slot = SetMine(bank, slot);

	// check for validity of the arguments before calling the command
	if (isnt(bank, "-1") && isnt(slot, "-1"))
		FakeClientCommand(pEdict, "loadpreconfig", bank, slot);

	return;
}

/*
* we need to check if there is free slot for this weapon ie. if the bot can carry it
* we test backup slot first because of the way the weapons are spawned
* ie. "weaker" weapon menus come first
*/
bool CanSpawnIt(bot_t *pBot, int iID)
{
	if (pBot->backup_weapon == -1)
	{
		pBot->backup_weapon = iID;

		return TRUE;
	}

	if (pBot->main_weapon == -1)
	{
		pBot->main_weapon = iID;

		return TRUE;
	}

	return FALSE;
}

/*
* move weapons to main slot
*/
void RearrangeWeapons(bot_t *pBot)
{
	if ((pBot->main_weapon == -1) && (pBot->backup_weapon != -1) &&
		(pBot->backup_weapon != fa_weapon_coltgov) &&
		(pBot->backup_weapon != fa_weapon_ber92f) &&
		(pBot->backup_weapon != fa_weapon_ber93r) &&
		(pBot->backup_weapon != fa_weapon_anaconda) &&
		(pBot->backup_weapon != fa_weapon_desert))
	{
		pBot->main_weapon = pBot->backup_weapon;

		// we have clear the backup slot to prevent wierd behaviour
		pBot->backup_weapon = -1;


		//@@@@@@@@@@@@@TEMPORARY
#ifdef _DEBUG
		ALERT(at_console, "\n!!!!REARRANGING WEAPONS (backup cleared)\n");
#endif
	}
}

/*
* move weapons so that the main slot always holds the best available weapon
*/
bool RearrangeWeapons(bot_t *pBot, int weapon)
{
	// there is a SR or MG or AR in main slot so leave it that way
	if ((pBot->main_weapon != -1) &&
		(UTIL_IsSniperRifle(pBot->main_weapon) || UTIL_IsMachinegun(pBot->main_weapon) ||
		UTIL_IsAssaultRifle(pBot->main_weapon)))
		return FALSE;

	// there is a Shotgun or SMG in main slot and we're spawning "better" weapon
	if ((pBot->main_weapon != -1) &&
		(UTIL_IsShotgun(pBot->main_weapon) || UTIL_IsSMG(pBot->main_weapon)) &&
		(UTIL_IsSniperRifle(weapon) || UTIL_IsMachinegun(weapon) ||
		UTIL_IsAssaultRifle(weapon)))
	{
		// so move current main weapon to backup slot
		pBot->backup_weapon = pBot->main_weapon;


		//@@@@@@@@@@@@@TEMPORARY
#ifdef _DEBUG
		ALERT(at_console, "\n!!!!REARRANGING WEAPONS (main became backup)\n");
#endif
	}

	return TRUE;
}

/*
* handles Firearms 2.5 custom class spawning menu system
*/
void FA25SpawnBaseArmor(bot_t *pBot, const char *line_buffer)
{
	edict_t *pEdict = pBot->pEdict;

	if (strcmp(line_buffer, "light_armor") == 0)
	{
		FakeClientCommand(pEdict, "vguimenuoption", "1", NULL);
	}
	else if (strcmp(line_buffer, "medium_armor") == 0)
	{
		FakeClientCommand(pEdict, "vguimenuoption", "2", NULL);
	}
	else if (strcmp(line_buffer, "heavy_armor") == 0)
	{
		FakeClientCommand(pEdict, "vguimenuoption", "3", NULL);
	}
}

/*
* handles Firearms 2.5 custom class spawning menu system
* reads both MB as well as FA (all versions) tokens
* also converts weapons that don't exist in FA25 to similar weapon that exist in FA25
*/
void FA25SpawnAdditionalArmor(bot_t *pBot, const char *line_buffer)
{
	edict_t *pEdict = pBot->pEdict;

	if ((strcmp(line_buffer, "armor_helmet") == 0) || (strcmp(line_buffer, "helmet") == 0))
	{
		FakeClientCommand(pEdict, "vguimenuoption", "2", NULL);
	}
	if ((strcmp(line_buffer, "armor_arms") == 0) || (strcmp(line_buffer, "arms") == 0))
	{
		FakeClientCommand(pEdict, "vguimenuoption", "3", NULL);
	}
	if ((strcmp(line_buffer, "armor_legs") == 0) || (strcmp(line_buffer, "legs") == 0))
	{
		FakeClientCommand(pEdict, "vguimenuoption", "4", NULL);
	}
}

/*
* handles Firearms 2.5 custom class spawning menu system
* reads both MB as well as FA (all versions) tokens
* also converts weapons that don't exist in FA25 to similar weapon that exist in FA25
*/
void FA25SpawnItems(bot_t *pBot, const char *line_buffer)
{
	edict_t *pEdict = pBot->pEdict;

	if ((strcmp(line_buffer, "frag_grenade") == 0) || (strcmp(line_buffer, "frag") == 0))
	{
		pBot->grenade_slot = fa_weapon_frag;

		FakeClientCommand(pEdict, "vguimenuoption", "2", NULL);
	}
	// concussion is nearly same weapon so we could spawn flashbang
	if ((strcmp(line_buffer, "concussion_grenade") == 0) ||
		(strcmp(line_buffer, "conc") == 0) || (strcmp(line_buffer, "flashbang") == 0))
	{
		pBot->grenade_slot = fa_weapon_flashbang;

		FakeClientCommand(pEdict, "vguimenuoption", "3", NULL);
	}
	if ((strcmp(line_buffer, "stiel_grenade") == 0) || (strcmp(line_buffer, "stg24") == 0))
	{
		pBot->grenade_slot = fa_weapon_stg24;

		FakeClientCommand(pEdict, "vguimenuoption", "4", NULL);
	}
	if ((strcmp(line_buffer, "claymore_mine") == 0) || (strcmp(line_buffer, "claymore") == 0))
	{
		pBot->claymore_slot = fa_weapon_claymore;

		FakeClientCommand(pEdict, "vguimenuoption", "5", NULL);
	}
	if (strcmp(line_buffer, "bandages") == 0)
	{
		FakeClientCommand(pEdict, "vguimenuoption", "6", NULL);
	}
}

/*
* handles Firearms 2.5 custom class spawning menu system
* reads both MB as well as FA (all versions) tokens
* also converts weapons that don't exist in FA25 to similar weapon that exist in FA25
*/
void FA25SpawnSidearms(bot_t *pBot, const char *line_buffer)
{
	edict_t *pEdict = pBot->pEdict;

	if ((strcmp(line_buffer, "colt_gov") == 0) ||
		(strcmp(line_buffer, "1911a1") == 0) || (strcmp(line_buffer, "acpsa") == 0))
	{
		pBot->backup_weapon = fa_weapon_coltgov;

		FakeClientCommand(pEdict, "vguimenuoption", "2", NULL);
	}
	if ((strcmp(line_buffer, "colt_anaconda") == 0) ||
		(strcmp(line_buffer, "anaconda") == 0) || (strcmp(line_buffer, "rvsascp") == 0))
	{
		pBot->backup_weapon = fa_weapon_anaconda;

		FakeClientCommand(pEdict, "vguimenuoption", "3", NULL);
	}
	if ((strcmp(line_buffer, "ber_93r") == 0) ||
		(strcmp(line_buffer, "93r") == 0) || (strcmp(line_buffer, "sa3rb") == 0))
	{
		pBot->backup_weapon = fa_weapon_ber93r;

		FakeClientCommand(pEdict, "vguimenuoption", "4", NULL);
	}
	if ((strcmp(line_buffer, "ber_92f") == 0) ||
		(strcmp(line_buffer, "92f") == 0) || (strcmp(line_buffer, "sasil") == 0))
	{
		pBot->backup_weapon = fa_weapon_ber92f;

		FakeClientCommand(pEdict, "vguimenuoption", "5", NULL);
	}
	if ((strcmp(line_buffer, "desert_eagle") == 0) ||
		(strcmp(line_buffer, "deagle") == 0) || (strcmp(line_buffer, "aesa") == 0))
	{
		pBot->backup_weapon = fa_weapon_desert;

		FakeClientCommand(pEdict, "vguimenuoption", "6", NULL);
	}
}

/*
* handles Firearms 2.5 custom class spawning menu system
* reads both MB as well as FA (all versions) tokens
* also converts weapons that don't exist in FA25 to similar weapon that exist in FA25
*/
void FA25SpawnShotguns(bot_t *pBot, const char *line_buffer)
{
	edict_t *pEdict = pBot->pEdict;

	if ((strcmp(line_buffer, "benelli_m3") == 0) || (strcmp(line_buffer, "m3") == 0) ||
		(strcmp(line_buffer, "remington") == 0) || (strcmp(line_buffer, "tac12") == 0))
	{
		if (CanSpawnIt(pBot, fa_weapon_benelli))
			FakeClientCommand(pEdict, "vguimenuoption", "2", NULL);
	}
	if ((strcmp(line_buffer, "saiga_12k") == 0) ||
		(strcmp(line_buffer, "12k") == 0) || (strcmp(line_buffer, "autoshot") == 0))
	{
		if (CanSpawnIt(pBot, fa_weapon_saiga))
			FakeClientCommand(pEdict, "vguimenuoption", "3", NULL);
	}
}

/*
* handles Firearms 2.5 custom class spawning menu system
* reads both MB as well as FA (all versions) tokens
* also converts weapons that don't exist in FA25 to similar weapon that exist in FA25
* we also handle that single difference between FA24 and FA25 here
*/
void FA25SpawnSMGs(bot_t *pBot, const char *line_buffer)
{
	edict_t *pEdict = pBot->pEdict;

	if ((strcmp(line_buffer, "sterling") == 0) || (strcmp(line_buffer, "9mmlc") == 0) ||
		(strcmp(line_buffer, "mp5k") == 0) || (strcmp(line_buffer, "smgsil") == 0))
	{
		int the_weapon;

		// there's a sterling in FA25
		if (g_mod_version == FA_25)
			the_weapon = fa_weapon_sterling;
		// there's a mp5k in FA24
		else
			the_weapon = fa_weapon_mp5k;

		if (CanSpawnIt(pBot, the_weapon))
			FakeClientCommand(pEdict, "vguimenuoption", "2", NULL);
	}
	if ((strcmp(line_buffer, "hk_mp5") == 0) ||
		(strcmp(line_buffer, "mp5") == 0) || (strcmp(line_buffer, "tacsmg") == 0))
	{
		if (CanSpawnIt(pBot, fa_weapon_mp5a5))
			FakeClientCommand(pEdict, "vguimenuoption", "3", NULL);
	}
	if ((strcmp(line_buffer, "mc51") == 0) || (strcmp(line_buffer, "bfsmg") == 0))
	{
		if (CanSpawnIt(pBot, fa_weapon_mc51))
			FakeClientCommand(pEdict, "vguimenuoption", "4", NULL);
	}
	// we can also use uzi token to spawn this weapon
	if ((strcmp(line_buffer, "uzi") == 0) || (strcmp(line_buffer, "m11") == 0) ||
		(strcmp(line_buffer, "akimbo") == 0))
	{
		if (CanSpawnIt(pBot, fa_weapon_m11))
			FakeClientCommand(pEdict, "vguimenuoption", "5", NULL);
	}
	if ((strcmp(line_buffer, "bizon") == 0) || (strcmp(line_buffer, "hcmsmg") == 0))
	{
		if (CanSpawnIt(pBot, fa_weapon_bizon))
			FakeClientCommand(pEdict, "vguimenuoption", "6", NULL);
	}
}

/*
* handles Firearms 2.5 custom class spawning menu system
* reads both MB as well as FA (all versions) tokens
* also converts weapons that don't exist in FA25 to similar weapon that exist in FA25
*/
void FA25SpawnRifles(bot_t *pBot, const char *line_buffer)
{
	edict_t *pEdict = pBot->pEdict;

	if ((strcmp(line_buffer, "ak47") == 0) || (strcmp(line_buffer, "arbt") == 0))
	{
		if (CanSpawnIt(pBot, fa_weapon_ak47))
			FakeClientCommand(pEdict, "vguimenuoption", "2", NULL);
	}
	if ((strcmp(line_buffer, "famas") == 0) || (strcmp(line_buffer, "artac") == 0))
	{
#ifndef NOFAMAS
		if (CanSpawnIt(pBot, fa_weapon_famas))
			FakeClientCommand(pEdict, "vguimenuoption", "3", NULL);
#endif
	}
	if ((strcmp(line_buffer, "hk_g3a3") == 0) || (strcmp(line_buffer, "tacarlcm") == 0))
	{
		if (CanSpawnIt(pBot, fa_weapon_g3a3))
			FakeClientCommand(pEdict, "vguimenuoption", "4", NULL);
	}
	if ((strcmp(line_buffer, "hk_g36e") == 0) || (strcmp(line_buffer, "natoarscp") == 0))
	{
		if (CanSpawnIt(pBot, fa_weapon_g36e))
			FakeClientCommand(pEdict, "vguimenuoption", "5", NULL);
	}
	// we can also use ak74 to spawn this weapon
	if ((strcmp(line_buffer, "colt_m16") == 0) || (strcmp(line_buffer, "m16") == 0) ||
		(strcmp(line_buffer, "argl") == 0) || (strcmp(line_buffer, "ak74") == 0))
	{
		if (CanSpawnIt(pBot, fa_weapon_m16))
			FakeClientCommand(pEdict, "vguimenuoption", "6", NULL);
	}
	if ((strcmp(line_buffer, "m4") == 0) || (strcmp(line_buffer, "carbine") == 0))
	{
		if (CanSpawnIt(pBot, fa_weapon_m4))
			FakeClientCommand(pEdict, "vguimenuoption", "7", NULL);
	}
	// we can also use m14 token to spawn this weapon
	if ((strcmp(line_buffer, "m14") == 0) || (strcmp(line_buffer, "aug") == 0) ||
		(strcmp(line_buffer, "bparscp") == 0))
	{
		if (CanSpawnIt(pBot, fa_weapon_aug))
			FakeClientCommand(pEdict, "vguimenuoption", "8", NULL);
	}
}

/*
* handles Firearms 2.5 custom class spawning menu system
* reads both MB as well as FA (all versions) tokens
* also converts weapons that don't exist in FA25 to similar weapon that exist in FA25
*/
void FA25SpawnSnipes(bot_t *pBot, const char *line_buffer)
{
	edict_t *pEdict = pBot->pEdict;

	// we can also use ssg3000 and dragunov tokens to spawn this weapon
	// both are similar weapons
	if ((strcmp(line_buffer, "ssg_3000") == 0) || (strcmp(line_buffer, "ssg3000") == 0) ||
		(strcmp(line_buffer, "dragunov") == 0) || (strcmp(line_buffer, "psg1") == 0) ||
		(strcmp(line_buffer, "srscp") == 0))
	{
		if (CanSpawnIt(pBot, fa_weapon_psg1))
			FakeClientCommand(pEdict, "vguimenuoption", "2", NULL);
	}
	if ((strcmp(line_buffer, "barrett_m82") == 0) ||
		(strcmp(line_buffer, "m82") == 0) || (strcmp(line_buffer, "atsrscp") == 0))
	{
		if (CanSpawnIt(pBot, fa_weapon_m82))
			FakeClientCommand(pEdict, "vguimenuoption", "3", NULL);
	}
}

/*
* handles Firearms 2.5 custom class spawning menu system
* reads both MB as well as FA (all versions) tokens
* also converts weapons that don't exist in FA25 to similar weapon that exist in FA25
*/
void FA25SpawnMGuns(bot_t *pBot, const char *line_buffer)
{
	edict_t *pEdict = pBot->pEdict;

	// we can also use m249 and pkm tokens to spawn this weapon
	if ((strcmp(line_buffer, "m60") == 0) ||
		(strcmp(line_buffer, "m249") == 0) ||
		(strcmp(line_buffer, "pkm") == 0) ||
		(strcmp(line_buffer, "hcmmg") == 0))
	{
		if (CanSpawnIt(pBot, fa_weapon_m60))
			FakeClientCommand(pEdict, "vguimenuoption", "2", NULL);
	}
}

/*
* handles Firearms 2.5 custom class spawning menu system
* reads both MB as well as FA (all versions) tokens
* also converts weapons that don't exist in FA25 to similar weapon that exist in FA25
*/
void FA25SpawnGLs(bot_t *pBot, const char *line_buffer)
{
	edict_t *pEdict = pBot->pEdict;

	if ((strcmp(line_buffer, "gl_m79") == 0) ||
		(strcmp(line_buffer, "m79") == 0) || (strcmp(line_buffer, "40mmgl") == 0))
	{
		if (CanSpawnIt(pBot, fa_weapon_m79))
			FakeClientCommand(pEdict, "vguimenuoption", "2", NULL);
	}
}

/*
* handles skills menu, we need to check if current skill system allows us to spawn the skill and
* we also need to check if the bot already has a skill needed to pick this skill
* (ie. Marks1 for Marks2, Arty1 for Arty2 etc.)
*/
bool BotSpawnSkills(bot_t *pBot, const char *line_buffer)
{
	int number_of_skills;

	edict_t *pEdict = pBot->pEdict;

	number_of_skills = NumberOfFASkills(pBot);
	
	if ((strcmp(line_buffer, "marksmanship1") == 0) && !(pBot->bot_fa_skill & MARKS1))
	{
		// handle only systems 1 and 2
		// if the bot was promoted then ignore the skill count
		if (pBot->IsTask(TASK_PROMOTED) || ((internals.GetFASkillSystem() == 1.0) && (number_of_skills < 1)) ||
			((internals.GetFASkillSystem() == 2.0) && (number_of_skills < 4)))
		{
			pBot->bot_fa_skill |= MARKS1;		// set also FA_skill that are spawned

			FakeClientCommand(pEdict, "setskill", "0", NULL);
		}

		return TRUE;
	}

	if ((strcmp(line_buffer, "marksmanship2") == 0) && !(pBot->bot_fa_skill & MARKS2))
	{
		if (pBot->IsTask(TASK_PROMOTED) || ((internals.GetFASkillSystem() == 1.0) && (number_of_skills < 1)) ||
			((internals.GetFASkillSystem() == 2.0) && (number_of_skills < 4)))
		{
			// has the bot marksmanship1
			if (pBot->bot_fa_skill & MARKS1)
			{
				pBot->bot_fa_skill |= MARKS2;

				FakeClientCommand(pEdict, "setskill", "1", NULL);
			}
			// otherwise take the marksmanship1 first
			else
			{
				pBot->bot_fa_skill |= MARKS1;

				FakeClientCommand(pEdict, "setskill", "0", NULL);
			}
		}
		
		return TRUE;
	}

	if ((strcmp(line_buffer, "nomenclature") == 0) && !(pBot->bot_fa_skill & NOMEN))
	{
		if (pBot->IsTask(TASK_PROMOTED) || ((internals.GetFASkillSystem() == 1.0) && (number_of_skills < 1)) ||
			((internals.GetFASkillSystem() == 2.0) && (number_of_skills < 4)))
		{
			pBot->bot_fa_skill |= NOMEN;

			FakeClientCommand(pEdict, "setskill", "2", NULL);
		}

		return TRUE;		
	}

	if ((strcmp(line_buffer, "battlefield_agility") == 0) && !(pBot->bot_fa_skill & BATTAG))
	{
		if (pBot->IsTask(TASK_PROMOTED) || ((internals.GetFASkillSystem() == 1.0) && (number_of_skills < 1)) ||
			((internals.GetFASkillSystem() == 2.0) && (number_of_skills < 4)))
		{			
			pBot->bot_fa_skill |= BATTAG;

			FakeClientCommand(pEdict, "setskill", "3", NULL);
		}
		
		return TRUE;
	}

	if ((strcmp(line_buffer, "leadership") == 0) && !(pBot->bot_fa_skill & LEADER))
	{
		if (pBot->IsTask(TASK_PROMOTED) || ((internals.GetFASkillSystem() == 1.0) && (number_of_skills < 1)) ||
			((internals.GetFASkillSystem() == 2.0) && (number_of_skills < 4)))
		{		
			pBot->bot_fa_skill |= LEADER;

			FakeClientCommand(pEdict, "setskill", "4", NULL);
		}
		
		return TRUE;
	}

	if ((strcmp(line_buffer, "first_aid") == 0) && !(pBot->bot_fa_skill & FAID))
	{
		if (pBot->IsTask(TASK_PROMOTED) || ((internals.GetFASkillSystem() == 1.0) && (number_of_skills < 1)) ||
			((internals.GetFASkillSystem() == 2.0) && (number_of_skills < 4)))
		{
			pBot->bot_fa_skill |= FAID;

			FakeClientCommand(pEdict, "setskill", "5", NULL);
		}

		return TRUE;
	}

	if ((strcmp(line_buffer, "field_medicine") == 0) && !(pBot->bot_fa_skill & MEDIC))
	{
		if (pBot->IsTask(TASK_PROMOTED) || ((internals.GetFASkillSystem() == 1.0) && (number_of_skills < 1)) ||
			((internals.GetFASkillSystem() == 2.0) && (number_of_skills < 4)))
		{
			if (pBot->bot_fa_skill & FAID)
			{
				pBot->bot_fa_skill |= MEDIC;

				FakeClientCommand(pEdict, "setskill", "6", NULL);
			}
			else
			{
				pBot->bot_fa_skill |= FAID;

				FakeClientCommand(pEdict, "setskill", "5", NULL);
			}
		}
		
		return TRUE;
	}

	if ((strcmp(line_buffer, "artillery1") == 0) && !(pBot->bot_fa_skill & ARTY1))
	{
		if (pBot->IsTask(TASK_PROMOTED) || ((internals.GetFASkillSystem() == 1.0) && (number_of_skills < 1)) ||
			((internals.GetFASkillSystem() == 2.0) && (number_of_skills < 4)))
		{		
			pBot->bot_fa_skill |= ARTY1;

			FakeClientCommand(pEdict, "setskill", "7", NULL);
		}
		
		return TRUE;
	}
	
	if ((strcmp(line_buffer, "artillery2") == 0) && !(pBot->bot_fa_skill & ARTY2))
	{
		if (pBot->IsTask(TASK_PROMOTED) || ((internals.GetFASkillSystem() == 1.0) && (number_of_skills < 1)) ||
			((internals.GetFASkillSystem() == 2.0) && (number_of_skills < 4)))
		{
			if (pBot->bot_fa_skill & ARTY1)
			{
				pBot->bot_fa_skill |= ARTY2;

				FakeClientCommand(pEdict, "setskill", "8", NULL);
			}
			else
			{
				pBot->bot_fa_skill |= ARTY1;

				FakeClientCommand(pEdict, "setskill", "7", NULL);
			}
		}
		
		return TRUE;
	}

	if ((strcmp(line_buffer, "stealth") == 0) && !(pBot->bot_fa_skill & STEALTH))
	{
		if (pBot->IsTask(TASK_PROMOTED) || ((internals.GetFASkillSystem() == 1.0) && (number_of_skills < 1)) ||
			((internals.GetFASkillSystem() == 2.0) && (number_of_skills < 4)))
		{
			pBot->bot_fa_skill |= STEALTH;

			FakeClientCommand(pEdict, "setskill", "9", NULL);
		}

		return TRUE;
	}

	return FALSE;
}