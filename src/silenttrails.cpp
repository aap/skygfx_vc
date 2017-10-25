#include "skygfx.h"

// stolen from Silent:
// https://github.com/CookiePLMonster/VC-Trails

struct SMenuEntry
{
	ushort	action;
	char	description[8];
	uchar	style;
	uchar	targetMenu;
	ushort	posX;
	ushort	posY;
	ushort	align;
};

static_assert(sizeof(SMenuEntry) == 0x12, "SMenuEntry error!");

const SMenuEntry	displaySettingsPatch[] = {
							/*Trails*/
							{ 9, "FED_TRA", 0, 4, 40, 153, 1 },
							/*Subtitles*/
							{ 10, "FED_SUB", 0, 4, 40, 178, 1 },
							/*Widescreen*/
							{ 11, "FED_WIS", 0, 4, 40, 202, 1 },
							/*Map Legend*/
							{ 31, "MAP_LEG", 0, 4, 40, 228, 1 },
							/*Radar Mode*/
							{ 32, "FED_RDR", 0, 4, 40, 253, 1 },
							/*HUD Mode*/
							{ 33, "FED_HUD", 0, 4, 40, 278, 1 },
							/*Resolution*/
							{ 43, "FED_RES", 0, 4, 40, 303, 1 },
							/*Restore Defaults*/
							{ 47, "FET_DEF", 0, 4, 320, 328, 3 },
							/*Back*/
							{ 34, "FEDS_TB", 0, 33, 320, 353, 3 }
					};

void
enableTrailsSetting(void)
{
	if(gtaversion == VC_10){
		int pMenuEntries = *(int*)0x4966A0 + 0x3C8;
		uchar *pTrailsOptionEnabled = *(unsigned char**)0x498DEE;

		Patch<const void*>(0x4902D2, pTrailsOptionEnabled);	// write setting
		Patch<const void*>(0x48FF97, pTrailsOptionEnabled);	// read setting
		memcpy((void*)pMenuEntries, displaySettingsPatch, sizeof(displaySettingsPatch));
	}else if(gtaversion == VC_11){
		int pMenuEntries = *(int*)0x4966B0 + 0x3C8;
		uchar *pTrailsOptionEnabled = *(unsigned char**)0x498DFE;

		Patch<const void*>(0x4902E2, pTrailsOptionEnabled);
		// TODO: read
		memcpy((void*)pMenuEntries, displaySettingsPatch, sizeof(displaySettingsPatch));
	}else if(gtaversion == VC_STEAM){
		int pMenuEntries = *(int*)0x4965B0 + 0x3C8;
		uchar *pTrailsOptionEnabled = *(unsigned char**)0x498CFE;

		Patch<const void*>(0x4901E2, pTrailsOptionEnabled);
		// TODO: read
		memcpy((void*)pMenuEntries, displaySettingsPatch, sizeof(displaySettingsPatch));
	}
}