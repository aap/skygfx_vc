#include "skygfx.h"
#include "d3d8.h"
#include "d3d8types.h"

#define DEBUGKEYS 0
#define DEBUGTEX 0

#define MAXWEATHER 7

static uint32_t CRenderer__RenderEverythingBarRoads_A = AddressByVersion<uint32_t>(0x4A7930, 0x4A7A20, 0x4A79B0, 0x4C9F40, 0x4C9F60, 0x4C9E00);
WRAPPER void CRenderer__RenderEverythingBarRoads(void) { VARJMP(CRenderer__RenderEverythingBarRoads_A); }
static uint32_t CRenderer__RenderFadingInEntities_A = AddressByVersion<uint32_t>(0x4A7910, 0x4A7A00, 0x4A7990, 0x4CA140, 0x4CA160, 0x4CA000);
WRAPPER void CRenderer__RenderFadingInEntities(void) { VARJMP(CRenderer__RenderFadingInEntities_A); }
static uint32_t CTimeCycle__Update_A = AddressByVersion<uint32_t>(0x4ABF40, 0x4AC030, 0x4ABFC0, 0x4CEA40, 0x4CEA60, 0x4CE900);
WRAPPER void CTimeCycle__Update(void) { VARJMP(CTimeCycle__Update_A); }

static uint32_t rpSkinD3D8CreatePlainPipe_A = AddressByVersion<uint32_t>(0x5E0660, 0x5E0920, 0x5D6ED0, 0x6796D0, 0x679720, 0x678680);
WRAPPER void rpSkinD3D8CreatePlainPipe(void) { VARJMP(rpSkinD3D8CreatePlainPipe_A); }

int &skyBotRed = *AddressByVersion<int*>(0x9414D0, 0x941688, 0x9517C8, 0xA0D958, 0xA0D960, 0xA0C960);
int &skyBotGreen = *AddressByVersion<int*>(0x8F2BD0, 0x8F2C84, 0x902DC4, 0x97F208, 0x97F210, 0x97E210);
int &skyBotBlue = *AddressByVersion<int*>(0x8F625C, 0x8F6414, 0x906554, 0x9B6DF4, 0x9B6DFC, 0x9B5DFC);

byte &clockHour = *AddressByVersion<byte*>(0x95CDA6, 0x95CF5F, 0x96D09F, 0xA10B6B, 0xA10B74, 0xA0FB75);
byte &clockMinute = *AddressByVersion<byte*>(0x95CDC8, 0x95CF80, 0x96D0C0, 0xA10B92, 0xA10B9B, 0xA0FB9C);
// ADDRESS
byte &clockSecond = *AddressByVersion<byte*>(0x95CC7C, 0, 0, 0xA10A3C, 0xA10A44, 0xA0FA44);
short &oldWeather = *AddressByVersion<short*>(0x95CCEC, 0x95CEA4, 0x96CFE4, 0xA10AAA, 0xA10AB2, 0xA0FAB2);
short &newWeather = *AddressByVersion<short*>(0x95CC70, 0x95CE28, 0x96CF68, 0xA10A2E, 0xA10A36, 0xA0FA36);
float &weatherInterp = *AddressByVersion<float*>(0x8F2520, 0x8F25D4, 0x902714, 0x9787D8, 0x9787E0, 0x9777E0);


// car tweak
static float fresnelTable[24][MAXWEATHER], currentFresnel;
static float specPowerTable[24][MAXWEATHER], currentSpecPower;
static RwRGBAReal diffuseTable[24][MAXWEATHER], currentDiffuse;
static RwRGBAReal specularTable[24][MAXWEATHER], currentSpecular;

static void *pass1VS, *pass2VS;
static void *rimVS;
static void *worldPS;


static RwTexture *envTex, *envMaskTex;
static RwCamera *envCam;
static RwMatrix *envMatrix;

static RwIm2DVertex screenQuad[4];
static RwImVertexIndex screenindices[6] = { 0, 1, 2, 0, 2, 3 };

char*
getpath(char *path)
{
	static char tmppath[MAX_PATH];
	FILE *f;

	f = fopen(path, "r");
	if(f){
		fclose(f);
		return path;
	}
	strncpy(tmppath, asipath, MAX_PATH);
	strcat(tmppath, path);
	f = fopen(tmppath, "r");
	if(f){
		fclose(f);
		return tmppath;
	}
	return NULL;
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
	RwD3D9SetVertexShaderConstant(LOC_world, (void*)&worldMat, 4);
	RwD3D9SetVertexShaderConstant(LOC_worldIT, (void*)&worldMat, 4);
	RwD3D9SetVertexShaderConstant(LOC_view, (void*)&viewMat, 4);
	RwD3D9SetVertexShaderConstant(LOC_proj, (void*)&projMat, 4);

	RwMatrix *camfrm = RwFrameGetLTM(RwCameraGetFrame((RwCamera*)((RwGlobals*)RwEngineInst)->curCamera));
	RwD3D9SetVertexPixelShaderConstant(LOC_eye, (void*)RwMatrixGetPos(camfrm), 1);

	uploadLightColor(LOC_ambient, pAmbient, f);
	RwD3D9SetVertexShaderConstant(LOC_directDir, (void*)RwMatrixGetAt(RwFrameGetLTM(RpLightGetFrame(pDirect))), 1);
	uploadLightColor(LOC_directDiff, pDirect, f);
//	RwD3D9SetVertexShaderConstant(LOC_directSpec, (void*)&currentSpecular, 1);
	RwRGBAReal col = currentSpecular;
	col.red   *= col.alpha;
	col.green *= col.alpha;
	col.blue  *= col.alpha;
	RwD3D9SetVertexShaderConstant(LOC_directSpec, (void*)&col, 1);
	int i = 0;
	for(i = 0 ; i < NumExtraDirLightsInWorld; i++){
		RwD3D9SetVertexShaderConstant(LOC_lights+i*2, (void*)RwMatrixGetAt(RwFrameGetLTM(RpLightGetFrame(pExtraDirectionals[i]))), 1);
		uploadLightColor(LOC_lights+i*2+1, pExtraDirectionals[i], f);
	}
	static float zero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	for(; i < 4; i++){
		RwD3D9SetVertexShaderConstant(LOC_lights+i*2, (void*)zero, 1);
		RwD3D9SetVertexShaderConstant(LOC_lights+i*2+1, (void*)zero, 1);
	}
}

/*
WRAPPER void rpGeometryNativeWrite(RwStream*, RpGeometry*) { EAXJMP(0x673720); }
WRAPPER RwStream *RwStreamOpen(RwStreamType, RwStreamAccessType, const void*) { EAXJMP(0x6459C0); }
WRAPPER RwBool RwStreamClose(RwStream*, void*) { EAXJMP(0x6458F0); }

void
writenative(RpGeometry *g)
{
	RwStream *stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMWRITE, "nativeout.dff");
	g->flags |= rpGEOMETRYNATIVE;
	rpGeometryNativeWrite(stream, g);
	RwStreamClose(stream, NULL);
}
*/

void
carRenderCB(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
	RpAtomic *atomic = (RpAtomic*)object;
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
	RxD3D8InstanceData *inst = (RxD3D8InstanceData*)&header[1];

//	static int written = 0;
//	if(written == 0){
//		writenative(atomic->geometry);
//		written = 1;
//	}

	RwD3D9SetFVF(inst->vertexShader);
	uploadConstants(currentSpecular.alpha);

	// first pass - diffuse and reflection
	RwD3D9SetVertexShader(pass1VS);
	RwD3D9SetPixelShader(NULL);

	RwD3D8SetTexture(envTex, 1);
	RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MULTIPLYADD);
	RwD3D8SetTextureStageState(1, D3DTSS_COLORARG0, D3DTA_CURRENT);
	RwD3D8SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	RwD3D8SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_SPECULAR);
	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_CURRENT);

	int alpha = rwD3D8RenderStateIsVertexAlphaEnable();

	float reflProps[4];
	reflProps[1] = currentSpecPower;
	reflProps[2] = currentFresnel;
	RwRGBAReal color;

	for(int i = 0; i < header->numMeshes; i++){
		RpMaterial *m = inst->material;
		if(flags & (rpGEOMETRYTEXTURED|rpGEOMETRYTEXTURED2))
			RwD3D8SetTexture(m->texture, 0);
		else
			RwD3D8SetTexture(NULL, 0);
		if(inst->vertexAlpha || m->color.alpha != 0xFFu){
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
		// TODO: don't ignore diffuse tweak light (but zero anyway)
		reflProps[0] = m->surfaceProps.specular;
		if(DEBUGKEYS && GetAsyncKeyState(VK_F5) & 0x8000)
			reflProps[0] = 0.0;
		RwD3D9SetVertexPixelShaderConstant(LOC_surfProps, (void*)&reflProps, 1);
		RwRGBARealFromRwRGBA(&color, &m->color);
		if(DEBUGKEYS && GetAsyncKeyState(VK_F4) & 0x8000)
			color.red = color.green = color.blue = 0.0f;
		RwD3D9SetVertexPixelShaderConstant(LOC_matCol, (void*)&color, 1);

		RwD3D8SetStreamSource(0, inst->vertexBuffer, inst->stride);
		if(inst->indexBuffer){
			RwD3D8SetIndices(inst->indexBuffer, inst->baseIndex);
			RwD3D8DrawIndexedPrimitive(inst->primType, 0, inst->numVertices, 0, inst->numIndices);
		}else
			RwD3D8DrawPrimitive(inst->primType, inst->baseIndex, inst->numVertices);

		inst++;
	}

	RwD3D8SetTexture(NULL, 1);
	RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

	if(DEBUGKEYS && GetAsyncKeyState(VK_F6) & 0x8000)
		return;

	// second pass - specular
	inst = (RxD3D8InstanceData*)&header[1];
	if(!alpha)
		rwD3D8RenderStateVertexAlphaEnable(1);
	RwD3D9SetVertexShader(pass2VS);
	RwUInt32 dst, fog;
	RwRenderStateGet(rwRENDERSTATEFOGENABLE, &fog);
	RwRenderStateGet(rwRENDERSTATEDESTBLEND, &dst);
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
	RwD3D8SetTexture(NULL, 0);
	for(int i = 0; i < header->numMeshes; i++){
		RpMaterial *m = inst->material;
		if(m->surfaceProps.specular > 0.0f){
			reflProps[0] = m->surfaceProps.specular;
			if(DEBUGKEYS && GetAsyncKeyState(VK_F5) & 0x8000)
				reflProps[0] = 0.0;
			RwD3D9SetVertexPixelShaderConstant(LOC_surfProps, (void*)&reflProps, 1);
			RwRGBARealFromRwRGBA(&color, &m->color);
			if(DEBUGKEYS && GetAsyncKeyState(VK_F4) & 0x8000)
				color.red = color.green = color.blue = 0.0f;
			RwD3D9SetVertexPixelShaderConstant(LOC_matCol, (void*)&color, 1);

			RwD3D8SetStreamSource(0, inst->vertexBuffer, inst->stride);
			if(inst->indexBuffer){
				RwD3D8SetIndices(inst->indexBuffer, inst->baseIndex);
				RwD3D8DrawIndexedPrimitive(inst->primType, 0, inst->numVertices, 0, inst->numIndices);
			}else
				RwD3D8DrawPrimitive(inst->primType, inst->baseIndex, inst->numVertices);
		}
		inst++;
	}

	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)fog);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)dst);
}

//
// init
//

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

//float mult = 1.0;

void
updateTweakValues(void)
{
	CTimeCycle__Update();

	//*(float*)0x8F29B4 *= mult;
	//*(float*)0x94144C *= mult;
	//*(float*)0x942FC0 *= mult;

	int nextHour = (clockHour+1)%24;
	float timeInterp = isVC() ? (clockSecond/60.0f + clockMinute)/60.0f : clockMinute/60.0f;
	blerp[0] = (1.0f-timeInterp)*(1.0f-weatherInterp);
	blerp[1] = timeInterp*(1.0f-weatherInterp);
	blerp[2] = (1.0f-timeInterp)*weatherInterp;
	blerp[3] = timeInterp*weatherInterp;

	currentFresnel = interploateFloat(fresnelTable[clockHour], fresnelTable[nextHour]);
	currentSpecPower = interploateFloat(specPowerTable[clockHour], specPowerTable[nextHour]);
	interploateRGBA(&currentDiffuse, diffuseTable[clockHour], diffuseTable[nextHour]);
	interploateRGBA(&currentSpecular, specularTable[clockHour], specularTable[nextHour]);
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
initFloat(float t[24][MAXWEATHER], float f)
{
	for(int i = 0; i < 24; i++)
		for(int j = 0; j < MAXWEATHER; j++)
			t[i][j] = f;
}

void
readLight(char *s, int line, int field, void *table)
{
	int r, g, b, a;
	RwRGBAReal (*t)[24][MAXWEATHER] = (RwRGBAReal (*)[24][MAXWEATHER])table;
	sscanf(s, "%d, %d, %d, %d", &r, &g, &b, &a);
	RwRGBAReal c = { r/255.0f, g/255.0f, b/255.0f, a/100.0f };
	(*t)[line][field]  = c;
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
initColor(RwRGBAReal t[24][MAXWEATHER], RwRGBAReal c)
{
	for(int i = 0; i < 24; i++)
		for(int j = 0; j < MAXWEATHER; j++)
			t[i][j] = c;
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
		c = getc(file);
	}
	ungetc(c, file);
}

RxPipeline *carpipe, *worldpipe;

class CVehicleModelInfo {
public:
	void SetClump(RpClump *clump);
	void SetClump_hook(RpClump *clump);
};

static uint32_t CVehicleModelInfo__SetClump_A = AddressByVersion<uint32_t>(0x51FC60, 0x51FE90, 0x51FE20, 0x57A800, 0x57A820, 0x57A6F0);
WRAPPER void CVehicleModelInfo::SetClump(RpClump*) { VARJMP(CVehicleModelInfo__SetClump_A); }

static RpAtomic*
setAtomicPipelineCB(RpAtomic *atomic, void *data)
{
	RpAtomicSetPipeline(atomic, (RxPipeline*)data);
	// has to be reset otherwise matfx pipe will be attached again on copy
	*RWPLUGINOFFSET(int, atomic, MatFXAtomicDataOffset) = 0;
	return atomic;
}

void
CVehicleModelInfo::SetClump_hook(RpClump *clump)
{
	this->SetClump(clump);
	RpClumpForAllAtomics(clump, setAtomicPipelineCB, carpipe);
}

static uint32_t CVisibilityPlugins__SetClumpModelInfo_A = AddressByVersion<uint32_t>(0x528ED0, 0x529110, 0x529110, 0, 0, 0);
WRAPPER void CVisibilityPlugins__SetClumpModelInfo(RpClump*, CClumpModelInfo*) { VARJMP(CVisibilityPlugins__SetClumpModelInfo_A); }
static uint32_t CBaseModelInfo__AddTexDictionaryRef_A = AddressByVersion<uint32_t>(0x4F6B80, 0x4F6C30, 0x4F6BC0, 0, 0, 0);
WRAPPER void __fastcall CBaseModelInfo__AddTexDictionaryRef(CClumpModelInfo*) { VARJMP(CBaseModelInfo__AddTexDictionaryRef_A); }


static RpAtomicCallBack CClumpModelInfo__SetAtomicRendererCB_A = AddressByVersion<RpAtomicCallBack>(0x4F8940, 0x4F8A20, 0x4F89B0, 0, 0, 0);
static void *CVisibilityPlugins__RenderPlayerCB_A = AddressByVersion<void*>(0x528B30, 0x528D70, 0x528D00, 0, 0, 0);

// ADDRESS
static uint32_t CSimpleModelInfo__SetAtomic_A = AddressByVersion<uint32_t>(0x517950, 0x517B60, 0x517AF0, 0x56F790, 0, 0);
WRAPPER void CSimpleModelInfo::SetAtomic(int, RpAtomic*) { VARJMP(CSimpleModelInfo__SetAtomic_A); }

static uint32_t RenderScene_A = AddressByVersion<uint32_t>(0x48E030, 0x48E0F0, 0x48E080, 0x4A6570, 0x4A6590, 0x4A6440);
WRAPPER void RenderScene(void) { VARJMP(RenderScene_A); }
//static uint32_t CTimeCycle__Initialise_A = AddressByVersion<uint32_t>(0x4ABAE0, 0, 0, 0x4D05E0, 0, 0);
//WRAPPER void CTimeCycle__Initialise(void) { VARJMP(CTimeCycle__Initialise_A); }

void
RenderScene_hook(void)
{
	RenderScene();
	if(xboxcarpipe)
		RenderEnvTex();
}

RxPipeline*
createPipe(void)
{
	RxPipeline *pipe;
	pipe = RxPipelineCreate();
	if(pipe){
		RxLockedPipe *lpipe;
		lpipe = RxPipelineLock(pipe);
		if(lpipe){
			RxNodeDefinition *nodedef= RxNodeDefinitionGetD3D8AtomicAllInOne();
			RxLockedPipeAddFragment(lpipe, 0, nodedef, NULL);
			RxLockedPipeUnlock(lpipe);
			return pipe;
		}
		RxPipelineDestroy(pipe);
	}
	return NULL;
}

void
RxD3D8AllInOneSetInstanceCallBack(RxPipelineNode *node, RxD3D8AllInOneInstanceCallBack callback)
{
	*(RxD3D8AllInOneInstanceCallBack*)node->privateData = callback;
}

//RxPipeline *&skinpipe = *AddressByVersion<RxPipeline**>(0x663CAC, 0x663CAC, 0x673DB0, 0x78A0D4, 0x78A0DC, 0x7890DC);

void
neoInit(void)
{
	HRSRC resource;
	RwUInt32 *shader;
	RxPipelineNode *node;
	FILE *dat;
	char *path;

//	HMODULE sky = GetModuleHandleA("iii_anim.asi");
//	if(sky)
//		MessageBox(NULL, "iii_anim was found", "Error", MB_ICONERROR | MB_OK);

	if(!RwD3D9Supported())
		return;

	// car pipeline
	if(xboxcarpipe >= 0){
		carpipe = createPipe();
		RxNodeDefinition *nodedef = RxNodeDefinitionGetD3D8AtomicAllInOne();
		node = RxPipelineFindNodeByName(carpipe, nodedef->name, NULL, NULL);
		RxD3D8AllInOneSetRenderCallBack(node, carRenderCB);
		Patch(AddressByVersion<uint32_t>(0x5FDFF0, 0x5FDDD8, 0x60ADD0, 0x698088, 0x698088, 0x697090), &CVehicleModelInfo::SetClump_hook);
		
		resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_VEHICLEONEVS), RT_RCDATA);
		shader = (RwUInt32*)LoadResource(dllModule, resource);
		RwD3D9CreateVertexShader(shader, &pass1VS);
		assert(pass1VS);
		FreeResource(shader);
	
		resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_VEHICLETWOVS), RT_RCDATA);
		shader = (RwUInt32*)LoadResource(dllModule, resource);
		RwD3D9CreateVertexShader(shader, &pass2VS);
		assert(pass2VS);
		FreeResource(shader);
		
		path = getpath("neo\\carTweakingTable.dat");
		assert(path && "couldn't load 'neo\\carTweakingTable.dat'");
		dat = fopen(path, "r");
		readWeatherTimeBlock(dat, readFloat, fresnelTable);		// default 0.4
		readWeatherTimeBlock(dat, readFloat, specPowerTable);		// default 18.0
		readWeatherTimeBlock(dat, readLight, diffuseTable);		// default 0.0, 0.0, 0.0, 0.0
		readWeatherTimeBlock(dat, readLight, specularTable);		// default 0.7, 0.7, 0.7, 1.0
		fclose(dat);
		// make it update
		InjectHook(AddressByVersion<uint32_t>(0x48C9A2, 0x48CAA2, 0x48CA32, 0x4A45F5, 0x4A4615, 0x4A446F), updateTweakValues);
		
		// reflection things
		RwRaster *envFB = RwRasterCreate(envMapSize, envMapSize, 0, rwRASTERTYPECAMERATEXTURE);
		RwRaster *envZB = RwRasterCreate(envMapSize, envMapSize, 0, rwRASTERTYPEZBUFFER);
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

		path = getpath("neo\\CarReflectionMask.bmp");
		assert(path && "couldn't load 'neo\\CarReflectionMask.bmp'");
		RwImage *envMaskI = RtBMPImageRead(path);
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
		
		InjectHook(AddressByVersion<uint32_t>(0x48E5F9, 0x48E6B9, 0x48E649, 0x4A604A, 0x4A606A, 0x4A5F1A), RenderScene_hook);
	}

	// rim pipeline
	if(rimlight >= 0)
		neoRimPipeInit();

	if(xboxworldpipe >= 0 && (isIII() || isVC()))
		neoWorldPipeInit();
}
