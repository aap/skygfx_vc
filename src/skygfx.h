#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES
#pragma warning(disable: 4244)	// int to float
#pragma warning(disable: 4800)	// int to bool
#pragma warning(disable: 4838)  // narrowing conversion

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

// want to use RwEngineInst instead of RwEngineInstance
#undef RWSRCGLOBAL
#define RWSRCGLOBAL(variable) \
   (((RwGlobals *)RwEngineInst)->variable)

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef uintptr_t addr;
#define nil NULL

#include "MemoryMgr.h"

class ScreenFX
{
public:
	static RwRaster *pFrontBuffer;
	static void *gradingPS;

	static bool m_bYCbCrFilter;
	static float m_lumaScale;
	static float m_lumaOffset;
	static float m_cbScale;
	static float m_cbOffset;
	static float m_crScale;
	static float m_crOffset;

	static void Initialise(void);
	static void CreateImmediateModeData(RwCamera *cam, RwRect *rect);
	static void UpdateFrontBuffer(void);
	static void AVColourCorrection(void);
	static void Render(void);
};

//#define RELEASE

struct GlobalScene
{
	RpWorld *world;
	RwCamera *camera;
};
extern GlobalScene &Scene;

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
//	void *vmt;			 // VC is different....
//	char name[24];
//	void *colModel;
//	void *twodEffects;
//	short id;
//	ushort refCount;
//	short txdSlot;
//	uchar type; // ModelInfo type
//	uchar num2dEffects;
//	bool freeCol;
};

struct CSimpleModelInfo : CBaseModelInfo
{
//	RpAtomic *atomics[3];
//	float lodDistances[3];
//	uchar numAtomics;
//	uchar alpha;
//	ushort flags;

	void SetAtomic(int n, RpAtomic *atomic);
	void SetAtomic_hook(int n, RpAtomic *atomic);
	void SetAtomicVC_hook(int n, RpAtomic *atomic);
};

struct CClumpModelInfo : CBaseModelInfo
{
//	RpClump *clump;

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
struct Config {
	int neoWorldPipe, neoWorldPipeKey;
	int neoGlossPipe, neoGlossPipeKey;
	bool iCanHasNeoWorld, iCanHasNeoGloss, iCanHasNeoCar, iCanHasNeoRim;
};
extern Config config;
extern int blendstyle, blendkey;
extern int texgenstyle, texgenkey;
extern int neocarpipe, neocarpipekey;
extern int rimlight, rimlightkey;
extern int neowaterdrops, neoblooddrops;
extern int envMapSize;

char *getpath(char *path);
RwImage *readTGA(const char *afilename);

void DefinedState(void);
extern RwD3D8Vertex *blurVertices;
extern RwImVertexIndex *blurIndices;


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

//
// plugins
//
struct GlossMatExt
{
	bool didLookup;
	RwTexture *glosstex;
};
extern RwInt32 GlossOffset;
#define GETGLOSSEXT(m) RWPLUGINOFFSET(GlossMatExt, m, GlossOffset)
void GlossAttach(void);
void hookplugins(void);

extern RwTexture *rampTex;
void reloadRamp(void);

#define MATRIXPRINT(_matrix)                               \
MACRO_START                                                \
{                                                          \
    if (NULL != (_matrix))                                 \
    {                                                      \
        const RwV3d * const _x = &(_matrix)->right;        \
        const RwV3d * const _y = &(_matrix)->up;           \
        const RwV3d * const _z = &(_matrix)->at;           \
        const RwV3d * const _w = &(_matrix)->pos;          \
                                                           \
        printf("[ [ %8.4f, %8.4f, %8.4f, %8.4f ]\n"        \
               "  [ %8.4f, %8.4f, %8.4f, %8.4f ]\n"        \
               "  [ %8.4f, %8.4f, %8.4f, %8.4f ]\n"        \
               "  [ %8.4f, %8.4f, %8.4f, %8.4f ] ]\n"      \
               "  %08x == flags\n",                        \
               _x->x, _x->y, _x->z, (RwReal) 0,            \
               _y->x, _y->y, _y->z, (RwReal) 0,            \
               _z->x, _z->y, _z->z, (RwReal) 0,            \
               _w->x, _w->y, _w->z, (RwReal) 1,            \
               (_matrix)->flags);                          \
     }							   \
}							   \
MACRO_STOP

