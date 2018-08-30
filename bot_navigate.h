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
// bot_navigate.h
// 
////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef BOT_NAVIGATE_H
#define BOT_NAVIGATE_H

#include <map>
#include <list>
#include <string>

#include "bot.h"

typedef bool (*FUN)(bot_t *pBot);
struct NaviPair {
	NaviPair() { ptr = NULL; }
	std::string name;
	FUN ptr;
};

typedef std::map<std::string,FUN> NavigationMethods;
typedef std::list<NaviPair> NavigationList;


extern NavigationMethods NMethodsDict;
extern NavigationList NMethodsList;

bool BotOnPath1(bot_t *pBot);
int BotOnPath(bot_t *pBot);

#endif // BOT_NAVIGATE_H
