#ifndef VERSIONINFO_H_
#define VERSIONINFO_H_

#include "../lib/libBoxee/bxversion.h"

#ifdef _WIN32
//#include "svn_rev.h"
#include "../linux/svn_rev.h"
#endif

#ifdef _LINUX 
#include "xbmc/linux/svn_rev.h"
#endif

#ifdef SVN_REV
#define VERSION_STRING BOXEE_VERSION"."SVN_REV
#else
#define VERSION_STRING BOXEE_VERSION
#endif

#endif /*VERSIONINFO_H_*/

