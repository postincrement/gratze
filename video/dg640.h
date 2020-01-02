#ifndef DG640_H_
#define DG640_H_

#define   DG640_VIDEO_RAM_SIZE_K        2

#define   DG640_FONT_WIDTH               8
#define   DG640_FONT_HEIGHT              16

#define   DG640_SCREEN_WIDTH_CHARS      64
#define   DG640_SCREEN_HEIGHT_CHARS     16

#define   DG640_SCREEN_WIDTH_PIXELS     (DG640_SCREEN_WIDTH_CHARS * DG640_FONT_WIDTH)
#define   DG640_SCREEN_HEIGHT_PIXELS    (DG640_SCREEN_HEIGHT_CHARS * DG640_FONT_HEIGHT)

extern void CreateDG640PixelData(const Options & options, const Emulator::FontInfo & fontInfo, std::vector<uint8_t> & fontData);

#define DG640_VIDEO_DRIVER(addr) \
  INFO_VIDEO_MEMORY_MAPPED(addr, addr + DG640_VIDEO_RAM_SIZE_K * 1024 - 1, \
                           DG640_SCREEN_WIDTH_CHARS, \
                           DG640_SCREEN_HEIGHT_CHARS, \
                           DG640_FONT_WIDTH,  \
                           DG640_FONT_HEIGHT, \
                           256, NULL, &CreateDG640PixelData)


#endif // DG640_H_