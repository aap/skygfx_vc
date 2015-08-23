#include "skygfx_vc.h"
#include <initguid.h>
#include <d3d9.h>
#include <d3d9types.h>

IDirect3DDevice9 *d3d9device = NULL;

int
initD3D9(void *d3d8device)
{
	IUnknown *foo = (IUnknown*)d3d8device;
	foo->QueryInterface(IID_IDirect3DDevice9, (void**)&d3d9device);
	if(d3d9device == NULL)
		return 0;
	return 1;
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
	// TODO: cache
	HRESULT res = d3d9device->CreateVertexShader((DWORD*)function, (IDirect3DVertexShader9**)shader);
	return res >= 0;
}

RwBool
RwD3D9CreatePixelShader(const RwUInt32 *function, void **shader)
{
	// TODO: cache
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
	d3d9device->SetVertexShaderConstantF(registerAddress, (const float*)constantData, constantCount);
	d3d9device->SetPixelShaderConstantF(registerAddress, (const float*)constantData, constantCount);
}
