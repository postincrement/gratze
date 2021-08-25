#ifndef CONFIG_H_
#define CONFIG_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef __MINGW32__

#define O_BINARY      	0
#define DIR_SEPERATOR	'/'

#else

#define DIR_SEPERATOR	'\\'

#endif

#endif // CONFIG_H_
