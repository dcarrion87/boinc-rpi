/* jconfig.vc --- jconfig.h for Microsoft Visual C++ on Windows 95 or NT. */
/* see jconfig.doc for explanations */

#ifndef _WIN32
#include "sah_config.h"
#else
#include "win-sah_config.h"
#endif

#define HAVE_PROTOTYPES
#define HAVE_UNSIGNED_CHAR
#define HAVE_UNSIGNED_SHORT
/* #define void char */
/* #define const */
#undef CHAR_IS_UNSIGNED
/* #define HAVE_STDDEF_H */
/* #define HAVE_STDLIB_H */
#undef NEED_BSD_STRINGS
#ifdef HAVE_SYS_TYPES_H
#define NEED_SYS_TYPES_H
#else
#undef NEED_SYS_TYPES_H
#endif
#undef NEED_FAR_POINTERS	/* we presume a 32-bit flat memory model */
#undef NEED_SHORT_EXTERNAL_NAMES
#undef INCOMPLETE_TYPES_BROKEN

/* Define "boolean" as unsigned char, not int, per Windows custom */
#ifndef __RPCNDR_H__		/* don't conflict if rpcndr.h already read */
typedef unsigned char boolean;
#elif defined(__MINGW32__)       /* no conflict in MinGW rpcndr.h */
typedef unsigned char boolean;
#endif

#define HAVE_BOOLEAN		/* prevent jmorecfg.h from redefining it */


#ifdef JPEG_INTERNALS

#undef RIGHT_SHIFT_IS_UNSIGNED

#endif /* JPEG_INTERNALS */

#ifdef JPEG_CJPEG_DJPEG

#define BMP_SUPPORTED		/* BMP image file format */
#define GIF_SUPPORTED		/* GIF image file format */
#define PPM_SUPPORTED		/* PBMPLUS PPM/PGM image file format */
#undef RLE_SUPPORTED		/* Utah RLE image file format */
#define TARGA_SUPPORTED		/* Targa image file format */

#define TWO_FILE_COMMANDLINE	/* optional */
#ifdef _WIN32
#define USE_SETMODE		/* Microsoft has setmode() */
#endif
#undef NEED_SIGNAL_CATCHER
#undef DONT_USE_B_MODE
#undef PROGRESS_REPORT		/* optional */

#endif /* JPEG_CJPEG_DJPEG */
