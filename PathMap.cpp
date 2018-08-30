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
// PathMap.cpp
// 
////////////////////////////////////////////////////////////////////////////////////////////////

#include "PathMap.h"
#include <stdio.h>

PathMap::PathMap()
{

}

PathMap::~PathMap()
{

}

/*
* add new path for brain analyzer.
*/
void PathMap::add(int from, int to, int weight, 
                  int from_flags, int from_border,
                  int to_flags, int to_border) {
	if (from < 0) return;
	Info info;
	//this is not quite average, but it is simpler.
	(*this)[from][to].weight_own += weight;
	(*this)[from].flags = from_flags;
	(*this)[from].border = from_border;
	//add reverse path with original weight too.
	//We aren't sure this path is reliable, that is why
	//we only add weight once.
	(*this)[to][from].weight_own = weight;
	(*this)[to].flags = to_flags;
	(*this)[to].border = to_border;
}

/*
* Load pathmap for brain analyzer from presaved/stored file.
*/
bool PathMap::load(FILE *f) {
   if (f==NULL) return false;
   
   int i, from, to, weight;
   int from_flags, from_border;
   
   while ((i = fscanf(f, "from=%d to=%d num=%d | flags=%d border=%d\n",
		&from, &to, &weight, &from_flags, &from_border))!=EOF) {
	if (from < 0) continue;
	Info info;
	(*this)[from][to].weight_own = weight;
	(*this)[from].flags = from_flags;
	(*this)[from].border = from_border;
   }
   return true;
}


void PathMap::save(const string file_name) const {
	FILE *path_f = fopen(file_name.c_str(), "w+");
	if (path_f == NULL) {
		return;
	}

	for (PathMap_CI pm_ci = begin(); pm_ci != end(); ++pm_ci) {
		for (NeighbourMap_CI nbm_ci = pm_ci->second.begin();
				nbm_ci != pm_ci->second.end(); ++nbm_ci) {
			fprintf(path_f, "from=%d to=%d num=%d | flags=%d border=%d\n", 
				pm_ci->first, nbm_ci->first, nbm_ci->second.weight_own,
				pm_ci->second.flags, pm_ci->second.border);
		}
	}
	fclose(path_f);
}

/*
Find the easiest way to the wpt with parameters in "to"
This way can be not the shortest (sometimes).
You can deside this by functor "to"
*/
int PathMap::findPath(const int from, const equalWpt &to,   PointList *pl, const int steps) {
    static int ticket = 1;
    ++ticket;
    
    NeighbourMap round0, round1;
    NeighbourMap *cur_round=&round0, *next_round=&round1, *tmp_round;
    (*cur_round)[from];
    (*cur_round)[from];
    (*this)[from].parent=-1;
    (*this)[from].ticket=ticket;
    (*this)[from].weight=1;
    
    for (int i=0; i<steps; ++i) {
	next_round->clear();
	for (NeighbourMap_I pl_i = cur_round->begin();
		pl_i != cur_round->end(); ++pl_i) {
	    if (to(pl_i->first, (*this)[pl_i->first])) { //WE FOUND THE WAY!!!
		//create chance (point way).
		if (pl!=NULL) {
		    pl->push_front(pl_i->first);
		}
		int cur_num = pl_i->first;
		int right_way=pl_i->first;
		while ((*this)[cur_num].parent != -1) {
		    right_way = cur_num;
		    cur_num = (*this)[cur_num].parent;
		    if (pl!=NULL) {
			pl->push_front(cur_num);
		    }
		}
		return right_way;
	    }
	    //find neighbourgs of current border.
	    //and push their into border for next round.
	    //@@@@ cerr<<"look point " << pl_i->first << " weight=" 
	    //<< (*this)[pl_i->first].weight << endl;
	    
	    for (NeighbourMap_I nm_i = (*this)[pl_i->first].begin();
			nm_i != (*this)[pl_i->first].end(); ++nm_i) {
		//number of steps is prefered, but weight is important too.
		//+1.0 - we add step, 1.0/weight_own - weight of current step.
		double new_weight = (*this)[pl_i->first].weight + 1.0 + 1.0/nm_i->second.weight_own;
		if ((*this)[nm_i->first].ticket != ticket) { /*don't use wpt before*/
			(*next_round)[nm_i->first];
			(*this)[nm_i->first].weight = new_weight;
			(*this)[nm_i->first].parent = pl_i->first;
			(*this)[nm_i->first].ticket = ticket;
		}
		else if (new_weight < (*this)[nm_i->first].weight) {
			(*next_round)[nm_i->first];
			(*this)[nm_i->first].weight = new_weight;
			(*this)[nm_i->first].parent = pl_i->first;
			//cerr << "overload" <<endl;
		}
	    }	    
	}
	//we create new border, make it current.
	tmp_round = cur_round;
	cur_round = next_round;
	next_round= tmp_round;
    }
    return -1;
}

TeamPathMap teamPathMap;
