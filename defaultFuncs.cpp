IDirect3DDevice8 *RwD3DDevice;
D3DMATERIAL8 d3dmaterial = { { 0.0f, 0.0f, 0.0f, 1.0f },
                             { 0.0f, 0.0f, 0.0f, 1.0f },
                             { 0.0f, 0.0f, 0.0f, 1.0f },
                             { 0.0f, 0.0f, 0.0f, 1.0f },
                             0.0f };
D3DMATERIAL8 lastmaterial;
D3DCOLORVALUE AmbientSaturated;

RwBool
RwD3D8SetSurfaceProperties(RwRGBA *color, RwSurfaceProperties *surfProps, int modulate)
{
	if(modulate && !(color->red == 0xFF && color->green == 0xFF && color->blue == 0xFF && color->alpha == 0xFF)){
		float d = surfProps->diffuse/255.0f;
		float a = surfProps->ambient/255.0f;
		d3dmaterial.Diffuse.r = color->red   * d;
		d3dmaterial.Diffuse.g = color->green * d;
		d3dmaterial.Diffuse.b = color->blue  * d;
		d3dmaterial.Diffuse.a = color->alpha / 255.0f;
		d3dmaterial.Ambient.r = color->red   * AmbientSaturated.r * a;
		d3dmaterial.Ambient.g = color->green * AmbientSaturated.g * a;
		d3dmaterial.Ambient.b = color->blue  * AmbientSaturated.b * a;
		d3dmaterial.Ambient.a = color->alpha / 255.0f;
	}else{
		d3dmaterial.Diffuse.r = surfProps->diffuse;
		d3dmaterial.Diffuse.g = surfProps->diffuse;
		d3dmaterial.Diffuse.b = surfProps->diffuse;
		d3dmaterial.Diffuse.a = 1.0f;
		if(surfProps->ambient == 1.0f){
			d3dmaterial.Ambient.r = AmbientSaturated.r;
			d3dmaterial.Ambient.g = AmbientSaturated.g;
			d3dmaterial.Ambient.b = AmbientSaturated.b;
		}else{
			d3dmaterial.Ambient.r = AmbientSaturated.r * surfProps->ambient;
			d3dmaterial.Ambient.g = AmbientSaturated.g * surfProps->ambient;
			d3dmaterial.Ambient.b = AmbientSaturated.b * surfProps->ambient;
		}
		d3dmaterial.Ambient.a = 1.0f;
	}
	if(lastmaterial.Diffuse.r != d3dmaterial.Diffuse.r ||
	   lastmaterial.Diffuse.g != d3dmaterial.Diffuse.g ||
	   lastmaterial.Diffuse.b != d3dmaterial.Diffuse.b ||
	   lastmaterial.Diffuse.a != d3dmaterial.Diffuse.a ||
	   lastmaterial.Ambient.r != d3dmaterial.Ambient.r ||
	   lastmaterial.Ambient.g != d3dmaterial.Ambient.g ||
	   lastmaterial.Ambient.b != d3dmaterial.Ambient.b ||
	   lastmaterial.Ambient.a != d3dmaterial.Ambient.a){
		lastmaterial = d3dmaterial;
		return RwD3DDevice->SetMaterial(&d3dmaterial) >= 0;
	}
	return true;
}


void
rxD3D8DefaultRenderCallback_fixed(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
	int lighting, dither, shademode;
	int nocolor = 0;
	RwD3D8GetRenderState(D3DRS_LIGHTING, &lighting);
	if(lighting){
		if(flags & rpGEOMETRYPRELIT){
			RwD3D8SetRenderState(D3DRS_COLORVERTEX, 1);
			RwD3D8SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_COLOR1);
		}else{
			RwD3D8SetRenderState(D3DRS_COLORVERTEX, 0);
			RwD3D8SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL);
		}
	}else{
		if(!(flags & rpGEOMETRYPRELIT)){
			nocolor = 1;
			RwD3D8GetRenderState(D3DRS_DITHERENABLE, &dither);
			RwD3D8GetRenderState(D3DRS_SHADEMODE, &shademode);
			RwD3D8SetRenderState(D3DRS_TEXTUREFACTOR, 0xFF000000u);
			RwD3D8SetRenderState(D3DRS_DITHERENABLE, 0);
			RwD3D8SetRenderState(D3DRS_SHADEMODE, 1);
		}
	}

	int clip;
	if(type == rpATOMIC)
		clip = !RwD3D8CameraIsSphereFullyInsideFrustum((RwCamera*)((RwGlobals*)RwEngineInst)->curCamera,
		                                               RpAtomicGetWorldBoundingSphere((RpAtomic*)object));
	else
		clip = !RwD3D8CameraIsBBoxFullyInsideFrustum((RwCamera*)((RwGlobals*)RwEngineInst)->curCamera,
		                                             &((RpWorldSector*)object)->tightBoundingBox);
	RwD3D8SetRenderState(D3DRS_CLIPPING, clip);
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
			RwD3D8SetSurfaceProperties(&inst->material->color, &inst->material->surfaceProps,
			                           flags & rpGEOMETRYMODULATEMATERIALCOLOR);
		}
		if(vertbuf != inst->vertexBuffer)
			RwD3D8SetStreamSource(0, vertbuf = inst->vertexBuffer, inst->stride);
		if(inst->indexBuffer){
			RwD3D8SetIndices(inst->indexBuffer, inst->baseIndex);
			RwD3D8DrawIndexedPrimitive(inst->primType, 0, inst->numVertices, 0, inst->numIndices);
		}else
			RwD3D8DrawPrimitive(inst->primType, inst->baseIndex, inst->numVertices);

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
}

