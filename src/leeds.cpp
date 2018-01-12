#include "skygfx.h"
#include "d3d8.h"
#include "d3d8types.h"

// World pipe and MBlur

extern D3DMATERIAL8 d3dmaterial;
extern RwReal lastDiffuse;
extern RwReal lastAmbient;
extern RwUInt32 lastWasTextured;
extern RwRGBA lastColor;
extern float lastAmbRed;
extern float lastAmbGreen;
extern float lastAmbBlue;

extern IDirect3DDevice8 *&RwD3DDevice;
extern D3DMATERIAL8 &lastmaterial;
extern D3DCOLORVALUE &AmbientSaturated;

float &CTimeCycle__m_fCurrentAmbientRed = *AddressByVersion<float*>(0, 0, 0, 0x978D18, 0, 0);
float &CTimeCycle__m_fCurrentAmbientGreen = *AddressByVersion<float*>(0, 0, 0, 0xA0D8CC, 0, 0);
float &CTimeCycle__m_fCurrentAmbientBlue = *AddressByVersion<float*>(0, 0, 0, 0xA0FCA8, 0, 0);

//int doTextures = 1;
//float prelitMult = 0.5f;
//float prelitAdd = 0.1f;

RwBool
leedsSetSurfaceProps(RwRGBA *color, RwSurfaceProperties *surfProps, RwUInt32 flags)
{
	static D3DCOLORVALUE black = { 0, 0, 0, 0 };
	RwRGBA c = *color;

	// REMOVE: we do this in InitialiseGame_hook now
	if(config.leedsWorldAmbTweak < 0) config.leedsWorldAmbTweak = 1.0f;
	if(config.leedsWorldEmissTweak < 0) config.leedsWorldEmissTweak = 0.0f;

	RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);
	d3dmaterial.Diffuse = black;
	d3dmaterial.Diffuse.a = color->alpha/255.0f;


	if(isIII()){
		c.red *= config.currentAmbientMultRed;
		c.green *= config.currentAmbientMultGreen;
		c.blue *= config.currentAmbientMultBlue;
	}else{
		c.red *= CTimeCycle__m_fCurrentAmbientRed;
		c.green *= CTimeCycle__m_fCurrentAmbientGreen;
		c.blue *= CTimeCycle__m_fCurrentAmbientBlue;
	}
c.red *= config.leedsWorldAmbTweak;
c.green *= config.leedsWorldAmbTweak;
c.blue *= config.leedsWorldAmbTweak;

	RwD3D8SetRenderState(D3DRS_AMBIENT, D3DCOLOR_ARGB(0, c.red, c.green, c.blue));
	RwD3D8SetRenderState(D3DRS_COLORVERTEX, 1);
	RwD3D8SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_COLOR1);
	d3dmaterial.Ambient = black;

	RwD3D8SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL);
	d3dmaterial.Emissive.r = AmbientSaturated.r * 0.5f;
	d3dmaterial.Emissive.g = AmbientSaturated.g * 0.5f;
	d3dmaterial.Emissive.b = AmbientSaturated.b * 0.5f;

d3dmaterial.Emissive.r += config.leedsWorldEmissTweak;
d3dmaterial.Emissive.g += config.leedsWorldEmissTweak;
d3dmaterial.Emissive.b += config.leedsWorldEmissTweak;
if(d3dmaterial.Emissive.r > 1.0f) d3dmaterial.Emissive.r = 1.0f;
if(d3dmaterial.Emissive.g > 1.0f) d3dmaterial.Emissive.g = 1.0f;
if(d3dmaterial.Emissive.b > 1.0f) d3dmaterial.Emissive.b = 1.0f;

	if(memcmp(&lastmaterial, &d3dmaterial, sizeof(D3DMATERIAL8)) != 0){
		lastmaterial = d3dmaterial;
		return RwD3DDevice->SetMaterial(&d3dmaterial) >= 0;
	}
	return true;
}

void
leedsRenderCallback(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
	_rwD3D8EnableClippingIfNeeded(object, type);

	int lighting;
	RwD3D8GetRenderState(D3DRS_LIGHTING, &lighting);

	if(!(flags & (rpGEOMETRYTEXTURED|rpGEOMETRYTEXTURED2)))
		RwD3D8SetTexture(NULL, 0);
	RwBool curalpha = rwD3D8RenderStateIsVertexAlphaEnable();
	void *vertbuf = (void*)0xFFFFFFFF;
	RxD3D8ResEntryHeader *header = (RxD3D8ResEntryHeader*)&repEntry[1];
	RxD3D8InstanceData *inst = (RxD3D8InstanceData*)&header[1];
	RwD3D8SetPixelShader(NULL);
	RwD3D8SetVertexShader(inst->vertexShader);

	for(int i = 0; i < header->numMeshes; i++){
		if(flags & (rpGEOMETRYTEXTURED|rpGEOMETRYTEXTURED2)){
//if(doTextures){
			RwD3D8SetTexture(inst->material->texture, 0);
			RwD3D8SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE2X);
//}else
//RwD3D8SetTexture(nil, 0);
		}
		RwBool a = inst->vertexAlpha || inst->material->color.alpha != 0xFFu;
		if(curalpha != a)
			rwD3D8RenderStateVertexAlphaEnable(curalpha = a);
		if(lighting){
			RwD3D8SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, inst->vertexAlpha != 0);
			leedsSetSurfaceProps(&inst->material->color, &inst->material->surfaceProps, flags);
		}
		if(vertbuf != inst->vertexBuffer)
			RwD3D8SetStreamSource(0, vertbuf = inst->vertexBuffer, inst->stride);

		drawDualPass(inst);
		inst++;
	}

	d3dmaterial.Emissive.r = 0.0f;
	d3dmaterial.Emissive.g = 0.0f;
	d3dmaterial.Emissive.b = 0.0f;
}





bool &CMBlur::ms_bJustInitialised = *AddressByVersion<bool*>(0x95CDAB, 0, 0, 0xA10B71, 0, 0);;
bool &CMBlur::BlurOn = *AddressByVersion<bool*>(0x95CDAD, 0x5FDB84, 0x60AB7C, 0x697D54, 0x697D54, 0x696D5C);
RwRaster *&CMBlur::pFrontBuffer = *AddressByVersion<RwRaster**>(0x8E2C48, 0x8E2CFC, 0x8F2E3C, 0x9753A4, 0x9753AC, 0x9743AC);
RwRaster *CMBlur::pBackBuffer;

RwRaster *CMBlur::ms_pRadiosityRaster1;
RwRaster *CMBlur::ms_pRadiosityRaster2;
RwIm2DVertex CMBlur::ms_radiosityVerts[44];
RwImVertexIndex CMBlur::ms_radiosityIndices[7*6] = {
	0, 1, 2, 2, 1, 3,
		4, 5, 2, 2, 5, 3,
	4, 5, 6, 6, 5, 7,
		8, 9, 6, 6, 9, 7,
	8, 9, 10, 10, 9, 11,
		12, 13, 10, 10, 13, 11,
	12, 13, 14, 14, 13, 15,
};


extern RwD3D8Vertex *blurVertices;
extern RwImVertexIndex *blurIndices;

RwD3D8Vertex screen_vertices[24];
RwImVertexIndex screen_indices[] = {
	0, 1, 2, 2, 1, 3,
	4, 5, 6, 6, 5, 7,
	8, 9, 10, 10, 9, 11,
};

void
makequad(RwCamera *cam, RwIm2DVertex *v, int width, int height, int texwidth = 0, int texheight = 0)
{
	float w, h, tw, th;
	w = width;
	h = height;
	tw = texwidth > 0 ? texwidth : w;
	th = texheight > 0 ? texheight : h;

	const float nearz = RwIm2DGetNearScreenZ();
	const float recipz = 1.0f/RwCameraGetNearClipPlane(cam);

	RwIm2DVertexSetScreenX(&v[0], 0);
	RwIm2DVertexSetScreenY(&v[0], 0);
	RwIm2DVertexSetScreenZ(&v[0], 0);
	RwIm2DVertexSetCameraZ(&v[0], nearz);
	RwIm2DVertexSetRecipCameraZ(&v[0], recipz);
	RwIm2DVertexSetU(&v[0], 0.5f / tw, recipz);
	RwIm2DVertexSetV(&v[0], 0.5f / th, recipz);
	RwIm2DVertexSetIntRGBA(&v[0], 255, 255, 255, 255);

	RwIm2DVertexSetScreenX(&v[1], 0);
	RwIm2DVertexSetScreenY(&v[1], h);
	RwIm2DVertexSetScreenZ(&v[1], 0);
	RwIm2DVertexSetCameraZ(&v[1], nearz);
	RwIm2DVertexSetRecipCameraZ(&v[1], recipz);
	RwIm2DVertexSetU(&v[1], 0.5f / tw, recipz);
	RwIm2DVertexSetV(&v[1], (h + 0.5f) / th, recipz);
	RwIm2DVertexSetIntRGBA(&v[1], 255, 255, 255, 255);

	RwIm2DVertexSetScreenX(&v[2], w);
	RwIm2DVertexSetScreenY(&v[2], 0);
	RwIm2DVertexSetScreenZ(&v[2], 0);
	RwIm2DVertexSetCameraZ(&v[2], nearz);
	RwIm2DVertexSetRecipCameraZ(&v[2], recipz);
	RwIm2DVertexSetU(&v[2], (w + 0.5f) / tw, recipz);
	RwIm2DVertexSetV(&v[2], 0.5f / th, recipz);
	RwIm2DVertexSetIntRGBA(&v[2], 255, 255, 255, 255);

	RwIm2DVertexSetScreenX(&v[3], w);
	RwIm2DVertexSetScreenY(&v[3], h);
	RwIm2DVertexSetScreenZ(&v[3], 0);
	RwIm2DVertexSetCameraZ(&v[3], nearz);
	RwIm2DVertexSetRecipCameraZ(&v[3], recipz);
	RwIm2DVertexSetU(&v[3], (w + 0.5f) / tw, recipz);
	RwIm2DVertexSetV(&v[3], (h + 0.5f) / th, recipz);
	RwIm2DVertexSetIntRGBA(&v[3], 255, 255, 255, 255);
}

void
CMBlur::Initialise(void)
{
	if(pBackBuffer){
		RwRasterDestroy(pBackBuffer);
		pBackBuffer = nil;
	}
	if(pBackBuffer == nil)
		pBackBuffer = RwRasterCreate(RwRasterGetWidth(pFrontBuffer),
			RwRasterGetHeight(pFrontBuffer),
			RwRasterGetDepth(pFrontBuffer),
			rwRASTERTYPECAMERATEXTURE);
}

void
CMBlur::OverlayRender_leeds(RwCamera *cam, RwRaster *frontbuf, RwRGBA *col, uint8 type)
{
	int i;
	int bufw, bufh;
	int screenw, screenh;
	int blurintensity;
	int motionblurintensity;

	DefinedState();
	RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERNEAREST);
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, 0);
	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, 0);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, 0);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);

	bufw = RwRasterGetWidth(frontbuf);
	bufh = RwRasterGetHeight(frontbuf);
	screenw = RwCameraGetRaster(cam)->width;
	screenh = RwCameraGetRaster(cam)->height;

	blurintensity = config.currentBlurAlpha*0.8f;
	motionblurintensity = 32;

	// Blur
	makequad(cam, screen_vertices, screenw, screenh, bufw, bufh);
	for(i = 0; i < 4; i++){
		screen_vertices[i].x += config.currentBlurOffset;
		RwIm2DVertexSetIntRGBA(&screen_vertices[i], 255, 255, 255, blurintensity);
	}
	makequad(cam, screen_vertices+4, screenw, screenh, bufw, bufh);
	for(i = 4; i < 8; i++){
		screen_vertices[i].x += config.currentBlurOffset;
		screen_vertices[i].y += config.currentBlurOffset;
		RwIm2DVertexSetIntRGBA(&screen_vertices[i], 255, 255, 255, blurintensity);
	}
	makequad(cam, screen_vertices+8, screenw, screenh, bufw, bufh);
	for(i = 8; i < 12; i++){
		screen_vertices[i].y += config.currentBlurOffset;
		RwIm2DVertexSetIntRGBA(&screen_vertices[i], 255, 255, 255, blurintensity);
	}

	// Colour filter
	makequad(cam, screen_vertices+12, screenw, screenh, bufw, bufh);
	for(i = 12; i < 16; i++)
		RwIm2DVertexSetIntRGBA(&screen_vertices[i], col->red, col->green, col->blue, 255);

	// Blend with last frame
	makequad(cam, screen_vertices+16, screenw, screenh, bufw, bufh);
	for(i = 16; i < 20; i++)
		RwIm2DVertexSetIntRGBA(&screen_vertices[i], 255, 255, 255, motionblurintensity);

/*
	makequad(cam, screen_vertices+20, screenw, screenh, bufw, bufh);
	for(i = 20; i < 24; i++)
		RwIm2DVertexSetIntRGBA(&screen_vertices[i], 0, 0, 0, 0);
*/

	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, pBackBuffer);

	// Blur
	if(config.trailsBlur){
		RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
		RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
		RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, screen_vertices, 12, screen_indices, 3*6);
	}

	// Apply colour filter
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, screen_vertices+12, 4, screen_indices, 6);

	// Motion blur
	if(ms_bJustInitialised)
		ms_bJustInitialised = false;
	else if(config.trailsMotionBlur){
		RwRenderStateSet(rwRENDERSTATETEXTURERASTER, frontbuf);
		RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
		RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
		RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, screen_vertices+16, 4, screen_indices, 6);
	}
}

void
CMBlur::RadiosityInit(RwCamera *cam)
{
	ms_pRadiosityRaster1 = RwRasterCreate(256, 128, RwCameraGetRaster(cam)->depth, rwRASTERTYPECAMERATEXTURE);
	ms_pRadiosityRaster2 = RwRasterCreate(256, 128, RwCameraGetRaster(cam)->depth, rwRASTERTYPECAMERATEXTURE);
//	ms_pRadiosityRaster1 = RwRasterCreate(pFrontBuffer->width, pFrontBuffer->height, pFrontBuffer->depth, rwRASTERTYPECAMERATEXTURE);
//	ms_pRadiosityRaster2 = RwRasterCreate(pFrontBuffer->width, pFrontBuffer->height, pFrontBuffer->depth, rwRASTERTYPECAMERATEXTURE);
}

void
CMBlur::RadiosityCreateImmediateData(RwCamera *cam)
{
	static float uOffsets[] = { -1.0f, 1.0f, 0.0f, 0.0f,   -1.0f, 1.0f, -1.0f, 1.0f };
	static float vOffsets[] = { 0.0f, 0.0f, -1.0f, 1.0f,   -1.0f, -1.0f, 1.0f, 1.0f };
	int i;
	RwUInt32 c;
	int w, h;

	w = 256;
	h = 128;

	// TODO: tex coords correct?
	makequad(cam, ms_radiosityVerts, 256, 128);
	makequad(cam, ms_radiosityVerts+4, RwCameraGetRaster(cam)->width, RwCameraGetRaster(cam)->height);

	// black vertices; at 8
	for(i = 0; i < 4; i++){
		ms_radiosityVerts[i+8] = ms_radiosityVerts[i];
		ms_radiosityVerts[i+8].emissiveColor = 0;
	}

	// two sets blur vertices; at 12
	c = D3DCOLOR_ARGB(0xFF, 36, 36, 36);
	for(i = 0; i < 2*4*4; i++){
		ms_radiosityVerts[i+12] = ms_radiosityVerts[i%4];
		ms_radiosityVerts[i+12].emissiveColor = c;
		switch(i%4){
		case 0:
			ms_radiosityVerts[i+12].u = (uOffsets[i/4] + 0.5f) / w;
			ms_radiosityVerts[i+12].v = (vOffsets[i/4] + 0.5f) / h;
			break;
		case 1:
			ms_radiosityVerts[i+12].u = (uOffsets[i/4] + 0.5f) / w;
			ms_radiosityVerts[i+12].v = (h + vOffsets[i/4] + 0.5f) / h;
			break;
		case 2:
			ms_radiosityVerts[i+12].u = (w + uOffsets[i/4] + 0.5f) / w;
			ms_radiosityVerts[i+12].v = (vOffsets[i/4] + 0.5f) / h;
			break;
		case 3:
			ms_radiosityVerts[i+12].u = (w + uOffsets[i/4] + 0.5f) / w;
			ms_radiosityVerts[i+12].v = (h + vOffsets[i/4] + 0.5f) / h;
			break;
		}
	}
}

void
CMBlur::RadiosityRender(RwCamera *cam, int limit, int intensity)
{
	int i;
	RwRaster *fb;
	RwRaster *fb1, *fb2, *tmp;

	if(ms_pRadiosityRaster1 == nil || ms_pRadiosityRaster2 == nil)
		RadiosityInit(cam);

	fb = RwCameraGetRaster(cam);
	assert(ms_pRadiosityRaster1);
	assert(ms_pRadiosityRaster1);

	RwRect r;
	r.x = 0;
	r.y = 0;
	r.w = 256;
	r.h = 128;

//	RwRasterPushContext(ms_pRadiosityRaster2);
//	RwRasterRenderScaled(fb, &r);
////	RwRasterRenderFast(fb, 0, 0);
//	RwRasterPopContext();

	RwD3D8SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);

	RadiosityCreateImmediateData(cam);


	// Have to jump through extra hoops to downsample because RwRasterRender* doesn't seem to work
	static RwIm2DVertex tmpverts[4];
	makequad(cam, tmpverts, RwCameraGetRaster(cam)->width, RwCameraGetRaster(cam)->height,
		RwRasterGetWidth(pBackBuffer), RwRasterGetHeight(pBackBuffer));
	RwIm2DVertexSetScreenX(&tmpverts[0], 0);
	RwIm2DVertexSetScreenY(&tmpverts[0], 0);
	RwIm2DVertexSetScreenX(&tmpverts[1], 0);
	RwIm2DVertexSetScreenY(&tmpverts[1], 128);
	RwIm2DVertexSetScreenX(&tmpverts[2], 256);
	RwIm2DVertexSetScreenY(&tmpverts[2], 0);
	RwIm2DVertexSetScreenX(&tmpverts[3], 256);
	RwIm2DVertexSetScreenY(&tmpverts[3], 128);

	RwCameraEndUpdate(cam);
	RwCameraSetRaster(cam, ms_pRadiosityRaster2);
	RwCameraBeginUpdate(cam);

	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)0);
	RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, pBackBuffer);
	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, tmpverts, 4, ms_radiosityIndices, 6);


	for(i = 0; i < 4; i++)
		ms_radiosityVerts[i].emissiveColor = D3DCOLOR_ARGB(limit/2, 255, 255, 255);

	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, nil);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
	RwD3D8SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_REVSUBTRACT);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, ms_radiosityVerts, 4, ms_radiosityIndices, 6);
	RwD3D8SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);

	fb1 = ms_pRadiosityRaster1;
	fb2 = ms_pRadiosityRaster2;
	for(i = 0; i < 4; i++){
		RwCameraEndUpdate(cam);
		RwCameraSetRaster(cam, fb1);
		RwCameraBeginUpdate(cam);

		RwRenderStateSet(rwRENDERSTATETEXTURERASTER, nil);
		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)0);
		RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, ms_radiosityVerts+8, 4, ms_radiosityIndices, 6);

		RwRenderStateSet(rwRENDERSTATETEXTURERASTER, fb2);
		RwRenderStateSet(rwRENDERSTATETEXTUREADDRESSU, (void*)rwTEXTUREADDRESSCLAMP);
		RwRenderStateSet(rwRENDERSTATETEXTUREADDRESSV, (void*)rwTEXTUREADDRESSCLAMP);
		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
		RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
		RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
		if((i % 2) == 0)
			RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, ms_radiosityVerts+12, 4*4, ms_radiosityIndices, 6*7);
		else
			RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, ms_radiosityVerts+28, 4*4, ms_radiosityIndices, 6*7);

		tmp = fb1;
		fb1 = fb2;
		fb2 = tmp;
	}

	RwCameraEndUpdate(cam);
	RwCameraSetRaster(cam, fb);
	RwCameraBeginUpdate(cam);

	for(i = 4; i < 8; i++)
		ms_radiosityVerts[i].emissiveColor = D3DCOLOR_ARGB(intensity*4, 255, 255, 255);

	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, fb2);
	RwRenderStateSet(rwRENDERSTATETEXTUREADDRESSU, (void*)rwTEXTUREADDRESSCLAMP);
	RwRenderStateSet(rwRENDERSTATETEXTUREADDRESSV, (void*)rwTEXTUREADDRESSCLAMP);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, ms_radiosityVerts+4, 4, ms_radiosityIndices, 6);
	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, ms_radiosityVerts+4, 4, ms_radiosityIndices, 6);

	RwD3D8SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
}

void
CMBlur::MotionBlurRender_custom(RwCamera *cam, uint8 red, uint8 green, uint8 blue, uint8 alpha, uint8 type/*, uint8 mbluralpha*/)
{
	RwRGBA col;
	if(BlurOn){
		col.red = red;
		col.green = green;
		col.blue = blue;
		col.alpha = alpha;

		if(pBackBuffer == nil ||
		   RwRasterGetWidth(pBackBuffer) != RwRasterGetWidth(pFrontBuffer) ||
		   RwRasterGetHeight(pBackBuffer) != RwRasterGetHeight(pFrontBuffer))
			Initialise();

		RwRasterPushContext(pBackBuffer);
		RwRasterRenderFast(cam->frameBuffer, 0, 0);
		RwRasterPopContext();

		if(config.radiosity < 0) config.radiosity = 0;
		if(config.radiosity)
			RadiosityRender(cam, config.radiosityLimit, config.radiosityIntensity);

		RwRasterPushContext(pBackBuffer);
		RwRasterRenderFast(cam->frameBuffer, 0, 0);
		RwRasterPopContext();

		OverlayRender_leeds(cam, pFrontBuffer, &col, type);

		RwRasterPushContext(pFrontBuffer);
		RwRasterRenderFast(cam->frameBuffer, 0, 0);
		RwRasterPopContext();
	}
}


#include "debugmenu_public.h"
void
leedsMenu(void)
{
#ifdef DEBUG
//	DebugMenuAddVarBool32("SkyGFX", "Leeds Car env map debug", &debugEnvMap, nil);
//	DebugMenuAddVarBool32("SkyGFX", "Leeds Car env", &enableEnv, nil);


//	DebugMenuAddVarBool32("SkyGFX|Debug", "textures", &doTextures, nil);
#endif
	DebugMenuAddVarBool32("SkyGFX|Advanced", "Leeds Radiosity", &config.radiosity, nil);
	DebugMenuAddFloat32("SkyGFX|Advanced", "Leeds Ambient Tweak", &config.leedsWorldAmbTweak, nil, 0.02f, 0.0f, 1.0f);
	DebugMenuAddFloat32("SkyGFX|Advanced", "Leeds Emissive Tweak", &config.leedsWorldEmissTweak, nil, 0.02f, 0.0f, 1.0f);
}
