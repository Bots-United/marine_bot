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
// Config.h
// 
////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef G__CONFIG_H
#define G__CONFIG_H

#if defined(WIN32)
#pragma warning(disable:4786)
#endif

extern "C" {
#  include <stdlib.h>
#  include <stdio.h>
};
#include <cstdio>
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <queue>

class errGetVal {
   public:
      std::string key;
      errGetVal(const std::string err_key) {
         key = err_key;
      };
      virtual ~errGetVal() {};
};

extern FILE *conf_in;

class Section;
typedef std::map<std::string, Section *> SectionList;

class Section {
    public:
	Section() {};
	~Section();
	std::string getVal(const std::string key) const;
	SectionList sectionList;
	std::map<std::string, std::string>	item;
};

typedef SectionList::const_iterator	CSI;
typedef SectionList::iterator		SI;
typedef std::map<std::string, std::string>::const_iterator	CII;
typedef std::map<std::string, std::string>::iterator		II;

class ConfEntry {
    public:
	FILE *f;
	Section *sect;
	std::string f_name;
};

struct ParserParam {
    //Section *conf_p;
    std::stack<Section *> sect_stack;
    std::stack<FILE *> in_stack;
    std::queue<ConfEntry> confEntryQueue; //this is for parcing multiple files.
    Section *root_p;
    const std::string *value_ptr;
    Section *sect_ptr;
    ParserParam() { value_ptr = NULL; sect_ptr = NULL; root_p = NULL; }
};

Section * parceConfig(char *confFileName);
void printSection(Section *sect, int level=0);

int conf_parse(void * param_p);

class SetupBaseType {
    public:
	virtual ~SetupBaseType() {}
	virtual void set(std::string &c_val) = 0;
};

class SetupBaseType_int : public SetupBaseType {
    private:
	int &val;
    public:
	SetupBaseType_int(int &c_val) : val(c_val) {}
	void set(std::string &str_val) {
	    val = atoi(str_val.c_str());
	}
};

class SetupBaseType_uint : public SetupBaseType {
    private:
	unsigned int &val;
    public:
	SetupBaseType_uint(unsigned int &c_val) : val(c_val) {}
	void set(std::string &str_val) {
	    val = (unsigned int) strtoul(str_val.c_str(), NULL, 10);
	}
};

class SetupBaseType_short : public SetupBaseType {
    private:
	short &val;
    public:
	SetupBaseType_short(short &c_val) : val(c_val) {}
	void set(std::string &str_val) {
	    val = (short) atoi(str_val.c_str());
	}
};

class SetupBaseType_ushort : public SetupBaseType {
    private:
	unsigned short &val;
    public:
	SetupBaseType_ushort(unsigned short &c_val) : val(c_val) {}
	void set(std::string &str_val) {
	    val = (unsigned short) strtoul(str_val.c_str(), NULL, 10);
	}
};

class SetupBaseType_long : public SetupBaseType {
    private:
	long &val;
    public:
	SetupBaseType_long(long &c_val) : val(c_val) {}
	void set(std::string &str_val) {
	    val = strtol(str_val.c_str(), NULL, 10);
	}
};

class SetupBaseType_ulong : public SetupBaseType {
    private:
	unsigned long &val;
    public:
	SetupBaseType_ulong(unsigned long &c_val) : val(c_val) {}
	void set(std::string &str_val) {
	    val = strtoul(str_val.c_str(), NULL, 10);
	}
};

class SetupBaseType_double : public SetupBaseType {
    private:
	double &val;
    public:
	SetupBaseType_double(double &c_val) : val(c_val) {}
	void set(std::string &str_val) {
	    val = strtod(str_val.c_str(), NULL);
	}
};

class SetupBaseType_string : public SetupBaseType {
    private:
	std::string &val;
    public:
	SetupBaseType_string(std::string &c_val) : val(c_val) {}
	void set(std::string &str_val) {
	    val = str_val;
	}
};

class SetupBaseType_yesno : public SetupBaseType {
    private:
	bool &val;
    public:
	SetupBaseType_yesno(bool &c_val) : val(c_val) {}
	void set(std::string &str_val) {
	    if (str_val == "yes") {
		val = true;
	    }
	    else {
		val = false;
	    }
	}
};

class SetupRow {
   public:
      std::string    key;
      SetupBaseType  *val_converter;
      bool           fatality;
      std::string    default_val;
      
      SetupRow(std::string &n_key, SetupBaseType *n_val_converter, 
    		bool n_fatality, std::string &n_default_val) :
         key(n_key),
	 val_converter(n_val_converter),
	 fatality(n_fatality),
	 default_val(n_default_val) {}
      SetupRow(const char *n_key, SetupBaseType *n_val_converter, 
    		bool n_fatality, std::string &n_default_val) :
	 val_converter(n_val_converter),
	 fatality(n_fatality),
	 default_val(n_default_val) {
	 key = n_key;
      }
      ~SetupRow() {
          delete val_converter;
	  val_converter = NULL;
      }
};

class SetupVars {
    private:
	std::vector<SetupRow *> var_row;
    public:
	~SetupVars() {
	    for (size_t i=0; i<var_row.size(); ++i) {
		delete var_row[i];
	    }
	};
	void Add(std::string &key, SetupBaseType * sbt, 
		bool fatality, std::string default_val) {
	    var_row.push_back(new SetupRow(key, sbt, fatality, default_val));
	}
	void Add(const char *key, SetupBaseType * sbt, 
		bool fatality, std::string default_val) {
	    var_row.push_back(new SetupRow(key, sbt, fatality, default_val));
	}
	void Set(const Section *sect) {
	    std::string conf_value;
	    for (size_t i=0; i<var_row.size(); ++i) {
		try {
		    conf_value = sect->getVal(var_row[i]->key);
		    var_row[i]->val_converter->set(conf_value);
		} catch (...) {
		    if (var_row[i]->fatality == true) { 
			//fatal error, 
			//must be _right_ value in config file.
			throw;
		    }
		    //set the default.
		    var_row[i]->val_converter->set(var_row[i]->default_val);
		}
	    }
	}
};

#endif
