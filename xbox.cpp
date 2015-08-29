#include "skygfx_vc.h"
#include "d3d8.h"
#include "d3d8types.h"

#define DEBUGKEYS 0
#define DEBUGTEX 0

#define MAXWEATHER 7

static uint32_t CRenderer__RenderEverythingBarRoads_A = AddressByVersion<uint32_t>(0x4A7930, 0x4C9F40);
WRAPPER void CRenderer__RenderEverythingBarRoads(void) { VARJMP(CRenderer__RenderEverythingBarRoads_A); }
static uint32_t CRenderer__RenderFadingInEntities_A = AddressByVersion<uint32_t>(0x4A7910, 0x4CA140);
WRAPPER void CRenderer__RenderFadingInEntities(void) { VARJMP(CRenderer__RenderFadingInEntities_A); }
static uint32_t CTimeCycle__Update_A = AddressByVersion<uint32_t>(0x4ABF40, 0x4CEA40);
WRAPPER void CTimeCycle__Update(void) { VARJMP(CTimeCycle__Update_A); }

static uint32_t rwD3D8RWGetRasterStage_A = AddressByVersion<uint32_t>(0x5B5390, 0x659840);
WRAPPER int rwD3D8RWGetRasterStage(int) { VARJMP(rwD3D8RWGetRasterStage_A); }
static uint32_t rpSkinD3D8CreatePlainPipe_A = AddressByVersion<uint32_t>(0x5E0660, 0x6796D0);
WRAPPER void rpSkinD3D8CreatePlainPipe(void) { VARJMP(rpSkinD3D8CreatePlainPipe_A); }

int &skyBotRed = *AddressByVersion<int*>(0x9414D0, 0xA0D958);
int &skyBotGreen = *AddressByVersion<int*>(0x8F2BD0, 0x97F208);
int &skyBotBlue = *AddressByVersion<int*>(0x8F625C, 0x9B6DF4);

byte &clockHour = *AddressByVersion<byte*>(0x95CDA6, 0xA10B6B);
byte &clockMinute = *AddressByVersion<byte*>(0x95CDC8, 0xA10B92);
byte &clockSecond = *(byte*)0xA10A3C;	// no seconds in III
short &oldWeather = *AddressByVersion<short*>(0x95CCEC, 0xA10AAA);
short &newWeather = *AddressByVersion<short*>(0x95CC70, 0xA10A2E);
float &weatherInterp = *AddressByVersion<float*>(0x8F2520, 0x9787D8);


// car tweak
static float fresnelTable[24][MAXWEATHER], currentFresnel;
static float specPowerTable[24][MAXWEATHER], currentSpecPower;
static RwRGBAReal diffuseTable[24][MAXWEATHER], currentDiffuse;
static RwRGBAReal specularTable[24][MAXWEATHER], currentSpecular;

// rim tweak
static RwRGBAReal rampStartTable[24][MAXWEATHER], currentRampStart;
static RwRGBAReal rampEndTable[24][MAXWEATHER], currentRampEnd;
static float offsetTable[24][MAXWEATHER], currentOffset;
static float scaleTable[24][MAXWEATHER], currentScale;
static float scalingTable[24][MAXWEATHER], currentScaling;

static void *pass1VS = NULL, *pass2VS = NULL;

static RwTexture *envTex, *envMaskTex;
static RwCamera *envCam;
static RwMatrix *envMatrix;

static RwIm2DVertex screenQuad[4];
static RwImVertexIndex screenindices[6] = { 0, 1, 2, 0, 2, 3 };

void
generateEnvTexCoords(bool textureSpace)
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
makeScreenQuad(void)
{
	int width = envTex->raster->width;
	int height = envTex->raster->height;
	screenQuad[0].x = 0.0f;
	screenQuad[0].y = 0.0f;
	screenQuad[0].z = RwIm2DGetNearScreenZ();
	screenQuad[0].rhw = 1.0f / envCam->nearPlane;
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
	generateEnvTexCoords(1);
}

float blerp[4];

float
interploateFloat(float *f1, float *f2)
{
	return f1[oldWeather]*blerp[0] +
	       f2[oldWeather]*blerp[1] +
	       f1[newWeather]*blerp[2] +
	       f2[newWeather]*blerp[3];
}

void
interploateRGBA(RwRGBAReal *out, RwRGBAReal *c1, RwRGBAReal *c2)
{
	out->red   = c1[oldWeather].red*blerp[0] +
	             c2[oldWeather].red*blerp[1] +
	             c1[newWeather].red*blerp[2] +
	             c2[newWeather].red*blerp[3];
	out->green = c1[oldWeather].green*blerp[0] +
	             c2[oldWeather].green*blerp[1] +
	             c1[newWeather].green*blerp[2] +
	             c2[newWeather].green*blerp[3];
	out->blue  = c1[oldWeather].blue*blerp[0] +
	             c2[oldWeather].blue*blerp[1] +
	             c1[newWeather].blue*blerp[2] +
	             c2[newWeather].blue*blerp[3];
	out->alpha = c1[oldWeather].alpha*blerp[0] +
	             c2[oldWeather].alpha*blerp[1] +
	             c1[newWeather].alpha*blerp[2] +
	             c2[newWeather].alpha*blerp[3];
}

void
updateTweakValues(void)
{
	CTimeCycle__Update();
	int nextHour = (clockHour+1)%24;
	float timeInterp = isVC() ? (clockSecond/60.0f + clockMinute)/60.0f : clockMinute/60.0f;
	blerp[0] = (1.0f-timeInterp)*(1.0f-weatherInterp);
	blerp[1] = timeInterp*(1.0f-weatherInterp);
	blerp[2] = (1.0f-timeInterp)*weatherInterp;
	blerp[3] = timeInterp*weatherInterp;

	currentFresnel = interploateFloat(fresnelTable[clockHour], fresnelTable[nextHour]);
	currentSpecPower = interploateFloat(specPowerTable[clockHour], specPowerTable[nextHour]);
	currentOffset = interploateFloat(offsetTable[clockHour], offsetTable[nextHour]);
	currentScale = interploateFloat(scaleTable[clockHour], scaleTable[nextHour]);
	currentScaling = interploateFloat(scalingTable[clockHour], scalingTable[nextHour]);
	interploateRGBA(&currentDiffuse, diffuseTable[clockHour], diffuseTable[nextHour]);
	interploateRGBA(&currentSpecular, specularTable[clockHour], specularTable[nextHour]);
	interploateRGBA(&currentRampStart, rampStartTable[clockHour], rampStartTable[nextHour]);
	interploateRGBA(&currentRampEnd, rampEndTable[clockHour], rampEndTable[nextHour]);
}

void
readFloat(char *s, int line, int field, void *table)
{
	float f;
	float (*t)[24][MAXWEATHER] = (float (*)[24][MAXWEATHER])table;
	sscanf(s, "%f", &f);
	(*t)[line][field]  = f;
}

void
readColor(char *s, int line, int field, void *table)
{
	int r, g, b, a;
	RwRGBAReal (*t)[24][MAXWEATHER] = (RwRGBAReal (*)[24][MAXWEATHER])table;
	sscanf(s, "%d, %d, %d, %d", &r, &g, &b, &a);
	RwRGBAReal c = { r/255.0f, g/255.0f, b/255.0f, a/255.0f };
	(*t)[line][field]  = c;
}

void
readWeatherTimeBlock(FILE *file, void (*func)(char *s, int line, int field, void *table), void *table)
{
	char buf[24], *p;
	int c;
	int line, field;
	int numweather = isIII() ? 4 : 7;

	line = 0;
	c = getc(file);
	while(c != EOF && line < 24){
		field = 0;
		printf("line: %c %d\n", c, c);
		if(c != EOF && c != '#'){
			while(c != EOF && c != '\n' && field < numweather){
				p = buf;
				while(c != EOF && c == '\t')
					c = getc(file);
				*p++ = c;
				while(c = getc(file), c != EOF && c != '\t' && c != '\n')
					*p++ = c;
				*p++ = '\0';
				func(buf, line, field, table);
				field++;
			}
			line++;
		}
		while(c != EOF && c != '\n')
			c = getc(file);
		c = getc(file); // next after newline
	}
	ungetc(c, file);
}

void
readDatFiles(void)
{
	FILE *dat;

	dat = fopen("neo\\carTweakingTable.dat", "r");
	assert(dat && "couldn't load 'neo\\carTweakingTable.dat'");
	readWeatherTimeBlock(dat, readFloat, fresnelTable);
	readWeatherTimeBlock(dat, readFloat, specPowerTable);
	readWeatherTimeBlock(dat, readColor, diffuseTable);
	readWeatherTimeBlock(dat, readColor, specularTable);
	fclose(dat);

	dat = fopen("neo\\rimTweakingTable.dat", "r");
	assert(dat && "couldn't load 'neo\\rimTweakingTable.dat'");
	readWeatherTimeBlock(dat, readColor, rampStartTable);
	readWeatherTimeBlock(dat, readColor, rampEndTable);
	readWeatherTimeBlock(dat, readFloat, offsetTable);
	readWeatherTimeBlock(dat, readFloat, scaleTable);
	readWeatherTimeBlock(dat, readFloat, scalingTable);
	fclose(dat);

	// make it update
	MemoryVP::InjectHook(AddressByVersion<uint32_t>(0x48C9A2, 0x4A45F5), updateTweakValues);
}

void
neoInit(void)
{
	readDatFiles();

	HRSRC resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_VEHICLEONEVS), RT_RCDATA);
	RwUInt32 *shader = (RwUInt32*)LoadResource(dllModule, resource);
	RwD3D9CreateVertexShader(shader, &pass1VS);
	assert(pass1VS);
	FreeResource(shader);

	resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_VEHICLETWOVS), RT_RCDATA);
	shader = (RwUInt32*)LoadResource(dllModule, resource);
	RwD3D9CreateVertexShader(shader, &pass2VS);
	assert(pass2VS);
	FreeResource(shader);

	RwRaster *envFB = RwRasterCreate(128, 128, 0, rwRASTERTYPECAMERATEXTURE);
	RwRaster *envZB = RwRasterCreate(128, 128, 0, rwRASTERTYPEZBUFFER);
	envCam = RwCameraCreate();
	RwCameraSetRaster(envCam, envFB);
	RwCameraSetZRaster(envCam, envZB);
	RwCameraSetFrame(envCam, RwFrameCreate());
	RwCameraSetNearClipPlane(envCam, 0.1f);
	RwCameraSetFarClipPlane(envCam, 250.0f);
	RwV2d vw;
	vw.x = vw.y = 0.4f;
	RwCameraSetViewWindow(envCam, &vw);

	envTex = RwTextureCreate(envFB);
	RwTextureSetFilterMode(envTex, rwFILTERLINEAR);

	RwImage *envMaskI = RtBMPImageRead("neo\\CarReflectionMask.bmp");
	assert(envMaskI);
	RwInt32 width, height, depth, format;
	RwImageFindRasterFormat(envMaskI, 4, &width, &height, &depth, &format);
	RwRaster *envMask = RwRasterCreate(width, height, depth, format);
	RwRasterSetFromImage(envMask, envMaskI);
	envMaskTex = RwTextureCreate(envMask);
	RwImageDestroy(envMaskI);

	envMatrix = RwMatrixCreate();
	envMatrix->right.x = -1.0f;
	envMatrix->right.y = 0.0f;
	envMatrix->right.z = 0.0f;
	envMatrix->up.x = 0.0f;
	envMatrix->up.y = -1.0f;
	envMatrix->up.z = 0.0f;
	envMatrix->at.x = 0.0f;
	envMatrix->at.y = 0.0f;
	envMatrix->at.z = 1.0f;

	makeScreenQuad();
}

void
RenderReflectionScene(void)
{
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, 0);
	CRenderer__RenderEverythingBarRoads();
	CRenderer__RenderFadingInEntities();
}

void
RenderEnvTex(void)
{
	RwCamera *cam = (RwCamera*)((RwGlobals*)RwEngineInst)->curCamera;
	if(envTex == NULL)
		neoInit();

	RwCameraEndUpdate(cam);

	RwV2d oldvw, vw = { 2.0f, 2.0f };
	oldvw = envCam->viewWindow;
	RwCameraSetViewWindow(envCam, &vw);
	RwMatrix *cammatrix = RwFrameGetMatrix(RwCameraGetFrame(cam));
	envMatrix->pos = cammatrix->pos;
	RwMatrixUpdate(envMatrix);
	RwFrameTransform(RwCameraGetFrame(envCam), envMatrix, rwCOMBINEREPLACE);
	RwRGBA color = { skyBotRed, skyBotGreen, skyBotBlue, 255 };
	RwCameraClear(envCam, &color, rwCAMERACLEARIMAGE | rwCAMERACLEARZ);

	RwCameraBeginUpdate(envCam);
	RenderReflectionScene();
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDZERO);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDSRCCOLOR);
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, envMaskTex->raster);
	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, screenQuad, 4, screenindices, 6);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, 0);
	RwCameraEndUpdate(envCam);
	RwCameraSetViewWindow(envCam, &oldvw);

	RwCameraBeginUpdate(cam);
	if(DEBUGTEX){
		RwRenderStateSet(rwRENDERSTATETEXTURERASTER, envTex->raster);
		RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, screenQuad, 4, screenindices, 6);
	}
}


void
uploadLightColor(RwUInt32 address, RpLight *light, float f)
{
	RwRGBAReal col = light->color;
	col.red *= f;
	col.green *= f;
	col.blue *= f;
	col.alpha = 1.0f;
	RwD3D9SetVertexPixelShaderConstant(address, (void*)&col, 1);
}

void
uploadConstants(float f)
{
	D3DMATRIX worldMat, viewMat, projMat;

	RwD3D8GetTransform(D3DTS_WORLD, &worldMat);
	RwD3D8GetTransform(D3DTS_VIEW, &viewMat);
	RwD3D8GetTransform(D3DTS_PROJECTION, &projMat);
	RwD3D9SetVertexPixelShaderConstant(LOC_world, (void*)&worldMat, 4);
	RwD3D9SetVertexPixelShaderConstant(LOC_worldIT, (void*)&worldMat, 4);
	RwD3D9SetVertexPixelShaderConstant(LOC_view, (void*)&viewMat, 4);
	RwD3D9SetVertexPixelShaderConstant(LOC_proj, (void*)&projMat, 4);

	RwMatrix *camfrm = RwFrameGetLTM(RwCameraGetFrame((RwCamera*)((RwGlobals*)RwEngineInst)->curCamera));
	RwD3D9SetVertexPixelShaderConstant(LOC_eye, (void*)RwMatrixGetPos(camfrm), 1);

	uploadLightColor(LOC_ambient, pAmbient, f);
	RwD3D9SetVertexPixelShaderConstant(LOC_directDir, (void*)RwMatrixGetAt(RwFrameGetLTM(RpLightGetFrame(pDirect))), 1);
	uploadLightColor(LOC_directDiff, pDirect, f);
	RwD3D9SetVertexPixelShaderConstant(LOC_directSpec, (void*)&currentSpecular, 1);
	int i = 0;
	for(i = 0 ; i < NumExtraDirLightsInWorld; i++){
		RwD3D9SetVertexPixelShaderConstant(LOC_lights+i*2, (void*)RwMatrixGetAt(RwFrameGetLTM(RpLightGetFrame(pExtraDirectionals[i]))), 1);
		uploadLightColor(LOC_lights+i*2+1, pExtraDirectionals[i], f);
	}
	static float zero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	for(; i < 4; i++){
		RwD3D9SetVertexPixelShaderConstant(LOC_lights+i*2, (void*)zero, 1);
		RwD3D9SetVertexPixelShaderConstant(LOC_lights+i*2+1, (void*)zero, 1);
	}
}

int
rpMatFXD3D8AtomicMatFXEnvRender_xbox(RxD3D8InstanceData *inst, int flags, int sel, RwTexture *texture, RwTexture *envMap)
{
	MatFX *matfx = *RWPLUGINOFFSET(MatFX*, inst->material, MatFXMaterialDataOffset);
	MatFXEnv *env = &matfx->fx[sel];
	float factor = env->envCoeff*255.0f;
	RwUInt8 intens = factor;
	RpMaterial *m = inst->material;

	if(inst->vertexAlpha || inst->material->color.alpha != 0xFFu){
		if(!rwD3D8RenderStateIsVertexAlphaEnable())
			rwD3D8RenderStateVertexAlphaEnable(1);
	}else{
		if(rwD3D8RenderStateIsVertexAlphaEnable())
			rwD3D8RenderStateVertexAlphaEnable(0);
	}
	if(flags & 0x84 && texture)
		RwD3D8SetTexture(texture, 0);
	else
		RwD3D8SetTexture(NULL, 0);

	RwD3D8SetTexture(envTex, 1);
	RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MULTIPLYADD);
	RwD3D8SetTextureStageState(1, D3DTSS_COLORARG0, D3DTA_CURRENT);
	RwD3D8SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	RwD3D8SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_SPECULAR);
	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_CURRENT);

	RwD3D9SetFVF(inst->vertexShader);
	RwD3D9SetVertexShader(pass1VS);
	RwD3D9SetPixelShader(NULL);

	uploadConstants(1.0f);
	float reflProps[4];
	reflProps[0] = m->surfaceProps.specular;
	if(DEBUGKEYS && GetAsyncKeyState(VK_F5) & 0x8000)
		reflProps[0] = 0.0;
	reflProps[1] = currentSpecPower;
	reflProps[2] = currentFresnel;
	RwD3D9SetVertexPixelShaderConstant(LOC_surfProps, (void*)&reflProps, 1);
	RwRGBAReal color;
	RwRGBARealFromRwRGBA(&color, &m->color);
	if(DEBUGKEYS && GetAsyncKeyState(VK_F4) & 0x8000)
		color.red = color.green = color.blue = 0.0f;
	RwD3D9SetVertexPixelShaderConstant(LOC_matCol, (void*)&color, 1);

	RwD3D8SetStreamSource(0, inst->vertexBuffer, inst->stride);
	RwD3D8SetIndices(inst->indexBuffer, inst->baseIndex);

	if(inst->indexBuffer)
		RwD3D8DrawIndexedPrimitive(inst->primType, 0, inst->numVertices, 0, inst->numIndices);
	else
		RwD3D8DrawPrimitive(inst->primType, inst->baseIndex, inst->numVertices);

	RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

	if(DEBUGKEYS && GetAsyncKeyState(VK_F6) & 0x8000)
		return 0;

	// pass two (specular)
	RwD3D8SetTexture(NULL, 1);
	if(!rwD3D8RenderStateIsVertexAlphaEnable())
		rwD3D8RenderStateVertexAlphaEnable(1);
	RwUInt32 dst;
	RwRenderStateGet(rwRENDERSTATEDESTBLEND, &dst);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
	RwD3D8SetTexture(NULL, 0);
	RwD3D9SetVertexShader(pass2VS);
	if(inst->indexBuffer)
		RwD3D8DrawIndexedPrimitive(inst->primType, 0, inst->numVertices, 0, inst->numIndices);
	else
		RwD3D8DrawPrimitive(inst->primType, inst->baseIndex, inst->numVertices);

	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)dst);
	RwD3D9SetVertexShader(NULL);
	RwD3D9SetPixelShader(NULL);
	return 0;
}


//
// Skin
//

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
	RwD3D9SetVertexPixelShaderConstant(LOC_rampStart, (void*)&currentRampStart, 1);
	RwD3D9SetVertexPixelShaderConstant(LOC_rampEnd, (void*)&currentRampEnd, 1);
	float rim[4] = { currentOffset, currentScale, currentScaling, 0.0f };
	RwD3D9SetVertexPixelShaderConstant(LOC_rim, (void*)&rim, 1);

	int alpha = rwD3D8RenderStateIsVertexAlphaEnable();
	int bar = -1;
	RxD3D8ResEntryHeader *header = (RxD3D8ResEntryHeader*)&repEntry[1];
	RxD3D8InstanceData *inst = (RxD3D8InstanceData*)&header[1];

	RwD3D9SetFVF(inst->vertexShader);
	RwD3D9SetVertexShader(rimVS);
	RwD3D9SetPixelShader(NULL);

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
		if(DEBUGKEYS && GetAsyncKeyState(VK_F6) & 0x8000)
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
	if(RwD3D9Supported()){
		HRSRC resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_RIMVS), RT_RCDATA);
		RwUInt32 *shader = (RwUInt32*)LoadResource(dllModule, resource);
		RwD3D9CreateVertexShader(shader, &rimVS);
		assert(rimVS);
		FreeResource(shader);
		MemoryVP::Patch(AddressByVersion<uint32_t>(0x5E0649, 0x6796B9), rxD3D8SkinRenderCallback);
	}
	rpSkinD3D8CreatePlainPipe();
}
