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
#include "MemoryMgr.h"

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;

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

#define PTRFROMCALL(addr) (uint32_t)(*(uint32_t*)((uint32_t)addr+1) + (uint32_t)addr + 5)
#define INTERCEPT(saved, func, a) \
{ \
	saved = PTRFROMCALL(a); \
	MemoryVP::InjectHook(a, func); \
}

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
void hookWaterDrops(void);
void neoInit(void);
void RenderEnvTex(void);
void DefinedState(void);

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

/*
 * Xbox water drops
 */

struct VertexTex2
{
	RwReal      x;
	RwReal      y;
	RwReal      z;
	RwReal      rhw;
	RwUInt32    emissiveColor;
	RwReal      u0;
	RwReal      v0;
	RwReal      u1;
	RwReal      v1;
};

class WaterDrop
{
public:
	float x, y, time;		// shorts on xbox (short float?)
	float size, uvsize, ttl;	// "
	uchar alpha;
	bool active;
	bool fades;

	void Fade(void);
};

class WaterDropMoving
{
public:
	WaterDrop *drop;
	float dist;
};

class WaterDrops
{
public:
	enum {
		MAXDROPS = 2000,
		MAXDROPSMOVING = 700
	};

	// Logic

	static float ms_xOff, ms_yOff;	// not quite sure what these are
	static WaterDrop ms_drops[MAXDROPS];
	static int ms_numDrops;
	static WaterDropMoving ms_dropsMoving[MAXDROPSMOVING];
	static int ms_numDropsMoving;

	static bool ms_enabled;
	static bool ms_movingEnabled;

	static float ms_distMoved, ms_vecLen, ms_rainStrength;
	static RwV3d ms_vec;
	static RwV3d ms_lastAt;
	static RwV3d ms_lastPos;
	static RwV3d ms_posDelta;

	static int ms_splashDuration;
	static CPlaceable_III *ms_splashObject;

	static void Process(void);
	static void CalculateMovement(void);
	static void SprayDrops(void);
	static void MoveDrop(WaterDropMoving *moving);
	static void ProcessMoving(void);
	static void Fade(void);

	static WaterDrop *PlaceNew(float x, float y, float size, float time, bool fades);
	static void NewTrace(WaterDropMoving*);
	static void NewDropMoving(WaterDrop*);
	// this has one more argument in VC: ttl, but it's always 2000.0
	static void FillScreenMoving(float amount);
	static void FillScreen(int n);
	static void Clear(void);
	static void Reset(void);

	static void RegisterSplash(CPlaceable_III *plc);
	static bool NoRain(void);

	// Rendering

	static RwTexture *ms_maskTex;
	static RwTexture *ms_tex;	// TODO
	static RwRaster *ms_maskRaster;
	static RwRaster *ms_raster;	// TODO
	static int ms_fbWidth, ms_fbHeight;
	static void *ms_vertexBuf;
	static void *ms_indexBuf;
	static VertexTex2 *ms_vertPtr;
	static int ms_numBatchedDrops;
	static int ms_initialised;

	static void InitialiseRender(RwCamera *cam);
	static void AddToRenderList(WaterDrop *drop);
	static void Render(void);
};
