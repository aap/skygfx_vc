#include "skygfx.h"
#include "d3d8.h"
#include "d3d8types.h"
#include <time.h>

RwTexture *rampTex;

#pragma pack(push, 1)
struct TGAHeader
{
	RwInt8  IDlen;
	RwInt8  colorMapType;
	RwInt8  imageType;
	RwInt16 colorMapOrigin;
	RwInt16 colorMapLength;
	RwInt8  colorMapDepth;
	RwInt16 xOrigin, yOrigin;
	RwInt16 width, height;
	RwUInt8 depth;
	RwUInt8 descriptor;
};
#pragma pack(pop)

RwImage*
readTGA(const char *afilename)
{
	RwImage *image;

	RwStream *stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, afilename);

	TGAHeader header;
	int depth = 0, palDepth = 0;
	RwStreamRead(stream, &header, sizeof(header));

	assert(header.imageType == 1 || header.imageType == 2);
	RwStreamSkip(stream, header.IDlen);
	if(header.colorMapType){
		assert(header.colorMapOrigin == 0);
		depth = (header.colorMapLength <= 16) ? 4 : 8;
		palDepth = header.colorMapDepth;
		assert(palDepth == 24 || palDepth == 32);
	}else{
		depth = header.depth;
		assert(depth == 24 || depth == 32);
	}

	image = RwImageCreate(header.width, header.height, depth == 24 ? 32 : depth);
	RwImageAllocatePixels(image);
	RwRGBA *palette = header.colorMapType ? image->palette : NULL;
	if(palette){
		int maxlen = depth == 4 ? 16 : 256;
		int i;
		for(i = 0; i < header.colorMapLength; i++){
			RwStreamRead(stream, &palette[i].blue, 1);
			RwStreamRead(stream, &palette[i].green, 1);
			RwStreamRead(stream, &palette[i].red, 1);
			palette[i].alpha = 0xFF;
			if(palDepth == 32)
				RwStreamRead(stream, &palette[i].alpha, 1);
		}
		for(; i < maxlen; i++){
			palette[i].red = palette[i].green = palette[i].blue = 0;
			palette[i].alpha = 0xFF;
		}
	}

	RwUInt8 *pixels = image->cpPixels;
	if(!(header.descriptor & 0x20))
		pixels += (image->height-1)*image->stride;
	for(int y = 0; y < image->height; y++){
		RwUInt8 *line = pixels;
		for(int x = 0; x < image->width; x++){
			if(palette)
				RwStreamRead(stream, line++, 1);
			else{
				RwStreamRead(stream, &line[2], 1);
				RwStreamRead(stream, &line[1], 1);
				RwStreamRead(stream, &line[0], 1);
				line += 3;
				if(depth == 32)
					RwStreamRead(stream, line++, 1);
				if(depth == 24)
					*line++ = 0xFF;
			}
		}
		pixels += (header.descriptor&0x20) ?
		              image->stride : -image->stride;
	}

	RwStreamClose(stream, NULL);
	return image;
}

void
reloadRamp(void)
{
	RwImage *img = readTGA("curves.tga");
	RwInt32 w, h, d, flags;
	RwImageFindRasterFormat(img, 4, &w, &h, &d, &flags);
	RwRaster *ras = RwRasterCreate(w, h, d, flags);
	ras = RwRasterSetFromImage(ras, img);
	assert(ras);
	rampTex = RwTextureCreate(ras);
	RwTextureAddRef(rampTex);
	RwImageDestroy(img);
}



/*
 * Water Drops
 */

static uint32_t RenderEffects_A = AddressByVersion<uint32_t>(0x48E090, 0, 0, 0x4A6510, 0, 0);
WRAPPER void RenderEffects(void) { VARJMP(RenderEffects_A); }

#define INJECTRESET(x) \
	uint32_t reset_call_##x; \
	void reset_hook_##x(void){ \
		((voidfunc)reset_call_##x)(); \
		WaterDrops::Reset(); \
	}

INJECTRESET(1)
INJECTRESET(2)
INJECTRESET(3)
INJECTRESET(4)
INJECTRESET(5)

uint32_t splashbreak;
void __declspec(naked)
splashhook(void)
{
	_asm{
		push	ebp
		call	WaterDrops::RegisterSplash
		pop	ebp
		mov	eax, splashbreak
		jmp	eax
	}
}

void
RenderEffects_hook(void)
{
	RenderEffects();
	WaterDrops::Process();
	WaterDrops::Render();
}

void
hookWaterDrops(void)
{
	if(gtaversion == III_10 || gtaversion == VC_10){
		// ADDRESSES
		MemoryVP::InjectHook(AddressByVersion<uint32_t>(0x48E603, 0, 0, 0x4A604F, 0, 0), RenderEffects_hook);
		INTERCEPT(reset_call_1, reset_hook_1, AddressByVersion<uint32_t>(0x48C1AB, 0, 0, 0x4A4DD6, 0, 0));
		INTERCEPT(reset_call_2, reset_hook_2, AddressByVersion<uint32_t>(0x48C530, 0, 0, 0x4A48EA, 0, 0));
		INTERCEPT(reset_call_3, reset_hook_3, AddressByVersion<uint32_t>(0x42155D, 0, 0, 0x42BCD6, 0, 0));
		INTERCEPT(reset_call_4, reset_hook_4, AddressByVersion<uint32_t>(0x42177A, 0, 0, 0x42C0BC, 0, 0));
		INTERCEPT(reset_call_5, reset_hook_5, AddressByVersion<uint32_t>(0x421926, 0, 0, 0x42C318, 0, 0));

		INTERCEPT(splashbreak, splashhook, AddressByVersion<uint32_t>(0x4BC7D0, 0, 0, 0x4E8721, 0, 0));
	}
}

struct AudioHydrant	// or particle object thing?
{
	int entity;
	union {
		CPlaceable_III *particleObjectIII;	// CParticleObject actually
		CPlaceable *particleObject;
	};
};

IDirect3DDevice8 *&RwD3DDevice = *AddressByVersion<IDirect3DDevice8**>(0x662EF0, 0x662EF0, 0x67342C, 0x7897A8, 0x7897B0, 0x7887B0);

// ADDRESS
RwCamera *&rwcam = *AddressByVersion<RwCamera**>(0x72676C, 0, 0, 0x8100BC, 0, 0);
float &CTimer__ms_fTimeStep = *AddressByVersion<float*>(0x8E2CB4, 0, 0, 0x975424, 0, 0);
float &CWeather__Rain = *AddressByVersion<float*>(0x8E2BFC, 0, 0, 0x975340, 0, 0);
bool &CCutsceneMgr__ms_running = *AddressByVersion<bool*>(0x95CCF5, 0, 0, 0xA10AB2, 0, 0);

AudioHydrant *audioHydrants = AddressByVersion<AudioHydrant*>(0x62DAAC, 0, 0, 0x70799C, 0, 0);

static uint32_t CCullZones__CamNoRain_A = AddressByVersion<uint32_t>(0x525CE0, 0, 0, 0x57E0E0, 0, 0);
WRAPPER bool CCullZones__CamNoRain(void) { VARJMP(CCullZones__CamNoRain_A); }
static uint32_t CCullZones__PlayerNoRain_A = AddressByVersion<uint32_t>(0x525D00, 0, 0, 0x57E0C0, 0, 0);
WRAPPER bool CCullZones__PlayerNoRain(void) { VARJMP(CCullZones__PlayerNoRain_A); }


#define MAXSIZE 15
#define MINSIZE 4

float scaling;
#define SC(x) ((int)((x)*scaling))

float WaterDrops::ms_xOff, WaterDrops::ms_yOff;	// not quite sure what these are
WaterDrop WaterDrops::ms_drops[MAXDROPS];
int WaterDrops::ms_numDrops;
WaterDropMoving WaterDrops::ms_dropsMoving[MAXDROPSMOVING];
int WaterDrops::ms_numDropsMoving;

bool WaterDrops::ms_enabled = 1;
bool WaterDrops::ms_movingEnabled = 1;

int WaterDrops::ms_splashDuration;
CPlaceable_III *WaterDrops::ms_splashObject;

float WaterDrops::ms_distMoved, WaterDrops::ms_vecLen, WaterDrops::ms_rainStrength;
RwV3d WaterDrops::ms_vec;
RwV3d WaterDrops::ms_lastAt;
RwV3d WaterDrops::ms_lastPos;
RwV3d WaterDrops::ms_posDelta;

RwTexture *WaterDrops::ms_maskTex;
RwTexture *WaterDrops::ms_tex;	// TODO
RwRaster *WaterDrops::ms_maskRaster;
RwRaster *WaterDrops::ms_raster;	// TODO
int WaterDrops::ms_fbWidth, WaterDrops::ms_fbHeight;
void *WaterDrops::ms_vertexBuf;
void *WaterDrops::ms_indexBuf;
VertexTex2 *WaterDrops::ms_vertPtr;
int WaterDrops::ms_numBatchedDrops;
int WaterDrops::ms_initialised;

#define DROPFVF (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX2)

void
WaterDrop::Fade(void)
{
	int delta = CTimer__ms_fTimeStep * 1000.0f / 50.0f;
	this->time += delta;
	if(this->time >= this->ttl){
		WaterDrops::ms_numDrops--;
		this->active = 0;
	}else if(this->fades)
		this->alpha = 255 - time/ttl * 255;
}


void
WaterDrops::Process(void)
{
	if(!ms_initialised)
		InitialiseRender(rwcam);
	WaterDrops::CalculateMovement();
	WaterDrops::SprayDrops();
	WaterDrops::ProcessMoving();
	WaterDrops::Fade();
}

#define RAD2DEG(x) (180.0f*(x)/M_PI)

void
WaterDrops::CalculateMovement(void)
{
	RwV3d pos;
	RwMatrix *modelMatrix;
	modelMatrix = &RwCameraGetFrame(rwcam)->modelling;
	RwV3dSub(&ms_posDelta, &modelMatrix->pos, &ms_lastPos);
	ms_distMoved = RwV3dLength(&ms_posDelta);
	pos.x = (modelMatrix->at.x - ms_lastAt.x) * 10.0f;
	pos.y = (modelMatrix->at.y - ms_lastAt.y) * 10.0f;
	pos.z = (modelMatrix->at.z - ms_lastAt.z) * 10.0f;
	// ^ result unused for now
	ms_lastAt = modelMatrix->at;
	ms_lastPos = modelMatrix->pos;

	ms_vec.x = RwV3dDotProduct(&modelMatrix->right, &ms_posDelta);
	ms_vec.y = RwV3dDotProduct(&modelMatrix->up, &ms_posDelta);
	ms_vec.z = RwV3dDotProduct(&modelMatrix->at, &ms_posDelta);
	RwV3dScale(&ms_vec, &ms_vec, 10.0f);
	ms_vecLen = sqrt(ms_vec.y*ms_vec.y + ms_vec.x*ms_vec.x);

	float c = modelMatrix->at.z;
	if(c > 1.0f) c = 1.0f;
	if(c < -1.0f) c = -1.0f;
	ms_rainStrength = RAD2DEG(acos(c));
}

void
WaterDrops::SprayDrops(void)
{
	AudioHydrant *hyd;
	RwV3d dist;
	static int ndrops[] = {
		125, 250, 500, 1000, 1000,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	{
		static bool keystate = false;
		if(GetAsyncKeyState(VK_TAB) & 0x8000){
			if(!keystate){
				keystate = true;
				FillScreenMoving(10.0f);
			}
		}else
			keystate = false;
	}

	if(CWeather__Rain != 0.0f && ms_enabled){
		int tmp = 180.0f - ms_rainStrength;
		if(tmp < 40) tmp = 40;
		FillScreenMoving((tmp - 40.0f) / 150.0f * CWeather__Rain * 0.5f);
	}
	for(hyd = audioHydrants; hyd < &audioHydrants[8]; hyd++)
		if(hyd->particleObject){
			if(isIII())
				RwV3dSub(&dist, &hyd->particleObjectIII->matrix.matrix.pos, &ms_lastPos);
			else
				RwV3dSub(&dist, &hyd->particleObject->matrix.matrix.pos, &ms_lastPos);
			if(RwV3dDotProduct(&dist, &dist) <= 40.0f)
				FillScreenMoving(1.0f);
		}
	if(ms_splashDuration >= 0){
		if(ms_numDrops < MAXDROPS){
			if(isIII()){
				RwV3dSub(&dist, &ms_splashObject->matrix.matrix.pos, &ms_lastPos);
				int n = ndrops[ms_splashDuration] * (1.0f - (RwV3dLength(&dist) - 5.0f) / 15.0f);
				while(n--)
					if(ms_numDrops < MAXDROPS){
						float x = rand() % ms_fbWidth;
						float y = rand() % ms_fbHeight;
						float time = rand() % (SC(MAXSIZE) - SC(MINSIZE)) + SC(MINSIZE);
						PlaceNew(x, y, time, 10000.0f, 0);
					}
			}else
				// VC does STRANGE things here
				FillScreenMoving(1.0f);
		}
		ms_splashDuration--;
	}
}

void
WaterDrops::MoveDrop(WaterDropMoving *moving)
{
	WaterDrop *drop = moving->drop;
	if(!ms_movingEnabled)
		return;
	if(!drop->active){
		moving->drop = NULL;
		ms_numDropsMoving--;
		return;
	}
	if(ms_vec.z <= 0.0f || ms_distMoved <= 0.3f)
		return;

	if(ms_vecLen <= 0.5f /* || cam shit */){
		float d = ms_vec.z*0.2f;
		float dx, dy, sum;
		dx = drop->x - ms_fbWidth*0.5f + ms_vec.x;
		// TODO: 1.2 instead of 0.5 when camshit == 16
		dy = drop->y - ms_fbHeight*0.5f - ms_vec.y;
		sum = fabs(dx) + fabs(dy);
		if(sum >= 0.001f){
			dx *= (1.0/sum);
			dy *= (1.0/sum);
		}
		moving->dist += d;
		if(moving->dist > 20.0f)
			NewTrace(moving);
		drop->x += dx * d;
		drop->y += dy * d;
	}else{
		moving->dist += ms_vecLen;
		if(moving->dist > 20.0f)
			NewTrace(moving);
		drop->x -= ms_vec.x;
		drop->y += ms_vec.y;
	}

	if(drop->x < 0.0f || drop->y < 0.0f ||
	   drop->x > ms_fbWidth || drop->y > ms_fbHeight){
		moving->drop = NULL;
		ms_numDropsMoving--;
	}
}

void
WaterDrops::ProcessMoving(void)
{
	WaterDropMoving *moving;
	if(!ms_movingEnabled)
		return;
	for(moving = ms_dropsMoving; moving < &ms_dropsMoving[MAXDROPSMOVING]; moving++)
		if(moving->drop)
			MoveDrop(moving);
}

void
WaterDrops::Fade(void)
{
	WaterDrop *drop;
	for(drop = &ms_drops[0]; drop < &ms_drops[MAXDROPS]; drop++)
		if(drop->active)
			drop->Fade();
}


WaterDrop*
WaterDrops::PlaceNew(float x, float y, float size, float ttl, bool fades)
{
	WaterDrop *drop;
	int i;

	if(NoRain())
		return NULL;

	for(i = 0, drop = ms_drops; i < MAXDROPS; i++, drop++)
		if(ms_drops[i].active == 0)
			goto found;
	return NULL;
found:
	ms_numDrops++;
	drop->x = x;
	drop->y = y;
	drop->size = size;
	drop->uvsize = (SC(MAXSIZE) - size + 1.0f) / (SC(MAXSIZE) - SC(MINSIZE) + 1.0f);
	drop->fades = fades;
	drop->active = 1;
	drop->alpha = 0xFF;
	drop->time = 0.0f;
	drop->ttl = ttl;
	return drop;
}

void
WaterDrops::NewTrace(WaterDropMoving *moving)
{
	if(ms_numDrops < MAXDROPS){
		moving->dist = 0.0f;
		PlaceNew(moving->drop->x, moving->drop->y, SC(MINSIZE), 500.0f, 1);
	}
}

void
WaterDrops::NewDropMoving(WaterDrop *drop)
{
	WaterDropMoving *moving;
	for(moving = ms_dropsMoving; moving < &ms_dropsMoving[MAXDROPSMOVING]; moving++)
		if(moving->drop == NULL)
			goto found;
	return;
found:
	ms_numDropsMoving++;
	moving->drop = drop;
	moving->dist = 0.0f;
}

void
WaterDrops::FillScreen(int n)
{
	float x, y, time;
	WaterDrop *drop;

	if(!ms_initialised)
		return;
	ms_numDrops = 0;
	for(drop = &ms_drops[0]; drop < &ms_drops[MAXDROPS]; drop++){
		drop->active = 0;
		if(drop < &ms_drops[n]){
			x = rand() % ms_fbWidth;
			y = rand() % ms_fbHeight;
			time = rand() % (SC(MAXSIZE) - SC(MINSIZE)) + SC(MINSIZE);
			PlaceNew(x, y, time, 2000.0f, 1);
		}
	}
}

void
WaterDrops::FillScreenMoving(float amount)
{
	int n = (ms_vec.z <= 5.0f ? 1.0f : 1.5f)*amount*20.0f;
	float x, y, time;
	WaterDrop *drop;

	while(n--)
		if(ms_numDrops < MAXDROPS && ms_numDropsMoving < MAXDROPSMOVING){
			x = rand() % ms_fbWidth;
			y = rand() % ms_fbHeight;
			time = rand() % (SC(MAXSIZE) - SC(MINSIZE)) + SC(MINSIZE);
			drop = PlaceNew(x, y, time, 2000.0f, 1);
			if(drop)
				NewDropMoving(drop);
		}
}

void
WaterDrops::Clear(void)
{
	WaterDrop *drop;
	for(drop = &ms_drops[0]; drop < &ms_drops[MAXDROPS]; drop++)
		drop->active = 0;
	ms_numDrops = 0;
}

void
WaterDrops::Reset(void)
{
	Clear();
	ms_splashDuration = -1;
	ms_splashObject = NULL;
}

void
WaterDrops::RegisterSplash(CPlaceable_III *plc)
{
	RwV3d dist;
	if(isIII())
		RwV3dSub(&dist, &((CPlaceable_III*)plc)->matrix.matrix.pos, &ms_lastPos);
	else
		RwV3dSub(&dist, &plc->matrix.matrix.pos, &ms_lastPos);
	if(RwV3dLength(&dist) <= 20.0f){
		ms_splashDuration = 14;
		ms_splashObject = plc;
	}
}

bool
WaterDrops::NoRain(void)
{
	return CCullZones__CamNoRain() || CCullZones__PlayerNoRain();
}

void
WaterDrops::InitialiseRender(RwCamera *cam)
{
	char *path;

	srand(time(NULL));
	ms_fbWidth = cam->frameBuffer->width;
	ms_fbHeight = cam->frameBuffer->height;

	scaling = ms_fbHeight/480.0f;

	path = getpath("neo\\dropmask.tga");
	assert(path && "couldn't load 'neo\\dropmask.tga'");
	RwImage *img = readTGA(path);
	RwInt32 w, h, d, flags;
	RwImageFindRasterFormat(img, 4, &w, &h, &d, &flags);
	ms_maskRaster = RwRasterCreate(w, h, d, flags);
	ms_maskRaster = RwRasterSetFromImage(ms_maskRaster, img);
	assert(ms_maskRaster);
	ms_maskTex = RwTextureCreate(ms_maskRaster);
	ms_maskTex->filterAddressing = 0x3302;
	RwTextureAddRef(ms_maskTex);
	RwImageDestroy(img);

	IDirect3DVertexBuffer8 *vbuf;
	IDirect3DIndexBuffer8 *ibuf;
	RwD3DDevice->CreateVertexBuffer(MAXDROPS*4*sizeof(VertexTex2), D3DUSAGE_WRITEONLY, DROPFVF, D3DPOOL_MANAGED, &vbuf);
	ms_vertexBuf = vbuf;
	RwD3DDevice->CreateIndexBuffer(MAXDROPS*6*sizeof(short), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED, &ibuf);
	ms_indexBuf = ibuf;
	RwUInt16 *idx;
	ibuf->Lock(0, 0, (BYTE**)&idx, 0);
	for(int i = 0; i < MAXDROPS; i++){
		idx[i*6+0] = i*4+0;
		idx[i*6+1] = i*4+1;
		idx[i*6+2] = i*4+2;
		idx[i*6+3] = i*4+0;
		idx[i*6+4] = i*4+2;
		idx[i*6+5] = i*4+3;
	}
	ibuf->Unlock();

	for(w = 1; w < cam->frameBuffer->width; w <<= 1);
	for(h = 1; h < cam->frameBuffer->height; h <<= 1);
	ms_raster = RwRasterCreate(w, h, 0, 5);
	ms_tex = RwTextureCreate(ms_raster);
	ms_tex->filterAddressing = 0x3302;
	RwTextureAddRef(ms_tex);

	/* remove old effect in VC; not sure which it is though :/ */
	if(gtaversion == VC_10){
		MemoryVP::Nop(AddressByVersion<uint32_t>(0, 0, 0, 0x56185B, 0, 0), 5);
		MemoryVP::Nop(AddressByVersion<uint32_t>(0, 0, 0, 0x560D63, 0, 0), 5);
		MemoryVP::Nop(AddressByVersion<uint32_t>(0, 0, 0, 0x560EE3, 0, 0), 5);
	}

	ms_initialised = 1;
}

void
WaterDrops::AddToRenderList(WaterDrop *drop)
{
	static float xy[] = {
		-1.0f, -1.0f, -1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f, -1.0f
	};
	static float uv[] = {
		0.0f, 0.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 0.0f
	};
	int i;
	float scale;

	float u1_1, u1_2;
	float v1_1, v1_2;
	float tmp;

	tmp = drop->uvsize*(300.0f - 40.0f) + 40.0f;
	u1_1 = drop->x + ms_xOff - tmp;
	v1_1 = drop->y + ms_yOff - tmp;
	u1_2 = drop->x + ms_xOff + tmp;
	v1_2 = drop->y + ms_yOff + tmp;
	u1_1 = (u1_1 <= 0.0f ? 0.0f : u1_1) / ms_raster->width;
	v1_1 = (v1_1 <= 0.0f ? 0.0f : v1_1) / ms_raster->height;
	u1_2 = (u1_2 >= ms_fbWidth  ? ms_fbWidth  : u1_2) / ms_raster->width;
	v1_2 = (v1_2 >= ms_fbHeight ? ms_fbHeight : v1_2) / ms_raster->height;

	scale = drop->size * 0.5f;

	for(i = 0; i < 4; i++, ms_vertPtr++){
		ms_vertPtr->x = drop->x + xy[i*2]*scale + ms_xOff;
		ms_vertPtr->y = drop->y + xy[i*2+1]*scale + ms_yOff;
		ms_vertPtr->z = 0.0f;
		ms_vertPtr->rhw = 1.0f;
		ms_vertPtr->emissiveColor = D3DCOLOR_ARGB(drop->alpha, 0xFF, 0xFF, 0xFF);	// TODO
		ms_vertPtr->u0 = uv[i*2];
		ms_vertPtr->v0 = uv[i*2+1];
		ms_vertPtr->u1 = i >= 2 ? u1_2 : u1_1;
		ms_vertPtr->v1 = i % 3 == 0 ? v1_2 : v1_1;
	}
	ms_numBatchedDrops++;
}

void
WaterDrops::Render(void)
{
	WaterDrop *drop;

	// TODO: vehicle/cam...
	if(!ms_enabled || ms_numDrops <= 0 || CCutsceneMgr__ms_running)
		return;

	IDirect3DVertexBuffer8 *vbuf = (IDirect3DVertexBuffer8*)ms_vertexBuf;
	vbuf->Lock(0, 0, (BYTE**)&ms_vertPtr, 0);
	ms_numBatchedDrops = 0;
	for(drop = &ms_drops[0]; drop < &ms_drops[MAXDROPS]; drop++)
		if(drop->active)
			AddToRenderList(drop);
	vbuf->Unlock();

	RwRasterPushContext(ms_raster);
	RwRasterRenderFast(RwCameraGetRaster(rwcam), 0, 0);
	RwRasterPopContext();

	DefinedState();
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, 0);
	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, 0);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, 0);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);

	RwD3D8SetTexture(ms_maskTex, 0);
	RwD3D8SetTexture(ms_tex, 1);

	RwD3D8SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
	RwD3D8SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
	RwD3D8SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	RwD3D8SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
	RwD3D8SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TEXTURE);
	RwD3D8SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
	RwD3D8SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TEXTURE);
	RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MODULATE);
	RwD3D8SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	RwD3D8SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);
	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_CURRENT);
	RwD3D8SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	RwD3D8SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

	RwD3D8SetVertexShader(DROPFVF);
	RwD3D8SetStreamSource(0, vbuf, sizeof(VertexTex2));
	RwD3D8SetIndices(ms_indexBuf, 0);
	RwD3D8DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, ms_numBatchedDrops*4, 0, ms_numBatchedDrops*6);

	RwD3D8SetTexture(NULL, 1);
	RwD3D8SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D8SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	DefinedState();

	RwRenderStateSet(rwRENDERSTATEFOGENABLE, 0);
	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)1);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)1);
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)0);
}