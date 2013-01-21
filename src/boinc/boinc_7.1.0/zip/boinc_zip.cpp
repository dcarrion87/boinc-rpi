// This file is part of BOINC.
// http://boinc.berkeley.edu
// Copyright (C) 2012 University of California
//
// BOINC is free software; you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// BOINC is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with BOINC.  If not, see <http://www.gnu.org/licenses/>.


#if defined(_WIN32) && !defined(__STDWX_H__)
#pragma warning( disable : 4786 )  // Disable warning messages for vector
#include "boinc_win.h"
#elif defined(_WIN32) && defined(__STDWX_H__)
#include "stdwx.h"
#else
#ifndef __APPLE_CC__
#include "config.h"
#endif
#include <algorithm>
#include <string>
#include <string.h>
using std::string;

#endif

#ifdef __cplusplus
extern "C" {
#else
extern {
#endif
int unzip(int argc, char** argv);
//int zipmain(int argc, char** argv);
#include "./unzip/unzip.h"
#include "./zip/zip.h"
}

#include "boinc_zip.h"
#include "filesys.h" // from BOINC for DirScan

// send in an output filename, advanced options (usually NULL), and numFileIn, szfileIn

#ifndef _MAX_PATH
#define _MAX_PATH 255
#endif

unsigned char g_ucSort;

// a "binary predicate" for use by the std::sort algorithm
// return true if "first > second" according to the g_ucSort type

bool StringVectorSort(const std::string& first, const std::string& second) {
    bool bRet = false;
    if (g_ucSort & SORT_NAME 
        && g_ucSort & SORT_ASCENDING
        && strcmp(first.c_str(), second.c_str())<0
    ) {
        bRet = true;
    } else if (g_ucSort & SORT_NAME 
        && g_ucSort & SORT_DESCENDING
        && strcmp(first.c_str(), second.c_str())>0
    ) {
        bRet = true;
    } else if (g_ucSort & SORT_TIME) {
        struct stat st[2];
        stat(first.c_str(), &st[0]);
        stat(second.c_str(), &st[1]);
        if (g_ucSort & SORT_ASCENDING) {
            bRet = st[0].st_mtime < st[1].st_mtime;
        } else {
            bRet = st[0].st_mtime > st[1].st_mtime;
        }
    }
    return bRet;
}

int boinc_zip(
    int bZipType, const std::string szFileZip, const std::string szFileIn
) {
    ZipFileList tempvec;
    tempvec.push_back(szFileIn);
    return boinc_zip(bZipType, szFileZip, &tempvec);
}

// same, but with char[] instead of string
//
int boinc_zip(int bZipType, const char* szFileZip, const char* szFileIn) {
    string strFileZip, strFileIn;
    strFileZip.assign(szFileZip);
    strFileIn.assign(szFileIn);
    ZipFileList tempvec;
    tempvec.push_back(strFileIn);
    return boinc_zip(bZipType, strFileZip, &tempvec);
}

int boinc_zip(
    int bZipType, const std::string szFileZip, const ZipFileList* pvectszFileIn
) {
    int carg;
    char** av;
    int iRet = 0, i = 0, nVecSize = 0;

    if (pvectszFileIn) nVecSize = pvectszFileIn->size();

    // if unzipping but no file out, so it just uses cwd, only 3 args
    //if (bZipType == UNZIP_IT)
    //      carg = 3 + nVecSize;
    //else
    carg = 3 + nVecSize;

    // make a dynamic array
    av = (char**) calloc(carg+1, sizeof(char*));
    for (i=0;i<(carg+1);i++) {
        av[i] = (char*) calloc(_MAX_PATH,sizeof(char));
    }

    // just form an argc/argv to spoof the "main"
    // default options are to recurse into directories
    //if (options && strlen(options)) 
    //      strcpy(av[1], options);

    if (bZipType == ZIP_IT) {
        strcpy(av[0], "zip");
        // default zip options -- no dir names, no subdirs, highest compression, quiet mode
        if (strlen(av[1])==0) {
            strcpy(av[1], "-j9q");
        }
        strcpy(av[2], szFileZip.c_str());

        //sz 3 onward will be each vector
        int jj;
        for (jj=0; jj<nVecSize; jj++) {
            strcpy(av[3+jj], pvectszFileIn->at(jj).c_str());
        }
    } else {
        strcpy(av[0], "unzip");
        // default unzip options -- preserve subdirs, overwrite 
        if (strlen(av[1])==0) {
            strcpy(av[1], "-oq");
        }
        strcpy(av[2], szFileZip.c_str());

        // if they passed in a directory unzip there
        if (carg == 4) {
            sprintf(av[3], "-d%s", pvectszFileIn->at(0).c_str());
        }
    }
    av[carg] = NULL;
    // printf("args: %s %s %s %s\n", av[0], av[1], av[2], av[3]);

    if (bZipType == ZIP_IT) {
        if (access(szFileZip.c_str(), 0) == 0) {
            // old zip file exists so unlink
            // (otherwise zip will reuse, doesn't seem to be a flag to 
            // bypass zip reusing it
            unlink(szFileZip.c_str());   
        }
        iRet = zipmain(carg, av);
    } else {
        // make sure zip file exists
        if (access(szFileZip.c_str(), 0) == 0) {
            iRet = UzpMain(carg, av);
        } else {
            iRet = 2;   
        }
    }

    for (i=0;i<carg;i++) {
        free(av[i]);
    }
    free(av);
    return iRet;
}


// -------------------------------------------------------------------
//
// Description: Supply a directory and a pattern as arguments, along
//    with a FileList variable to hold the result list; sorted by name.
//    Returns a vector of files in the directory which match the
//    pattern.  Returns true for success, or false if there was a problem.
//
// CMC Note: this is a 'BOINC-friendly' implementation of "old" CPDN code
//    the "wildcard matching"/regexp is a bit crude, it matches substrings in
//    order in a file; to match all files such as *.pc.*.x4.*.nc" pass
//        ".pc|.x4.|.nc" for 'pattern'
//
// --------------------------------------------------------------------

bool boinc_filelist(
    const string directory,
    const string pattern,
    ZipFileList* pList,
    const unsigned char ucSort,
    const bool bClear
) {    
    g_ucSort = ucSort;  // set the global sort type right off the bat
    string strFile;
    // at most three |'s may be passed in pattern match
    int iPos[3], iFnd, iCtr, i, lastPos;  
    string strFullPath;
    char strPart[3][32];
    string spattern = pattern;
    string strDir = directory;
    string strUserDir = directory;
    int iLen = strUserDir.size();

    if (!pList) return false;

    // wildcards are blank!
    if (pattern == "*" || pattern == "*.*") spattern.assign("");

    if (bClear) {
        pList->clear();  // removes old entries that may be in pList
    }

    // first tack on a final slash on user dir if required
    if (strUserDir[iLen-1] != '\\' && strUserDir[iLen] != '/') {
        // need a final slash, but what type?
        // / is safe on all OS's for CPDN at least
        // but if they already used \ use that
        // well they didn't use a backslash so just use a slash
        if (strUserDir.find("\\") == string::npos) {
            strUserDir += "/";
        } else {
            strUserDir += "\\";
        }
    }

    // transform strDir to either all \\ or all /
    int j;
    for (j=0; j<(int)directory.size(); j++)  {
        // take off final / or backslash
        if (j == ((int)directory.size()-1) 
             && (strDir[j] == '/' || strDir[j]=='\\')
        ) {
            strDir.resize(directory.size()-1);
        } else {
#ifdef _WIN32  // transform paths appropriate for OS
           if (directory[j] == '/') strDir[j] = '\\';
#else
           if (directory[j] == '\\') strDir[j] = '/';
#endif
        }
    }

    DirScanner dirscan(strDir);
    memset(strPart, 0x00, 3*32);
    while (dirscan.scan(strFile)) {
        iCtr = 0;
        lastPos = 0;
        iPos[0] = -1;
        iPos[1] = -1;
        iPos[2] = -1;
        // match the filename against the regexp to see if it's a hit
        // first get all the |'s to get the pieces to verify
        //
        while (iCtr<3 && (iPos[iCtr] = (int) spattern.find('|', lastPos)) > -1) {
            if (iCtr==0)  {
                strncpy(strPart[0], spattern.c_str(), iPos[iCtr]);
            } else {
                strncpy(strPart[iCtr], spattern.c_str()+lastPos, iPos[iCtr]-lastPos);
            }
            lastPos = iPos[iCtr]+1;
            iCtr++;
        }
        if (iCtr>0) {
            // found a | so need to get the part from lastpos onward
            strncpy(strPart[iCtr], spattern.c_str()+lastPos, spattern.length() - lastPos);
        }

        // check no | were found at all
        if (iCtr == 0) {
            strcpy(strPart[0], spattern.c_str());
            iCtr++; // fake iCtr up 1 to get in the loop below
        }

        bool bFound = true;
        for (i = 0; i <= iCtr && bFound; i++) {
            if (i==0)  {
                iFnd = (int) strFile.find(strPart[0]);
                bFound = (bool) (iFnd > -1);
            } else {
                // search forward of the old part found
                iFnd = (int) strFile.find(strPart[i], iFnd+1);
                bFound = bFound && (bool) (iFnd > -1);
            }
        }

        if (bFound) {
            // this pattern matched the file, add to vector
            // NB: first get stat to make sure it really is a file
            strFullPath = strUserDir + strFile;
            // only add if the file really exists (i.e. not a directory)
            if (is_file(strFullPath.c_str())) {
                pList->push_back(strFullPath);
            }
        }
    }

    // sort by file creation time
    // sort if list is greather than 1
    if (pList->size()>1)  {
       sort(pList->begin(), pList->end(), StringVectorSort);  // may as well sort it?
    }
    return true;
}

// Read compressed file to memory.
//
int boinc_UnzipToMemory (char *zip, char *file, string &retstr) {
    UzpOpts opts; // options for UzpUnzipToMemory()
    UzpCB funcs;  // function pointers for UzpUnzipToMemory()
    UzpBuffer buf;
    int ret;

    memset(&opts, 0, sizeof(opts));
    memset(&funcs, 0, sizeof(funcs));

    funcs.structlen = sizeof(UzpCB);
    funcs.msgfn = (MsgFn *)printf;
    funcs.inputfn = (InputFn *)scanf;
    funcs.pausefn = (PauseFn *)(0x01);
    funcs.passwdfn = (PasswdFn *)(NULL);

    memset(&buf, 0, sizeof(buf));    // data-blocks needs to be empty
    ret = UzpUnzipToMemory(zip, file, &opts, &funcs, &buf);

    if (ret) {
        retstr =  (string) buf.strptr;
    }

    if (buf.strptr) free (buf.strptr);
  
    return ret;

}

const char *BOINC_RCSID_bdf38b2dfb = "$Id: boinc_zip.cpp 18195 2009-05-22 21:19:44Z davea $";

