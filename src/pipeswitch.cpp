#include "skygfx.h"
#include "debugmenu_public.h"
#include <d3d8.h>
#include <d3d8types.h>

static uint32_t CSimpleModelInfo__SetAtomic_A; // for reference: AddressByVersion<uint32_t>(0x517950, 0x517B60, 0x517AF0, 0x56F790, 0, 0);
WRAPPER void CSimpleModelInfo::SetAtomic(int, RpAtomic*) { VARJMP(CSimpleModelInfo__SetAtomic_A); }



void CRenderer__RenderEverythingBarRoads(void);
void CRenderer__RenderFadingInEntities(void);
// ADDRESS
//static uint32_t CRenderer__RenderRoads_A = AddressByVersion<uint32_t>(0x4A78B0, 0, 0, 0x4CA180, 0, 0);
//WRAPPER void CRenderer__RenderRoads(void) { VARJMP(CRenderer__RenderRoads_A); }
//static uint32_t CRenderer__RenderOneRoad_A = AddressByVersion<uint32_t>(0x4A7B90, 0, 0, 0, 0, 0);
//WRAPPER void CRenderer__RenderOneRoad(void*) { VARJMP(CRenderer__RenderOneRoad_A); }
void CRenderer__RenderOneRoad(void*);	// gloss pipe version...will attach pipe
static uint32_t SetAmbientColours_A = AddressByVersion<uint32_t>(0x526F60, 0, 0, 0x57FB10, 0, 0);
WRAPPER void SetAmbientColours(void) { VARJMP(SetAmbientColours_A); }
static uint32_t CClouds__RenderBackground_A = AddressByVersion<uint32_t>(0x4F7F00, 0, 0, 0x53F650, 0, 0);
WRAPPER void CClouds__RenderBackground(int16 tr, int16 tg, int16 tb, int16 br, int16 bg, int16 bb, uint8 a) { VARJMP(CClouds__RenderBackground_A); }

extern IDirect3DDevice8 *&RwD3DDevice;

RwIm2DVertex seamQuad[4];
int seamOffX = 1;
int seamOffY = 1;

void
makeSeamQuad(void)
{
	RwCamera *cam = (RwCamera*)RWSRCGLOBAL(curCamera);

	float xmin = 0.0f;
	float ymin = 0.0f;
	float xmax = ScreenFX::pFrontBuffer->width;
	float ymax = ScreenFX::pFrontBuffer->height;

	xmin -= 0.5f;
	xmax -= 0.5f;
	ymin -= 0.5f;
	ymax -= 0.5f;

	xmin += seamOffX;
	xmax += seamOffX;
	ymin += seamOffY;
	ymax += seamOffY;

	const float z = RwIm2DGetFarScreenZ();
	const float recipz = 1.0f/RwCameraGetFarClipPlane(cam);

	RwIm2DVertexSetScreenX(&seamQuad[0], xmin);
	RwIm2DVertexSetScreenY(&seamQuad[0], ymin);
	RwIm2DVertexSetScreenZ(&seamQuad[0], z);
	RwIm2DVertexSetRecipCameraZ(&seamQuad[0], recipz);
	RwIm2DVertexSetU(&seamQuad[0], 0.0f, recipz);
	RwIm2DVertexSetV(&seamQuad[0], 0.0f, recipz);
	RwIm2DVertexSetIntRGBA(&seamQuad[0], 255, 255, 255, 255);

	RwIm2DVertexSetScreenX(&seamQuad[1], xmin);
	RwIm2DVertexSetScreenY(&seamQuad[1], ymax);
	RwIm2DVertexSetScreenZ(&seamQuad[1], z);
	RwIm2DVertexSetRecipCameraZ(&seamQuad[1], recipz);
	RwIm2DVertexSetU(&seamQuad[1], 0.0f, recipz);
	RwIm2DVertexSetV(&seamQuad[1], 1.0f, recipz);
	RwIm2DVertexSetIntRGBA(&seamQuad[1], 255, 255, 255, 255);

	RwIm2DVertexSetScreenX(&seamQuad[2], xmax);
	RwIm2DVertexSetScreenY(&seamQuad[2], ymax);
	RwIm2DVertexSetScreenZ(&seamQuad[2], z);
	RwIm2DVertexSetRecipCameraZ(&seamQuad[2], recipz);
	RwIm2DVertexSetU(&seamQuad[2], 1.0f, recipz);
	RwIm2DVertexSetV(&seamQuad[2], 1.0f, recipz);
	RwIm2DVertexSetIntRGBA(&seamQuad[2], 255, 255, 255, 255);

	RwIm2DVertexSetScreenX(&seamQuad[3], xmax);
	RwIm2DVertexSetScreenY(&seamQuad[3], ymin);
	RwIm2DVertexSetScreenZ(&seamQuad[3], z);
	RwIm2DVertexSetRecipCameraZ(&seamQuad[3], recipz);
	RwIm2DVertexSetU(&seamQuad[3], 1.0f, recipz);
	RwIm2DVertexSetV(&seamQuad[3], 0.0f, recipz);
	RwIm2DVertexSetIntRGBA(&seamQuad[3], 255, 255, 255, 255);
}

void
fixSeams(void)
{
	ScreenFX::Update();
	ScreenFX::UpdateFrontBuffer();
	makeSeamQuad();

	RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERNEAREST);
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)TRUE);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)ScreenFX::pFrontBuffer);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)FALSE);

	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, seamQuad, 4, ScreenFX::screenIndices, 6);
}



CarPipe carpipe;

/*
 * Custom World pipe
 */

void
CSimpleModelInfo::SetAtomic_hook(int n, RpAtomic *atomic)
{

	this->SetAtomic(n, atomic);
	WorldPipe::Get()->Attach(atomic);
}

void
CSimpleModelInfo::SetAtomicVC_hook(int n, RpAtomic *atomic)
{
	this->SetAtomic(n, atomic);
	ushort flags = *(ushort*)((uchar*)this + 0x42);
	if(flags & 4/* && config.iCanHasNeoGloss*/)	// isRoad
		GlossPipe::Get()->Attach(atomic);
	else
		WorldPipe::Get()->Attach(atomic);
}

// ADDRESS

extern "C" {
__declspec(dllexport) void
AttachWorldPipeToRwObject(RwObject *obj)
{
	if(RwObjectGetType(obj) == rpATOMIC)
		WorldPipe::Get()->Attach((RpAtomic*)obj);
	else if(RwObjectGetType(obj) == rpCLUMP)
		RpClumpForAllAtomics((RpClump*)obj, CustomPipe::setatomicCB, WorldPipe::Get());
}
}

WorldPipe*
WorldPipe::Get(void)
{
	static WorldPipe worldpipe;
	return &worldpipe;
}

void
WorldPipe::Init(void)
{
	WorldPipe::Get()->CreateRwPipeline();
	WorldPipe::Get()->SetRenderCallback(RenderCallback);

	if(DebugMenuLoad()){
		DebugMenuAddVarBool32("SkyGFX", "Seam fixer", &config.seamfix, nil);
		DebugMenuAddVar("SkyGFX", "Seam Offset X", &seamOffX, nil, 1, -10, 10, nil);
		DebugMenuAddVar("SkyGFX", "Seam Offset Y", &seamOffY, nil, 1, -10, 10, nil);
	}

	if(gtaversion == III_10){
		InterceptCall(&CSimpleModelInfo__SetAtomic_A, &CSimpleModelInfo::SetAtomic_hook, 0x4768F1);
		InjectHook(0x476707, &CSimpleModelInfo::SetAtomic_hook);
	}else if(gtaversion == VC_10){
		// virtual in VC because of added CWeaponModelInfo
		InterceptVmethod(&CSimpleModelInfo__SetAtomic_A, &CSimpleModelInfo::SetAtomicVC_hook, 0x697FF8);
		Patch(0x698028, &CSimpleModelInfo::SetAtomicVC_hook);
	}
}

void
WorldPipe::RenderCallback(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
							{
								static bool keystate = false;
								if(GetAsyncKeyState(config.worldPipeKey) & 0x8000){
									if(!keystate){
										keystate = true;
										config.worldPipeSwitch = (config.worldPipeSwitch+1)%WORLD_NUMPIPES;
									}
								}else
									keystate = false;
							}

	switch(config.worldPipeSwitch){
		case WORLD_DEFAULT: rxD3D8DefaultRenderCallback_xbox(repEntry, object, type, flags); break;
		case WORLD_NEO: NeoWorldPipe::Get()->renderCB(repEntry, object, type, flags); break;
		case WORLD_LEEDS: leedsRenderCallback(repEntry, object, type, flags); break;
	}
}




/*
 * Custom Car pipe
 */

extern "C" {
__declspec(dllexport) void
AttachCarPipeToRwObject(RwObject *obj)
{
	if(RwObjectGetType(obj) == rpATOMIC)
		carpipe.Attach((RpAtomic*)obj);
	else if(RwObjectGetType(obj) == rpCLUMP)
		RpClumpForAllAtomics((RpClump*)obj, CustomPipe::setatomicCB, &carpipe);
}
}

class CVehicleModelInfo {
public:
	static addr SetClump_A;
	void SetClump(RpClump *clump);
	void SetClump_hook(RpClump *clump){
		this->SetClump(clump);
		AttachCarPipeToRwObject((RwObject*)clump);
	}
};
addr CVehicleModelInfo::SetClump_A;
WRAPPER void CVehicleModelInfo::SetClump(RpClump*) { VARJMP(SetClump_A); }

class CVisibilityPlugins
{
public:
	static addr SetAtomicRenderCallback_A;
	static void SetAtomicRenderCallback(RpAtomic *atomic, RpAtomicCallBackRender cb);
	static void SetAtomicRenderCallback_hook(RpAtomic *atomic, RpAtomicCallBackRender cb){
		SetAtomicRenderCallback(atomic, cb);
		AttachCarPipeToRwObject((RwObject*)atomic);
	}
};
addr CVisibilityPlugins::SetAtomicRenderCallback_A;
WRAPPER void CVisibilityPlugins::SetAtomicRenderCallback(RpAtomic*, RpAtomicCallBackRender) { VARJMP(SetAtomicRenderCallback_A); }

static uint32_t RenderScene_A;
WRAPPER void RenderScene(void) { VARJMP(RenderScene_A); }

void
RenderScene_hook(void)
{
	// This is needed because the game sets far after RwCameraBeginUpdate,
	// so do it again here
	RwCamera *cam = (RwCamera*)RWSRCGLOBAL(curCamera);
	RwCameraEndUpdate(cam);
	RwCameraBeginUpdate(cam);

	RenderScene();
	if(config.seamfix)
		fixSeams();

	switch(config.carPipeSwitch){
		case CAR_NEO: NeoCarPipe::RenderEnvTex(); break;
		case CAR_LEEDS: LeedsCarPipe::RenderEnvTex(); break;
	}
}


void
CarPipe::Init(void)
{
	carpipe.CreateRwPipeline();
	carpipe.SetRenderCallback(RenderCallback);

	InterceptVmethod(&CVehicleModelInfo::SetClump_A, &CVehicleModelInfo::SetClump_hook,
	                 AddressByVersion<addr>(0x5FDFF0, 0x5FDDD8, 0x60ADD0, 0x698088, 0x698088, 0x697090));
	// ADDRESS
	if(is10())
		InterceptCall(&CVisibilityPlugins::SetAtomicRenderCallback_A, &CVisibilityPlugins::SetAtomicRenderCallback_hook,
		              AddressByVersion<addr>(0x5207C6, 0, 0, 0x579DFB, 0, 0));
	InterceptCall(&RenderScene_A, RenderScene_hook, AddressByVersion<addr>(0x48E5F9, 0x48E6B9, 0x48E649, 0x4A604A, 0x4A606A, 0x4A5F1A));
//	InterceptCall(&DoRWRenderHorizon_A, DoRWRenderHorizon_hook, AddressByVersion<addr>(0x48E035, 0, 0, 0x4A6575, 0, 0));
}

void
CarPipe::RenderCallback(RwResEntry *repEntry, void *object, RwUInt8 type, RwUInt32 flags)
{
						{
							static bool keystate = false;
							if(GetAsyncKeyState(config.carPipeKey) & 0x8000){
								if(!keystate){
									keystate = true;
									config.carPipeSwitch = (config.carPipeSwitch+1)%CAR_NUMPIPES;
								}
							}else
								keystate = false;
						}
	switch(config.carPipeSwitch){
		case CAR_DEFAULT: rwD3D8AtomicMatFXRenderCallback(repEntry, object, type, flags); break;
		case CAR_NEO: NeoCarPipe::Get()->renderCB(repEntry, object, type, flags); break;
		case CAR_LEEDS: LeedsCarPipe::RenderCallback(repEntry, object, type, flags); break;
	}
}



//
// Custom Render functions for env maps
//

void
DeActivateDirectional(void)
{
	pDirect->object.object.flags = 0;
}

// III; VC untested
struct AlphaObjectInfo
{
	void *entity;
	float dist;
};
struct CLink_AlphaObjectInfo
{
	AlphaObjectInfo item;
	CLink_AlphaObjectInfo *prev;
	CLink_AlphaObjectInfo *next;
};
struct CLinkList_AlphaObjectInfo
{
	CLink_AlphaObjectInfo head;
	CLink_AlphaObjectInfo tail;
	CLink_AlphaObjectInfo freeHead;
	CLink_AlphaObjectInfo freeTail;
	CLink_AlphaObjectInfo *links;
};

int &CRenderer__ms_nNoOfVisibleEntities = *AddressByVersion<int*>(0x940730, 0, 0, 0xA0D1E4, 0, 0);
void **CRenderer__ms_aVisibleEntityPtrs = AddressByVersion<void**>(0x6E9920, 0, 0, 0x7D54F8, 0, 0);

CLinkList_AlphaObjectInfo &CVisibilityPlugins__m_alphaEntityList = *AddressByVersion<CLinkList_AlphaObjectInfo*>(0x943084, 0, 0, 0xA0FD84, 0, 0);

// A simple function just enough for a reflection map
void
RenderEveryBarCarsPeds(void)
{
	int i;
	void *ent;
	int type;

	DeActivateDirectional();
	SetAmbientColours();

	for(i = 0; i < CRenderer__ms_nNoOfVisibleEntities; i++){
		ent = CRenderer__ms_aVisibleEntityPtrs[i];
		type = FIELD(uint8, ent, 0x50)&7;
		if(type != 1)	// building
			continue;
		CRenderer__RenderOneRoad(ent);
	}
}

void
RenderAlphaListBarCarsPeds(void)
{
	CLink_AlphaObjectInfo *lnk;
	void *ent;
	int type;

	DeActivateDirectional();
	SetAmbientColours();

	for(lnk = CVisibilityPlugins__m_alphaEntityList.tail.prev;
	    lnk != &CVisibilityPlugins__m_alphaEntityList.head;
	    lnk = lnk->prev){
		ent = lnk->item.entity;
		type = FIELD(uint8, ent, 0x50)&7;
		if(type != 1)	// building
			continue;
		CRenderer__RenderOneRoad(ent);
	}
}
