#ifndef __SHAIRPORT_H__
#define __SHAIRPORT_H__

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "socketlib.h"
#include <regex.h>
#include <sys/types.h>
#include <regex.h>


#define HWID_SIZE 6
#define SHAIRPORT_LOG 1
#define LOG_INFO     1
#define LOG_DEBUG    5
#define LOG_DEBUG_V  6
#define LOG_DEBUG_VV 7

struct shairbuffer
{
  char *data;
  int   current;
  int   maxsize;
  int   marker;
};

struct keyring
{
  char *aeskey;
  char *aesiv;
  char *fmt;
};

struct comms
{
  int  in[2];
  int  out[2];
};

struct connection
{
  struct shairbuffer  recv;
  struct shairbuffer  resp;
  struct keyring      *keys; // Does not point to malloc'd memory.
#ifndef BOXEE
  struct comms        *hairtunes;
#endif
  int                 clientSocket;
  char                *password;
};

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

int shairport_main(int argc, char **argv);
void shairport_exit(void);
int shairport_loop(void);
int shairport_is_running(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
