///////////////////////////////////////////////////////////////////////////////////////////////
//
// This file contains both GNU as VALVE licenced material.
//
// This source file contains VALVE LCC licence material.
// Read and Agree to the above stated header
// before distibuting or editing this source code.
//
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
// bot_weapons.h
// 
//////////////////////////////////////////////////////////////////////////////////////////////// 

#ifndef BOT_WEAPONS_H
#define BOT_WEAPONS_H

// weapon ID values for Firearms
// FA base IDs ie. 2.4 to 2.9
#define FAB_WEAPON_KNIFE			1
#define FAB_WEAPON_COLTGOV			2
#define FAB_WEAPON_BER92F			3
#define FAB_WEAPON_BER93R			4
#define FAB_WEAPON_ANACONDA			5
#define FAB_WEAPON_DESERT			6
#define FAB_WEAPON_BENELLI			7 // this is called remington in Firearms 2.8 and above
#define FAB_WEAPON_SAIGA			8

// weapon ID values for FA2.4
#define FA24_WEAPON_MP5K			9

// weapon ID values for FA2.5
#define FA25_WEAPON_MP5A5			10
#define FA25_WEAPON_MC51			11
#define FA25_WEAPON_M11				12
#define FA25_WEAPON_M79				13
#define FA25_WEAPON_FRAG			14
#define FA25_WEAPON_FLASHBANG		15
#define FA25_WEAPON_CLAYMORE		16
#define FA25_WEAPON_STG24			17
#define FA25_WEAPON_AK47			18
#define FA25_WEAPON_FAMAS			19
#define FA25_WEAPON_PSG1			20
#define FA25_WEAPON_G3A3			21
#define FA25_WEAPON_G36E			22
#define FA25_WEAPON_M16				23
#define FA25_WEAPON_AUG				24
#define FA25_WEAPON_M82				25
#define FA25_WEAPON_M4				26
#define FA25_WEAPON_M60				27
#define FA25_WEAPON_BIZON			28
#define FA25_WEAPON_STERLING		29

// weapon ID values for FA 2.6, 2.65 and 2.7
#define FA26_WEAPON_SVD				11
#define FA26_WEAPON_UZI				12
#define FA26_WEAPON_CONCUSSION		15
#define FA26_WEAPON_SSG3000			24
#define FA26_WEAPON_AK74			26
#define FA26_WEAPON_PKM				30		// this is m249 in FA27, but the ID is same

// weapon ID values for FA 2.8
#define FA28_WEAPON_MP5A5			9
#define FA28_WEAPON_BIZON			10
#define FA28_WEAPON_STERLING		11
#define FA28_WEAPON_M79				12
#define FA28_WEAPON_FRAG			13
#define FA28_WEAPON_CONCUSSION		14
#define FA28_WEAPON_CLAYMORE		15
#define FA28_WEAPON_AK47			16
#define FA28_WEAPON_FAMAS			17
#define FA28_WEAPON_G3A3			18
#define FA28_WEAPON_G36E			19
#define FA28_WEAPON_M16				20
#define FA28_WEAPON_SSG3000			21
#define FA28_WEAPON_M82				22
#define FA28_WEAPON_AK74			23
#define FA28_WEAPON_M60				24
#define FA28_WEAPON_M249			25
#define FA28_WEAPON_UZI				26
#define FA28_WEAPON_SVD				27
#define FA28_WEAPON_PKM				28

// FA 2.9 weapon additions
#define FA29_WEAPON_M14				29
#define FA29_WEAPON_M4				30

typedef struct
{
   char szClassname[64];
   int  iAmmo1;     // ammo index for primary ammo
   int  iAmmo1Max;  // max primary ammo
   int  iAmmo2;     // ammo index for secondary ammo
   int  iAmmo2Max;  // max secondary ammo
   int  iSlot;      // HUD slot (0 based)
   int  iPosition;  // slot position
   int  iId;        // weapon ID
   int  iFlags;     // flags???
} bot_weapon_t;

extern int fa_weapon_knife;
extern int fa_weapon_coltgov;
extern int fa_weapon_ber92f;
extern int fa_weapon_ber93r;
extern int fa_weapon_anaconda;
extern int fa_weapon_desert;
extern int fa_weapon_benelli;	// remington in FA28 and above
extern int fa_weapon_saiga;
extern int fa_weapon_mp5k;		// FA24 weapon
extern int fa_weapon_mp5a5;
extern int fa_weapon_mc51;		// FA25 weapon
extern int fa_weapon_m11;		// FA25 weapon
extern int fa_weapon_bizon;
extern int fa_weapon_sterling;
extern int fa_weapon_uzi;
extern int fa_weapon_m79;
extern int fa_weapon_frag;
extern int fa_weapon_concussion;
extern int fa_weapon_flashbang;
extern int fa_weapon_stg24;		// stielhandgranate
extern int fa_weapon_claymore;
extern int fa_weapon_ak47;
extern int fa_weapon_famas;
extern int fa_weapon_g3a3;
extern int fa_weapon_g36e;
extern int fa_weapon_m16;
extern int fa_weapon_ak74;
extern int fa_weapon_m14;
extern int fa_weapon_m4;
extern int fa_weapon_aug;		// FA25 weapon
extern int fa_weapon_psg1;		// FA25 weapon
extern int fa_weapon_ssg3000;
extern int fa_weapon_m82;
extern int fa_weapon_svd;
extern int fa_weapon_m60;
extern int fa_weapon_m249;
extern int fa_weapon_pkm;


#endif // BOT_WEAPONS_H

