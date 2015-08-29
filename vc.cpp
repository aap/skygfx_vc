#include "skygfx_vc.h"

static uint32_t RwMatrixCreate_A = AddressByVersion<uint32_t>(0x5A3330, 0x644620);
WRAPPER RwMatrix *RwMatrixCreate(void) { VARJMP(RwMatrixCreate_A); }
static uint32_t RwMatrixDestroy_A = AddressByVersion<uint32_t>(0x5A3300, 0x6445F0);
WRAPPER RwBool RwMatrixDestroy(RwMatrix*) { VARJMP(RwMatrixDestroy_A); }
static uint32_t RwMatrixMultiply_A = AddressByVersion<uint32_t>(0x5A28F0, 0x6437C0);
WRAPPER RwMatrix *RwMatrixMultiply(RwMatrix*, const RwMatrix*, const RwMatrix*) { VARJMP(RwMatrixMultiply_A); }
static uint32_t RwMatrixInvert_A = AddressByVersion<uint32_t>(0x5A2C90, 0x643F40);
WRAPPER RwMatrix *RwMatrixInvert(RwMatrix*, const RwMatrix*) { VARJMP(RwMatrixInvert_A); }
static uint32_t RwMatrixUpdate_A = AddressByVersion<uint32_t>(0x5A28E0, 0x6437B0);
WRAPPER RwMatrix *RwMatrixUpdate(RwMatrix*) { VARJMP(RwMatrixUpdate_A); }
static uint32_t RwMatrixRotate_A = AddressByVersion<uint32_t>(0x5A2BF0, 0x643EA0);
WRAPPER RwMatrix *RwMatrixRotate(RwMatrix*, const RwV3d*, RwReal, RwOpCombineType) { VARJMP(RwMatrixRotate_A); }

static uint32_t RwFrameCreate_A = AddressByVersion<uint32_t>(0x5A1A00, 0x644AA0);
WRAPPER RwFrame *RwFrameCreate(void) { VARJMP(RwFrameCreate_A); }
static uint32_t RwFrameUpdateObjects_A = AddressByVersion<uint32_t>(0x5A1C60, 0x644D00);
WRAPPER RwFrame *RwFrameUpdateObjects(RwFrame*) { VARJMP(RwFrameUpdateObjects_A); }
static uint32_t RwFrameGetLTM_A = AddressByVersion<uint32_t>(0x5A1CE0, 0x644D80);
WRAPPER RwMatrix *RwFrameGetLTM(RwFrame*) { VARJMP(RwFrameGetLTM_A); }
static uint32_t RwFrameTransform_A = AddressByVersion<uint32_t>(0x5A2140, 0x6451E0);
WRAPPER RwFrame *RwFrameTransform(RwFrame*, const RwMatrix*, RwOpCombineType) { VARJMP(RwFrameTransform_A); }

static uint32_t RwCameraCreate_A = AddressByVersion<uint32_t>(0x5A5360, 0x64AB50);
WRAPPER RwCamera *RwCameraCreate(void) { VARJMP(RwCameraCreate_A); }
static uint32_t RwCameraBeginUpdate_A = AddressByVersion<uint32_t>(0x5A5030, 0x64A820);
WRAPPER RwCamera *RwCameraBeginUpdate(RwCamera*) { VARJMP(RwCameraBeginUpdate_A); }
static uint32_t RwCameraEndUpdate_A = AddressByVersion<uint32_t>(0x5A5020, 0x64A810);
WRAPPER RwCamera *RwCameraEndUpdate(RwCamera*) { VARJMP(RwCameraEndUpdate_A); }
static uint32_t RwCameraSetNearClipPlane_A = AddressByVersion<uint32_t>(0x5A5070, 0x64A860);
WRAPPER RwCamera *RwCameraSetNearClipPlane(RwCamera*, RwReal) { VARJMP(RwCameraSetNearClipPlane_A); }
static uint32_t RwCameraSetFarClipPlane_A = AddressByVersion<uint32_t>(0x5A5140, 0x64A930);
WRAPPER RwCamera *RwCameraSetFarClipPlane(RwCamera*, RwReal) { VARJMP(RwCameraSetFarClipPlane_A); }
static uint32_t RwCameraSetViewWindow_A = AddressByVersion<uint32_t>(0x5A52B0, 0x64AAA0);
WRAPPER RwCamera *RwCameraSetViewWindow(RwCamera*, const RwV2d*) { VARJMP(RwCameraSetViewWindow_A); }
static uint32_t RwCameraClear_A = AddressByVersion<uint32_t>(0x5A51E0, 0x64A9D0);
WRAPPER RwCamera *RwCameraClear(RwCamera*, RwRGBA*, RwInt32) { VARJMP(RwCameraClear_A); }

static uint32_t RwRasterCreate_A = AddressByVersion<uint32_t>(0x5AD930, 0x655490);
WRAPPER RwRaster *RwRasterCreate(RwInt32, RwInt32, RwInt32, RwInt32) { VARJMP(RwRasterCreate_A); }
static uint32_t RwRasterSetFromImage_A = AddressByVersion<uint32_t>(0x5BBF50, 0x6602B0);
WRAPPER RwRaster *RwRasterSetFromImage(RwRaster*, RwImage*) { VARJMP(RwRasterSetFromImage_A); }
static uint32_t RwRasterPushContext_A = AddressByVersion<uint32_t>(0x5AD7C0, 0x655320);
WRAPPER RwRaster *RwRasterPushContext(RwRaster*) { VARJMP(RwRasterPushContext_A); }
static uint32_t RwRasterPopContext_A = AddressByVersion<uint32_t>(0x5AD870, 0x6553D0);
WRAPPER RwRaster *RwRasterPopContext(void) { VARJMP(RwRasterPopContext_A); }
static uint32_t RwRasterRenderFast_A = AddressByVersion<uint32_t>(0x5AD710, 0x655270);
WRAPPER RwRaster *RwRasterRenderFast(RwRaster*, RwInt32, RwInt32) { VARJMP(RwRasterRenderFast_A); }

static uint32_t RwTextureCreate_A = AddressByVersion<uint32_t>(0x5A72D0, 0x64DE60);
WRAPPER RwTexture *RwTextureCreate(RwRaster*) { VARJMP(RwTextureCreate_A); }

static uint32_t RwRenderStateGet_A = AddressByVersion<uint32_t>(0x5A4410, 0x649BF0);
WRAPPER RwBool RwRenderStateGet(RwRenderState, void*) { VARJMP(RwRenderStateGet_A); }
static uint32_t RwRenderStateSet_A = AddressByVersion<uint32_t>(0x5A43C0, 0x649BA0);
WRAPPER RwBool RwRenderStateSet(RwRenderState, void*) { VARJMP(RwRenderStateSet_A); }

static uint32_t RwD3D8SetTexture_A = AddressByVersion<uint32_t>(0x5B53A0, 0x659850);
WRAPPER RwBool RwD3D8SetTexture(RwTexture*, RwUInt32) { VARJMP(RwD3D8SetTexture_A); }
static uint32_t RwD3D8SetRenderState_A = AddressByVersion<uint32_t>(0x5B3CF0, 0x6582A0);
WRAPPER RwBool RwD3D8SetRenderState(RwUInt32, RwUInt32) { VARJMP(RwD3D8SetRenderState_A); }
static uint32_t RwD3D8GetRenderState_A = AddressByVersion<uint32_t>(0x5B3D40, 0x6582F0);
WRAPPER void RwD3D8GetRenderState(RwUInt32, void*) { VARJMP(RwD3D8GetRenderState_A); }
static uint32_t RwD3D8SetTextureStageState_A = AddressByVersion<uint32_t>(0x5B3D60, 0x658310);
WRAPPER RwBool RwD3D8SetTextureStageState(RwUInt32, RwUInt32, RwUInt32) { VARJMP(RwD3D8SetTextureStageState_A); }
static uint32_t RwD3D8SetVertexShader_A = AddressByVersion<uint32_t>(0x5BAF90, 0x65F2F0);
WRAPPER RwBool RwD3D8SetVertexShader(RwUInt32) { VARJMP(RwD3D8SetVertexShader_A); }
static uint32_t RwD3D8SetStreamSource_A = AddressByVersion<uint32_t>(0x5BB010, 0x65F370);
WRAPPER RwBool RwD3D8SetStreamSource(RwUInt32, void*, RwUInt32) { VARJMP(RwD3D8SetStreamSource_A); }
static uint32_t RwD3D8SetIndices_A = AddressByVersion<uint32_t>(0x5BB060, 0x65F3C0);
WRAPPER RwBool RwD3D8SetIndices(void*, RwUInt32) { VARJMP(RwD3D8SetIndices_A); }
static uint32_t RwD3D8DrawIndexedPrimitive_A = AddressByVersion<uint32_t>(0x5BB0B0, 0x65F410);
WRAPPER RwBool RwD3D8DrawIndexedPrimitive(RwUInt32, RwUInt32, RwUInt32, RwUInt32, RwUInt32) { VARJMP(RwD3D8DrawIndexedPrimitive_A); }
static uint32_t RwD3D8DrawPrimitive_A = AddressByVersion<uint32_t>(0x5BB140, 0x65F4A0);
WRAPPER RwBool RwD3D8DrawPrimitive(RwUInt32, RwUInt32, RwUInt32) { VARJMP(RwD3D8DrawPrimitive_A); }
static uint32_t RwD3D8SetSurfaceProperties_A = AddressByVersion<uint32_t>(0x5BB490, 0x65F7F0);
WRAPPER RwBool RwD3D8SetSurfaceProperties(const RwRGBA*, const RwSurfaceProperties*, RwBool) { VARJMP(RwD3D8SetSurfaceProperties_A); }
static uint32_t RwD3D8SetTransform_A = AddressByVersion<uint32_t>(0x5BB1D0, 0x65F530);
WRAPPER RwBool RwD3D8SetTransform(RwUInt32, const void*) { VARJMP(RwD3D8SetTransform_A); }
static uint32_t RwD3D8GetTransform_A = AddressByVersion<uint32_t>(0x5BB310, 0x65F670);
WRAPPER void RwD3D8GetTransform(RwUInt32, void*) { VARJMP(RwD3D8GetTransform_A); }

//WRAPPER RwBool RwD3D8SetLight(RwInt32, const void*) { EAXJMP(0x65FB20); }
//WRAPPER RwBool RwD3D8EnableLight(RwInt32, RwBool) { EAXJMP(0x65FC10); }

static uint32_t rwD3D8RenderStateIsVertexAlphaEnable_A = AddressByVersion<uint32_t>(0x5B5A50, 0x659F60);
WRAPPER RwBool rwD3D8RenderStateIsVertexAlphaEnable(void) { VARJMP(rwD3D8RenderStateIsVertexAlphaEnable_A); };
static uint32_t rwD3D8RenderStateVertexAlphaEnable_A = AddressByVersion<uint32_t>(0x5B57E0, 0x659CF0);
WRAPPER void rwD3D8RenderStateVertexAlphaEnable(RwBool x) { VARJMP(rwD3D8RenderStateVertexAlphaEnable_A); };
static uint32_t RpAtomicGetWorldBoundingSphere_A = AddressByVersion<uint32_t>(0x59E800, 0x640710);
WRAPPER const RwSphere *RpAtomicGetWorldBoundingSphere(RpAtomic*) { VARJMP(RpAtomicGetWorldBoundingSphere_A); };
static uint32_t RwD3D8CameraIsSphereFullyInsideFrustum_A = AddressByVersion<uint32_t>(0x5BBC40, 0x65FFB0);
WRAPPER RwBool RwD3D8CameraIsSphereFullyInsideFrustum(const void*, const void*) { VARJMP(RwD3D8CameraIsSphereFullyInsideFrustum_A); };
static uint32_t RwD3D8CameraIsBBoxFullyInsideFrustum_A = AddressByVersion<uint32_t>(0x5BBCA0, 0x660010);
WRAPPER RwBool RwD3D8CameraIsBBoxFullyInsideFrustum(const void*, const void*) { VARJMP(RwD3D8CameraIsBBoxFullyInsideFrustum_A); };

static uint32_t RtBMPImageRead_A = AddressByVersion<uint32_t>(0x5AFE70, 0x657870);
WRAPPER RwImage *RtBMPImageRead(const RwChar*) { VARJMP(RtBMPImageRead_A); }
static uint32_t RwImageFindRasterFormat_A = AddressByVersion<uint32_t>(0x5BBF80, 0x6602E0);
WRAPPER RwImage *RwImageFindRasterFormat(RwImage*, RwInt32, RwInt32*, RwInt32*, RwInt32*, RwInt32*) { VARJMP(RwImageFindRasterFormat_A); }
static uint32_t RwImageDestroy_A = AddressByVersion<uint32_t>(0x5A9180, 0x6512B0);
WRAPPER RwBool RwImageDestroy(RwImage*) { VARJMP(RwImageDestroy_A); }

static uint32_t RwIm2DGetNearScreenZ_A = AddressByVersion<uint32_t>(0x5A43A0, 0x649B80);
WRAPPER RwReal RwIm2DGetNearScreenZ(void) { VARJMP(RwIm2DGetNearScreenZ_A); }
static uint32_t RwIm2DRenderIndexedPrimitive_A = AddressByVersion<uint32_t>(0x5A4440, 0x649C20);
WRAPPER RwBool RwIm2DRenderIndexedPrimitive(RwPrimitiveType, RwIm2DVertex*, RwInt32, RwImVertexIndex*, RwInt32) { VARJMP(RwIm2DRenderIndexedPrimitive_A); }

static uint32_t _rwObjectHasFrameSetFrame_A = AddressByVersion<uint32_t>(0x5BC950, 0x660CC0);
WRAPPER void _rwObjectHasFrameSetFrame(void*, RwFrame*) { VARJMP(_rwObjectHasFrameSetFrame_A); }