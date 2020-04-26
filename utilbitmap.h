#ifndef UTILBITMAP_H
#define UTILBITMAP_H

void BitOrderInvert(unsigned char *buf, int nbytes);
void TwoByteSwap(unsigned char *buf, int nbytes);
void FourByteSwap(unsigned char *buf, int nbytes);
int RepadBitmap (char *pSrc, char *pDst, unsigned int srcPad,
                 unsigned int dstPad, int width, int height);

#endif /* UTILBITMAP_H */
