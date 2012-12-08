/* Platform independent version definitions... */

#ifndef BOINC_VERSION_H
#define BOINC_VERSION_H

/* Major part of BOINC version number */
#define BOINC_MAJOR_VERSION 6

/* Minor part of BOINC version number */
#define BOINC_MINOR_VERSION 12

/* Release part of BOINC version number */
#define BOINC_RELEASE 34

/* String representation of BOINC version number */
#define BOINC_VERSION_STRING "6.12.34"

/* Package is a pre-release (Alpha/Beta) package */
/* #define BOINC_PRERELEASE 1 */

#if (defined(_WIN32) || defined(__APPLE__))
/* Name of package */
#define PACKAGE "boinc"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "BOINC"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "BOINC 6.12.34"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "boinc"

/* Define to the version of this package. */
#define PACKAGE_VERSION "6.12.34"

#endif /* #if (defined(_WIN32) || defined(__APPLE__)) */

#endif /* #ifndef BOINC_VERSION_H */

