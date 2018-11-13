#include "skygfx.h"

struct Grade
{
	float r, g, b, a;
};


RwRaster *ScreenFX::pFrontBuffer;
void *ScreenFX::gradingPS;

bool ScreenFX::m_bYCbCrFilter = false;
float ScreenFX::m_lumaScale = 219.0f/255.0f;
float ScreenFX::m_lumaOffset = 16.0f/255.0f;
float ScreenFX::m_cbScale = 1.23f;
float ScreenFX::m_cbOffset = 0.0f;
float ScreenFX::m_crScale = 1.23f;
float ScreenFX::m_crOffset = 0.0f;

static int curScreenWidth;
static int curScreenHeight;
static int curScreenDepth;

RwIm2DVertex ScreenFX::screenVertices[4];
RwImVertexIndex ScreenFX::screenIndices[6] = { 0, 1, 2, 0, 2, 3 };

void (*ScreenFX::nullsub_orig)(void);

void
ScreenFX::CreateImmediateModeData(RwCamera *cam, RwRect *rect)
{
	float zero = 0.0f;
	float xmax = rect->w;
	float ymax = rect->h;
	// whatever this is...
	if(RwRasterGetDepth(RwCameraGetRaster(cam)) == 16){
		zero += 0.5f;
		xmax += 0.5f;
		ymax += 0.5f;
	}else{
		zero -= 0.5f;
		xmax -= 0.5f;
		ymax -= 0.5f;
	}
	const float recipz = 1.0f/RwCameraGetNearClipPlane(cam);
	RwIm2DVertexSetScreenX(&screenVertices[0], zero);
	RwIm2DVertexSetScreenY(&screenVertices[0], zero);
	RwIm2DVertexSetScreenZ(&screenVertices[0], RwIm2DGetNearScreenZ());
	RwIm2DVertexSetRecipCameraZ(&screenVertices[0], recipz);
	RwIm2DVertexSetU(&screenVertices[0], 0.0f, recipz);
	RwIm2DVertexSetV(&screenVertices[0], 0.0f, recipz);
	RwIm2DVertexSetIntRGBA(&screenVertices[0], 255, 255, 255, 255);

	RwIm2DVertexSetScreenX(&screenVertices[1], zero);
	RwIm2DVertexSetScreenY(&screenVertices[1], ymax);
	RwIm2DVertexSetScreenZ(&screenVertices[1], RwIm2DGetNearScreenZ());
	RwIm2DVertexSetRecipCameraZ(&screenVertices[1], recipz);
	RwIm2DVertexSetU(&screenVertices[1], 0.0f, recipz);
	RwIm2DVertexSetV(&screenVertices[1], 1.0f, recipz);
	RwIm2DVertexSetIntRGBA(&screenVertices[1], 255, 255, 255, 255);

	RwIm2DVertexSetScreenX(&screenVertices[2], xmax);
	RwIm2DVertexSetScreenY(&screenVertices[2], ymax);
	RwIm2DVertexSetScreenZ(&screenVertices[2], RwIm2DGetNearScreenZ());
	RwIm2DVertexSetRecipCameraZ(&screenVertices[2], recipz);
	RwIm2DVertexSetU(&screenVertices[2], 1.0f, recipz);
	RwIm2DVertexSetV(&screenVertices[2], 1.0f, recipz);
	RwIm2DVertexSetIntRGBA(&screenVertices[2], 255, 255, 255, 255);

	RwIm2DVertexSetScreenX(&screenVertices[3], xmax);
	RwIm2DVertexSetScreenY(&screenVertices[3], zero);
	RwIm2DVertexSetScreenZ(&screenVertices[3], RwIm2DGetNearScreenZ());
	RwIm2DVertexSetRecipCameraZ(&screenVertices[3], recipz);
	RwIm2DVertexSetU(&screenVertices[3], 1.0f, recipz);
	RwIm2DVertexSetV(&screenVertices[3], 0.0f, recipz);
	RwIm2DVertexSetIntRGBA(&screenVertices[3], 255, 255, 255, 255);
}

// This is called by the seam fixer even when we don't have D3D9!!
void
ScreenFX::Initialise(void)
{
	RwRect r = { 0, 0, 0, 0};
	int width, height;
	// MULTIPLE INIT
	if(RwD3D9Supported()){
		#include "gradingPS.h"
		RwD3D9CreatePixelShader((RwUInt32*)g_ps20_main, &gradingPS);
		assert(gradingPS);
	}
	if(pFrontBuffer){
		RwRasterDestroy(pFrontBuffer);
		pFrontBuffer = nil;
	}
	if(pFrontBuffer == nil){
		curScreenWidth = RwRasterGetWidth(RwCameraGetRaster(Scene.camera));
		curScreenHeight = RwRasterGetHeight(RwCameraGetRaster(Scene.camera));
		curScreenDepth = RwRasterGetDepth(RwCameraGetRaster(Scene.camera));
		width = 1;
		height = 1;
		while(width < curScreenWidth) width *= 2;
		while(height < curScreenHeight) height *= 2;
		pFrontBuffer = RwRasterCreate(width, height, curScreenDepth, rwRASTERTYPECAMERATEXTURE);
//printf("create frontbuffer: %p %d\n", pFrontBuffer, pFrontBuffer->cType);
//fflush(stdout);
	}
	r.w = width;
	r.h = height;
	CreateImmediateModeData(Scene.camera, &r);
}

//RwRaster *&_RwD3D8CurrentRasterTarget = *(RwRaster**)0x660F78;

void
ScreenFX::UpdateFrontBuffer(void)
{
//printf("render frontbuffer: %p %d\n", pFrontBuffer, pFrontBuffer->cType);
//fflush(stdout);
//	assert(pFrontBuffer->cType == rwRASTERTYPECAMERATEXTURE);
	RwRasterPushContext(pFrontBuffer);
//	assert(_RwD3D8CurrentRasterTarget);
//	assert(_RwD3D8CurrentRasterTarget->cType == rwRASTERTYPECAMERATEXTURE);
	RwRasterRenderFast(Scene.camera->frameBuffer, 0, 0);
	RwRasterPopContext();
}

void
ScreenFX::AVColourCorrection(void)
{
	static RwMatrix RGB2YUV = {
		{  0.299f,	-0.168736f,	 0.500f }, 0,
		{  0.587f,	-0.331264f,	-0.418688f }, 0,
		{  0.114f,	 0.500f,	-0.081312f }, 0,
		{  0.000f,	 0.000f,	 0.000f }, 0,
	};
	
	static RwMatrix YUV2RGB = {
		{  1.000f,	 1.000f,	 1.000f }, 0,
		{  0.000f,	-0.344136f,	 1.772f }, 0,
		{  1.402f,	-0.714136f,	 0.000f }, 0,
		{  0.000f,	 0.000f,	 0.000f }, 0,
	};

	if(!m_bYCbCrFilter)
		return;

	UpdateFrontBuffer();

	DefinedState();

	RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERNEAREST);
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)FALSE);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)pFrontBuffer);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)FALSE);

	RwMatrix m;
	m.right.x = m_lumaScale;
	m.up.x = 0.0f;
	m.at.x = 0.0f;
	m.pos.x = m_lumaOffset;
	m.right.y = 0.0f;
	m.up.y = m_cbScale;
	m.at.y = 0.0f;
	m.pos.y = m_cbOffset;
	m.right.z = 0.0f;
	m.up.z = 0.0f;
	m.at.z = m_crScale;
	m.pos.z = m_crOffset;
	m.flags = 0;
	RwMatrixOptimize(&m, nil);
	RwMatrix m2;

	RwMatrixMultiply(&m2, &RGB2YUV, &m);
	RwMatrixMultiply(&m, &m2, &YUV2RGB);
	Grade red, green, blue;
	red.r = m.right.x;
	red.g = m.up.x;
	red.b = m.at.x;
	red.a = m.pos.x;
	green.r = m.right.y;
	green.g = m.up.y;
	green.b = m.at.y;
	green.a = m.pos.y;
	blue.r = m.right.z;
	blue.g = m.up.z;
	blue.b = m.at.z;
	blue.a = m.pos.z;

	RwD3D9SetPixelShaderConstant(0, &red, 1);
	RwD3D9SetPixelShaderConstant(1, &green, 1);
	RwD3D9SetPixelShaderConstant(2, &blue, 1);

	RwD3D9SetIm2DPixelShader(gradingPS);
	// don't use these
	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, screenVertices, 4, screenIndices, 6);
	RwD3D9SetIm2DPixelShader(nil);

	RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)TRUE);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)NULL);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
}

void
ScreenFX::Render(void)
{
	static int render = -1;
	if(render == -1)
		render = RwD3D9Supported();
	if(!render)
		return;

	ScreenFX::Update();

	nullsub_orig();

	AVColourCorrection();
}

void
ScreenFX::Update(void)
{
	if(curScreenWidth != RwRasterGetWidth(RwCameraGetRaster(Scene.camera)) ||
	   curScreenHeight != RwRasterGetHeight(RwCameraGetRaster(Scene.camera)) ||
	   curScreenDepth != RwRasterGetDepth(RwCameraGetRaster(Scene.camera)))
		Initialise();
}
