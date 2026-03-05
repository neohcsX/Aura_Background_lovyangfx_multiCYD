#include "SD.h"
#include "bmpLoader.h"

#pragma pack(push, 1)
struct BMPFileHeader {
  uint16_t bfType;
  uint32_t bfSize;
  uint16_t bfReserved1;
  uint16_t bfReserved2;
  uint32_t bfOffBits;
};

struct BMPInfoHeader {
  uint32_t biSize;
  int32_t  biWidth;
  int32_t  biHeight;
  uint16_t biPlanes;
  uint16_t biBitCount;
  uint32_t biCompression;
  uint32_t biSizeImage;
  int32_t  biXPelsPerMeter;
  int32_t  biYPelsPerMeter;
  uint32_t biClrUsed;
  uint32_t biClrImportant;
};
#pragma pack(pop)

uint16_t* bg_small = nullptr;

const uint16_t* load_bg_bmp_565_120x160(const char* path)
{

  if (bg_small == nullptr) {
    bg_small = (uint16_t*)malloc(BG_W2 * BG_H2 * sizeof(uint16_t));
    if (!bg_small) {
      Serial.println("Failed to allocate memory for background image");
      return nullptr;
    }
  }

  File f = SD.open(path, FILE_READ);
  if(!f) { Serial.printf("SD.open failed: %s\n", path); return nullptr; }

  BMPFileHeader fh;
  BMPInfoHeader ih;

  if(f.read((uint8_t*)&fh, sizeof(fh)) != sizeof(fh)) { f.close(); return nullptr; }
  if(f.read((uint8_t*)&ih, sizeof(ih)) != sizeof(ih)) { f.close(); return nullptr; }

  if(fh.bfType != 0x4D42) { Serial.printf("Not a BMP: %s\n", path); f.close(); return nullptr; }

  int w = ih.biWidth;
  int h = ih.biHeight;
  bool topDown = false;
  if(h < 0) { topDown = true; h = -h; }

  if(w != BG_W2 || h != BG_H2) {
    Serial.printf("BMP wrong size %dx%d (need %dx%d): %s\n", w, h, BG_W2, BG_H2, path);
    f.close(); return nullptr;
  }

  const uint16_t bpp = ih.biBitCount;
  const uint32_t comp = ih.biCompression;

  f.seek(fh.bfOffBits);

  if(bpp == 16) {
    if(comp != 0 && comp != 3) {
      Serial.printf("Unsupported BMP compression (16bpp): %u\n", (unsigned)comp);
      f.close(); return nullptr;
    }

    uint32_t rowBytes = ((w * 16 + 31) / 32) * 4;
    static uint8_t rowBuf[BG_W2 * 2 + 4];

    for(int row = 0; row < h; row++) {
      int dstY = topDown ? row : (h - 1 - row);
      if(f.read(rowBuf, rowBytes) != rowBytes) { f.close(); return nullptr; }

      for(int x = 0; x < w; x++) {
        uint16_t px = (uint16_t)rowBuf[x*2] | ((uint16_t)rowBuf[x*2 + 1] << 8);
        bg_small[dstY * w + x] = px;
      }
    }

    f.close();
    return bg_small;
  }

  if(bpp == 24 && comp == 0) {
    uint32_t rowBytes = ((w * 24 + 31) / 32) * 4;
    static uint8_t rowBuf[BG_W2 * 3 + 4];

    for(int row = 0; row < h; row++) {
      int dstY = topDown ? row : (h - 1 - row);
      if(f.read(rowBuf, rowBytes) != rowBytes) { f.close(); return nullptr; }

      for(int x = 0; x < w; x++) {
        uint8_t b = rowBuf[x*3 + 0];
        uint8_t g = rowBuf[x*3 + 1];
        uint8_t r = rowBuf[x*3 + 2];
        bg_small[dstY * w + x] =
          ((r & 0xF8) << 8) |
          ((g & 0xFC) << 3) |
          (b >> 3);
      }
    }

    f.close();
    return bg_small;
  }

  Serial.printf("Unsupported BMP format bpp=%u comp=%u: %s\n", bpp, (unsigned)comp, path);
  f.close();
  return nullptr;

}