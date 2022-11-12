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
// h_export.cpp
// 
////////////////////////////////////////////////////////////////////////////////////////////////

#if defined(WIN32)
#pragma warning(disable: 4005 91 4996)
#endif

#include "defines.h"

#include "extdll.h"
#include "enginecallback.h"
#include "util.h"
#include "cbase.h"

#include "bot.h"
#include "engine.h"

#ifndef __linux__

HINSTANCE h_Library = nullptr;
HGLOBAL h_global_argv = nullptr;
void FreeNameFuncGlobals();
void LoadSymbols(char *filename);

#else

void *h_Library = NULL;
char h_global_argv[1024];

#endif

enginefuncs_t g_engfuncs;
globalvars_t  *gpGlobals;
char *g_argv;

static FILE *fp, *fl;

int mod_id;
// global mod name, it's used to build virtual path inside HL folder
char mod_dir_name[32];
int g_mod_version;
// global bool steam detected
// set to false by default if steam found then set to true
bool is_steam = FALSE;

GETENTITYAPI other_GetEntityAPI = nullptr;
GETNEWDLLFUNCTIONS other_GetNewDLLFunctions = nullptr;
GIVEFNPTRSTODLL other_GiveFnptrsToDll = nullptr;


#ifndef __linux__

// Required DLL entry point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
   if (fdwReason == DLL_PROCESS_ATTACH)
   {
   }
   else if (fdwReason == DLL_PROCESS_DETACH)
   {
	  FreeNameFuncGlobals();  // Free exported symbol table

      if (h_Library)
         FreeLibrary(h_Library);

      if (h_global_argv)
      {
         GlobalUnlock(h_global_argv);
         GlobalFree(h_global_argv);
      }
   }

   return TRUE;
}

#endif

#ifndef __linux__
#ifdef __BORLANDC__
extern "C" DLLEXPORT void EXPORT GiveFnptrsToDll(enginefuncs_t* pengfuncsFromEngine, globalvars_t *pGlobals)
#else
void DLLEXPORT GiveFnptrsToDll( enginefuncs_t* pengfuncsFromEngine, globalvars_t *pGlobals )
#endif
#else
extern "C" void DLLEXPORT GiveFnptrsToDll( enginefuncs_t* pengfuncsFromEngine, globalvars_t *pGlobals )
#endif
{
	int pos;
	char game_dir[256];
	char game_dll_filename[256];
	char the_game[80];
	char temp_filename[256];
	
	// get the engine functions from the engine...
	
	memcpy(&g_engfuncs, pengfuncsFromEngine, sizeof(enginefuncs_t));
	gpGlobals = pGlobals;
	
	// find the directory name of the currently running MOD...
	(*g_engfuncs.pfnGetGameDir)(game_dir);

#ifdef _DEBUG
	extern int debug_engine;

	if (debug_engine)
	{
		fp=fopen("!mb_engine_debug.txt","a");
		fprintf(fp, "\n<h_export.cpp>Current virtual path returned by GetGameDir(): \"%s\"\n", game_dir);
		fclose(fp);
	}
#endif

	// STEAM detector is based on the fact that STEAM engine returns only mod directory name
	// (eg. "firearms") while original WON HL returns whole path to HL (eg. "C:\Half-Life/firearms")
	// therefore we have to scan the string backwards till first directory separator
	pos = strlen(game_dir) - 1;
	while ((pos) && (game_dir[pos] != '/'))
		pos--;

	// there was no directory separator which means that we are running STEAM
	if (pos == 0)
	{
		is_steam = TRUE;	// STEAM found
	}
	
	// code by *** Shrike ***
	if (is_steam)
	{
		pos = 0;
 
		if (strchr(game_dir, '/') != nullptr)
		{
			// scan backwards till first directory separator...
			while ((pos) && (game_dir[pos] != '/'))
				pos--;

			if (pos == 0)
			{
				// Error getting directory name!

				ALERT( at_error, "MarineBot - Error determining MOD directory name (under STEAM)!" );

				/**/
#ifdef _DEBUG
				// debuging
				fp=fopen("!mb_engine_debug.txt","a");
				fprintf(fp,"Error determining MOD directory name (under STEAM)!\n");
				fclose(fp);
#endif
				/**/
			}
			pos++;
		}
	}
	else
	{
		pos = strlen(game_dir) - 1;
		// scan backwards till first directory separator...
		while ((pos) && (game_dir[pos] != '/'))
			pos--;

		if (pos == 0)
		{
			// Error getting directory name!

			ALERT( at_error, "MarineBot - Error determining MOD directory name!" );

			/**/
#ifdef _DEBUG
			// debuging
			fp=fopen("!mb_engine_debug.txt","a");
			fprintf(fp,"Error determining MOD directory name!\n");
			fclose(fp);
#endif
			/**/
		}
		pos++;
	}
	
	strcpy(mod_dir_name, &game_dir[pos]);

#ifdef _DEBUG
	extern int debug_engine;

	if (debug_engine)
	{
		fp=fopen("!mb_engine_debug.txt","a");
		fprintf(fp, "\n<h_export.cpp>Mod directory name: \"%s\"\n", mod_dir_name);
		fclose(fp);
	}
#endif
	
	game_dll_filename[0] = 0;
	the_game[0] = 0;//																						NEW CODE 094

	// we need to know which mod do we play so check mod directory name first
	if (strcmpi(mod_dir_name, "firearms") == 0)
		strcpy(the_game, mod_dir_name);
	// mod directory hasn't default name so get "game" title from liblist.gam
	else
	{
		// find liblist.gam file located in mod root directory
		UTIL_BuildFileName(temp_filename, mod_dir_name, "liblist.gam", FALSE);

		fl = fopen(temp_filename,"r");

		if (fl != nullptr)
		{
			char line_string_buffer[80];

			// scan line strings one by one
			while (fscanf (fl, "%s", line_string_buffer) != EOF)
			{
				if (strcmp(line_string_buffer, "game") == 0)		// find game(mod) title
				{
					char temp[80];

					fgets (temp , 60, fl);			// get a few chars after the "game" string

					// is there a word "Firearms" in the "game" string
					if ((strstr(temp, "Firearms") != nullptr) || (strstr(temp, "FireArms") != nullptr))
					{
						strcpy(the_game, "firearms");

						break;
					}
					else
					{
						/**/
#ifdef _DEBUG						
						// debuging
						fp=fopen("!mb_engine_debug.txt","a");
						fprintf(fp,"Can't find \"Firearms\" in liblist.gam on line beginning on \"game\" (line buffer is \"%s\")\n", line_string_buffer);
						fclose(fp);
#endif			
						/**/
					}
				}
			}

			fclose(fl);
		}
		else
		{
			/**/
#ifdef _DEBUG			
			// debuging
			fp=fopen("!mb_engine_debug.txt","a");
			fprintf(fp,"Can't open \"%s\"\n", temp_filename);
			fclose(fp);
#endif
			/**/
		}
	}

	// we finally can check which mod do we play
	if (strcmpi(the_game, "firearms") == 0)
	{
		// added by *** Shrike ***
		mod_id = FIREARMS_DLL;
		char compare[20];
		char ver_com[20];

		g_mod_version = FA_30;	// set the latest available version by default

		// find liblist.gam file located in mod root directory
		UTIL_BuildFileName(temp_filename, mod_dir_name, "liblist.gam", FALSE);

		fl = fopen(temp_filename,"r");
		
		// code by *** Shrike ***
		if (fl != nullptr)
		{
			while (fscanf (fl, "%s", compare) != EOF)	// scan line strings
			{
				if (strcmp(compare, "version") == 0)	// find version
				{
					fgets (ver_com , 20, fl);	// get 20 chars after "version"
					int i = strlen (ver_com);	// get string lenght
					
					//fp=fopen("!mb_engine_debug.txt","a"); fprintf(fp,"mod: %d\n", g_mod_version); fclose(fp);	// debuging.

					for (int j=0; j<i; j++)	// remove " from scaned line
					{
						while (ver_com[j] == '"')
							ver_com[j] = ' ';
					}	
					while (ver_com[0] == ' ')	// shuffle chars to left
					{
						i = strlen (ver_com);	// get string lenght

						for (int x=0; x<i; x++)
							ver_com[x] = ver_com[x+1];

						ver_com[i] = ' ';
					}

					//fp=fopen("!mb_engine_debug.txt","a");	fprintf(fp,"version string: %s\n", ver_com); fclose(fp);	// debuging.

					if (strncmp(ver_com, "3.0", 3) == 0)	// scan for 3.0
					{
						g_mod_version = FA_30;
						//fp=fopen("!mb_engine_debug.txt","a"); fprintf(fp,"3.0 selected\nmod: %d\n", g_mod_version); fclose(fp);

						break;
					}
					else if (strncmp(ver_com, "2.9", 3) == 0)	// scan for 2.9
					{
						g_mod_version = FA_29;
						//fp=fopen("!mb_engine_debug.txt","a"); fprintf(fp,"2.9 selected\nmod: %d\n", g_mod_version); fclose(fp);

						break;
					}
					else if (strncmp(ver_com, "2.8", 3) == 0)	// scan for 2.8
					{
						g_mod_version = FA_28;
						//fp=fopen("!mb_engine_debug.txt","a"); fprintf(fp,"2.8 selected\nmod: %d\n", g_mod_version); fclose(fp);
						break;
					}
					else if (strncmp(ver_com, "2.7", 3) == 0)	// scan for 2.7
					{
						g_mod_version = FA_27;
						//fp=fopen("!mb_engine_debug.txt","a"); fprintf(fp,"2.7 selected\nmod : %d\n", g_mod_version); fclose(fp);

						break;
					}
					else if (strncmp(ver_com, "2.6", 3) == 0)	// scan for 2.6 also for 2.65
					{
						g_mod_version = FA_26;
						//fp=fopen("!mb_engine_debug.txt","a"); fprintf(fp,"2.6 or 2.65 selected\nmod : %d\n", g_mod_version); fclose(fp);
						break;
					}
					else if ((strncmp(ver_com, "RC2.5", 5) == 0) ||	// scan for 2.5
						(strncmp(ver_com, "2.5", 3) == 0))	// to read also what we force users to do when we had this code messed in mb08b
					{
						g_mod_version = FA_25;
						//fp=fopen("!mb_engine_debug.txt","a"); fprintf(fp,"2.5 selected\nmod : %d\n", g_mod_version); fclose(fp);
						break;
					}
					else if ((strncmp(ver_com, "FA-RC-2.4", 9) == 0) ||	// scan for 2.4
						(strncmp(ver_com, "2.4", 3) == 0))	// just to make thigs same
					{
						g_mod_version = FA_24;
						//fp=fopen("!mb_engine_debug.txt","a"); fprintf(fp,"2.4 selected\nmod : %d\n", g_mod_version); fclose(fp);
						break;
					}
					else
					{
						/**/
#ifdef _DEBUG						
						// debuging
						fp=fopen("!mb_engine_debug.txt","a");
						fprintf(fp,"Can't get version number out of liblist.gam (\"version\" line buffer is \"%s\")\n", ver_com);
						fclose(fp);
#endif			
						/**/
					}
				}
			}

			fclose(fl);
		}
		else
		{
			/**/
#ifdef _DEBUG			
			// debuging
			fp=fopen("!mb_engine_debug.txt","a");
			fprintf(fp,"Can't open \"%s\"\n", temp_filename);
			fclose(fp);
#endif
			/**/
		}


		temp_filename[0] = 0;

		UTIL_BuildFileName(temp_filename, mod_dir_name, "dlls", FALSE);

#ifndef __linux__
		UTIL_BuildFileName(game_dll_filename, temp_filename, "firearms.dll", FALSE);
#else
		UTIL_BuildFileName(game_dll_filename, temp_filename, "fa_i386.so", FALSE);
#endif
	}
	
	if (game_dll_filename[0])
	{
#ifndef __linux__
		h_Library = LoadLibrary(game_dll_filename);
#else
		h_Library = dlopen(game_dll_filename, RTLD_NOW);
#endif
	}
	else
	{
		// Directory error or Unsupported MOD!
		
		ALERT( at_error, "MarineBot - MOD dll not found (or unsupported MOD)!" );
		
		return;
	}


#ifndef __linux__
   h_global_argv = GlobalAlloc(GMEM_SHARE, 1024);
   g_argv = (char *)GlobalLock(h_global_argv);
#else
   g_argv = (char *)h_global_argv;
#endif

   other_GetEntityAPI = (GETENTITYAPI)GetProcAddress(h_Library, "GetEntityAPI");

   if (other_GetEntityAPI == nullptr)
   {
      // Can't find GetEntityAPI!

		ALERT( at_error, "MarineBot - Can't get MOD's GetEntityAPI!" );

		/**/
#ifdef _DEBUG
		// debuging
		fp=fopen("!mb_engine_debug.txt","a");
		fprintf(fp,"Can't get MOD's GetEntityAPI!\n");
		fclose(fp);
#endif
		/**/
   }

   other_GetNewDLLFunctions = (GETNEWDLLFUNCTIONS)GetProcAddress(h_Library, "GetNewDLLFunctions");

   /*/
   // NOTE by Frank: This is always NULL in Firearms so I think we can ignore it
   if (other_GetNewDLLFunctions == NULL)
   {
	   // Can't find GetNewDLLFunctions!
	   
	   ALERT( at_error, "MarineBot - Can't get MOD's GetNewDLLFunctions!" );
	   
	   fp=fopen("!mb_engine_debug.txt","a");
	   fprintf(fp,"Can't get MOD's GetNewDLLFunctions!\n");
	   fclose(fp);
   }
   /**/

   other_GiveFnptrsToDll = (GIVEFNPTRSTODLL)GetProcAddress(h_Library, "GiveFnptrsToDll");

   if (other_GiveFnptrsToDll == nullptr)
   {
      // Can't find GiveFnptrsToDll!

      ALERT( at_error, "MarineBot - Can't get MOD's GiveFnptrsToDll!" );

	  /**/
#ifdef _DEBUG
	  // debuging
	  fp=fopen("!mb_engine_debug.txt","a");
	  fprintf(fp,"Can't get MOD's GiveFnptrsToDll!\n");
	  fclose(fp);
#endif
	  /**/
   }


#ifndef __linux__
   LoadSymbols(game_dll_filename);  // Load exported symbol table
#endif

   pengfuncsFromEngine->pfnCmd_Args = Cmd_Args;
   pengfuncsFromEngine->pfnCmd_Argv = Cmd_Argv;
   pengfuncsFromEngine->pfnCmd_Argc = Cmd_Argc;

   pengfuncsFromEngine->pfnPrecacheModel = pfnPrecacheModel;
   pengfuncsFromEngine->pfnPrecacheSound = pfnPrecacheSound;
   pengfuncsFromEngine->pfnSetModel = pfnSetModel;
   pengfuncsFromEngine->pfnModelIndex = pfnModelIndex;
   pengfuncsFromEngine->pfnModelFrames = pfnModelFrames;
   pengfuncsFromEngine->pfnSetSize = pfnSetSize;
   pengfuncsFromEngine->pfnChangeLevel = pfnChangeLevel;
   pengfuncsFromEngine->pfnGetSpawnParms = pfnGetSpawnParms;
   pengfuncsFromEngine->pfnSaveSpawnParms = pfnSaveSpawnParms;
   pengfuncsFromEngine->pfnVecToYaw = pfnVecToYaw;
   pengfuncsFromEngine->pfnVecToAngles = pfnVecToAngles;
   pengfuncsFromEngine->pfnMoveToOrigin = pfnMoveToOrigin;
   pengfuncsFromEngine->pfnChangeYaw = pfnChangeYaw;
   pengfuncsFromEngine->pfnChangePitch = pfnChangePitch;
   pengfuncsFromEngine->pfnFindEntityByString = pfnFindEntityByString;
   pengfuncsFromEngine->pfnGetEntityIllum = pfnGetEntityIllum;
   pengfuncsFromEngine->pfnFindEntityInSphere = pfnFindEntityInSphere;
   pengfuncsFromEngine->pfnFindClientInPVS = pfnFindClientInPVS;
   pengfuncsFromEngine->pfnEntitiesInPVS = pfnEntitiesInPVS;
   pengfuncsFromEngine->pfnMakeVectors = pfnMakeVectors;
   pengfuncsFromEngine->pfnAngleVectors = pfnAngleVectors;
   pengfuncsFromEngine->pfnCreateEntity = pfnCreateEntity;
   pengfuncsFromEngine->pfnRemoveEntity = pfnRemoveEntity;
   pengfuncsFromEngine->pfnCreateNamedEntity = pfnCreateNamedEntity;
   pengfuncsFromEngine->pfnMakeStatic = pfnMakeStatic;
   pengfuncsFromEngine->pfnEntIsOnFloor = pfnEntIsOnFloor;
   pengfuncsFromEngine->pfnDropToFloor = pfnDropToFloor;
   pengfuncsFromEngine->pfnWalkMove = pfnWalkMove;
   pengfuncsFromEngine->pfnSetOrigin = pfnSetOrigin;
   pengfuncsFromEngine->pfnEmitSound = pfnEmitSound;
   pengfuncsFromEngine->pfnEmitAmbientSound = pfnEmitAmbientSound;
   pengfuncsFromEngine->pfnTraceLine = pfnTraceLine;
   pengfuncsFromEngine->pfnTraceToss = pfnTraceToss;
   pengfuncsFromEngine->pfnTraceMonsterHull = pfnTraceMonsterHull;
   pengfuncsFromEngine->pfnTraceHull = pfnTraceHull;
   pengfuncsFromEngine->pfnTraceModel = pfnTraceModel;
   pengfuncsFromEngine->pfnTraceTexture = pfnTraceTexture;
   pengfuncsFromEngine->pfnTraceSphere = pfnTraceSphere;
   pengfuncsFromEngine->pfnGetAimVector = pfnGetAimVector;
   pengfuncsFromEngine->pfnServerCommand = pfnServerCommand;
   pengfuncsFromEngine->pfnServerExecute = pfnServerExecute;
   pengfuncsFromEngine->pfnClientCommand = pfnClientCommand;
   pengfuncsFromEngine->pfnParticleEffect = pfnParticleEffect;
   pengfuncsFromEngine->pfnLightStyle = pfnLightStyle;
   pengfuncsFromEngine->pfnDecalIndex = pfnDecalIndex;
   pengfuncsFromEngine->pfnPointContents = pfnPointContents;
   pengfuncsFromEngine->pfnMessageBegin = pfnMessageBegin;
   pengfuncsFromEngine->pfnMessageEnd = pfnMessageEnd;
   pengfuncsFromEngine->pfnWriteByte = pfnWriteByte;
   pengfuncsFromEngine->pfnWriteChar = pfnWriteChar;
   pengfuncsFromEngine->pfnWriteShort = pfnWriteShort;
   pengfuncsFromEngine->pfnWriteLong = pfnWriteLong;
   pengfuncsFromEngine->pfnWriteAngle = pfnWriteAngle;
   pengfuncsFromEngine->pfnWriteCoord = pfnWriteCoord;
   pengfuncsFromEngine->pfnWriteString = pfnWriteString;
   pengfuncsFromEngine->pfnWriteEntity = pfnWriteEntity;
   pengfuncsFromEngine->pfnCVarRegister = pfnCVarRegister;
   pengfuncsFromEngine->pfnCVarGetFloat = pfnCVarGetFloat;
   pengfuncsFromEngine->pfnCVarGetString = pfnCVarGetString;
   pengfuncsFromEngine->pfnCVarSetFloat = pfnCVarSetFloat;
   pengfuncsFromEngine->pfnCVarSetString = pfnCVarSetString;
   // if you're getting errors here then go to 'defines.h' and set the correct flag
   // matching your HL SDK version
   pengfuncsFromEngine->pfnPvAllocEntPrivateData = pfnPvAllocEntPrivateData;
   pengfuncsFromEngine->pfnPvEntPrivateData = pfnPvEntPrivateData;
   pengfuncsFromEngine->pfnFreeEntPrivateData = pfnFreeEntPrivateData;
   pengfuncsFromEngine->pfnSzFromIndex = pfnSzFromIndex;
   pengfuncsFromEngine->pfnAllocString = pfnAllocString;
   pengfuncsFromEngine->pfnGetVarsOfEnt = pfnGetVarsOfEnt;
   pengfuncsFromEngine->pfnPEntityOfEntOffset = pfnPEntityOfEntOffset;
   pengfuncsFromEngine->pfnEntOffsetOfPEntity = pfnEntOffsetOfPEntity;
   pengfuncsFromEngine->pfnIndexOfEdict = pfnIndexOfEdict;
   pengfuncsFromEngine->pfnPEntityOfEntIndex = pfnPEntityOfEntIndex;
   pengfuncsFromEngine->pfnFindEntityByVars = pfnFindEntityByVars;
   pengfuncsFromEngine->pfnGetModelPtr = pfnGetModelPtr;
   pengfuncsFromEngine->pfnRegUserMsg = pfnRegUserMsg;
   pengfuncsFromEngine->pfnAnimationAutomove = pfnAnimationAutomove;
   pengfuncsFromEngine->pfnGetBonePosition = pfnGetBonePosition;
   pengfuncsFromEngine->pfnFunctionFromName = pfnFunctionFromName;
   pengfuncsFromEngine->pfnNameForFunction = pfnNameForFunction;
   pengfuncsFromEngine->pfnClientPrintf = pfnClientPrintf;
   pengfuncsFromEngine->pfnServerPrint = pfnServerPrint;
   pengfuncsFromEngine->pfnGetAttachment = pfnGetAttachment;
   pengfuncsFromEngine->pfnCRC32_Init = pfnCRC32_Init;
   pengfuncsFromEngine->pfnCRC32_ProcessBuffer = pfnCRC32_ProcessBuffer;
   pengfuncsFromEngine->pfnCRC32_ProcessByte = pfnCRC32_ProcessByte;
   pengfuncsFromEngine->pfnCRC32_Final = pfnCRC32_Final;
   pengfuncsFromEngine->pfnRandomLong = pfnRandomLong;
   pengfuncsFromEngine->pfnRandomFloat = pfnRandomFloat;
   pengfuncsFromEngine->pfnSetView = pfnSetView;
   pengfuncsFromEngine->pfnTime = pfnTime;
   pengfuncsFromEngine->pfnCrosshairAngle = pfnCrosshairAngle;
   pengfuncsFromEngine->pfnLoadFileForMe = pfnLoadFileForMe;
   pengfuncsFromEngine->pfnFreeFile = pfnFreeFile;
   pengfuncsFromEngine->pfnEndSection = pfnEndSection;
   pengfuncsFromEngine->pfnCompareFileTime = pfnCompareFileTime;
   pengfuncsFromEngine->pfnGetGameDir = pfnGetGameDir;
   pengfuncsFromEngine->pfnCvar_RegisterVariable = pfnCvar_RegisterVariable;
   pengfuncsFromEngine->pfnFadeClientVolume = pfnFadeClientVolume;
   pengfuncsFromEngine->pfnSetClientMaxspeed = pfnSetClientMaxspeed;
   pengfuncsFromEngine->pfnCreateFakeClient = pfnCreateFakeClient;
   pengfuncsFromEngine->pfnRunPlayerMove = pfnRunPlayerMove;
   pengfuncsFromEngine->pfnNumberOfEntities = pfnNumberOfEntities;
   pengfuncsFromEngine->pfnGetInfoKeyBuffer = pfnGetInfoKeyBuffer;
   pengfuncsFromEngine->pfnInfoKeyValue = pfnInfoKeyValue;
   pengfuncsFromEngine->pfnSetKeyValue = pfnSetKeyValue;
   pengfuncsFromEngine->pfnSetClientKeyValue = pfnSetClientKeyValue;
   pengfuncsFromEngine->pfnIsMapValid = pfnIsMapValid;
   pengfuncsFromEngine->pfnStaticDecal = pfnStaticDecal;
   pengfuncsFromEngine->pfnPrecacheGeneric = pfnPrecacheGeneric;
   pengfuncsFromEngine->pfnGetPlayerUserId = pfnGetPlayerUserId;
   pengfuncsFromEngine->pfnGetPlayerAuthId = pfnGetPlayerAuthId;
   pengfuncsFromEngine->pfnBuildSoundMsg = pfnBuildSoundMsg;
   pengfuncsFromEngine->pfnIsDedicatedServer = pfnIsDedicatedServer;
   pengfuncsFromEngine->pfnCVarGetPointer = pfnCVarGetPointer;
   pengfuncsFromEngine->pfnGetPlayerWONId = pfnGetPlayerWONId;

   // SDK 2.0 additions...
   pengfuncsFromEngine->pfnInfo_RemoveKey = pfnInfo_RemoveKey;
   pengfuncsFromEngine->pfnGetPhysicsKeyValue = pfnGetPhysicsKeyValue;
   pengfuncsFromEngine->pfnSetPhysicsKeyValue = pfnSetPhysicsKeyValue;
   pengfuncsFromEngine->pfnGetPhysicsInfoString = pfnGetPhysicsInfoString;
   pengfuncsFromEngine->pfnPrecacheEvent = pfnPrecacheEvent;
   pengfuncsFromEngine->pfnPlaybackEvent = pfnPlaybackEvent;
   pengfuncsFromEngine->pfnSetFatPVS = pfnSetFatPVS;
   pengfuncsFromEngine->pfnSetFatPAS = pfnSetFatPAS;
   pengfuncsFromEngine->pfnCheckVisibility = pfnCheckVisibility;
   pengfuncsFromEngine->pfnDeltaSetField = pfnDeltaSetField;
   pengfuncsFromEngine->pfnDeltaUnsetField = pfnDeltaUnsetField;
   pengfuncsFromEngine->pfnDeltaAddEncoder = pfnDeltaAddEncoder;
   pengfuncsFromEngine->pfnGetCurrentPlayer = pfnGetCurrentPlayer;
   pengfuncsFromEngine->pfnCanSkipPlayer = pfnCanSkipPlayer;
   pengfuncsFromEngine->pfnDeltaFindField = pfnDeltaFindField;
   pengfuncsFromEngine->pfnDeltaSetFieldByIndex = pfnDeltaSetFieldByIndex;
   pengfuncsFromEngine->pfnDeltaUnsetFieldByIndex = pfnDeltaUnsetFieldByIndex;
   pengfuncsFromEngine->pfnSetGroupMask = pfnSetGroupMask;
   pengfuncsFromEngine->pfnCreateInstancedBaseline = pfnCreateInstancedBaseline;
   pengfuncsFromEngine->pfnCvar_DirectSet = pfnCvar_DirectSet;
   pengfuncsFromEngine->pfnForceUnmodified = pfnForceUnmodified;
   pengfuncsFromEngine->pfnGetPlayerStats = pfnGetPlayerStats;
   pengfuncsFromEngine->pfnAddServerCommand = pfnAddServerCommand;

   // give the engine functions to the other DLL...
   (*other_GiveFnptrsToDll)(pengfuncsFromEngine, pGlobals);
}

