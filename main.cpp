#include "skygfx_vc.h"
#include "d3d8.h"
#include "d3d8types.h"

HMODULE dllModule;

WRAPPER int rpMatFXD3D8AtomicMatFXEnvRender(RxD3D8InstanceData*, int, int, RwTexture*, RwTexture*) { EAXJMP(0x674EE0); }
WRAPPER int rpMatFXD3D8AtomicMatFXDefaultRender(RxD3D8InstanceData*, int, RwTexture*) { EAXJMP(0x674380); }
int &MatFXMaterialDataOffset = *(int*)0x7876CC;

D3DMATERIAL8 &gMaterial = *(D3DMATERIAL8*)0x6DDEB8;
D3DMATERIAL8 &gLastMaterial = *(D3DMATERIAL8*)0x789760;
int &maxNumLights = *(int*)0x789BF4;
int &lightsCache = *(int*)0x789BF8;

RwMatrix &defmat = *(RwMatrix*)0x67FB18;
RwTexture *&reflectionTex = *(RwTexture**)0x9B5EF8;

WRAPPER int rwD3D8RasterIsCubeRaster(RwRaster*) { EAXJMP(0x63EE40); }

int &hour = *(int*)0xA10B6B;
int &minute = *(int*)0xA10B92;

IUnknown *&RwD3DDevice = *(IUnknown**)0x7897A8;

int iCanHasD3D9 = 0;
void **&RwEngineInst = *(void***)0x7870C0;
RpLight *&pAmbient = *(RpLight**)0x974B44;
RpLight *&pDirect = *(RpLight**)0x94DD40;
RpLight **pExtraDirectionals = (RpLight**)0x69A140;
int &NumExtraDirLightsInWorld = *(int*)0x94DB48;
int &skyBotRed = *(int*)0xA0D958;
int &skyBotGreen = *(int*)0x97F208;
int &skyBotBlue = *(int*)0x9B6DF4;

RwTexture *envTex, *envMaskTex;
RwCamera *envCam;
RwMatrix *envMatrix;

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
	if(rwD3D8RasterIsCubeRaster(tex->raster)){
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

void RwD3D8GetLight(RwInt32 n, void *light)
{
	if(n >= maxNumLights)
		memset(light, 0, 0x68);
	memcpy(light, (const void *)(lightsCache + 108 * n), 0x68);
}

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

float specPower = 0.25f;

void
reloadLights(void)
{
	char tmp[32];
	char modulePath[MAX_PATH];

	GetModuleFileName(dllModule, modulePath, MAX_PATH);
	size_t nLen = strlen(modulePath);
	modulePath[nLen-1] = L'i';
	modulePath[nLen-2] = L'n';
	modulePath[nLen-3] = L'i';

	GetPrivateProfileString("SkyGfx", "specPower", "25", tmp, sizeof(tmp), modulePath);
	specPower = atof(tmp);
}

static void *xboxVS = NULL, *xboxPS = NULL;
static void *pass1VS = NULL, *pass2VS = NULL;

void
uploadLightColor(RwUInt32 address, RpLight *light, float f)
{
	RwRGBAReal col = light->color;
	col.red *= f;
	col.green *= f;
	col.blue *= f;
	col.alpha = 1.0f;
	RwD3D9SetVertexPixelShaderConstant(address, (void*)&col, 1);
}

void
uploadConstants(float f)
{
	D3DMATRIX worldMat, viewMat, projMat;

	RwD3D8GetTransform(D3DTS_WORLD, &worldMat);
	RwD3D8GetTransform(D3DTS_VIEW, &viewMat);
	RwD3D8GetTransform(D3DTS_PROJECTION, &projMat);
	RwD3D9SetVertexPixelShaderConstant(LOC_world, (void*)&worldMat, 4);
//	RwMatrix out, world;
//	d3dtorwmat(&world, &worldMat);
//	world.flags = 0;
//	RwMatrixInvert(&out, &world);
//	rwtod3dmat(&worldMat, &out);
	RwD3D9SetVertexPixelShaderConstant(LOC_worldIT, (void*)&worldMat, 4);
	RwD3D9SetVertexPixelShaderConstant(LOC_view, (void*)&viewMat, 4);
	RwD3D9SetVertexPixelShaderConstant(LOC_proj, (void*)&projMat, 4);

	RwMatrix *camfrm = RwFrameGetLTM(RwCameraGetFrame((RwCamera*)((RwGlobals*)RwEngineInst)->curCamera));
	RwD3D9SetVertexPixelShaderConstant(LOC_eye, (void*)RwMatrixGetPos(camfrm), 1);

	RwRGBAReal col;
	uploadLightColor(LOC_ambient, pAmbient, f);
	RwD3D9SetVertexPixelShaderConstant(LOC_directDir, (void*)RwMatrixGetAt(RwFrameGetLTM(RpLightGetFrame(pDirect))), 1);
	uploadLightColor(LOC_directDiff, pDirect, f);
	col.red   = f*178/255.0f;
	col.green = f*178/255.0f;
	col.blue  = f*178/255.0f;
	col.alpha = 1.0f;
	RwD3D9SetVertexPixelShaderConstant(LOC_directSpec, (void*)&col, 1);
	int i = 0;
	for(i = 0 ; i < NumExtraDirLightsInWorld; i++){
		RwD3D9SetVertexPixelShaderConstant(LOC_lights+i*2, (void*)RwMatrixGetAt(RwFrameGetLTM(RpLightGetFrame(pExtraDirectionals[i]))), 1);
		uploadLightColor(LOC_lights+i*2+1, pExtraDirectionals[i], f);
	}
	static float zero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	for(; i < 4; i++){
		RwD3D9SetVertexPixelShaderConstant(LOC_lights+i*2, (void*)zero, 1);
		RwD3D9SetVertexPixelShaderConstant(LOC_lights+i*2+1, (void*)zero, 1);
	}
}

int
rpMatFXD3D8AtomicMatFXEnvRender_spec(RxD3D8InstanceData *inst, int flags, int sel, RwTexture *texture, RwTexture *envMap)
{
	MatFX *matfx = *RWPLUGINOFFSET(MatFX*, inst->material, MatFXMaterialDataOffset);
	MatFXEnv *env = &matfx->fx[sel];
	float factor = env->envCoeff*255.0f;
	RwUInt8 intens = factor;
	RpMaterial *m = inst->material;

	{
		static bool keystate = false;
		if(GetAsyncKeyState(VK_F10) & 0x8000){
			if(!keystate){
				keystate = true;
				reloadLights();
			}
		}else
			keystate = false;
	}

//	if(factor == 0.0f || !envMap){
//		if(sel == 0)
//			return rpMatFXD3D8AtomicMatFXDefaultRender(inst, flags, texture);
//		return 0;
//	}
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

	RwD3D8SetTexture(envTex, 1);
	RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MULTIPLYADD);
	RwD3D8SetTextureStageState(1, D3DTSS_COLORARG0, D3DTA_CURRENT);
	RwD3D8SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	RwD3D8SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_SPECULAR);
	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_CURRENT);

	RwD3D8SetVertexShader(NULL);
	RwD3D9SetFVF(inst->vertexShader);
	RwD3D9SetVertexShader(pass1VS);
	RwD3D9SetPixelShader(NULL);

	uploadConstants(1.0f);
	float reflProps[4];
	reflProps[0] = m->surfaceProps.specular;
	if(GetAsyncKeyState(VK_F5) & 0x8000)
		reflProps[0] = 0.0;
	reflProps[1] = specPower;
	reflProps[2] = 0.4f;	// fresnel
	RwD3D9SetVertexPixelShaderConstant(LOC_surfProps, (void*)&reflProps, 1);
	RwRGBAReal color;
	RwRGBARealFromRwRGBA(&color, &m->color);
	if(GetAsyncKeyState(VK_F4) & 0x8000)
		color.red = color.green = color.blue = 0.0f;
	RwD3D9SetVertexPixelShaderConstant(LOC_matCol, (void*)&color, 1);

	RwD3D8SetStreamSource(0, inst->vertexBuffer, inst->stride);
	RwD3D8SetIndices(inst->indexBuffer, inst->baseIndex);

	if(inst->indexBuffer)
		RwD3D8DrawIndexedPrimitive(inst->primType, 0, inst->numVertices, 0, inst->numIndices);
	else
		RwD3D8DrawPrimitive(inst->primType, inst->baseIndex, inst->numVertices);

	RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

	if(GetAsyncKeyState(VK_F6) & 0x8000)
		return 0;

	// pass two (specular)
	RwD3D8SetTexture(NULL, 1);
	if(!rwD3D8RenderStateIsVertexAlphaEnable())
		rwD3D8RenderStateVertexAlphaEnable(1);
	RwUInt32 dst;
	RwRenderStateGet(rwRENDERSTATEDESTBLEND, &dst);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
	RwD3D8SetTexture(NULL, 0);
	RwD3D9SetVertexShader(pass2VS);
	if(inst->indexBuffer)
		RwD3D8DrawIndexedPrimitive(inst->primType, 0, inst->numVertices, 0, inst->numIndices);
	else
		RwD3D8DrawPrimitive(inst->primType, inst->baseIndex, inst->numVertices);

	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)dst);
	RwD3D9SetVertexShader(NULL);
	RwD3D9SetPixelShader(NULL);
	return 0;
}

int
rpMatFXD3D8AtomicMatFXEnvRender_dual(RxD3D8InstanceData *inst, int flags, int sel, RwTexture *texture, RwTexture *envMap)
{
	MatFX *matfx = *RWPLUGINOFFSET(MatFX*, inst->material, MatFXMaterialDataOffset);
	MatFXEnv *env = &matfx->fx[sel];
	float factor;
	RwUInt8 intens; 
	factor = env->envCoeff*255.0f;
	intens = factor;

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
		return rpMatFXD3D8AtomicMatFXEnvRender_spec(inst, flags, sel, texture, envMap);

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

void __declspec(naked)
dualPassHook(void)
{
	_asm{
		lea     eax, [esi-10h]
		push	eax
		call	drawDualPass
		mov	dword ptr [esp], 0x678DA8
		retn
	}
}

RwIm2DVertex screenQuad[4];
RwImVertexIndex screenindices[6] = { 0, 1, 2, 0, 2, 3 };

void
generateEnvTexCoords(bool textureSpace)
{
	float minU, minV, maxU, maxV;
	if(textureSpace){
		minU = minV = 0.0f;
		maxU = maxV = 1.0f;
	}else{
		assert(0 && "not implemented");
	}
	screenQuad[0].u = minU;
	screenQuad[0].v = minV;
	screenQuad[1].u = minU;
	screenQuad[1].v = maxV;
	screenQuad[2].u = maxU;
	screenQuad[2].v = maxV;
	screenQuad[3].u = maxU;
	screenQuad[3].v = minV;
}

void
makeScreenQuad(void)
{
	int width = envTex->raster->width;
	int height = envTex->raster->height;
	screenQuad[0].x = 0.0f;
	screenQuad[0].y = 0.0f;
	screenQuad[0].z = RwIm2DGetNearScreenZ();
	screenQuad[0].rhw = 1.0f / envCam->nearPlane;
	screenQuad[0].emissiveColor = 0xFFFFFFFF;
	screenQuad[1].x = 0.0f;
	screenQuad[1].y = height;
	screenQuad[1].z = screenQuad[0].z;
	screenQuad[1].rhw = screenQuad[0].rhw;
	screenQuad[1].emissiveColor = 0xFFFFFFFF;
	screenQuad[2].x = width;
	screenQuad[2].y = height;
	screenQuad[2].z = screenQuad[0].z;
	screenQuad[2].rhw = screenQuad[0].rhw;
	screenQuad[2].emissiveColor = 0xFFFFFFFF;
	screenQuad[3].x = width;
	screenQuad[3].y = 0;
	screenQuad[3].z = screenQuad[0].z;
	screenQuad[3].rhw = screenQuad[0].rhw;
	screenQuad[3].emissiveColor = 0xFFFFFFFF;
	generateEnvTexCoords(1);
}

void
initXboxRefl(void)
{
//	HRSRC resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_XBOXVEHICLEVS), RT_RCDATA);
//	RwUInt32 *shader = (RwUInt32*)LoadResource(dllModule, resource);
//	RwD3D9CreateVertexShader(shader, &xboxVS);
//	assert(xboxVS);
//	FreeResource(shader);
//
//	resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_XBOXVEHICLEPS), RT_RCDATA);
//	shader = (RwUInt32*)LoadResource(dllModule, resource);
//	RwD3D9CreatePixelShader(shader, &xboxPS);
//	assert(xboxPS);
//	FreeResource(shader);

	HRSRC resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_VEHICLEONEVS), RT_RCDATA);
	RwUInt32 *shader = (RwUInt32*)LoadResource(dllModule, resource);
	RwD3D9CreateVertexShader(shader, &pass1VS);
	assert(pass1VS);
	FreeResource(shader);

	resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_VEHICLETWOVS), RT_RCDATA);
	shader = (RwUInt32*)LoadResource(dllModule, resource);
	RwD3D9CreateVertexShader(shader, &pass2VS);
	assert(pass2VS);
	FreeResource(shader);

	RwRaster *envFB = RwRasterCreate(128, 128, 0, rwRASTERTYPECAMERATEXTURE);
	RwRaster *envZB = RwRasterCreate(128, 128, 0, rwRASTERTYPEZBUFFER);
	envCam = RwCameraCreate();
	RwCameraSetRaster(envCam, envFB);
	RwCameraSetZRaster(envCam, envZB);
	RwCameraSetFrame(envCam, RwFrameCreate());
	RwCameraSetNearClipPlane(envCam, 0.1f);
	RwCameraSetFarClipPlane(envCam, 250.0f);
	RwV2d vw;
	vw.x = vw.y = 0.4f;
	RwCameraSetViewWindow(envCam, &vw);

	envTex = RwTextureCreate(envFB);
	RwTextureSetFilterMode(envTex, rwFILTERLINEAR);

	RwImage *envMaskI = RtBMPImageRead("neo\\CarReflectionMask.bmp");
	assert(envMaskI);
	RwInt32 width, height, depth, format;
	RwImageFindRasterFormat(envMaskI, 4, &width, &height, &depth, &format);
	RwRaster *envMask = RwRasterCreate(width, height, depth, format);
	RwRasterSetFromImage(envMask, envMaskI);
	envMaskTex = RwTextureCreate(envMask);
	RwImageDestroy(envMaskI);

	envMatrix = RwMatrixCreate();
	envMatrix->right.x = -1.0f;
	envMatrix->right.y = 0.0f;
	envMatrix->right.z = 0.0f;
	envMatrix->up.x = 0.0f;
	envMatrix->up.y = -1.0f;
	envMatrix->up.z = 0.0f;
	envMatrix->at.x = 0.0f;
	envMatrix->at.y = 0.0f;
	envMatrix->at.z = 1.0f;

	makeScreenQuad();
}

WRAPPER void RenderScene(void) { EAXJMP(0x4A6570); }

WRAPPER void CRenderer__RenderEverythingBarRoads(void) { EAXJMP(0x4C9F40); }
WRAPPER void CRenderer__RenderFadingInEntities(void) { EAXJMP(0x4CA140); }

RwRGBA *skycolor = (RwRGBA*)0x983B80;

void
RenderReflectionScene(void)
{
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, 0);
	CRenderer__RenderEverythingBarRoads();
	CRenderer__RenderFadingInEntities();
}

void
RenderScene_hook(void)
{
	RenderScene();

	RwCamera *cam = (RwCamera*)((RwGlobals*)RwEngineInst)->curCamera;
	if(envTex == NULL)
		initXboxRefl();

	RwCameraEndUpdate(cam);

	RwV2d oldvw, vw = { 2.0f, 2.0f };
	oldvw = envCam->viewWindow;
	RwCameraSetViewWindow(envCam, &vw);
	RwMatrix *cammatrix = RwFrameGetMatrix(RwCameraGetFrame(cam));
	envMatrix->pos = cammatrix->pos;
	RwMatrixUpdate(envMatrix);
	RwFrameTransform(RwCameraGetFrame(envCam), envMatrix, rwCOMBINEREPLACE);
	RwRGBA color = { skyBotRed, skyBotGreen, skyBotBlue, 255 };
	RwCameraClear(envCam, &color, rwCAMERACLEARIMAGE | rwCAMERACLEARZ);

	RwCameraBeginUpdate(envCam);
	RenderReflectionScene();
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDZERO);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDSRCCOLOR);
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, envMaskTex->raster);
	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, screenQuad, 4, screenindices, 6);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, 0);
	RwCameraEndUpdate(envCam);
	RwCameraSetViewWindow(envCam, &oldvw);

	RwCameraBeginUpdate(cam);
//	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, envTex->raster);
//	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, screenQuad, 4, screenindices, 6);
}

WRAPPER void D3D8DeviceSystemStart(void) { EAXJMP(0x65BFC0); }

void
D3D8DeviceSystemStart_hook(void)
{
	D3D8DeviceSystemStart();
	iCanHasD3D9 = initD3D9((IDirect3DDevice8*)RwD3DDevice);
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
	if(GetPrivateProfileInt("SkyGfx", "IIIReflections", FALSE, modulePath)){
		MemoryVP::InjectHook(0x57A8BA, createIIIEnvFrame);
		MemoryVP::InjectHook(0x57A8C7, 0x57A8F4, PATCH_JUMP);
	}
	MemoryVP::Patch<DWORD>(0x699D44, 0x3f800000);
	MemoryVP::InjectHook(0x6765C8, rpMatFXD3D8AtomicMatFXEnvRender_dual);
	MemoryVP::InjectHook(0x6755D0, ApplyEnvMapTextureMatrix, PATCH_JUMP);


	if(GetPrivateProfileInt("SkyGfx", "dualPass", TRUE, modulePath))
		MemoryVP::InjectHook(0x678D69, dualPassHook, PATCH_JUMP);

	if(GetPrivateProfileInt("SkyGfx", "disableBackfaceCulling", FALSE, modulePath)){
		// hope I didn't miss anything
		MemoryVP::Patch<BYTE>(0x4C9E5F, 1);	// in CRenderer::RenderOneNonRoad()
		MemoryVP::Patch<BYTE>(0x4C9F08, 1);	// in CRenderer::RenderBoats()
		MemoryVP::Patch<BYTE>(0x4C9F5D, 1);	// in CRenderer::RenderEverythingBarRoads()
		MemoryVP::Patch<BYTE>(0x4CA157, 1);	// in CRenderer::RenderFadingInEntities()
		MemoryVP::Patch<BYTE>(0x4CA199, 1);	// in CReenvnderer::RenderRoads()
	}

	MemoryVP::InjectHook(0x4A604A, RenderScene_hook);
	MemoryVP::InjectHook(0x65BB1F, D3D8DeviceSystemStart_hook);

	MemoryVP::Patch<BYTE>(0x5D4EEE, rwBLENDINVSRCALPHA);

	MemoryVP::InjectHook(0x66D65E, rpSkinD3D8CreatePlainPipe_hook);

//	MemoryVP::Nop(0x600F1C, 2);
//	MemoryVP::Nop(0x600F56, 2);
//
//	MemoryVP::Nop(0x6018DA, 6);
//	MemoryVP::Nop(0x601910, 2);
//	char *fmtstr = "%lu Y %lu Y %lu";
//	MemoryVP::Patch(0x601970, fmtstr);

	reloadLights();

	MemoryVP::InjectHook(0x401000, printf, PATCH_JUMP);
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

		if (*(DWORD*)0x667BF5 == 0xB85548EC)	// 1.0
			patch10();
		else
			return FALSE;
	}

	return TRUE;
}
