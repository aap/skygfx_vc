#include "skygfx.h"
#include "d3d8.h"
#include "d3d8types.h"
#include <DirectXMath.h>

/*
 * The following code is fairly close to the Xbox code.
 * If you don't like it, complain to R* Vienna.
 * The tweaking values look different on the real xbox somehow :/
 */


class RimPipe : public CustomPipe
{
	void CreateShaders(void);
	void LoadTweakingTable(void);
public:
	static void *vertexShader;
	static InterpolatedColor rampStart;
	static InterpolatedColor rampEnd;
	static InterpolatedFloat offset;
	static InterpolatedFloat scale;
	static InterpolatedFloat scaling;

	RimPipe(void);
	static RimPipe *Get(void);
	void Init(void);
	static void RenderCallback(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags);
	static void ShaderSetup(RwMatrix *world);
	static void ObjectSetup(void);
	static void RenderMesh(RxD3D8InstanceData *inst);
};

void *RimPipe::vertexShader;
InterpolatedColor RimPipe::rampStart(Color(0.0f, 0.0f, 0.0f, 1.0f));
InterpolatedColor RimPipe::rampEnd(Color(1.0f, 1.0f, 1.0f, 1.0f));
InterpolatedFloat RimPipe::offset(0.5f);
InterpolatedFloat RimPipe::scale(1.5f);
InterpolatedFloat RimPipe::scaling(2.0f);

//
// Hooks
//

RxPipeline *&skinpipe = *AddressByVersion<RxPipeline**>(0x663CAC, 0x663CAC, 0x673DB0, 0x78A0D4, 0x78A0DC, 0x7890DC);

extern "C" {
__declspec(dllexport) void
AttachRimPipeToRwObject(RwObject *obj)
{
	if(config.iCanHasNeoRim){
		if(RwObjectGetType(obj) == rpATOMIC)
			RimPipe::Get()->Attach((RpAtomic*)obj);
		else if(RwObjectGetType(obj) == rpCLUMP)
			RpClumpForAllAtomics((RpClump*)obj, CustomPipe::setatomicCB, RimPipe::Get());
	}
}
}

class CPed
{
public:
	uchar pad[0x4C];
	RwObject *rwobject;
	static addr SetModelIndex_A;
	void SetModelIndex(int id);
	void SetModelIndex_hook(int id){
		this->SetModelIndex(id);
		AttachRimPipeToRwObject(this->rwobject);
	}
};
addr CPed::SetModelIndex_A;
WRAPPER void CPed::SetModelIndex(int id) { VARJMP(SetModelIndex_A); }

void neoRimPipeInit(void)
{
	RxPipelineNode *node;
	RimPipe::Get()->Init();
	node = RxPipelineFindNodeByName(skinpipe, "nodeD3D8SkinAtomicAllInOne.csl", NULL, NULL);
	*(void**)node->privateData = RimPipe::RenderCallback;
	// set the pipeline for peds unless iii_anim is taking care of that already
	if(gtaversion == III_10 && !GetModuleHandleA("iii_anim.asi")){
		InterceptVmethod(&CPed::SetModelIndex_A, &CPed::SetModelIndex_hook, 0x5F81A8);
		Patch(0x5F82B0, &CPed::SetModelIndex_hook);
		Patch(0x5F8380, &CPed::SetModelIndex_hook);
		Patch(0x5F8C38, &CPed::SetModelIndex_hook);
		Patch(0x5FA50C, &CPed::SetModelIndex_hook);
	}
}


RimPipe::RimPipe(void)
{
	rwPipeline = NULL;
}

RimPipe*
RimPipe::Get(void)
{
	static RimPipe rimpipe;
	return &rimpipe;
}

void
RimPipe::Init(void)
{
	CreateRwPipeline();
	SetRenderCallback(RenderCallback);
	LoadTweakingTable();
	CreateShaders();
}

void
RimPipe::CreateShaders(void)
{
	HRSRC resource;
	RwUInt32 *shader;
	resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_RIMVS), RT_RCDATA);
	shader = (RwUInt32*)LoadResource(dllModule, resource);
	RwD3D9CreateVertexShader(shader, &vertexShader);
	assert(RimPipe::vertexShader);
	FreeResource(shader);
}

void
RimPipe::LoadTweakingTable(void)
{

	char *path;
	FILE *dat;
	path = getpath("neo\\rimTweakingTable.dat");
	if(path == NULL){
		MessageBox(NULL, "Couldn't load 'neo\\rimTweakingTable.dat'", "Error", MB_ICONERROR | MB_OK);
		exit(0);
	}
	dat = fopen(path, "r");
	neoReadWeatherTimeBlock(dat, &rampStart);
	neoReadWeatherTimeBlock(dat, &rampEnd);
	neoReadWeatherTimeBlock(dat, &offset);
	neoReadWeatherTimeBlock(dat, &scale);
	neoReadWeatherTimeBlock(dat, &scaling);
	fclose(dat);
}

//
// Rendering
//

void
RimPipe::ShaderSetup(RwMatrix *world)
{
	DirectX::XMMATRIX worldMat, viewMat, projMat;
	RwCamera *cam = (RwCamera*)RWSRCGLOBAL(curCamera);

	RwMatrix view;
	RwMatrixInvert(&view, RwFrameGetLTM(RwCameraGetFrame(cam)));

//	RwToD3DMatrix(&worldMat, world);
	// Better get the D3D world matrix because for skinned geometry
	// the vertices might already be in world space. (III cutscene heads)
	RwD3D8GetTransform(D3DTS_WORLD, &worldMat);
	worldMat = DirectX::XMMatrixTranspose(worldMat);

	RwToD3DMatrix(&viewMat, &view);
	viewMat.r[0] = DirectX::XMVectorNegate(viewMat.r[0]);
	MakeProjectionMatrix(&projMat, cam);

	DirectX::XMMATRIX combined = DirectX::XMMatrixMultiply(projMat, DirectX::XMMatrixMultiply(viewMat, worldMat));
	RwD3D9SetVertexShaderConstant(LOC_combined, (void*)&combined, 4);
	RwD3D9SetVertexShaderConstant(LOC_world, (void*)&worldMat, 4);

	DirectX::XMVECTOR v = DirectX::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f);
	v = DirectX::XMVector4Transform(v, viewMat);
	RwD3D9SetVertexShaderConstant(LOC_viewVec, &v, 1);

	if(pAmbient)
		UploadLightColor(pAmbient, LOC_ambient);
	else
		UploadZero(LOC_ambient);
	if(pDirect){
		UploadLightColor(pDirect, LOC_directCol);
		UploadLightDirection(pDirect, LOC_directDir);
	}else{
		UploadZero(LOC_directCol);
		UploadZero(LOC_directDir);
	}
	for(int i = 0 ; i < 4; i++)
		if(i < NumExtraDirLightsInWorld && RpLightGetType(pExtraDirectionals[i]) == rpLIGHTDIRECTIONAL){
			UploadLightDirection(pExtraDirectionals[i], LOC_lightDir+i);
			UploadLightColor(pExtraDirectionals[i], LOC_lightCol+i);
		}else{
			UploadZero(LOC_lightDir+i);
			UploadZero(LOC_lightCol+i);
		}

	// Rim values
	Color c = rampStart.Get();
	RwD3D9SetVertexShaderConstant(LOC_rampStart, (void*)&c, 1);
	c = rampEnd.Get();
	RwD3D9SetVertexShaderConstant(LOC_rampEnd, (void*)&c, 1);
	float f[4];
	f[0] = offset.Get();
	f[1] = scale.Get();
	f[2] = scaling.Get();
	if(!rimlight)
		f[2] = 0.0f;
	f[3] = 1.0f;
	RwD3D9SetVertexShaderConstant(LOC_rim, (void*)f, 1);
}

void
RimPipe::ObjectSetup(void)
{
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
	RwD3D8SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
	RwD3D9SetVertexShader(vertexShader);                                   // 9!
	RwD3D8SetPixelShader(NULL);                                            // 8!
}

int textured = 1;

void
RimPipe::RenderMesh(RxD3D8InstanceData *inst)
{
						//{
						//	static bool keystate = false;
						//	if(GetAsyncKeyState(VK_F12) & 0x8000){
						//		if(!keystate){
						//			keystate = true;
						//			textured = (textured+1)%2;
						//		}
						//	}else
						//		keystate = false;
						//}

	RwD3D8SetStreamSource(0, inst->vertexBuffer, inst->stride);
	RwD3D9SetFVF(inst->vertexShader);				       // 9!
	RwRGBAReal color;
	RwRGBARealFromRwRGBA(&color, &inst->material->color);
	RwD3D9SetVertexShaderConstant(LOC_matCol, (void*)&color, 1);

	RwD3D8SetTexture(inst->material->texture, 0);
	// have to set these after the texture, RW sets texture stage states automatically
//	if(textured)
		RwD3D8SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
//	else
//		RwD3D8SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	RwD3D8SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
	RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TEXTURE);

	RwUInt32 tf = D3DCOLOR_ARGB(inst->material->color.alpha, 0xFF, 0xFF, 0xFF);
	if(tf == 0xFFFFFFFF){
		RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
		RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	}else{
		RwD3D8SetRenderState(D3DRS_TEXTUREFACTOR, tf);
		RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
		RwD3D8SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_CURRENT);
		RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
		RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
		RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
	}

	drawDualPass(inst);
}

void
RimPipe::RenderCallback(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
						{
							static bool keystate = false;
							if(GetAsyncKeyState(rimlightkey) & 0x8000){
								if(!keystate){
									keystate = true;
									rimlight = (rimlight+1)%2;
								}
							}else
								keystate = false;
						}
	if(!rimlight){
		rxD3D8DefaultRenderCallback_xbox(repEntry, object, type, flags);
		return;
	}
	ShaderSetup(RwFrameGetLTM(RpAtomicGetFrame((RpAtomic*)object)));

	RxD3D8ResEntryHeader *header = (RxD3D8ResEntryHeader*)&repEntry[1];
	RxD3D8InstanceData *inst = (RxD3D8InstanceData*)&header[1];

	ObjectSetup();
	for(int i = 0; i < header->numMeshes; i++){
		RenderMesh(inst);
		inst++;
	}
	RwD3D8SetTexture(NULL, 1);
	RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	RwD3D8SetTexture(NULL, 2);
	RwD3D8SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D8SetTextureStageState(2, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
}
