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
// defines.h
// 
////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef MB_DEFINES_H
#define MB_DEFINES_H

#include <string.h>

// Marine Bot source code is compatible with original HL SDK v2.3 by default,
// but by setting this flag the source code will compile even under new HL SDK by Allied Modders
// (it affects several engine functions mainly in engine.cpp)
//#define NEWSDKAM		1

// by setting this flag MB source codes will successfully compile under the new Valve HL SDK released in year 2013
//#define NEWSDKVALVE	1

// by setting this flag the source codes will compile under Metamod patched HL SDK v2.3
//#define NEWSDKMM		1

// if this flag is set then Marine Bot will strip Famas out of the game (it won't link Famas related entities)
//#define NOFAMAS		1

// was used as a hot fix for the 'cutscenes crashing Dedicated Server' bug,
// there's already a proper code dealing with that so this flag has no meaning anymore
//#define SERVER

#ifndef _WINDOWS_
typedef unsigned short			WORD;
typedef unsigned long			DWORD;
typedef long					LONG;
typedef unsigned char			BYTE;
#endif

// needed for the 4 engine functions in original HL SDK v2.3 that are different on Linux
// to actually pass through the compilation on Linux ... weird, maybe I'm just missing something
#if !defined ( NEWSDKAM ) && !defined ( NEWSDKVALVE ) && !defined ( NEWSDKMM ) && !defined ( WIN32 )
typedef long int					int32;
typedef unsigned long int			uint32;
#endif


#endif // DEFINES_H
