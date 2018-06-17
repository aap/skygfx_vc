#include "skygfx.h"
#include "d3d8.h"
#include "d3d8types.h"

// World pipe

extern D3DMATERIAL8 d3dmaterial;
extern RwReal lastDiffuse;
extern RwReal lastAmbient;
extern RwUInt32 lastWasTextured;
extern RwRGBA lastColor;
extern float lastAmbRed;
extern float lastAmbGreen;
extern float lastAmbBlue;

extern IDirect3DDevice8 *&RwD3DDevice;
extern D3DMATERIAL8 &lastmaterial;
extern D3DCOLORVALUE &AmbientSaturated;

float &CTimeCycle__m_fCurrentAmbientRed = *AddressByVersion<float*>(0, 0, 0, 0x978D18, 0, 0);
float &CTimeCycle__m_fCurrentAmbientGreen = *AddressByVersion<float*>(0, 0, 0, 0xA0D8CC, 0, 0);
float &CTimeCycle__m_fCurrentAmbientBlue = *AddressByVersion<float*>(0, 0, 0, 0xA0FCA8, 0, 0);

RwBool
leedsSetSurfaceProps(RwRGBA *color, RwSurfaceProperties *surfProps, RwUInt32 flags)
{
	static D3DCOLORVALUE black = { 0, 0, 0, 0 };
	RwRGBA c = *color;

	// REMOVE: we do this in InitialiseGame_hook now
	if(config.leedsWorldAmbTweak < 0) config.leedsWorldAmbTweak = 1.0f;
	if(config.leedsWorldEmissTweak < 0) config.leedsWorldEmissTweak = 0.0f;

	RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);
	d3dmaterial.Diffuse = black;
	d3dmaterial.Diffuse.a = color->alpha/255.0f;


	if(isIII()){
		c.red *= config.currentAmbientMultRed;
		c.green *= config.currentAmbientMultGreen;
		c.blue *= config.currentAmbientMultBlue;
	}else{
		c.red *= CTimeCycle__m_fCurrentAmbientRed;
		c.green *= CTimeCycle__m_fCurrentAmbientGreen;
		c.blue *= CTimeCycle__m_fCurrentAmbientBlue;
	}
c.red *= config.leedsWorldAmbTweak;
c.green *= config.leedsWorldAmbTweak;
c.blue *= config.leedsWorldAmbTweak;

	RwD3D8SetRenderState(D3DRS_AMBIENT, D3DCOLOR_ARGB(0, c.red, c.green, c.blue));
	RwD3D8SetRenderState(D3DRS_COLORVERTEX, 1);
	RwD3D8SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_COLOR1);
	d3dmaterial.Ambient = black;

	RwD3D8SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL);
	d3dmaterial.Emissive.r = AmbientSaturated.r * 0.5f;
	d3dmaterial.Emissive.g = AmbientSaturated.g * 0.5f;
	d3dmaterial.Emissive.b = AmbientSaturated.b * 0.5f;

d3dmaterial.Emissive.r += config.leedsWorldEmissTweak;
d3dmaterial.Emissive.g += config.leedsWorldEmissTweak;
d3dmaterial.Emissive.b += config.leedsWorldEmissTweak;
if(d3dmaterial.Emissive.r > 1.0f) d3dmaterial.Emissive.r = 1.0f;
if(d3dmaterial.Emissive.g > 1.0f) d3dmaterial.Emissive.g = 1.0f;
if(d3dmaterial.Emissive.b > 1.0f) d3dmaterial.Emissive.b = 1.0f;

	if(memcmp(&lastmaterial, &d3dmaterial, sizeof(D3DMATERIAL8)) != 0){
		lastmaterial = d3dmaterial;
		return RwD3DDevice->SetMaterial(&d3dmaterial) >= 0;
	}
	return true;
}

void
leedsRenderCallback(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
	_rwD3D8EnableClippingIfNeeded(object, type);

	int lighting;
	RwD3D8GetRenderState(D3DRS_LIGHTING, &lighting);

	if(!(flags & (rpGEOMETRYTEXTURED|rpGEOMETRYTEXTURED2)))
		RwD3D8SetTexture(NULL, 0);
	RwBool curalpha = rwD3D8RenderStateIsVertexAlphaEnable();
	void *vertbuf = (void*)0xFFFFFFFF;
	RxD3D8ResEntryHeader *header = (RxD3D8ResEntryHeader*)&repEntry[1];
	RxD3D8InstanceData *inst = (RxD3D8InstanceData*)&header[1];
	RwD3D8SetPixelShader(NULL);
	RwD3D8SetVertexShader(inst->vertexShader);

	for(int i = 0; i < header->numMeshes; i++){
		if(flags & (rpGEOMETRYTEXTURED|rpGEOMETRYTEXTURED2)){
			RwD3D8SetTexture(inst->material->texture, 0);
			RwD3D8SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE2X);
		}
		RwBool a = inst->vertexAlpha || inst->material->color.alpha != 0xFFu;
		if(curalpha != a)
			rwD3D8RenderStateVertexAlphaEnable(curalpha = a);
		if(lighting){
			RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, inst->vertexAlpha != 0);
			leedsSetSurfaceProps(&inst->material->color, &inst->material->surfaceProps, flags);
		}
		if(vertbuf != inst->vertexBuffer)
			RwD3D8SetStreamSource(0, vertbuf = inst->vertexBuffer, inst->stride);

		drawDualPass(inst);
		inst++;
	}

	d3dmaterial.Emissive.r = 0.0f;
	d3dmaterial.Emissive.g = 0.0f;
	d3dmaterial.Emissive.b = 0.0f;
}


#include "debugmenu_public.h"
void
leedsMenu(void)
{
	DebugMenuAddVarBool32("SkyGFX|Advanced", "Leeds Radiosity", &config.radiosity, nil);
	DebugMenuAddFloat32("SkyGFX|Advanced", "Leeds Ambient Tweak", &config.leedsWorldAmbTweak, nil, 0.02f, 0.0f, 1.0f);
	DebugMenuAddFloat32("SkyGFX|Advanced", "Leeds Emissive Tweak", &config.leedsWorldEmissTweak, nil, 0.02f, 0.0f, 1.0f);
}
