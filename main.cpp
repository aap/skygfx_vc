#include "skygfx.h"
#include "ini_parser.hpp"
#include "d3d8.h"
#include "d3d8types.h"

HMODULE dllModule;
char asipath[MAX_PATH];
int gtaversion = -1;

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
static addr rpMatFXD3D8AtomicMatFXEnvRender_A = AddressByVersion<addr>(0x5CF6C0, 0x5CF980, 0x5D8F7C, 0x674EE0, 0x674F30, 0x673E90);
WRAPPER int rpMatFXD3D8AtomicMatFXEnvRender(RxD3D8InstanceData* a1, int a2, int a3, RwTexture* a4, RwTexture* a5)
{
	if (gtaversion != III_STEAM)
		VARJMP(rpMatFXD3D8AtomicMatFXEnvRender_A);

	__asm
	{
		//mov ecx, a3
		//mov edx, a2
		//mov eax, a1
		mov ecx,  [esp+0Ch]
		mov edx,  [esp+8]
		mov eax,  [esp+4]
		jmp rpMatFXD3D8AtomicMatFXEnvRender_A
	}
}
//static addr rpMatFXD3D8AtomicMatFXDefaultRender_A = AddressByVersion<addr>(0x5CEB80, 0x5CEE40, 0x5DB760, 0x674380, 0x6743D0, 0x673330);
//WRAPPER int rpMatFXD3D8AtomicMatFXDefaultRender(RxD3D8InstanceData*, int, RwTexture*) { VARJMP(rpMatFXD3D8AtomicMatFXDefaultRender_A); }
int &MatFXMaterialDataOffset = *AddressByVersion<int*>(0x66188C, 0x66188C, 0x671944, 0x7876CC, 0x7876D4, 0x7866D4);
int &MatFXAtomicDataOffset = *AddressByVersion<int*>(0x66189C, 0x66189C, 0x671930, 0x7876DC, 0x7876E4, 0x7866E4);

RwMatrix &defmat = *AddressByVersion<RwMatrix*>(0x5E6738, 0x5E6738, 0x62C170, 0x67FB18, 0x67FB18, 0x67EB18);

void **&RwEngineInst = *AddressByVersion<void***>(0x661228, 0x661228, 0x671248, 0x7870C0, 0x7870C8, 0x7860C8);
// ADDRESS
RpWorld *&pRpWorld = *AddressByVersion<RpWorld**>(0x726768, 0, 0, 0x8100B8, 0, 0);
RpLight *&pAmbient = *AddressByVersion<RpLight**>(0x885B6C, 0x885B1C, 0x895C5C, 0x974B44, 0x974B4C, 0x973B4C);
RpLight *&pDirect = *AddressByVersion<RpLight**>(0x880F7C, 0x880F2C, 0x89106C, 0x94DD40, 0x94DD48, 0x94CD48);
RpLight **pExtraDirectionals = AddressByVersion<RpLight**>(0x60009C, 0x5FFE84, 0x60CE7C, 0x69A140, 0x69A140, 0x699140);
int &NumExtraDirLightsInWorld = *AddressByVersion<int*>(0x64C608, 0x64C608, 0x65C608, 0x94DB48, 0x94DB50, 0x94CB50);

int blendstyle, blendkey;
int texgenstyle, texgenkey;
int xboxcarpipe, xboxcarpipekey;
int rimlight, rimlightkey;
int xboxwaterdrops;
int envMapSize;
Config config;

int dualpass;

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

static addr ApplyEnvMapTextureMatrix_A = AddressByVersion<addr>(0x5CFD40, 0x5D0000, 0x5D89E0, 0x6755D0, 0x675620, 0x674580);
WRAPPER void ApplyEnvMapTextureMatrix(RwTexture* a1, int a2, RwFrame* a3)
{
	if (gtaversion != III_STEAM){
		VARJMP(ApplyEnvMapTextureMatrix_A);
	}
	else
	{
		__asm
		{
			mov ecx,  [esp+0Ch]
			mov edx,  [esp+8]
			mov eax,  [esp+4]
			jmp ApplyEnvMapTextureMatrix_A
		}
	}
}

void
ApplyEnvMapTextureMatrix_hook(RwTexture *tex, int n, RwFrame *frame)
{
	{
		static bool keystate = false;
		if(GetAsyncKeyState(texgenkey) & 0x8000){
			if(!keystate){
				keystate = true;
				texgenstyle = (texgenstyle+1)%2;
			}
		}else
			keystate = false;
	}
	RwD3D8SetTexture(tex, n);
	if(isVC() && rwD3D8RasterIsCubeRaster(tex->raster)){
		RwD3D8SetTextureStageState(n, D3DTSS_TEXCOORDINDEX, 0x30000);
		return;
	}
	RwD3D8SetTextureStageState(n, D3DRS_ALPHAREF, 2);
	RwD3D8SetTextureStageState(n, D3DTSS_TEXCOORDINDEX, 0x10000);
	if(frame){
		D3DMATRIX invmat;

		RwMatrix *m1 = RwMatrixCreate();
		m1->flags = 0;
		RwMatrix *m2 = RwMatrixCreate();
		m2->flags = 0;
		RwMatrix *m3 = RwMatrixCreate();
		m3->flags = 0;

		RwMatrix *camfrm = RwFrameGetLTM(RwCameraGetFrame((RwCamera*)((RwGlobals*)RwEngineInst)->curCamera));
		RwMatrix *envfrm = RwFrameGetLTM(frame);
		if(texgenstyle == 0){
			memcpy(m2, camfrm, 0x40);
			m2->pos.x = 0.0f;
			m2->pos.y = 0.0f;
			m2->pos.z = 0.0f;
			m2->right.x = -m2->right.x;
			m2->right.y = -m2->right.y;
			m2->right.z = -m2->right.z;
			m2->flags = 0;
	
			RwMatrixInvert(m3, envfrm);
			m3->pos.x = -1.0f;
			m3->pos.y = -1.0f;
			m3->pos.z = -1.0f;
	
			m3->right.x *= -0.5f;
			m3->right.y *= -0.5f;
			m3->right.z *= -0.5f;
			m3->up.x *= -0.5f;
			m3->up.y *= -0.5f;
			m3->up.z *= -0.5f;
			m3->at.x *= -0.5f;
			m3->at.y *= -0.5f;
			m3->at.z *= -0.5f;
			m3->pos.x *= -0.5f;
			m3->pos.y *= -0.5f;
			m3->pos.z *= -0.5f;
			m3->flags = 0;
			RwMatrixMultiply(m1, m2, m3);
	
			rwtod3dmat(&invmat, m1);
			RwD3D8SetTransform(D3DTS_TEXTURE0+n, &invmat);
		}else{
			RwMatrixInvert(m1, envfrm);
			RwMatrixMultiply(m2, m1, camfrm);
			m2->right.x = -m2->right.x;
			m2->right.y = -m2->right.y;
			m2->right.z = -m2->right.z;
			m2->pos.x = 0.0f;
			m2->pos.y = 0.0f;
			m2->pos.z = 0.0f;
			m2->flags = 0;
			RwMatrixMultiply(m1, m2, &defmat);
			rwtod3dmat(&invmat, m1);
			RwD3D8SetTransform(D3DTS_TEXTURE0+n, &invmat);
		}

		RwMatrixDestroy(m1);
		RwMatrixDestroy(m2);
		RwMatrixDestroy(m3);
		return;
	}
	RwD3D8SetTransform(D3DTS_TEXTURE0+n, &defmat);
}

int rpMatFXD3D8AtomicMatFXDefaultRender(RxD3D8InstanceData *inst, int flags, RwTexture *texture)
{
	if(flags & 0x84 && texture)
		RwD3D8SetTexture(texture, 0);
	else
		RwD3D8SetTexture(NULL, 0);
	if(inst->vertexAlpha || inst->material->color.alpha != 0xFFu){
		if(!rwD3D8RenderStateIsVertexAlphaEnable())
			rwD3D8RenderStateVertexAlphaEnable(1);
	}else{
		if(rwD3D8RenderStateIsVertexAlphaEnable())
			rwD3D8RenderStateVertexAlphaEnable(0);
	}
	RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, inst->vertexAlpha != 0);
	RwD3D8SetPixelShader(0);
	RwD3D8SetVertexShader(inst->vertexShader);
	RwD3D8SetStreamSource(0, inst->vertexBuffer, inst->stride);
	drawDualPass(inst);
	return 0;
}

RwTexture *&pWaterTexReflection = *AddressByVersion<RwTexture**>(0, 0, 0, 0x77FA5C, 0x77FA5C, 0x77EA5C);

int
rpMatFXD3D8AtomicMatFXEnvRender_dual(RxD3D8InstanceData *inst, int flags, int sel, RwTexture *texture, RwTexture *envMap)
{
	MatFX *matfx = *RWPLUGINOFFSET(MatFX*, inst->material, MatFXMaterialDataOffset);
	MatFXEnv *env = &matfx->fx[sel];
	{
		static bool keystate = false;
		if(GetAsyncKeyState(blendkey) & 0x8000){
			if(!keystate){
				keystate = true;
				blendstyle = (blendstyle+1)%2;
			}
		}else
			keystate = false;
	}
	// rather ugly way to single out water. more research needed here
	if(blendstyle == 1 || isVC() && envMap == pWaterTexReflection)
		return rpMatFXD3D8AtomicMatFXEnvRender(inst, flags, sel, texture, envMap);

	static float mult = isIII() ? 2.0f : 4.0f;
	float factor = env->envCoeff*mult*255.0f;
	RwUInt8 intens = (RwUInt8)factor;

	if(factor == 0.0f || !envMap){
		if(sel == 0)
			return rpMatFXD3D8AtomicMatFXDefaultRender(inst, flags, texture);
		return 0;
	}
	if(inst->vertexAlpha || inst->material->color.alpha != 0xFFu){
		if(!rwD3D8RenderStateIsVertexAlphaEnable())
			rwD3D8RenderStateVertexAlphaEnable(1);
	}else{
		if(rwD3D8RenderStateIsVertexAlphaEnable())
			rwD3D8RenderStateVertexAlphaEnable(0);
	}
	if(flags & 0x84 && texture)
		RwD3D8SetTexture(texture, 0);
	else
		RwD3D8SetTexture(NULL, 0);
	RwD3D8SetVertexShader(inst->vertexShader);
	RwD3D8SetStreamSource(0, inst->vertexBuffer, inst->stride);
	RwD3D8SetIndices(inst->indexBuffer, inst->baseIndex);
	if(inst->indexBuffer)
		RwD3D8DrawIndexedPrimitive(inst->primType, 0, inst->numVertices, 0, inst->numIndices);
	else
		RwD3D8DrawPrimitive(inst->primType, inst->baseIndex, inst->numVertices);
	
	ApplyEnvMapTextureMatrix(envMap, 0, env->envFrame);
	if(!rwD3D8RenderStateIsVertexAlphaEnable())
		rwD3D8RenderStateVertexAlphaEnable(1);
	RwUInt32 src, dst, lighting, zwrite, fog, fogcol;
	RwRenderStateGet(rwRENDERSTATESRCBLEND, &src);
	RwRenderStateGet(rwRENDERSTATEDESTBLEND, &dst);
//	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
	RwD3D8GetRenderState(D3DRS_LIGHTING, &lighting);
	RwD3D8GetRenderState(D3DRS_ZWRITEENABLE, &zwrite);
	RwD3D8GetRenderState(D3DRS_FOGENABLE, &fog);
	RwD3D8SetRenderState(D3DRS_FOGENABLE, 0);
//	RwD3D8SetRenderState(D3DRS_LIGHTING, 0);
	RwD3D8SetRenderState(D3DRS_ZWRITEENABLE, 0);
	if(fog){
		RwD3D8GetRenderState(D3DRS_FOGCOLOR, &fogcol);
		RwD3D8SetRenderState(D3DRS_FOGCOLOR, 0);
	}

	RwUInt32 texfactor = ((intens | ((intens | (intens << 8)) << 8)) << 8) | intens;
	RwD3D8SetRenderState(D3DRS_TEXTUREFACTOR, texfactor);
	RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MODULATE);
	RwD3D8SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_CURRENT);
	RwD3D8SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_TFACTOR);
	// alpha unused
	//RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	//RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_CURRENT);
	//RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);

	if(inst->indexBuffer)
		RwD3D8DrawIndexedPrimitive(inst->primType, 0, inst->numVertices, 0, inst->numIndices);
	else
		RwD3D8DrawPrimitive(inst->primType, inst->baseIndex, inst->numVertices);

	rwD3D8RenderStateVertexAlphaEnable(0);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)src);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)dst);
	RwD3D8SetRenderState(D3DRS_LIGHTING, lighting);
	RwD3D8SetRenderState(D3DRS_ZWRITEENABLE, zwrite);
	RwD3D8SetRenderState(D3DRS_FOGENABLE, fog);
	if(fog)
		RwD3D8SetRenderState(D3DRS_FOGCOLOR, fogcol);
	RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	RwD3D8SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, 0);
	RwD3D8SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
	return 0;
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
	if(!dualpass){
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

RwD3D8Vertex *blurVertices = AddressByVersion<RwD3D8Vertex*>(0x62F780, 0x62F780, 0x63F780, 0x7097A8, 0x7097A8, 0x7087A8);
RwImVertexIndex *blurIndices = AddressByVersion<RwImVertexIndex*>(0x5FDD90, 0x5FDB78, 0x60AB70, 0x697D48, 0x697D48, 0x696D50);
static addr DefinedState_A = AddressByVersion<addr>(0x526330, 0x526570, 0x526500, 0x57F9C0, 0x57F9E0, 0x57F7F0);
WRAPPER void DefinedState(void) { VARJMP(DefinedState_A); }

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

static addr CGame__InitialiseRenderWare_A = AddressByVersion<addr>(0x48BBA0, 0x48BC90, 0x48BC20, 0x4A51A0, 0x4A51C0, 0x4A5070);
WRAPPER bool CGame__InitialiseRenderWare(void) { VARJMP(CGame__InitialiseRenderWare_A); }

bool
CGame__InitialiseRenderWare_hook(void)
{
	bool ret = CGame__InitialiseRenderWare();
	neoInit();
	return ret;
}

RwCamera *&pRwCamera = *AddressByVersion<RwCamera**>(0x72676C, 0x72676C, 0x7368AC, 0x8100BC, 0x8100C4, 0x80F0C4);

WRAPPER void rpMatFXD3D8AtomicMatFXEnvRender_dual_IIISteam()
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
		call rpMatFXD3D8AtomicMatFXEnvRender_dual
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


#ifndef RELEASE

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
	//hookPixelShader();
	setcurveps();
	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, blurVertices, 4, blurIndices, 6);
	RwD3D9SetIm2DPixelShader(NULL);
	//overridePS = NULL;
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

void (*InitialiseGame)(void);
void
InitialiseGame_hook(void)
{
	if(isIII())
		// fall back to generic.txd when reading from dff
		InjectHook(AddressByVersion<addr>(0x5AAE1B, 0x5AB0DB, 0x5AD708, 0, 0, 0), RwTextureRead_generic);
	neoInit();
	InitialiseGame();
}

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

void
patch(void)
{
	using namespace std;
//	char tmp[32];
	string tmp;
	char modulePath[MAX_PATH];

	GetModuleFileName(dllModule, modulePath, MAX_PATH);
	strncpy(asipath, modulePath, MAX_PATH);
	char *p = strrchr(asipath, '\\');
	if(p) p[1] = '\0';
	size_t nLen = strlen(modulePath);
	modulePath[nLen-1] = L'i';
	modulePath[nLen-2] = L'n';
	modulePath[nLen-3] = L'i';

	linb::ini cfg;
	cfg.load_file(modulePath);

	// hook for all things that are initialized once when a game is started
	// ADDRESS
	if(gtaversion == III_10 || gtaversion == VC_10){
		InterceptCall(&InitialiseGame, InitialiseGame_hook, AddressByVersion<addr>(0x582E6C, 0, 0, 0x600411, 0, 0));
		hookplugins();
	}

	blendkey = readhex(cfg.get("SkyGfx", "texblendSwitchKey", "0x0").c_str());
	texgenkey = readhex(cfg.get("SkyGfx", "texgenSwitchKey", "0x0").c_str());
	xboxcarpipekey = readhex(cfg.get("SkyGfx", "neoCarPipeKey", "0x0").c_str());
	rimlightkey = readhex(cfg.get("SkyGfx", "neoRimLightKey", "0x0").c_str());
	config.neoWorldPipeKey = readhex(cfg.get("SkyGfx", "neoWorldPipeKey", "0x0").c_str());
	config.neoGlossPipeKey = readhex(cfg.get("SkyGfx", "neoGlossPipeKey", "0x0").c_str());

	tmp = cfg.get("SkyGfx", "texblendSwitch", "");
	if(tmp != ""){
		blendstyle = readint(tmp);
		if (gtaversion != III_STEAM)
			InjectHook(AddressByVersion<addr>(0x5D0CE8, 0x5D0FA8, 0, 0x6765C8, 0x676618, 0x675578), rpMatFXD3D8AtomicMatFXEnvRender_dual);
		else
			InjectHook(0x5D8D37, rpMatFXD3D8AtomicMatFXEnvRender_dual_IIISteam);
	}
	blendstyle %= 2;
	tmp = cfg.get("SkyGfx", "texgenSwitch", "");
	if(tmp != ""){
		texgenstyle = readint(tmp);
		if (gtaversion != III_STEAM)
			InjectHook(ApplyEnvMapTextureMatrix_A, ApplyEnvMapTextureMatrix_hook, PATCH_JUMP);
		else
			InjectHook(ApplyEnvMapTextureMatrix_A, ApplyEnvMapTextureMatrix_hook_IIISteam, PATCH_JUMP);
	}
	texgenstyle %= 2;

	tmp = cfg.get("SkyGfx", "IIIEnvFrame", "");
	if(isVC() && tmp != "" && readint(tmp)){
		InjectHook(AddressByVersion<addr>(0, 0, 0, 0x57A8BA, 0x57A8DA, 0x57A7AA), createIIIEnvFrame);
		InjectHook(AddressByVersion<addr>(0, 0, 0, 0x57A8C7, 0x57A8E7, 0x57A7B7),
		           AddressByVersion<addr>(0, 0, 0, 0x57A8F4, 0x57A914, 0x57A7E4), PATCH_JUMP);
	}
	tmp = cfg.get("SkyGfx", "VCEnvFrame", "");
	if(isIII() && tmp != "" && readint(tmp)){
		InjectHook(AddressByVersion<addr>(0x5218A2, 0x521AE2, 0x521A72, 0, 0, 0), createVCEnvFrame);
		InjectHook(AddressByVersion<addr>(0x5218AC, 0x521AEC, 0x521A7C, 0, 0, 0),
		           AddressByVersion<addr>(0x52195E, 0x521B9E, 0x521B2E, 0, 0, 0), PATCH_JUMP);
	}

	if(isIII())
		InjectHook(AddressByVersion<addr>(0x59BABF, 0x59BD7F, 0x598E6F, 0, 0, 0), CreateTextureFilterFlags);

	xboxcarpipe = readint(cfg.get("SkyGfx", "neoCarPipe", ""), -1);
	config.iCanHasNeoCar = xboxcarpipe >= 0;
	envMapSize = readint(cfg.get("SkyGfx", "envMapSize", ""), 128);
	rimlight = readint(cfg.get("SkyGfx", "neoRimLightPipe", ""), -1);
	config.iCanHasNeoRim = rimlight >= 0;
	int n = 1;
	while(n < envMapSize) n *= 2;
	envMapSize = n;
	config.neoWorldPipe = readint(cfg.get("SkyGfx", "neoWorldPipe", ""), -1);
	config.iCanHasNeoWorld = config.neoWorldPipe >= 0;
	config.neoGlossPipe = readint(cfg.get("SkyGfx", "neoGlossPipe", ""), -1);
	config.iCanHasNeoGloss = config.neoGlossPipe >= 0;

	tmp = cfg.get("SkyGfx", "dualPass", "");
	if(tmp != ""){
		dualpass = readint(tmp);
		if (gtaversion != III_STEAM)
			InjectHook(AddressByVersion<addr>(0x5DFB99, 0x5DFE59, 0, 0x678D69, 0x678DB9, 0x677D19), dualPassHook, PATCH_JUMP);
		else
			InjectHook(0x5EE675, dualPassHook_IIISteam, PATCH_JUMP);
		InjectHook(AddressByVersion<addr>(0x5CEB80, 0x5CEE40, 0x5DB760, 0x674380, 0x6743D0, 0x673330), rpMatFXD3D8AtomicMatFXDefaultRender, PATCH_JUMP);
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

	tmp = cfg.get("SkyGfx", "neoWaterDrops", "");
	xboxwaterdrops = readint(tmp);
	if(tmp != "" && xboxwaterdrops)
		hookWaterDrops();

	tmp = cfg.get("SkyGfx", "replaceDefaultPipeline", "");
	if(tmp != "" && readint(tmp)){
		if(gtaversion == III_10){
			Patch(0x5DB427 +2, D3D8AtomicDefaultInstanceCallback_fixed);
			Patch(0x5DB43B +3, rxD3D8DefaultRenderCallback_xbox);
		}else if(gtaversion == VC_10){
			Patch(0x67BAB7 +2, D3D8AtomicDefaultInstanceCallback_fixed);
			Patch(0x67BACB +3, rxD3D8DefaultRenderCallback_xbox);
		}
	}

#ifndef RELEASE
	if(gtaversion == III_10){
		// ignore txd.img
		InjectHook(0x48C12E, 0x48C14C, PATCH_JUMP);

		int i = readint(cfg.get("SkyGfx", "curve", ""), -1);
		if(i >= 0){
			curveIdx = i % 256;
			InjectHook(0x48E44B, curvehook, PATCH_JUMP);
		}
		InjectHook(0x405DB0, printf, PATCH_JUMP);

		// patch loadscreens
		Patch<uint>(0x48D774, 0x6024448b);  // 8B 44 24 60 - mov eax,[esp+60h]
		Patch<uchar>(0x48D778, 0x50);	      // push eax
		InjectHook(0x48D779, 0x48D79B, PATCH_JUMP);

		//MemoryVP::InjectHook(0x48E603, RenderEffectsHook);
		//MemoryVP::InjectHook(0x5219B3, CVehicleModelInfo__SetEnvironmentMapCB_hook);
		//MemoryVP::Patch<void*>(0x521986+1, CVehicleModelInfo__SetEnvironmentMapCB_hook);

		// clear framebuffer
		Patch<uchar>(0x48CFC1+1, 3);
		Patch<uchar>(0x48D0AD+1, 3);
		Patch<uchar>(0x48E6A7+1, 3);
		Patch<uchar>(0x48E78C+1, 3);

	}
#endif
#ifndef RELEASE
	if(gtaversion == VC_10){
		// remove "%s has not been pre-instanced", we don't really care
		Nop(0x40C32B, 5);
		//MemoryVP::InjectHook(0x650ACB, RwTextureRead_VC);
		InjectHook(0x401000, printf, PATCH_JUMP);

		// try to reduce timecyc flickering...didn't work
		//static float s = 0.0f;
		//Patch(0x4CEA71 + 2, &s);
		//InjectHook(0x4CF5C7, 0x4D0189, PATCH_JUMP);
		//
		//Patch(0x4CF47D, 0xc031);	// xor eax, eax
		//Nop(0x4CF47D + 2, 2);

		// enable loadscreens. BREAKS FOR SOME REASON (OLA?)
		//MemoryVP::Nop(0x4A69D4, 1);
		//// ff 74 24 78             push   DWORD PTR [esp+0x78]
		//MemoryVP::Patch(0x4A69D4+1, 0x782474ff);
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
		if(gtaversion != -1)
			patch();
	}

	return TRUE;
}
