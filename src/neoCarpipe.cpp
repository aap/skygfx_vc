#include "skygfx.h"
#include "d3d8.h"
#include "d3d8types.h"
#include <DirectXMath.h>

//#define DEBUGTEX

static uint32_t CRenderer__RenderEverythingBarRoads_A = AddressByVersion<uint32_t>(0x4A7930, 0x4A7A20, 0x4A79B0, 0x4C9F40, 0x4C9F60, 0x4C9E00);
WRAPPER void CRenderer__RenderEverythingBarRoads(void) { VARJMP(CRenderer__RenderEverythingBarRoads_A); }
static uint32_t CRenderer__RenderFadingInEntities_A = AddressByVersion<uint32_t>(0x4A7910, 0x4A7A00, 0x4A7990, 0x4CA140, 0x4CA160, 0x4CA000);
WRAPPER void CRenderer__RenderFadingInEntities(void) { VARJMP(CRenderer__RenderFadingInEntities_A); }

int &skyBotRed = *AddressByVersion<int*>(0x9414D0, 0x941688, 0x9517C8, 0xA0D958, 0xA0D960, 0xA0C960);
int &skyBotGreen = *AddressByVersion<int*>(0x8F2BD0, 0x8F2C84, 0x902DC4, 0x97F208, 0x97F210, 0x97E210);
int &skyBotBlue = *AddressByVersion<int*>(0x8F625C, 0x8F6414, 0x906554, 0x9B6DF4, 0x9B6DFC, 0x9B5DFC);


class CarPipe : public CustomPipe
{
	void CreateShaders(void);
	void LoadTweakingTable(void);

	static void MakeScreenQuad(void);
	static void MakeQuadTexCoords(bool textureSpace);
	static void RenderReflectionScene(void);
public:
	static InterpolatedFloat fresnel;
	static InterpolatedFloat power;
	static InterpolatedLight diffColor;
	static InterpolatedLight specColor;
	static void *vertexShaderPass1;
	static void *vertexShaderPass2;
	// reflection map
	static RwCamera *reflectionCam;
	static RwTexture *reflectionMask;
	static RwTexture *reflectionTex;
	static RwIm2DVertex screenQuad[4];
	static RwImVertexIndex screenindices[6];

	CarPipe(void);
	void Init(void);
	static void RenderEnvTex(void);
	static void SetupEnvMap(void);
	static void RenderCallback(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags);
	static void ShaderSetup(RwMatrix *world);
	static void DiffusePass(RxD3D8ResEntryHeader *header);
	static void SpecularPass(RxD3D8ResEntryHeader *header);
};

InterpolatedFloat CarPipe::fresnel(0.4f);
InterpolatedFloat CarPipe::power(18.0f);
InterpolatedLight CarPipe::diffColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
InterpolatedLight CarPipe::specColor(Color(0.7f, 0.7f, 0.7f, 1.0f));
void *CarPipe::vertexShaderPass1;
void *CarPipe::vertexShaderPass2;
// reflection map
RwCamera *CarPipe::reflectionCam;
RwTexture *CarPipe::reflectionMask;
RwTexture *CarPipe::reflectionTex;
RwIm2DVertex CarPipe::screenQuad[4];
RwImVertexIndex CarPipe::screenindices[6] = { 0, 1, 2, 0, 2, 3 };

CarPipe carpipe;

//
// Hooks
//

extern "C" {
__declspec(dllexport) void
AttachCarPipeToRwObject(RwObject *obj)
{
	if(config.iCanHasNeoCar){
		if(RwObjectGetType(obj) == rpATOMIC)
			carpipe.Attach((RpAtomic*)obj);
		else if(RwObjectGetType(obj) == rpCLUMP)
			RpClumpForAllAtomics((RpClump*)obj, CustomPipe::setatomicCB, &carpipe);
	}
}
}

class CVehicleModelInfo {
public:
	static addr SetClump_A;
	void SetClump(RpClump *clump);
	void SetClump_hook(RpClump *clump){
		this->SetClump(clump);
		AttachCarPipeToRwObject((RwObject*)clump);
	}
};
addr CVehicleModelInfo::SetClump_A;
WRAPPER void CVehicleModelInfo::SetClump(RpClump*) { VARJMP(SetClump_A); }

class CVisibilityPlugins
{
public:
	static addr SetAtomicRenderCallback_A;
	static void SetAtomicRenderCallback(RpAtomic *atomic, RpAtomicCallBackRender cb);
	static void SetAtomicRenderCallback_hook(RpAtomic *atomic, RpAtomicCallBackRender cb){
		SetAtomicRenderCallback(atomic, cb);
		AttachCarPipeToRwObject((RwObject*)atomic);
	}
};
addr CVisibilityPlugins::SetAtomicRenderCallback_A;
WRAPPER void CVisibilityPlugins::SetAtomicRenderCallback(RpAtomic*, RpAtomicCallBackRender) { VARJMP(SetAtomicRenderCallback_A); }

static uint32_t RenderScene_A;
WRAPPER void RenderScene(void) { VARJMP(RenderScene_A); }

void
RenderScene_hook(void)
{
	RenderScene();
	CarPipe::RenderEnvTex();
}

void
neoCarPipeInit(void)
{
	carpipe.Init();
	CarPipe::SetupEnvMap();
	InterceptVmethod(&CVehicleModelInfo::SetClump_A, &CVehicleModelInfo::SetClump_hook,
	                 AddressByVersion<addr>(0x5FDFF0, 0x5FDDD8, 0x60ADD0, 0x698088, 0x698088, 0x697090));
	// ADDRESS
	if(is10())
		InterceptCall(&CVisibilityPlugins::SetAtomicRenderCallback_A, &CVisibilityPlugins::SetAtomicRenderCallback_hook,
		              AddressByVersion<addr>(0x5207C6, 0, 0, 0x579DFB, 0, 0));
	InterceptCall(&RenderScene_A, RenderScene_hook, AddressByVersion<addr>(0x48E5F9, 0x48E6B9, 0x48E649, 0x4A604A, 0x4A606A, 0x4A5F1A));
}

//
// Reflection map
//

void
CarPipe::SetupEnvMap(void)
{
	reflectionMask = RwTextureRead("CarReflectionMask", NULL);

	RwRaster *envFB = RwRasterCreate(envMapSize, envMapSize, 0, rwRASTERTYPECAMERATEXTURE);
	RwRaster *envZB = RwRasterCreate(envMapSize, envMapSize, 0, rwRASTERTYPEZBUFFER);
	reflectionCam = RwCameraCreate();
	RwCameraSetRaster(reflectionCam, envFB);
	RwCameraSetZRaster(reflectionCam, envZB);
	RwCameraSetFrame(reflectionCam, RwFrameCreate());
	RwCameraSetNearClipPlane(reflectionCam, 0.1f);
	RwCameraSetFarClipPlane(reflectionCam, 250.0f);
	RwV2d vw;
	vw.x = vw.y = 0.4f;
	RwCameraSetViewWindow(reflectionCam, &vw);
	RpWorldAddCamera(pRpWorld, reflectionCam);

	reflectionTex = RwTextureCreate(envFB);
	RwTextureSetFilterMode(reflectionTex, rwFILTERLINEAR);

	MakeScreenQuad();
}

void
CarPipe::MakeQuadTexCoords(bool textureSpace)
{
	float minU, minV, maxU, maxV;
	if(textureSpace){
		minU = minV = 0.0f;
		maxU = maxV = 1.0f;
	}else{
		assert(0 && "not implemented");
	}
	screenQuad[0].u = minU;
	screenQuad[0].v = minV;
	screenQuad[1].u = minU;
	screenQuad[1].v = maxV;
	screenQuad[2].u = maxU;
	screenQuad[2].v = maxV;
	screenQuad[3].u = maxU;
	screenQuad[3].v = minV;
}

void
CarPipe::MakeScreenQuad(void)
{
	int width = reflectionTex->raster->width;
	int height = reflectionTex->raster->height;
	screenQuad[0].x = 0.0f;
	screenQuad[0].y = 0.0f;
	screenQuad[0].z = RwIm2DGetNearScreenZ();
	screenQuad[0].rhw = 1.0f / reflectionCam->nearPlane;
	screenQuad[0].emissiveColor = 0xFFFFFFFF;
	screenQuad[1].x = 0.0f;
	screenQuad[1].y = height;
	screenQuad[1].z = screenQuad[0].z;
	screenQuad[1].rhw = screenQuad[0].rhw;
	screenQuad[1].emissiveColor = 0xFFFFFFFF;
	screenQuad[2].x = width;
	screenQuad[2].y = height;
	screenQuad[2].z = screenQuad[0].z;
	screenQuad[2].rhw = screenQuad[0].rhw;
	screenQuad[2].emissiveColor = 0xFFFFFFFF;
	screenQuad[3].x = width;
	screenQuad[3].y = 0;
	screenQuad[3].z = screenQuad[0].z;
	screenQuad[3].rhw = screenQuad[0].rhw;
	screenQuad[3].emissiveColor = 0xFFFFFFFF;
	MakeQuadTexCoords(true);
}

void
CarPipe::RenderReflectionScene(void)
{
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, 0);
	CRenderer__RenderEverythingBarRoads();
	CRenderer__RenderFadingInEntities();
}

void
CarPipe::RenderEnvTex(void)
{
	RwCamera *cam = (RwCamera*)((RwGlobals*)RwEngineInst)->curCamera;
	RwCameraEndUpdate(cam);

	RwV2d oldvw, vw = { 2.0f, 2.0f };
	oldvw = reflectionCam->viewWindow;
	RwCameraSetViewWindow(reflectionCam, &vw);

	static RwMatrix *reflectionMatrix;
	if(reflectionMatrix == NULL){
		reflectionMatrix = RwMatrixCreate();
		reflectionMatrix->right.x = -1.0f;
		reflectionMatrix->right.y = 0.0f;
		reflectionMatrix->right.z = 0.0f;
		reflectionMatrix->up.x = 0.0f;
		reflectionMatrix->up.y = -1.0f;
		reflectionMatrix->up.z = 0.0f;
		reflectionMatrix->at.x = 0.0f;
		reflectionMatrix->at.y = 0.0f;
		reflectionMatrix->at.z = 1.0f;
	}
	RwMatrix *cammatrix = RwFrameGetMatrix(RwCameraGetFrame(cam));
	reflectionMatrix->pos = cammatrix->pos;
	RwMatrixUpdate(reflectionMatrix);
	RwFrameTransform(RwCameraGetFrame(reflectionCam), reflectionMatrix, rwCOMBINEREPLACE);
	RwRGBA color = { skyBotRed, skyBotGreen, skyBotBlue, 255 };
	RwCameraClear(reflectionCam, &color, rwCAMERACLEARIMAGE | rwCAMERACLEARZ);

	RwCameraBeginUpdate(reflectionCam);
	RenderReflectionScene();
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDZERO);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDSRCCOLOR);
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, reflectionMask->raster);
	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, screenQuad, 4, screenindices, 6);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, 0);
	RwCameraEndUpdate(reflectionCam);
	RwCameraSetViewWindow(reflectionCam, &oldvw);

	RwCameraBeginUpdate(cam);
#ifdef DEBUGTEX
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, reflectionTex->raster);
	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, screenQuad, 4, screenindices, 6);
#endif
}

//
//
//

CarPipe::CarPipe(void)
{
	rwPipeline = NULL;
}

void
CarPipe::Init(void)
{
	CreateRwPipeline();
	SetRenderCallback(RenderCallback);
	CreateShaders();
	LoadTweakingTable();
}

void
CarPipe::CreateShaders(void)
{
	HRSRC resource;
	RwUInt32 *shader;
	resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_VEHICLEONEVS), RT_RCDATA);
	shader = (RwUInt32*)LoadResource(dllModule, resource);
	RwD3D9CreateVertexShader(shader, &vertexShaderPass1);
	assert(vertexShaderPass1);
	FreeResource(shader);

	resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_VEHICLETWOVS), RT_RCDATA);
	shader = (RwUInt32*)LoadResource(dllModule, resource);
	RwD3D9CreateVertexShader(shader, &vertexShaderPass2);
	assert(vertexShaderPass2);
	FreeResource(shader);
}

void
CarPipe::LoadTweakingTable(void)
{
	char *path;
	FILE *dat;
	path = getpath("neo\\carTweakingTable.dat");
	if(path == NULL){
		MessageBox(NULL, "Couldn't load 'neo\\carTweakingTable.dat'", "Error", MB_ICONERROR | MB_OK);
		exit(0);
	}
	dat = fopen(path, "r");
	neoReadWeatherTimeBlock(dat, &fresnel);
	neoReadWeatherTimeBlock(dat, &power);
	neoReadWeatherTimeBlock(dat, &diffColor);
	neoReadWeatherTimeBlock(dat, &specColor);
	fclose(dat);
}

//
// Rendering
//

void
UploadLightColorWithSpecular(RpLight *light, int loc)
{
	float c[4];
	if(RpLightGetFlags(light) & rpLIGHTLIGHTATOMICS){
		Color s = CarPipe::specColor.Get();
		c[0] = light->color.red * s.a;
		c[1] = light->color.green * s.a;
		c[2] = light->color.blue * s.a;
		c[3] = 1.0f;
		RwD3D9SetVertexShaderConstant(loc, (void*)c, 1);
	}else
		UploadZero(loc);
}

void
CarPipe::ShaderSetup(RwMatrix *world)
{
	DirectX::XMMATRIX worldMat, viewMat, projMat, texMat;
	RwCamera *cam = (RwCamera*)RWSRCGLOBAL(curCamera);

	RwMatrix view;
	RwMatrixInvert(&view, RwFrameGetLTM(RwCameraGetFrame(cam)));

	RwToD3DMatrix(&worldMat, world);
	RwToD3DMatrix(&viewMat, &view);
	viewMat.r[0] = DirectX::XMVectorNegate(viewMat.r[0]);
	MakeProjectionMatrix(&projMat, cam);

	DirectX::XMMATRIX combined = DirectX::XMMatrixMultiply(projMat, DirectX::XMMatrixMultiply(viewMat, worldMat));
	RwD3D9SetVertexShaderConstant(LOC_combined, (void*)&combined, 4);
	RwD3D9SetVertexShaderConstant(LOC_world, (void*)&worldMat, 4);
	texMat = DirectX::XMMatrixIdentity();
	RwD3D9SetVertexShaderConstant(LOC_tex, (void*)&texMat, 4);

	RwMatrix *camfrm = RwFrameGetLTM(RwCameraGetFrame(cam));
	RwD3D9SetVertexShaderConstant(LOC_eye, (void*)RwMatrixGetPos(camfrm), 1);

	if(pAmbient)
		UploadLightColorWithSpecular(pAmbient, LOC_ambient);
	else
		UploadZero(LOC_ambient);
	if(pDirect){
		UploadLightColorWithSpecular(pDirect, LOC_directCol);
		UploadLightDirection(pDirect, LOC_directDir);
	}else{
		UploadZero(LOC_directCol);
		UploadZero(LOC_directDir);
	}
	for(int i = 0 ; i < 4; i++)
		if(i < NumExtraDirLightsInWorld && RpLightGetType(pExtraDirectionals[i]) == rpLIGHTDIRECTIONAL){
			UploadLightDirection(pExtraDirectionals[i], LOC_lightDir+i);
			UploadLightColorWithSpecular(pExtraDirectionals[i], LOC_lightCol+i);
		}else{
			UploadZero(LOC_lightDir+i);
			UploadZero(LOC_lightCol+i);
		}
	Color spec = specColor.Get();
	spec.r *= spec.a;
	spec.g *= spec.a;
	spec.b *= spec.a;
	RwD3D9SetVertexShaderConstant(LOC_directSpec, (void*)&spec, 1);
}

void
CarPipe::DiffusePass(RxD3D8ResEntryHeader *header)
{
	RxD3D8InstanceData *inst = (RxD3D8InstanceData*)&header[1];

	RwD3D8SetTexture(reflectionTex, 1);
	RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_LERP);
	RwD3D8SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);
	RwD3D8SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	RwD3D8SetTextureStageState(1, D3DTSS_COLORARG0, D3DTA_SPECULAR);
	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_CURRENT);

	RwD3D8SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D8SetTextureStageState(2, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	RwD3D8SetTextureStageState(3, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D8SetTextureStageState(3, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);

	RwD3D9SetVertexShader(vertexShaderPass1);                                      // 9!

	for(int i = 0; i < header->numMeshes; i++){
		RwD3D8SetStreamSource(0, inst->vertexBuffer, inst->stride);
		RwD3D9SetFVF(inst->vertexShader);				       // 9!

		RwD3D8SetTexture(inst->material->texture, 0);
		// have to set these after the texture, RW sets texture stage states automatically
		RwD3D8SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		RwD3D8SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
		RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TEXTURE);
		RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
		RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
		RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TEXTURE);

		Color c = diffColor.Get();
		Color diff(c.r*c.a, c.g*c.a, c.b*c.a, 1.0f-c.a);
		RwRGBAReal mat;
		RwRGBARealFromRwRGBA(&mat, &inst->material->color);
		mat.red = mat.red*diff.a + diff.r;
		mat.green = mat.green*diff.a + diff.g;
		mat.blue = mat.blue*diff.a + diff.b;
		RwD3D9SetVertexShaderConstant(LOC_matCol, (void*)&mat, 1);

		float reflProps[4];
		reflProps[0] = inst->material->surfaceProps.specular;
		reflProps[1] = fresnel.Get();
		reflProps[2] = 0.6f;	// unused
		reflProps[3] = power.Get();
		RwD3D9SetVertexShaderConstant(LOC_reflProps, (void*)reflProps, 1);

		drawDualPass(inst);
		inst++;
	}
}

void
CarPipe::SpecularPass(RxD3D8ResEntryHeader *header)
{
	RwUInt32 src, dst, fog, zwrite;
	RxD3D8InstanceData *inst = (RxD3D8InstanceData*)&header[1];

	RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, &zwrite);
	RwRenderStateGet(rwRENDERSTATEFOGENABLE, &fog);
	RwRenderStateGet(rwRENDERSTATESRCBLEND, &src);
	RwRenderStateGet(rwRENDERSTATEDESTBLEND, &dst);

	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
	RwD3D8SetTexture(NULL, 0);
	RwD3D8SetTexture(NULL, 1);
	RwD3D8SetTexture(NULL, 2);
	RwD3D8SetTexture(NULL, 3);
	RwD3D8SetPixelShader(NULL);                                                    // 8!
	RwD3D9SetVertexShader(vertexShaderPass2);                                      // 9!
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
	for(int i = 0; i < header->numMeshes; i++){
		if(inst->material->surfaceProps.specular != 0.0f){
			RwD3D8SetStreamSource(0, inst->vertexBuffer, inst->stride);
			RwD3D9SetFVF(inst->vertexShader);                              // 9!

			if(inst->indexBuffer){
				RwD3D8SetIndices(inst->indexBuffer, inst->baseIndex);
				RwD3D8DrawIndexedPrimitive(inst->primType, 0, inst->numVertices, 0, inst->numIndices);
			}else
				RwD3D8DrawPrimitive(inst->primType, inst->baseIndex, inst->numVertices);
		}
		inst++;
	}
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)zwrite);
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)fog);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)src);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)dst);
}

void
CarPipe::RenderCallback(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
						{
							static bool keystate = false;
							if(GetAsyncKeyState(xboxcarpipekey) & 0x8000){
								if(!keystate){
									keystate = true;
									xboxcarpipe = (xboxcarpipe+1)%2;
								}
							}else
								keystate = false;
						}
	if(!xboxcarpipe){
		rwD3D8AtomicMatFXRenderCallback(repEntry, object, type, flags);
		return;
	}

	RxD3D8ResEntryHeader *header = (RxD3D8ResEntryHeader*)&repEntry[1];
	ShaderSetup(RwFrameGetLTM(RpAtomicGetFrame((RpAtomic*)object)));
	DiffusePass(header);
	SpecularPass(header);
	RwD3D8SetTexture(NULL, 1);
	RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	RwD3D8SetTexture(NULL, 2);
	RwD3D8SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D8SetTextureStageState(2, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
}
