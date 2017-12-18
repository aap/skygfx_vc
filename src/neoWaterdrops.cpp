#include "skygfx.h"
#include "d3d8.h"
#include "d3d8types.h"
#include <time.h>

#include <injector\injector.hpp>
#include <injector\hooking.hpp>
#include <injector\calling.hpp>
#include <injector\assembly.hpp>
#include <injector\utility.hpp>

enum CamModeIII
{
	CAMIII_TOPDOWN1 = 1,
	CAMIII_TOPDOWN2,
	CAMIII_BEHINDCAR,
	CAMIII_FOLLOWPED,
	CAMIII_AIMING,
	CAMIII_DEBUG,
	CAMIII_SNIPER,
	CAMIII_ROCKET,
	CAMIII_MODELVIEW,
	CAMIII_BILL,
	CAMIII_SYPHON,
	CAMIII_CIRCLE,
	CAMIII_CHEESYZOOM,
	CAMIII_WHEELCAM,
	CAMIII_FIXED,
	CAMIII_FIRSTPERSON,
	CAMIII_FLYBY,
	CAMIII_CAMONASTRING,
	CAMIII_REACTIONCAM,
	CAMIII_FOLLOWPEDWITHBINDING,
	CAMIII_CHRISWITHBINDINGPLUSROTATION,
	CAMIII_BEHINDBOAT,
	CAMIII_PLAYERFALLENWATER,
	CAMIII_CAMONTRAINROOF,
	CAMIII_CAMRUNNINGSIDETRAIN,
	CAMIII_BLOODONTHETRACKS,
	CAMIII_IMTHEPASSENGERWOOWOO,
	CAMIII_SYPHONCRIMINFRONT,
	CAMIII_PEDSDEADBABY,
	CAMIII_CUSHYPILLOWSARSE,
	CAMIII_LOOKATCARS,
	CAMIII_ARRESTCAMONE,
	CAMIII_ARRESTCAMTWO,
	CAMIII_M16FIRSTPERSON_34,
	CAMIII_SPECIALFIXEDFORSYPHON,
	CAMIII_FIGHT,
	CAMIII_TOPDOWNPED,
	CAMIII_FIRSTPERSONPEDONPC_38,
	CAMIII_FIRSTPERSONPEDONPC_39,
	CAMIII_FIRSTPERSONPEDONPC_40,
	CAMIII_FIRSTPERSONPEDONPC_41,
	CAMIII_FIRSTPERSONPEDONPC_42,
	CAMIII_EDITOR,
	CAMIII_M16FIRSTPERSON_44
};

#define MAXSIZE 15
#define MINSIZE 4
#define DROPFVF (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX2)
#define RAD2DEG(x) (180.0f*(x)/M_PI)

float scaling;
#define SC(x) ((int)((x)*scaling))

float WaterDrops::ms_xOff, WaterDrops::ms_yOff;	// not quite sure what these are
WaterDrop WaterDrops::ms_drops[MAXDROPS];
int WaterDrops::ms_numDrops;
WaterDropMoving WaterDrops::ms_dropsMoving[MAXDROPSMOVING];
int WaterDrops::ms_numDropsMoving;

bool WaterDrops::ms_enabled;
bool WaterDrops::ms_movingEnabled;

int WaterDrops::ms_splashDuration;
CPlaceable_III *WaterDrops::ms_splashObject;

float WaterDrops::ms_distMoved, WaterDrops::ms_vecLen, WaterDrops::ms_rainStrength;
RwV3d WaterDrops::ms_vec;
RwV3d WaterDrops::ms_lastAt;
RwV3d WaterDrops::ms_lastPos;
RwV3d WaterDrops::ms_posDelta;

RwTexture *WaterDrops::ms_maskTex;
RwTexture *WaterDrops::ms_tex;
RwRaster *WaterDrops::ms_raster;
int WaterDrops::ms_fbWidth, WaterDrops::ms_fbHeight;
void *WaterDrops::ms_vertexBuf;
void *WaterDrops::ms_indexBuf;
VertexTex2 *WaterDrops::ms_vertPtr;
int WaterDrops::ms_numBatchedDrops;
int WaterDrops::ms_initialised;

IDirect3DDevice8 *&RwD3DDevice = *AddressByVersion<IDirect3DDevice8**>(0x662EF0, 0x662EF0, 0x67342C, 0x7897A8, 0x7897B0, 0x7887B0);

uchar *TheCamera = AddressByVersion<uchar*>(0x6FACF8, 0, 0, 0x7E4688, 0, 0);
float &CTimer__ms_fTimeStep = *AddressByVersion<float*>(0x8E2CB4, 0, 0, 0x975424, 0, 0);
float &CWeather__Rain = *AddressByVersion<float*>(0x8E2BFC, 0, 0, 0x975340, 0, 0);
bool &CCutsceneMgr__ms_running = *AddressByVersion<bool*>(0x95CCF5, 0, 0, 0xA10AB2, 0, 0);

short
getCamMode()
{
	uchar selector = *(TheCamera + 0x76);
	uchar *cam;
	if(isIII()){
		cam = TheCamera + 0x1A4;
		cam += selector * 0x1A4;
	}else{
		cam = TheCamera + 0x188;
		cam += selector * 0x1CC;
	}
	return *(short*)(cam + 0xC);
}

struct AudioHydrant	// or particle object thing?
{
	int entity;
	union {
		CPlaceable_III *particleObjectIII;	// CParticleObject actually
		CPlaceable *particleObject;
	};
};
AudioHydrant *audioHydrants = AddressByVersion<AudioHydrant*>(0x62DAAC, 0, 0, 0x70799C, 0, 0);

static uint32_t CCullZones__CamNoRain_A = AddressByVersion<uint32_t>(0x525CE0, 0, 0, 0x57E0E0, 0, 0);
WRAPPER bool CCullZones__CamNoRain(void) { VARJMP(CCullZones__CamNoRain_A); }
static uint32_t CCullZones__PlayerNoRain_A = AddressByVersion<uint32_t>(0x525D00, 0, 0, 0x57E0C0, 0, 0);
WRAPPER bool CCullZones__PlayerNoRain(void) { VARJMP(CCullZones__PlayerNoRain_A); }
static uint32_t FindPlayerVehicle_A = AddressByVersion<uint32_t>(0x4A10C0, 0, 0, 0x4BC1E0, 0, 0);
WRAPPER bool FindPlayerVehicle(void) { VARJMP(FindPlayerVehicle_A); }

struct CPad
{
	static CPad *GetPad(int);
	bool GetLookBehindForCar(void);
	bool GetLookLeft(void);
	bool GetLookRight(void);
};
static uint32_t GetPad_A = AddressByVersion<uint32_t>(0x492F60, 0, 0, 0x4AB060, 0, 0);
WRAPPER CPad *CPad::GetPad(int id) { VARJMP(GetPad_A); }
static uint32_t GetLookBehindForCar_A = AddressByVersion<uint32_t>(0x4932F0, 0, 0, 0x4AAC30, 0, 0);
WRAPPER bool CPad::GetLookBehindForCar(void) { VARJMP(GetLookBehindForCar_A); }
static uint32_t GetLookLeft_A = AddressByVersion<uint32_t>(0x493290, 0, 0, 0x4AAC90, 0, 0);
WRAPPER bool CPad::GetLookLeft(void) { VARJMP(GetLookLeft_A); }
static uint32_t GetLookRight_A = AddressByVersion<uint32_t>(0x4932C0, 0, 0, 0x4AAC60, 0, 0);
WRAPPER bool CPad::GetLookRight(void) { VARJMP(GetLookRight_A); }

#define INJECTRESET(x) \
	addr reset_call_##x; \
	void reset_hook_##x(void){ \
		((voidfunc)reset_call_##x)(); \
		WaterDrops::Reset(); \
	}

INJECTRESET(1)
INJECTRESET(2)
INJECTRESET(3)
INJECTRESET(4)
INJECTRESET(5)

void
hookWaterDrops()
{
	if (is10()) 
	{
		static injector::hook_back<void(*)()> RenderEffects;
		auto RenderEffects_hook = []()
		{
			RenderEffects.fun();
			WaterDrops::Process();
			WaterDrops::Render();
		}; RenderEffects.fun = injector::MakeCALL(AddressByVersion<addr>(0x48E603, 0, 0, 0x4A604F, 0, 0), static_cast<void(*)()>(RenderEffects_hook), true).get();

		static auto splashbreak = injector::GetBranchDestination(AddressByVersion<addr>(0x4BC7D0, 0, 0, 0x4E8721, 0, 0), true).as_int();
		struct splashhook
		{
			void operator()(injector::reg_pack& regs)
			{
				WaterDrops::RegisterSplash((CPlaceable_III*)regs.ebp, 20.0f);
				*(uint32_t*)regs.esp = splashbreak;
			}
		}; injector::MakeInline<splashhook>(AddressByVersion<addr>(0x4BC7D0, 0, 0, 0x4E8721, 0, 0));

		if (config.neoblooddrops)
		{
			static injector::hook_back<void(__cdecl*)(int, int, int, int, int, int, int, int, int)> AddParticle;
			auto AddParticleHook = [](int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9)
			{
				AddParticle.fun(a1, a2, a3, a4, a5, a6, a7, a8, a9);
				if (config.neoblooddrops)
					WaterDrops::FillScreenMoving(0.2f, true);
			}; AddParticle.fun = injector::MakeCALL(AddressByVersion<addr>(0x4E78D1, 0, 0, 0x52A4B3, 0, 0), static_cast<void(__cdecl*)(int, int, int, int, int, int, int, int, int)>(AddParticleHook), true).get();
			injector::MakeCALL(AddressByVersion<addr>(0x55CF2E, 0, 0, 0x5D343A, 0, 0), static_cast<void(__cdecl*)(int, int, int, int, int, int, int, int, int)>(AddParticleHook), true);
		}

		//boats
		struct boathook
		{
			void operator()(injector::reg_pack& regs)
			{
				regs.eax = regs.esp + 0x1A8;
				WaterDrops::FillScreenMoving(0.05f, false);
			}
		}; injector::MakeInline<boathook>(AddressByVersion<addr>(0x541497, 0, 0, 0x5A42E6, 0, 0), AddressByVersion<addr>(0x541497 + 7, 0, 0, 0x5A42E6 + 7, 0, 0));

		INTERCEPT(reset_call_1, reset_hook_1, AddressByVersion<addr>(0x48C1AB, 0, 0, 0x4A4DD6, 0, 0));	// CGame::Initialise
		INTERCEPT(reset_call_2, reset_hook_2, AddressByVersion<addr>(0x48C530, 0, 0, 0x4A48EA, 0, 0));	// CGame::ReInitGameObjectVariables
		INTERCEPT(reset_call_3, reset_hook_3, AddressByVersion<addr>(0x42155D, 0, 0, 0x42BCD6, 0, 0));	// CGameLogic::Update
		INTERCEPT(reset_call_4, reset_hook_4, AddressByVersion<addr>(0x42177A, 0, 0, 0x42C0BC, 0, 0));	// CGameLogic::Update
		INTERCEPT(reset_call_5, reset_hook_5, AddressByVersion<addr>(0x421926, 0, 0, 0x42C318, 0, 0));  // CGameLogic::Update

		if (gtaversion == VC_10) 
		{
			injector::MakeNOP(AddressByVersion<addr>(0, 0, 0, 0x560D63, 0, 0), 5, true); //remove old effect in VC

			if (config.neoblooddrops)
			{
				//chainsaw
				struct bloodhook
				{
					void operator()(injector::reg_pack& regs)
					{
						WaterDrops::FillScreenMoving(1.0f, true);
					}
				}; injector::MakeInline<bloodhook>(AddressByVersion<addr>(0, 0, 0, 0x5D3989, 0, 0));
			}

			//fountains
			struct fountainhook // near movie studio
			{
				void operator()(injector::reg_pack& regs)
				{
					regs.edi = regs.esp + 0x3C0;
					WaterDrops::RegisterSplash((CPlaceable_III*)regs.ebx, 5.0f);
				}
			}; injector::MakeInline<fountainhook>(AddressByVersion<addr>(0, 0, 0, 0x4E7F64, 0, 0), AddressByVersion<addr>(0, 0, 0, 0x4E7F64+7, 0, 0));

			struct fountainhook2 //Ocean Beach Fountain
			{
				void operator()(injector::reg_pack& regs)
				{
					regs.edi = regs.esp + 0x390;
					WaterDrops::RegisterSplash((CPlaceable_III*)regs.ebx, 10.0f);
				}
			}; injector::MakeInline<fountainhook2>(AddressByVersion<addr>(0, 0, 0, 0x4E795D, 0, 0), AddressByVersion<addr>(0, 0, 0, 0x4E795D+7, 0, 0));

			//pools
			static injector::hook_back<void(__cdecl*)(int, int, int, int, int, int, int, int, int)> AddParticle;
			auto AddParticlePoolHook = [](int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9)
			{
				AddParticle.fun(a1, a2, a3, a4, a5, a6, a7, a8, a9);
				WaterDrops::FillScreenMoving(0.5f, false);
			}; AddParticle.fun = injector::MakeCALL(AddressByVersion<addr>(0, 0, 0, 0x50489F, 0, 0), static_cast<void(__cdecl*)(int, int, int, int, int, int, int, int, int)>(AddParticlePoolHook), true).get();
		}
	}
}

void
WaterDrops::RegisterSplash(CPlaceable_III *plc, float distance = 20.0f)
{
	RwV3d dist;
	if(isIII())
		RwV3dSub(&dist, &plc->matrix.matrix.pos, &ms_lastPos);
	else
		RwV3dSub(&dist, &((CPlaceable*)plc)->matrix.matrix.pos, &ms_lastPos);

	if(RwV3dLength(&dist) <= distance){
		ms_splashDuration = 14;
		ms_splashObject = plc;
	}
}

void
WaterDrops::InitialiseRender(RwCamera *cam)
{
	srand(time(NULL));
	ms_fbWidth = cam->frameBuffer->width;
	ms_fbHeight = cam->frameBuffer->height;

	scaling = ms_fbHeight / 480.0f;

	IDirect3DVertexBuffer8 *vbuf;
	IDirect3DIndexBuffer8 *ibuf;
	RwD3DDevice->CreateVertexBuffer(MAXDROPS * 4 * sizeof(VertexTex2), D3DUSAGE_WRITEONLY, DROPFVF, D3DPOOL_MANAGED, &vbuf);
	ms_vertexBuf = vbuf;
	RwD3DDevice->CreateIndexBuffer(MAXDROPS * 6 * sizeof(short), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED, &ibuf);
	ms_indexBuf = ibuf;
	RwUInt16 *idx;
	ibuf->Lock(0, 0, (BYTE**)&idx, 0);
	for(int i = 0; i < MAXDROPS; i++){
		idx[i * 6 + 0] = i * 4 + 0;
		idx[i * 6 + 1] = i * 4 + 1;
		idx[i * 6 + 2] = i * 4 + 2;
		idx[i * 6 + 3] = i * 4 + 0;
		idx[i * 6 + 4] = i * 4 + 2;
		idx[i * 6 + 5] = i * 4 + 3;
	}
	ibuf->Unlock();

	int w, h;
	for(w = 1; w < cam->frameBuffer->width; w <<= 1);
	for(h = 1; h < cam->frameBuffer->height; h <<= 1);
	ms_raster = RwRasterCreate(w, h, 0, 5);
	ms_tex = RwTextureCreate(ms_raster);
	ms_tex->filterAddressing = 0x3302;
	RwTextureAddRef(ms_tex);

	ms_initialised = 1;
}

void
WaterDrops::Process()
{
	if(!ms_initialised || Scene.camera->frameBuffer->width != ms_fbWidth)
		InitialiseRender(Scene.camera);
	WaterDrops::CalculateMovement();
	WaterDrops::SprayDrops();
	WaterDrops::ProcessMoving();
	WaterDrops::Fade();
}

void
WaterDrops::CalculateMovement()
{
	RwMatrix *modelMatrix;
	modelMatrix = &RwCameraGetFrame(Scene.camera)->modelling;
	RwV3dSub(&ms_posDelta, &modelMatrix->pos, &ms_lastPos);
	ms_distMoved = RwV3dLength(&ms_posDelta);
	// RwV3d pos;
	// pos.x = (modelMatrix->at.x - ms_lastAt.x) * 10.0f;
	// pos.y = (modelMatrix->at.y - ms_lastAt.y) * 10.0f;
	// pos.z = (modelMatrix->at.z - ms_lastAt.z) * 10.0f;
	// ^ result unused for now
	ms_lastAt = modelMatrix->at;
	ms_lastPos = modelMatrix->pos;

	ms_vec.x = -RwV3dDotProduct(&modelMatrix->right, &ms_posDelta);
	ms_vec.y = RwV3dDotProduct(&modelMatrix->up, &ms_posDelta);
	ms_vec.z = RwV3dDotProduct(&modelMatrix->at, &ms_posDelta);
	RwV3dScale(&ms_vec, &ms_vec, 10.0f);
	ms_vecLen = sqrt(ms_vec.y*ms_vec.y + ms_vec.x*ms_vec.x);

	short mode = getCamMode();
	bool istopdown = mode == CAMIII_TOPDOWN1 || mode == CAMIII_TOPDOWN2 || mode == CAMIII_TOPDOWNPED;
	bool carlookdirection = 0;
	if(mode == CAMIII_FIRSTPERSON && FindPlayerVehicle()){
		CPad *p = CPad::GetPad(0);
		if (p->GetLookBehindForCar() || p->GetLookLeft() || p->GetLookRight())
			carlookdirection = 1;
	}
	ms_enabled = !istopdown && !carlookdirection;
	ms_movingEnabled = !istopdown && !carlookdirection;

	float c = modelMatrix->at.z;
	if(c > 1.0f) c = 1.0f;
	if(c < -1.0f) c = -1.0f;
	ms_rainStrength = RAD2DEG(acos(c));
}

bool
WaterDrops::NoRain()
{
	return CCullZones__CamNoRain() || CCullZones__PlayerNoRain();
}

WaterDrop*
WaterDrops::PlaceNew(float x, float y, float size, float ttl, bool fades, int R = 0xFF, int G = 0xFF, int B = 0xFF)
{
	WaterDrop *drop;
	int i;

	for(i = 0, drop = ms_drops; i < MAXDROPS; i++, drop++)
		if (ms_drops[i].active == 0)
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
	drop->r = R;
	drop->g = G;
	drop->b = B;
	drop->alpha = 0xFF;
	drop->time = 0.0f;
	drop->ttl = ttl;
	return drop;
}

void WaterDrops::NewDropMoving(WaterDrop *drop)
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
WaterDrops::FillScreenMoving(float amount, bool isBlood = false)
{
	int n = (ms_vec.z <= 5.0f ? 1.0f : 1.5f)*amount*20.0f;
	float x, y, time;
	WaterDrop *drop;

	while(n--)
		if(ms_numDrops < MAXDROPS && ms_numDropsMoving < MAXDROPSMOVING){
			x = rand() % ms_fbWidth;
			y = rand() % ms_fbHeight;
			time = rand() % (SC(MAXSIZE) - SC(MINSIZE)) + SC(MINSIZE);
			drop = NULL;
			if(!isBlood)
				drop = PlaceNew(x, y, time, 2000.0f, 1);
			else
				drop = PlaceNew(x, y, time, 2000.0f, 1, 0xFF, 0x00, 0x00);
			if(drop)
				NewDropMoving(drop);
		}
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

	if(!NoRain() && CWeather__Rain != 0.0f && ms_enabled){
		int tmp = 180.0f - ms_rainStrength;
		if (tmp < 40) tmp = 40;
		FillScreenMoving((tmp - 40.0f) / 150.0f * CWeather__Rain * 0.5f);
	}
	for(hyd = audioHydrants; hyd < &audioHydrants[8]; hyd++)
		if (hyd->particleObject){
			if(isIII())
				RwV3dSub(&dist, &hyd->particleObjectIII->matrix.matrix.pos, &ms_lastPos);
			else
				RwV3dSub(&dist, &hyd->particleObject->matrix.matrix.pos, &ms_lastPos);
			if(RwV3dDotProduct(&dist, &dist) <= 40.0f)
				FillScreenMoving(1.0f);
		}
	if(ms_splashDuration >= 0){
		if(ms_numDrops < MAXDROPS) {
			if(isIII()){
				if(ms_splashObject){
					RwV3dSub(&dist, &ms_splashObject->matrix.matrix.pos, &ms_lastPos);
					int n = ndrops[ms_splashDuration] * (1.0f - (RwV3dLength(&dist) - 5.0f) / 15.0f);
					while(n--)
						if(ms_numDrops < MAXDROPS){
							float x = rand() % ms_fbWidth;
							float y = rand() % ms_fbHeight;
							float time = rand() % (SC(MAXSIZE) - SC(MINSIZE)) + SC(MINSIZE);
							PlaceNew(x, y, time, 10000.0f, true);
						}
				}else
					FillScreenMoving(1.0f);
			}else
				FillScreenMoving(1.0f); // VC does STRANGE things here
		}
		ms_splashDuration--;
	}
}

void
WaterDrops::NewTrace(WaterDropMoving *moving)
{
	if(ms_numDrops < MAXDROPS){
		moving->dist = 0.0f;
		PlaceNew(moving->drop->x, moving->drop->y, SC(MINSIZE), 500.0f, 1, moving->drop->r, moving->drop->g, moving->drop->b);
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

	short mode = getCamMode();
	if(ms_vecLen <= 0.5f || mode == CAMIII_FIRSTPERSON){
		// movement out of center 
		float d = ms_vec.z*0.2f;
		float dx, dy, sum;
		dx = drop->x - ms_fbWidth*0.5f + ms_vec.x;
		if(mode == CAMIII_FIRSTPERSON)
			dy = drop->y - ms_fbHeight*1.2f - ms_vec.y;
		else
			dy = drop->y - ms_fbHeight*0.5f - ms_vec.y;
		sum = fabs(dx) + fabs(dy);
		if(sum >= 0.001f){
			dx *= (1.0 / sum);
			dy *= (1.0 / sum);
		}
		moving->dist += d;
		if(moving->dist > 20.0f)
			NewTrace(moving);
		drop->x += dx * d;
		drop->y += dy * d;
	}else{
		// movement when camera turns
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
WaterDrops::ProcessMoving()
{
	WaterDropMoving *moving;
	if(!ms_movingEnabled)
		return;
	for(moving = ms_dropsMoving; moving < &ms_dropsMoving[MAXDROPSMOVING]; moving++)
		if(moving->drop)
			MoveDrop(moving);
}

void
WaterDrop::Fade()
{
	int delta = CTimer__ms_fTimeStep * 1000.0f / 50.0f;
	this->time += delta;
	if(this->time >= this->ttl){
		WaterDrops::ms_numDrops--;
		this->active = 0;
	}else if (this->fades)
		this->alpha = 255 - time / ttl * 255;
}

void
WaterDrops::Fade()
{
	WaterDrop *drop;
	for(drop = &ms_drops[0]; drop < &ms_drops[MAXDROPS]; drop++)
		if(drop->active)
			drop->Fade();
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
	u1_2 = (u1_2 >= ms_fbWidth ? ms_fbWidth : u1_2) / ms_raster->width;
	v1_2 = (v1_2 >= ms_fbHeight ? ms_fbHeight : v1_2) / ms_raster->height;

	scale = drop->size * 0.5f;

	for(i = 0; i < 4; i++){
		ms_vertPtr->x = drop->x + xy[i * 2] * scale + ms_xOff;
		ms_vertPtr->y = drop->y + xy[i * 2 + 1] * scale + ms_yOff;
		ms_vertPtr->z = 0.0f;
		ms_vertPtr->rhw = 1.0f;
		ms_vertPtr->emissiveColor = D3DCOLOR_ARGB(drop->alpha, drop->r, drop->g, drop->b);
		ms_vertPtr->u0 = uv[i * 2];
		ms_vertPtr->v0 = uv[i * 2 + 1];
		ms_vertPtr->u1 = i >= 2 ? u1_2 : u1_1;
		ms_vertPtr->v1 = i % 3 == 0 ? v1_2 : v1_1;
		ms_vertPtr++;
	}
	ms_numBatchedDrops++;
}

void
WaterDrops::Render()
{
	WaterDrop *drop;

	bool nofirstperson = FindPlayerVehicle() == 0 && getCamMode() == CAMIII_FIRSTPERSON;
	if(!ms_enabled || ms_numDrops <= 0 || nofirstperson || CCutsceneMgr__ms_running)
		return;

	IDirect3DVertexBuffer8 *vbuf = (IDirect3DVertexBuffer8*)ms_vertexBuf;
	vbuf->Lock(0, 0, (BYTE**)&ms_vertPtr, 0);
	ms_numBatchedDrops = 0;
	for(drop = &ms_drops[0]; drop < &ms_drops[MAXDROPS]; drop++)
		if(drop->active)
			AddToRenderList(drop);
	vbuf->Unlock();

	RwRasterPushContext(ms_raster);
	RwRasterRenderFast(RwCameraGetRaster(Scene.camera), 0, 0);
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
	RwD3D8DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, ms_numBatchedDrops * 4, 0, ms_numBatchedDrops * 2);

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

/////////////////
void WaterDrops::FillScreen(int n)
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

void WaterDrops::Clear()
{
	WaterDrop *drop;
	for(drop = &ms_drops[0]; drop < &ms_drops[MAXDROPS]; drop++)
		drop->active = 0;
	ms_numDrops = 0;
}

void WaterDrops::Reset()
{
	Clear();
	ms_splashDuration = -1;
	ms_splashObject = NULL;
}
