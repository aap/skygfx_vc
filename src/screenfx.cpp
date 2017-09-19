#include "skygfx.h"

struct Grade
{
	float r, g, b, a;
};


// ADDRESS
static uint32_t CMBlur__CreateImmediateModeData_A = AddressByVersion<uint32_t>(0x50A800, 0, 0, 0x55F1D0, 0, 0);
WRAPPER void CMBlur__CreateImmediateModeData(RwCamera *cam, RwRect *rect) { VARJMP(CMBlur__CreateImmediateModeData_A); }

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

void
ScreenFX::Initialise(void)
{
	RwRect r = { 0, 0, 0, 0};
	int width, height;
	if(gradingPS == nil){
		HRSRC resource = FindResource(dllModule, MAKEINTRESOURCE(IDR_GRADINGPS), RT_RCDATA);
		RwUInt32 *shader = (RwUInt32*)LoadResource(dllModule, resource);
		RwD3D9CreatePixelShader(shader, &gradingPS);
		assert(gradingPS);
		FreeResource(shader);
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
	}
	r.w = width;
	r.h = height;
	CMBlur__CreateImmediateModeData(Scene.camera, &r);
}

void
ScreenFX::UpdateFrontBuffer(void)
{
	RwRasterPushContext(pFrontBuffer);
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
	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, blurVertices, 4, blurIndices, 6);
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
	if(curScreenWidth != RwRasterGetWidth(RwCameraGetRaster(Scene.camera)) ||
	   curScreenHeight != RwRasterGetHeight(RwCameraGetRaster(Scene.camera)) ||
	   curScreenDepth != RwRasterGetDepth(RwCameraGetRaster(Scene.camera)))
		Initialise();

	AVColourCorrection();
}
