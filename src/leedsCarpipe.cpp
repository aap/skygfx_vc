#include "skygfx.h"
#include "d3d8.h"
#include "d3d8types.h"
#include <DirectXMath.h>
#include "debugmenu_public.h"

//#define DEBUGTEX

static int debugEnvMap = 0;
static int enableEnv = 1;
static int chromeCheat = 0;

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

#ifdef DEBUGTEX
	MakeScreenQuad();
#endif
}

#ifdef DEBUGTEX
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
#endif

void
LeedsCarPipe::RenderReflectionScene(void)
{
//	RwRenderStateSet(rwRENDERSTATEFOGENABLE, 0);
//	CRenderer__RenderRoads();
//	CRenderer__RenderEverythingBarRoads();	
//	CRenderer__RenderFadingInEntities();

	CClouds__RenderBackground(skyTopRed, skyTopGreen, skyTopBlue,
		skyBotRed, skyBotGreen, skyBotBlue, 255);
	CClouds__Render();
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)1);
	RenderEverythingBarCarsPeds();
	RenderAlphaListBarCarsPeds();
}

RwTexture **gpCoronaTexture = (RwTexture**)AddressByVersion<int*>(0x5FAF44, 0, 0, 0x695538, 0, 0);
static uint32_t CGeneral__GetATanOfXY_A = AddressByVersion<uint32_t>(0x48CC30, 0, 0, 0x4A55E0, 0, 0);
WRAPPER double CGeneral__GetATanOfXY(float x, float y) { VARJMP(CGeneral__GetATanOfXY_A); }

static RwIm2DVertex coronaVerts[4*4];
static RwImVertexIndex coronaIndices[6*4];
static int numCoronaVerts, numCoronaIndices;

static void
AddCorona(float x, float y, float sz)
{
	float nearz, recipz;
	RwIm2DVertex *v;
	nearz = RwIm2DGetNearScreenZ();
	recipz = 1.0f / RwCameraGetNearClipPlane((RwCamera*)RWSRCGLOBAL(curCamera));

	v = &coronaVerts[numCoronaVerts];
	RwIm2DVertexSetScreenX(&v[0], x);
	RwIm2DVertexSetScreenY(&v[0], y);
	RwIm2DVertexSetScreenZ(&v[0], nearz);
	RwIm2DVertexSetRecipCameraZ(&v[0], recipz);
	RwIm2DVertexSetU(&v[0], 0.0f, recipz);
	RwIm2DVertexSetV(&v[0], 0.0f, recipz);
	RwIm2DVertexSetIntRGBA(&v[0], 0xFF, 0xFF, 0xFF, 0xFF);

	RwIm2DVertexSetScreenX(&v[1], x);
	RwIm2DVertexSetScreenY(&v[1], y + sz);
	RwIm2DVertexSetScreenZ(&v[1], nearz);
	RwIm2DVertexSetRecipCameraZ(&v[1], recipz);
	RwIm2DVertexSetU(&v[1], 0.0f, recipz);
	RwIm2DVertexSetV(&v[1], 1.0f, recipz);
	RwIm2DVertexSetIntRGBA(&v[1], 0xFF, 0xFF, 0xFF, 0xFF);

	RwIm2DVertexSetScreenX(&v[2], x + sz);
	RwIm2DVertexSetScreenY(&v[2], y + sz);
	RwIm2DVertexSetScreenZ(&v[2], nearz);
	RwIm2DVertexSetRecipCameraZ(&v[2], recipz);
	RwIm2DVertexSetU(&v[2], 1.0f, recipz);
	RwIm2DVertexSetV(&v[2], 1.0f, recipz);
	RwIm2DVertexSetIntRGBA(&v[2], 0xFF, 0xFF, 0xFF, 0xFF);

	RwIm2DVertexSetScreenX(&v[3], x + sz);
	RwIm2DVertexSetScreenY(&v[3], y);
	RwIm2DVertexSetScreenZ(&v[3], nearz);
	RwIm2DVertexSetRecipCameraZ(&v[3], recipz);
	RwIm2DVertexSetU(&v[3], 1.0f, recipz);
	RwIm2DVertexSetV(&v[3], 0.0f, recipz);
	RwIm2DVertexSetIntRGBA(&v[3], 0xFF, 0xFF, 0xFF, 0xFF);


	coronaIndices[numCoronaIndices++] = numCoronaVerts;
	coronaIndices[numCoronaIndices++] = numCoronaVerts + 1;
	coronaIndices[numCoronaIndices++] = numCoronaVerts + 2;
	coronaIndices[numCoronaIndices++] = numCoronaVerts;
	coronaIndices[numCoronaIndices++] = numCoronaVerts + 2;
	coronaIndices[numCoronaIndices++] = numCoronaVerts + 3;
	numCoronaVerts += 4;
}

void
DrawEnvMapCoronas(RwV3d at)
{
	const float BIG = 89.0f * LeedsCarPipe::reflectionTex->raster->width/128.0f;
	const float SMALL = 38.0f * LeedsCarPipe::reflectionTex->raster->height/128.0f;

	float x;
	numCoronaVerts = 0;
	numCoronaIndices = 0;
	x = CGeneral__GetATanOfXY(-at.y, at.x)/(2*M_PI) - 1.0f;
	x *= BIG+SMALL;
	AddCorona(x, 12.0f, SMALL);	x += SMALL;
	AddCorona(x, 0.0f, BIG);	x += BIG;
	AddCorona(x, 12.0f, SMALL);	x += SMALL;
	AddCorona(x, 0.0f, BIG);	x += BIG;

	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, gpCoronaTexture[0]->raster);
	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, coronaVerts, numCoronaVerts, coronaIndices, numCoronaIndices);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)FALSE);
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
	DrawEnvMapCoronas(RwFrameGetLTM(RwCameraGetFrame(reflectionCam))->at);
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

extern IDirect3DDevice8 *&RwD3DDevice;
extern D3DMATERIAL8 d3dmaterial;
extern D3DMATERIAL8 &lastmaterial;
static int lightingEnabled;	// d3d lighting on
static int setMaterial;	// set d3d material, needed for lighting
// these two are mutually exclusive:
static int setMaterialColor;	// multiply material color at vertex stage
static int modulateMaterial;	// modulate by material color at texture stage

void
GetLeedsEnvMap(RpAtomic *atm, float *envmat)
{
	static RwMatrix scalenormal_flipU = {
		{ -0.5f, 0.0f, 0.0f }, 0,
		{ 0.0f, -0.5f, 0.0f }, 0,
		{ 0.0f, 0.0f, 1.0f }, 0,
		{ 0.5f, 0.5f, 0.0f }, 0,	
	};

	RwMatrix tmpmat, cammat, envinv;
	RwMatrix env;

	RwMatrix *camfrm = RwFrameGetLTM(RwCameraGetFrame(RwCameraGetCurrentCamera()));
	// Matrix to get normals from camera to world space
	memcpy(&cammat, camfrm, sizeof(cammat));
	cammat.pos.x = 0.0f;
	cammat.pos.y = 0.0f;
	cammat.pos.z = 0.0f;
	cammat.right.x = -cammat.right.x;
	cammat.right.y = -cammat.right.y;
	cammat.right.z = -cammat.right.z;
	cammat.flags = rwMATRIXTYPEORTHONORMAL;

	// Now back to camera space but kill pitch
	// TODO: do the inversion in place here?
	env.pos.x = 0.0f;
	env.pos.y = 0.0f;
	env.pos.z = 0.0f;
	env.at.x = camfrm->at.x;
	env.at.y = camfrm->at.y;
	env.at.z = 0.0f;
	float invlen = 1.0f/sqrt(env.at.x*env.at.x + env.at.y*env.at.y);
	env.at.x *= invlen;
	env.at.y *= invlen;
	env.up.x = 0.0f;
	env.up.y = 0.0f;
	env.up.z = 1.0f;
	env.right.x = -env.at.y;
	env.right.y = env.at.x;
	env.right.z = 0;
	env.flags = rwMATRIXTYPEORTHONORMAL;
	RwMatrixInvert(&envinv, &env);

	RwMatrixMultiply(&tmpmat, &cammat, &envinv);
	RwMatrixMultiply((RwMatrix*)envmat, &tmpmat, &scalenormal_flipU);
	envmat[3] = 0.0f;
	envmat[7] = 0.0f;
	envmat[11] = 0.0f;
	envmat[15] = 1.0f;
}

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

	// Not supported because we need the second stage for the envmap
	if(modulateMaterial){
		RwUInt32 tf;
		RwRGBA *c = &inst->material->color;
		// this happens when the geometry is lit but there were no lights
		if(!lightingEnabled &&
//		   flags & rpGEOMETRYLIGHT &&	// we don't really want this, unlit geometry should be black too
		   !(flags & rpGEOMETRYPRELIT)){
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

	RwRenderStateGet(rwRENDERSTATEFOGENABLE, &fog);
	RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, &zwrite);

	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);

	RwD3D8SetTexture(LeedsCarPipe::reflectionTex, 0);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);

	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
	if(chromeCheat || config.carPipeSwitch == CAR_VCS)
		RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
	else
		RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);


	float envcoeff = 0.0f;
	MatFXEnv *env = getEnvData(inst->material);
	if(enableEnv && env && env->envCoeff)
		envcoeff = env->envCoeff*config.leedsEnvMult;
	int t = envcoeff*128;
	if(chromeCheat)
		t = 255;
	RwD3D8SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_ARGB(t, 255, 255, 255));

	RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

	RwD3D8SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	RwD3D8SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
	RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TEXTURE);

	RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
	RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TEXTURE);

	RwD3D8SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACENORMAL);
	RwD3D8SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);

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
	_rwD3D8EnableClippingIfNeeded(object, type);

	rxD3D8SetAmbientLight();

	RxD3D8ResEntryHeader *header = (RxD3D8ResEntryHeader*)&repEntry[1];
	RxD3D8InstanceData *inst = (RxD3D8InstanceData*)&header[1];


	float envmat[16];
	GetLeedsEnvMap((RpAtomic*)object, envmat);
	RwD3D8SetTransform(D3DTS_TEXTURE0, envmat);

	leedsCarRenderFFPObjectSetUp(flags);

	modulateMaterial |= setMaterialColor;
	setMaterialColor = 0;

	for(int i = 0; i < header->numMeshes; i++){
		leedsCarRenderFFPMeshSetUp(inst);
		leedsCarRenderFFPMesh(inst, flags);
		rxD3D8DefaultRenderFFPMeshCombinerTearDown();

		if(lightingEnabled){
			RwD3D8SetRenderState(D3DRS_LIGHTING, 0);
			leedsCarRenderFFPEnvMesh(inst, flags);
			RwD3D8SetRenderState(D3DRS_LIGHTING, lightingEnabled);
		}

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
	leedsCarRenderCallback(repEntry, object, type, flags);

}
