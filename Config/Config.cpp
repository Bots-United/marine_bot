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
// Config.cpp
// 
////////////////////////////////////////////////////////////////////////////////////////////////

#include <cstdio>
#include <iostream>
#include "Config.h"
#include "config.tab.hpp"

using namespace std;

//extern FILE *conf_in;

std::string Section :: getVal(const std::string key) const {
	const CII ii = item.find(key);
    if (ii == item.end()) {
       throw errGetVal(key);
    }
    else {
       return ii->second;
    }
};


Section * parceConfig(char *confFileName) {
    if (confFileName == nullptr) {
	conf_in = stdin;
    }
    else {
	conf_in = fopen(confFileName, "r");
    }
    if (conf_in == nullptr) {
	return nullptr;
    }

    Section *conf_p = new Section;
    
    ParserParam pp;
    ConfEntry ce;
    ce.f = conf_in;
    ce.f_name = confFileName;
    ce.sect = conf_p;
    pp.confEntryQueue.push(ce);
    pp.root_p = conf_p;

    try {
      while (!pp.confEntryQueue.empty()) {
	ce = pp.confEntryQueue.front();
	pp.confEntryQueue.pop();
	conf_in = ce.f;
	pp.sect_stack.push(ce.sect);
	if (conf_parse((void*) &pp)!=0) {
	    printf(" in file %s\n", ce.f_name.c_str());
	    delete conf_p;
	    conf_p = nullptr;
	    fclose(conf_in);
	    break;
	}
	fclose(conf_in);
      }
    } catch(...) {
	if (conf_p != nullptr) delete conf_p;
	conf_p = nullptr;
	fclose(conf_in);
    }
    return conf_p;
}

Section :: ~Section() {
    for (SI p = sectionList.begin();
	    p != sectionList.end(); 
	    ++p) {
	delete (p->second);
    }
}

void printSection(Section *sect, int level) {
    typedef map<string,Section *>::const_iterator CI;
	int i=0;
    for (CI p = sect->sectionList.begin();
	    p != sect->sectionList.end(); 
	    ++p) {
		for (i=0; i<level; i++) {
			cout << "   ";
		}
		std::cout << p->first << " { " << std::endl;
		printSection(p->second, level+1);
		for (i=0; i<level; i++) {
             std::cout << "   ";
        }
		std::cout << "}" << std::endl;
    }
    
    typedef map<string, string>::const_iterator CIV;
    for (CIV pi = sect->item.begin();
			pi != sect->item.end(); 
			++pi) {
		for (i=0; i<level; i++) {
			cout << "   ";
		}
		cout << pi->first << " = \"" << pi->second << "\";" << endl;
    }
};
