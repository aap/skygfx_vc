#include "skygfx.h"

void GlossAttach(void);

static addr PluginAttach_A;
WRAPPER int PluginAttach(void) { VARJMP(PluginAttach_A); }
int
PluginAttach_hook(void)
{
	int ret = PluginAttach();
	GlossAttach();
	return ret;
}

RwInt32 GlossOffset;

void *GlossConstructor(void *object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
	GlossMatExt *glossext = GETGLOSSEXT(object);
	glossext->didLookup = false;
	glossext->glosstex = NULL;
	return object;
}

void *GlossDestructor(void *object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
	return object;
}

void *GlossCopy(void *dstObject, const void *srcObject, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
	*GETGLOSSEXT(dstObject) = *GETGLOSSEXT(srcObject);
	return dstObject;
}

void GlossAttach(void)
{
	GlossOffset = RpMaterialRegisterPlugin(sizeof(GlossMatExt), MAKECHUNKID(0xaffe, 0), GlossConstructor, GlossDestructor, GlossCopy);
}

void
hookplugins(void)
{
	// ADDRESS
	InterceptCall(&PluginAttach_A, PluginAttach_hook, AddressByVersion<addr>(0x48E8D2, 0, 0, 0x4A5BB5, 0, 0));
}
