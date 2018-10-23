#include "skygfx.h"
#include "ini_parser.hpp"
#include "d3d8.h"
#include "d3d8types.h"
#include "debugmenu_public.h"

HMODULE dllModule;
DebugMenuAPI gDebugMenuAPI;
char asipath[MAX_PATH];
int gtaversion = -1;


void enableTrailsSetting(void);
void patchWater(void);

char*
getpath(char *path)
{
	static char tmppath[MAX_PATH];
	FILE *f;

	f = fopen(path, "r");
	if(f){
		fclose(f);
		return path;
	}
	strncpy(tmppath, asipath, MAX_PATH);
	strcat(tmppath, path);
	f = fopen(tmppath, "r");
	if(f){
		fclose(f);
		return tmppath;
	}
	return NULL;
}

static addr rwD3D8RasterIsCubeRaster_A = AddressByVersion<addr>(0, 0, 0, 0x63EE40, 0x63EE90, 0x63DDF0); // VC only
WRAPPER int rwD3D8RasterIsCubeRaster(RwRaster*) { VARJMP(rwD3D8RasterIsCubeRaster_A); }

void **&RwEngineInst = *AddressByVersion<void***>(0x661228, 0x661228, 0x671248, 0x7870C0, 0x7870C8, 0x7860C8);
RpLight *&pAmbient = *AddressByVersion<RpLight**>(0x885B6C, 0x885B1C, 0x895C5C, 0x974B44, 0x974B4C, 0x973B4C);
RpLight *&pDirect = *AddressByVersion<RpLight**>(0x880F7C, 0x880F2C, 0x89106C, 0x94DD40, 0x94DD48, 0x94CD48);
RpLight **pExtraDirectionals = AddressByVersion<RpLight**>(0x60009C, 0x5FFE84, 0x60CE7C, 0x69A140, 0x69A140, 0x699140);
int &NumExtraDirLightsInWorld = *AddressByVersion<int*>(0x64C608, 0x64C608, 0x65C608, 0x94DB48, 0x94DB50, 0x94CB50);
GlobalScene &Scene = *AddressByVersion<GlobalScene*>(0x726768, 0x726768, 0x7368A8, 0x8100B8, 0x8100C0, 0x80F0C0);

RwD3D8Vertex *blurVertices = AddressByVersion<RwD3D8Vertex*>(0x62F780, 0x62F780, 0x63F780, 0x7097A8, 0x7097A8, 0x7087A8);
RwImVertexIndex *blurIndices = AddressByVersion<RwImVertexIndex*>(0x5FDD90, 0x5FDB78, 0x60AB70, 0x697D48, 0x697D48, 0x696D50);
static addr DefinedState_A = AddressByVersion<addr>(0x526330, 0x526570, 0x526500, 0x57F9C0, 0x57F9E0, 0x57F7F0);
WRAPPER void DefinedState(void) { VARJMP(DefinedState_A); }

RsGlobalType &RsGlobal = *AddressByVersion<RsGlobalType*>(0x8F4360, 0, 0, 0, 0, 0);

SkyGFXConfig config;
bool d3d9;




extern "C" {

__declspec(dllexport) int
SkyGFXGetVersion(const char *s)
{
	return VERSION;
}

__declspec(dllexport) int
SkyGFXGetParam(const char *s)
{
#define X(v) if(strcmp(s, #v) == 0) return config.v;
	INTPARAMS
#undef X
	return 0;
}

__declspec(dllexport) void
SkyGFXSetParam(const char *s, int val)
{
#define X(v) if(strcmp(s, #v) == 0) { config.v = val; return; }
	INTPARAMS
#undef X
}

__declspec(dllexport) float
SkyGFXGetParamF(const char *s)
{
#define X(v) if(strcmp(s, #v) == 0) return config.v;
	FLOATPARAMS
#undef X
	return 0;
}

__declspec(dllexport) void
SkyGFXSetParamF(const char *s, float val)
{
#define X(v) if(strcmp(s, #v) == 0) { config.v = val; return; }
	FLOATPARAMS
#undef X
}

__declspec(dllexport) void*
SkyGFXGetParamPtr(const char *s)
{
#define X(v) if(strcmp(s, #v) == 0) return (void*)&config.v;
	INTPARAMS
	FLOATPARAMS
#undef X
	return 0;
}

}

MatFXEnv*
getEnvData(RpMaterial *mat)
{
	MatFX *matfx = *RWPLUGINOFFSET(MatFX*, mat, MatFXMaterialDataOffset);
	if(matfx == nil || matfx->effects != rpMATFXEFFECTENVMAP && matfx->effects != rpMATFXEFFECTBUMPENVMAP)
		return nil;
	MatFXNothing *n = &matfx->fx[0].n;
	if(n->effect == rpMATFXEFFECTENVMAP)
		return (MatFXEnv*)n;
	n = &matfx->fx[1].n;
	if(n->effect == rpMATFXEFFECTENVMAP)
		return (MatFXEnv*)n;
	return nil;
}

void
rwtod3dmat(D3DMATRIX *d3d, RwMatrix *rw)
{
	d3d->m[0][0] = rw->right.x;
	d3d->m[0][1] = rw->right.y;
	d3d->m[0][2] = rw->right.z;
	d3d->m[0][3] = 0.0f;
	d3d->m[1][0] = rw->up.x;
	d3d->m[1][1] = rw->up.y;
	d3d->m[1][2] = rw->up.z;
	d3d->m[1][3] = 0.0f;
	d3d->m[2][0] = rw->at.x;
	d3d->m[2][1] = rw->at.y;
	d3d->m[2][2] = rw->at.z;
	d3d->m[2][3] = 0.0f;
	d3d->m[3][0] = rw->pos.x;
	d3d->m[3][1] = rw->pos.y;
	d3d->m[3][2] = rw->pos.z;
	d3d->m[3][3] = 1.0f;
}

void
d3dtorwmat(RwMatrix *rw, D3DMATRIX *d3d)
{
	rw->right.x	= d3d->m[0][0];
	rw->right.y	= d3d->m[0][1];
	rw->right.z	= d3d->m[0][2]; 
	rw->up.x	= d3d->m[1][0];
	rw->up.y	= d3d->m[1][1];
	rw->up.z	= d3d->m[1][2];
	rw->at.x	= d3d->m[2][0];
	rw->at.y	= d3d->m[2][1];
	rw->at.z	= d3d->m[2][2];
	rw->pos.x	= d3d->m[3][0];
	rw->pos.y	= d3d->m[3][1];
	rw->pos.z	= d3d->m[3][2];
}

void
_rwD3D8EnableClippingIfNeeded(void *object, RwUInt8 type)
{
	int clip;
	if(type == rpATOMIC)
		clip = !RwD3D8CameraIsSphereFullyInsideFrustum((RwCamera*)((RwGlobals*)RwEngineInst)->curCamera,
		                                               RpAtomicGetWorldBoundingSphere((RpAtomic*)object));
	else
		clip = !RwD3D8CameraIsBBoxFullyInsideFrustum((RwCamera*)((RwGlobals*)RwEngineInst)->curCamera,
		                                             &((RpWorldSector*)object)->tightBoundingBox);
	RwD3D8SetRenderState(D3DRS_CLIPPING, clip);
}


RwFrame*
createIIIEnvFrame(void)
{
	RwFrame *f;
	f = RwFrameCreate();
	f->modelling.right.x = 1.0f;
	f->modelling.right.y = 0.0f;
	f->modelling.right.z = 0.0f;
	f->modelling.at.x = 0.0f;
	f->modelling.at.y = 0.0f;
	f->modelling.at.z = 1.0f;
	f->modelling.up.x = 0.0f;
	f->modelling.up.y = 1.0f;
	f->modelling.up.z = 0.0f;
	f->modelling.pos.x = 0.0f;
	f->modelling.pos.y = 0.0f;
	f->modelling.pos.z = 0.0f;
	f->modelling.flags |= 0x20003;
	RwFrameUpdateObjects(f);
	RwFrameGetLTM(f);
	return f;
}

void
drawDualPass(RxD3D8InstanceData *inst)
{
	if(!config.dualpass){
		if(inst->indexBuffer){
			RwD3D8SetIndices(inst->indexBuffer, inst->baseIndex);
			RwD3D8DrawIndexedPrimitive(inst->primType, 0, inst->numVertices, 0, inst->numIndices);
		}else
			RwD3D8DrawPrimitive(inst->primType, inst->baseIndex, inst->numVertices);
		return;
	}

	RwD3D8SetIndices(inst->indexBuffer, inst->baseIndex);

	int hasAlpha, alphafunc, alpharef, zwrite;
	RwD3D8GetRenderState(D3DRS_ALPHABLENDENABLE, &hasAlpha);
	RwD3D8GetRenderState(D3DRS_ZWRITEENABLE, &zwrite);
	if(hasAlpha && zwrite){
		RwD3D8GetRenderState(D3DRS_ALPHAFUNC, &alphafunc);
		RwD3D8GetRenderState(D3DRS_ALPHAREF, &alpharef);

		RwD3D8SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
		RwD3D8SetRenderState(D3DRS_ALPHAREF, 128);

		if(inst->indexBuffer) RwD3D8DrawIndexedPrimitive(inst->primType, 0, inst->numVertices, 0, inst->numIndices);
		else RwD3D8DrawPrimitive(inst->primType, inst->baseIndex, inst->numVertices);

		RwD3D8SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_LESS);
		RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, FALSE);

		if(inst->indexBuffer) RwD3D8DrawIndexedPrimitive(inst->primType, 0, inst->numVertices, 0, inst->numIndices);
		else RwD3D8DrawPrimitive(inst->primType, inst->baseIndex, inst->numVertices);

		RwD3D8SetRenderState(D3DRS_ALPHAFUNC, alphafunc);
		RwD3D8SetRenderState(D3DRS_ALPHAREF, alpharef);
		RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
	}else{
		if(inst->indexBuffer) RwD3D8DrawIndexedPrimitive(inst->primType, 0, inst->numVertices, 0, inst->numIndices);
		else RwD3D8DrawPrimitive(inst->primType, inst->baseIndex, inst->numVertices);
	}
}

static addr dualPassHook_R = AddressByVersion<addr>(0x5DFBD8, 0x5DFE98, 0, 0x678DA8, 0x678DF8, 0x677D58); 
void __declspec(naked)
dualPassHook(void)
{
	_asm{
		lea     eax, [esi-10h]
		push	eax
		call	drawDualPass
		pop	eax
		push	dword ptr dualPassHook_R
		retn
	}
}

void __declspec(naked)
dualPassHook_IIISteam(void)
{
	_asm{
		push	ebx
		call	drawDualPass
		mov	dword ptr [esp], 0x5EE6B2
		retn
	}
}


void
renderSniperTrails(RwRaster *raster)
{
	RwRGBA color;
	color.red = 180;
	color.green = 255;
	color.blue = 180;
	color.alpha = 120;

	DefinedState();
	RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)1);
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, 0);
	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, 0);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, 0);
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, raster);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
	RwUInt32 emissiveColor = D3DCOLOR_ARGB(color.alpha, color.red, color.green, color.blue);
	for(int i = 0; i < 4; i++)
		blurVertices[i].emissiveColor = emissiveColor;
	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, blurVertices, 4, blurIndices, 6);
}

static addr &sniperTrailsHook_unk_addr = *AddressByVersion<addr*>(0, 0, 0, 0x97F888, 0x97F890, 0x97E890);
static addr sniperTrailsHook_R = AddressByVersion<addr>(0, 0, 0, 0x55EB90, 0x55EBB0, 0x55EA80); 

void __declspec(naked)
sniperTrailsHook(void)
{
	_asm{
		push	[esp+28h];
		call	renderSniperTrails
		pop	eax
	}
	sniperTrailsHook_unk_addr = 0;
	_asm{
		push	dword ptr sniperTrailsHook_R
		retn
	}
}

RwFrame*
createVCEnvFrame(void)
{
	RwFrame *f;
	RwV3d axis = { 1.0f, 0.0f, 0.0f };
	f = RwFrameCreate();
	RwMatrixRotate(&f->modelling, &axis, 60.0f, rwCOMBINEREPLACE);
	RwFrameUpdateObjects(f);
	RwFrameGetLTM(f);
	return f;
}

RwTexture*
CreateTextureFilterFlags(RwRaster *raster)
{
	RwTexture *tex = RwTextureCreate(raster);
	tex->filterAddressing = (tex->filterAddressing & 0xFFFFFF00) | 2;
	return tex;
}

WRAPPER void rpMatFXD3D8AtomicMatFXEnvRender_ps2_IIISteam()
{
	__asm
	{
		sub esp, 20
		mov [esp], eax
		mov [esp+4], edx
		mov [esp+8], ecx
		mov ecx, [esp+24h]
		mov [esp+0Ch], ecx
		mov ecx, [esp+28h]
		mov [esp+10h], ecx
		call _rpMatFXD3D8AtomicMatFXEnvRender_ps2
		add esp, 20
		retn
	}
}

WRAPPER void ApplyEnvMapTextureMatrix_hook_IIISteam()
{
	__asm
	{
		push ecx
		push edx
		push eax
		call ApplyEnvMapTextureMatrix_hook
		add esp, 12
		retn

	}
}

struct TxdStore {
	static void PushCurrentTxd(void);
	static int PopCurrentTxd(void);
	static int FindTxdSlot(char*);
	static void SetCurrentTxd(int);
	//static char *GetTxdName(int handle);
};

static addr TxdStore_PushCurrentTxd_A = AddressByVersion<addr>(0x527900, 0x527B40, 0x527AD0, 0, 0, 0);
WRAPPER void TxdStore::PushCurrentTxd(void) { VARJMP(TxdStore_PushCurrentTxd_A); }
static addr TxdStore_PopCurrentTxd_A = AddressByVersion<addr>(0x527910, 0x527B50, 0x527AE0, 0, 0, 0);
WRAPPER int TxdStore::PopCurrentTxd(void) { VARJMP(TxdStore_PopCurrentTxd_A); }
static addr TxdStore_FindTxdSlot_A = AddressByVersion<addr>(0x5275D0, 0x527810, 0x5277A0, 0x580D70, 0, 0);
WRAPPER int TxdStore::FindTxdSlot(char*) { VARJMP(TxdStore_FindTxdSlot_A); }
static addr TxdStore_SetCurrentTxd_A = AddressByVersion<addr>(0x5278C0, 0x527B00, 0x527A90, 0, 0, 0);
WRAPPER void TxdStore::SetCurrentTxd(int) { VARJMP(TxdStore_SetCurrentTxd_A); }

//static addr TxdStore_GetTxdName_A = AddressByVersion<addr>(0, 0, 0, 0x580E50, 0, 0);
//WRAPPER void TxdStore::GetTxdName(int) { VARJMP(TxdStore_SetCurrentTxd_A); }

static int &gameTxdSlot = *AddressByVersion<int*>(0x628D88, 0x628D88, 0x638D88, 0, 0, 0); // only needed in III

RwTexture*
RwTextureRead_generic(char *name, char *mask)
{
	RwTexture *tex;
	tex = RwTextureRead(name, mask);
	if(tex)
		return tex;
	TxdStore::PushCurrentTxd();
	TxdStore::SetCurrentTxd(gameTxdSlot);
	tex = RwTextureRead(name, mask);
	TxdStore::PopCurrentTxd();
	return tex;
}

#if 0

void *curvePS;

void
setcurveps(void)
{
	if(curvePS == NULL){
		HRSRC resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_CURVEPS), RT_RCDATA);
		RwUInt32 *shader = (RwUInt32*)LoadResource(dllModule, resource);
		RwD3D9CreatePixelShader(shader, &curvePS);
		assert(curvePS);
		FreeResource(shader);
	}
	RwD3D9SetIm2DPixelShader(curvePS);
}

class CMBlur {
public: static RwRaster *&pFrontBuffer;
	static void CreateImmediateModeData(RwCamera*, RwRect*);
};
RwRaster *&CMBlur::pFrontBuffer = *AddressByVersion<RwRaster**>(0x8E2C48, 0x8E2CFC, 0x8F2E3C, 0x9753A4, 0x9753AC, 0x9743AC);
WRAPPER void CMBlur::CreateImmediateModeData(RwCamera*, RwRect*) { EAXJMP(0x50A800); }

unsigned int curveIdx;

#define KEYDOWN(k) (GetAsyncKeyState(k) & 0x8000)
void
applyCurve(void)
{
	RwCamera *cam = *(RwCamera**)(0x6FACF8 + 0x7A0);

	if(GetAsyncKeyState(VK_F9) & 0x8000)
		return;


	{
		static bool keystate = false;
		if(KEYDOWN(VK_CONTROL) && KEYDOWN(VK_LEFT)){
			if(!keystate){
				keystate = true;
				curveIdx = (curveIdx-1) % 256;
			}
		}else
			keystate = false;
	}
	{
		static bool keystate = false;
		if(KEYDOWN(VK_CONTROL) && KEYDOWN(VK_RIGHT)){
			if(!keystate){
				keystate = true;
				curveIdx = (curveIdx+1) % 256;
			}
		}else
			keystate = false;
	}	
	{
		static bool keystate = false;
		if(KEYDOWN(VK_CONTROL) && KEYDOWN(VK_DOWN)){
			if(!keystate){
				keystate = true;
				curveIdx = 0;
			}
		}else
			keystate = false;
	}	

	static RwRaster *rampRaster = NULL;
	if(rampTex == NULL){
		reloadRamp();
		rampTex->filterAddressing = 0x3301;
		int w, h;
		for(w = 1; w < cam->frameBuffer->width; w <<= 1);
		for(h = 1; h < cam->frameBuffer->height; h <<= 1);
		rampRaster = RwRasterCreate(w, h, 0, 5);
		RwRect rect = { 0, 0, w, h };
		CMBlur::CreateImmediateModeData(cam, &rect);
	}

	RwRasterPushContext(rampRaster);
	RwRasterRenderFast(cam->frameBuffer, 0, 0);
	RwRasterPopContext();

	float v = curveIdx/255.0f;
	RwD3D9SetPixelShaderConstant(0, &v, 1);
	RwD3D8SetTexture(rampTex, 1);

	DefinedState();
	RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERNEAREST);
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, 0);
	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, 0);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, 0);
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, rampRaster);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, 0);
	setcurveps();
	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, blurVertices, 4, blurIndices, 6);
	RwD3D9SetIm2DPixelShader(NULL);
	DefinedState();
	RwD3D8SetTexture(NULL, 1);
}

void __declspec(naked)
curvehook(void)
{
	_asm{
		pop	ebx
		call	applyCurve
		retn
	}
}
#endif

unsigned __int64 rand_seed = 1;
float ps2randnormalize = 1.0f/0x7FFFFFFF;

int ps2rand()
{
	rand_seed = 0x5851F42D4C957F2D * rand_seed + 1;
	return ((rand_seed >> 32) & 0x7FFFFFFF);
}

void ps2srand(unsigned int seed)
{
	rand_seed = seed;
}

struct CVector { float x, y, z; };

WRAPPER void CParticle__AddParticle(int partcltype,CVector*,CVector*,void*entity,float,uint *rwrgba,int,int,int,int) { EAXJMP(0x50D190); }

void
footsplash_ps2(uchar *ebx, uchar *esp)
{
	RwMatrix *mtx;
	CVector *speed;
	CVector vec1, vec2;
	uint col;
	mtx = (RwMatrix*)(ebx+4);
	speed = (CVector*)(ebx+0x78);

	vec1.x = mtx->pos.x - mtx->at.x*0.3f + mtx->up.x*0.3f;
	vec1.y = mtx->pos.y - mtx->at.y*0.3f + mtx->up.y*0.3f;
	vec1.z = mtx->pos.z - mtx->at.z*0.3f + mtx->up.z*0.3f - 1.2f;
	vec2.x = speed->x*0.45;
	vec2.y = speed->y*0.45;
	vec2.z = (ps2rand()&0xFFFF)/65536.0f * 0.02f + 0.03f;
	col = *(uint*)0x5F85C4;
	CParticle__AddParticle(44, &vec1, &vec2, NULL, 0.0f, &col, 0, 0, 0, 0);

	if(config.neowaterdrops == 2)
		WaterDrops::FillScreenMoving(0.1f, false);
}

void __declspec(naked)
footsplash_hook(void)
{
	_asm {
		push	esp
		push	ebx
		call	footsplash_ps2
		add	esp,8
		push	0x4CCDC3
		retn
	}
}

static RwTexture *flametex[45];
static RwRaster *flameras[45];

void
loadNeoFlameTextures(void)
{
	char texname[8];
	int i;
	for(i = 1; i <= 45; i++){
		sprintf(texname, "flame%02d", i);
		flametex[i-1] = RwTextureRead(texname, nil);
		if(flametex[i-1])
			flameras[i-1] = flametex[i-1]->raster;
	}
}

void __declspec(naked)
neoflame_hook_iii(void)
{
	_asm {
		call	loadNeoFlameTextures
		call	RwTextureRead
		push	0x50C8D8
		retn
	}
}

void __declspec(naked)
neoflame_hook_vc(void)
{
	_asm {
		call	loadNeoFlameTextures
		call	RwTextureRead
		push	0x56522B
		retn
	}
}


// BETA sliding in oddjob2 text for III, thanks Fire_Head for finding this
float &OddJob2XOffset = *(float*)0x8F1B5C;
WRAPPER void CFont__PrintString(float x, float y, short *str) { EAXJMP(0x500F50); }
void
CFont__PrintString__Oddjob2(float x, float y, short *str)
{
	CFont__PrintString(x - OddJob2XOffset * RsGlobal.width/640.0f, y, str);
}

void (*CMBlur__MotionBlurRenderIII_orig)(RwCamera*, RwUInt8, RwUInt8, RwUInt8, RwUInt8, int, int);
void
CMBlur__MotionBlurRenderIII(RwCamera *cam, RwUInt8 red, RwUInt8 green, RwUInt8 blue, RwUInt8 alpha, int type, int bluralpha)
{
	// REMOVE: we do this in InitialiseGame_hook now
	if(config.trailsSwitch < 0) config.trailsSwitch = 0;
	if(config.disableColourOverlay)
		return;
	switch(config.trailsSwitch){
	default:
	case 0: CMBlur__MotionBlurRenderIII_orig(cam, red, green, blue, alpha, type, bluralpha); break;
	case 1: CMBlur::MotionBlurRender_leeds(cam, red, green, blue, alpha, type); break;
	case 2: CMBlur::MotionBlurRender_mobile(cam, red, green, blue, alpha, type); break;
	}
}

void (*CMBlur__MotionBlurRenderVC_orig)(RwCamera*, RwUInt8, RwUInt8, RwUInt8, RwUInt8, int);
void
CMBlur__MotionBlurRenderVC(RwCamera *cam, RwUInt8 red, RwUInt8 green, RwUInt8 blue, RwUInt8 alpha, int type)
{
	// REMOVE: we do this in InitialiseGame_hook now
	if(config.trailsSwitch < 0) config.trailsSwitch = 0;
	if(config.disableColourOverlay)
		return;
	switch(config.trailsSwitch){
	default:
	case 0:	CMBlur__MotionBlurRenderVC_orig(cam, red, green, blue, alpha, type); break;
	case 1: CMBlur::MotionBlurRender_leeds(cam, red, green, blue, alpha, type); break;
	case 2: CMBlur::MotionBlurRender_mobile(cam, red, green, blue, alpha, type); break;
	}
}

//
// real time reflection test; III 1.0
//
/*
WRAPPER RpAtomic *CVehicleModelInfo__SetEnvironmentMapCB(RpAtomic*, RwTexture*) { EAXJMP(0x521820); }
void RenderEffects(void);

RwTexture *realtimeenvmap;

RpAtomic*
CVehicleModelInfo__SetEnvironmentMapCB_hook(RpAtomic *atomic, RwTexture *envmap)
{
	RwCamera *cam = *(RwCamera**)(0x6FACF8 + 0x7A0);
	if(realtimeenvmap == NULL){
		int w, h;
		for(w = 1; w < cam->frameBuffer->width; w <<= 1);
		for(h = 1; h < cam->frameBuffer->height; h <<= 1);
		RwRaster *envraster = RwRasterCreate(w, h, 0, 5);
		realtimeenvmap = RwTextureCreate(envraster);
		realtimeenvmap->filterAddressing = 0x1102;
	}
	return CVehicleModelInfo__SetEnvironmentMapCB(atomic, realtimeenvmap);
}

void
RenderEffectsHook(void)
{
	RenderEffects();
	RwCamera *cam = *(RwCamera**)(0x6FACF8 + 0x7A0);
	RwRasterPushContext(realtimeenvmap->raster);
	RwRasterRenderFast(cam->frameBuffer, 0, 0);
	RwRasterPopContext();
}
*/

int dontnag;
void
errorMessage(char *msg)
{
	if(dontnag) return;
	MessageBox(NULL, msg, "Error - SkyGFX", MB_ICONERROR | MB_OK);
}

#define ONCE do{ static int once = 0; assert(once == 0); once = 1; }while(0)

int
readhex(const char *str)
{
	int n = 0;
	if(strlen(str) > 2)
		sscanf(str+2, "%X", &n);
	return n;
}

int
readint(const std::string &s, int default = 0)
{
	try{
		return std::stoi(s);
	}catch(...){
		return default;
	}
}

float
readfloat(const std::string &s, float default = 0)
{
	try{
		return std::stof(s);
	}catch(...){
		return default;
	}
}

void (*InitialiseGame)(void);
void
InitialiseGame_hook(void)
{
	static bool postrw_once;
	// Post RW Init
	if(!postrw_once){
		d3d9 = RwD3D9Supported();

		// This is where we can sanitize some values:
		if(config.trailsSwitch < 0) config.trailsSwitch = 0;
		if(config.radiosity < 0) config.radiosity = 0;
		if(config.leedsEnvMult < 0) config.leedsEnvMult = isIII() ? 0.22f : 0.4f;
		if(config.leedsWorldPrelightTweakMult < 0) config.leedsWorldPrelightTweakMult = 1.0f;
		if(config.leedsWorldPrelightTweakAdd < 0) config.leedsWorldPrelightTweakAdd = 0.0f;


		if (isIII()) // fall back to generic.txd when reading from dff
			InjectHook(AddressByVersion<addr>(0x5AAE1B, 0x5AB0DB, 0x5AD708, 0, 0, 0), RwTextureRead_generic);
		CarPipe::Init();
		WorldPipe::Init();
		LeedsCarPipe::Get()->Init();

		neoInit();

		if(config.rimlight < 0) config.rimlight = 0;
		if(config.neoGlossPipe < 0) config.neoGlossPipe = 0;
		if(config.carPipeSwitch < 0) config.carPipeSwitch = 0;
		if(config.worldPipeSwitch < 0) config.worldPipeSwitch = 0;

		postrw_once = true;
	}

	InitialiseGame();
}

int (*RsEventHandler_orig)(int a, int b);
int
delayedPatches(int a, int b)
{
	if(DebugMenuLoad()){
		DebugMenuEntry *e;
		static const char *ps2pcmobile[] = { "PS2", "PC", "Mobile" };
		static const char *pcps2[] = { "PC", "PS2" };
		e = DebugMenuAddVar("SkyGFX", "MatFX blend", &config.blendstyle, nil, 1, 0, 2, ps2pcmobile);
		DebugMenuEntrySetWrap(e, true);
		e = DebugMenuAddVar("SkyGFX", "MatFX texgen", &config.texgenstyle, nil, 1, 0, 2, ps2pcmobile);
		DebugMenuEntrySetWrap(e, true);
		e = DebugMenuAddVar("SkyGFX", "MatFX PS2 lighting", &config.ps2light, nil, 1, 0, 1, pcps2);
		DebugMenuEntrySetWrap(e, true);
		DebugMenuAddVarBool32("SkyGFX", "Dual pass", &config.dualpass, nil);
		static const char *pipeStr[] = {
			"invalid", "default", "neo", "lcs", "vcs"
		};
		static const char *postfxStr[] = {
			"invalid", "default", "leeds", "mobile"
		};
		DebugMenuAddVar("SkyGFX", "World pipe", &config.worldPipeSwitch, nil, 1, WORLD_DEFAULT, WORLD_LEEDS, pipeStr+1);
		DebugMenuAddVar("SkyGFX", "Car pipe", &config.carPipeSwitch, nil, 1, CAR_DEFAULT, CAR_VCS, pipeStr+1);
		DebugMenuAddVar("SkyGFX|Advanced", "Leeds Car Env Mult", &config.leedsEnvMult, nil, 0.1f, 0.0f, 2.0f);
		DebugMenuAddVar("SkyGFX|Advanced", "Trails switch", &config.trailsSwitch, nil, 1, 0, 2, postfxStr+1);

		DebugMenuAddVarBool32("SkyGFX", "Neo rim light pipe", &config.rimlight, nil);
		DebugMenuAddVarBool32("SkyGFX", "Neo gloss pipe", &config.neoGlossPipe, nil);
		if(config.neowaterdrops){
			static const char *dropstrings[] = { "None", "Neo", "Extended" };
			DebugMenuAddVar("SkyGFX", "Neo water drops", &config.neowaterdrops, nil, 1, 0, 2, dropstrings);
			DebugMenuAddVarBool32("SkyGFX", "Neo-style blood drops", &config.neoblooddrops, nil);
		}

//		if(config.disableColourOverlay >= 0)
			DebugMenuAddVarBool32("SkyGFX", "Disable Colour Overlay", &config.disableColourOverlay, nil);

		void neoMenu();
		neoMenu();
		void leedsMenu();
		leedsMenu();

		extern int seamOffX;
		extern int seamOffY;
		DebugMenuAddVarBool32("SkyGFX", "Seam fixer", &config.seamfix, nil);
		DebugMenuAddVar("SkyGFX", "Seam Offset X", &seamOffX, nil, 1, -10, 10, nil);
		DebugMenuAddVar("SkyGFX", "Seam Offset Y", &seamOffY, nil, 1, -10, 10, nil);


		DebugMenuAddVarBool8("SkyGFX|ScreenFX", "Enable YCbCr tweak", (int8_t*)&ScreenFX::m_bYCbCrFilter, nil);
		DebugMenuAddVar("SkyGFX|ScreenFX", "Y scale", &ScreenFX::m_lumaScale, nil, 0.004f, 0.0f, 10.0f);
		DebugMenuAddVar("SkyGFX|ScreenFX", "Y offset", &ScreenFX::m_lumaOffset, nil, 0.004f, -1.0f, 1.0f);
		DebugMenuAddVar("SkyGFX|ScreenFX", "Cb scale", &ScreenFX::m_cbScale, nil, 0.004f, 0.0f, 10.0f);
		DebugMenuAddVar("SkyGFX|ScreenFX", "Cb offset", &ScreenFX::m_cbOffset, nil, 0.004f, -1.0f, 1.0f);
		DebugMenuAddVar("SkyGFX|ScreenFX", "Cr scale", &ScreenFX::m_crScale, nil, 0.004f, 0.0f, 10.0f);
		DebugMenuAddVar("SkyGFX|ScreenFX", "Cr offset", &ScreenFX::m_crOffset, nil, 0.004f, -1.0f, 1.0f);

#ifdef DEBUG
	extern float envmult;
	extern bool ps2filtering;
	extern bool envdebug;

	if(gtaversion == III_10){
		DebugMenuAddVar("SkyGFX", "Env Mult", &envmult, nil, 0.05f, 0.0f, 2.0f);
		DebugMenuAddVarBool8("SkyGFX", "ps2 env filter", (int8*)&ps2filtering, nil);
#ifdef ENVTEST
		DebugMenuAddVarBool8("SkyGFX", "env debug", (int8*)&envdebug, nil);
#endif
	}
#endif
	}

	return RsEventHandler_orig(a, b);
}

//void (*RwCameraEndUpdate_hooked)(RwCamera *cam);
//void endOfFrame(RwCamera *cam)
//{
//	ScreenFX::Render();
//	RwCameraEndUpdate_hooked(cam);
//}

void
patch(void)
{
	using namespace std;
//	char tmp[32];
	string tmp;
	char modulePath[MAX_PATH];

	config.trailsBlur = 1;
	config.trailsMotionBlur = 1;
	config.currentBlurAlpha = 39.0f;
	config.currentBlurOffset = 2.1f;

	// Fail if RenderWare has already been started
	if(Scene.camera){
		MessageBox(NULL, "SkyGFX cannot be loaded by the default Mss32 ASI loader.\nUse another ASI loader.", "Error", MB_ICONERROR | MB_OK);
		return;
	}

	GetModuleFileName(dllModule, modulePath, MAX_PATH);
	char *p = strrchr(modulePath, '\\');
	if(p) p[1] = '\0';
	strncpy(asipath, modulePath, MAX_PATH);
	strcat(modulePath, "skygfx.ini");
	linb::ini cfg;
	cfg.load_file(modulePath);

	if(gtaversion == III_10 || gtaversion == VC_10){
		InterceptCall(&ScreenFX::nullsub_orig, ScreenFX::Render, AddressByVersion<uint32_t>(0x48D445, 0, 0, 0x4A6151, 0, 0));
//		InterceptCall(&RwCameraEndUpdate_hooked, endOfFrame, AddressByVersion<uint32_t>(0x48D450, 0, 0, 0x4A615C, 0, 0));
		InterceptCall(&RsEventHandler_orig, delayedPatches, AddressByVersion<uint32_t>(0x58275E, 0, 0, 0x5FFAFE, 0, 0));
	}

	// ADDRESS
	if(gtaversion == III_10 || gtaversion == VC_10){
		// Everything that is initialized with RenderWare
		//InterceptCall(&InitialiseRenderWare, InitialiseRenderWare_hook, AddressByVersion<addr>(0x48D52F, 0, 0, 0x4A5B6B, 0, 0));
		// Everything that is initialized whenever a game is started
		InterceptCall(&InitialiseGame, InitialiseGame_hook, AddressByVersion<addr>(0x582E6C, 0, 0, 0x600411, 0, 0));

		hookplugins();
	}

	dontnag = readint(cfg.get("SkyGfx", "dontNag", ""), 0);
	config.seamfix = readint(cfg.get("SkyGfx", "seamFix", ""), 0);

//config.seamfix = 0;

	// set to reasonable values when used
	config.radiosity = readint(cfg.get("SkyGfx", "radiosity", ""), -1);
	config.trailsSwitch = readint(cfg.get("SkyGfx", "trailsSwitch", ""), -1);
	config.leedsWorldPrelightTweakMult = readfloat(cfg.get("SkyGfx", "leedsWorldPrelightTweakMult", ""), -9999.0f);
	config.leedsWorldPrelightTweakAdd = readfloat(cfg.get("SkyGfx", "leedsWorldPrelightTweakAdd", ""), -9999.0f);

	// TODO_INI
	config.radiosityRenderPasses = 2;
	config.radiosityFilterPasses = 4;

	config.disableColourOverlay = readint(cfg.get("SkyGfx", "disableColourOverlay", ""), -1);
	if(config.disableColourOverlay < 0) config.disableColourOverlay = 0;
// TEMP
//	if(disableColourOverlay >= 0){
		if(gtaversion == III_10)
			InterceptCall(&CMBlur__MotionBlurRenderIII_orig, CMBlur__MotionBlurRenderIII, 0x46F978);
		else if(gtaversion == VC_10)
			InterceptCall(&CMBlur__MotionBlurRenderVC_orig, CMBlur__MotionBlurRenderVC, 0x46BE0F);
//	}

	int ps2water = readint(cfg.get("SkyGfx", "ps2Water", ""), 0);

	if(gtaversion == III_10)
		InjectHook(AddressByVersion<addr>(0x5D0D2B, 0x0, 0, 0x0, 0x0, 0x0), _rpMatFXD3D8AtomicMatFXRenderBlack_fixed);
	tmp = cfg.get("SkyGfx", "texblendSwitch", "");
	if(tmp != ""){
		config.blendstyle = readint(tmp);

		// MatFX env coefficient on cars
		static float envCoeff = 1.0f;
//		static float envCoeff = 0.5f;
		Patch(AddressByVersion<addr>(0x5217DF, 0, 0, 0x578B7B, 0, 0) + 2, &envCoeff);
		if(ps2water && gtaversion == VC_10)
			patchWater();

		// disable car colour textures (messes up matfx)
		// but it could be intended...what to do?
		// TODO_INI
		if(gtaversion == III_10){
			Nop(0x520EEA, 2);
			Nop(0x520F8A, 2);
		}

		InterceptCall(&_rpMatFXPipelinesCreate_orig, _rpMatFXPipelinesCreate, AddressByVersion<addr>(0x5B27B1, 0, 0, 0x656031, 0, 0));

		void _rwD3D8AtomicMatFXRenderCallback(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags);
		InjectHook(AddressByVersion<addr>(0x5D0B80, 0, 0, 0x676460, 0, 0), _rwD3D8AtomicMatFXRenderCallback, PATCH_JUMP);

	// no longer needed
	//	if(gtaversion != III_STEAM)
	//		InjectHook(AddressByVersion<addr>(0x5D0CE8, 0x5D0FA8, 0, 0x6765C8, 0x676618, 0x675578), _rpMatFXD3D8AtomicMatFXEnvRender_ps2);
	//	else
	//		InjectHook(0x5D8D37, rpMatFXD3D8AtomicMatFXEnvRender_ps2_IIISteam);
	}

	config.blendstyle %= 3;
	tmp = cfg.get("SkyGfx", "texgenSwitch", "");
	if(tmp != ""){
		config.texgenstyle = readint(tmp);
		if (gtaversion != III_STEAM)
			InjectHook(ApplyEnvMapTextureMatrix_A, ApplyEnvMapTextureMatrix_hook, PATCH_JUMP);
		else
			InjectHook(ApplyEnvMapTextureMatrix_A, ApplyEnvMapTextureMatrix_hook_IIISteam, PATCH_JUMP);
	}
	config.texgenstyle %= 3;

	config.ps2light = readint(cfg.get("SkyGfx", "ps2light", "1"));

	if(isVC() && readint(cfg.get("SkyGfx", "IIIEnvFrame", ""))){
		InjectHook(AddressByVersion<addr>(0, 0, 0, 0x57A8BA, 0x57A8DA, 0x57A7AA), createIIIEnvFrame);
		InjectHook(AddressByVersion<addr>(0, 0, 0, 0x57A8C7, 0x57A8E7, 0x57A7B7),
				   AddressByVersion<addr>(0, 0, 0, 0x57A8F4, 0x57A914, 0x57A7E4), PATCH_JUMP);
	}
	if(isIII() && readint(cfg.get("SkyGfx", "VCEnvFrame", ""))){
		InjectHook(AddressByVersion<addr>(0x5218A2, 0x521AE2, 0x521A72, 0, 0, 0), createVCEnvFrame);
		InjectHook(AddressByVersion<addr>(0x5218AC, 0x521AEC, 0x521A7C, 0, 0, 0),
				   AddressByVersion<addr>(0x52195E, 0x521B9E, 0x521B2E, 0, 0, 0), PATCH_JUMP);
	}

	if(isIII())
		InjectHook(AddressByVersion<addr>(0x59BABF, 0x59BD7F, 0x598E6F, 0, 0, 0), CreateTextureFilterFlags);

	config.carPipeSwitch = readint(cfg.get("SkyGfx", "carPipe", ""), -1);
	config.worldPipeSwitch = readint(cfg.get("SkyGfx", "worldPipe", ""), -1);

	config.envMapSize = readint(cfg.get("SkyGfx", "envMapSize", ""), 128);
	int n = 1;
	while(n < config.envMapSize) n *= 2;
	config.envMapSize = n;
	// set to a reasonable value when used
	config.leedsEnvMult = readfloat(cfg.get("SkyGfx", "leedsEnvMult", ""), -9999.0f);

	config.rimlight = readint(cfg.get("SkyGfx", "neoRimLightPipe", ""), -1);
	config.neoGlossPipe = readint(cfg.get("SkyGfx", "neoGlossPipe", ""), -1);

	tmp = cfg.get("SkyGfx", "dualPass", "");
	if(tmp != ""){
		config.dualpass = readint(tmp);
		if (gtaversion != III_STEAM)
			InjectHook(AddressByVersion<addr>(0x5DFB99, 0x5DFE59, 0, 0x678D69, 0x678DB9, 0x677D19), dualPassHook, PATCH_JUMP);
		else
			InjectHook(0x5EE675, dualPassHook_IIISteam, PATCH_JUMP);
		InjectHook(AddressByVersion<addr>(0x5CEB80, 0x5CEE40, 0x5DB760, 0x674380, 0x6743D0, 0x673330), _rpMatFXD3D8AtomicMatFXDefaultRender, PATCH_JUMP);
	}

	if(isVC() && readint(cfg.get("SkyGfx", "disableBackfaceCulling", ""), 0)){
		// hope I didn't miss anything
		Patch<uchar>(AddressByVersion<addr>(0, 0, 0, 0x4C9E5F, 0x4C9E7F, 0x4C9D1F), 1);	// in CRenderer::RenderOneNonRoad()
		Patch<uchar>(AddressByVersion<addr>(0, 0, 0, 0x4C9F08, 0x4C9F28, 0x4C9DC8), 1);	// in CRenderer::RenderBoats()
		Patch<uchar>(AddressByVersion<addr>(0, 0, 0, 0x4C9F5D, 0x4C9F7D, 0x4C9E1D), 1);	// in CRenderer::RenderEverythingBarRoads()
		Patch<uchar>(AddressByVersion<addr>(0, 0, 0, 0x4CA157, 0x4CA177, 0x4CA017), 1);	// in CRenderer::RenderFadingInEntities()
		Patch<uchar>(AddressByVersion<addr>(0, 0, 0, 0x4CA199, 0x4CA1B9, 0x4CA059), 1);	// in CRenderer::RenderRoads()
		// ADDRESS
		if(gtaversion == VC_10)
			Patch<uchar>(AddressByVersion<addr>(0, 0, 0, 0x4E0146, 0, 0), 1);	// in CCutsceneObject::Render()
	}

	// fix blend mode
	if(isVC()){
		// auto aim
		Patch<uchar>(AddressByVersion<addr>(0, 0, 0, 0x5D4EEE, 0x5D4F0E, 0x5D4CBE), rwBLENDINVSRCALPHA);
		// sniper dot
		Patch<uchar>(AddressByVersion<addr>(0, 0, 0, 0x558024, 0x558044, 0x557F14), rwBLENDINVSRCALPHA);
	}

	if(isVC())
		InjectHook(AddressByVersion<addr>(0, 0, 0, 0x55EA39, 0x55EA59, 0x55E929), sniperTrailsHook, PATCH_JUMP);

	config.neowaterdrops = readint(cfg.get("SkyGfx", "neoWaterDrops", ""));
	config.neoblooddrops = readint(cfg.get("SkyGfx", "neoBloodDrops", ""));
	if(config.neowaterdrops)
		hookWaterDrops();

	if(readint(cfg.get("SkyGfx", "replaceDefaultPipeline", ""))){
		if(gtaversion == III_10){
			Patch(0x5DB427 +2, D3D8AtomicDefaultInstanceCallback_fixed);
			Patch(0x5DB42D +3, D3D8AtomicDefaultReinstanceCallback_fixed);
			Patch(0x5DB43B +3, rxD3D8DefaultRenderCallback_xbox);
		}else if(gtaversion == VC_10){
			Patch(0x67BAB7 +2, D3D8AtomicDefaultInstanceCallback_fixed);
			Patch(0x67BABD +3, D3D8AtomicDefaultReinstanceCallback_fixed);
			Patch(0x67BACB +3, rxD3D8DefaultRenderCallback_xbox);
		}
	}

	if(gtaversion == III_10 && readint(cfg.get("SkyGfx", "ps2FootSplash", ""))){
		// footsplash stuff - needs ps2 particle.cfg PED_SPLASH
		static float randscl = 1/63556.0f;
		static float splashscl = 0.4f;
		static float splashadd = -0.2f;
		InjectHook(0x4CC4EA, ps2rand);
		InjectHook(0x4CC514, ps2rand);
		InjectHook(0x4CC53E, ps2rand);
		Patch<float*>(0x4CC4FA +2, &randscl);
		Patch<float*>(0x4CC524 +2, &randscl);
		Patch<float*>(0x4CC500 +2, &splashscl);
		Patch<float*>(0x4CC52A +2, &splashscl);
		Patch<float*>(0x4CC506 +2, &splashadd);
		Patch<float*>(0x4CC530 +2, &splashadd);
		InjectHook(0x4CCC1A, footsplash_hook, PATCH_JUMP);
	}

	if(readint(cfg.get("SkyGfx", "ps2Loadscreen", ""))){
		if(gtaversion == III_10){
			// ff 74 24 60             push   DWORD PTR [esp+0x60]
			Patch<uint>(0x48D774, 0x602474ff);
			InjectHook(0x48D778, 0x48D79B, PATCH_JUMP);
		}
		if(gtaversion == VC_10){
			// BREAKS WITH OLD OLA
			Nop(0x4A69D4, 1);
			// ff 74 24 78             push   DWORD PTR [esp+0x78]
			Patch(0x4A69D4+1, 0x782474ff);
		}
	}

/*
	// load xbox flames
	if(gtaversion == III_10){
		InjectHook(0x50C8D3, neoflame_hook_iii, PATCH_JUMP);
		Patch(0x50CB0E + 6, flameras);
		Patch(0x50CBA4 + 6, flameras);
	}else if(gtaversion == VC_10){
		InjectHook(0x565226, neoflame_hook_vc, PATCH_JUMP);
		Patch(0x5655C7 + 6, flameras);
		Patch(0x565671 + 6, flameras);
	}
*/

	ScreenFX::m_bYCbCrFilter = readint(cfg.get("SkyGfx", "YCbCrCorrection", ""), 0);
	ScreenFX::m_lumaScale = readfloat(cfg.get("SkyGfx", "lumaScale", ""), 219.0f/255.0f);
	ScreenFX::m_lumaOffset = readfloat(cfg.get("SkyGfx", "lumaOffset", ""), 16.0f/255.0f);
	ScreenFX::m_cbScale = readfloat(cfg.get("SkyGfx", "CbScale", ""), 1.23f);
	ScreenFX::m_cbOffset = readfloat(cfg.get("SkyGfx", "CbOffset", ""), 0.0f);
	ScreenFX::m_crScale = readfloat(cfg.get("SkyGfx", "CrScale", ""), 1.23f);
	ScreenFX::m_crOffset = readfloat(cfg.get("SkyGfx", "CrOffset", ""), 0.0f);

	enableTrailsSetting();

//	Nop(0x4A6594, 5);	// water
//	Nop(0x4A65AE, 5);	// transparent water

	// disable inlined RenderOneFlatLargeWaterPoly
//	InjectHook(0x5C1265, 0x5C1442, PATCH_JUMP);
	// disable RenderOneFlatSmallWaterPolyBlended
//	InjectHook(0x5BD0A0, 0x5BD73D, PATCH_JUMP);
	// wavy atomic
//	Nop(0x5C0C46, 3);
//	Nop(0x5C0E15, 3);
//	Nop(0x5C0FEB, 3);
//	Nop(0x5C11C0, 3);
	// mask atomic
//	Nop(0x5C1579, 3);

	// TODO_INI
	// use get-in-vehicle camera from PS2 (thanks, Fire_Head)
	if(gtaversion == III_10){
		Nop(0x4713DB, 2);
		Nop(0x47143B, 2);
	}

#ifdef DEBUG
	if(gtaversion == III_10){
		// ignore txd.img
		InjectHook(0x48C12E, 0x48C14C, PATCH_JUMP);

#if 0
		int i = readint(cfg.get("SkyGfx", "curve", ""), -1);
		if(i >= 0){
			curveIdx = i % 256;
			InjectHook(0x48E44B, curvehook, PATCH_JUMP);
		}
#endif
//		InjectHook(0x405DB0, printf, PATCH_JUMP);


		//MemoryVP::InjectHook(0x48E603, RenderEffectsHook);
		//MemoryVP::InjectHook(0x5219B3, CVehicleModelInfo__SetEnvironmentMapCB_hook);
		//MemoryVP::Patch<void*>(0x521986+1, CVehicleModelInfo__SetEnvironmentMapCB_hook);

		// beta sliding in text
		InjectHook(0x509DDE, CFont__PrintString__Oddjob2);
		InjectHook(0x509E51, CFont__PrintString__Oddjob2);
		

		// clear framebuffer
		Patch<uchar>(0x48CFC1+1, 3);
		Patch<uchar>(0x48D0AD+1, 3);
		Patch<uchar>(0x48E6A7+1, 3);
		Patch<uchar>(0x48E78C+1, 3);
	}
#endif
#ifdef DEBUG
	if(gtaversion == VC_10){
		// remove "%s has not been pre-instanced", we don't really care
		Nop(0x40C32B, 5);
		//MemoryVP::InjectHook(0x650ACB, RwTextureRead_VC);
		InjectHook(0x401000, printf, PATCH_JUMP);

//		static float lod0dist = 150.0f;
//		Patch(0x5818A3 + 2, &lod0dist);

		// try to reduce timecyc flickering...didn't work
		//static float s = 0.0f;
		//Patch(0x4CEA71 + 2, &s);
		//InjectHook(0x4CF5C7, 0x4D0189, PATCH_JUMP);
		//
		//Patch(0x4CF47D, 0xc031);	// xor eax, eax
		//Nop(0x4CF47D + 2, 2);

		// fix directional colour
		Patch(0x58051D + 2, 0x94B210);
		Patch(0x58054D + 2, 0x94B210+4);
		Patch(0x58057D + 2, 0x94B210+8);

		// disable level load screens
		Nop(0x40E00E, 5);
		Nop(0x40E01C, 5);
		Nop(0x40E157, 5);
	}
#endif
}

BOOL WINAPI
DllMain(HINSTANCE hInst, DWORD reason, LPVOID)
{
	if(reason == DLL_PROCESS_ATTACH){
		dllModule = hInst;

		//freopen("log.txt", "w", stdout);
		if(GetAsyncKeyState(VK_F8) & 0x8000){
			AllocConsole();
			freopen("CONIN$", "r", stdin);
			freopen("CONOUT$", "w", stdout);
			freopen("CONOUT$", "w", stderr);
		}
		AddressByVersion<addr>(0, 0, 0, 0, 0, 0);
		if(is10())
			patch();
	}

	return TRUE;
}
