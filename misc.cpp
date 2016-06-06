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



float WaterDrops::xOff, WaterDrops::yOff;	// not quite sure what these are
WaterDrop WaterDrops::drops[MAXDROPS];
int WaterDrops::numDrops;
int WaterDrops::initialised;

RwTexture *WaterDrops::maskTex;
RwTexture *WaterDrops::tex;	// TODO
RwRaster *WaterDrops::maskRaster;
RwRaster *WaterDrops::raster;	// TODO

int WaterDrops::fbWidth, WaterDrops::fbHeight;

void *WaterDrops::vertexBuf;
void *WaterDrops::indexBuf;
VertexTex2 *WaterDrops::vertPtr;
int WaterDrops::numBatchedDrops;

#define DROPFVF (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX2)

IDirect3DDevice8 *&RwD3DDevice = *AddressByVersion<IDirect3DDevice8**>(0x662EF0, 0x662EF0, 0x67342C, 0x7897A8, 0x7897B0, 0x7887B0);


float &CTimer__ms_fTimeStep = *AddressByVersion<float*>(0, 0, 0, 0x975424, 0, 0);

void
WaterDrop::Fade(void)
{
	int delta = CTimer__ms_fTimeStep * 1000.0f / 50.0f;
	this->time += delta;
	if(this->time >= this->ttl){
		WaterDrops::numDrops--;
		this->active = 0;
	}else if(this->fades)
		this->alpha = 255 - time/ttl * 255;
}

void
WaterDrops::Initialise(RwCamera *cam)
{
	char *path;

	srand(time(NULL));
	fbWidth = cam->frameBuffer->width;
	fbHeight = cam->frameBuffer->height;

	path = getpath("neo\\dropmask.tga");
	assert(path && "couldn't load 'neo\\dropmask.tga'");
	RwImage *img = readTGA(path);
	RwInt32 w, h, d, flags;
	RwImageFindRasterFormat(img, 4, &w, &h, &d, &flags);
	maskRaster = RwRasterCreate(w, h, d, flags);
	maskRaster = RwRasterSetFromImage(maskRaster, img);
	assert(maskRaster);
	maskTex = RwTextureCreate(maskRaster);
	maskTex->filterAddressing = 0x3302;
	RwTextureAddRef(maskTex);
	RwImageDestroy(img);

	IDirect3DVertexBuffer8 *vbuf;
	IDirect3DIndexBuffer8 *ibuf;
	RwD3DDevice->CreateVertexBuffer(MAXDROPS*4*sizeof(VertexTex2), D3DUSAGE_WRITEONLY, DROPFVF, D3DPOOL_MANAGED, &vbuf);
	vertexBuf = vbuf;
	RwD3DDevice->CreateIndexBuffer(MAXDROPS*6*sizeof(short), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED, &ibuf);
	indexBuf = ibuf;
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
	raster = RwRasterCreate(w, h, 0, 5);
	tex = RwTextureCreate(raster);
	tex->filterAddressing = 0x3302;
	RwTextureAddRef(tex);

	initialised = 1;
}

void
WaterDrops::PlaceNew(float x0, float y0, float size, float ttl, bool fades)
{
	WaterDrop *drop;
	int i;

	// TODO: don't place unconditionally

	for(i = 0, drop = drops; i < MAXDROPS; i++, drop++)
		if(drops[i].active == 0)
			goto found;
	return;
found:
	numDrops++;
	drop->x0 = x0;
	drop->y0 = y0;
	drop->size = size;
	drop->uvsize = (15.0f - size + 1.0f) / (15.0f - 4.0f + 1.0f);	// lolwut
	drop->fades = fades;
	drop->active = 1;
	drop->alpha = 0xFF;
	drop->time = 0.0f;
	drop->ttl = ttl;
}

// VC
RwCamera *&rwcam = *(RwCamera**)0x8100BC;

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
	u1_1 = drop->x0 + xOff - tmp;
	v1_1 = drop->y0 + yOff - tmp;
	u1_2 = drop->x0 + xOff + tmp;
	v1_2 = drop->y0 + yOff + tmp;
	u1_1 = (u1_1 <= 0.0f ? 0.0f : u1_1) / raster->width;
	v1_1 = (v1_1 <= 0.0f ? 0.0f : v1_1) / raster->height;
	u1_2 = (u1_2 >= fbWidth  ? fbWidth  : u1_2) / raster->width;
	v1_2 = (v1_2 >= fbHeight ? fbHeight : v1_2) / raster->height;

	scale = drop->size * 0.5f;

	for(i = 0; i < 4; i++, vertPtr++){
		vertPtr->x = drop->x0 + xy[i*2]*scale + xOff;
		vertPtr->y = drop->y0 + xy[i*2+1]*scale + yOff;
		vertPtr->z = 0.0f;
		vertPtr->rhw = 1.0f;
		vertPtr->emissiveColor = D3DCOLOR_ARGB(drop->alpha, 0xFF, 0xFF, 0xFF);	// TODO
		vertPtr->u0 = uv[i*2];
		vertPtr->v0 = uv[i*2+1];
		vertPtr->u1 = i >= 2 ? u1_2 : u1_1;
		vertPtr->v1 = i % 3 == 0 ? v1_2 : v1_1;
	}
	numBatchedDrops++;
}

void
WaterDrops::Clear(void)
{
	WaterDrop *drop;
	for(drop = &drops[0]; drop < &drops[MAXDROPS]; drop++)
		drop->active = 0;
	numDrops = 0;
}

void
WaterDrops::FillScreen(int n)
{
	float x0, y0, x1;
	WaterDrop *drop;

	if(!initialised)
		return;
	numDrops = 0;
	for(drop = &drops[0]; drop < &drops[MAXDROPS]; drop++){
		drop->active = 0;
		if(drop < &drops[n]){
			x0 = rand() % fbWidth;
			y0 = rand() % fbHeight;
			x1 = rand() % ((int)floor(15.0f) - 4) + 4;
			PlaceNew(x0, y0, x1, 2000.0f, 1);
		}
	}
}

void
WaterDrops::Process(void)
{
	WaterDrop *drop;
	if(!initialised)
		Initialise(rwcam);

	{
		static bool keystate = false;
		if(GetAsyncKeyState(VK_TAB) & 0x8000){
			if(!keystate){
				keystate = true;
				FillScreen(300);
			}
		}else
			keystate = false;
	}

	for(drop = &drops[0]; drop < &drops[MAXDROPS]; drop++)
		if(drop->active)
			drop->Fade();
			
}

void
WaterDrops::Render(void)
{
	WaterDrop *drop;
	IDirect3DVertexBuffer8 *vbuf = (IDirect3DVertexBuffer8*)vertexBuf;
	vbuf->Lock(0, 0, (BYTE**)&vertPtr, 0);
	numBatchedDrops = 0;
	for(drop = &drops[0]; drop < &drops[MAXDROPS]; drop++)
		if(drop->active)
			AddToRenderList(drop);
	vbuf->Unlock();

	RwCameraEndUpdate(rwcam);
	RwRasterPushContext(raster);
	RwRasterRenderFast(RwCameraGetRaster(rwcam), 0, 0);
	RwRasterPopContext();
	RwCameraBeginUpdate(rwcam);

	DefinedState();
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, 0);
	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, 0);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, 0);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);

	RwD3D8SetTexture(maskTex, 0);
	RwD3D8SetTexture(tex, 1);

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
	RwD3D8SetIndices(indexBuf, 0);
	RwD3D8DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, numBatchedDrops*4, 0, numBatchedDrops*6);

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