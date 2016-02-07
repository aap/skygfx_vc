#include "skygfx.h"

RwTexture *rampTex;

#pragma pack(push, 1)
struct TGAHeader
{
	RwInt8  IDlen;
	RwInt8  colorMapType;
	RwInt8  imageType;
	RwInt16 colorMapOrigin;
	RwInt16 colorMapLength;
	RwInt8  colorMapDepth;
	RwInt16 xOrigin, yOrigin;
	RwInt16 width, height;
	RwUInt8 depth;
	RwUInt8 descriptor;
};
#pragma pack(pop)

RwImage*
readTGA(const char *afilename)
{
	RwImage *image;

	RwStream *stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, afilename);

	TGAHeader header;
	int depth = 0, palDepth = 0;
	RwStreamRead(stream, &header, sizeof(header));

	assert(header.imageType == 1 || header.imageType == 2);
	RwStreamSkip(stream, header.IDlen);
	if(header.colorMapType){
		assert(header.colorMapOrigin == 0);
		depth = (header.colorMapLength <= 16) ? 4 : 8;
		palDepth = header.colorMapDepth;
		assert(palDepth == 24 || palDepth == 32);
	}else{
		depth = header.depth;
		assert(depth == 24 || depth == 32);
	}

	image = RwImageCreate(header.width, header.height, depth == 24 ? 32 : depth);
	RwImageAllocatePixels(image);
	RwRGBA *palette = header.colorMapType ? image->palette : NULL;
	if(palette){
		int maxlen = depth == 4 ? 16 : 256;
		int i;
		for(i = 0; i < header.colorMapLength; i++){
			RwStreamRead(stream, &palette[i].blue, 1);
			RwStreamRead(stream, &palette[i].green, 1);
			RwStreamRead(stream, &palette[i].red, 1);
			palette[i].alpha = 0xFF;
			if(palDepth == 32)
				RwStreamRead(stream, &palette[i].alpha, 1);
		}
		for(; i < maxlen; i++){
			palette[i].red = palette[i].green = palette[i].blue = 0;
			palette[i].alpha = 0xFF;
		}
	}

	RwUInt8 *pixels = image->cpPixels;
	if(!(header.descriptor & 0x20))
		pixels += (image->height-1)*image->stride;
	for(int y = 0; y < image->height; y++){
		RwUInt8 *line = pixels;
		for(int x = 0; x < image->width; x++){
			if(palette)
				RwStreamRead(stream, line++, 1);
			else{
				RwStreamRead(stream, &line[2], 1);
				RwStreamRead(stream, &line[1], 1);
				RwStreamRead(stream, &line[0], 1);
				line += 3;
				if(depth == 32)
					RwStreamRead(stream, line++, 1);
				if(depth == 24)
					*line++ = 0xFF;
			}
		}
		pixels += (header.descriptor&0x20) ?
		              image->stride : -image->stride;
	}

	RwStreamClose(stream, NULL);
	return image;
}

void
reloadRamp(void)
{
	RwImage *img = readTGA("curves.tga");
	RwInt32 w, h, d, flags;
	RwImageFindRasterFormat(img, 4, &w, &h, &d, &flags);
	RwRaster *ras = RwRasterCreate(w, h, d, flags);
	ras = RwRasterSetFromImage(ras, img);
	assert(ras);
	rampTex = RwTextureCreate(ras);
	RwTextureAddRef(rampTex);
	RwImageDestroy(img);
}