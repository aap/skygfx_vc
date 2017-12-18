#include "skygfx.h"
#include "d3d8.h"
#include "d3d8types.h"

extern IDirect3DDevice8 *&RwD3DDevice;
extern D3DMATERIAL8 &lastmaterial;

// ADDRESS
bool &CWeather__LightningFlash = *(bool*)AddressByVersion<uint32_t>(0x95CDA3, 0, 0, 0xA10B67, 0, 0);

/*
 * The following code is fairly close to the Xbox code.
 * In particular quite a bit of actually unused code is implemented.
 * If you don't like it, complain to R* Vienna.
 * With usePixelShader = false and modulate2x = true the lightmaps are
 * rendered without using a pixel shader.
 */

NeoWorldPipe*
NeoWorldPipe::Get(void)
{
	static NeoWorldPipe worldpipe;
	return &worldpipe;
}

NeoWorldPipe::NeoWorldPipe(void)
 : lightmapBlend(1.0f)
{
	CreateRwPipeline();
	modulate2x = false;
//	isActive = true;
	usePixelShader = RwD3D9Supported();
}

void
NeoWorldPipe::Attach(RpAtomic *atomic)
{
// TEMP: unconditionally so leeds pipe will work
//	if(/*isActive &&*/ *RWPLUGINOFFSET(int, atomic, MatFXAtomicDataOffset))
		CustomPipe::Attach(atomic);
}

static void rendercbwrapper(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
	NeoWorldPipe::Get()->RenderCallback(repEntry, object, type, flags);
}

void
NeoWorldPipe::Init(void)
{
	SetRenderCallback(rendercbwrapper);
	LoadTweakingTable();

	if(d3d9)
		CreateShaders();
}

void
NeoWorldPipe::CreateShaders(void)
{
	HRSRC resource;
	RwUInt32 *shader;
	if(isIII())
		resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_WORLDPS), RT_RCDATA);
	else
		resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_VCWORLDPS), RT_RCDATA);
	shader = (RwUInt32*)LoadResource(dllModule, resource);
	RwD3D9CreatePixelShader(shader, &pixelShader);
	assert(NeoWorldPipe::pixelShader);
	FreeResource(shader);
}

void
NeoWorldPipe::LoadTweakingTable(void)
{

	char *path;
	FILE *dat;
	path = getpath("neo\\worldTweakingTable.dat");
	if(path == NULL){
		errorMessage("Couldn't load 'neo\\worldTweakingTable.dat'");
		canUse = false;
		return;
	}
	dat = fopen(path, "r");
	neoReadWeatherTimeBlock(dat, &lightmapBlend);
	fclose(dat);
}

//
// Rendering
//

void
NeoWorldPipe::RenderObjectSetup(RwUInt32 flags)
{
	// NB: the flags set here are part of the class, *not* the global ones with the same names
	RwD3D8GetRenderState(D3DRS_LIGHTING, &lightingEnabled);
	if(lightingEnabled && flags & rpGEOMETRYLIGHT){
		// We have to set the d3d material when lighting is on.
		setMaterial = 1;
		if(flags & rpGEOMETRYMODULATEMATERIALCOLOR){
			if(flags & rpGEOMETRYPRELIT){
				// Have to do modulation at texture stage
				// when we have prelighting and material color.
				setMaterialColor = 0;
				modulateMaterial = 1;
			}else{
				// Otherwise do it at lighting stage.
				setMaterialColor = 1;
				modulateMaterial = 0;
			}
		}else{
			// no material color at all
			setMaterialColor = 0;
			modulateMaterial = 0;
		}
	}else{
		// Can only regard material color at texture stage without lighting.
		setMaterial = 0;
		setMaterialColor = 0;
		modulateMaterial = 1;
	}
	if(lightingEnabled){
		if(flags & rpGEOMETRYPRELIT){
			RwD3D8SetRenderState(D3DRS_COLORVERTEX, 1);
			RwD3D8SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_COLOR1);
		}else{
			RwD3D8SetRenderState(D3DRS_COLORVERTEX, 0);
			RwD3D8SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL);
		}
	}
	RwD3D8SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL);	// not found in xbox code for some reason
}

// Mostly unused because we have pixel shader
void
NeoWorldPipe::RenderMeshSetUp(RxD3D8InstanceData *inst)
{
	static D3DMATERIAL8 material = { { 0.0f, 0.0f, 0.0f, 1.0f },
	                                 { 0.0f, 0.0f, 0.0f, 1.0f },
	                                 { 0.0f, 0.0f, 0.0f, 1.0f },
	                                 { 0.0f, 0.0f, 0.0f, 1.0f },
	                                 0.0f };
	float a, d;
	RpMaterial *m;
	// NB: the flags set here are part of the class, *not* the global ones with the same names
	if(setMaterial){
		m = inst->material;
		if(setMaterialColor){
			a = m->surfaceProps.ambient / 255.0f;
			d = m->surfaceProps.diffuse / 255.0f;
			material.Diffuse.r = m->color.red   * d;
			material.Diffuse.g = m->color.green * d;
			material.Diffuse.b = m->color.blue  * d;
			material.Diffuse.a = m->color.alpha * d;
			material.Ambient.r = m->color.red   * a;
			material.Ambient.g = m->color.green * a;
			material.Ambient.b = m->color.blue  * a;
			material.Ambient.a = m->color.alpha * a;
		}else{
			d = m->surfaceProps.diffuse;
			material.Diffuse.r = d;
			material.Diffuse.g = d;
			material.Diffuse.b = d;
			material.Diffuse.a = d;
			a = m->surfaceProps.ambient;
			material.Ambient.r = a;
			material.Ambient.g = a;
			material.Ambient.b = a;
			material.Ambient.a = a;
		}
		if(memcmp(&lastmaterial, &material, sizeof(D3DMATERIAL8)) != 0){
			lastmaterial = material;
			RwD3DDevice->SetMaterial(&material);
		}
	}
	// normally done in rxD3D8DefaultRenderFFPObjectSetUp, but we don't know vertexAlpha for the whole geomtetry
	if(lightingEnabled)
		RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, inst->vertexAlpha ? D3DMCS_COLOR1 : D3DMCS_MATERIAL);
}

// Not really used as we should have a pixel shader
void
NeoWorldPipe::RenderMeshCombinerSetUp(RxD3D8InstanceData *inst, RwUInt32 flags)
{
	RwD3D8SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	RwD3D8SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
	RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TEXTURE);
	RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
	RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TEXTURE);

	RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, modulate2x ? D3DTOP_MODULATE2X : D3DTOP_MODULATE);
	RwD3D8SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	RwD3D8SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);
	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);

	RwRGBA *c = &inst->material->color;
	RwUInt32 tf = D3DCOLOR_ARGB(c->alpha, 0xFF, 0xFF, 0xFF);
	if(tf == 0xFFFFFFFF){
		RwD3D8SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_DISABLE);
		RwD3D8SetTextureStageState(2, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	}else{
		RwD3D8SetRenderState(D3DRS_TEXTUREFACTOR, tf);
		RwD3D8SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
		RwD3D8SetTextureStageState(2, D3DTSS_COLORARG1, D3DTA_CURRENT);
		RwD3D8SetTextureStageState(2, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
		RwD3D8SetTextureStageState(2, D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
		RwD3D8SetTextureStageState(2, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
	}
}

void
NeoWorldPipe::RenderMeshCombinerTearDown(void)
{
	RwD3D8SetPixelShader(NULL);                     // 8!
	RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	RwD3D8SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D8SetTextureStageState(2, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
}

void
NeoWorldPipe::RenderMesh(RxD3D8InstanceData *inst, RwUInt32 flags)
{
	RwD3D8SetStreamSource(0, inst->vertexBuffer, inst->stride);
	if(flags & (rpGEOMETRYTEXTURED|rpGEOMETRYTEXTURED2))
		RwD3D8SetTexture(inst->material->texture, 0);
	else
		RwD3D8SetTexture(NULL, 0);
	MatFX *matfx = *RWPLUGINOFFSET(MatFX*, inst->material, MatFXMaterialDataOffset);
	RwD3D8SetTexture(((MatFXDual*)matfx->fx)[0].tex, 1);
	if(!usePixelShader)
		RenderMeshCombinerSetUp(inst, flags);
	RwD3D8SetVertexShader(inst->vertexShader);

	if(usePixelShader){
		float lm[4];
		float f = 1.0f;
		if(!CWeather__LightningFlash)
			f = lightmapBlend.Get();
		if(config.worldPipeSwitch != WORLD_NEO)
			f = 0.0f;
		lm[0] = lm[1] = lm[2] = f;
		lm[3] = inst->material->color.alpha/255.0f;
		RwD3D9SetPixelShader(pixelShader);              // 9!
		RwD3D9SetPixelShaderConstant(0, lm, 1);
	}else{
		RwD3D8SetPixelShader(NULL);                     // 8!
	}
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,
	                 (void*)(inst->vertexAlpha || inst->material->color.alpha != 0xFF));
	drawDualPass(inst);
	RenderMeshCombinerTearDown();
}

void
NeoWorldPipe::RenderCallback(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
	int objinit = 0;
	MatFX *matfx;

	if(!canUse)
		return;

	rxD3D8SetAmbientLight();

	RxD3D8ResEntryHeader *header = (RxD3D8ResEntryHeader*)&repEntry[1];
	RxD3D8InstanceData *inst = (RxD3D8InstanceData*)&header[1];

	_rwD3D8EnableClippingIfNeeded(object, type);
	RenderObjectSetup(flags);
	for(int i = 0; i < header->numMeshes; i++){
		matfx = *RWPLUGINOFFSET(MatFX*, inst->material, MatFXMaterialDataOffset);
		if(matfx ? ((MatFXDual*)matfx->fx)[0].tex : NULL){
			if(matfx->effects == rpMATFXEFFECTDUAL){
				RenderMeshSetUp(inst);
				RenderMesh(inst, flags);
			}
		}else{
			if(objinit == 0){
				rxD3D8DefaultRenderFFPObjectSetUp(flags);
				objinit = 1;
			}
			rxD3D8DefaultRenderFFPMeshSetUp(inst);
			rxD3D8DefaultRenderFFPMesh(inst, flags);
		}
		inst++;
	}
}
