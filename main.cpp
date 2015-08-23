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

IUnknown *&RwD3DDevice = *(IUnknown**)0x7897A8;

int iCanHasD3D9 = 0;
void **&RwEngineInst = *(void***)0x7870C0;
RpLight *&pAmbient = *(RpLight**)0x974B44;
RpLight *&pDirect = *(RpLight**)0x94DD40;
RpLight **pExtraDirectionals = (RpLight**)0x69A140;
int &NumExtraDirLightsInWorld = *(int*)0x94DB48;



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

RwBool
RwD3D8SetLight_Specular(RwInt32 index, D3DLIGHT8 *light)
{
	light->Specular = light->Diffuse;
	if(index == 0){
//		light->Specular.r = light->Specular.g = light->Specular.b = 0.5f;
//		light->Specular.a = 1.0f;
		float scale = 1/255.0f;
		light->Specular.r = 178*scale;
		light->Specular.g = 178*scale;
		light->Specular.b = 178*scale;
		light->Specular.a = 1.0f;
	}
	return RwD3D8SetLight(index, light);
}

static void *xboxVS = NULL, *xboxPS = NULL;

void
uploadConstants(void)
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
	RwD3D9SetVertexPixelShaderConstant(LOC_ambient, (void*)&pAmbient->color, 1);
	RwD3D9SetVertexPixelShaderConstant(LOC_directDir, (void*)RwMatrixGetAt(RwFrameGetLTM(RpLightGetFrame(pDirect))), 1);
	col = pDirect->color;
	col.alpha = 1.0f;
	RwD3D9SetVertexPixelShaderConstant(LOC_directDiff, (void*)&col, 1);
	col.red   = 178/255.0f;
	col.green = 178/255.0f;
	col.blue  = 178/255.0f;
	col.alpha = 0.4f;
	RwD3D9SetVertexPixelShaderConstant(LOC_directSpec, (void*)&col, 1);
	int i = 0;
	for(i = 0 ; i < NumExtraDirLightsInWorld; i++){
		RwD3D9SetVertexPixelShaderConstant(LOC_lights+i*2, (void*)RwMatrixGetAt(RwFrameGetLTM(RpLightGetFrame(pExtraDirectionals[i]))), 1);
		col = pExtraDirectionals[i]->color;
		col.alpha = 0.4f;
		RwD3D9SetVertexPixelShaderConstant(LOC_lights+i*2+1, (void*)&col, 1);
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

	if(xboxVS == NULL){
		HRSRC resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_XBOXVEHICLEVS), RT_RCDATA);
		RwUInt32 *shader = (RwUInt32*)LoadResource(dllModule, resource);
		RwD3D9CreateVertexShader(shader, &xboxVS);
		assert(xboxVS);
		FreeResource(shader);

		resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_XBOXVEHICLEPS), RT_RCDATA);
		shader = (RwUInt32*)LoadResource(dllModule, resource);
		RwD3D9CreatePixelShader(shader, &xboxPS);
		assert(xboxPS);
		FreeResource(shader);
	}


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

	float mult = 1.0f/255.0f;
//	gMaterial.Specular.r = 1.0f;//m->color.red * mult;
//	gMaterial.Specular.g = 1.0f;//m->color.green * mult;
//	gMaterial.Specular.b = 1.0f;//m->color.blue * mult;
//	gMaterial.Specular.a = 1.0f;//m->color.alpha * mult;
//	gMaterial.Power = specPower;
//	gLastMaterial.Diffuse.r++;	// invalidate cache
//	RwSurfaceProperties sp = m->surfaceProps;
//	RwD3D8SetSurfaceProperties(&m->color, &m->surfaceProps, flags & 0x40);

//	RwD3D8SetRenderState(D3DRS_SPECULARENABLE, 1);
//	RwD3D8SetRenderState(D3DRS_LOCALVIEWER, 1);
//	RwD3D8SetRenderState(D3DRS_SPECULARMATERIALSOURCE, 0);

//	if(!(GetAsyncKeyState(VK_F6) & 0x8000)){
		RwD3D8SetVertexShader(NULL);
		RwD3D9SetFVF(inst->vertexShader);
		RwD3D9SetVertexShader(xboxVS);
		RwD3D9SetPixelShader(xboxPS);
		uploadConstants();
		RwSurfaceProperties sp = m->surfaceProps;
		sp.specular = specPower;
		RwD3D9SetVertexPixelShaderConstant(LOC_surfProps, (void*)&sp, 1);
		RwRGBAReal color;
		RwRGBARealFromRwRGBA(&color, &m->color);
		RwD3D9SetVertexPixelShaderConstant(LOC_matCol, (void*)&color, 1);
//	}else{
//		RwD3D8SetVertexShader(inst->vertexShader);
//	}

	RwD3D8SetStreamSource(0, inst->vertexBuffer, inst->stride);
	RwD3D8SetIndices(inst->indexBuffer, inst->baseIndex);

	if(inst->indexBuffer)
		RwD3D8DrawIndexedPrimitive(inst->primType, 0, inst->numVertices, 0, inst->numIndices);
	else
		RwD3D8DrawPrimitive(inst->primType, inst->baseIndex, inst->numVertices);
//	RwD3D8SetRenderState(D3DRS_SPECULARENABLE, 0);
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

	MemoryVP::InjectHook(0x65BB1F, D3D8DeviceSystemStart_hook);

	MemoryVP::InjectHook(0x679C48, RwD3D8SetLight_Specular);
	MemoryVP::InjectHook(0x679F0B, RwD3D8SetLight_Specular);

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
