#include "skygfx.h"
#include "d3d8.h"
#include "d3d8types.h"

HMODULE dllModule;
char asipath[MAX_PATH];
int gtaversion = -1;
static uint32_t rwD3D8RasterIsCubeRaster_A = AddressByVersion<uint32_t>(0, 0, 0, 0x63EE40, 0x63EE90, 0x63DDF0); // VC only
WRAPPER int rwD3D8RasterIsCubeRaster(RwRaster*) { VARJMP(rwD3D8RasterIsCubeRaster_A); }
static uint32_t rpMatFXD3D8AtomicMatFXEnvRender_A = AddressByVersion<uint32_t>(0x5CF6C0, 0x5CF980, 0x5D8F7C, 0x674EE0, 0x674F30, 0x673E90);
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
static uint32_t rpMatFXD3D8AtomicMatFXDefaultRender_A = AddressByVersion<uint32_t>(0x5CEB80, 0x5CEE40, 0x5DB760, 0x674380, 0x6743D0, 0x673330);
WRAPPER int rpMatFXD3D8AtomicMatFXDefaultRender(RxD3D8InstanceData*, int, RwTexture*) { VARJMP(rpMatFXD3D8AtomicMatFXDefaultRender_A); }
int &MatFXMaterialDataOffset = *AddressByVersion<int*>(0x66188C, 0x66188C, 0x671944, 0x7876CC, 0x7876D4, 0x7866D4);
int &MatFXAtomicDataOffset = *AddressByVersion<int*>(0x66189C, 0x66189C, 0x671930, 0x7876DC, 0x7876E4, 0x7866E4);

RwMatrix &defmat = *AddressByVersion<RwMatrix*>(0x5E6738, 0x5E6738, 0x62C170, 0x67FB18, 0x67FB18, 0x67EB18);

void **&RwEngineInst = *AddressByVersion<void***>(0x661228, 0x661228, 0x671248, 0x7870C0, 0x7870C8, 0x7860C8);
RpLight *&pAmbient = *AddressByVersion<RpLight**>(0x885B6C, 0x885B1C, 0x895C5C, 0x974B44, 0x974B4C, 0x973B4C);
RpLight *&pDirect = *AddressByVersion<RpLight**>(0x880F7C, 0x880F2C, 0x89106C, 0x94DD40, 0x94DD48, 0x94CD48);
RpLight **pExtraDirectionals = AddressByVersion<RpLight**>(0x60009C, 0x5FFE84, 0x60CE7C, 0x69A140, 0x69A140, 0x699140);
int &NumExtraDirLightsInWorld = *AddressByVersion<int*>(0x64C608, 0x64C608, 0x65C608, 0x94DB48, 0x94DB50, 0x94CB50);

int blendstyle, blendkey;
int texgenstyle, texgenkey;
int xboxcarpipe, xboxcarpipekey;
int rimlight, rimlightkey;
int xboxworldpipe, xboxworldpipekey;
int envMapSize;

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

static uint32_t ApplyEnvMapTextureMatrix_A = AddressByVersion<uint32_t>(0x5CFD40, 0x5D0000, 0x5D89E0, 0x6755D0, 0x675620, 0x674580);
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


//int
//rpMatFXD3D8AtomicMatFXEnvRender_hook(RxD3D8InstanceData *inst, int flags, int sel, RwTexture *texture, RwTexture *envMap)
//{
//	MatFX *matfx = *RWPLUGINOFFSET(MatFX*, inst->material, MatFXMaterialDataOffset);
//	MatFXEnv *env = &matfx->fx[sel];
//	float factor = env->envCoeff*255.0f;
//	RwUInt8 intens = factor;
//	if(factor == 0.0f || !envMap){
//		if(sel == 0)
//			return rpMatFXD3D8AtomicMatFXDefaultRender(inst, flags, texture);
//		return 0;
//	}
//	if(inst->vertexAlpha || inst->material->color.alpha != 0xFFu){
//		if(!rwD3D8RenderStateIsVertexAlphaEnable())
//			rwD3D8RenderStateVertexAlphaEnable(1);
//	}else{
//		if(rwD3D8RenderStateIsVertexAlphaEnable())
//			rwD3D8RenderStateVertexAlphaEnable(0);
//	}
//	if(flags & 0x84 && texture)
//		RwD3D8SetTexture(texture, 0);
//	else
//		RwD3D8SetTexture(NULL, 0);
//	RwD3D8SetTexture(NULL, 1);
//
//	ApplyEnvMapTextureMatrix(envMap, 0, env->envFrame);
//	RwUInt32 texfactor = ((intens | ((intens | (intens << 8)) << 8)) << 8) | intens;
//	RwD3D8SetRenderState(D3DRS_TEXTUREFACTOR, texfactor);
//	RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MULTIPLYADD);
//	RwD3D8SetTextureStageState(1, D3DTSS_COLORARG0, D3DTA_CURRENT);
//	RwD3D8SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
//	RwD3D8SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_TFACTOR);
//
//	RwD3D8SetVertexShader(inst->vertexShader);
//	RwD3D8SetStreamSource(0, inst->vertexBuffer, inst->stride);
//	RwD3D8SetIndices(inst->indexBuffer, inst->baseIndex);
//	if(inst->indexBuffer)
//		RwD3D8DrawIndexedPrimitive(inst->primType, 0, inst->numVertices, 0, inst->numIndices);
//	else
//		RwD3D8DrawPrimitive(inst->primType, inst->baseIndex, inst->numVertices);
//	RwD3D8SetTexture(NULL, 1);
//	RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
//	RwD3D8SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, 0);
//	RwD3D8SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
//	return 0;
//}


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
	RwUInt8 intens = factor;

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
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
	RwD3D8GetRenderState(D3DRS_LIGHTING, &lighting);
	RwD3D8GetRenderState(D3DRS_ZWRITEENABLE, &zwrite);
	RwD3D8GetRenderState(D3DRS_FOGENABLE, &fog);
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
	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_CURRENT);
	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);

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
	RwD3D8SetIndices(inst->indexBuffer, inst->baseIndex);

	int hasAlpha, alphafunc, alpharef;
	RwD3D8GetRenderState(D3DRS_ALPHABLENDENABLE, &hasAlpha);
	if(hasAlpha){
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

static uint32_t dualPassHook_R = AddressByVersion<uint32_t>(0x5DFBD8, 0x5DFE98, 0, 0x678DA8, 0x678DF8, 0x677D58); 
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
static uint32_t DefinedState_A = AddressByVersion<uint32_t>(0x526330, 0x526570, 0x526500, 0x57F9C0, 0x57F9E0, 0x57F7F0);
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

static uint32_t &sniperTrailsHook_unk_addr = *AddressByVersion<uint32_t*>(0, 0, 0, 0x97F888, 0x97F890, 0x97E890);
static uint32_t sniperTrailsHook_R = AddressByVersion<uint32_t>(0, 0, 0, 0x55EB90, 0x55EBB0, 0x55EA80); 

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

static uint32_t CGame__InitialiseRenderWare_A = AddressByVersion<uint32_t>(0x48BBA0, 0x48BC90, 0x48BC20, 0x4A51A0, 0x4A51C0, 0x4A5070);
WRAPPER bool CGame__InitialiseRenderWare(void) { VARJMP(CGame__InitialiseRenderWare_A); }

bool
CGame__InitialiseRenderWare_hook(void)
{
	bool ret = CGame__InitialiseRenderWare();
	neoInit();
	return ret;
}

int
readhex(char *str)
{
	int n = 0;
	if(strlen(str) > 2)
		sscanf(str+2, "%X", &n);
	return n;
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

struct CControllerState
{
  __int16 LEFTSTICKX;
  __int16 LEFTSTICKY;
  __int16 RIGHTSTICKX;
  __int16 RIGHTSTICKY;
  __int16 LEFTSHOULDER1;
  __int16 LEFTSHOULDER2;
  __int16 RIGHTSHOULDER1;
  __int16 RIGHTSHOULDER2;
  __int16 DPADUP;
  __int16 DPADDOWN;
  __int16 DPADLEFT;
  __int16 DPADRIGHT;
  __int16 START;
  __int16 SELECT;
  __int16 SQUARE;
  __int16 TRIANGLE;
  __int16 CROSS;
  __int16 CIRCLE;
  __int16 LEFTSHOCK;
  __int16 RIGHTSHOCK;
  __int16 NETWORK_TALK;
};

#pragma pack(push, 1)
struct CPad
{
  CControllerState NewState;
  CControllerState OldState;
  CControllerState PCTempKeyState;
  CControllerState PCTempJoyState;
  CControllerState PCTempMouseState;
  char Phase;
  char gap_d3[1];
  __int16 Mode;
  __int16 ShakeDur;
  char ShakeFreq;
  char bHornHistory[5];
  char iCurrHornHistory;
  char DisablePlayerControls;
  char JustOutOfFrontEnd;
  char bApplyBrakes;
  char field_e2[12];
  char gap_ee[2];
  int LastTimeTouched;
  int AverageWeapon;
  int AverageEntries;
};
#pragma pack(pop)

CPad *Pads = (CPad*)0x6F0360;

struct CCam {
	void WorkOutCamHeight(float *vec, float a, float b);
	void WorkOutCamHeight_hook(float *vec, float a, float b);
	void Process_FollowPed(float *vec, float a, float b, float c);
	void Process_FollowPed_hook(float *vec, float a, float b, float c);
	void Process_Editor(float *vec, float a, float b, float c);
	void Process_Editor_hook(float *vec, float a, float b, float c);
	void Process_Debug(float *vec, float a, float b, float c);
	void Process_Debug_hook(float *vec, float a, float b, float c);

	char bytes[12];
	short mode;
	char pad8[90];
	float offset;
	char pad7[60];
	float angle1;
	float timething;
	float fov;
	char pad6[4];
	float angle2;
	char pad5[20];
	float unkflt1;
	char pad4[36];
	float vec2[3];
	char pad3[60];
	float vec1[3];
	float pos[3];
	float pos2[3];
	float up[3];
	char pad2[24];
	int attachment;
	char pad1[12];
	int unk3;
	int unk2;
	int unk1;
};
WRAPPER void CCam::WorkOutCamHeight(float *vec, float a, float b) { EAXJMP(0x466650); }
WRAPPER void CCam::Process_FollowPed(float *vec, float a, float b, float c) { EAXJMP(0x45E3A0); }
WRAPPER void CCam::Process_Editor(float *vec, float a, float b, float c) { EAXJMP(0x45C590); }
WRAPPER void CCam::Process_Debug(float *vec, float a, float b, float c) { EAXJMP(0x45CCC0); }

void
CCam::Process_Editor_hook(float *vec, float a, float b, float c)
{
	Pads[1] = Pads[0];
	this->Process_Editor(vec, a, b, c);
}

void
CCam::Process_Debug_hook(float *vec, float a, float b, float c)
{
	Pads[1] = Pads[0];
	this->Process_Debug(vec, a, b, c);
}

// distance values around 468953

float *tanvalCar[3] = {
	(float*)0x5F08C4,
	(float*)0x5F08C8,
	(float*)0x5F046C
};
float *distCar[3] = {
	(float*)0x468959,
	(float*)0x468978,
	(float*)0x468998
};
float &carViewMode = *(float*)0x6FADD0;
float carFov = 85.0f;
float foo;

void
CCam::Process_FollowPed_hook(float *vec, float a, float b, float c)
{
	this->Process_FollowPed(vec, a, b, c);
	this->fov = carFov;
}

// 2: fov 75, tan 12.5 (gta3_30.jpg)
// 2: fov 85, tan 7.5 (gta3_30.jpg)
// 2: fov 85, tan 9.0 (gta3_30.jpg)

void
CCam::WorkOutCamHeight_hook(float *vec, float a, float b)
{
	this->WorkOutCamHeight(vec, a, b);
	int i = (int)carViewMode - 1;
	if(i < 1 && i > 3)
		return;

	if((GetAsyncKeyState('1') & 0x8000) && (GetAsyncKeyState(VK_LMENU) & 0x8000))
		carFov -= 0.1f;
	if((GetAsyncKeyState('2') & 0x8000) && (GetAsyncKeyState(VK_LMENU) & 0x8000))
		carFov += 0.1f;
	this->fov = carFov;

	if((GetAsyncKeyState('3') & 0x8000) && (GetAsyncKeyState(VK_LMENU) & 0x8000))
		*tanvalCar[i] -= 0.1f;
	if((GetAsyncKeyState('4') & 0x8000) && (GetAsyncKeyState(VK_LMENU) & 0x8000))
		*tanvalCar[i] += 0.1f;

	if((GetAsyncKeyState('5') & 0x8000) && (GetAsyncKeyState(VK_LMENU) & 0x8000))
		*distCar[i] -= 0.1f;
	if((GetAsyncKeyState('6') & 0x8000) && (GetAsyncKeyState(VK_LMENU) & 0x8000))
		*distCar[i] += 0.1f;

	printf("%f %f %f %f\n", carViewMode, this->fov, *tanvalCar[i], *distCar[i]);
//	printf("%f\n", tanval1);
//	printf("(%f %f %f) (%f %f %f) %f %f %f %f\n", this->vec1[0], this->vec1[1], this->vec1[2],
//		this->pos[0], this->pos[1], this->pos[2],
//		this->angle1, this->angle2, this->offset, foo);
}

WRAPPER int readfile_(void*, void*, int) { EAXJMP(0x48DF50); }
WRAPPER void *openfile_(char*, char*) { EAXJMP(0x48DF90); }

struct dirent {
	int off, siz;
	char name[24];
};

FILE *logfile;

void*
openfile(char *path, char *mode)
{
	printf("opening %s\n", path);
	return openfile_(path, mode);
}

int
readfile(void *f, void *dst, int n)
{
	int ret = readfile_(f, dst, n);
	if(ret){
		dirent *d = (dirent*)dst;
		printf("%d %x %x %s\n", ret, d->off, d->siz, d->name);
	}
	return ret;
}

struct TxdStore {
	static void PushCurrentTxd(void);
	static RwTexDictionary *PopCurrentTxd(void);
	static RwTexDictionary *FindTxdSlot(char*);
	static void SetCurrentTxd(RwTexDictionary*);
};

static uint32_t TxdStore_PushCurrentTxd_A = AddressByVersion<uint32_t>(0x527900, 0x527B40, 0x527AD0, 0, 0, 0);
WRAPPER void TxdStore::PushCurrentTxd(void) { VARJMP(TxdStore_PushCurrentTxd_A); }
static uint32_t TxdStore_PopCurrentTxd_A = AddressByVersion<uint32_t>(0x527910, 0x527B50, 0x527AE0, 0, 0, 0);
WRAPPER RwTexDictionary *TxdStore::PopCurrentTxd(void) { VARJMP(TxdStore_PopCurrentTxd_A); }
static uint32_t TxdStore_FindTxdSlot_A = AddressByVersion<uint32_t>(0x5275D0, 0x527810, 0x5277A0, 0, 0, 0);
WRAPPER RwTexDictionary *TxdStore::FindTxdSlot(char*) { VARJMP(TxdStore_FindTxdSlot_A); }
static uint32_t TxdStore_SetCurrentTxd_A = AddressByVersion<uint32_t>(0x5278C0, 0x527B00, 0x527A90, 0, 0, 0);
WRAPPER void TxdStore::SetCurrentTxd(RwTexDictionary*) { VARJMP(TxdStore_SetCurrentTxd_A); }

RwTexture*
RwTextureRead_generic(char *name, char *mask)
{
	RwTexture *tex;
	RwTexDictionary *dict;
	static RwTexDictionary *generic = NULL;
	tex = RwTextureRead(name, mask);
	if(tex)
		return tex;
	TxdStore::PushCurrentTxd();
	if(!generic)
		generic = TxdStore::FindTxdSlot("generic");
	TxdStore::SetCurrentTxd(generic);
	tex = RwTextureRead(name, mask);
	TxdStore::PopCurrentTxd();
	return tex;
}

void
patch(void)
{
	char tmp[32];
	char modulePath[MAX_PATH];

	GetModuleFileName(dllModule, modulePath, MAX_PATH);
	strncpy(asipath, modulePath, MAX_PATH);
	char *p = strrchr(asipath, '\\');
	if(p) p[1] = '\0';
	size_t nLen = strlen(modulePath);
	modulePath[nLen-1] = L'i';
	modulePath[nLen-2] = L'n';
	modulePath[nLen-3] = L'i';

	GetPrivateProfileString("SkyGfx", "texblendSwitchKey", "0x76", tmp, sizeof(tmp), modulePath);
	blendkey = readhex(tmp);
	GetPrivateProfileString("SkyGfx", "texgenSwitchKey", "0x77", tmp, sizeof(tmp), modulePath);
	texgenkey = readhex(tmp);
	GetPrivateProfileString("SkyGfx", "xboxCarPipeKey", "0x75", tmp, sizeof(tmp), modulePath);
	xboxcarpipekey = readhex(tmp);
	GetPrivateProfileString("SkyGfx", "rimLightKey", "0x74", tmp, sizeof(tmp), modulePath);
	rimlightkey = readhex(tmp);
	GetPrivateProfileString("SkyGfx", "xboxWorldPipeKey", "0x73", tmp, sizeof(tmp), modulePath);
	xboxworldpipekey = readhex(tmp);
	blendstyle = GetPrivateProfileInt("SkyGfx", "texblendSwitch", 0, modulePath);
	if(blendstyle >= 0)
	{
		if (gtaversion != III_STEAM)
			MemoryVP::InjectHook(AddressByVersion<uint32_t>(0x5D0CE8, 0x5D0FA8, 0, 0x6765C8, 0x676618, 0x675578), rpMatFXD3D8AtomicMatFXEnvRender_dual);
		else
			MemoryVP::InjectHook(0x5D8D37, rpMatFXD3D8AtomicMatFXEnvRender_dual_IIISteam);
	}
	blendstyle %= 2;
	texgenstyle = GetPrivateProfileInt("SkyGfx", "texgenSwitch", 0, modulePath);
	if(texgenstyle >= 0)
	{
		if (gtaversion != III_STEAM)
			MemoryVP::InjectHook(ApplyEnvMapTextureMatrix_A, ApplyEnvMapTextureMatrix_hook, PATCH_JUMP);
		else
			MemoryVP::InjectHook(ApplyEnvMapTextureMatrix_A, ApplyEnvMapTextureMatrix_hook_IIISteam, PATCH_JUMP);
	}
	texgenstyle %= 2;
	if(isVC() && GetPrivateProfileInt("SkyGfx", "IIIReflections", FALSE, modulePath)){
		MemoryVP::InjectHook(AddressByVersion<uint32_t>(0, 0, 0, 0x57A8BA, 0x57A8DA, 0x57A7AA), createIIIEnvFrame);
		MemoryVP::InjectHook(AddressByVersion<uint32_t>(0, 0, 0, 0x57A8C7, 0x57A8E7, 0x57A7B7),
							 AddressByVersion<uint32_t>(0, 0, 0, 0x57A8F4, 0x57A914, 0x57A7E4), PATCH_JUMP);
	}
	if(isIII() && GetPrivateProfileInt("SkyGfx", "VCReflections", FALSE, modulePath)){
		MemoryVP::InjectHook(AddressByVersion<uint32_t>(0x5218A2, 0x521AE2, 0x521A72, 0, 0, 0), createVCEnvFrame);
		MemoryVP::InjectHook(AddressByVersion<uint32_t>(0x5218AC, 0x521AEC, 0x521A7C, 0, 0, 0),
		                     AddressByVersion<uint32_t>(0x52195E, 0x521B9E, 0x521B2E, 0, 0, 0), PATCH_JUMP);
	}

	if(isIII())
		MemoryVP::InjectHook(AddressByVersion<uint32_t>(0x59BABF, 0x59BD7F, 0x598E6F, 0, 0, 0), CreateTextureFilterFlags);

	xboxcarpipe = GetPrivateProfileInt("SkyGfx", "xboxCarPipe", 0, modulePath);
	envMapSize = GetPrivateProfileInt("SkyGfx", "envMapSize", 128, modulePath);
	rimlight = GetPrivateProfileInt("SkyGfx", "rimLight", 0, modulePath);
	int n = 1;
	while(n < envMapSize) n *= 2;
	envMapSize = n;
	xboxworldpipe = GetPrivateProfileInt("SkyGfx", "xboxWorldPipe", -1, modulePath);

	if(GetPrivateProfileInt("SkyGfx", "dualPass", TRUE, modulePath))
	{
		if (gtaversion != III_STEAM)
			MemoryVP::InjectHook(AddressByVersion<uint32_t>(0x5DFB99, 0x5DFE59, 0, 0x678D69, 0x678DB9, 0x677D19), dualPassHook, PATCH_JUMP);
		else
			MemoryVP::InjectHook(0x5EE675, dualPassHook_IIISteam, PATCH_JUMP);
	}

	if(isVC() && GetPrivateProfileInt("SkyGfx", "disableBackfaceCulling", FALSE, modulePath)){
		// hope I didn't miss anything
		MemoryVP::Patch<BYTE>(AddressByVersion<uint32_t>(0, 0, 0, 0x4C9E5F, 0x4C9E7F, 0x4C9D1F), 1);	// in CRenderer::RenderOneNonRoad()
		MemoryVP::Patch<BYTE>(AddressByVersion<uint32_t>(0, 0, 0, 0x4C9F08, 0x4C9F28, 0x4C9DC8), 1);	// in CRenderer::RenderBoats()
		MemoryVP::Patch<BYTE>(AddressByVersion<uint32_t>(0, 0, 0, 0x4C9F5D, 0x4C9F7D, 0x4C9E1D), 1);	// in CRenderer::RenderEverythingBarRoads()
		MemoryVP::Patch<BYTE>(AddressByVersion<uint32_t>(0, 0, 0, 0x4CA157, 0x4CA177, 0x4CA017), 1);	// in CRenderer::RenderFadingInEntities()
		MemoryVP::Patch<BYTE>(AddressByVersion<uint32_t>(0, 0, 0, 0x4CA199, 0x4CA1B9, 0x4CA059), 1);	// in CReenvnderer::RenderRoads()
	}

	// fix blend mode
	if(isVC()){
		// auto aim
		MemoryVP::Patch<BYTE>(AddressByVersion<uint32_t>(0, 0, 0, 0x5D4EEE, 0x5D4F0E, 0x5D4CBE), rwBLENDINVSRCALPHA);
		// sniper dot
		MemoryVP::Patch<BYTE>(AddressByVersion<uint32_t>(0, 0, 0, 0x558024, 0x558044, 0x557F14), rwBLENDINVSRCALPHA);
	}

	if(isVC())
		MemoryVP::InjectHook(AddressByVersion<uint32_t>(0, 0, 0, 0x55EA39, 0x55EA59, 0x55E929), sniperTrailsHook, PATCH_JUMP);

	// when loaded late, init here; otherwise init with RW
	if(pRwCamera)
		neoInit();
	else
		MemoryVP::InjectHook(AddressByVersion<uint32_t>(0x48D52F, 0x48D62F, 0x48D5BF, 0x4A5B6B, 0x4A5B8B, 0x4A5A3B), CGame__InitialiseRenderWare_hook);

	if(isIII()){
		int n = GetPrivateProfileInt("SkyGfx", "txdLimit", 850, modulePath);
		if(n != 850){
			MemoryVP::Patch<int>(0x406979, n); //same address for all versions, lol
			MemoryVP::Patch<int>(AddressByVersion<uint32_t>(0x527458, 0x527698, 0x527628, 0, 0, 0), n);
		}

		// fall back to generic.txd when reading from dff
		MemoryVP::InjectHook(AddressByVersion<uint32_t>(0x5AAE1B, 0x5AB0DB, 0x5AD708, 0, 0, 0), RwTextureRead_generic);
	}
/*
	if(gtaversion == III_10){
//		MemoryVP::Patch<float>(0x45C120, 80.0f);
		*tanvalCar[1] = 9.0f;
		*(float*)0x5F53C4 = 1.244444444f;
		MemoryVP::InjectHook(0x45C334, &CCam::WorkOutCamHeight_hook);
		MemoryVP::InjectHook(0x459A9B, &CCam::Process_FollowPed_hook);
//		MemoryVP::InjectHook(0x459C97, &CCam::Process_Editor_hook);
//		MemoryVP::InjectHook(0x459ABA, &CCam::Process_Debug_hook);

		MemoryVP::Patch<int>(0x4A17F0, 6000);

		//MemoryVP::Nop(0x48C2FD, 5);
		//MemoryVP::Patch<char*>(0x524ED6, "DATA_ps2\\cullzone.dat");
		//MemoryVP::Patch<int>(0x524F4D, 9830);
	}
*/
	//if(gtaversion == VC_10){
	//	MemoryVP::InjectHook(0x40FBD3, openfile);
	//	MemoryVP::InjectHook(0x40FBE9, readfile);
	//	MemoryVP::InjectHook(0x40FDD9, readfile);
	//	MemoryVP::InjectHook(0x401000, printf, PATCH_JUMP);
	//}
}

BOOL WINAPI
DllMain(HINSTANCE hInst, DWORD reason, LPVOID)
{
	if(reason == DLL_PROCESS_ATTACH){
		dllModule = hInst;

		if(GetAsyncKeyState(VK_F8) & 0x8000){
			AllocConsole();
			freopen("CONIN$", "r", stdin);
			freopen("CONOUT$", "w", stdout);
			freopen("CONOUT$", "w", stderr);
		}
		AddressByVersion<uint32_t>(0, 0, 0, 0, 0, 0);
		if (gtaversion != -1)
			patch();
	}

	return TRUE;
}
