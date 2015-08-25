#include "skygfx_vc.h"
#include "d3d8.h"
#include "d3d8types.h"

WRAPPER int rwD3D8RWGetRasterStage(int) { EAXJMP(0x659840); }
WRAPPER void rpSkinD3D8CreatePlainPipe(void) { EAXJMP(0x6796D0); }

void *rimVS = NULL;

void
rxD3D8SkinRenderCallback(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
	RpAtomic *atomic = (RpAtomic*)object;
	int lighting, dither, shademode;
	int foo = 0;
	RwD3D8GetRenderState(D3DRS_LIGHTING, &lighting);
	if(lighting){
		if(flags & rpGEOMETRYPRELIT){
			RwD3D8SetRenderState(D3DRS_COLORVERTEX, 1);
			RwD3D8SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, 1);
		}else{
			RwD3D8SetRenderState(D3DRS_COLORVERTEX, 0);
			RwD3D8SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, 0);
		}
	}else{
		if(!(flags & rpGEOMETRYPRELIT)){
			foo = 1;
			RwD3D8GetRenderState(D3DRS_DITHERENABLE, &dither);
			RwD3D8GetRenderState(D3DRS_SHADEMODE, &shademode);
			RwD3D8SetRenderState(D3DRS_TEXTUREFACTOR, 0xFF000000u);
			RwD3D8SetRenderState(D3DRS_DITHERENABLE, 0);
			RwD3D8SetRenderState(D3DRS_SHADEMODE, 1);
		}
	}

	int clip;
	if(type != 1){
		if(RwD3D8CameraIsBBoxFullyInsideFrustum((RwCamera*)((RwGlobals*)RwEngineInst)->curCamera,
		                                        (char*)object + 104))
			clip = 0;
		else
			clip = 1;
	}else{
		if(RwD3D8CameraIsBBoxFullyInsideFrustum((RwCamera*)((RwGlobals*)RwEngineInst)->curCamera,
		                                        RpAtomicGetWorldBoundingSphere(atomic)))
			clip = 0;
		else
			clip = 1;
	}
	RwD3D8SetRenderState(D3DRS_CLIPPING, clip);
	if(!(flags & (rpGEOMETRYTEXTURED|rpGEOMETRYTEXTURED2))){
		RwD3D8SetTexture(0, 0);
		if(foo){
			RwD3D8SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
			RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
			RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		}
	}

	uploadConstants(1.0f);
	RwRGBAReal rampStart, rampEnd;
	rampStart.red = rampStart.green = rampStart.blue = 0.0f;
	rampEnd.red = rampEnd.green = rampEnd.blue = 1.0f;
	rampStart.alpha = rampEnd.alpha = 1.0f;
	RwD3D9SetVertexPixelShaderConstant(LOC_rampStart, (void*)&rampStart, 1);
	RwD3D9SetVertexPixelShaderConstant(LOC_rampEnd, (void*)&rampEnd, 1);
	float rim[4] = { 0.5f, 1.0f, 2.0f, 0.0f };
	RwD3D9SetVertexPixelShaderConstant(LOC_rim, (void*)&rim, 1);

	int alpha = rwD3D8RenderStateIsVertexAlphaEnable();
	int bar = -1;
	RxD3D8ResEntryHeader *header = (RxD3D8ResEntryHeader*)&repEntry[1];
	RxD3D8InstanceData *inst = (RxD3D8InstanceData*)&header[1];
	RwD3D8SetPixelShader(NULL);
	RwD3D8SetVertexShader(inst->vertexShader);

	RwD3D9SetVertexShader(rimVS);

	for(int i = 0; i < header->numMeshes; i++){
		if(flags & (rpGEOMETRYTEXTURED|rpGEOMETRYTEXTURED2)){
			RwD3D8SetTexture(inst->material->texture, 0);
			if(foo){
				RwD3D8SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
				RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
			}
		}
		if(inst->vertexAlpha || inst->material->color.alpha != 0xFFu){
			if(!alpha){
				alpha = 1;
				rwD3D8RenderStateVertexAlphaEnable(1);
			}
		}else{
			if(alpha){
				alpha = 0;
				rwD3D8RenderStateVertexAlphaEnable(0);
			}
		}
		if(lighting){
			RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, inst->vertexAlpha != 0);
			RwD3D8SetSurfaceProperties(&inst->material->color, &inst->material->surfaceProps, flags & 0x40);
		}

		RwSurfaceProperties sp = inst->material->surfaceProps;
		sp.specular = 1.0f;	// rim light
		if(GetAsyncKeyState(VK_F6) & 0x8000)
			sp.specular = 0.0f;
		RwD3D9SetVertexPixelShaderConstant(LOC_surfProps, (void*)&sp, 1);
		RwRGBAReal color;
		RwRGBARealFromRwRGBA(&color, &inst->material->color);
		RwD3D9SetVertexPixelShaderConstant(LOC_matCol, (void*)&color, 1);

		RwD3D8SetStreamSource(0, inst->vertexBuffer, inst->stride);
		if(inst->indexBuffer){
			RwD3D8SetIndices(inst->indexBuffer, inst->baseIndex);
			RwD3D8DrawIndexedPrimitive(inst->primType, 0, inst->numVertices, 0, inst->numIndices);
		}else
			RwD3D8DrawPrimitive(inst->primType, inst->baseIndex, inst->numVertices);

		inst++;
	}

	RwD3D8SetVertexShader(NULL);
	if(foo){
		RwD3D8SetRenderState(D3DRS_DITHERENABLE, dither);
		RwD3D8SetRenderState(D3DRS_SHADEMODE, shademode);
		if(rwD3D8RWGetRasterStage(0)){
			RwD3D8SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
			RwD3D8SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
			RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
		}else{
			RwD3D8SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
			RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
		}
	}
}

void
rpSkinD3D8CreatePlainPipe_hook(void)
{
	if(iCanHasD3D9){
		HRSRC resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_RIMVS), RT_RCDATA);
		RwUInt32 *shader = (RwUInt32*)LoadResource(dllModule, resource);
		RwD3D9CreateVertexShader(shader, &rimVS);
		assert(rimVS);
		FreeResource(shader);
		MemoryVP::Patch(0x6796B9, rxD3D8SkinRenderCallback);
	}
	rpSkinD3D8CreatePlainPipe();
}
