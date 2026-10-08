#ifndef GRATZE_NFD_EXT_H
#define GRATZE_NFD_EXT_H

#include "nativefiledialog/nfd.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  const nfdchar_t *filterList;
  const nfdchar_t *defaultPath;
  const nfdchar_t *title;
  const nfdchar_t *defaultFilename;
} nfd_OpenDialogExt;

typedef nfd_OpenDialogExt nfd_SaveDialogExt;

nfdresult_t NFD_OpenDialogExt(const nfd_OpenDialogExt *args, nfdchar_t **outPath);
nfdresult_t NFD_SaveDialogExt(const nfd_SaveDialogExt *args, nfdchar_t **outPath);

#ifdef __cplusplus
}
#endif

#endif
