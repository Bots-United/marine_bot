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
// bot_func.h
// 
////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef BOT_FUNC_H
#define BOT_FUNC_H

#include "Config.h"

// for new config system by Andrey Kotrekhov
class SetupBaseType_float : public SetupBaseType
{
    private:
		float &val;
    public:
		SetupBaseType_float(float &c_val) : val(c_val) {}
		void set(std::string &str_val)
		{
			val = float(strtod(str_val.c_str(), NULL));
		}
};

class SetupBaseType_team : public SetupBaseType
{
    private:
		std::string &val;
    public:
		SetupBaseType_team(std::string &c_val) : val(c_val) {}
		void set(std::string &str_val)
		{
			if (str_val == "red")
			{
				val = "1";
			}
			else
			{
				val = "2";
			}
		}
};

//prototypes of bot functions
void BotNameInit(void);
bool BotCreate( edict_t *pPlayer, const char *arg1, const char *arg2,
                const char *arg3, const char *arg4, const char *arg5 );

// new BotCreate() prototype for new config system by Andrey Kotrekhov
#include <string>
bool BotCreate( edict_t *pPlayer, const std::string &s_arg1, const std::string &s_arg2,
			   const std::string &s_arg3, const std::string &s_arg4, const std::string &s_arg5 );

void BotFixIdealPitch( edict_t *pEdict );
float BotChangePitch( bot_t *pBot, float speed );
void BotFixIdealYaw( edict_t *pEdict );
float BotChangeYaw( bot_t *pBot, float speed );
//bool BotFindWaypoint( bot_t *pBot );
bool BotHeadTowardWaypoint( bot_t *pBot );
//void BotOnLadder( bot_t *pBot, float moved_distance );
void BotUnderWater( bot_t *pBot );
//void BotUseLift( bot_t *pBot, float moved_distance );
bool BotStuckInCorner( bot_t *pBot );
void BotTurnAtWall( bot_t *pBot, TraceResult *tr );
bool BotCantMoveForward( bot_t *pBot, TraceResult *tr );
bool IsForwardBlocked(bot_t *pBot);
bool IsDeathFall(edict_t *pEdict);
bool BotCanJumpUp( bot_t *pBot );
bool BotCanDuckJumpUp( bot_t *pBot );
bool BotCanDuckUnder( bot_t *pBot );
void BotRandomTurn( bot_t *pBot );
bool BotCheckWallOnLeft( bot_t *pBot );
bool BotCheckWallOnRight( bot_t *pBot );

bool InitFAWeapons(void);
void BotWeaponArraysInit(Section *conf_weapons);
//edict_t *BotFindEnemy( bot_t *pBot );
void BotShootAtEnemy( bot_t *pBot );

int BotChooseCorrectSkill(bot_t *pBot);

bool BotFindWaypoint(bot_t *pBot, bool ladder);
bool BotHandleLadder(bot_t *pBot, float moved_distance);
bool BotCantStrafeLeft(edict_t *pEdict);
bool BotCantStrafeRight(edict_t *pEdict);
bool BotFollowTeammate(bot_t *pBot);
bool BotMedicGetToPatient(bot_t *pBot);
bool BotMedicGetToCorpse(bot_t *pBot, float &distance);
void BotEvadeClaymore(bot_t *pBot);

Vector BotBodyTarget(bot_t *pBot);
void BotPlantClaymore(bot_t *pBot);


// util.cpp functions
edict_t *UTIL_FindEntityInSphere( edict_t *pentStart, const Vector &vecCenter, float flRadius );
edict_t *UTIL_FindEntityInSphere(edict_t *pCallerEdict, const char *ent_name);
//edict_t *UTIL_FindEntityByString( edict_t *pentStart, const char *szKeyword, const char *szValue );	// used only locally in util.cpp
edict_t *UTIL_FindEntityByClassname( edict_t *pentStart, const char *szName );
//edict_t *UTIL_FindEntityByTargetname( edict_t *pentStart, const char *szName );	// NOT USED
void ClientPrint( edict_t *pEdict, int msg_dest, const char *msg_name);
//void UTIL_SayText( const char *pText, edict_t *pEdict );				// NOT USED
//void UTIL_HostSay( edict_t *pEntity, int teamonly, char *message );	// NOT USED
int UTIL_GetTeam(edict_t *pEntity);
int UTIL_GetBotIndex(edict_t *pEdict);
//bot_t *UTIL_GetBotPointer(edict_t *pEdict);							// NOT USED
bool IsAlive(edict_t *pEdict);
bool FInViewCone(Vector *pOrigin, edict_t *pEdict);
bool FInNarrowViewCone(Vector *pOrigin, edict_t *pEdict, float cone = 0.8);
bool FVisible(const Vector &vecOrigin, edict_t *pEdict, bool ignore_water);
int FPlayerVisible( const Vector &vecOrigin, edict_t *pEdict );
bool FVisible( const Vector &vecOrigin, edict_t *pEdict );
int FPlayerVisible( const Vector &vecOrigin, const Vector &vecLookerOrigin, edict_t *pEdict );
Vector GetGunPosition(edict_t *pEdict);
Vector VecBModelOrigin(edict_t *pEdict);
int InFieldOfView(edict_t *pEdict, const Vector &dest);
bool UTIL_IsNewerVersion(void);
bool UTIL_IsOldVersion(void);
void UTIL_SetBit(int the_bit, int &bit_map);
void UTIL_ClearBit(int the_bit, int &bit_map);
bool UTIL_IsBitSet(int the_bit, int bit_map);
void UTIL_ShowMenu( edict_t *pEdict, int slots, int displaytime, bool needmore, char *pText );
void UTIL_MarineBotFileName(char *filename, char *arg1, char *arg2);
void UTIL_MBLogPrint(char *message, ...);
//int ForwardTrace(const Vector &vecOrigin, edict_t *pEdict);	// NOT USED
void UTIL_Radio(edict_t *pEdict, char *word);
void UTIL_Voice(edict_t *pEdict, char *word);
void UTIL_Say(edict_t *pEdict, char *message);
void UTIL_TeamSay(edict_t *pEdict, char *message);
bool UTIL_IsAssaultRifle(int weapon);
bool UTIL_IsSniperRifle(int weapon);
bool UTIL_IsMachinegun(int weapon);
bool UTIL_IsSMG(int weapon);
bool UTIL_IsShotgun(int weapon);
bool UTIL_IsHandgun(int weapon);
bool IsBipodWeapon(int weapon);
void UTIL_ChangeWeapon(bot_t *pBot);
void UTIL_ChangeFireMode(bot_t *pBot, int new_mode, CHANGE_FM_TEST wtest);
void UTIL_ShowWeapon(bot_t *pBot);
void UTIL_CheckAmmoReserves(bot_t *pBot);
int UTIL_FindBotByName(const char *name_string);
bool UTIL_KickBot(int which_one);
bool UTIL_KillBot(int which_one);
int UTIL_CountPlayers(int team);
int UTIL_BalanceTeams();
int UTIL_DoTheBalance();
int UTIL_ChangeBotSkillLevel(bool by_one, int skill_level);
int UTIL_ChangeAimSkillLevel(int skill_level);
//int UTIL_GetIDFromName(const char *weapon_name);		// NOT USED
bool UTIL_IsStealth(edict_t *pEdict);
bool UTIL_CanMountSilencer(bot_t *pBot);
bool UTIL_GoProne(bot_t *pBot);
void SetStanceByte(bot_t *pBot, int flag);
bool UTIL_HeardIt(bot_t *pBot, edict_t *pInvoker, float range);
int UTIL_RemoveFalsePaths(bool print_details);
int UTIL_CheckPathsForProblems(bool log_in_file);
void UTIL_RepairWaypointRangeandPosition(edict_t *pEdict);
void UTIL_RepairWaypointRangeandPosition(edict_t *pEdict, bool dont_move);
bool UTIL_RepairWaypointRangeandPosition(int wpt_index, edict_t *pEdict);
bool UTIL_RepairWaypointRangeandPosition(int wpt_index, edict_t *pEdict, bool dont_move);
//float UTIL_IsInRange(edict_t *pEdict, int wpt_index);	// NOT USED
bool UTIL_IsSmallRange(int wpt_index);
bool UTIL_IsDontMoveWpt(edict_t *pEdict, int wpt_index, bool passed);
int UTIL_GetLadderDir(bot_t *pBot);
bool UTIL_CheckForwardForBreakable(edict_t *pEdict);
bool UTIL_CheckForBreakableAround(bot_t *pBot);
int UTIL_DoWaypointAction(bot_t *pBot);
bool UTIL_IsAnyMedic(bot_t *pBot, edict_t *pWounded, bool passive);
bool UTIL_CanMedEvac(bot_t *pBot, edict_t *pWounded);
bool UTIL_PatientNeedsTreatment(edict_t *pPatient);
void UTIL_HumanizeTheName(const char *original_name, char *name);
bool IsOfficialWpt(const char *waypointer);
void PrintOutput(edict_t *pEdict, char *message, MESSAGE_TYPE msg_type);
void PrintOutput(edict_t *pEdict, char *message, MESSAGE_TYPE msg_type = msg_null);
void EchoConsole(const char *message);
void HudNotify(char *msg);
void HudNotify(char *msg, bool islogging);
void UTIL_HighlightTrace(Vector v_source, Vector v_dest, edict_t *pEdict);
void UTIL_DebugInFile(char *msg);
void UTIL_DebugDev(char *msg, int wpt = -100, int path = -100);

#ifdef _DEBUG
void UTIL_DumpEdictToFile(edict_t *pEdict);
#endif


// waypoint.cpp prototypes
bool WaypointMatchingTriggerMessage(char *the_text);
void UpdateWaypointData(void);


// mb_hud prototypes
static unsigned short FixedUnsigned16( float value, float scale );
static short FixedSigned16( float value, float scale );

void FullCustHudMessage(edict_t *pEntity, const char *msg_name, int channel, int pos_x, int pos_y, int gfx, Vector color1, Vector color2, int brightness, float fade_in, float fade_out, float duration);
void StdHudMessage(edict_t *pEntity, const char *msg_name, int gfx, int time);
void StdHudMessageToAll(const char *msg_name, int gfx, int time);
void CustHudMessage(edict_t *pEntity, const char *msg_name, Vector color1, Vector color2, int gfx, int time);
void CustHudMessageToAll(const char *msg_name, Vector color1, Vector color2, int gfx, int time);
void DisplayMsg(edict_t *pEntity, const char *msg);
void CustDisplayMsg(edict_t *pEntity, const char *msg, int x_pos, float time);

#ifdef _DEBUG
void DrawBeam(edict_t *pEntity, Vector start, Vector end, int life, int red, int green, int blue, int speed);
void ReDrawBeam(edict_t *pEdict, Vector start, Vector end, int team);
#endif

#endif // BOT_FUNC_H

