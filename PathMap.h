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
// PathMap.h
// 
////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef PATHMAP_H 
#define PATHMAP_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#if defined(WIN32)
#pragma warning(disable:4786)
#endif

#include <cstdio>
#include <map>
#include <string>
#include <list>
#include <iostream>

//#include "extdll.h"
//#include "bot.h"
//#include "waypoint.h"
using namespace std;

typedef list <int> PointList;
typedef PointList::iterator PointList_I;


class Info {
    public:
        Info() {
			weight_own = 1;
		};

        int weight_own; //dynamic weight of this path.
		//danger and difficulties will decrease this value.
};

class NeighbourMap : public map<const int, Info> {
   public:
	NeighbourMap() {
		ticket = -1;
		weight = 0;
		parent = -1;
	};
	int ticket;
	double weight;
	int parent;
	
	int   flags;
	int   border;
};
typedef NeighbourMap::iterator NeighbourMap_I;
typedef NeighbourMap::const_iterator NeighbourMap_CI;

class equalWpt {
private:
	int to;
public:
	equalWpt(int wpt) { to = wpt; };
	inline bool operator()(const int ind, const NeighbourMap &nm) const {
	   if(ind == to) {
	      return true;
	   }
	   else {
	      return false;
	   }
	};
};

/*
Functor for finding any special waypoint like sniper wpt, bandage or ammo wpt.
Use W_FL_BANDAGES, W_FL_AMMOBOX and etc as flags in constructor.
See waypoint.h for wpts flags.
*/
class equalSpcWpt : public equalWpt {
private:
	int border;
	int flags;
public:
	equalSpcWpt(int find_flags, int find_border=-1):equalWpt(-1) { 
		border = find_border; flags = find_flags;
	};
	inline bool operator()(const int ind, const NeighbourMap &nm)  const {
	   if (nm.flags & flags) {
		if (border != -1 && border!=nm.border) {
		   return false;
		}
		return true;
	   }
	   else {
	      return false;
	   }
	};
};

class PathMap : public map <int, NeighbourMap>
{
public:
	PathMap();
	virtual ~PathMap();
	virtual void add(const int from, const int to, const int weight, 
				int from_flags=0, int from_border=-1,
                                int to_flags=0, int to_border=-1);
	virtual void save(const string file_name = "unknown.map_path") const;
	virtual bool load(FILE *f);
	int findPath(const int from, const equalWpt &to,   PointList *pl=NULL, const int steps=8);
};

typedef PathMap::iterator PathMap_I;
typedef PathMap::const_iterator PathMap_CI;

typedef map<const int, PathMap> TeamPathMap;
typedef TeamPathMap::iterator TeamPathMap_I;
typedef TeamPathMap::const_iterator TeamPathMap_CI;

extern TeamPathMap teamPathMap;

#endif 
