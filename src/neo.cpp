#include "skygfx.h"
#include "d3d8.h"
#include "d3d8types.h"

byte &CClock__ms_nGameClockHours = *AddressByVersion<byte*>(0x95CDA6, 0x95CF5F, 0x96D09F, 0xA10B6B, 0xA10B74, 0xA0FB75);
byte &CClock__ms_nGameClockMinutes = *AddressByVersion<byte*>(0x95CDC8, 0x95CF80, 0x96D0C0, 0xA10B92, 0xA10B9B, 0xA0FB9C);
// ADDRESS
short &CClock__ms_nGameClockSeconds = *AddressByVersion<short*>(0x95CC7C, 0, 0, 0xA10A3C, 0xA10A44, 0xA0FA44);
short &CWeather__OldWeatherType = *AddressByVersion<short*>(0x95CCEC, 0x95CEA4, 0x96CFE4, 0xA10AAA, 0xA10AB2, 0xA0FAB2);
short &CWeather__NewWeatherType = *AddressByVersion<short*>(0x95CC70, 0x95CE28, 0x96CF68, 0xA10A2E, 0xA10A36, 0xA0FA36);
float &CWeather__InterpolationValue = *AddressByVersion<float*>(0x8F2520, 0x8F25D4, 0x902714, 0x9787D8, 0x9787E0, 0x9777E0);

RwTexDictionary *neoTxd;

void
RwToD3DMatrix(void *d3d, RwMatrix *rw)
{
	D3DMATRIX *m = (D3DMATRIX*)d3d;
	m->m[0][0] = rw->right.x;
	m->m[0][1] = rw->up.x;
	m->m[0][2] = rw->at.x;
	m->m[0][3] = rw->pos.x;
	m->m[1][0] = rw->right.y;
	m->m[1][1] = rw->up.y;
	m->m[1][2] = rw->at.y;
	m->m[1][3] = rw->pos.y;
	m->m[2][0] = rw->right.z;
	m->m[2][1] = rw->up.z;
	m->m[2][2] = rw->at.z;
	m->m[2][3] = rw->pos.z;
	m->m[3][0] = 0.0f;
	m->m[3][1] = 0.0f;
	m->m[3][2] = 0.0f;
	m->m[3][3] = 1.0f;
}

void
MakeProjectionMatrix(void *d3d, RwCamera *cam, float nbias, float fbias)
{
	float f = cam->farPlane + fbias;
	float n = cam->nearPlane + nbias;
	D3DMATRIX *m = (D3DMATRIX*)d3d;
	m->m[0][0] = cam->recipViewWindow.x;
	m->m[0][1] = 0.0f;
	m->m[0][2] = 0.0f;
	m->m[0][3] = 0.0f;
	m->m[1][0] = 0.0f;
	m->m[1][1] = cam->recipViewWindow.y;
	m->m[1][2] = 0.0f;
	m->m[1][3] = 0.0f;
	m->m[2][0] = 0.0f;
	m->m[2][1] = 0.0f;
	m->m[2][2] = f/(f-n);
	m->m[2][3] = -n*m->m[2][2];
	m->m[3][0] = 0.0f;
	m->m[3][1] = 0.0f;
	m->m[3][2] = 1.0f;
	m->m[3][3] = 0.0f;
}

void
neoInit(void)
{
	if(!RwD3D9Supported()){
		config.iCanHasNeoGloss = false;
		config.iCanHasNeoRim = false;
		config.iCanHasNeoCar = false;
	}

	// World pipe has a non-shader fallback so works without d3d9
	if(config.iCanHasNeoWorld)
		neoWorldPipeInit();

	if(xboxcarpipe >= 0 || xboxwaterdrops){
		char *path = getpath("neo\\neo.txd");
		if(path == NULL){
			MessageBox(NULL, "Couldn't load 'neo\\neo.txd'", "Error", MB_ICONERROR | MB_OK);
			exit(0);
		}
		RwStream *stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, path);
		if(RwStreamFindChunk(stream, rwID_TEXDICTIONARY, NULL, NULL))
			neoTxd = RwTexDictionaryStreamRead(stream);
		RwStreamClose(stream, NULL);
		if(neoTxd == NULL){
			MessageBox(NULL, "Couldn't find Tex Dictionary inside 'neo\\neo.txd'", "Error", MB_ICONERROR | MB_OK);
			exit(0);
		}
		// we can just set this to current because we're executing before CGame::Initialise
		// which sets up "generic" as the current TXD
		RwTexDictionarySetCurrent(neoTxd);
	}

	WaterDrops::ms_maskTex = RwTextureRead("dropmask", NULL);

	if(config.iCanHasNeoGloss)
		neoGlossPipeInit();

	if(config.iCanHasNeoCar)
		neoCarPipeInit();

	if(config.iCanHasNeoRim)
		neoRimPipeInit();
}

#define INTERP_SETUP \
		int h1 = CClock__ms_nGameClockHours;								  \
		int h2 = (h1+1)%24;										  \
		int w1 = CWeather__OldWeatherType;								  \
		int w2 = CWeather__NewWeatherType;								  \
		float timeInterp = (CClock__ms_nGameClockSeconds/60.0f + CClock__ms_nGameClockMinutes)/60.0f;	  \
		float c0 = (1.0f-timeInterp)*(1.0f-CWeather__InterpolationValue);				  \
		float c1 = timeInterp*(1.0f-CWeather__InterpolationValue);					  \
		float c2 = (1.0f-timeInterp)*CWeather__InterpolationValue;					  \
		float c3 = timeInterp*CWeather__InterpolationValue;
#define INTERP(v) v[h1][w1]*c0 + v[h2][w1]*c1 + v[h1][w2]*c2 + v[h2][w2]*c3;
#define INTERPF(v,f) v[h1][w1].f*c0 + v[h2][w1].f*c1 + v[h1][w2].f*c2 + v[h2][w2].f*c3;


InterpolatedFloat::InterpolatedFloat(float init)
{
	curInterpolator = 61;	// compared against second
	for(int h = 0; h < 24; h++)
		for(int w = 0; w < NUMWEATHERS; w++)
			data[h][w] = init;
}

void
InterpolatedFloat::Read(char *s, int line, int field)
{
	sscanf(s, "%f", &data[line][field]);
}

float
InterpolatedFloat::Get(void)
{
	if(curInterpolator != CClock__ms_nGameClockSeconds){
		INTERP_SETUP
		curVal = INTERP(data);
		curInterpolator = CClock__ms_nGameClockSeconds;
	}
	return curVal;
}

InterpolatedColor::InterpolatedColor(const Color &init)
{
	curInterpolator = 61;	// compared against second
	for(int h = 0; h < 24; h++)
		for(int w = 0; w < NUMWEATHERS; w++)
			data[h][w] = init;
}

void
InterpolatedColor::Read(char *s, int line, int field)
{
	int r, g, b, a;
	sscanf(s, "%i, %i, %i, %i", &r, &g, &b, &a);
	data[line][field] = Color(r/255.0f, g/255.0f, b/255.0f, a/255.0f);
}

Color
InterpolatedColor::Get(void)
{
	if(curInterpolator != CClock__ms_nGameClockSeconds){
		INTERP_SETUP
		curVal.r = INTERPF(data, r);
		curVal.g = INTERPF(data, g);
		curVal.b = INTERPF(data, b);
		curVal.a = INTERPF(data, a);
		curInterpolator = CClock__ms_nGameClockSeconds;
	}
	return curVal;
}

void
InterpolatedLight::Read(char *s, int line, int field)
{
	int r, g, b, a;
	sscanf(s, "%i, %i, %i, %i", &r, &g, &b, &a);
	data[line][field] = Color(r/255.0f, g/255.0f, b/255.0f, a/100.0f);
}

void
neoReadWeatherTimeBlock(FILE *file, InterpolatedValue *interp)
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
				interp->Read(buf, line, field);
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


RxPipeline*
neoCreatePipe(void)
{
	RxPipeline *pipe;
	RxPipelineNode *node;

	pipe = RxPipelineCreate();
	if(pipe){
		RxLockedPipe *lpipe;
		lpipe = RxPipelineLock(pipe);
		if(lpipe){
			RxNodeDefinition *nodedef= RxNodeDefinitionGetD3D8AtomicAllInOne();
			RxLockedPipeAddFragment(lpipe, 0, nodedef, NULL);
			RxLockedPipeUnlock(lpipe);

			node = RxPipelineFindNodeByName(pipe, nodedef->name, NULL, NULL);
			RxD3D8AllInOneSetInstanceCallBack(node, D3D8AtomicDefaultInstanceCallback_fixed);
			RxD3D8AllInOneSetRenderCallBack(node, rxD3D8DefaultRenderCallback_xbox);
			return pipe;
		}
		RxPipelineDestroy(pipe);
	}
	return NULL;
}

void
CustomPipe::CreateRwPipeline(void)
{
	rwPipeline = neoCreatePipe();
}

void
CustomPipe::SetRenderCallback(RxD3D8AllInOneRenderCallBack cb)
{
	RxPipelineNode *node;
	RxNodeDefinition *nodedef = RxNodeDefinitionGetD3D8AtomicAllInOne();
	node = RxPipelineFindNodeByName(rwPipeline, nodedef->name, NULL, NULL);
	originalRenderCB = RxD3D8AllInOneGetRenderCallBack(node);
	RxD3D8AllInOneSetRenderCallBack(node, cb);
}

void
CustomPipe::Attach(RpAtomic *atomic)
{
	RpAtomicSetPipeline(atomic, rwPipeline);
	/*
	 * From rw 3.7 changelog:
	 *
	 *	07/01/03 Cloning atomics with MatFX - BZ#3430
	 *	  When cloning cloning an atomic with matfx applied the matfx copy callback
	 *	  would also setup the matfx pipes after they had been setup by the atomic
	 *	  clone function. This could overwrite things like skin or patch pipes with
	 *	  generic matfx pipes when it shouldn't.
	 *
	 * Since we're not using 3.5 we have to fix this manually:
	 */
	*RWPLUGINOFFSET(int, atomic, MatFXAtomicDataOffset) = 0;
}

RpAtomic*
CustomPipe::setatomicCB(RpAtomic *atomic, void *data)
{
	((CustomPipe*)data)->Attach(atomic);
	return atomic;
}

/* Shader helpers */

void
UploadZero(int loc)
{
	static float z[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	RwD3D9SetVertexShaderConstant(loc, (void*)z, 1);
}

void
UploadLightColor(RpLight *light, int loc)
{
	float c[4];
	if(RpLightGetFlags(light) & rpLIGHTLIGHTATOMICS){
		c[0] = light->color.red;
		c[1] = light->color.green;
		c[2] = light->color.blue;
		c[3] = 1.0f;
		RwD3D9SetVertexShaderConstant(loc, (void*)c, 1);
	}else
		UploadZero(loc);
}

void
UploadLightDirection(RpLight *light, int loc)
{
	float c[4];
	if(RpLightGetFlags(light) & rpLIGHTLIGHTATOMICS){
		RwV3d *at = RwMatrixGetAt(RwFrameGetLTM(RpLightGetFrame(light)));
		c[0] = at->x;
		c[1] = at->y;
		c[2] = at->z;
		c[3] = 1.0f;
		RwD3D9SetVertexShaderConstant(loc, (void*)c, 1);
	}else
		UploadZero(loc);
}

void
UploadLightDirectionInv(RpLight *light, int loc)
{
	float c[4];
	RwV3d *at = RwMatrixGetAt(RwFrameGetLTM(RpLightGetFrame(light)));
	c[0] = -at->x;
	c[1] = -at->y;
	c[2] = -at->z;
	RwD3D9SetVertexShaderConstant(loc, (void*)c, 1);
}
