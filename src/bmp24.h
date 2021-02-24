#ifndef _BMP_H_
#define _BMP_H_

#include "SPIFFS.h"

typedef struct _BMP BMP;

#ifdef __cplusplus
extern "C"
{
#endif
	void BMP_Free(BMP *bmp);
	BMP *BMP_ReadFile(File f);
	unsigned int BMP_GetWidth(BMP *bmp);
	unsigned int BMP_GetHeight(BMP *bmp);
	void BMP_GetPixelRGB(File f, BMP *bmp, unsigned int x, unsigned int y, unsigned char *r, unsigned char *g, unsigned char *b);

#ifdef __cplusplus
}
#endif

#endif
