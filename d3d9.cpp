#include <windows.h>
#include <rwcore.h>
#include <initguid.h>
#include <d3d9.h>
#include <d3d9types.h>
#include "MemoryMgr.h"

IDirect3DDevice9 *d3d9device = NULL;

IUnknown *&RwD3DDevice = *(IUnknown**)0x7897A8;

WRAPPER void D3D8DeviceSystemStart(void) { EAXJMP(0x65BFC0); }

// get d3d9 device when d3d8 device is created
int
D3D8DeviceSystemStart_hook(void)
{
	D3D8DeviceSystemStart();
	RwD3DDevice->QueryInterface(IID_IDirect3DDevice9, (void**)&d3d9device);
	return 1;
}

void
d3d9attach(void)
{
	MemoryVP::InjectHook(0x65BB1F, D3D8DeviceSystemStart_hook);
	if(*(void**)0x8100BC){
		// loaded late (camera is created)
		RwD3DDevice->QueryInterface(IID_IDirect3DDevice9, (void**)&d3d9device);
	}else{
		// loaded early (no camera yet)
	}
}

RwBool
RwD3D9Supported(void)
{
	return d3d9device != NULL;
}

void
RwD3D9SetVertexShader(void *shader)
{
	d3d9device->SetVertexShader((IDirect3DVertexShader9*)shader);
}

void
RwD3D9SetPixelShader(void *shader)
{
	d3d9device->SetPixelShader((IDirect3DPixelShader9*)shader);
}

RwBool
RwD3D9CreateVertexShader(const RwUInt32 *function, void **shader)
{
	HRESULT res = d3d9device->CreateVertexShader((DWORD*)function, (IDirect3DVertexShader9**)shader);
	return res >= 0;
}

RwBool
RwD3D9CreatePixelShader(const RwUInt32 *function, void **shader)
{
	HRESULT res = d3d9device->CreatePixelShader((DWORD*)function, (IDirect3DPixelShader9**)shader);
	return res >= 0;
}

void
RwD3D9SetFVF(RwUInt32 fvf)
{
	d3d9device->SetFVF(fvf);
}

void
RwD3D9SetVertexShaderConstant(RwUInt32 registerAddress, const void *constantData, RwUInt32 constantCount)
{
	d3d9device->SetVertexShaderConstantF(registerAddress, (const float*)constantData, constantCount);
}

void
RwD3D9SetPixelShaderConstant(RwUInt32 registerAddress, const void *constantData, RwUInt32 constantCount)
{
	d3d9device->SetPixelShaderConstantF(registerAddress, (const float*)constantData, constantCount);
}

void
RwD3D9SetVertexPixelShaderConstant(RwUInt32 registerAddress, const void *constantData, RwUInt32 constantCount)
{
	RwD3D9SetVertexShaderConstant(registerAddress, constantData, constantCount);
	RwD3D9SetPixelShaderConstant(registerAddress, constantData, constantCount);
}
