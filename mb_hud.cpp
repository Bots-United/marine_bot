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
// mb_hud.cpp
// 
////////////////////////////////////////////////////////////////////////////////////////////////

#include "defines.h"

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "bot.h"

extern edict_t *listenserver_edict;
extern float HUDmsg_time;


static unsigned short FixedUnsigned16( float value, float scale )
{
	int output;

	output = value * scale;
	if ( output < 0 )
		output = 0;
	if ( output > 0xFFFF )
		output = 0xFFFF;

	return (unsigned short)output;
}


static short FixedSigned16( float value, float scale )
{
	int output;

	output = value * scale;

	if ( output > 32767 )
		output = 32767;

	if ( output < -32768 )
		output = -32768;

	return (short)output;
}

/*
* creates a HUD text message
* the maximal length of the text is 512 chars because of engine limitation
*/
void FullCustHudMessage(edict_t *pEntity, const char *msg_name, int channel, int pos_x, int pos_y, int gfx, Vector color1, Vector color2, int brightness, float fade_in, float fade_out, float duration)
{
	MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, NULL, pEntity);
	WRITE_BYTE( TE_TEXTMESSAGE);
	WRITE_BYTE( channel & 0xFF );			// channel ???
	WRITE_SHORT( FixedSigned16( pos_x, -1<<13 ) );	// x coord on the screen
	WRITE_SHORT( FixedSigned16( pos_y, -1<<13 ) );	// y coord on the screen
	WRITE_BYTE(gfx);		// 0 no effect // 1 flashing // 2 fade over effect

	WRITE_BYTE(color1.x);	//r	// all to 255 is white // all to 0 is transparent
	WRITE_BYTE(color1.y);	//g
	WRITE_BYTE(color1.z);	//b
	WRITE_BYTE(brightness);	//opacity/brightness

	WRITE_BYTE(color2.x);	//r
	WRITE_BYTE(color2.y);	//g
	WRITE_BYTE(color2.z);	//b
	WRITE_BYTE(brightness);
	WRITE_SHORT( FixedUnsigned16( fade_in, 1<<8 ) );	// fade in time
	WRITE_SHORT( FixedUnsigned16( fade_out, 1<<8 ) );	// fade out time
	WRITE_SHORT( FixedUnsigned16( duration, 1<<8 ) );	// hold time
	
	if (gfx == 2)
		WRITE_SHORT( FixedUnsigned16( 4, 1<<8 ) );	// Red over lay after 4 seconds (only in gfx mode 2)
	
	WRITE_STRING(msg_name);
	MESSAGE_END();
}


/*
* displays plain HUD message for given time
* gfx may only be 0 or 1
*/
void StdHudMessage(edict_t *pEntity, const char *msg_name, int gfx, int time)
{
	// to prevent crashing HL due to overloading the engine
	if (HUDmsg_time > gpGlobals->time)
		return;

	edict_t *pEdict;
	Vector color;

	if ((gfx == NULL ) || ((gfx < 0 || gfx > 1)))
		gfx = 0;

	if ((time == NULL ) || ((time < 1 || time > 10)))
		time = 4;
	
	if (pEntity == NULL)
		pEdict = listenserver_edict;
	else
		pEdict = pEntity;

	// shade of red
	color = Vector(200, 60, 40);

	FullCustHudMessage(pEdict, msg_name, 2, 0, 0, gfx, color, color, 200, 0.03, 0.1, time);
}


/*
* sends the plain HUD message to all real clients (ie. not to bots)
*/
void StdHudMessageToAll(const char *msg_name, int gfx, int time)
{
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if ((clients[i].pEntity != NULL) && (clients[i].IsHuman()))
		{
			StdHudMessage(clients[i].pEntity, msg_name, gfx, time);
		}
	}
}


/*
* displays a HUD message where user can specify also color of the message
*/
void CustHudMessage(edict_t *pEntity, const char *msg_name, Vector color1 , Vector color2, int gfx, int time)
{
	if (HUDmsg_time < gpGlobals->time)
		FullCustHudMessage(pEntity, msg_name, 3, 1, 0, gfx, color1, color2, 200, 0.03, 1.0, time);
}


/*
* sends the custom HUD message to all real clients (ie. not to bots)
*/
void CustHudMessageToAll(const char *msg_name, Vector color1, Vector color2, int gfx, int time)
{
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if ((clients[i].pEntity != NULL) && (clients[i].IsHuman()))
		{
			CustHudMessage(clients[i].pEntity, msg_name, color1, color2, gfx, time);
		}
	}
}


/*
* displays a transparent HUD message at top left corner for 5 seconds
*/
void DisplayMsg(edict_t *pEntity, const char *msg)
{
	if (HUDmsg_time > gpGlobals->time)
		return;
	
	// to prevent overloading HL engine, crashed if this message is used alot
	// so just don't allow to print this message every frame
	HUDmsg_time = gpGlobals->time + 0.5;

	// we're using just one color for this message
	Vector color = Vector(0, 255, 20);
		
	// displays the message
	FullCustHudMessage(pEntity, msg, 3, 0, 0, 0, color, color, 200, 0.03, 0.03, 5.0);
}


/*
* displays a transparent HUD message by the top of the screen for given time
*/
void CustDisplayMsg(edict_t *pEntity, const char *msg, int x_pos, float time)
{
	if (HUDmsg_time > gpGlobals->time)
		return;

	HUDmsg_time = gpGlobals->time + 0.5;
	Vector color = Vector(0, 255, 20);

	FullCustHudMessage(pEntity, msg, 3, x_pos, 0, 0, color, color, 200, 0.03, 0.03, time);
}
