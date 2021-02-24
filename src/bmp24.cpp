#include "bmp24.h"

typedef struct _BMP_Header
{
	unsigned short Magic;		  /* Magic identifier: "BM" */
	unsigned int FileSize;		  /* Size of the BMP file in bytes */
	unsigned short Reserved1;	  /* Reserved */
	unsigned short Reserved2;	  /* Reserved */
	unsigned int DataOffset;	  /* Offset of image data relative to the file's start */
	unsigned int HeaderSize;	  /* Size of the header in bytes */
	unsigned int Width;			  /* Bitmap's width */
	unsigned int Height;		  /* Bitmap's height */
	unsigned short Planes;		  /* Number of color planes in the bitmap */
	unsigned short BitsPerPixel;  /* Number of bits per pixel */
	unsigned int CompressionType; /* Compression type */
	unsigned int ImageDataSize;	  /* Size of uncompressed image's data */
	unsigned int HPixelsPerMeter; /* Horizontal resolution (pixels per meter) */
	unsigned int VPixelsPerMeter; /* Vertical resolution (pixels per meter) */
	unsigned int ColorsUsed;	  /* Number of color indexes in the color table that are actually used by the bitmap */
	unsigned int ColorsRequired;  /* Number of color indexes that are required for displaying the bitmap */
} BMP_Header;

struct _BMP
{
	BMP_Header Header;
};

bool ReadHeader(BMP *bmp, File f);
int ReadUINT(unsigned int *x, File f);
int ReadUSHORT(unsigned short *x, File f);

void BMP_Free(BMP *bmp)
{
	if (bmp == NULL)
	{
		return;
	}
	free(bmp);
}

BMP *BMP_ReadFile(File f)
{
	BMP *bmp;
	if (f == NULL)
	{
		return NULL;
	}
	bmp = (BMP *)malloc(sizeof(BMP));
	if (bmp == NULL)
	{
		return NULL;
	}

	if (!ReadHeader(bmp, f) || bmp->Header.Magic != 0x4D42)
	{
		f.close();
		free(bmp);
		return NULL;
	}

	if ((bmp->Header.BitsPerPixel != 32 && bmp->Header.BitsPerPixel != 24 && bmp->Header.BitsPerPixel != 8) || bmp->Header.CompressionType != 0 || bmp->Header.HeaderSize != 40)
	{
		f.close();
		free(bmp);
		return NULL;
	}

	if (bmp->Header.BitsPerPixel == 8)
	{
		f.close();
		free(bmp);
		return NULL;
	}

	if (bmp->Header.ImageDataSize == 0)
	{
		bmp->Header.ImageDataSize = 3 * bmp->Header.Width * bmp->Header.Height;
	}

	return bmp;
}

unsigned int BMP_GetWidth(BMP *bmp)
{
	if (bmp == NULL)
	{
		return -1;
	}
	return (bmp->Header.Width);
}

unsigned int BMP_GetHeight(BMP *bmp)
{
	if (bmp == NULL)
	{
		return -1;
	}
	return (bmp->Header.Height);
}

void BMP_GetPixelRGB(File f, BMP *bmp, unsigned int x, unsigned int y, unsigned char *r, unsigned char *g, unsigned char *b)
{
	unsigned char pixel[3];
	unsigned int bytes_per_row;
	unsigned char bytes_per_pixel;

	if (bmp == NULL || x >= bmp->Header.Width || y >= bmp->Header.Height)
	{
		return;
	}
	bytes_per_pixel = bmp->Header.BitsPerPixel >> 3;
	bytes_per_row = bmp->Header.ImageDataSize / bmp->Header.Height;

	uint32_t target_pos = bmp->Header.DataOffset + ((bmp->Header.Height - y - 1) * bytes_per_row + x * bytes_per_pixel);
	f.seek(target_pos);
	if (f.read(pixel, 3) < 0)
	{
		return;
	}

	if (r)
		*r = pixel[2];
	if (g)
		*g = pixel[1];
	if (b)
		*b = pixel[0];
}

bool ReadHeader(BMP *bmp, File f)
{
	if (bmp == NULL || f == NULL)
	{
		return false;
	}

	if (!ReadUSHORT(&(bmp->Header.Magic), f))
		return false;
	if (!ReadUINT(&(bmp->Header.FileSize), f))
		return false;
	if (!ReadUSHORT(&(bmp->Header.Reserved1), f))
		return false;
	if (!ReadUSHORT(&(bmp->Header.Reserved2), f))
		return false;
	if (!ReadUINT(&(bmp->Header.DataOffset), f))
		return false;
	if (!ReadUINT(&(bmp->Header.HeaderSize), f))
		return false;
	if (!ReadUINT(&(bmp->Header.Width), f))
		return false;
	if (!ReadUINT(&(bmp->Header.Height), f))
		return false;
	if (!ReadUSHORT(&(bmp->Header.Planes), f))
		return false;
	if (!ReadUSHORT(&(bmp->Header.BitsPerPixel), f))
		return false;
	if (!ReadUINT(&(bmp->Header.CompressionType), f))
		return false;
	if (!ReadUINT(&(bmp->Header.ImageDataSize), f))
		return false;
	if (!ReadUINT(&(bmp->Header.HPixelsPerMeter), f))
		return false;
	if (!ReadUINT(&(bmp->Header.VPixelsPerMeter), f))
		return false;
	if (!ReadUINT(&(bmp->Header.ColorsUsed), f))
		return false;
	if (!ReadUINT(&(bmp->Header.ColorsRequired), f))
		return false;

	return true;
}

int ReadUINT(unsigned int *x, File f)
{
	unsigned char little[4];

	if (x == NULL || f == NULL)
	{
		return 0;
	}

	if (f.read(little, 4) != 4)
	{
		return 0;
	}
	*x = (little[3] << 24 | little[2] << 16 | little[1] << 8 | little[0]);
	return 1;
}

int ReadUSHORT(unsigned short *x, File f)
{
	unsigned char little[2];

	if (x == NULL || f == NULL)
	{
		return 0;
	}

	if (f.read(little, 2) != 2)
	{
		return 0;
	}
	*x = (little[1] << 8 | little[0]);
	return 1;
}
