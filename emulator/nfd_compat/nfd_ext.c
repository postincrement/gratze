#include "nfd.h"

nfdresult_t NFD_OpenDialogExt(const nfd_OpenDialogExt *args, nfdchar_t **outPath)
{
  const nfdchar_t *filter = NULL;
  const nfdchar_t *path = NULL;

  if (args) {
    filter = args->filterList;
    path = args->defaultPath;
  }

  return NFD_OpenDialog(filter, path, outPath);
}

nfdresult_t NFD_SaveDialogExt(const nfd_SaveDialogExt *args, nfdchar_t **outPath)
{
  const nfdchar_t *filter = NULL;
  const nfdchar_t *path = NULL;

  if (args) {
    filter = args->filterList;
    path = args->defaultFilename ? args->defaultFilename : args->defaultPath;
  }

  return NFD_SaveDialog(filter, path, outPath);
}
