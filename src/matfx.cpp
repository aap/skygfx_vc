#include "skygfx.h"
#include "d3d8.h"
#include "d3d8types.h"
#include <DirectXMath.h>

static void *ps2StandardVS;
static void *ps2StandardEnvVS;
static void *ps2StandardEnvOnlyVS;
static void *pcStandardVS;
static void *pcStandardEnvVS;
static void *pcStandardEnvOnlyVS;
static void *nolightVS;
static void *nolightEnvVS;
static void *nolightEnvOnlyVS;
static void *pcEnvPS;
static void *mobileEnvPS;

void
_rpMatFXD3D8AtomicMatFXRenderBlack_fixed(RxD3D8InstanceData *inst)
{
	// FIX: set texture to get its alpha
	RwD3D8SetTexture(inst->material->texture, 0);
	// have to reset this to ignore vertex colours
	RwD3D8SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2);
	RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);


	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)(inst->vertexAlpha || inst->material->color.alpha != 0xFF));
	RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, inst->vertexAlpha ? D3DMCS_COLOR1 : D3DMCS_MATERIAL);

	RwD3D8SetPixelShader(nil);
	RwD3D8SetVertexShader(inst->vertexShader);
	RwD3D8SetStreamSource(0, inst->vertexBuffer, inst->stride);
	if(inst->indexBuffer){
		RwD3D8SetIndices(inst->indexBuffer, inst->baseIndex);
		RwD3D8DrawIndexedPrimitive(inst->primType, 0, inst->numVertices, 0, inst->numIndices);
	}else
		RwD3D8DrawPrimitive(inst->primType, inst->baseIndex, inst->numVertices);
}

void
_rpMatFXD3D8AtomicMatFXDefaultRender(RxD3D8InstanceData *inst, int flags, RwTexture *texture)
{
	if(flags & (rpGEOMETRYTEXTURED|rpGEOMETRYTEXTURED2) && texture)
		RwD3D8SetTexture(texture, 0);
	else
		RwD3D8SetTexture(NULL, 0);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)(inst->vertexAlpha || inst->material->color.alpha != 0xFF));
	RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, inst->vertexAlpha != 0);
	RwD3D8SetPixelShader(0);
	RwD3D8SetVertexShader(inst->vertexShader);
	RwD3D8SetStreamSource(0, inst->vertexBuffer, inst->stride);
	drawDualPass(inst);
}


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



addr ApplyEnvMapTextureMatrix_A = AddressByVersion<addr>(0x5CFD40, 0x5D0000, 0x5D89E0, 0x6755D0, 0x675620, 0x674580);
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

// map [-1; -1] -> [0; 1], flip V
static RwMatrix scalenormal = {
	{ 0.5f, 0.0f, 0.0f }, 0,
	{ 0.0f, -0.5f, 0.0f }, 0,
	{ 0.0f, 0.0f, 1.0f }, 0,
	{ 0.5f, 0.5f, 0.0f }, 0,
	
};

// flipped U for PS2
static RwMatrix scalenormal_flipU = {
	{ -0.5f, 0.0f, 0.0f }, 0,
	{ 0.0f, -0.5f, 0.0f }, 0,
	{ 0.0f, 0.0f, 1.0f }, 0,
	{ 0.5f, 0.5f, 0.0f }, 0,
	
};

void
ApplyEnvMapTextureMatrix_hook(RwTexture *tex, int n, RwFrame *frame)
{
	RwD3D8SetTexture(tex, n);
	if(isVC() && rwD3D8RasterIsCubeRaster(tex->raster)){
		RwD3D8SetTextureStageState(n, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR);
		return;
	}
	RwD3D8SetTextureStageState(n, D3DRS_ALPHAREF, 2);
	RwD3D8SetTextureStageState(n, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACENORMAL);
	if(frame){
		RwMatrix *envframemat = RwMatrixCreate();
		RwMatrix *tmpmat = RwMatrixCreate();
		RwMatrix *envmat = RwMatrixCreate();

		RwMatrixInvert(envframemat, RwFrameGetLTM(frame));
		if(config.texgenstyle == 0){
			// PS2
			// can this be simplified?
			*tmpmat = *RwFrameGetLTM(RwCameraGetFrame((RwCamera*)RWSRCGLOBAL(curCamera)));
			RwV3dNegate(&tmpmat->right, &tmpmat->right);
			tmpmat->flags = 0;
			tmpmat->pos.x = 0.0f;
			tmpmat->pos.y = 0.0f;
			tmpmat->pos.z = 0.0f;
			RwMatrixMultiply(envmat, tmpmat, envframemat);
			*tmpmat = *envmat;
			// important because envframemat can have a translation that we don't like
			tmpmat->pos.x = 0.0f;
			tmpmat->pos.y = 0.0f;
			tmpmat->pos.z = 0.0f;
			// for some reason we flip in U as well
			RwMatrixMultiply(envmat, tmpmat, &scalenormal_flipU);
		}else if(config.texgenstyle == 2){
			static RwMatrix tmpmat2;
			// Mobile
			*tmpmat = *RwFrameGetLTM(RwCameraGetFrame((RwCamera*)RWSRCGLOBAL(curCamera)));
			tmpmat2 = *tmpmat;
			RwV3dNegate(&tmpmat->right, &tmpmat->right);
			tmpmat->flags = 0;
			tmpmat->pos.x = 0.0f;
			tmpmat->pos.y = 0.0f;
			tmpmat->pos.z = 0.0f;
			tmpmat2.pos.x = 0.5f;
			tmpmat2.pos.y = 0.5f;
			RwMatrixMultiply(envframemat, tmpmat, &tmpmat2);
			RwMatrixMultiply(envmat, envframemat, &scalenormal);
		}else{
			// PC
			RwMatrixMultiply(tmpmat, envframemat, RwFrameGetLTM(RwCameraGetFrame((RwCamera*)RWSRCGLOBAL(curCamera))));
			RwV3dNegate(&tmpmat->right, &tmpmat->right);
			tmpmat->flags = 0;
			tmpmat->pos.x = 0.0f;
			tmpmat->pos.y = 0.0f;
			tmpmat->pos.z = 0.0f;
			RwMatrixMultiply(envmat, tmpmat, &scalenormal);
		}

		RwD3D8SetTransform(D3DTS_TEXTURE0+n, envmat);

		RwMatrixDestroy(envmat);
		RwMatrixDestroy(tmpmat);
		RwMatrixDestroy(envframemat);
	}else
		RwD3D8SetTransform(D3DTS_TEXTURE0+n, &scalenormal);
}



#ifdef DEBUG
float envmult = 1.0f;
bool ps2filtering = false;
#endif

//#define ENVTEST

#ifdef ENVTEST
bool envdebug = false;
#endif


static addr _rpMatFXD3D8AtomicMatFXBumpMapRender_A = AddressByVersion<addr>(0x5CFE40, 0, 0, 0x6756F0, 0, 0);
WRAPPER void _rpMatFXD3D8AtomicMatFXBumpMapRender(RxD3D8InstanceData *inst, int flags, RwTexture *texture, RwTexture *bumpMap, RwTexture *envMap) { VARJMP(_rpMatFXD3D8AtomicMatFXBumpMapRender_A); }

static addr _rpMatFXD3D8AtomicMatFXDualPassRender_A = AddressByVersion<addr>(0x5CED20, 0, 0, 0x674510, 0, 0);
WRAPPER void _rpMatFXD3D8AtomicMatFXDualPassRender(RxD3D8InstanceData *inst, int flags, RwTexture *texture, RwTexture *dualTexture) { VARJMP(_rpMatFXD3D8AtomicMatFXDualPassRender_A); }

void
_rpMatFXD3D8AtomicMatFXEnvRender_ps2(RxD3D8InstanceData *inst, int flags, int sel, RwTexture *texture, RwTexture *envMap)
{
	MatFX *matfx = *RWPLUGINOFFSET(MatFX*, inst->material, MatFXMaterialDataOffset);
	MatFXEnv *env = &matfx->fx[sel].e;

	// Render PC envmap
	if(config.blendstyle == 1){
		float oldcoeff = env->envCoeff;
		static float carMult = isIII() ? 0.5f : 0.25f;
		if(env->envFBalpha == 0)	// hacky way to distinguish vehicles from water
			env->envCoeff *= carMult;
		rpMatFXD3D8AtomicMatFXEnvRender(inst, flags, sel, texture, envMap);
		env->envCoeff = oldcoeff;
		return;
	}
	if(config.blendstyle == 2)
		// not supported without shader
		return;

	uint8 intens = (uint8)(env->envCoeff*255.0f);

#ifdef DEBUG
// some debug stuff
//if(GetAsyncKeyState(VK_F1) & 0x8000)
//	intens = intens/2;
//if(GetAsyncKeyState(VK_F2) & 0x8000)
//	intens = 0;

intens *= envmult;

#ifdef ENVTEST
static int &frameCounter = *(int*)0x9412EC;
static int lastFrame;
static RwTexture *envtex;
extern RwTexture *reloadTestTex(void);
	if(envtex == nil || ((frameCounter & 0xF)==0 && lastFrame != frameCounter)){
		lastFrame = frameCounter;
		envtex = reloadTestTex();
	}
	envMap = envtex;
if(envdebug){
	intens = 255;
}
#endif
#endif

	if(intens == 0 || !envMap){
		if(sel == 0)
			_rpMatFXD3D8AtomicMatFXDefaultRender(inst, flags, texture);
		return;
	}

#ifdef DEBUG
// simulate broken filtering
if(ps2filtering)
	envMap->filterAddressing = texture == nil ? 0x1101 : texture->filterAddressing;
else
	envMap->filterAddressing = 0x1102;
#endif

	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)(inst->vertexAlpha || inst->material->color.alpha != 0xFF));
	if(flags & (rpGEOMETRYTEXTURED|rpGEOMETRYTEXTURED2) && texture)
		RwD3D8SetTexture(texture, 0);
	else
		RwD3D8SetTexture(nil, 0);
	RwD3D8SetVertexShader(inst->vertexShader);
	RwD3D8SetStreamSource(0, inst->vertexBuffer, inst->stride);
	RwD3D8SetIndices(inst->indexBuffer, inst->baseIndex);
	if(inst->indexBuffer)
		RwD3D8DrawIndexedPrimitive(inst->primType, 0, inst->numVertices, 0, inst->numIndices);
	else
		RwD3D8DrawPrimitive(inst->primType, inst->baseIndex, inst->numVertices);

	// Effect pass
	
	ApplyEnvMapTextureMatrix(envMap, 0, env->envFrame);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
	RwUInt32 src, dst, lighting, zwrite, fog, fogcol;
	RwRenderStateGet(rwRENDERSTATESRCBLEND, &src);
	RwRenderStateGet(rwRENDERSTATEDESTBLEND, &dst);

	// This is of course not using framebuffer alpha,
	// but if the diffuse texture had no alpha, the result should actually be rather the same
	if(env->envFBalpha)
		RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
	else
		RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
#ifdef ENVTEST
	if(envdebug){
		RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
		RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
	}
#endif
	RwD3D8GetRenderState(D3DRS_LIGHTING, &lighting);
	RwD3D8GetRenderState(D3DRS_ZWRITEENABLE, &zwrite);
	RwD3D8GetRenderState(D3DRS_FOGENABLE, &fog);
	RwD3D8SetRenderState(D3DRS_ZWRITEENABLE, 0);
	if(fog){
		RwD3D8GetRenderState(D3DRS_FOGCOLOR, &fogcol);
		RwD3D8SetRenderState(D3DRS_FOGCOLOR, 0);
	}

	D3DCOLOR texfactor = D3DCOLOR_RGBA(intens, intens, intens, intens);
	RwD3D8SetRenderState(D3DRS_TEXTUREFACTOR, texfactor);
	RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MODULATE);
	RwD3D8SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_CURRENT);
	RwD3D8SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_TFACTOR);
#ifdef ENVTEST
	if(envdebug)
		RwD3D8SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
#endif
	// alpha unused
	//RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	//RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_CURRENT);
	//RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);

	if(inst->indexBuffer)
		RwD3D8DrawIndexedPrimitive(inst->primType, 0, inst->numVertices, 0, inst->numIndices);
	else
		RwD3D8DrawPrimitive(inst->primType, inst->baseIndex, inst->numVertices);

	// Reset states

	rwD3D8RenderStateVertexAlphaEnable(0);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)src);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)dst);
	RwD3D8SetRenderState(D3DRS_LIGHTING, lighting);
	RwD3D8SetRenderState(D3DRS_ZWRITEENABLE, zwrite);
	if(fog)
		RwD3D8SetRenderState(D3DRS_FOGCOLOR, fogcol);
	RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	RwD3D8SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, 0);
	RwD3D8SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
}





//
// Shader pipeline
//

void
RwD3D9SetVertexShaderMatrix(RwUInt32 registerAddress, const RwMatrix *rwmat)
{
	float d3dmat[16];
	d3dmat[0] = rwmat->right.x;
	d3dmat[1] = rwmat->up.x;
	d3dmat[2] = rwmat->at.x;
	d3dmat[3] = rwmat->pos.x;
	d3dmat[4] = rwmat->right.y;
	d3dmat[5] = rwmat->up.y;
	d3dmat[6] = rwmat->at.y;
	d3dmat[7] = rwmat->pos.y;
	d3dmat[8] = rwmat->right.z;
	d3dmat[9] = rwmat->up.z;
	d3dmat[10] = rwmat->at.z;
	d3dmat[11] = rwmat->pos.z;
	d3dmat[12] = 0.0f;
	d3dmat[13] = 0.0f;
	d3dmat[14] = 0.0f;
	d3dmat[15] = 1.0f;
	RwD3D9SetVertexShaderConstant(registerAddress, d3dmat, 4);
}

// Calculate matrix for world-space normals
void
ApplyEnvMapTextureMatrix_world(RwTexture *tex, int n, RwFrame *frame)
{
	RwD3D8SetTexture(tex, n);
	RwD3D8SetTextureStageState(n, D3DRS_ALPHAREF, 2);
	if(frame){
		RwMatrix *envframemat = RwMatrixCreate();
		RwMatrix *tmpmat = RwMatrixCreate();
		RwMatrix *envmat = RwMatrixCreate();

		if(config.texgenstyle == 0){
			// PS2
			RwMatrixInvert(envframemat, RwFrameGetLTM(frame));
			envframemat->pos.x = 0.0f;
			envframemat->pos.y = 0.0f;
			envframemat->pos.z = 0.0f;
			envframemat->flags = 0;
			// for some reason we flip U
			RwMatrixMultiply(envmat, envframemat, &scalenormal_flipU);
		}else if(config.texgenstyle == 2){
			// Mobile
			*envframemat = *RwFrameGetLTM(RwCameraGetFrame((RwCamera*)RWSRCGLOBAL(curCamera)));
			envframemat->pos.x = 0.5f;
			envframemat->pos.y = 0.5f;
			envframemat->pos.z = 0.0f;
			envframemat->flags = 0;
			RwMatrixMultiply(envmat, envframemat, &scalenormal);
		}else{
			// PC
			RwMatrixInvert(envframemat, RwFrameGetLTM(frame));
			RwMatrixMultiply(tmpmat, envframemat, RwFrameGetLTM(RwCameraGetFrame((RwCamera*)RWSRCGLOBAL(curCamera))));
			RwV3dNegate(&tmpmat->right, &tmpmat->right);
			tmpmat->pos.x = 0.0f;
			tmpmat->pos.y = 0.0f;
			tmpmat->pos.z = 0.0f;
			tmpmat->flags = 0;
			RwMatrixMultiply(envmat, tmpmat, &scalenormal);
			// since we have world space normals, also apply view matrix
			RwMatrixInvert(envframemat, RwFrameGetLTM(RwCameraGetFrame((RwCamera*)RWSRCGLOBAL(curCamera))));
			envframemat->right.x = -envframemat->right.x;
			envframemat->up.x = -envframemat->up.x;
			envframemat->at.x = -envframemat->at.x;
			envframemat->pos.x = 0.0f;
			envframemat->pos.y = 0.0f;
			envframemat->pos.z = 0.0f;
			envframemat->flags = 0;
			RwMatrixMultiply(tmpmat, envframemat, envmat);
			*envmat = *tmpmat;
		}

		RwD3D9SetVertexShaderMatrix(LOC_tex, envmat);

		RwMatrixDestroy(envmat);
		RwMatrixDestroy(tmpmat);
		RwMatrixDestroy(envframemat);
	}else
		RwD3D9SetVertexShaderMatrix(LOC_tex, &scalenormal);
}


void
_rpMatFXD3D8AtomicMatFXDefaultRender_shader(RxD3D8InstanceData *inst, int flags, RwTexture *texture)
{
	if(flags & (rpGEOMETRYTEXTURED|rpGEOMETRYTEXTURED2) && texture)
		RwD3D8SetTexture(texture, 0);
	else
		RwD3D8SetTexture(NULL, 0);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)(inst->vertexAlpha || inst->material->color.alpha != 0xFF));
	RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, inst->vertexAlpha != 0);
	RwD3D8SetPixelShader(0);
	RwD3D9SetFVF(inst->vertexShader);
	void *lightingShader = config.ps2light ? ps2StandardVS : pcStandardVS;
	RwD3D9SetVertexShader(flags&rpGEOMETRYLIGHT ? lightingShader : nolightVS);
	RwD3D8SetStreamSource(0, inst->vertexBuffer, inst->stride);
	drawDualPass(inst);
	RwD3D9SetVertexShader(nil);
}

// PS2-style env map
void
_rpMatFXD3D8AtomicMatFXEnvRender_ps2_shader(RxD3D8InstanceData *inst, int flags, int sel, RwTexture *texture, RwTexture *envMap)
{
	// Diffuse pass
	_rpMatFXD3D8AtomicMatFXDefaultRender_shader(inst, flags, texture);

	// Env pass
	MatFX *matfx = *RWPLUGINOFFSET(MatFX*, inst->material, MatFXMaterialDataOffset);
	MatFXEnv *env = &matfx->fx[sel].e;
#ifdef DEBUG
env->envCoeff *= envmult;
// simulate broken filtering
if(ps2filtering)
	envMap->filterAddressing = texture == nil ? 0x1101 : texture->filterAddressing;
else
	envMap->filterAddressing = 0x1102;
#endif
	ApplyEnvMapTextureMatrix_world(envMap, 0, env->envFrame);
	void *lightingShader = config.ps2light ? ps2StandardEnvOnlyVS : pcStandardEnvOnlyVS;
	RwD3D9SetVertexShader(flags&rpGEOMETRYLIGHT ? lightingShader : nolightEnvOnlyVS);

	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
	RwUInt32 src, dst, zwrite, fog, fogcol;
	RwRenderStateGet(rwRENDERSTATESRCBLEND, &src);
	RwRenderStateGet(rwRENDERSTATEDESTBLEND, &dst);

	// This is of course not using framebuffer alpha,
	// but if the diffuse texture had no alpha, the result should actually be rather the same
	if(env->envFBalpha)
		RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
	else
		RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
	RwD3D8GetRenderState(D3DRS_ZWRITEENABLE, &zwrite);
	RwD3D8GetRenderState(D3DRS_FOGENABLE, &fog);
	RwD3D8SetRenderState(D3DRS_ZWRITEENABLE, 0);
	if(fog){
		RwD3D8GetRenderState(D3DRS_FOGCOLOR, &fogcol);
		RwD3D8SetRenderState(D3DRS_FOGCOLOR, 0);
	}

	uint8 intens = (uint8)(env->envCoeff*255.0f);
	D3DCOLOR texfactor = D3DCOLOR_RGBA(intens, intens, intens, intens);
	RwD3D8SetRenderState(D3DRS_TEXTUREFACTOR, texfactor);
	RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MODULATE);
	RwD3D8SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_CURRENT);
	RwD3D8SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_TFACTOR);

	if(inst->indexBuffer)
		RwD3D8DrawIndexedPrimitive(inst->primType, 0, inst->numVertices, 0, inst->numIndices);
	else
		RwD3D8DrawPrimitive(inst->primType, inst->baseIndex, inst->numVertices);

	// Reset states

	rwD3D8RenderStateVertexAlphaEnable(0);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)src);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)dst);
	RwD3D8SetRenderState(D3DRS_ZWRITEENABLE, zwrite);
	if(fog)
		RwD3D8SetRenderState(D3DRS_FOGCOLOR, fogcol);
	RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	RwD3D8SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, 0);
	RwD3D8SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
	RwD3D9SetVertexShader(nil);
}

// PC-style env map
void
_rpMatFXD3D8AtomicMatFXEnvRender_pc_shader(RxD3D8InstanceData *inst, int flags, int sel, RwTexture *texture, RwTexture *envMap)
{
	MatFX *matfx = *RWPLUGINOFFSET(MatFX*, inst->material, MatFXMaterialDataOffset);
	MatFXEnv *env = &matfx->fx[sel].e;

	if(flags & (rpGEOMETRYTEXTURED|rpGEOMETRYTEXTURED2) && texture)
		RwD3D8SetTexture(texture, 0);
	else
		RwD3D8SetTexture(NULL, 0);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)(inst->vertexAlpha || inst->material->color.alpha != 0xFF));
	RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, inst->vertexAlpha != 0);

	RwD3D9SetPixelShader(pcEnvPS);
	ApplyEnvMapTextureMatrix_world(envMap, 1, env->envFrame);
	float envCoeff[4] = { env->envCoeff, env->envCoeff, env->envCoeff, env->envCoeff };
	RwD3D9SetPixelShaderConstant(0, envCoeff, 1);

	RwD3D9SetFVF(inst->vertexShader);
	void *lightingShader = config.ps2light ? ps2StandardEnvVS : pcStandardEnvVS;
	RwD3D9SetVertexShader(flags&rpGEOMETRYLIGHT ? lightingShader : nolightEnvVS);
	RwD3D8SetStreamSource(0, inst->vertexBuffer, inst->stride);
	drawDualPass(inst);

	RwD3D9SetPixelShader(nil);
	RwD3D9SetVertexShader(nil);
	RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
}

// Mobile-style env map
void
_rpMatFXD3D8AtomicMatFXEnvRender_mobile_shader(RxD3D8InstanceData *inst, int flags, int sel, RwTexture *texture, RwTexture *envMap)
{
	MatFX *matfx = *RWPLUGINOFFSET(MatFX*, inst->material, MatFXMaterialDataOffset);
	MatFXEnv *env = &matfx->fx[sel].e;

	if(flags & (rpGEOMETRYTEXTURED|rpGEOMETRYTEXTURED2) && texture)
		RwD3D8SetTexture(texture, 0);
	else
		RwD3D8SetTexture(NULL, 0);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)(inst->vertexAlpha || inst->material->color.alpha != 0xFF));
	RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, inst->vertexAlpha != 0);

	RwD3D9SetPixelShader(mobileEnvPS);
	ApplyEnvMapTextureMatrix_world(envMap, 1, env->envFrame);
	float envCoeff[4] = { env->envCoeff*1.45f, env->envCoeff*1.45f, env->envCoeff*1.45f, env->envCoeff*1.45f };
	RwD3D9SetPixelShaderConstant(0, envCoeff, 1);

	RwD3D9SetFVF(inst->vertexShader);
	void *lightingShader = config.ps2light ? ps2StandardEnvVS : pcStandardEnvVS;
	RwD3D9SetVertexShader(flags&rpGEOMETRYLIGHT ? lightingShader : nolightEnvVS);
	RwD3D8SetStreamSource(0, inst->vertexBuffer, inst->stride);
	drawDualPass(inst);

	RwD3D9SetPixelShader(nil);
	RwD3D9SetVertexShader(nil);
	RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
}

void
_rpMatFXD3D8AtomicMatFXEnvRender_switch_shader(RxD3D8InstanceData *inst, int flags, int sel, RwTexture *texture, RwTexture *envMap)
{
	float oldcoeff;
	static float carMult = isIII() ? 0.5f : 0.25f;

	MatFX *matfx = *RWPLUGINOFFSET(MatFX*, inst->material, MatFXMaterialDataOffset);
	MatFXEnv *env = &matfx->fx[sel].e;

	if(env->envCoeff == 0.0f || !envMap){
		if(sel == 0)
			_rpMatFXD3D8AtomicMatFXDefaultRender_shader(inst, flags, texture);
		return;
	}

	oldcoeff = env->envCoeff;
	switch(config.blendstyle){
	// PS2
	case 0:
		_rpMatFXD3D8AtomicMatFXEnvRender_ps2_shader(inst, flags, sel, texture, envMap);
		break;
	// PC
	case 1:
		if(env->envFBalpha == 0)	// hacky way to distinguish vehicles from water
			env->envCoeff *= carMult;
		_rpMatFXD3D8AtomicMatFXEnvRender_pc_shader(inst, flags, sel, texture, envMap);
		break;
	// Mobile
	case 2:
		if(env->envFBalpha == 0){	// hacky way to distinguish vehicles from water
			if(isIII())
				env->envCoeff *= 0.5f;
			else
				env->envCoeff = 0.15f;	// this is kinda shit
		}
		_rpMatFXD3D8AtomicMatFXEnvRender_mobile_shader(inst, flags, sel, texture, envMap);
		break;
	}
	env->envCoeff = oldcoeff;
}

void
MatFXShaderSetup(RwMatrix *world, RwUInt32 flags)
{
	int lighting = !!(flags & rpGEOMETRYLIGHT);
	DirectX::XMMATRIX worldMat, viewMat, projMat, texMat;
	RwCamera *cam = RwCameraGetCurrentCamera();

	RwMatrix view;
	RwMatrixInvert(&view, RwFrameGetLTM(RwCameraGetFrame(cam)));

	RwToD3DMatrix(&worldMat, world);
	RwToD3DMatrix(&viewMat, &view);
	viewMat.r[0] = DirectX::XMVectorNegate(viewMat.r[0]);
	RwD3D8GetTransform(D3DTS_PROJECTION, &projMat);
	projMat = DirectX::XMMatrixTranspose(projMat);

	DirectX::XMMATRIX combined = DirectX::XMMatrixMultiply(projMat, DirectX::XMMatrixMultiply(viewMat, worldMat));
	RwD3D9SetVertexShaderConstant(LOC_combined, (void*)&combined, 4);
	RwD3D9SetVertexShaderConstant(LOC_world, (void*)&worldMat, 4);

	if(lighting && pAmbient && rwObjectTestFlags(pAmbient, rpLIGHTLIGHTATOMICS))
		UploadLightColor(pAmbient, LOC_ambient);
	else
		UploadZero(LOC_ambient);
	if(lighting && pDirect && rwObjectTestFlags(pDirect, rpLIGHTLIGHTATOMICS)){
		UploadLightColor(pDirect, LOC_directCol);
		UploadLightDirection(pDirect, LOC_directDir);
	}else{
		UploadZero(LOC_directCol);
		UploadZero(LOC_directDir);
	}
	for(int i = 0 ; i < 4; i++)
		if(i < NumExtraDirLightsInWorld &&
		   lighting &&
		   RpLightGetType(pExtraDirectionals[i]) == rpLIGHTDIRECTIONAL &&
		   rwObjectTestFlags(pExtraDirectionals[i], rpLIGHTLIGHTATOMICS)){
			UploadLightDirection(pExtraDirectionals[i], LOC_lightDir+i);
			UploadLightColor(pExtraDirectionals[i], LOC_lightCol+i);
		}else{
			UploadZero(LOC_lightDir+i);
			UploadZero(LOC_lightCol+i);
		}
}


void
_rwD3D8AtomicMatFXRenderCallback(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
	RwBool lighting;
	RwBool forceBlack;
	RxD3D8ResEntryHeader *header;
	RxD3D8InstanceData *inst;
	RwInt32 i;

	static bool useShader = RwD3D9Supported();

	if(flags & rpGEOMETRYPRELIT){
		RwD3D8SetRenderState(D3DRS_COLORVERTEX, 1);
		RwD3D8SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_COLOR1);
	}else{
		RwD3D8SetRenderState(D3DRS_COLORVERTEX, 0);
		RwD3D8SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL);
	}

	_rwD3D8EnableClippingIfNeeded(object, type);

	RwD3D8GetRenderState(D3DRS_LIGHTING, &lighting);
	if(lighting || flags&rpGEOMETRYPRELIT){
		forceBlack = 0;
	}else{
		forceBlack = 1;
		RwD3D8SetTexture(nil, 0);
		RwD3D8SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_RGBA(0, 0, 0, 255));
		RwD3D8SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
		RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
	}

	if(useShader)
		MatFXShaderSetup(RwFrameGetLTM(RpAtomicGetFrame((RpAtomic*)object)), flags);

	header = (RxD3D8ResEntryHeader*)(repEntry + 1);
	inst = (RxD3D8InstanceData*)(header + 1);
	for(i = 0; i < header->numMeshes; i++){
		if(forceBlack)
			_rpMatFXD3D8AtomicMatFXRenderBlack_fixed(inst);
		else{
			if(lighting)
				RwD3D8SetSurfaceProperties(&inst->material->color, &inst->material->surfaceProps, flags & rpGEOMETRYMODULATEMATERIALCOLOR);
			MatFX *matfx = *RWPLUGINOFFSET(MatFX*, inst->material, MatFXMaterialDataOffset);
			int effect = matfx ? matfx->effects : rpMATFXEFFECTNULL;

			if(useShader){
				if(lighting){
					RwRGBAReal color;
					RwD3D9SetVertexShaderConstant(LOC_surfProps, &inst->material->surfaceProps, 1);
					RwRGBARealFromRwRGBA(&color, &inst->material->color);
					RwD3D9SetVertexShaderConstant(LOC_matCol, &color, 1);
				}else{
					RwRGBAReal black = { 0.0f, 0.0f, 0.0f, 1.0f };
					RwD3D9SetVertexShaderConstant(LOC_surfProps, &black, 1);
					RwD3D9SetVertexShaderConstant(LOC_matCol, &black, 1);
				}
			}

			switch(effect){
			case rpMATFXEFFECTNULL:
			default:
				if(useShader)
					_rpMatFXD3D8AtomicMatFXDefaultRender_shader(inst, flags, inst->material->texture);
				else
					_rpMatFXD3D8AtomicMatFXDefaultRender(inst, flags, inst->material->texture);
				break;
			case rpMATFXEFFECTBUMPMAP:
				_rpMatFXD3D8AtomicMatFXBumpMapRender(inst, flags, inst->material->texture, matfx->fx[0].b.bumpedTex, nil);
				break;
			case rpMATFXEFFECTENVMAP:
				if(useShader)
					_rpMatFXD3D8AtomicMatFXEnvRender_switch_shader(inst, flags, 0, inst->material->texture, matfx->fx[0].e.envTex);
				else
					_rpMatFXD3D8AtomicMatFXEnvRender_ps2(inst, flags, 0, inst->material->texture, matfx->fx[0].e.envTex);
				break;
			case rpMATFXEFFECTBUMPENVMAP:
				_rpMatFXD3D8AtomicMatFXBumpMapRender(inst, flags, inst->material->texture, matfx->fx[0].b.bumpedTex, matfx->fx[1].e.envTex);
				break;
			case rpMATFXEFFECTDUAL:
				_rpMatFXD3D8AtomicMatFXDualPassRender(inst, flags, inst->material->texture, matfx->fx[0].d.dualTex);
				break;
			}
		}
		inst++;
	}

	if(forceBlack){
		RwD3D8SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
		RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	}
}

int (*_rpMatFXPipelinesCreate_orig)(void);
int _rpMatFXPipelinesCreate(void)
{
	// This can be called multiple times, when RW is restarted!
	// So don't assume old pointers are valid

	// MULTIPLE INIT
	if(RwD3D9Supported()){
		{
			#include "ps2StandardVS.h"
			RwD3D9CreateVertexShader((RwUInt32*)g_vs20_main, &ps2StandardVS);
			assert(ps2StandardVS);
		}
		{
			#include "ps2StandardEnvVS.h"
			RwD3D9CreateVertexShader((RwUInt32*)g_vs20_main, &ps2StandardEnvVS);
			assert(ps2StandardEnvVS);
		}
		{
			#include "ps2StandardEnvOnlyVS.h"
			RwD3D9CreateVertexShader((RwUInt32*)g_vs20_main, &ps2StandardEnvOnlyVS);
			assert(ps2StandardEnvOnlyVS);
		}
		{
			#include "pcStandardVS.h"
			RwD3D9CreateVertexShader((RwUInt32*)g_vs20_main, &pcStandardVS);
			assert(pcStandardVS);
		}
		{
			#include "pcStandardEnvVS.h"
			RwD3D9CreateVertexShader((RwUInt32*)g_vs20_main, &pcStandardEnvVS);
			assert(pcStandardEnvVS);
		}
		{
			#include "pcStandardEnvOnlyVS.h"
			RwD3D9CreateVertexShader((RwUInt32*)g_vs20_main, &pcStandardEnvOnlyVS);
			assert(pcStandardEnvOnlyVS);
		}
		{
			#include "nolightVS.h"
			RwD3D9CreateVertexShader((RwUInt32*)g_vs20_main, &nolightVS);
			assert(nolightVS);
		}
		{
			#include "nolightEnvVS.h"
			RwD3D9CreateVertexShader((RwUInt32*)g_vs20_main, &nolightEnvVS);
			assert(nolightEnvVS);
		}
		{
			#include "nolightEnvOnlyVS.h"
			RwD3D9CreateVertexShader((RwUInt32*)g_vs20_main, &nolightEnvOnlyVS);
			assert(nolightEnvOnlyVS);
		}


		{
			#include "pcEnvPS.h"
			RwD3D9CreatePixelShader((RwUInt32*)g_ps20_main, &pcEnvPS);
			assert(pcEnvPS);
		}
		{
			#include "mobileEnvPS.h"
			RwD3D9CreatePixelShader((RwUInt32*)g_ps20_main, &mobileEnvPS);
			assert(mobileEnvPS);
		}
	}

	return _rpMatFXPipelinesCreate_orig();
}
