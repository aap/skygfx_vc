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

typedef unsigned char uchar;
typedef unsigned short ushort;

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

class WaterDrop
{
public:
	float x0, y0, time;		// shorts on xbox (short float?)
	float size, uvsize, ttl;	// "
	uchar alpha;
	bool active;
	bool fades;

	void Fade(void);
};

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

class WaterDrops
{
public:
	enum {
		MAXDROPS = 2000
	};
	static float xOff, yOff;	// not quite sure what these are
	static WaterDrop drops[MAXDROPS];
	static int numDrops;
	static int initialised;

	static RwTexture *maskTex;
	static RwTexture *tex;	// TODO
	static RwRaster *maskRaster;
	static RwRaster *raster;	// TODO

	static int fbWidth, fbHeight;

	static void *vertexBuf;
	static void *indexBuf;
	static VertexTex2 *vertPtr;
	static int numBatchedDrops;

	static void Initialise(RwCamera *cam);
	static void PlaceNew(float x0, float y0, float x1, float z1, bool flag);
	static void AddToRenderList(WaterDrop *drop);
	static void Clear(void);
	static void FillScreen(int n);
	static void Process(void);
	static void Render(void);
};
