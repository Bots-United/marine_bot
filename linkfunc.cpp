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
// linkfunc.cpp
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

#ifdef __BORLANDC__
extern HINSTANCE _h_Library;
#elif _WIN32
extern HINSTANCE h_Library;
#else
extern void *h_Library;
#endif

#ifdef __BORLANDC__

#define LINK_ENTITY_TO_FUNC(mapClassName) \
 extern "C" EXPORT void mapClassName( entvars_t *pev ); \
 void mapClassName( entvars_t *pev ) { \
      static LINK_ENTITY_FUNC otherClassName = NULL; \
      static int skip_this = 0; \
      if (skip_this) return; \
      if (otherClassName == NULL) \
         otherClassName = (LINK_ENTITY_FUNC)GetProcAddress(_h_Library, #mapClassName); \
      if (otherClassName == NULL) { \
         skip_this = 1; return; \
      } \
      (*otherClassName)(pev); }

#else

#define LINK_ENTITY_TO_FUNC(mapClassName) \
 extern "C" EXPORT void mapClassName( entvars_t *pev ); \
 void mapClassName( entvars_t *pev ) { \
      static LINK_ENTITY_FUNC otherClassName = NULL; \
      static int skip_this = 0; \
      if (skip_this) return; \
      if (otherClassName == NULL) \
         otherClassName = (LINK_ENTITY_FUNC)GetProcAddress(h_Library, #mapClassName); \
      if (otherClassName == NULL) { \
         skip_this = 1; return; \
      } \
      (*otherClassName)(pev); }

#endif


// new stuff for 1.1.0.4 release
//LINK_ENTITY_TO_FUNC(CreateInterface);

// entities for Valve's hl.dll and Standard SDK...
LINK_ENTITY_TO_FUNC(aiscripted_sequence);
LINK_ENTITY_TO_FUNC(ambient_generic);
LINK_ENTITY_TO_FUNC(ammo_357);
LINK_ENTITY_TO_FUNC(ammo_9mmAR);
LINK_ENTITY_TO_FUNC(ammo_9mmbox);
LINK_ENTITY_TO_FUNC(ammo_9mmclip);
LINK_ENTITY_TO_FUNC(ammo_ARgrenades);
LINK_ENTITY_TO_FUNC(ammo_buckshot);
LINK_ENTITY_TO_FUNC(ammo_crossbow);
LINK_ENTITY_TO_FUNC(ammo_egonclip);
LINK_ENTITY_TO_FUNC(ammo_gaussclip);
LINK_ENTITY_TO_FUNC(ammo_glockclip);
LINK_ENTITY_TO_FUNC(ammo_mp5clip);
LINK_ENTITY_TO_FUNC(ammo_mp5grenades);
LINK_ENTITY_TO_FUNC(ammo_rpgclip);
LINK_ENTITY_TO_FUNC(beam);
LINK_ENTITY_TO_FUNC(bmortar);
LINK_ENTITY_TO_FUNC(bodyque);
LINK_ENTITY_TO_FUNC(button_target);
LINK_ENTITY_TO_FUNC(cine_blood);
LINK_ENTITY_TO_FUNC(controller_energy_ball);
LINK_ENTITY_TO_FUNC(controller_head_ball);
LINK_ENTITY_TO_FUNC(crossbow_bolt);
LINK_ENTITY_TO_FUNC(cycler);
LINK_ENTITY_TO_FUNC(cycler_prdroid);
LINK_ENTITY_TO_FUNC(cycler_sprite);
LINK_ENTITY_TO_FUNC(cycler_weapon);
LINK_ENTITY_TO_FUNC(cycler_wreckage);
LINK_ENTITY_TO_FUNC(DelayedUse);
LINK_ENTITY_TO_FUNC(env_beam);
LINK_ENTITY_TO_FUNC(env_beverage);
LINK_ENTITY_TO_FUNC(env_blood);
LINK_ENTITY_TO_FUNC(env_bubbles);
LINK_ENTITY_TO_FUNC(env_debris);
LINK_ENTITY_TO_FUNC(env_explosion);
LINK_ENTITY_TO_FUNC(env_fade);
LINK_ENTITY_TO_FUNC(env_funnel);
LINK_ENTITY_TO_FUNC(env_global);
LINK_ENTITY_TO_FUNC(env_glow);
LINK_ENTITY_TO_FUNC(env_laser);
LINK_ENTITY_TO_FUNC(env_lightning);
LINK_ENTITY_TO_FUNC(env_message);
LINK_ENTITY_TO_FUNC(env_render);
LINK_ENTITY_TO_FUNC(env_shake);
LINK_ENTITY_TO_FUNC(env_shooter);
LINK_ENTITY_TO_FUNC(env_smoker);
LINK_ENTITY_TO_FUNC(env_sound);
LINK_ENTITY_TO_FUNC(env_spark);
LINK_ENTITY_TO_FUNC(env_sprite);
LINK_ENTITY_TO_FUNC(fireanddie);
LINK_ENTITY_TO_FUNC(func_breakable);
LINK_ENTITY_TO_FUNC(func_button);
LINK_ENTITY_TO_FUNC(func_conveyor);
LINK_ENTITY_TO_FUNC(func_door);
LINK_ENTITY_TO_FUNC(func_door_rotating);
LINK_ENTITY_TO_FUNC(func_friction);
LINK_ENTITY_TO_FUNC(func_guntarget);
LINK_ENTITY_TO_FUNC(func_healthcharger);
LINK_ENTITY_TO_FUNC(func_illusionary);
LINK_ENTITY_TO_FUNC(func_ladder);
LINK_ENTITY_TO_FUNC(func_monsterclip);
LINK_ENTITY_TO_FUNC(func_mortar_field);
LINK_ENTITY_TO_FUNC(func_pendulum);
LINK_ENTITY_TO_FUNC(func_plat);
LINK_ENTITY_TO_FUNC(func_platrot);
LINK_ENTITY_TO_FUNC(func_pushable);
LINK_ENTITY_TO_FUNC(func_recharge);
LINK_ENTITY_TO_FUNC(func_rot_button);
LINK_ENTITY_TO_FUNC(func_rotating);
LINK_ENTITY_TO_FUNC(func_tank);
LINK_ENTITY_TO_FUNC(func_tankcontrols);
LINK_ENTITY_TO_FUNC(func_tanklaser);
LINK_ENTITY_TO_FUNC(func_tankmortar);
LINK_ENTITY_TO_FUNC(func_tankrocket);
LINK_ENTITY_TO_FUNC(func_trackautochange);
LINK_ENTITY_TO_FUNC(func_trackchange);
LINK_ENTITY_TO_FUNC(func_tracktrain);
LINK_ENTITY_TO_FUNC(func_train);
LINK_ENTITY_TO_FUNC(func_traincontrols);
LINK_ENTITY_TO_FUNC(func_wall);
LINK_ENTITY_TO_FUNC(func_wall_toggle);
LINK_ENTITY_TO_FUNC(func_water);
LINK_ENTITY_TO_FUNC(game_counter);
LINK_ENTITY_TO_FUNC(game_counter_set);
LINK_ENTITY_TO_FUNC(game_end);
LINK_ENTITY_TO_FUNC(game_player_equip);
LINK_ENTITY_TO_FUNC(game_player_hurt);
LINK_ENTITY_TO_FUNC(game_player_team);
LINK_ENTITY_TO_FUNC(game_score);
LINK_ENTITY_TO_FUNC(game_team_master);
LINK_ENTITY_TO_FUNC(game_team_set);
LINK_ENTITY_TO_FUNC(game_text);
LINK_ENTITY_TO_FUNC(game_zone_player);
LINK_ENTITY_TO_FUNC(garg_stomp);
LINK_ENTITY_TO_FUNC(gibshooter);
LINK_ENTITY_TO_FUNC(grenade);
LINK_ENTITY_TO_FUNC(hornet);
LINK_ENTITY_TO_FUNC(hvr_rocket);
LINK_ENTITY_TO_FUNC(info_bigmomma);
LINK_ENTITY_TO_FUNC(info_intermission);
LINK_ENTITY_TO_FUNC(info_landmark);
LINK_ENTITY_TO_FUNC(info_node);
LINK_ENTITY_TO_FUNC(info_node_air);
LINK_ENTITY_TO_FUNC(info_null);
LINK_ENTITY_TO_FUNC(info_player_deathmatch);
LINK_ENTITY_TO_FUNC(info_player_start);
LINK_ENTITY_TO_FUNC(info_target);
LINK_ENTITY_TO_FUNC(info_teleport_destination);
LINK_ENTITY_TO_FUNC(infodecal);
LINK_ENTITY_TO_FUNC(item_airtank);
LINK_ENTITY_TO_FUNC(item_antidote);
LINK_ENTITY_TO_FUNC(item_battery);
LINK_ENTITY_TO_FUNC(item_healthkit);
LINK_ENTITY_TO_FUNC(item_longjump);
LINK_ENTITY_TO_FUNC(item_security);
LINK_ENTITY_TO_FUNC(item_sodacan);
LINK_ENTITY_TO_FUNC(item_suit);
LINK_ENTITY_TO_FUNC(laser_spot);
LINK_ENTITY_TO_FUNC(light);
LINK_ENTITY_TO_FUNC(light_environment);
LINK_ENTITY_TO_FUNC(light_spot);
LINK_ENTITY_TO_FUNC(momentary_door);
LINK_ENTITY_TO_FUNC(momentary_rot_button);
LINK_ENTITY_TO_FUNC(monstermaker);
LINK_ENTITY_TO_FUNC(monster_alien_controller);
LINK_ENTITY_TO_FUNC(monster_alien_grunt);
LINK_ENTITY_TO_FUNC(monster_alien_slave);
LINK_ENTITY_TO_FUNC(monster_apache);
LINK_ENTITY_TO_FUNC(monster_babycrab);
LINK_ENTITY_TO_FUNC(monster_barnacle);
LINK_ENTITY_TO_FUNC(monster_barney);
LINK_ENTITY_TO_FUNC(monster_barney_dead);
LINK_ENTITY_TO_FUNC(monster_bigmomma);
LINK_ENTITY_TO_FUNC(monster_bloater);
LINK_ENTITY_TO_FUNC(monster_bullchicken);
LINK_ENTITY_TO_FUNC(monster_cine2_hvyweapons);
LINK_ENTITY_TO_FUNC(monster_cine2_scientist);
LINK_ENTITY_TO_FUNC(monster_cine2_slave);
LINK_ENTITY_TO_FUNC(monster_cine3_barney);
LINK_ENTITY_TO_FUNC(monster_cine3_scientist);
LINK_ENTITY_TO_FUNC(monster_cine_barney);
LINK_ENTITY_TO_FUNC(monster_cine_panther);
LINK_ENTITY_TO_FUNC(monster_cine_scientist);
LINK_ENTITY_TO_FUNC(monster_cockroach);
LINK_ENTITY_TO_FUNC(monster_flyer);
LINK_ENTITY_TO_FUNC(monster_flyer_flock);
LINK_ENTITY_TO_FUNC(monster_furniture);
LINK_ENTITY_TO_FUNC(monster_gargantua);
LINK_ENTITY_TO_FUNC(monster_generic);
LINK_ENTITY_TO_FUNC(monster_gman);
LINK_ENTITY_TO_FUNC(monster_grunt_repel);
LINK_ENTITY_TO_FUNC(monster_headcrab);
LINK_ENTITY_TO_FUNC(monster_hevsuit_dead);
LINK_ENTITY_TO_FUNC(monster_hgrunt_dead);
LINK_ENTITY_TO_FUNC(monster_houndeye);
LINK_ENTITY_TO_FUNC(monster_human_assassin);
LINK_ENTITY_TO_FUNC(monster_human_grunt);
LINK_ENTITY_TO_FUNC(monster_ichthyosaur);
LINK_ENTITY_TO_FUNC(monster_leech);
LINK_ENTITY_TO_FUNC(monster_miniturret);
LINK_ENTITY_TO_FUNC(monster_mortar);
LINK_ENTITY_TO_FUNC(monster_nihilanth);
LINK_ENTITY_TO_FUNC(monster_osprey);
LINK_ENTITY_TO_FUNC(monster_rat);
LINK_ENTITY_TO_FUNC(monster_satchel);
LINK_ENTITY_TO_FUNC(monster_scientist);
LINK_ENTITY_TO_FUNC(monster_scientist_dead);
LINK_ENTITY_TO_FUNC(monster_sentry);
LINK_ENTITY_TO_FUNC(monster_sitting_scientist);
LINK_ENTITY_TO_FUNC(monster_snark);
LINK_ENTITY_TO_FUNC(monster_tentacle);
LINK_ENTITY_TO_FUNC(monster_tentaclemaw);
LINK_ENTITY_TO_FUNC(monster_tripmine);
LINK_ENTITY_TO_FUNC(monster_turret);
LINK_ENTITY_TO_FUNC(monster_vortigaunt);
LINK_ENTITY_TO_FUNC(monster_zombie);
LINK_ENTITY_TO_FUNC(multi_manager);
LINK_ENTITY_TO_FUNC(multisource);
LINK_ENTITY_TO_FUNC(nihilanth_energy_ball);
LINK_ENTITY_TO_FUNC(node_viewer);
LINK_ENTITY_TO_FUNC(node_viewer_fly);
LINK_ENTITY_TO_FUNC(node_viewer_human);
LINK_ENTITY_TO_FUNC(node_viewer_large);
LINK_ENTITY_TO_FUNC(path_corner);
LINK_ENTITY_TO_FUNC(path_track);
LINK_ENTITY_TO_FUNC(player);
LINK_ENTITY_TO_FUNC(player_loadsaved);
LINK_ENTITY_TO_FUNC(player_weaponstrip);
LINK_ENTITY_TO_FUNC(rpg_rocket);
LINK_ENTITY_TO_FUNC(scripted_sentence);
LINK_ENTITY_TO_FUNC(scripted_sequence);
LINK_ENTITY_TO_FUNC(soundent);
LINK_ENTITY_TO_FUNC(spark_shower);
LINK_ENTITY_TO_FUNC(speaker);
LINK_ENTITY_TO_FUNC(squidspit);
LINK_ENTITY_TO_FUNC(streak_spiral);
LINK_ENTITY_TO_FUNC(target_cdaudio);
LINK_ENTITY_TO_FUNC(test_effect);
LINK_ENTITY_TO_FUNC(testhull);
LINK_ENTITY_TO_FUNC(trigger);
LINK_ENTITY_TO_FUNC(trigger_auto);
LINK_ENTITY_TO_FUNC(trigger_autosave);
// hot fix for obj_armory, obj_willow & tc_thebes cutscenes DedicatedServer crashes
// we cannot allow the engine to use this entity otherwise the server will crash in cutscenes
#ifndef SERVER
LINK_ENTITY_TO_FUNC(trigger_camera);
#endif
LINK_ENTITY_TO_FUNC(trigger_cdaudio);
LINK_ENTITY_TO_FUNC(trigger_changelevel);
LINK_ENTITY_TO_FUNC(trigger_changetarget);
LINK_ENTITY_TO_FUNC(trigger_counter);
LINK_ENTITY_TO_FUNC(trigger_endsection);
LINK_ENTITY_TO_FUNC(trigger_gravity);
LINK_ENTITY_TO_FUNC(trigger_hurt);
LINK_ENTITY_TO_FUNC(trigger_monsterjump);
LINK_ENTITY_TO_FUNC(trigger_multiple);
LINK_ENTITY_TO_FUNC(trigger_once);
LINK_ENTITY_TO_FUNC(trigger_push);
LINK_ENTITY_TO_FUNC(trigger_relay);
LINK_ENTITY_TO_FUNC(trigger_teleport);
LINK_ENTITY_TO_FUNC(trigger_transition);
LINK_ENTITY_TO_FUNC(weapon_357);
LINK_ENTITY_TO_FUNC(weapon_9mmAR);
LINK_ENTITY_TO_FUNC(weapon_9mmhandgun);
LINK_ENTITY_TO_FUNC(weapon_crossbow);
LINK_ENTITY_TO_FUNC(weapon_crowbar);
LINK_ENTITY_TO_FUNC(weapon_egon);
LINK_ENTITY_TO_FUNC(weapon_gauss);
LINK_ENTITY_TO_FUNC(weapon_glock);
LINK_ENTITY_TO_FUNC(weapon_handgrenade);
LINK_ENTITY_TO_FUNC(weapon_hornetgun);
LINK_ENTITY_TO_FUNC(weapon_mp5);
LINK_ENTITY_TO_FUNC(weapon_python);
LINK_ENTITY_TO_FUNC(weapon_rpg);
LINK_ENTITY_TO_FUNC(weapon_satchel);
LINK_ENTITY_TO_FUNC(weapon_shotgun);
LINK_ENTITY_TO_FUNC(weapon_snark);
LINK_ENTITY_TO_FUNC(weapon_tripmine);
LINK_ENTITY_TO_FUNC(weaponbox);
LINK_ENTITY_TO_FUNC(world_items);
LINK_ENTITY_TO_FUNC(worldspawn);
LINK_ENTITY_TO_FUNC(xen_hair);
LINK_ENTITY_TO_FUNC(xen_hull);
LINK_ENTITY_TO_FUNC(xen_plantlight);
LINK_ENTITY_TO_FUNC(xen_spore_large);
LINK_ENTITY_TO_FUNC(xen_spore_medium);
LINK_ENTITY_TO_FUNC(xen_spore_small);
LINK_ENTITY_TO_FUNC(xen_tree);
LINK_ENTITY_TO_FUNC(xen_ttrigger);

// entities for Team Fortress 1.5
LINK_ENTITY_TO_FUNC(building_dispenser);
LINK_ENTITY_TO_FUNC(building_sentrygun);
LINK_ENTITY_TO_FUNC(building_sentrygun_base);
LINK_ENTITY_TO_FUNC(detpack);
LINK_ENTITY_TO_FUNC(dispenser_refill_timer);
LINK_ENTITY_TO_FUNC(func_nobuild);
LINK_ENTITY_TO_FUNC(func_nogrenades);
LINK_ENTITY_TO_FUNC(ghost);
LINK_ENTITY_TO_FUNC(i_p_t);
LINK_ENTITY_TO_FUNC(i_t_g);
LINK_ENTITY_TO_FUNC(i_t_t);
LINK_ENTITY_TO_FUNC(info_areadef);
LINK_ENTITY_TO_FUNC(info_player_teamspawn);
LINK_ENTITY_TO_FUNC(info_tf_teamcheck);
LINK_ENTITY_TO_FUNC(info_tf_teamset);
LINK_ENTITY_TO_FUNC(info_tfdetect);
LINK_ENTITY_TO_FUNC(info_tfgoal);
LINK_ENTITY_TO_FUNC(info_tfgoal_timer);
LINK_ENTITY_TO_FUNC(item_armor1);
LINK_ENTITY_TO_FUNC(item_armor2);
LINK_ENTITY_TO_FUNC(item_armor3);
LINK_ENTITY_TO_FUNC(item_artifact_envirosuit);
LINK_ENTITY_TO_FUNC(item_artifact_invisibility);
LINK_ENTITY_TO_FUNC(item_artifact_invulnerability);
LINK_ENTITY_TO_FUNC(item_artifact_super_damage);
LINK_ENTITY_TO_FUNC(item_cells);
LINK_ENTITY_TO_FUNC(item_health);
LINK_ENTITY_TO_FUNC(item_rockets);
LINK_ENTITY_TO_FUNC(item_shells);
LINK_ENTITY_TO_FUNC(item_spikes);
LINK_ENTITY_TO_FUNC(item_tfgoal);
LINK_ENTITY_TO_FUNC(teledeath);
LINK_ENTITY_TO_FUNC(tf_ammo_rpgclip);
LINK_ENTITY_TO_FUNC(tf_flame);
LINK_ENTITY_TO_FUNC(tf_flamethrower_burst);
LINK_ENTITY_TO_FUNC(tf_gl_grenade);
LINK_ENTITY_TO_FUNC(tf_ic_rocket);
LINK_ENTITY_TO_FUNC(tf_nailgun_nail);
LINK_ENTITY_TO_FUNC(tf_rpg_rocket);
LINK_ENTITY_TO_FUNC(tf_weapon_ac);
LINK_ENTITY_TO_FUNC(tf_weapon_autorifle);
LINK_ENTITY_TO_FUNC(tf_weapon_axe);
LINK_ENTITY_TO_FUNC(tf_weapon_caltrop);
LINK_ENTITY_TO_FUNC(tf_weapon_caltropgrenade);
LINK_ENTITY_TO_FUNC(tf_weapon_concussiongrenade);
LINK_ENTITY_TO_FUNC(tf_weapon_empgrenade);
LINK_ENTITY_TO_FUNC(tf_weapon_flamethrower);
LINK_ENTITY_TO_FUNC(tf_weapon_gasgrenade);
LINK_ENTITY_TO_FUNC(tf_weapon_genericprimedgrenade);
LINK_ENTITY_TO_FUNC(tf_weapon_gl);
LINK_ENTITY_TO_FUNC(tf_weapon_ic);
LINK_ENTITY_TO_FUNC(tf_weapon_knife);
LINK_ENTITY_TO_FUNC(tf_weapon_medikit);
LINK_ENTITY_TO_FUNC(tf_weapon_mirvbomblet);
LINK_ENTITY_TO_FUNC(tf_weapon_mirvgrenade);
LINK_ENTITY_TO_FUNC(tf_weapon_nailgrenade);
LINK_ENTITY_TO_FUNC(tf_weapon_napalmgrenade);
LINK_ENTITY_TO_FUNC(tf_weapon_ng);
LINK_ENTITY_TO_FUNC(tf_weapon_normalgrenade);
LINK_ENTITY_TO_FUNC(tf_weapon_pl);
LINK_ENTITY_TO_FUNC(tf_weapon_railgun);
LINK_ENTITY_TO_FUNC(tf_weapon_rpg);
LINK_ENTITY_TO_FUNC(tf_weapon_shotgun);
LINK_ENTITY_TO_FUNC(tf_weapon_sniperrifle);
LINK_ENTITY_TO_FUNC(tf_weapon_spanner);
LINK_ENTITY_TO_FUNC(tf_weapon_superng);
LINK_ENTITY_TO_FUNC(tf_weapon_supershotgun);
LINK_ENTITY_TO_FUNC(tf_weapon_tranq);
LINK_ENTITY_TO_FUNC(timer);

// entities for Opposing Force					NOT SURE IF THESE ARE NEEDED!
LINK_ENTITY_TO_FUNC(ammo_556);
LINK_ENTITY_TO_FUNC(ammo_762);
LINK_ENTITY_TO_FUNC(ammo_eagleclip);
LINK_ENTITY_TO_FUNC(ammo_spore);
LINK_ENTITY_TO_FUNC(charged_bolt);
LINK_ENTITY_TO_FUNC(displacer_ball);
LINK_ENTITY_TO_FUNC(eagle_laser);
LINK_ENTITY_TO_FUNC(env_blowercannon);
LINK_ENTITY_TO_FUNC(env_electrified_wire);
LINK_ENTITY_TO_FUNC(env_genewormcloud);
LINK_ENTITY_TO_FUNC(env_genewormspawn);
LINK_ENTITY_TO_FUNC(env_rope);
LINK_ENTITY_TO_FUNC(env_spritetrain);
LINK_ENTITY_TO_FUNC(func_op4mortarcontroller);
LINK_ENTITY_TO_FUNC(func_tank_of);
LINK_ENTITY_TO_FUNC(func_tankcontrols_of);
LINK_ENTITY_TO_FUNC(func_tanklaser_of);
LINK_ENTITY_TO_FUNC(func_tankmortar_of);
LINK_ENTITY_TO_FUNC(func_tankrocket_of);
LINK_ENTITY_TO_FUNC(gonomeguts);
LINK_ENTITY_TO_FUNC(grapple_tip);
LINK_ENTITY_TO_FUNC(hvr_blkop_rocket);
LINK_ENTITY_TO_FUNC(info_ctfdetect);
LINK_ENTITY_TO_FUNC(info_ctfspawn);
LINK_ENTITY_TO_FUNC(info_ctfspawn_powerup);
LINK_ENTITY_TO_FUNC(info_displacer_earth_target);
LINK_ENTITY_TO_FUNC(info_displacer_xen_target);
LINK_ENTITY_TO_FUNC(info_pitworm);
LINK_ENTITY_TO_FUNC(info_pitworm_steam_lock);
LINK_ENTITY_TO_FUNC(item_ctfaccelerator);
LINK_ENTITY_TO_FUNC(item_ctfbackpack);
LINK_ENTITY_TO_FUNC(item_ctfbase);
LINK_ENTITY_TO_FUNC(item_ctfflag);
LINK_ENTITY_TO_FUNC(item_ctflongjump);
LINK_ENTITY_TO_FUNC(item_ctfportablehev);
LINK_ENTITY_TO_FUNC(item_ctfregeneration);
LINK_ENTITY_TO_FUNC(item_generic);
LINK_ENTITY_TO_FUNC(item_nuclearbomb);
LINK_ENTITY_TO_FUNC(item_nuclearbombbutton);
LINK_ENTITY_TO_FUNC(item_nuclearbombtimer);
LINK_ENTITY_TO_FUNC(item_vest);
LINK_ENTITY_TO_FUNC(monster_ShockTrooper_dead);
LINK_ENTITY_TO_FUNC(monster_alien_babyvoltigore);
LINK_ENTITY_TO_FUNC(monster_alien_slave_dead);
LINK_ENTITY_TO_FUNC(monster_alien_voltigore);
LINK_ENTITY_TO_FUNC(monster_assassin_repel);
LINK_ENTITY_TO_FUNC(monster_blkop_apache);
LINK_ENTITY_TO_FUNC(monster_blkop_osprey);
LINK_ENTITY_TO_FUNC(monster_cleansuit_scientist);
LINK_ENTITY_TO_FUNC(monster_cleansuit_scientist_dead);
LINK_ENTITY_TO_FUNC(monster_drillsergeant);
LINK_ENTITY_TO_FUNC(monster_fgrunt_repel);
LINK_ENTITY_TO_FUNC(monster_geneworm);
LINK_ENTITY_TO_FUNC(monster_gonome);
LINK_ENTITY_TO_FUNC(monster_gonome_dead);
LINK_ENTITY_TO_FUNC(monster_grunt_ally_repel);
LINK_ENTITY_TO_FUNC(monster_hfgrunt_dead);
LINK_ENTITY_TO_FUNC(monster_houndeye_dead);
LINK_ENTITY_TO_FUNC(monster_human_friendly_grunt);
LINK_ENTITY_TO_FUNC(monster_human_grunt_ally);
LINK_ENTITY_TO_FUNC(monster_human_grunt_ally_dead);
LINK_ENTITY_TO_FUNC(monster_human_medic_ally);
LINK_ENTITY_TO_FUNC(monster_human_torch_ally);
LINK_ENTITY_TO_FUNC(monster_male_assassin);
LINK_ENTITY_TO_FUNC(monster_massassin_dead);
LINK_ENTITY_TO_FUNC(monster_medic_ally_repel);
LINK_ENTITY_TO_FUNC(monster_op4loader);
LINK_ENTITY_TO_FUNC(monster_otis);
LINK_ENTITY_TO_FUNC(monster_otis_dead);
LINK_ENTITY_TO_FUNC(monster_penguin);
LINK_ENTITY_TO_FUNC(monster_pitdrone);
LINK_ENTITY_TO_FUNC(monster_pitworm);
LINK_ENTITY_TO_FUNC(monster_pitworm_up);
LINK_ENTITY_TO_FUNC(monster_recruit);
LINK_ENTITY_TO_FUNC(monster_shockroach);
LINK_ENTITY_TO_FUNC(monster_shocktrooper);
LINK_ENTITY_TO_FUNC(monster_shocktrooper_repel);
LINK_ENTITY_TO_FUNC(monster_sitting_cleansuit_scientist);
LINK_ENTITY_TO_FUNC(monster_skeleton_dead);
LINK_ENTITY_TO_FUNC(monster_torch_ally_repel);
LINK_ENTITY_TO_FUNC(monster_zombie_barney);
LINK_ENTITY_TO_FUNC(monster_zombie_soldier);
LINK_ENTITY_TO_FUNC(monster_zombie_soldier_dead);
LINK_ENTITY_TO_FUNC(mortar_shell);
LINK_ENTITY_TO_FUNC(op4mortar);
LINK_ENTITY_TO_FUNC(pitdronespike);
LINK_ENTITY_TO_FUNC(pitworm_gib);
LINK_ENTITY_TO_FUNC(pitworm_gibshooter);
LINK_ENTITY_TO_FUNC(rope_sample);
LINK_ENTITY_TO_FUNC(rope_segment);
LINK_ENTITY_TO_FUNC(shock_beam);
LINK_ENTITY_TO_FUNC(spore);
LINK_ENTITY_TO_FUNC(trigger_ctfgeneric);
LINK_ENTITY_TO_FUNC(trigger_geneworm_hit);
LINK_ENTITY_TO_FUNC(trigger_kill_nogib);
LINK_ENTITY_TO_FUNC(trigger_playerfreeze);
LINK_ENTITY_TO_FUNC(trigger_xen_return);
LINK_ENTITY_TO_FUNC(weapon_displacer);
LINK_ENTITY_TO_FUNC(weapon_eagle);
LINK_ENTITY_TO_FUNC(weapon_grapple);
LINK_ENTITY_TO_FUNC(weapon_penguin);
LINK_ENTITY_TO_FUNC(weapon_pipewrench);
LINK_ENTITY_TO_FUNC(weapon_shockrifle);
LINK_ENTITY_TO_FUNC(weapon_shockroach);
LINK_ENTITY_TO_FUNC(weapon_sniperrifle);
LINK_ENTITY_TO_FUNC(weapon_sporelauncher);

// entities for FireArms 2.6

//ammo
LINK_ENTITY_TO_FUNC(ammo_ak47);
LINK_ENTITY_TO_FUNC(ammo_ak74);
LINK_ENTITY_TO_FUNC(ammo_anaconda);
LINK_ENTITY_TO_FUNC(ammo_benelli);
LINK_ENTITY_TO_FUNC(ammo_ber92f);
LINK_ENTITY_TO_FUNC(ammo_ber93r);
LINK_ENTITY_TO_FUNC(ammo_bizon);
LINK_ENTITY_TO_FUNC(ammo_coltgov);
LINK_ENTITY_TO_FUNC(ammo_concussion);
LINK_ENTITY_TO_FUNC(ammo_desert);
#ifndef NOFAMAS
LINK_ENTITY_TO_FUNC(ammo_famas);
#endif
LINK_ENTITY_TO_FUNC(ammo_frag);
LINK_ENTITY_TO_FUNC(ammo_g36e);
LINK_ENTITY_TO_FUNC(ammo_g3a3);
LINK_ENTITY_TO_FUNC(ammo_gp25);
LINK_ENTITY_TO_FUNC(ammo_m16);
LINK_ENTITY_TO_FUNC(ammo_m203);
LINK_ENTITY_TO_FUNC(ammo_m60);
LINK_ENTITY_TO_FUNC(ammo_m79);
LINK_ENTITY_TO_FUNC(ammo_m82);
LINK_ENTITY_TO_FUNC(ammo_mp5a5);
LINK_ENTITY_TO_FUNC(ammo_pkm);
LINK_ENTITY_TO_FUNC(ammo_saiga);
LINK_ENTITY_TO_FUNC(ammo_ssg3000);
LINK_ENTITY_TO_FUNC(ammo_sterling);
LINK_ENTITY_TO_FUNC(ammo_stg24);
LINK_ENTITY_TO_FUNC(ammo_svd);
LINK_ENTITY_TO_FUNC(ammo_uzi);
//weapons
LINK_ENTITY_TO_FUNC(weapon_ak47);
LINK_ENTITY_TO_FUNC(weapon_ak74);
LINK_ENTITY_TO_FUNC(weapon_anaconda);
LINK_ENTITY_TO_FUNC(weapon_benelli);
LINK_ENTITY_TO_FUNC(weapon_ber92f);
LINK_ENTITY_TO_FUNC(weapon_ber93r);
LINK_ENTITY_TO_FUNC(weapon_bizon);
LINK_ENTITY_TO_FUNC(weapon_claymore);
LINK_ENTITY_TO_FUNC(weapon_coltgov);
LINK_ENTITY_TO_FUNC(weapon_concussion);
LINK_ENTITY_TO_FUNC(weapon_desert);
#ifndef NOFAMAS
LINK_ENTITY_TO_FUNC(weapon_famas);
#endif
LINK_ENTITY_TO_FUNC(weapon_frag);
LINK_ENTITY_TO_FUNC(weapon_g36e);
LINK_ENTITY_TO_FUNC(weapon_g3a3);
LINK_ENTITY_TO_FUNC(weapon_knife);
LINK_ENTITY_TO_FUNC(weapon_m16);
LINK_ENTITY_TO_FUNC(weapon_m60);
LINK_ENTITY_TO_FUNC(weapon_m79);
LINK_ENTITY_TO_FUNC(weapon_m82);
LINK_ENTITY_TO_FUNC(weapon_mp5a5);
LINK_ENTITY_TO_FUNC(weapon_pkm);
LINK_ENTITY_TO_FUNC(weapon_saiga);
LINK_ENTITY_TO_FUNC(weapon_ssg3000);
LINK_ENTITY_TO_FUNC(weapon_sterling);
LINK_ENTITY_TO_FUNC(weapon_stg24);
LINK_ENTITY_TO_FUNC(weapon_svd);
LINK_ENTITY_TO_FUNC(weapon_uzi);
//items
LINK_ENTITY_TO_FUNC(item_bandage);
LINK_ENTITY_TO_FUNC(item_claymore);
LINK_ENTITY_TO_FUNC(item_concussion);
LINK_ENTITY_TO_FUNC(item_frag);
LINK_ENTITY_TO_FUNC(item_stg24);
//mics
LINK_ENTITY_TO_FUNC(ammobox);
LINK_ENTITY_TO_FUNC(fa_drop_zone);
LINK_ENTITY_TO_FUNC(fa_parachute);
LINK_ENTITY_TO_FUNC(fa_push_flag);
LINK_ENTITY_TO_FUNC(fa_push_point);
LINK_ENTITY_TO_FUNC(fa_sd_object);
LINK_ENTITY_TO_FUNC(fa_team_goal);
LINK_ENTITY_TO_FUNC(fa_team_item);
LINK_ENTITY_TO_FUNC(hospital);
LINK_ENTITY_TO_FUNC(info_firearms_detect);
LINK_ENTITY_TO_FUNC(info_playerstart_blue);
LINK_ENTITY_TO_FUNC(info_playerstart_red);
LINK_ENTITY_TO_FUNC(marker);
LINK_ENTITY_TO_FUNC(mortarshell);
LINK_ENTITY_TO_FUNC(weapmortar);

// probably new entities for FireArms 2.65
LINK_ENTITY_TO_FUNC(bullet);
LINK_ENTITY_TO_FUNC(env_spriteshooter);
LINK_ENTITY_TO_FUNC(medevac);
LINK_ENTITY_TO_FUNC(player_helmet);
LINK_ENTITY_TO_FUNC(trigger_zone);
LINK_ENTITY_TO_FUNC(vote_llama);

// new entities for FireArms 2.7
LINK_ENTITY_TO_FUNC(ammo_m249);
LINK_ENTITY_TO_FUNC(weapon_m249);
LINK_ENTITY_TO_FUNC(env_fog);

// new entities for FireArms 2.8
LINK_ENTITY_TO_FUNC(ammo_remington);
LINK_ENTITY_TO_FUNC(env_precip);
LINK_ENTITY_TO_FUNC(env_sun);
LINK_ENTITY_TO_FUNC(item_flashlight);
LINK_ENTITY_TO_FUNC(item_nvg);
LINK_ENTITY_TO_FUNC(weapon_remington);

// new entities for FireArms 2.9
LINK_ENTITY_TO_FUNC(ammo_m14);
LINK_ENTITY_TO_FUNC(ammo_m4);
LINK_ENTITY_TO_FUNC(weapon_m14);
LINK_ENTITY_TO_FUNC(weapon_m4);
LINK_ENTITY_TO_FUNC(trigger_noclaymores);
LINK_ENTITY_TO_FUNC(trigger_reinforcements);

// entities for FireArms 2.5
LINK_ENTITY_TO_FUNC(item_flashbang);
LINK_ENTITY_TO_FUNC(weapon_flashbang);
LINK_ENTITY_TO_FUNC(weapon_mc51);
LINK_ENTITY_TO_FUNC(weapon_m11);
LINK_ENTITY_TO_FUNC(weapon_psg1);
LINK_ENTITY_TO_FUNC(weapon_aug);
LINK_ENTITY_TO_FUNC(ammo_flashbang);
LINK_ENTITY_TO_FUNC(ammo_mc51);
LINK_ENTITY_TO_FUNC(ammo_m11);
LINK_ENTITY_TO_FUNC(ammo_psg1);
LINK_ENTITY_TO_FUNC(ammo_aug);
LINK_ENTITY_TO_FUNC(ammo_m112);

// entities for FireArms 2.4
LINK_ENTITY_TO_FUNC(ammo_mp5k);
LINK_ENTITY_TO_FUNC(weapon_mp5k);
