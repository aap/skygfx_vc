#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <rwcore.h>
#include <rwplcore.h>
#include <rpworld.h>
#include <rpmatfx.h>
#include <rtbmp.h>
#include "rwd3d9.h"
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "resource.h"
#include "MemoryMgr.h"

struct MatFXEnv
{
	RwFrame *envFrame;
	RwTexture *envTex;
	float envCoeff;
	int envFBalpha;
	int pad;
	int effect;
};

struct MatFXDual {
	RwTexture *tex;
	RwInt32 srcBlend;
	RwInt32 dstBlend;
};

struct MatFX
{
	MatFXEnv fx[2];
	int effects;
};

struct CClumpModelInfo
{
	void **vtable;
	char     name[24];
	RwInt32 data1[3];
	RwUInt8 unk1[4];
	RwInt32 unk2;
	RpClump *clump;

	RpClump *CreateInstance(void);
	void SetClump(RpClump*);
	void SetFrameIds(int ids);
};

struct CBaseModelInfo
{
	void *__vmt;
	char m_cName[24];
	void *m_pColModel;
	void *m_p2dEffect;
	WORD m_wObjectDataId;
	WORD m_wRefCount;
	WORD m_wTxdId;
	BYTE m_bType;
	BYTE m_bNum2dEffects;
	BYTE __field_2C;
	BYTE __padding0[3];
};

struct CSimpleModelInfo
{
	CBaseModelInfo __parent;
	void *atomics[3];
	float lodDistances[3];
	BYTE numAtomics;
	BYTE alpha;
	WORD flags;

	void SetAtomic(int n, RpAtomic *atomic);
	void SetAtomic_hook(int n, RpAtomic *atomic);
};

extern HMODULE dllModule;
extern char asipath[MAX_PATH];

// ini switches
extern int blendstyle, blendkey;
extern int texgenstyle, texgenkey;
extern int xboxcarpipe, xboxcarpipekey;
extern int rimlight, rimlightkey;
extern int xboxworldpipe, xboxworldpipekey;
extern int envMapSize;

void neoInit(void);
void RenderEnvTex(void);

extern void **&RwEngineInst;
extern RpLight *&pAmbient;
extern RpLight *&pDirect;
extern RpLight **pExtraDirectionals;
extern int &NumExtraDirLightsInWorld;
extern int &MatFXMaterialDataOffset;
extern int &MatFXAtomicDataOffset;

void drawDualPass(RxD3D8InstanceData *inst);

void rxD3D8DefaultRenderCallback(RwResEntry*, void*, RwUInt8, RwUInt32);
void rwD3D8AtomicMatFXRenderCallback(RwResEntry*, void*, RwUInt8, RwUInt32);
RwBool rwD3D8RenderStateIsVertexAlphaEnable(void);
void rwD3D8RenderStateVertexAlphaEnable(RwBool x);

enum {
	LOC_world       = 0,
	LOC_worldIT     = 4,
	LOC_view        = 8,
	LOC_proj        = 12,
	LOC_eye         = 16,
	LOC_ambient     = 17,
	LOC_directDir   = 18,
	LOC_directDiff  = 19,
	LOC_directSpec  = 20,
	LOC_lights      = 21,
	LOC_rampStart   = 31,
	LOC_rampEnd     = 32,
	LOC_rim         = 33,

	LOC_matCol      = 29,
	LOC_surfProps   = 30,
};

extern RwTexture *rampTex;
void reloadRamp(void);