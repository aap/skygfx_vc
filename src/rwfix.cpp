#include "skygfx.h"
#include "d3d8.h"
#include "d3d8types.h"

extern IDirect3DDevice8 *&RwD3DDevice;

/* Instance callback that doesn't premultiply vertex colors by material color */
RwBool
D3D8AtomicDefaultInstanceCallback_fixed(void *object, RxD3D8InstanceData *instancedData, RwBool reinstance)
{
	RpAtomic *atomic = (RpAtomic*)object;
	RwUInt32 mod = atomic->geometry->flags & rpGEOMETRYMODULATEMATERIALCOLOR;
	atomic->geometry->flags &= ~rpGEOMETRYMODULATEMATERIALCOLOR;
	RwBool ret = D3D8AtomicDefaultInstanceCallback(object, instancedData, reinstance);
	atomic->geometry->flags |= mod;
	return ret;
}

RwBool
D3D8AtomicDefaultReinstanceCallback_fixed(void *object, RwResEntry *resEntry, const RpMeshHeader *meshHeader, RxD3D8AllInOneInstanceCallBack instanceCallback)
{
	RpAtomic *atomic = (RpAtomic*)object;
	RwUInt32 mod = atomic->geometry->flags & rpGEOMETRYMODULATEMATERIALCOLOR;
	atomic->geometry->flags &= ~rpGEOMETRYMODULATEMATERIALCOLOR;
	RwBool ret = D3D8AtomicDefaultReinstanceCallback(object, resEntry, meshHeader, instanceCallback);
	atomic->geometry->flags |= mod;
	return ret;
}

D3DMATERIAL8 d3dmaterial = { { 0.0f, 0.0f, 0.0f, 1.0f },
                             { 0.0f, 0.0f, 0.0f, 1.0f },
                             { 0.0f, 0.0f, 0.0f, 1.0f },
                             { 0.0f, 0.0f, 0.0f, 1.0f },
                             0.0f };
// ADDRESS
D3DMATERIAL8 &lastmaterial = *AddressByVersion<D3DMATERIAL8*>(0x662EA8, 0, 0, 0x789760, 0, 0);
D3DCOLORVALUE &AmbientSaturated = *AddressByVersion<D3DCOLORVALUE*>(0x619458, 0, 0, 0x6DDE08, 0, 0);

//
// D3D9-like pipeline
//

RwReal lastDiffuse;
RwReal lastAmbient;
RwUInt32 lastWasTextured;
RwRGBA lastColor;
float lastAmbRed;
float lastAmbGreen;
float lastAmbBlue;

RwBool
RwD3D8SetSurfaceProperties_d3d9(RwRGBA *color, RwSurfaceProperties *surfProps, RwUInt32 flags)
{
/*
	if(lastDiffuse == surfProps->diffuse  &&
	   lastAmbient == surfProps->ambient  &&
	   lastColor.red == color->red        &&
	   lastColor.green == color->green    &&
	   lastColor.blue == color->blue      &&
	   lastColor.alpha == color->alpha    &&
	   lastWasTextured == (flags & (rpGEOMETRYTEXTURED|rpGEOMETRYTEXTURED2)) &&
	   lastAmbRed == AmbientSaturated.r   &&
	   lastAmbGreen == AmbientSaturated.g &&
	   lastAmbBlue == AmbientSaturated.b)
		return true;

	lastDiffuse = surfProps->diffuse;
	lastAmbient = surfProps->ambient;
	lastColor = *color;
	lastWasTextured = flags & (rpGEOMETRYTEXTURED|rpGEOMETRYTEXTURED2);
	lastAmbRed = AmbientSaturated.r;
	lastAmbGreen = AmbientSaturated.g;
	lastAmbBlue = AmbientSaturated.b;
*/
	if(flags & rpGEOMETRYMODULATEMATERIALCOLOR &&
	   !(color->red == 0xFF && color->green == 0xFF && color->blue == 0xFF && color->alpha == 0xFF)){
		float d = surfProps->diffuse/255.0f;
		float a = surfProps->ambient/255.0f;
		d3dmaterial.Diffuse.r = color->red   * d;
		d3dmaterial.Diffuse.g = color->green * d;
		d3dmaterial.Diffuse.b = color->blue  * d;
		d3dmaterial.Diffuse.a = color->alpha / 255.0f;
		if(flags & rpGEOMETRYPRELIT){
			// prelighting as ambient, material color as ambient light so they can be multiplied
			RwD3D8SetRenderState(D3DRS_AMBIENT, D3DCOLOR_ARGB(color->alpha, color->red, color->green, color->blue));
			RwD3D8SetRenderState(D3DRS_COLORVERTEX, 1);
			RwD3D8SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_COLOR1);
			d3dmaterial.Ambient.r = 0.0f;	    //
			d3dmaterial.Ambient.g = 0.0f;	    // unused
			d3dmaterial.Ambient.b = 0.0f;	    //
			// premultiplied rwAmbient as emissive
			RwD3D8SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL);
			d3dmaterial.Emissive.r = color->red   * AmbientSaturated.r * a;
			d3dmaterial.Emissive.g = color->green * AmbientSaturated.g * a;
			d3dmaterial.Emissive.b = color->blue  * AmbientSaturated.b * a;
		}else{
			// premultiplied rwAmbient as ambient
			RwD3D8SetRenderState(D3DRS_AMBIENT, D3DCOLOR_ARGB(0xFF,0xFF,0xFF,0xFF));
			RwD3D8SetRenderState(D3DRS_COLORVERTEX, 0);
			RwD3D8SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL);
			d3dmaterial.Ambient.r = color->red   * AmbientSaturated.r * a;
			d3dmaterial.Ambient.g = color->green * AmbientSaturated.g * a;
			d3dmaterial.Ambient.b = color->blue  * AmbientSaturated.b * a;
			// unused
			RwD3D8SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL);
			d3dmaterial.Emissive.r = 0.0f;
			d3dmaterial.Emissive.g = 0.0f;
			d3dmaterial.Emissive.b = 0.0f;
		}
	}else{
		d3dmaterial.Diffuse.r = surfProps->diffuse;
		d3dmaterial.Diffuse.g = surfProps->diffuse;
		d3dmaterial.Diffuse.b = surfProps->diffuse;
		d3dmaterial.Diffuse.a = 1.0f;
		RwD3D8SetRenderState(D3DRS_AMBIENT, D3DCOLOR_ARGB(0xFF,0xFF,0xFF,0xFF));
		RwD3D8SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL);
		if(flags & rpGEOMETRYPRELIT){
			RwD3D8SetRenderState(D3DRS_COLORVERTEX, 1);
			RwD3D8SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_COLOR1);
		}else{
			RwD3D8SetRenderState(D3DRS_COLORVERTEX, 0);
			RwD3D8SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL);
			d3dmaterial.Emissive.r = 0.0f;
			d3dmaterial.Emissive.g = 0.0f;
			d3dmaterial.Emissive.b = 0.0f;
		}
		if(surfProps->ambient == 1.0f){
			d3dmaterial.Ambient.r = AmbientSaturated.r;
			d3dmaterial.Ambient.g = AmbientSaturated.g;
			d3dmaterial.Ambient.b = AmbientSaturated.b;
		}else{
			d3dmaterial.Ambient.r = AmbientSaturated.r * surfProps->ambient;
			d3dmaterial.Ambient.g = AmbientSaturated.g * surfProps->ambient;
			d3dmaterial.Ambient.b = AmbientSaturated.b * surfProps->ambient;
		}
	}
	if(memcmp(&lastmaterial, &d3dmaterial, sizeof(D3DMATERIAL8)) != 0){
		lastmaterial = d3dmaterial;
		return RwD3DDevice->SetMaterial(&d3dmaterial) >= 0;
	}
	return true;
}

void
rxD3D8DefaultRenderCallback_d3d9(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
	_rwD3D8EnableClippingIfNeeded(object, type);

	int lighting, dither, shademode;
	int nocolor = 0;
	RwD3D8GetRenderState(D3DRS_LIGHTING, &lighting);
	if(!lighting){
		if(!(flags & rpGEOMETRYPRELIT)){
			nocolor = 1;
			RwD3D8GetRenderState(D3DRS_DITHERENABLE, &dither);
			RwD3D8GetRenderState(D3DRS_SHADEMODE, &shademode);
			RwD3D8SetRenderState(D3DRS_TEXTUREFACTOR, 0xFF000000u);
			RwD3D8SetRenderState(D3DRS_DITHERENABLE, 0);
			RwD3D8SetRenderState(D3DRS_SHADEMODE, 1);
		}
	}

	if(!(flags & (rpGEOMETRYTEXTURED|rpGEOMETRYTEXTURED2))){
		RwD3D8SetTexture(NULL, 0);
		if(nocolor){
			RwD3D8SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
			RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
			RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		}
	}
	RwBool curalpha = rwD3D8RenderStateIsVertexAlphaEnable();
	void *vertbuf = (void*)0xFFFFFFFF;
	RxD3D8ResEntryHeader *header = (RxD3D8ResEntryHeader*)&repEntry[1];
	RxD3D8InstanceData *inst = (RxD3D8InstanceData*)&header[1];
	RwD3D8SetPixelShader(NULL);
	RwD3D8SetVertexShader(inst->vertexShader);

	for(int i = 0; i < header->numMeshes; i++){
		if(flags & (rpGEOMETRYTEXTURED|rpGEOMETRYTEXTURED2)){
			RwD3D8SetTexture(inst->material->texture, 0);
			if(nocolor){
				RwD3D8SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
				RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
			}
		}
		RwBool a = inst->vertexAlpha || inst->material->color.alpha != 0xFFu;
		if(curalpha != a)
			rwD3D8RenderStateVertexAlphaEnable(curalpha = a);
		if(lighting){
			RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, inst->vertexAlpha != 0);
			RwD3D8SetSurfaceProperties_d3d9(&inst->material->color, &inst->material->surfaceProps, flags);
		}
		if(vertbuf != inst->vertexBuffer)
			RwD3D8SetStreamSource(0, vertbuf = inst->vertexBuffer, inst->stride);

		drawDualPass(inst);
/*
		if(inst->indexBuffer){
			RwD3D8SetIndices(inst->indexBuffer, inst->baseIndex);
			RwD3D8DrawIndexedPrimitive(inst->primType, 0, inst->numVertices, 0, inst->numIndices);
		}else
			RwD3D8DrawPrimitive(inst->primType, inst->baseIndex, inst->numVertices);
*/
		inst++;
	}

	if(nocolor){
		RwD3D8SetRenderState(D3DRS_DITHERENABLE, dither);
		RwD3D8SetRenderState(D3DRS_SHADEMODE, shademode);
		if(rwD3D8RWGetRasterStage(0)){		// what is this?
			RwD3D8SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
			RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TEXTURE);
			RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
		}else{
			RwD3D8SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
			RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
		}
	}
	d3dmaterial.Emissive.r = 0.0f;
	d3dmaterial.Emissive.g = 0.0f;
	d3dmaterial.Emissive.b = 0.0f;
}


//
// Xbox-like pipeline
//

int lightingEnabled;	// d3d lighting on
int setMaterial;	// set d3d material, needed for lighting
// these two are mutually exclusive:
int setMaterialColor;	// multiply material color at vertex stage
int modulateMaterial;	// modulate by material color at texture stage

void
rxD3D8DefaultRenderFFPObjectSetUp(RwUInt32 flags)
{
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
			RwD3D8SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, 1);
		}else{
			RwD3D8SetRenderState(D3DRS_COLORVERTEX, 0);
			RwD3D8SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, 0);
		}
	}
}

void
rxD3D8DefaultRenderFFPMeshSetUp(RxD3D8InstanceData *inst)
{
	float a, d;
	RpMaterial *m;
	if(setMaterial){
		m = inst->material;
		if(setMaterialColor){
			a = m->surfaceProps.ambient / 255.0f;
			d = m->surfaceProps.diffuse / 255.0f;
			d3dmaterial.Diffuse.r = m->color.red   * d;
			d3dmaterial.Diffuse.g = m->color.green * d;
			d3dmaterial.Diffuse.b = m->color.blue  * d;
			d3dmaterial.Diffuse.a = m->color.alpha * d;
			d3dmaterial.Ambient.r = m->color.red   * a;
			d3dmaterial.Ambient.g = m->color.green * a;
			d3dmaterial.Ambient.b = m->color.blue  * a;
			d3dmaterial.Ambient.a = m->color.alpha * a;
		}else{
			d = m->surfaceProps.diffuse;
			d3dmaterial.Diffuse.r = d;
			d3dmaterial.Diffuse.g = d;
			d3dmaterial.Diffuse.b = d;
			d3dmaterial.Diffuse.a = d;
			a = m->surfaceProps.ambient;
			d3dmaterial.Ambient.r = a;
			d3dmaterial.Ambient.g = a;
			d3dmaterial.Ambient.b = a;
			d3dmaterial.Ambient.a = a;
		}
		if(memcmp(&lastmaterial, &d3dmaterial, sizeof(D3DMATERIAL8)) != 0){
			lastmaterial = d3dmaterial;
			RwD3DDevice->SetMaterial(&d3dmaterial);
		}
	}
	if(lightingEnabled)
		RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, inst->vertexAlpha);
}

void
rxD3D8DefaultRenderFFPMeshCombinerSetUp(RxD3D8InstanceData *inst, RwUInt32 flags)
{
	if(flags & (rpGEOMETRYTEXTURED|rpGEOMETRYTEXTURED2)){
		RwD3D8SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		RwD3D8SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
		RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TEXTURE);
		RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
		RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
		RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TEXTURE);
	}else{
		RwD3D8SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
		RwD3D8SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
		RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
	}
	if(modulateMaterial){
		RwUInt32 tf;
		RwRGBA *c = &inst->material->color;
		//                     v-- what's that?
		if(!lightingEnabled && flags & rpGEOMETRYLIGHT && !(flags & rpGEOMETRYPRELIT)){
			if(flags & rpGEOMETRYMODULATEMATERIALCOLOR)
				tf = D3DCOLOR_ARGB(c->alpha, 0, 0, 0);
			else
				tf = D3DCOLOR_ARGB(0xFF, 0, 0, 0);
		}else{
			if(flags & rpGEOMETRYMODULATEMATERIALCOLOR)
				tf = D3DCOLOR_ARGB(c->alpha, c->red, c->green, c->blue);
			else
				tf = D3DCOLOR_ARGB(0xFF,0xFF,0xFF,0xFF);
		}
		RwD3D8SetRenderState(D3DRS_TEXTUREFACTOR, tf);
		RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MODULATE);
		RwD3D8SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TFACTOR);
		RwD3D8SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);
		RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
		RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
		RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
	}
}

void
rxD3D8DefaultRenderFFPMeshCombinerTearDown(void)
{
	if(modulateMaterial){
		RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
		RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	}
}

void
rxD3D8DefaultRenderFFPMesh(RxD3D8InstanceData *inst, RwUInt32 flags)
{
	RwD3D8SetStreamSource(0, inst->vertexBuffer, inst->stride);
	/* Better mark this mesh not textured if there is no texture.
	 * NULL textures can cause problems for the combiner...
	 * for me the next stage (modulation by tfactor) was ignored. */
	if(inst->material->texture == NULL)
		flags &= ~(rpGEOMETRYTEXTURED|rpGEOMETRYTEXTURED2);

	if(flags & (rpGEOMETRYTEXTURED|rpGEOMETRYTEXTURED2))
		RwD3D8SetTexture(inst->material->texture, 0);
	else
		RwD3D8SetTexture(NULL, 0);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,
	                 (void*)(inst->vertexAlpha || inst->material->color.alpha != 0xFF));
	RwD3D8SetVertexShader(inst->vertexShader);
	RwD3D8SetPixelShader(NULL);
	rxD3D8DefaultRenderFFPMeshCombinerSetUp(inst, flags);
	drawDualPass(inst);
	rxD3D8DefaultRenderFFPMeshCombinerTearDown();
}

void
rxD3D8SetAmbientLight(void)
{
	int r = AmbientSaturated.r*255;
	int g = AmbientSaturated.g*255;
	int b = AmbientSaturated.b*255;
	RwD3D8SetRenderState(D3DRS_AMBIENT, D3DCOLOR_ARGB(0xFF, r, g, b));
}

void
rxD3D8DefaultRenderCallback_xbox(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
	rxD3D8SetAmbientLight();

	RxD3D8ResEntryHeader *header = (RxD3D8ResEntryHeader*)&repEntry[1];
	RxD3D8InstanceData *inst = (RxD3D8InstanceData*)&header[1];

	rxD3D8DefaultRenderFFPObjectSetUp(flags);
	for(int i = 0; i < header->numMeshes; i++){
		rxD3D8DefaultRenderFFPMeshSetUp(inst);
		rxD3D8DefaultRenderFFPMesh(inst, flags);
		inst++;
	}
}
