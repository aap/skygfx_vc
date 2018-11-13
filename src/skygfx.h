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

// want to use RwEngineInst instead of RwEngineInstance
#undef RWSRCGLOBAL
#define RWSRCGLOBAL(variable) \
   (((RwGlobals *)RwEngineInst)->variable)

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef uintptr_t addr;
#define nil NULL
#define FIELD(type, var, offset) *(type*)((uint8*)var + offset)

#include "MemoryMgr.h"

#define VERSION 0x280

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

	static RwIm2DVertex screenVertices[4];
	static RwImVertexIndex screenIndices[6];

	static void (*nullsub_orig)(void);

	static void Initialise(void);
	static void CreateImmediateModeData(RwCamera *cam, RwRect *rect);
	static void UpdateFrontBuffer(void);
	static void AVColourCorrection(void);
	static void Render(void);
	static void Update(void);
};

//#define RELEASE

struct RsGlobalType
{
	const char *appName;
	int width;
	int height;
	int maximumWidth;
	int maximumHeight;
	int maxFPS;
	int quit;
	void *ps;
/*
	RsInputDevice keyboard;
	RsInputDevice mouse;
	RsInputDevice pad;
*/
};
extern RsGlobalType &RsGlobal;

struct MatFXNothing { int pad[5]; int effect; };

struct MatFXBump
{
	RwFrame *bumpFrame;
	RwTexture *bumpedTex;
	RwTexture *bumpTex;
	float negBumpCoefficient;
	int pad;
	int effect;
};

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

struct MatFXDual
{
	RwTexture *dualTex;
	RwInt32 srcBlend;
	RwInt32 dstBlend;
};


struct MatFX
{
	union {
		MatFXNothing n;
		MatFXBump b;
		MatFXEnv e;
		MatFXDual d;
	} fx[2];
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

enum eWorldPipe
{
	WORLD_DEFAULT,
	WORLD_NEO,
	WORLD_LEEDS,

	WORLD_NUMPIPES
};

enum eCarPipe
{
	CAR_DEFAULT,
	CAR_NEO,
	CAR_LCS,
	CAR_VCS,

	CAR_NUMPIPES
};

// ini switches
struct SkyGFXConfig {
	// These are exported
#define INTPARAMS \
	X(worldPipeSwitch) \
	X(carPipeSwitch) \
	X(trailsSwitch) \
	X(trailsBlur) \
	X(trailsMotionBlur) \
	X(radiosity) \
	X(radiosityIntensity) \
	X(radiosityLimit)
#define FLOATPARAMS \
	X(currentAmbientMultRed) \
	X(currentAmbientMultGreen) \
	X(currentAmbientMultBlue) \
	X(currentBlurAlpha) \
	X(currentBlurOffset) \
	X(leedsWorldPrelightTweakMult) \
	X(leedsWorldPrelightTweakAdd)

#define X(v) int v;
	INTPARAMS
#undef X

#define X(v) float v;
	FLOATPARAMS
#undef X

	int radiosityFilterPasses;
	int radiosityRenderPasses;

	int dualpass;
	int seamfix;

	int neoGlossPipe;
	int blendstyle;
	int texgenstyle;
	int ps2light;
	int rimlight;
	int neowaterdrops, neoblooddrops;
	int envMapSize;
	float leedsEnvMult;

	int disableColourOverlay;
};
extern SkyGFXConfig config;

void errorMessage(char *msg);
char *getpath(char *path);
RwImage *readTGA(const char *afilename);

void DefinedState(void);
extern RwD3D8Vertex *blurVertices;
extern RwImVertexIndex *blurIndices;

struct CMBlur
{
	static bool &ms_bJustInitialised;
	static bool &BlurOn;
	static RwRaster *&pFrontBuffer;	// this holds the last rendered frame
	static RwRaster *pBackBuffer;	// this is a copy of the current frame

	// VCS Radiosity;
	static RwRaster *ms_pRadiosityRaster1;
	static RwRaster *ms_pRadiosityRaster2;
	static RwIm2DVertex ms_radiosityVerts[44];
	static RwImVertexIndex ms_radiosityIndices[7*6];

	static void MotionBlurRender_leeds(RwCamera *cam, uint8 red, uint8 green, uint8 blue, uint8 alpha, uint8 type);
	static void OverlayRender_leeds(RwCamera *cam, RwRaster *frontbuf, RwRGBA *col, uint8 type);
	static void MotionBlurRender_mobile(RwCamera *cam, uint8 red, uint8 green, uint8 blue, uint8 alpha, uint8 type);

	static void RadiosityInit(RwCamera *cam);
	static void RadiosityCreateImmediateData(RwCamera *cam);
	static void RadiosityRender(RwCamera *cam, int limit, int intensity);

	static void Initialise(void);
};

#include "neo.h"

// Switch for other car pipes
class CarPipe : public CustomPipe
{
public:
	// dummy for other pipes
	static void RenderCallback(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags);
	static void Init(void);
};

// Switch for other car pipes
class WorldPipe : public CustomPipe
{
public:
	static WorldPipe *Get(void);
	static void RenderCallback(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags);
	static void Init(void);
};

class LeedsCarPipe : public CustomPipe
{
	static void MakeScreenQuad(void);
	static void MakeQuadTexCoords(bool textureSpace);
	static void RenderReflectionScene(void);
public:
	static RwCamera *reflectionCam;
	static RwTexture *reflectionMask;
	static RwTexture *reflectionTex;
	static RwIm2DVertex screenQuad[4];
	static RwImVertexIndex screenindices[6];
	static void *vertexShader;

	LeedsCarPipe(void);
	static LeedsCarPipe *Get(void);
	void Init(void);
	static void RenderEnvTex(void);
	static void SetupEnvMap(void);
	static void RenderCallback(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags);
};
//extern LeedsCarPipe leedsCarpipe;

void leedsWorldPipeInit(void);
void leedsWorldRenderCallback(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags);

void CClouds__RenderBackground(int16 tr, int16 tg, int16 tb, int16 br, int16 bg, int16 bb, uint8 a);
void CClouds__Render(void);
void RenderEverythingBarCarsPeds(void);
void RenderAlphaListBarCarsPeds(void);


extern void **&RwEngineInst;
extern RpLight *&pAmbient;
extern RpLight *&pDirect;
extern RpLight **pExtraDirectionals;
extern int &NumExtraDirLightsInWorld;
extern int &MatFXMaterialDataOffset;
extern int &MatFXAtomicDataOffset;

extern bool d3d9;

MatFXEnv *getEnvData(RpMaterial *mat);

void drawDualPass(RxD3D8InstanceData *inst);

void _rwD3D8EnableClippingIfNeeded(void *object, RwUInt8 type);

void rxD3D8DefaultRenderCallback(RwResEntry*, void*, RwUInt8, RwUInt32);
void rwD3D8AtomicMatFXRenderCallback(RwResEntry*, void*, RwUInt8, RwUInt32);
RwBool rwD3D8RenderStateIsVertexAlphaEnable(void);
void rwD3D8RenderStateVertexAlphaEnable(RwBool x);
RwBool D3D8AtomicDefaultInstanceCallback(void*, RxD3D8InstanceData*, RwBool);
RwBool D3D8AtomicDefaultReinstanceCallback(void*, RwResEntry*, const RpMeshHeader*, RxD3D8AllInOneInstanceCallBack);

RwBool D3D8AtomicDefaultInstanceCallback_fixed(void*, RxD3D8InstanceData*, RwBool);
RwBool D3D8AtomicDefaultReinstanceCallback_fixed(void*, RwResEntry*, const RpMeshHeader*, RxD3D8AllInOneInstanceCallBack);

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
int rwD3D8RasterIsCubeRaster(RwRaster*);

// matfx
extern addr ApplyEnvMapTextureMatrix_A;
void ApplyEnvMapTextureMatrix(RwTexture *tex, int n, RwFrame *frame);
void ApplyEnvMapTextureMatrix_hook(RwTexture *tex, int n, RwFrame *frame);
void _rpMatFXD3D8AtomicMatFXRenderBlack_fixed(RxD3D8InstanceData *inst);
void _rpMatFXD3D8AtomicMatFXEnvRender_ps2(RxD3D8InstanceData *inst, int flags, int sel, RwTexture *texture, RwTexture *envMap);
void _rpMatFXD3D8AtomicMatFXDefaultRender(RxD3D8InstanceData *inst, int flags, RwTexture *texture);
extern int (*_rpMatFXPipelinesCreate_orig)(void);
int _rpMatFXPipelinesCreate(void);

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

