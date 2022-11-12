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
// bot_manager.cpp
// 
////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(WIN32)
#pragma warning(disable: 4005)
#endif

#include "bot_manager.h"

client_t::client_t()
{
	pEntity = nullptr;
	client_is_human = false;
	client_bleeds = false;
	max_speed_time = -1.0;
}

// set all botmanager variables to defaults
botmanager_t::botmanager_t()
{
	ResetTeamsBalanceNeeded();
	ResetOverrideTeamsBalance();
	ResetTimeOfTeamsBalanceCheck();
	ResetTeamsBalanceValue();
	ResetListenServerFilling();
	ResetBotCheckTime();
	ResetBotsToBeAdded();
}

// set all botdebugger variables to default values
botdebugger_t::botdebugger_t()
{
	ResetObserverMode();
	ResetFreezeMode();
	ResetDontShoot();
	ResetIgnoreAll();
	ResetDebugAims();
	ResetDebugActions();
	ResetDebugCross();
	ResetDebugPaths();
	ResetDebugStuck();
	ResetDebugWaypoints();
	ResetDebugWeapons();
}

// what we do here is that we set all external variables to their default values
externals_t::externals_t()
{
	ResetIsLogging();
	ResetRandomSkill();
	ResetSpawnSkill();
	ResetReactionTime();
	ResetBalanceTime();
	ResetMinBots();
	ResetMaxBots();
	ResetCustomClasses();
	ResetInfoTime();
	ResetPresentationTime();
	ResetDontSpeak();
	ResetDontChat();
	ResetRichNames();
	ResetPassiveHealing();
}

// what we do here is that we set all internal variables to their default values
internals_t::internals_t()
{
	ResetIsEnemyDistanceLimit();
	ResetEnemyDistanceLimit();
	ResetHUDMessageTime();
	ResetTeamPlay();
	ResetTeamPlayChecked();
	ResetFASkillSystem();
	ResetSkillSystemChecked();
	ResetIsCustomWaypoints();
	ResetWaypointsAutoSave();
	ResetPathToContinue();
}

// allows setting defaults on map change
void internals_t::ResetOnMapChange()
{
	ResetIsEnemyDistanceLimit();
	ResetEnemyDistanceLimit();
	ResetHUDMessageTime();
	ResetTeamPlay();
	ResetTeamPlayChecked();
	ResetFASkillSystem();
	ResetSkillSystemChecked();
	ResetIsCustomWaypoints();
	ResetWaypointsAutoSave();
	ResetPathToContinue();
}