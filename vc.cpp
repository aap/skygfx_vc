#include "skygfx_vc.h"

WRAPPER RwMatrix *RwMatrixCreate(void) { EAXJMP(0x644620); }
WRAPPER RwBool RwMatrixDestroy(RwMatrix*) { EAXJMP(0x6445F0); }
WRAPPER RwMatrix *RwMatrixMultiply(RwMatrix*, const RwMatrix*, const RwMatrix*) { EAXJMP(0x6437C0); }
WRAPPER RwMatrix *RwMatrixInvert(RwMatrix*, const RwMatrix*) { EAXJMP(0x643F40); }
WRAPPER RwMatrix *RwMatrixUpdate(RwMatrix*) { EAXJMP(0x6437B0); }

WRAPPER RwFrame *RwFrameCreate(void) { EAXJMP(0x644AA0); }
WRAPPER RwFrame *RwFrameUpdateObjects(RwFrame*) { EAXJMP(0x644D00); }
WRAPPER RwMatrix *RwFrameGetLTM(RwFrame*) { EAXJMP(0x644D80); }
WRAPPER RwFrame *RwFrameTransform(RwFrame*, const RwMatrix*, RwOpCombineType) { EAXJMP(0x6451E0); }
WRAPPER RwCamera *RwCameraClear(RwCamera*, RwRGBA*, RwInt32) { EAXJMP(0x64A9D0); }

WRAPPER RwCamera *RwCameraCreate(void) { EAXJMP(0x64AB50); }
WRAPPER RwCamera *RwCameraBeginUpdate(RwCamera*) { EAXJMP(0x64A820); }
WRAPPER RwCamera *RwCameraEndUpdate(RwCamera*) { EAXJMP(0x64A810); }
WRAPPER RwCamera *RwCameraSetNearClipPlane(RwCamera*, RwReal) { EAXJMP(0x64A860); }
WRAPPER RwCamera *RwCameraSetFarClipPlane(RwCamera*, RwReal) { EAXJMP(0x64A930); }
WRAPPER RwCamera *RwCameraSetViewWindow(RwCamera*, const RwV2d*) { EAXJMP(0x64AAA0); }

WRAPPER RwRaster *RwRasterCreate(RwInt32, RwInt32, RwInt32, RwInt32) { EAXJMP(0x655490); }
WRAPPER RwRaster *RwRasterSetFromImage(RwRaster*, RwImage*) { EAXJMP(0x6602B0); }
WRAPPER RwRaster *RwRasterPushContext(RwRaster*) { EAXJMP(0x655320); }
WRAPPER RwRaster *RwRasterPopContext(void) { EAXJMP(0x6553D0); }
WRAPPER RwRaster *RwRasterRenderFast(RwRaster*, RwInt32, RwInt32) { EAXJMP(0x655270); }

WRAPPER RwTexture *RwTextureCreate(RwRaster*) { EAXJMP(0x64DE60); }

WRAPPER RwBool RwRenderStateGet(RwRenderState, void*) { EAXJMP(0x649BF0); }
WRAPPER RwBool RwRenderStateSet(RwRenderState, void*) { EAXJMP(0x649BA0); }

WRAPPER RwBool RwD3D8SetTexture(RwTexture*, RwUInt32) { EAXJMP(0x659850); }
WRAPPER RwBool RwD3D8SetRenderState(RwUInt32, RwUInt32) { EAXJMP(0x6582A0); }
WRAPPER void RwD3D8GetRenderState(RwUInt32, void*) { EAXJMP(0x6582F0); }
WRAPPER RwBool RwD3D8SetTextureStageState(RwUInt32, RwUInt32, RwUInt32) { EAXJMP(0x658310); }
WRAPPER RwBool RwD3D8SetVertexShader(RwUInt32) { EAXJMP(0x65F2F0); }
WRAPPER RwBool RwD3D8SetPixelShader(RwUInt32) { EAXJMP(0x65F330); }
WRAPPER RwBool RwD3D8SetStreamSource(RwUInt32, void*, RwUInt32) { EAXJMP(0x65F370); }
WRAPPER RwBool RwD3D8SetIndices(void*, RwUInt32) { EAXJMP(0x65F3C0); }
WRAPPER RwBool RwD3D8DrawIndexedPrimitive(RwUInt32, RwUInt32, RwUInt32, RwUInt32, RwUInt32) { EAXJMP(0x65F410); }
WRAPPER RwBool RwD3D8DrawPrimitive(RwUInt32, RwUInt32, RwUInt32) { EAXJMP(0x65F4A0); }
WRAPPER RwBool RwD3D8SetSurfaceProperties(const RwRGBA*, const RwSurfaceProperties*, RwBool) { EAXJMP(0x65F7F0); }
WRAPPER RwBool RwD3D8SetTransform(RwUInt32, const void*) { EAXJMP(0x65F530); }
WRAPPER void RwD3D8GetTransform(RwUInt32, void*) { EAXJMP(0x65F670); }

WRAPPER RwBool RwD3D8SetLight(RwInt32, const void*) { EAXJMP(0x65FB20); }
WRAPPER RwBool RwD3D8EnableLight(RwInt32, RwBool) { EAXJMP(0x65FC10); }

WRAPPER RwBool rwD3D8RenderStateIsVertexAlphaEnable(void) { EAXJMP(0x659F60); };
WRAPPER void rwD3D8RenderStateVertexAlphaEnable(RwBool x) { EAXJMP(0x659CF0); };
WRAPPER const RwSphere *RpAtomicGetWorldBoundingSphere(RpAtomic*) { EAXJMP(0x640710); };
WRAPPER RwBool RwD3D8CameraIsBBoxFullyInsideFrustum(const void*, const void*) { EAXJMP(0x660010); };

WRAPPER RwImage *RtBMPImageRead(const RwChar*) { EAXJMP(0x657870); }
WRAPPER RwImage *RwImageFindRasterFormat(RwImage*, RwInt32, RwInt32*, RwInt32*, RwInt32*, RwInt32*) { EAXJMP(0x6602E0); }
WRAPPER RwBool RwImageDestroy(RwImage*) { EAXJMP(0x6512B0); }

WRAPPER RwReal RwIm2DGetNearScreenZ(void) { EAXJMP(0x649B80); }
WRAPPER RwBool RwIm2DRenderIndexedPrimitive(RwPrimitiveType, RwIm2DVertex*, RwInt32, RwImVertexIndex*, RwInt32) { EAXJMP(0x649C20); }

WRAPPER void _rwObjectHasFrameSetFrame(void*, RwFrame*) { EAXJMP(0x660CC0); }