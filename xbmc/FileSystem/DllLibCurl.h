#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "DynamicDll.h"
#include "utils/CriticalSection.h"

/* put types of curl in namespace to avoid namespace pollution */
  #define CURL CURL_HANDLE
  #if (defined(_LINUX) && !defined(__APPLE__)) || defined(HAS_EMBEDDED)
  #include <curl/curl.h>
  #else
  #include "curl/curl.h"
  #endif
  #undef CURL
  
#ifdef A_WIN32
#define CURL_LIB_BASE_CLASS DllDynamic
#define CURL_RESOLVE_METHOD_RENAME RESOLVE_METHOD_RENAME
#define CURL_RESOLVE_METHOD_RENAME_FP RESOLVE_METHOD_RENAME_FP
#define CURL_DECLARE_LIB_WRAPPER() DECLARE_DLL_WRAPPER(DllLibCurl, DLL_PATH_LIBCURL)
#else
#define CURL_LIB_BASE_CLASS LibStatic
#define CURL_RESOLVE_METHOD_RENAME STATIC_RESOLVE_METHOD_RENAME
#define CURL_RESOLVE_METHOD_RENAME_FP STATIC_RESOLVE_METHOD_RENAME_FP
#define CURL_DECLARE_LIB_WRAPPER() DECLARE_LIB_WRAPPER(DllLibCurl)
#endif

    class DllLibCurlInterface
  {
  public:
    virtual ~DllLibCurlInterface() {}
    virtual CURLcode global_init(long flags)=0;
    virtual void global_cleanup(void)=0;
    virtual CURL_HANDLE * easy_init(void)=0;
    //virtual CURLcode easy_setopt(CURL_HANDLE *handle, CURLoption option, ...)=0;
    virtual CURLcode easy_perform(CURL_HANDLE * handle )=0;
    virtual void easy_reset(CURL_HANDLE * handle)=0;
    //virtual CURLcode easy_getinfo(CURL_HANDLE *curl, CURLINFO info, ... )=0;
    virtual void easy_cleanup(CURL_HANDLE * handle )=0;
    virtual CURL_HANDLE *easy_duphandle(CURL_HANDLE *handle )=0;
    virtual CURLM * multi_init(void)=0;
    virtual CURLMcode multi_add_handle(CURLM *multi_handle, CURL_HANDLE *easy_handle)=0;
    virtual CURLMcode multi_perform(CURLM *multi_handle, int *running_handles)=0;
    virtual CURLMcode multi_remove_handle(CURLM *multi_handle, CURL_HANDLE *easy_handle)=0;
    virtual CURLMcode multi_fdset(CURLM *multi_handle, fd_set *read_fd_set, fd_set *write_fd_set, fd_set *exc_fd_set, int *max_fd)=0;
    virtual CURLMcode multi_timeout(CURLM *multi_handle, long *timeout)=0;
    virtual CURLMsg*  multi_info_read(CURLM *multi_handle, int *msgs_in_queue)=0;
    virtual void multi_cleanup(CURL_HANDLE * handle )=0;
    virtual struct curl_slist* slist_append(struct curl_slist *, const char *)=0;
    virtual void  slist_free_all(struct curl_slist *)=0;
	virtual time_t getdate(const char *p, const time_t *unused)=0;
	virtual char *escape(const char *string, int length)=0;
	virtual void free_curl(void *p)=0;
	virtual char *easy_unescape(CURL_HANDLE *handle, const char *string, int length, int *outlength)=0;
	virtual char *easy_escape(CURL_HANDLE *handle, const char *string, int length)=0;
  };


  class DllLibCurl : public CURL_LIB_BASE_CLASS, DllLibCurlInterface
  {
	CURL_DECLARE_LIB_WRAPPER()
    DEFINE_METHOD1(CURLcode, global_init, (long p1))
    DEFINE_METHOD0(void, global_cleanup)
    DEFINE_METHOD0(CURL_HANDLE *, easy_init)
    DEFINE_METHOD_FP(CURLcode, easy_setopt, (CURL_HANDLE *p1, CURLoption p2, ...))
    DEFINE_METHOD1(CURLcode, easy_perform, (CURL_HANDLE * p1 ))
    DEFINE_METHOD1(void, easy_reset, (CURL_HANDLE * p1 ))
    DEFINE_METHOD_FP(CURLcode, easy_getinfo, (CURL_HANDLE *p1, CURLINFO p2, ... ))
    DEFINE_METHOD1(void, easy_cleanup, (CURL_HANDLE * p1))
    DEFINE_METHOD1(CURL_HANDLE *, easy_duphandle, (CURL_HANDLE * p1))
    DEFINE_METHOD0(CURLM *, multi_init)
    DEFINE_METHOD2(CURLMcode, multi_add_handle, (CURLM *p1, CURL_HANDLE *p2))
    DEFINE_METHOD2(CURLMcode, multi_perform, (CURLM *p1, int *p2))
    DEFINE_METHOD2(CURLMcode, multi_remove_handle, (CURLM *p1, CURL_HANDLE *p2))
    DEFINE_METHOD5(CURLMcode, multi_fdset, (CURLM *p1, fd_set *p2, fd_set *p3, fd_set *p4, int *p5))
    DEFINE_METHOD2(CURLMcode, multi_timeout, (CURLM *p1, long *p2))
    DEFINE_METHOD2(CURLMsg*,  multi_info_read, (CURLM *p1, int *p2))
    DEFINE_METHOD1(void, multi_cleanup, (CURLM *p1))
    DEFINE_METHOD2(struct curl_slist*, slist_append, (struct curl_slist * p1, const char * p2))
    DEFINE_METHOD1(void, slist_free_all, (struct curl_slist * p1))
	DEFINE_METHOD2(time_t, getdate, (const char *p1, const time_t *p2))
	DEFINE_METHOD2(char *, escape, (const char *p1, int p2))
	DEFINE_METHOD1(void, free_curl, (void *p1))
	DEFINE_METHOD4(char *, easy_unescape, (CURL_HANDLE *p1, const char *p2, int p3, int *p4))
	DEFINE_METHOD3(char *, easy_escape, (CURL_HANDLE *p1, const char *p2, int p3))
    BEGIN_METHOD_RESOLVE()
      CURL_RESOLVE_METHOD_RENAME(curl_global_init, global_init)
      CURL_RESOLVE_METHOD_RENAME(curl_global_cleanup, global_cleanup)
      CURL_RESOLVE_METHOD_RENAME(curl_easy_init, easy_init)
      CURL_RESOLVE_METHOD_RENAME_FP(curl_easy_setopt, easy_setopt)
      CURL_RESOLVE_METHOD_RENAME(curl_easy_perform, easy_perform)
      CURL_RESOLVE_METHOD_RENAME(curl_easy_reset, easy_reset)
      CURL_RESOLVE_METHOD_RENAME_FP(curl_easy_getinfo, easy_getinfo)
      CURL_RESOLVE_METHOD_RENAME(curl_easy_cleanup, easy_cleanup)
      CURL_RESOLVE_METHOD_RENAME(curl_easy_duphandle, easy_duphandle)
      CURL_RESOLVE_METHOD_RENAME(curl_multi_init, multi_init)
      CURL_RESOLVE_METHOD_RENAME(curl_multi_add_handle, multi_add_handle)
      CURL_RESOLVE_METHOD_RENAME(curl_multi_perform, multi_perform)
      CURL_RESOLVE_METHOD_RENAME(curl_multi_remove_handle, multi_remove_handle)
      CURL_RESOLVE_METHOD_RENAME(curl_multi_fdset, multi_fdset)
      CURL_RESOLVE_METHOD_RENAME(curl_multi_timeout, multi_timeout)
      CURL_RESOLVE_METHOD_RENAME(curl_multi_info_read, multi_info_read)
      CURL_RESOLVE_METHOD_RENAME(curl_multi_cleanup, multi_cleanup)
      CURL_RESOLVE_METHOD_RENAME(curl_slist_append, slist_append)
      CURL_RESOLVE_METHOD_RENAME(curl_slist_free_all, slist_free_all)
	  CURL_RESOLVE_METHOD_RENAME(curl_getdate, getdate)
	  CURL_RESOLVE_METHOD_RENAME(curl_escape, escape)
	  CURL_RESOLVE_METHOD_RENAME(curl_free, free_curl)	// Can't be free, to prevent name clashing when MEMORY_LEAK_DETECT is defined.
	  CURL_RESOLVE_METHOD_RENAME(curl_easy_unescape, easy_unescape)
	  CURL_RESOLVE_METHOD_RENAME(curl_easy_escape, easy_escape)
    END_METHOD_RESOLVE()

  };


  class DllLibCurlGlobal : public DllLibCurl
  {
  public:
    /* extend interface with buffered functions */
    void easy_aquire(const char *protocol, const char *hostname, CURL_HANDLE** easy_handle, CURLM** multi_handle);
    void easy_release(CURL_HANDLE** easy_handle, CURLM** multi_handle);
    void easy_duplicate(CURL_HANDLE* easy, CURLM* multi, CURL_HANDLE** easy_out, CURLM** multi_out);
    CURL_HANDLE* easy_duphandle(CURL_HANDLE* easy_handle);
    void CheckIdle();

    /* overloaded load and unload with reference counter */
    virtual bool Load();
    virtual void Unload();

    /* structure holding a session info */
    typedef struct SSession
    {
      unsigned int  m_idletimestamp;  // timestamp of when this object when idle
      CStdString    m_protocol;
      CStdString    m_hostname;
      bool          m_busy;
      CURL_HANDLE*  m_easy;
      CURLM*        m_multi;
    } SSession;

    typedef std::vector<SSession> VEC_CURLSESSIONS;
    
    VEC_CURLSESSIONS m_sessions;  
    CCriticalSection m_critSection;
  };

extern DllLibCurlGlobal g_curlInterface;
