#include "skygfx.h"

// ADDRESS
RwTexture *&gpWaterTex = *AddressByVersion<RwTexture**>(0, 0, 0, 0x77FA58, 0, 0);
RpAtomic *&ms_pWavyAtomic = *AddressByVersion<RpAtomic**>(0, 0, 0, 0xA10860, 0, 0);
RpAtomic *&ms_pMaskAtomic = *AddressByVersion<RpAtomic**>(0, 0, 0, 0xA0D9A8, 0, 0);
float &TEXTURE_ADDU = *AddressByVersion<float*>(0, 0, 0, 0x77FA6C, 0, 0);
float &TEXTURE_ADDV = *AddressByVersion<float*>(0, 0, 0, 0x77FA70, 0, 0);

MatFXEnv*
getEnvData(RpMaterial *mat)
{
	MatFX *matfx = *RWPLUGINOFFSET(MatFX*, mat, MatFXMaterialDataOffset);
	MatFXEnv *env = &matfx->fx[0];
	if(env->effect == rpMATFXEFFECTENVMAP)
		return env;
	env = &matfx->fx[1];
	if(env->effect == rpMATFXEFFECTENVMAP)
		return env;
	return nil;
}

// Wavy atomic is 32x32 units with 17x17 vertices
// Mask atomic is 64x64 units with 33x33 vertices
// One texture is stretched over 32 units

bool (*PreCalcWavyMask_orig)(float x, float y, float z, float a4, float a5, float a6, float a7, int _1, int _2, RwRGBA *colour);
bool
PreCalcWavyMask(float x, float y, float z, float a4, float a5, float a6, float a7, int _1, int _2, RwRGBA *colour)
{
	bool ret = PreCalcWavyMask_orig(x, y, z, a4, a5, a6, a7, _1, _2, colour);

	RpGeometry *maskgeo = RpAtomicGetGeometry(ms_pMaskAtomic);
	MatFXEnv *env = getEnvData(maskgeo->matList.materials[0]);
	env->envCoeff = 0.5f;

	RpGeometryLock(maskgeo, rpGEOMETRYLOCKTEXCOORDS | rpGEOMETRYLOCKPRELIGHT);

	int i, j;
	const float ustep = 2.0f / 32.0f;
	const float vstep = 2.0f / 32.0f;
	const float ustart = TEXTURE_ADDU + (x+16.0f)/32.0f;
	const float vstart = TEXTURE_ADDV + (y)/32.0f;
	RwTexCoords *uv = maskgeo->texCoords[0];

	for(i = 0; i < 33; i++)	// x
		for(j = 0; j < 33; j++){	// y
			uv[i*33 + j].u = ustart + ustep*i;
			uv[i*33 + j].v = vstart + vstep*j;
		}

	const float inc = 1.0f/17.0f;
	int ii, jj;
	RwRGBA *c = maskgeo->preLitLum;
	float fx, fy;
	fx = 0.0f;
	uchar alpha;
	for(i = 0; i < 17; i++){
		fy = 0.0f;
		for(j = 0; j < 17; j++){
			ii = 32-i;
			jj = 32-j;
			alpha = fx*fy*255;
			if(alpha > colour->alpha) alpha = colour->alpha;
			c[i*33 + j].alpha = alpha;
			c[i*33 + jj].alpha = alpha;
			c[ii*33 + jj].alpha = alpha;
			c[ii*33 + j].alpha = alpha;

			fy += inc;
		}
		fx += inc;
	}

	RpGeometryUnlock(maskgeo);


	// Wavy test

	ms_pWavyAtomic->geometry->matList.materials[0]->color.alpha = 192;
	RpGeometryLock(ms_pWavyAtomic->geometry, rpGEOMETRYLOCKPRELIGHT);
	c = ms_pWavyAtomic->geometry->preLitLum;
	for(i = 0; i < 17; i++)
		for(j = 0; j < 17; j++){
			c[i*17 + j].alpha = colour->alpha;
		}
	RpGeometryUnlock(ms_pWavyAtomic->geometry);


	return ret;
}

void
patchWater(void)
{
	// Replace sandy mask by clearwater
	Patch(0x5C2E8C + 1, &gpWaterTex);

	static float maskWaterEnvCoeff = 0.4f;	// 0.25 on PC, but later changed to 0.5 anyway...
	Patch(0x5C3816 + 2, &maskWaterEnvCoeff);

	InterceptCall(&PreCalcWavyMask_orig, PreCalcWavyMask, 0x5BF977);
}
