#include "skygfx_vc.h"

WRAPPER RwFrame *RwFrameCreate(void) { EAXJMP(0x644AA0); }
WRAPPER RwFrame *RwFrameUpdateObjects(RwFrame*) { EAXJMP(0x644D00); }
WRAPPER RwMatrix *RwFrameGetLTM(RwFrame*) { EAXJMP(0x644D80); }

WRAPPER RwBool RwRenderStateGet(RwRenderState, void*) { EAXJMP(0x649BF0); }
WRAPPER RwBool RwRenderStateSet(RwRenderState, void*) { EAXJMP(0x649BA0); }

WRAPPER RwBool RwD3D8SetTexture(RwTexture*, RwUInt32) { EAXJMP(0x659850); }
WRAPPER RwBool RwD3D8SetRenderState(RwUInt32, RwUInt32) { EAXJMP(0x6582A0); }
WRAPPER void RwD3D8GetRenderState(RwUInt32, void*) { EAXJMP(0x6582F0); }
WRAPPER RwBool RwD3D8SetTextureStageState(RwUInt32, RwUInt32, RwUInt32) { EAXJMP(0x658310); }
WRAPPER RwBool RwD3D8SetVertexShader(RwUInt32) { EAXJMP(0x65F2F0); }
WRAPPER RwBool RwD3D8SetStreamSource(RwUInt32, void*, RwUInt32) { EAXJMP(0x65F370); }
WRAPPER RwBool RwD3D8SetIndices(void*, RwUInt32) { EAXJMP(0x65F3C0); }
WRAPPER RwBool RwD3D8DrawIndexedPrimitive(RwUInt32, RwUInt32, RwUInt32, RwUInt32, RwUInt32) { EAXJMP(0x65F410); }
WRAPPER RwBool RwD3D8DrawPrimitive(RwUInt32, RwUInt32, RwUInt32) { EAXJMP(0x65F4A0); }
WRAPPER RwBool RwD3D8SetSurfaceProperties(const RwRGBA*, const RwSurfaceProperties*, RwBool) { EAXJMP(0x65F7F0); }

WRAPPER RwBool RwD3D8SetLight(RwInt32, const void*) { EAXJMP(0x65FB20); }
WRAPPER RwBool RwD3D8EnableLight(RwInt32, RwBool) { EAXJMP(0x65FC10); }