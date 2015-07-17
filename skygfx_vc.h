#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <rwcore.h>
#include <rwplcore.h>
#include <rpworld.h>
#include "d3d8.h"
#include "d3d8types.h"
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "MemoryMgr.h"

struct MatFXEnv
{
  RwFrame *envFrame;
  RwTexture *envTex;
  float envCoeff;
  int envFBalpha;
  int pad;
  int effect;
};


struct MatFX
{
  MatFXEnv fx[2];
  int effects;
};
