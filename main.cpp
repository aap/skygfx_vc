#include "skygfx_vc.h"

HMODULE dllModule;

WRAPPER int rpMatFXD3D8AtomicMatFXEnvRender(RxD3D8InstanceData*, int, int, RwTexture*, RwTexture*) { EAXJMP(0x674EE0); }
WRAPPER int rpMatFXD3D8AtomicMatFXDefaultRender(RxD3D8InstanceData*, int, RwTexture*) { EAXJMP(0x674380); }
WRAPPER RwBool rwD3D8RenderStateIsVertexAlphaEnable(void) { EAXJMP(0x659F60); };
WRAPPER void rwD3D8RenderStateVertexAlphaEnable(RwBool x) { EAXJMP(0x659CF0); };
WRAPPER void ApplyEnvMapTextureMatrix(RwTexture*, int, RwFrame*) { EAXJMP(0x6755D0); }
int &MatFXMaterialDataOffset = *(int*)0x7876CC;

D3DMATERIAL8 &gMaterial = *(D3DMATERIAL8*)0x6DDEB8;
D3DMATERIAL8 &gLastMaterial = *(D3DMATERIAL8*)0x789760;
int &maxNumLights = *(int*)0x789BF4;
int &lightsCache = *(int*)0x789BF8;

void RwD3D8GetLight(RwInt32 n, void *light)
{
	if(n >= maxNumLights)
		memset(light, 0, 0x68);
	memcpy(light, (const void *)(lightsCache + 108 * n), 0x68);
}

int switchkey;

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

	ApplyEnvMapTextureMatrix(envMap, 1, env->envFrame);
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

int matfxstyle = 0;

int
rpMatFXD3D8AtomicMatFXEnvRender_spec(RxD3D8InstanceData *inst, int flags, int sel, RwTexture *texture, RwTexture *envMap)
{
	MatFX *matfx = *RWPLUGINOFFSET(MatFX*, inst->material, MatFXMaterialDataOffset);
	MatFXEnv *env = &matfx->fx[sel];
	float factor = env->envCoeff*255.0f;
	RwUInt8 intens = factor;
	RpMaterial *m = inst->material;

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

//	float mult = m->surfaceProps.specular/255.0f;
	float mult = 1.0f/255.0f;
	gMaterial.Specular.r = m->color.red * mult;
	gMaterial.Specular.g = m->color.green * mult;
	gMaterial.Specular.b = m->color.blue * mult;
	gMaterial.Specular.a = m->color.alpha * mult;
	gMaterial.Power = 20.0f;
	gLastMaterial.Diffuse.r++;	// invalidate cache
	RwD3D8SetSurfaceProperties(&m->color, &m->surfaceProps, flags & 0x40);
	D3DLIGHT8 light;
	RwD3D8GetLight(0, &light);
	// taken from SA
//	light.Diffuse.r = light.Diffuse.g = light.Diffuse.b = 0.25f;
	light.Diffuse.r = light.Diffuse.g = light.Diffuse.b = 0.0f;
	light.Diffuse.a = 1.0f;
//	light.Ambient.r = light.Ambient.g = light.Ambient.b = 0.75f;
	light.Ambient.r = light.Ambient.g = light.Ambient.b = 0.0f;
	light.Ambient.a = 1.0f;
//	light.Specular.r = light.Specular.g = light.Specular.b = 0.65f;
	light.Specular.r = light.Specular.g = light.Specular.b = 1.0f;
	light.Specular.a = 1.0f;
	light.Range = 1000.0f;
	light.Falloff = 0.0f;
	light.Attenuation0 = 1.0f;
	light.Attenuation1 = 0.0f;
	light.Attenuation2 = 0.0f;
	RwD3D8SetLight(maxNumLights-1, &light);
	RwD3D8EnableLight(maxNumLights-1, 1);

	RwD3D8SetRenderState(D3DRS_SPECULARENABLE, 1);
	RwD3D8SetRenderState(D3DRS_LOCALVIEWER, 0);
	RwD3D8SetRenderState(D3DRS_SPECULARMATERIALSOURCE, 0);

	RwD3D8SetVertexShader(inst->vertexShader);
	RwD3D8SetStreamSource(0, inst->vertexBuffer, inst->stride);
	RwD3D8SetIndices(inst->indexBuffer, inst->baseIndex);
	if(inst->indexBuffer)
		RwD3D8DrawIndexedPrimitive(inst->primType, 0, inst->numVertices, 0, inst->numIndices);
	else
		RwD3D8DrawPrimitive(inst->primType, inst->baseIndex, inst->numVertices);
	RwD3D8SetRenderState(D3DRS_SPECULARENABLE, 0);
	RwD3D8EnableLight(maxNumLights-1, 0);
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
		if(GetAsyncKeyState(switchkey) & 0x8000){
			if(!keystate){
				keystate = true;
				matfxstyle = (matfxstyle+1)%2;
			}
		}else
			keystate = false;
	}
	if(matfxstyle == 1){
		float saved = env->envCoeff;
		env->envCoeff *= 0.25f;
		int ret = rpMatFXD3D8AtomicMatFXEnvRender(inst, flags, sel, texture, envMap);
		env->envCoeff = saved;
		return ret;
	}
	if(matfxstyle == 2)
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
	f->modelling.at.z = 1.0f;
	f->modelling.up.y = f->modelling.at.z;
	f->modelling.right.x = f->modelling.up.y;
	f->modelling.up.x = 0;
	f->modelling.right.z = f->modelling.up.x;
	f->modelling.right.y = f->modelling.right.z;
	f->modelling.at.y = 0;
	f->modelling.at.x = f->modelling.at.y;
	f->modelling.up.z = f->modelling.at.x;
	f->modelling.pos.z = 0;
	f->modelling.pos.y = f->modelling.pos.z;
	f->modelling.pos.x = f->modelling.pos.y;
	f->modelling.flags |= 0x20003u;
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

/*
struct AnimAssocDefinition
{
	char *name;
	char *blockName;
	int modelIndex;
	int animCount;
	char **animNames;
	void *animInfoList;
};
AnimAssocDefinition *animAssocDefinitions = (AnimAssocDefinition*)0x6857B0;
*/

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

	GetPrivateProfileString("SkyGfx", "switchKey", "0x76", tmp, sizeof(tmp), modulePath);
	switchkey = readhex(tmp);
	matfxstyle = GetPrivateProfileInt("SkyGfx", "reflSwitch", 0, modulePath) % 2;
	if(GetPrivateProfileInt("SkyGfx", "IIIReflections", FALSE, modulePath)){
		MemoryVP::InjectHook(0x57A8BA, createIIIEnvFrame);
		MemoryVP::InjectHook(0x57A8C7, 0x57A8F4, PATCH_JUMP);
	}
	MemoryVP::Patch<DWORD>(0x699D44, 0x3f800000);
	MemoryVP::InjectHook(0x6765C8, rpMatFXD3D8AtomicMatFXEnvRender_dual);

	if(GetPrivateProfileInt("SkyGfx", "dualPass", TRUE, modulePath))
		MemoryVP::InjectHook(0x678D69, dualPassHook, PATCH_JUMP);

	MemoryVP::Patch<BYTE>(0x5D4EEE, rwBLENDINVSRCALPHA);

//	MemoryVP::InjectHook(0x401000, printf, PATCH_JUMP);

/*
	FILE *f = fopen("animgrp.txt", "w");
	for(int i = 0; i < 61; i++){
		AnimAssocDefinition *def = &animAssocDefinitions[i];
		fprintf(f, "%s, %s, %d, %d\n", def->name, def->blockName, def->modelIndex, def->animCount);
		char **names = def->animNames;
		int *p = (int*)def->animInfoList;
		for(int j = 0; j < def->animCount; j++)
			fprintf(f, "\t%d, %s, %X\n", p[j*2+0], names[j], p[j*2+1]);
		fprintf(f, "\n");
	}
	fclose(f);
*/
}

BOOL WINAPI
DllMain(HINSTANCE hInst, DWORD reason, LPVOID)
{
	if(reason == DLL_PROCESS_ATTACH){
		dllModule = hInst;

/*
		AllocConsole();
		freopen("CONIN$", "r", stdin);
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);
*/

		if (*(DWORD*)0x667BF5 == 0xB85548EC)	// 1.0
			patch10();
		else
			return FALSE;
	}

	return TRUE;
}
