//
// RW 3.3
//

int lightingEnabled;	// d3d lighting on
int setMaterial;	// set d3d material, needed for lighting
// these two are mutually exclusive:
int setMaterialColor;	// multiply material color at vertex stage
int modulateMaterial;	// modulate by material color at texture stage

void _rxXbDefaultRenderFFPObjectSetUp(RxXboxResEntryHeader *res, void *object, char type, char flags)
{
	RwXboxGetCachedRenderState(D3DRS_LIGHTING, &lightingEnabled);
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
			RwXboxSetCachedRenderState(D3DRS_COLORVERTEX, 1);
			RwXboxSetCachedRenderState(D3DRS_EMISSIVEMATERIALSOURCE, 1);
		}else{
			RwXboxSetCachedRenderState(D3DRS_COLORVERTEX, 0);
			RwXboxSetCachedRenderState(D3DRS_EMISSIVEMATERIALSOURCE, 0);
		}
		RwXboxSetCachedRenderState(D3DRS_DIFFUSEMATERIALSOURCE, res->vertexAlpha);
	}
	normalizeNormals = 0;
	if(flags & rpGEOMETRYNORMALS && type == rpATOMIC && XbAtomicHasScaling(object)){
		RwXboxSetCachedRenderState(D3DRS_NORMALIZENORMALS, 1);
		normalizeNormals = 1;
	}
}

void _rxXbDefaultRenderFFPObjectTearDown(void)
{
	if(normalizeNormals)
		RwXboxSetCachedRenderState(142, 0);
}

RwBool _rxXbDefaultRenderFFPMeshSetUp(RxXboxInstanceData *inst)
{
	static D3DMATERIAL8 material =
		{ { 0.0f, 0.0f, 0.0f, 1.0f },
		  { 0.0f, 0.0f, 0.0f, 1.0f },
		  { 0.0f, 0.0f, 0.0f, 1.0f },
		  { 0.0f, 0.0f, 0.0f, 1.0f },
		  0.0f };
	float a, d;
	RpMaterial *m;
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
		D3DDevice_SetMaterial(&material);
	}
}

void _rxXbDefaultRenderFFPMeshCombinerSetUp(RxXboxInstanceData *inst, RwUInt8 flags)
{
	if(flags & (rpGEOMETRYTEXTURED|rpGEOMETRYTEXTURED2)){
		RwXboxSetCachedTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		RwXboxSetCachedTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
		RwXboxSetCachedTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TEXTURE);
		RwXboxSetCachedTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
		RwXboxSetCachedTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
		RwXboxSetCachedTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TEXTURE);
	}else{
		RwXboxSetCachedTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
		RwXboxSetCachedTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
		RwXboxSetCachedTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		RwXboxSetCachedTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
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
		RwXboxSetCachedRenderState(D3DRS_TEXTUREFACTOR, tf);
		RwXboxSetCachedTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MODULATE);
		RwXboxSetCachedTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TFACTOR);
		RwXboxSetCachedTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);
		RwXboxSetCachedTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
		RwXboxSetCachedTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
		RwXboxSetCachedTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
	}
}

void _rxXbDefaultRenderFFPMeshCombinerTearDown(void)
{
	if(modulateMaterial){
		RwXboxSetCachedTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
		RwXboxSetCachedTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	}
}

void _rxXbDefaultRenderFFPMesh(RxXboxResEntryHeader *res, RxXboxInstanceData *inst, int flags)
{
	if(flags & (rpGEOMETRYTEXTURED|rpGEOMETRYTEXTURED2))
		RwXboxRenderStateSetTexture(inst->material->texture, 0);
	else
		RwXboxRenderStateSetTexture(NULL, 0);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,
	                 res->vertexAlpha || inst->material->color.alpha != 0xFF);
	RwXboxSetCurrentVertexShader(inst->vertexShader);
	if(inst->pixelShader == NULL)
		_rxXbDefaultRenderFFPMeshCombinerSetUp(inst, flags);
	RwXboxSetCurrentPixelShader(inst->pixelShader);
	RwXboxDrawIndexedVertices(res->primType, inst->numIndices, inst->indexBuffer);
	if(inst->pixelShader == NULL)
		_rxXbDefaultRenderFFPMeshCombinerTearDown();
}

void _rxXbDefaultRenderCallback(RxXboxResEntryHeader *res, void *object, int type, int flags)
{
	D3DDevice_SetStreamSource(0, res->vertexBuffer, res->stride);
	_rxXbDefaultRenderFFPObjectSetUp(res, object, type, flags);
	for(i = res->begin; i != res->end; i++){
		_rxXbDefaultRenderFFPMeshSetUp(i);
		_rxXbDefaultRenderFFPMesh(res, i, flags);
	}
	_rxXbDefaultRenderFFPObjectTearDown();
}
