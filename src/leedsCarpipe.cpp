#include "skygfx.h"
#include "d3d8.h"
#include "d3d8types.h"
#include <DirectXMath.h>
#include "debugmenu_public.h"

#define DEBUGTEX

static int debugEnvMap = 0;
static int enableEnv = 1;

// ADDRESS
int &skyTopRed = *AddressByVersion<int*>(0x9403C0, 0, 0, 0xA0CE98, 0, 0);
int &skyTopGreen = *AddressByVersion<int*>(0x943074, 0, 0, 0xA0FD70, 0, 0);
int &skyTopBlue = *AddressByVersion<int*>(0x8F29B8, 0, 0, 0x978D1C, 0, 0);
extern int &skyBotRed;
extern int &skyBotGreen;
extern int &skyBotBlue;


// reflection map
RwCamera *LeedsCarPipe::reflectionCam;
RwTexture *LeedsCarPipe::reflectionTex;
RwIm2DVertex LeedsCarPipe::screenQuad[4];
RwImVertexIndex LeedsCarPipe::screenindices[6] = { 0, 1, 2, 0, 2, 3 };

void *LeedsCarPipe::vertexShader;

//LeedsCarPipe leedsCarpipe;

//
// Reflection map
//

void
LeedsCarPipe::SetupEnvMap(void)
{
	RwRaster *envFB = RwRasterCreate(config.envMapSize, config.envMapSize, 0, rwRASTERTYPECAMERATEXTURE);
	RwRaster *envZB = RwRasterCreate(config.envMapSize, config.envMapSize, 0, rwRASTERTYPEZBUFFER);
	reflectionCam = RwCameraCreate();
	RwCameraSetRaster(reflectionCam, envFB);
	RwCameraSetZRaster(reflectionCam, envZB);
	RwCameraSetFrame(reflectionCam, RwFrameCreate());
	RwCameraSetNearClipPlane(reflectionCam, 0.1f);
	RwCameraSetFarClipPlane(reflectionCam, 250.0f);
	RwV2d vw;
	vw.x = vw.y = 0.4f;
	RwCameraSetViewWindow(reflectionCam, &vw);
	RpWorldAddCamera(Scene.world, reflectionCam);

	reflectionTex = RwTextureCreate(envFB);
	RwTextureSetFilterMode(reflectionTex, rwFILTERLINEAR);

	MakeScreenQuad();
}

void
LeedsCarPipe::MakeQuadTexCoords(bool textureSpace)
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
LeedsCarPipe::MakeScreenQuad(void)
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
LeedsCarPipe::RenderReflectionScene(void)
{
//	RwRenderStateSet(rwRENDERSTATEFOGENABLE, 0);
//	CRenderer__RenderRoads();
//	CRenderer__RenderEverythingBarRoads();	
//	CRenderer__RenderFadingInEntities();
	CClouds__RenderBackground(skyTopRed, skyTopGreen, skyTopBlue,
		skyBotRed, skyBotGreen, skyBotBlue, 255);
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)1);
	RenderEveryBarCarsPeds();
	RenderAlphaListBarCarsPeds();
}

void
LeedsCarPipe::RenderEnvTex(void)
{
	RwCamera *cam = (RwCamera*)((RwGlobals*)RwEngineInst)->curCamera;
	RwCameraEndUpdate(cam);

	RwCameraSetViewWindow(reflectionCam, &cam->viewWindow);

	RwFrameTransform(RwCameraGetFrame(reflectionCam), &RwCameraGetFrame(cam)->ltm, rwCOMBINEREPLACE);
	RwRGBA color = { skyTopRed, skyTopGreen, skyTopBlue, 255 };
	RwCameraClear(reflectionCam, &color, rwCAMERACLEARIMAGE | rwCAMERACLEARZ);

	RwCameraBeginUpdate(reflectionCam);
	RenderReflectionScene();
	RwCameraEndUpdate(reflectionCam);

	RwCameraBeginUpdate(cam);
#ifdef DEBUGTEX
	if(debugEnvMap){
		RwRenderStateSet(rwRENDERSTATETEXTURERASTER, reflectionTex->raster);
		RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, screenQuad, 4, screenindices, 6);
	}
#endif
}

//
//
//

LeedsCarPipe::LeedsCarPipe(void)
{
	rwPipeline = NULL;
}

LeedsCarPipe*
LeedsCarPipe::Get(void)
{
	static LeedsCarPipe pipe;
	return &pipe;
}

void
LeedsCarPipe::Init(void)
{
	CreateRwPipeline();
	SetRenderCallback(RenderCallback);
//	CreateShaders();

	SetupEnvMap();
}

#if 0
void
LeedsCarPipe::CreateShaders(void)
{
	HRSRC resource;
	RwUInt32 *shader;
	resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_VCSVEHICLEVS), RT_RCDATA);
	shader = (RwUInt32*)LoadResource(dllModule, resource);
	RwD3D9CreateVertexShader(shader, &vertexShader);
	assert(vertexShader);
	FreeResource(shader);
}

//
// Rendering
//

void
LeedsCarPipe::ShaderSetup(RwMatrix *world)
{
	DirectX::XMMATRIX worldMat, viewMat, projMat, texMat;
	RwCamera *cam = (RwCamera*)RWSRCGLOBAL(curCamera);

	RwMatrix view;
	RwMatrixInvert(&view, RwFrameGetLTM(RwCameraGetFrame(cam)));

	RwToD3DMatrix(&worldMat, world);
	RwToD3DMatrix(&viewMat, &view);
	texMat = viewMat;
	viewMat.r[0] = DirectX::XMVectorNegate(viewMat.r[0]);
	MakeProjectionMatrix(&projMat, cam);

	DirectX::XMMATRIX combined = DirectX::XMMatrixMultiply(projMat, DirectX::XMMatrixMultiply(viewMat, worldMat));
	RwD3D9SetVertexShaderConstant(LOC_combined, (void*)&combined, 4);
	RwD3D9SetVertexShaderConstant(LOC_world, (void*)&worldMat, 4);
//	texMat = DirectX::XMMatrixIdentity();
	RwD3D9SetVertexShaderConstant(LOC_tex, (void*)&texMat, 4);

	RwMatrix *camfrm = RwFrameGetLTM(RwCameraGetFrame(cam));
	RwD3D9SetVertexShaderConstant(LOC_eye, (void*)RwMatrixGetPos(camfrm), 1);

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
}

void
LeedsCarPipe::DiffusePass(RxD3D8ResEntryHeader *header)
{
	RxD3D8InstanceData *inst = (RxD3D8InstanceData*)&header[1];

	RwD3D8SetTexture(reflectionTex, 1);

	// ARG0 + ARG1*ARG2
	RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MULTIPLYADD);
	RwD3D8SetTextureStageState(1, D3DTSS_COLORARG0, D3DTA_CURRENT);
	RwD3D8SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TFACTOR);
	RwD3D8SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_TEXTURE);

	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_CURRENT);
//RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
//RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

	RwD3D8SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D8SetTextureStageState(2, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	RwD3D8SetTextureStageState(3, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D8SetTextureStageState(3, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);

	RwD3D9SetVertexShader(vertexShader);                                      // 9!

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

		RwRGBAReal mat;
		RwRGBARealFromRwRGBA(&mat, &inst->material->color);
		RwD3D9SetVertexShaderConstant(LOC_matCol, (void*)&mat, 1);

		float envcoeff = 0.0f;
		if(enableEnv && inst->material->surfaceProps.specular)
		//	envcoeff = 0.3f;
			envcoeff = 0.12f;	// LCS default it would seem, but perhaps depends on vehicle
		int t = envcoeff*128;
		RwD3D8SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_ARGB(255, t, t, t));

		drawDualPass(inst);
		inst++;
	}
}
#endif

extern IDirect3DDevice8 *&RwD3DDevice;
extern D3DMATERIAL8 d3dmaterial;
extern D3DMATERIAL8 &lastmaterial;
static int lightingEnabled;	// d3d lighting on
static int setMaterial;	// set d3d material, needed for lighting
// these two are mutually exclusive:
static int setMaterialColor;	// multiply material color at vertex stage
static int modulateMaterial;	// modulate by material color at texture stage

static D3DMATRIX envtexmat;

void
leedsCarRenderFFPObjectSetUp(RwUInt32 flags)
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
			RwD3D8SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_COLOR1);
		}else{
			RwD3D8SetRenderState(D3DRS_COLORVERTEX, 0);
			RwD3D8SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL);
		}
	}
	RwD3D8SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL);	// not found in xbox code for some reason
}

void
leedsCarRenderFFPMeshSetUp(RxD3D8InstanceData *inst)
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
	// normally done in rxD3D8DefaultRenderFFPObjectSetUp, but we don't know vertexAlpha for the whole geomtetry
	if(lightingEnabled)
		RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, inst->vertexAlpha ? D3DMCS_COLOR1 : D3DMCS_MATERIAL);
}

void
leedsCarRenderFFPMeshCombinerSetUp(RxD3D8InstanceData *inst, RwUInt32 flags)
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

	// Not supported because we needs the second stage for the envmap
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
leedsCarRenderFFPMesh(RxD3D8InstanceData *inst, RwUInt32 flags)
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
	leedsCarRenderFFPMeshCombinerSetUp(inst, flags);
	drawDualPass(inst);
}

void
leedsCarRenderFFPEnvMesh(RxD3D8InstanceData *inst, RwUInt32 flags)
{
	int fog, zwrite;

	// REMOVE: we do this in InitialiseGame_hook now
	if(config.leedsEnvMult == -9999.0f)
		config.leedsEnvMult = isIII() ? 0.22 : 0.3;

	RwRenderStateGet(rwRENDERSTATEFOGENABLE, &fog);
	RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, &zwrite);

	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);

	RwD3D8SetTexture(LeedsCarPipe::reflectionTex, 0);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);

	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);


	float envcoeff = 0.0f;
	MatFXEnv *env = getEnvData(inst->material);
	if(enableEnv && env && env->envCoeff)
		envcoeff = env->envCoeff*config.leedsEnvMult;
	int t = envcoeff*128;
	RwD3D8SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_ARGB(255, t, t, t));

	RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

	RwD3D8SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	RwD3D8SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
	RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TEXTURE);

	RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

	RwD3D8SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACENORMAL);
	RwD3D8SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
	RwD3D8SetTransform(D3DTS_TEXTURE0, &envtexmat);

	if(inst->indexBuffer){
		RwD3D8SetIndices(inst->indexBuffer, inst->baseIndex);
		RwD3D8DrawIndexedPrimitive(inst->primType, 0, inst->numVertices, 0, inst->numIndices);
	}else
		RwD3D8DrawPrimitive(inst->primType, inst->baseIndex, inst->numVertices);

	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);

	RwD3D8SetTexture(NULL, 0);
	RwD3D8SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	RwD3D8SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
	RwD3D8SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)fog);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)zwrite);
}

void
leedsCarRenderCallback(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
	envtexmat.m[0][0] = 0.5f;
	envtexmat.m[0][1] = 0.0f;
	envtexmat.m[0][2] = 0.0f;
	envtexmat.m[0][3] = 0.0f;

	envtexmat.m[1][0] = 0.0f;
	envtexmat.m[1][1] = -0.5f;
	envtexmat.m[1][2] = 0.0f;
	envtexmat.m[1][3] = 0.0f;

	envtexmat.m[2][0] = 0.0f;
	envtexmat.m[2][1] = 0.0f;
	envtexmat.m[2][2] = 0.0f;
	envtexmat.m[2][3] = 0.0f;

	envtexmat.m[3][0] = 0.5f;
	envtexmat.m[3][1] = 0.5f;
	envtexmat.m[3][2] = 0.0f;
	envtexmat.m[3][3] = 1.0f;

	rxD3D8SetAmbientLight();

	RxD3D8ResEntryHeader *header = (RxD3D8ResEntryHeader*)&repEntry[1];
	RxD3D8InstanceData *inst = (RxD3D8InstanceData*)&header[1];

/*
	RwD3D8SetTexture(LeedsCarPipe::reflectionTex, 1);

	// ARG0 + ARG1*ARG2
	RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MULTIPLYADD);
	RwD3D8SetTextureStageState(1, D3DTSS_COLORARG0, D3DTA_CURRENT);
	RwD3D8SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TFACTOR);
	RwD3D8SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_TEXTURE);

	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_CURRENT);

	RwD3D8SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACENORMAL);
	RwD3D8SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
	RwD3D8SetTransform(D3DTS_TEXTURE1, &envtexmat);
*/

	leedsCarRenderFFPObjectSetUp(flags);

	modulateMaterial |= setMaterialColor;
	setMaterialColor = 0;

	for(int i = 0; i < header->numMeshes; i++){
		leedsCarRenderFFPMeshSetUp(inst);
		leedsCarRenderFFPMesh(inst, flags);
		rxD3D8DefaultRenderFFPMeshCombinerTearDown();

		RwD3D8SetRenderState(D3DRS_LIGHTING, 0);
		leedsCarRenderFFPEnvMesh(inst, flags);
		RwD3D8SetRenderState(D3DRS_LIGHTING, lightingEnabled);

		inst++;
	}
	RwD3D8SetTexture(NULL, 1);
	RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
//	RwD3D8SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 0);
//	RwD3D8SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
}



void
LeedsCarPipe::RenderCallback(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
//	if(GetAsyncKeyState(VK_TAB) & 0x8000){
//
//	_rwD3D8EnableClippingIfNeeded(object, type);
//
//	RxD3D8ResEntryHeader *header = (RxD3D8ResEntryHeader*)&repEntry[1];
//	ShaderSetup(RwFrameGetLTM(RpAtomicGetFrame((RpAtomic*)object)));
//	DiffusePass(header);
//	RwD3D8SetTexture(NULL, 1);
//	RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
//	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
//	RwD3D8SetTexture(NULL, 2);
//	RwD3D8SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_DISABLE);
//	RwD3D8SetTextureStageState(2, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
//	}else
	leedsCarRenderCallback(repEntry, object, type, flags);

}
