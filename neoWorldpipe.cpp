#include "skygfx.h"
#include "d3d8.h"
#include "d3d8types.h"

extern IDirect3DDevice8 *&RwD3DDevice;
extern D3DMATERIAL8 &lastmaterial;

// ADDRESS
static uint32_t CSimpleModelInfo__SetAtomic_A; // for reference: AddressByVersion<uint32_t>(0x517950, 0x517B60, 0x517AF0, 0x56F790, 0, 0);
WRAPPER void CSimpleModelInfo::SetAtomic(int, RpAtomic*) { VARJMP(CSimpleModelInfo__SetAtomic_A); }

bool &CWeather__LightningFlash = *(bool*)AddressByVersion<uint32_t>(0x95CDA3, 0, 0, 0xA10B67, 0, 0);

/*
 * The following code is fairly close to the Xbox code.
 * In particular quite a bit of actually unused code is implemented.
 * If you don't like it, complain to R* Vienna.
 * With usePixelShader = false and modulate2x = true the lightmaps are
 * rendered without using a pixel shader.
 */

class WorldPipe : CustomPipe
{
	void CreateShaders(void);
	void LoadTweakingTable(void);
public:
	bool isActive;
	bool modulate2x;
	int setMaterial;
	int setMaterialColor;
	int modulateMaterial;
	int lightingEnabled;
//	int lastColor;
//	float lastAmbient;
//	float lastDiffuse;
	bool usePixelShader;
	void *pixelShader;
	InterpolatedFloat lightmapBlend;

	WorldPipe(void);
	void Attach(RpAtomic *atomic);
	static WorldPipe *Get(void);
	void Init(void);

	void RenderCallback(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags);
	void RenderObjectSetup(RwUInt32 flags);
	void RenderMeshSetUp(RxD3D8InstanceData *inst);
	void RenderMeshCombinerSetUp(RxD3D8InstanceData *inst, RwUInt32 flags);
	void RenderMeshCombinerTearDown(void);
	void RenderMesh(RxD3D8InstanceData *inst, RwUInt32 flags);
};

void
CSimpleModelInfo::SetAtomic_hook(int n, RpAtomic *atomic)
{
	this->SetAtomic(n, atomic);
	WorldPipe::Get()->Attach(atomic);
}

// ADDRESS
void
neoWorldPipeInit(void)
{
	WorldPipe::Get()->Init();
	WorldPipe::Get()->modulate2x = true;
	if(gtaversion == III_10){
		InterceptCall(&CSimpleModelInfo__SetAtomic_A, &CSimpleModelInfo::SetAtomic_hook, 0x4768F1);
		InjectHook(0x476707, &CSimpleModelInfo::SetAtomic_hook);
	}else if(gtaversion == VC_10){
		// virtual in VC because of added CWeaponModelInfo
		InterceptVmethod(&CSimpleModelInfo__SetAtomic_A, &CSimpleModelInfo::SetAtomic_hook, 0x697FF8);
		Patch(0x698028, &CSimpleModelInfo::SetAtomic_hook);
	}
}

extern "C" {
__declspec(dllexport) void
AttachWorldPipeToRwObject(RwObject *obj)
{
	if(RwObjectGetType(obj) == rpATOMIC)
		WorldPipe::Get()->Attach((RpAtomic*)obj);
	else if(RwObjectGetType(obj) == rpCLUMP)
		RpClumpForAllAtomics((RpClump*)obj, CustomPipe::setatomicCB, WorldPipe::Get());
}
}

WorldPipe*
WorldPipe::Get(void)
{
	static WorldPipe worldpipe;
	return &worldpipe;
}

WorldPipe::WorldPipe(void)
 : lightmapBlend(1.0f)
{
	CreateRwPipeline();
	modulate2x = false;
	isActive = true;
	usePixelShader = RwD3D9Supported();
}

void
WorldPipe::Attach(RpAtomic *atomic)
{
	if(isActive && *RWPLUGINOFFSET(int, atomic, MatFXAtomicDataOffset))
		CustomPipe::Attach(atomic);
}

static void rendercbwrapper(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
	WorldPipe::Get()->RenderCallback(repEntry, object, type, flags);
}

void
WorldPipe::Init(void)
{
	SetRenderCallback(rendercbwrapper);
	LoadTweakingTable();
	CreateShaders();
}

void
WorldPipe::CreateShaders(void)
{
	if(RwD3D9Supported()){
		HRSRC resource;
		RwUInt32 *shader;
		resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_WORLDPS), RT_RCDATA);
		shader = (RwUInt32*)LoadResource(dllModule, resource);
		RwD3D9CreatePixelShader(shader, &pixelShader);
		assert(WorldPipe::pixelShader);
		FreeResource(shader);
	}
}

void
WorldPipe::LoadTweakingTable(void)
{

	char *path;
	FILE *dat;
	path = getpath("neo\\worldTweakingTable.dat");
	if(path == NULL){
		MessageBox(NULL, "Couldn't load 'neo\\worldTweakingTable.dat'", "Error", MB_ICONERROR | MB_OK);
		exit(0);
	}
	dat = fopen(path, "r");
	neoReadWeatherTimeBlock(dat, &lightmapBlend);
	fclose(dat);
}

//
// Rendering
//

void
WorldPipe::RenderObjectSetup(RwUInt32 flags)
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
	if(lightingEnabled)
		if(flags & rpGEOMETRYPRELIT){
			RwD3D8SetRenderState(D3DRS_COLORVERTEX, 1);
			RwD3D8SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, 1);
		}else{
			RwD3D8SetRenderState(D3DRS_COLORVERTEX, 0);
			RwD3D8SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, 0);
		}
}

// Mostly unused because we have pixel shader
void
WorldPipe::RenderMeshSetUp(RxD3D8InstanceData *inst)
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
	if(lightingEnabled)
		RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, inst->vertexAlpha);
}

// Not really used as we should have a pixel shader
void
WorldPipe::RenderMeshCombinerSetUp(RxD3D8InstanceData *inst, RwUInt32 flags)
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
WorldPipe::RenderMeshCombinerTearDown(void)
{
	RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	RwD3D8SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D8SetTextureStageState(2, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
}

void
WorldPipe::RenderMesh(RxD3D8InstanceData *inst, RwUInt32 flags)
{
							{
								static bool keystate = false;
								if(GetAsyncKeyState(xboxworldpipekey) & 0x8000){
									if(!keystate){
										keystate = true;
										xboxworldpipe = (xboxworldpipe+1)%2;
									}
								}else
									keystate = false;
							}

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
		if(!xboxworldpipe)
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
WorldPipe::RenderCallback(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
	int objinit = 0;
	MatFX *matfx;
	rxD3D8SetAmbientLight();

	RxD3D8ResEntryHeader *header = (RxD3D8ResEntryHeader*)&repEntry[1];
	RxD3D8InstanceData *inst = (RxD3D8InstanceData*)&header[1];

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
