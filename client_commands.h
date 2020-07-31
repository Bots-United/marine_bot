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
// client_commands.h
// 
////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

bool CustomClientCommands(edict_t* pEntity, const char* pcmd, const char* arg1, const char* arg2, const char* arg3, const char* arg4, const char* arg5);
void PlaySoundConfirmation(edict_t* pEntity, int msg_type);
void MBMenuSystem(edict_t* pEntity, const char* arg1);
void RandomSkillCommand(edict_t* pEntity, const char* arg1);
void SpawnSkillCommand(edict_t* pEntity, const char* arg1);
void SetBotSkillCommand(edict_t* pEntity, const char* arg1);
void BotSkillUpCommand(edict_t* pEntity, const char* arg1);
void BotSkillDownCommand(edict_t* pEntity, const char* arg1);
void SetAimSkillCommand(edict_t* pEntity, const char* arg1);
void SetReactionTimeCommand(edict_t* pEntity, const char* arg1);
void RangeLimitCommand(edict_t* pEntity, const char* arg1);
void SetWaypointDirectoryCommand(edict_t* pEntity, const char* arg1);
void PassiveHealingCommand(edict_t* pEntity, const char* arg1);
void DontSpeakCommand(edict_t* pEntity, const char* arg1);
void DontChatCommand(edict_t* pEntity, const char* arg1);