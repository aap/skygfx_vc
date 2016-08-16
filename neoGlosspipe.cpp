#include "skygfx.h"
#include "d3d8.h"
#include "d3d8types.h"
#include <DirectXMath.h>

void *GlossPipe::vertexShader;
void *GlossPipe::pixelShader;

//
// Hooks
//
static addr CRenderer__RenderOneRoad_A;
WRAPPER void CRenderer__RenderOneRoad(void *e) { VARJMP(CRenderer__RenderOneRoad_A); }
void CRenderer__RenderOneRoad_hook(void *e)
{
	RpAtomic *a = *(RpAtomic**)((uchar*)e + 0x4C);
	if(RwObjectGetType(a) == rpATOMIC)
		GlossPipe::Get()->Attach(a);
	CRenderer__RenderOneRoad(e);
}

void
neoGlossPipeInit(void)
{
	GlossPipe::Get()->Init();
	// ADDRESS
	if(gtaversion == III_10)
		InterceptCall(&CRenderer__RenderOneRoad_A, CRenderer__RenderOneRoad_hook, 0x4A78EF);
	// for VC the pipeline is set by the world pipe hook
}


GlossPipe*
GlossPipe::Get(void)
{
	static GlossPipe glosspipe;
	return &glosspipe;

}

GlossPipe::GlossPipe(void)
 : specular(1.0f, 1.0f, 1.0f, 1.0f)
{
	CreateRwPipeline();
	isActive = true;
	texdict = NULL;
}

void
GlossPipe::Init(void)
{
	SetRenderCallback(RenderCallback);
	CreateShaders();
	// way easier than neo code....
	texdict = neoTxd;
}

void
GlossPipe::CreateShaders(void)
{
	HRSRC resource;
	RwUInt32 *shader;
	resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_GLOSSVS), RT_RCDATA);
	shader = (RwUInt32*)LoadResource(dllModule, resource);
	RwD3D9CreateVertexShader(shader, &vertexShader);
	assert(vertexShader);
	FreeResource(shader);

	resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_GLOSSPS), RT_RCDATA);
	shader = (RwUInt32*)LoadResource(dllModule, resource);
	RwD3D9CreatePixelShader(shader, &pixelShader);
	assert(pixelShader);
	FreeResource(shader);
}

//
// Rendering
//

RwTexture*
GlossPipe::GetGlossTex(RpMaterial *mat)
{
	static char name[32];
	RwTexture *tex;
	GlossMatExt *glossext = NULL;
	// if we have the plugin, try to get the texture from that
	if(GlossOffset != 0){
		glossext = GETGLOSSEXT(mat);
		if(glossext->didLookup)
			return glossext->glosstex;
	}
	strcpy(name, mat->texture->name);
	strcat(name, "_gloss");
	tex = RwTexDictionaryFindNamedTexture(texdict, name);
	if(glossext){
		glossext->didLookup = true;
		glossext->glosstex = tex;
	}
	return tex;
}

void
GlossPipe::ShaderSetup(RwMatrix *world)
{
	DirectX::XMMATRIX worldMat, viewMat, projMat;
	RwCamera *cam = (RwCamera*)RWSRCGLOBAL(curCamera);

	RwMatrix view;
	RwMatrixInvert(&view, RwFrameGetLTM(RwCameraGetFrame(cam)));

	RwToD3DMatrix(&worldMat, world);
	RwToD3DMatrix(&viewMat, &view);
	viewMat.r[0] = DirectX::XMVectorNegate(viewMat.r[0]);
	// have to slightly adjust near clip to get rid of z-fighting
	MakeProjectionMatrix(&projMat, cam, 0.01f);

	DirectX::XMMATRIX combined = DirectX::XMMatrixMultiply(projMat, DirectX::XMMatrixMultiply(viewMat, worldMat));
	RwD3D9SetVertexShaderConstant(LOC_combined, (void*)&combined, 4);
	RwD3D9SetVertexShaderConstant(LOC_world, (void*)&worldMat, 4);

	RwMatrix *camfrm = RwFrameGetLTM(RwCameraGetFrame(cam));
	RwD3D9SetVertexShaderConstant(LOC_eye, (void*)RwMatrixGetPos(camfrm), 1);

	if(pDirect)
		UploadLightDirectionInv(pDirect, LOC_directDir);

	Color c1(0.0f, 0.0f, 1.0f, 1.0f);
	RwD3D9SetVertexShaderConstant(LOC_gloss2, &c1, 1);
	Color c2(0.5f, 0.5f, 0.5f, 0.5f);
	RwD3D9SetVertexShaderConstant(LOC_gloss3, &c2, 1);
}

void
GlossPipe::RenderGloss(RxD3D8ResEntryHeader *header)
{
	int zwrite, spec, fog, src, dst;
	RxD3D8InstanceData *inst = (RxD3D8InstanceData*)&header[1];

	RwD3D8GetRenderState(D3DRS_ZWRITEENABLE, &zwrite);
	RwD3D8GetRenderState(D3DRS_SPECULARENABLE, &spec);
	RwD3D8GetRenderState(D3DRS_FOGENABLE, &fog);
	RwRenderStateGet(rwRENDERSTATESRCBLEND, &src);
	RwRenderStateGet(rwRENDERSTATEDESTBLEND, &dst);

	RwD3D8SetRenderState(D3DRS_ZWRITEENABLE, 0);
	RwD3D8SetRenderState(D3DRS_SPECULARENABLE, 1);		  // is this even used?
	RwD3D8SetRenderState(D3DRS_FOGENABLE, 1);		  // why, vienna?

	RwD3D8SetTexture(NULL, 0);
	RwD3D8SetTexture(NULL, 1);
	RwD3D8SetTexture(NULL, 2);
	RwD3D8SetTexture(NULL, 3);

	RwD3D9SetVertexShader(vertexShader);                                   // 9!
	RwD3D9SetPixelShader(pixelShader);                                     // 8!

	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);

	for(int i = 0; i < header->numMeshes; i++){
		if(inst->material->texture){
			RwTexture *gloss = Get()->GetGlossTex(inst->material);
			if(gloss){
				RwD3D8SetStreamSource(0, inst->vertexBuffer, inst->stride);
				RwD3D9SetFVF(inst->vertexShader);				       // 9!

				RwD3D8SetTexture(gloss, 0);
				if(inst->indexBuffer){
					RwD3D8SetIndices(inst->indexBuffer, inst->baseIndex);
					RwD3D8DrawIndexedPrimitive(inst->primType, 0, inst->numVertices, 0, inst->numIndices);
				}else
					RwD3D8DrawPrimitive(inst->primType, inst->baseIndex, inst->numVertices);
			}
		}
		inst++;
	}
	RwD3D8SetVertexShader(NULL);
	RwD3D8SetPixelShader(NULL);
	RwD3D8SetRenderState(D3DRS_ZWRITEENABLE, zwrite);
	RwD3D8SetRenderState(D3DRS_SPECULARENABLE, spec);
	RwD3D8SetRenderState(D3DRS_FOGENABLE, fog);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)src);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)dst);
}

void
GlossPipe::RenderCallback(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
	RxD3D8ResEntryHeader *header = (RxD3D8ResEntryHeader*)&repEntry[1];
						{
							static bool keystate = false;
							if(GetAsyncKeyState(config.neoGlossPipeKey) & 0x8000){
								if(!keystate){
									keystate = true;
									GlossPipe::Get()->isActive = !GlossPipe::Get()->isActive;
								}
							}else
								keystate = false;
						}
	WorldPipe::Get()->RenderCallback(repEntry, object, type, flags);
	if(GlossPipe::Get()->isActive){
		ShaderSetup(RwFrameGetLTM(RpAtomicGetFrame((RpAtomic*)object)));
		RenderGloss(header);
	}
}