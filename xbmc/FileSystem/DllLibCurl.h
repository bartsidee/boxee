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
namespace XCURL
{
  #define CURL CURL_HANDLE
  #if (defined(_LINUX) && !defined(__APPLE__)) || defined(HAS_EMBEDDED)
  #include <curl/curl.h>
  #else
  #include "curl/curl.h"
  #endif
  #undef CURL

  class DllLibCurlInterface
  {
  public:
    virtual ~DllLibCurlInterface() {}
    virtual CURLcode global_init(long flags)=0;
    virtual void global_cleanup(void)=0;
    virtual CURL_HANDLE * easy_init(void)=0;
    virtual CURLcode easy_setopt(CURL_HANDLE *handle, CURLoption option, int64_t num)=0;
    virtual CURLcode easy_setopt(CURL_HANDLE *handle, CURLoption option, int num)=0;
    virtual CURLcode easy_setopt(CURL_HANDLE *handle, CURLoption option, void *param)=0;
    virtual CURLcode easy_setopt(CURL_HANDLE *handle, CURLoption option, const char *param)=0;
	virtual CURLcode easy_perform(CURL_HANDLE * handle )=0;
    virtual void easy_reset(CURL_HANDLE * handle)=0;
    virtual CURLcode easy_getinfo(CURL_HANDLE *curl, CURLINFO info, double *dParam )=0;
    virtual CURLcode easy_getinfo(CURL_HANDLE *curl, CURLINFO info, long *nParam )=0;
    virtual CURLcode easy_getinfo(CURL_HANDLE *curl, CURLINFO info, char *param )=0;
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
  };

  class DllLibCurl : public DllLibCurlInterface
  {
  public:
    virtual ~DllLibCurl() {}
    virtual CURLcode global_init(long flags) { return curl_global_init(flags); }
    virtual void global_cleanup(void) {  curl_global_cleanup();}
    virtual CURL_HANDLE * easy_init(void) { return curl_easy_init(); }
    virtual CURLcode easy_perform(CURL_HANDLE * handle ) { return curl_easy_perform(handle); }
    virtual void easy_reset(CURL_HANDLE * handle) { curl_easy_reset(handle); }
    virtual CURLcode easy_getinfo(CURL_HANDLE *curl, CURLINFO info, double *dParam ) { return curl_easy_getinfo(curl, info, dParam); }
    virtual CURLcode easy_getinfo(CURL_HANDLE *curl, CURLINFO info, long *nParam ) { return curl_easy_getinfo(curl, info, nParam); }
    virtual CURLcode easy_getinfo(CURL_HANDLE *curl, CURLINFO info, char *param ) { return curl_easy_getinfo(curl, info, param); }
    virtual void easy_cleanup(CURL_HANDLE * handle ) { curl_easy_cleanup(handle); }
    virtual CURL_HANDLE *easy_duphandle(CURL_HANDLE *handle ) { return curl_easy_duphandle(handle); }
    virtual CURLM * multi_init(void) { return curl_multi_init(); }
    virtual CURLMcode multi_add_handle(CURLM *multi_handle, CURL_HANDLE *easy_handle) { return curl_multi_add_handle(multi_handle, easy_handle); }
    virtual CURLcode easy_setopt(CURL_HANDLE *handle, CURLoption option, int64_t num) { return curl_easy_setopt(handle, option, num); }
    virtual CURLcode easy_setopt(CURL_HANDLE *handle, CURLoption option, int num) { return curl_easy_setopt(handle, option, num); }
    virtual CURLcode easy_setopt(CURL_HANDLE *handle, CURLoption option, void *param) { return curl_easy_setopt(handle, option, param); }
    virtual CURLcode easy_setopt(CURL_HANDLE *handle, CURLoption option, const char *param) { return curl_easy_setopt(handle, option, param); }
    virtual CURLMcode multi_perform(CURLM *multi_handle, int *running_handles) { return curl_multi_perform(multi_handle, running_handles); }
    virtual CURLMcode multi_remove_handle(CURLM *multi_handle, CURL_HANDLE *easy_handle) { return curl_multi_remove_handle(multi_handle, easy_handle); }
    virtual CURLMcode multi_fdset(CURLM *multi_handle, fd_set *read_fd_set, fd_set *write_fd_set, fd_set *exc_fd_set, int *max_fd) { return curl_multi_fdset(multi_handle, read_fd_set, write_fd_set, exc_fd_set, max_fd); }
    virtual CURLMcode multi_timeout(CURLM *multi_handle, long *timeout) { return curl_multi_timeout(multi_handle, timeout); }
    virtual CURLMsg*  multi_info_read(CURLM *multi_handle, int *msgs_in_queue) { return curl_multi_info_read(multi_handle, msgs_in_queue); }
    virtual void multi_cleanup(CURL_HANDLE * handle ) {  curl_multi_cleanup(handle); }
    virtual struct curl_slist* slist_append(struct curl_slist *list, const char *a) { return curl_slist_append(list, a); }
    virtual void  slist_free_all(struct curl_slist *a) { curl_slist_free_all(a); }
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
}

extern XCURL::DllLibCurlGlobal g_curlInterface;
