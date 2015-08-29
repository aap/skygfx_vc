#include "skygfx_vc.h"
#include "d3d8.h"
#include "d3d8types.h"

HMODULE dllModule;
int gtaversion = -1;

WRAPPER int rwD3D8RasterIsCubeRaster(RwRaster*) { EAXJMP(0x63EE40); }	// VC only
static uint32_t rpMatFXD3D8AtomicMatFXEnvRender_A = AddressByVersion<uint32_t>(0x5CF6C0, 0x674EE0);
WRAPPER int rpMatFXD3D8AtomicMatFXEnvRender(RxD3D8InstanceData*, int, int, RwTexture*, RwTexture*) { VARJMP(rpMatFXD3D8AtomicMatFXEnvRender_A); }
static uint32_t rpMatFXD3D8AtomicMatFXDefaultRender_A = AddressByVersion<uint32_t>(0x5CEB80, 0x674380);
WRAPPER int rpMatFXD3D8AtomicMatFXDefaultRender(RxD3D8InstanceData*, int, RwTexture*) { VARJMP(rpMatFXD3D8AtomicMatFXDefaultRender_A); }
int &MatFXMaterialDataOffset = *AddressByVersion<int*>(0x66188C, 0x7876CC);

//D3DMATERIAL8 &gMaterial = *(D3DMATERIAL8*)0x6DDEB8;
//D3DMATERIAL8 &gLastMaterial = *(D3DMATERIAL8*)0x789760;
//int &maxNumLights = *(int*)0x789BF4;
//int &lightsCache = *(int*)0x789BF8;

RwMatrix &defmat = *AddressByVersion<RwMatrix*>(0x5E6738, 0x67FB18);
//RwTexture *&reflectionTex = *(RwTexture**)0x9B5EF8;


void **&RwEngineInst = *AddressByVersion<void***>(0x661228, 0x7870C0);
RpLight *&pAmbient = *AddressByVersion<RpLight**>(0x885B6C, 0x974B44);
RpLight *&pDirect = *AddressByVersion<RpLight**>(0x880F7C, 0x94DD40);
RpLight **pExtraDirectionals = AddressByVersion<RpLight**>(0x60009C, 0x69A140);
int &NumExtraDirLightsInWorld = *AddressByVersion<int*>(0x64C608, 0x94DB48);

int blendstyle, texgenstyle;
int blendkey, texgenkey;

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

int reftex = 0;

void
ApplyEnvMapTextureMatrix(RwTexture *tex, int n, RwFrame *frame)
{
/*
	{
		static bool keystate = false;
		if(GetAsyncKeyState(VK_F6) & 0x8000){
			if(!keystate){
				keystate = true;
				reftex = (reftex+1)%2;
			}
		}else
			keystate = false;
	}
	if(reftex)
		tex = reflectionTex;
*/
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
			RwMatrixMultiply(m1, m2, m3);
	
			rwtod3dmat(&invmat, m1);
			RwD3D8SetTransform(D3DTS_TEXTURE0+n, &invmat);
		}else{
			RwMatrixInvert(m1, envfrm);
			RwMatrixMultiply(m2, m1, camfrm);
			m2->right.x = -m2->right.x;
			m2->right.y = -m2->right.y;
			m2->right.z = -m2->right.z;
			m2->flags = 0;
			m2->pos.x = 0.0f;
			m2->pos.y = 0.0f;
			m2->pos.z = 0.0f;
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

/*void RwD3D8GetLight(RwInt32 n, void *light)
{
	if(n >= maxNumLights)
		memset(light, 0, 0x68);
	memcpy(light, (const void *)(lightsCache + 108 * n), 0x68);
}*/

int
rpMatFXD3D8AtomicMatFXEnvRender_hook(RxD3D8InstanceData *inst, int flags, int sel, RwTexture *texture, RwTexture *envMap)
{
	MatFX *matfx = *RWPLUGINOFFSET(MatFX*, inst->material, MatFXMaterialDataOffset);
	MatFXEnv *env = &matfx->fx[sel];
	float factor = env->envCoeff*255.0f;
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
	RwD3D8SetTexture(NULL, 1);

	ApplyEnvMapTextureMatrix(envMap, 0, env->envFrame);
	RwUInt32 texfactor = ((intens | ((intens | (intens << 8)) << 8)) << 8) | intens;
	RwD3D8SetRenderState(D3DRS_TEXTUREFACTOR, texfactor);
	RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MULTIPLYADD);
	RwD3D8SetTextureStageState(1, D3DTSS_COLORARG0, D3DTA_CURRENT);
	RwD3D8SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	RwD3D8SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_TFACTOR);

	RwD3D8SetVertexShader(inst->vertexShader);
	RwD3D8SetStreamSource(0, inst->vertexBuffer, inst->stride);
	RwD3D8SetIndices(inst->indexBuffer, inst->baseIndex);
	if(inst->indexBuffer)
		RwD3D8DrawIndexedPrimitive(inst->primType, 0, inst->numVertices, 0, inst->numIndices);
	else
		RwD3D8DrawPrimitive(inst->primType, inst->baseIndex, inst->numVertices);
	RwD3D8SetTexture(NULL, 1);
	RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D8SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, 0);
	RwD3D8SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
	return 0;
}

int
rpMatFXD3D8AtomicMatFXEnvRender_dual(RxD3D8InstanceData *inst, int flags, int sel, RwTexture *texture, RwTexture *envMap)
{
	MatFX *matfx = *RWPLUGINOFFSET(MatFX*, inst->material, MatFXMaterialDataOffset);
	MatFXEnv *env = &matfx->fx[sel];
	float factor = env->envCoeff*255.0f;
	if(isIII())
		factor *= 2.0f;
	RwUInt8 intens = factor;

	{
		static bool keystate = false;
		if(GetAsyncKeyState(blendkey) & 0x8000){
			if(!keystate){
				keystate = true;
				blendstyle = (blendstyle+1)%3;
			}
		}else
			keystate = false;
	}
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
	if(blendstyle == 1){
		float saved = env->envCoeff;
		env->envCoeff *= 0.25f;
		int ret = rpMatFXD3D8AtomicMatFXEnvRender(inst, flags, sel, texture, envMap);
		env->envCoeff = saved;
		return ret;
	}
	if(blendstyle == 2)
		return rpMatFXD3D8AtomicMatFXEnvRender_xbox(inst, flags, sel, texture, envMap);

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

static uint32_t dualPassHook_R = AddressByVersion<uint32_t>(0x5DFBD8, 0x678DA8); 
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
	tex->filterAddressing = 0x1102;
	return tex;
}

static uint32_t RenderScene_A = AddressByVersion<uint32_t>(0x48E030, 0x4A6570); 
WRAPPER void RenderScene(void) { VARJMP(RenderScene_A); }

void
RenderScene_hook(void)
{
	RenderScene();

	if(blendstyle == 2)
		RenderEnvTex();
}

int
readhex(char *str)
{
	int n = 0;
	if(strlen(str) > 2)
		sscanf(str+2, "%X", &n);
	return n;
}

void
patch10(void)
{
	char tmp[32];
	char modulePath[MAX_PATH];

	GetModuleFileName(dllModule, modulePath, MAX_PATH);
	size_t nLen = strlen(modulePath);
	modulePath[nLen-1] = L'i';
	modulePath[nLen-2] = L'n';
	modulePath[nLen-3] = L'i';

	GetPrivateProfileString("SkyGfx", "texblendSwitchKey", "0x76", tmp, sizeof(tmp), modulePath);
	blendkey = readhex(tmp);
	GetPrivateProfileString("SkyGfx", "texgenSwitchKey", "0x77", tmp, sizeof(tmp), modulePath);
	texgenkey = readhex(tmp);
	blendstyle = GetPrivateProfileInt("SkyGfx", "texblendSwitch", 0, modulePath) % 3;
	texgenstyle = GetPrivateProfileInt("SkyGfx", "texgenSwitch", 0, modulePath) % 2;
	if(isVC() && GetPrivateProfileInt("SkyGfx", "IIIReflections", FALSE, modulePath)){
		MemoryVP::InjectHook(0x57A8BA, createIIIEnvFrame);
		MemoryVP::InjectHook(0x57A8C7, 0x57A8F4, PATCH_JUMP);
	}
	if(isIII() && GetPrivateProfileInt("SkyGfx", "viceCityReflections", FALSE, modulePath)){
		MemoryVP::InjectHook(0x5218A2, createVCEnvFrame);
		MemoryVP::InjectHook(0x5218AC, 0x52195E, PATCH_JUMP);
	}
	if(isVC())
		MemoryVP::Patch<DWORD>(0x699D44, 0x3f800000);
	MemoryVP::InjectHook(AddressByVersion<uint32_t>(0x5D0CE8, 0x6765C8), rpMatFXD3D8AtomicMatFXEnvRender_dual);
	MemoryVP::InjectHook(AddressByVersion<uint32_t>(0x5CFD40, 0x6755D0), ApplyEnvMapTextureMatrix, PATCH_JUMP);
	if(isIII())
		MemoryVP::InjectHook(0x59BABF, CreateTextureFilterFlags);


	if(GetPrivateProfileInt("SkyGfx", "dualPass", TRUE, modulePath))
		MemoryVP::InjectHook(AddressByVersion<uint32_t>(0x5DFB99, 0x678D69), dualPassHook, PATCH_JUMP);

	if(isVC() && GetPrivateProfileInt("SkyGfx", "disableBackfaceCulling", FALSE, modulePath)){
		// hope I didn't miss anything
		MemoryVP::Patch<BYTE>(0x4C9E5F, 1);	// in CRenderer::RenderOneNonRoad()
		MemoryVP::Patch<BYTE>(0x4C9F08, 1);	// in CRenderer::RenderBoats()
		MemoryVP::Patch<BYTE>(0x4C9F5D, 1);	// in CRenderer::RenderEverythingBarRoads()
		MemoryVP::Patch<BYTE>(0x4CA157, 1);	// in CRenderer::RenderFadingInEntities()
		MemoryVP::Patch<BYTE>(0x4CA199, 1);	// in CReenvnderer::RenderRoads()
	}

	// inject code to render xbox env map
	MemoryVP::InjectHook(AddressByVersion<uint32_t>(0x48E5F9, 0x4A604A), RenderScene_hook);

	// fix auto aim alpha
	if(isVC())
		MemoryVP::Patch<BYTE>(0x5D4EEE, rwBLENDINVSRCALPHA);

	MemoryVP::InjectHook(AddressByVersion<uint32_t>(0x5CDE9E, 0x66D65E), rpSkinD3D8CreatePlainPipe_hook);

//	MemoryVP::Nop(0x600F1C, 2);
//	MemoryVP::Nop(0x600F56, 2);
//
//	MemoryVP::Nop(0x6018DA, 6);
//	MemoryVP::Nop(0x601910, 2);
//	char *fmtstr = "%lu Y %lu Y %lu";
//	MemoryVP::Patch(0x601970, fmtstr);

//	MemoryVP::InjectHook(0x401000, printf, PATCH_JUMP);
}

BOOL WINAPI
DllMain(HINSTANCE hInst, DWORD reason, LPVOID)
{
	if(reason == DLL_PROCESS_ATTACH){
		dllModule = hInst;

/*		AllocConsole();
		freopen("CONIN$", "r", stdin);
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);*/

		AddressByVersion<uint32_t>(0, 0);
		if(gtaversion == III_10 || gtaversion == VC_10)
			patch10();
	}

	return TRUE;
}
