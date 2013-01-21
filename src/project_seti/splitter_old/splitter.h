/*
 *  splitter.h 
 *
 *  Global definitions from the splitter main program.  
 *
 * $Id: splitter.h,v 1.1.2.1 2006/01/13 00:25:01 jeffc Exp $
 *
 */

#ifndef SPLITTER_H
#define SPLITTER_H

#include "splitparms.h"
#include "splittypes.h"

#define MAX(x,y)  ( ((x)<(y)) ? (y) : (x)  )
#define MIN(x,y)  ( ((y)<(x)) ? (y) : (x)  )

/*  wulog: File for logging names of completed wu files */
/*  errorlog: File for logging errors                   */
#include "boinc_db.h"
#include "crypt.h"
#include "backend_lib.h"
#include "sched_config.h"

extern SCHED_CONFIG boinc_config;
extern DB_APP app;
extern R_RSA_PRIVATE_KEY key;
extern FILE *wulog,*errorlog;
extern workunit wuheaders[NSTRIPS];
extern tapeheader_t tapeheaders[TAPE_FRAMES_IN_BUFFER];
extern int noencode;
extern int dataclass;
extern int nodb;
extern int resumetape;
extern int startblock;
extern int norewind;
extern int output_xml;
extern int polyphase;
extern int wu_database_id[NSTRIPS];
extern int gregorian;
extern char * projectdir;

/* Buffer that holds tape data */
extern unsigned char *tapebuffer;

/* Number of records remaining in the buffer after WU creations is complete */
extern int records_in_buffer;

extern const char *wu_template;
extern const char *result_template;
// jeffc
//extern const char *result_template_filename;
extern char result_template_filename[];
extern char result_template_filepath[];
extern char wu_template_filename[];

/* Persistant sequence number of wu file */
extern int seqno;

#endif

/* 
 * $Log: splitter.h,v $
 * Revision 1.1.2.1  2006/01/13 00:25:01  jeffc
 * *** empty log message ***
 *
 * Revision 1.6  2004/07/09 22:35:39  jeffc
 * *** empty log message ***
 *
 * Revision 1.5  2004/06/16 20:57:19  jeffc
 * *** empty log message ***
 *
 * Revision 1.4  2003/09/26 20:48:52  jeffc
 * jeffc - merge in branch setiathome-4_all_platforms_beta.
 *
 * Revision 1.2.2.1  2003/09/22 17:39:35  korpela
 * *** empty log message ***
 *
 * Revision 1.3  2003/09/22 17:05:38  korpela
 * *** empty log message ***
 *
 * Revision 1.2  2003/08/05 17:23:42  korpela
 * More work on database stuff.
 * Further tweaks.
 *
 * Revision 1.1  2003/06/03 00:23:40  korpela
 *
 * Again
 *
 * Revision 3.2  2003/05/19 17:40:59  eheien
 * *** empty log message ***
 *
 * Revision 3.1  2001/08/16 23:42:08  korpela
 * Mods for splitter to make binary workunits.
 *
 * Revision 3.0  2001/08/01 19:04:57  korpela
 * Check this in before Paul screws it up.
 *
 * Revision 2.5  1999/10/20 19:20:26  korpela
 * *** empty log message ***
 *
 * Revision 2.4  1999/03/05 01:47:18  korpela
 * Added data_class field.
 *
 * Revision 2.3  1999/02/22  22:21:09  korpela
 * Added nodb option
 *
 * Revision 2.2  1999/02/11  16:46:28  korpela
 * Added startblock and norewind support, and wu_database_id.
 *
 * Revision 2.1  1998/11/02 16:41:21  korpela
 * Minor Change.
 *
 * Revision 2.0  1998/10/30  22:00:04  korpela
 * Conversion to C++ and merger with client source tree.
 *
 * Revision 1.1  1998/10/27  01:10:32  korpela
 * Initial revision
 *
 *
 */
