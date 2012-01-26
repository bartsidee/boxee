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
#include "system.h"

#ifdef HAS_OPENMAX

#include "DVDOmxUtils.h"
#include "utils/log.h"

#include <OMX_Core.h>
#include <OMX_Types.h>
#include <OMX_Component.h>

static const char *pszOmxComponentsName[] =
{
    OMX_DECODER_COMPONENT_NAME_LIST
};

CSemaphore::CSemaphore(int sCountMax, int sCountInitial)
{
    m_pMutex = new CCriticalSection;
    m_pEvent = new CEvent;
    m_sCount = sCountInitial;
    m_sCountMax = sCountMax;
}

CSemaphore::~CSemaphore()
{
    delete m_pMutex;
    delete m_pEvent;
}

bool CSemaphore::Increment()
{
    EnterCriticalSection(m_pMutex);
    m_sCount++;
    if(m_sCount > m_sCountMax) {
        m_sCount = m_sCountMax;
    } else {
        m_pEvent->Set();
    }
    LeaveCriticalSection(m_pMutex);

    return true;
}

bool CSemaphore::Decrement(int sTimeoutMs)
{
    while (1) {
        EnterCriticalSection(m_pMutex);
        if(m_sCount > 0) {
           m_sCount--;
           LeaveCriticalSection(m_pMutex);
           break;
        }
        LeaveCriticalSection(m_pMutex);

        if(sTimeoutMs == 0) {
            return false;
        } else if(sTimeoutMs == -1) {
            m_pEvent->Wait();
        } else {
            bool bResult = m_pEvent->WaitMSec(sTimeoutMs);
            if(!bResult) {
                return false;
            }
        }
    }

    return true;
}

CQueue::CQueue(int sQueueSize, int sElemSize)
{
    m_sNextPut   = 0;
    m_sNextGet   = 0;
    m_sQueueSize = sQueueSize;
    m_sItemSize  = sElemSize;
    m_sItems     = 0;
    m_pQueueData = new unsigned char [sQueueSize * sElemSize];
    m_pSemGet = new CSemaphore(sQueueSize, 0);
    m_pSemPut = new CSemaphore(sQueueSize, sQueueSize);
    m_pMutex  = new CCriticalSection;
    m_pGotElementEvent = new CEvent;
}

CQueue::~CQueue()
{
    delete m_pSemGet;
    delete m_pSemPut;
    delete m_pMutex;
    delete m_pGotElementEvent;
    delete [] m_pQueueData;
}

bool CQueue::Get(void *pElement, int sTimeoutMs)
{
    if(m_pSemGet->Decrement(sTimeoutMs)) {
        EnterCriticalSection(m_pMutex);
        m_sNextGet = (m_sNextGet + 1) % m_sQueueSize;
        m_sItems--;
        if(!m_sItems) {
            m_pGotElementEvent->Reset();
        }
        memcpy(pElement, m_pQueueData + (m_sNextGet * m_sItemSize), m_sItemSize);
        LeaveCriticalSection(m_pMutex);
        m_pSemPut->Increment();
        return true;
    }
    return false;
}

bool CQueue::Put(void *pElement, int sTimeoutMs)
{
    if(m_pSemPut->Decrement(sTimeoutMs)) {
        EnterCriticalSection(m_pMutex);
        m_sNextPut = (m_sNextPut + 1) % m_sQueueSize;
        memcpy(m_pQueueData + (m_sNextPut * m_sItemSize), pElement, m_sItemSize);
        m_pSemGet->Increment();
        m_sItems++;
        m_pGotElementEvent->Set();
        LeaveCriticalSection(m_pMutex);
        return true;
    }
    return false;
}

int CQueue::GetSize()
{
    EnterCriticalSection(m_pMutex);
    int size = m_sItems;
    LeaveCriticalSection(m_pMutex);
    return size;
}

bool CQueue::Peek(void *pElement, int *psElements)
{
    EnterCriticalSection(m_pMutex);
    *psElements = m_sItems;
    if(m_sItems) {
        int sNextGet = (m_sNextGet + 1) % m_sQueueSize;
        memcpy(pElement, m_pQueueData + (sNextGet * m_sItemSize), m_sItemSize);
    }
    LeaveCriticalSection(m_pMutex);
    return true;
}

bool CQueue::WaitForNotEmpty(int sTimeoutMs)
{
    if(!GetSize()) {
        if(sTimeoutMs == -1) {
            m_pGotElementEvent->Wait();
        } else {
            bool bResult = m_pGotElementEvent->WaitMSec(sTimeoutMs);
            if(!bResult) {
                return false;
            }
        }
    }
    return true;
}

int g_iOmxInitRefcount = 0;

bool COmxUtils::OmxInit()
{
    if(!g_iOmxInitRefcount) {
        OMX_ERRORTYPE err  = OMX_Init();
        if(err != OMX_ErrorNone) {
            CLog::Log(LOGDEBUG, "OMX_Init() failed = %X\n", err);
            return false;
        }
    }
    g_iOmxInitRefcount++;
    return true;
}

bool COmxUtils::OmxDeinit()
{
    if(g_iOmxInitRefcount == 1) {
        OMX_ERRORTYPE err  = OMX_Deinit();
        if(err != OMX_ErrorNone) {
            CLog::Log(LOGDEBUG, "OMX_Deinit() failed = %X\n", err);
            return false;
        }
    }
    if(g_iOmxInitRefcount > 0) {
      g_iOmxInitRefcount--;
    }
    return true;
}

const char *COmxUtils::OmxGetComponentName(EOmxDecoderTypes eType)
{
    return pszOmxComponentsName[eType];
}

#endif
