#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES

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

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef uintptr_t addr;

#include "MemoryMgr.h"

//#define RELEASE

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

struct CBaseModelInfo
{
	void *vmt;
	char name[24];
	void *colModel;
	void *twodEffects;
	short id;
	ushort refCount;
	short txdSlot;
	uchar type; // ModelInfo type
	uchar num2dEffects;
	bool freeCol;
};

struct CSimpleModelInfo : CBaseModelInfo
{
	RpAtomic *atomics[3];
	float lodDistances[3];
	uchar numAtomics;
	uchar alpha;
	ushort flags;

	void SetAtomic(int n, RpAtomic *atomic);
	void SetAtomic_hook(int n, RpAtomic *atomic);
};

struct CClumpModelInfo : CBaseModelInfo
{
	RpClump *clump;

	RpClump *CreateInstance(void);
	void SetClump(RpClump*);
	void SetFrameIds(int ids);
};

struct CMatrix
{
	RwMatrix matrix;
	RwMatrix *pMatrix;
	int haveRwMatrix;
};

struct CPlaceable_III
{
	void *vmt;
	CMatrix matrix;
};

struct CPlaceable
{
	CMatrix matrix;
};

typedef void (*voidfunc)(void);

extern HMODULE dllModule;
extern char asipath[MAX_PATH];

// ini switches
extern int blendstyle, blendkey;
extern int texgenstyle, texgenkey;
extern int xboxcarpipe, xboxcarpipekey;
extern int rimlight, rimlightkey;
extern int xboxworldpipe, xboxworldpipekey;
extern int envMapSize;

char *getpath(char *path);
RwImage *readTGA(const char *afilename);

void RenderEnvTex(void);
void DefinedState(void);

#include "neo.h"

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
RwBool D3D8AtomicDefaultInstanceCallback(void*, RxD3D8InstanceData*, RwBool);

RwBool D3D8AtomicDefaultInstanceCallback_fixed(void*, RxD3D8InstanceData*, RwBool);

void rxD3D8DefaultRenderCallback_d3d9(RwResEntry*, void*, RwUInt8, RwUInt32);
RwBool RwD3D8SetSurfaceProperties_d3d9(RwRGBA*, RwSurfaceProperties*, RwUInt32);

void rxD3D8SetAmbientLight(void);
void rxD3D8DefaultRenderFFPObjectSetUp(RwUInt32 flags);
void rxD3D8DefaultRenderFFPMeshSetUp(RxD3D8InstanceData *inst);
void rxD3D8DefaultRenderFFPMeshCombinerSetUp(RxD3D8InstanceData *inst, RwUInt32 flags);
void rxD3D8DefaultRenderFFPMeshCombinerTearDown(void);
void rxD3D8DefaultRenderFFPMesh(RxD3D8InstanceData *inst, RwUInt32 flags);
void rxD3D8DefaultRenderCallback_xbox(RwResEntry*, void*, RwUInt8, RwUInt32);

int rwD3D8RWGetRasterStage(int);

enum {
	LOC_world       = 0,
	LOC_worldIT     = 4,
	LOC_view        = 8,
	LOC_proj        = 12,
	LOC_eye         = 16,
	LOC_ambient     = 17,
	LOC_directDir   = 18,
	LOC_directDiff  = 19,	// remove
	LOC_directCol   = 19,
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
