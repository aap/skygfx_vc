RwBool vertexAlphaEnabled;
RwBool textureAlphaEnabled;

RwBool
_rwD3D8RenderStateVertexAlphaEnable(RwBool enable)
{
	if(enable){
		if(vertexAlphaEnabled)
			return 1;
		if(textureAlphaEnabled){
			RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
			RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		}else{
			RwD3D8SetRenderState(D3DRS_ALPHABLENDENABLE, (void*)1);
			RwD3D8SetRenderState(D3DRS_ALPHATESTENABLE, (void*)1);
			RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
		}
		RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
		vertexAlphaEnabled = 1;
	}else{
 		if(!vertexAlphaEnabled)
			return 1;
		if(textureAlphaEnabled){
			RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
			RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		}else{
			RwD3D8SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
			RwD3D8SetRenderState(D3DRS_ALPHATESTENABLE, 0);
			RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		}
		vertexAlphaEnabled = 0;
	}
	return 1;
}

rwD3D8RWSetRasterStage(RwRaster *raster, RwInt32 stage)
{
	D3D8RasterExt *natras = nil;
	if(raster)
		natras = RWPLUGINOFFSET(D3D8RasterExt, raster, RwD3D8RasterExtOffset);
	if(stage == 0){
		if(raster && natras->texture && natras->hasAlpha){
			if(!textureAlphaEnabled){
				textureAlphaEnabled = 1;
				if(!vertexAlphaEnabled){
					RwD3D8SetRenderState(D3DRS_ALPHABLENDENABLE, (void*)1);
					RwD3D8SetRenderState(D3DRS_ALPHATESTENABLE, (void*)1);
				}
			}
		}else{
			if(textureAlphaEnabled){
				textureAlphaEnabled = 0;
				if(!vertexAlphaEnabled){
					RwD3D8SetRenderState(D3DRS_ALPHABLENDENABLE, (void*)0);
					RwD3D8SetRenderState(D3DRS_ALPHATESTENABLE, (void*)0);
				}
			}
		}
	}
foo:
}
