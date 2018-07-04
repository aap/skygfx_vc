void
_rwD3D8AtomicMatFXRenderCallback(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
	RwBool lighting;
	RwBool forceBlack;
	RxD3D8ResEntryHeader *header;
	RxD3D8InstanceData *inst;
	RwInt32 i;

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
		RwD3D8SetTexture(nil, 0);
		RwD3D8SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_RGBA(0, 0, 0, 255));
		RwD3D8SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
		RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
	}

	header = (RxD3D8ResEntryHeader*)(repEntry + 1);
	inst = (RxD3D8InstanceData*)(header + 1);
	for(i = 0; i < header->numMeshes; i++){
		if(forceBlack)
			_rpMatFXD3D8AtomicMatFXRenderBlack(inst);
		else{
			if(lighting)
				RwD3D8SetSurfaceProperties(&inst->material->color, &inst->material->surfaceProps, flags & rpGEOMETRYMODULATEMATERIALCOLOR);
			MatFX *matfx = *RWPLUGINOFFSET(MatFX*, inst->material, MatFXMaterialDataOffset);
			int effect = matfx ? matfx->effects : rpMATFXEFFECTNULL;
			switch(effect){
			case rpMATFXEFFECTNULL:
			default:
				_rpMatFXD3D8AtomicMatFXDefaultRender(inst, flags, inst->material->texture);
				break;
			case rpMATFXEFFECTBUMPMAP:
				_rpMatFXD3D8AtomicMatFXBumpMapRender(inst, flags, inst->material->texture, matfx->fx[0].b.bumpedTex, nil);
				break;
			case rpMATFXEFFECTENVMAP:
				_rpMatFXD3D8AtomicMatFXEnvRender(inst, flags, 0, inst->material->texture, matfx->fx[0].e.envTex);
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
