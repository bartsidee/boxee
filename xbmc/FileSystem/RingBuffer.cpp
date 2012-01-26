

#include "RingBuffer.h"

CRingBuffer::CRingBuffer()
  {
    InitializeCriticalSection(&m_critSection);
    m_pBuf = NULL;
    //m_pTmpBuf = NULL;
    m_nBufSize = 0;
    m_iReadPtr = 0;
    m_iWritePtr = 0;
  }

CRingBuffer::~CRingBuffer()
  {
    Destroy();
    DeleteCriticalSection(&m_critSection);
  }

BOOL CRingBuffer::Create( int iBufSize )
  {
    BOOL bResult = FALSE;
    ::EnterCriticalSection(&m_critSection );
    if ( m_pBuf )
      delete [] m_pBuf;

    m_pBuf = NULL;

    m_pBuf = new char[ iBufSize ];
    if ( m_pBuf )
    {
      m_nBufSize = iBufSize;
      ZeroMemory( m_pBuf, m_nBufSize );

      bResult = TRUE;
    }
    m_iReadPtr = 0;
    m_iWritePtr = 0;
    ::LeaveCriticalSection(&m_critSection );
    return bResult;
  }

int CRingBuffer::Size()
  {
    return m_nBufSize;
  }

void CRingBuffer::Destroy()
  {
    ::EnterCriticalSection(&m_critSection );
    if ( m_pBuf )
      delete [] m_pBuf;

    m_pBuf = NULL;
    m_nBufSize = 0;
    m_iReadPtr = 0;
    m_iWritePtr = 0;
    ::LeaveCriticalSection(&m_critSection );
  }

void CRingBuffer::Clear()
  {
    ::EnterCriticalSection(&m_critSection );
    m_iReadPtr = 0;
    m_iWritePtr = 0;
    ::LeaveCriticalSection(&m_critSection );
  }

void CRingBuffer::Lock()
  {
    ::EnterCriticalSection(&m_critSection );
  }

void CRingBuffer::Unlock()
  {
    ::LeaveCriticalSection(&m_critSection );
  }

BOOL CRingBuffer::Append(const CRingBuffer &buf)
  {
	::EnterCriticalSection(&m_critSection);
	
    if (m_pBuf == NULL)
      Create(buf.GetMaxReadSize() + 1);

    if (buf.GetMaxReadSize() > GetMaxWriteSize())
    {
      ::LeaveCriticalSection(&m_critSection);
      return FALSE;
    }

    int iReadPtr = buf.m_iReadPtr;
    if (iReadPtr < buf.m_iWritePtr)
      WriteBinary(&buf.m_pBuf[iReadPtr], buf.m_iWritePtr - iReadPtr);
    else if (iReadPtr > buf.m_iWritePtr)
    {
      WriteBinary(&buf.m_pBuf[iReadPtr], buf.m_nBufSize - iReadPtr);
      if (buf.m_iWritePtr > 0)
        WriteBinary(&buf.m_pBuf[0], buf.m_iWritePtr);
    }
    
    ::LeaveCriticalSection(&m_critSection);
    return TRUE;
  }

BOOL CRingBuffer::ReadBinary( CRingBuffer &buf, int nBufLen )
  {
    if (buf.m_pBuf == NULL)
      buf.Create(nBufLen + 1);

    ::EnterCriticalSection(&m_critSection );
    BOOL bResult = FALSE;
    if ( nBufLen <= GetMaxReadSize() )
    {
      if ( m_iReadPtr + nBufLen <= m_nBufSize )
      {
        buf.WriteBinary( &m_pBuf[m_iReadPtr], nBufLen );
        m_iReadPtr += nBufLen;
      }
      else // harder case, buffer wraps
      {
        int iFirstChunkSize = m_nBufSize - m_iReadPtr;
        int iSecondChunkSize = nBufLen - iFirstChunkSize;

        buf.WriteBinary( &m_pBuf[m_iReadPtr], iFirstChunkSize );
        buf.WriteBinary( &m_pBuf[0], iSecondChunkSize );

        m_iReadPtr = iSecondChunkSize;
      }
      bResult = TRUE;
    }
    else 
    {
      CLog::Log(LOGWARNING,"%s, buffer underflow! max size: %d. trying to read: %d", __FUNCTION__, GetMaxReadSize(), nBufLen);
    }
    ::LeaveCriticalSection(&m_critSection );
    return bResult;
  }

BOOL CRingBuffer::Copy(const CRingBuffer &buf)
  {
    Clear();
    return Append(buf);
  }

int CRingBuffer::GetMaxReadSize() const
  {
    int iBytes = 0;
    ::EnterCriticalSection(&m_critSection );
    if ( m_pBuf )
    {
      if ( m_iReadPtr <= m_iWritePtr )
        iBytes = m_iWritePtr - m_iReadPtr;

      else if ( m_iReadPtr > m_iWritePtr )
        iBytes = (m_nBufSize - m_iReadPtr) + m_iWritePtr;
    }
    ::LeaveCriticalSection(&m_critSection );
    return iBytes;
  }

int CRingBuffer::GetMaxWriteSize() const
  {
    int iBytes = 0;
    ::EnterCriticalSection(&m_critSection );
    if ( m_pBuf )
    {
      // not all the buffer is for our use. 1 bytes has to be kept as EOF marker. 
      // otherwise we cant tell if (m_iReadPtr == m_iWritePtr) is full or empty
      if ( m_iWritePtr < m_iReadPtr )
        iBytes = m_iReadPtr - m_iWritePtr - 1;

      if ( m_iWritePtr >= m_iReadPtr )
        iBytes = (m_nBufSize - m_iWritePtr) + m_iReadPtr - 1;
    }
    ::LeaveCriticalSection(&m_critSection );
    return iBytes;
  }

BOOL CRingBuffer::WriteBinary(const char * pBuf, int nBufLen )
  {
    ::EnterCriticalSection(&m_critSection );
    BOOL bResult = FALSE;
    {
      if ( nBufLen <= GetMaxWriteSize() )
      {
        // easy case, no wrapping
        if ( m_iWritePtr + nBufLen < m_nBufSize )
        {
          CopyMemory( &m_pBuf[m_iWritePtr], pBuf, nBufLen );
          m_iWritePtr += nBufLen;
        }
        else // harder case we need to wrap
        {
          int iFirstChunkSize = m_nBufSize - m_iWritePtr;
          int iSecondChunkSize = nBufLen - iFirstChunkSize;

          CopyMemory( &m_pBuf[m_iWritePtr], pBuf, iFirstChunkSize );
          CopyMemory( &m_pBuf[0], &pBuf[iFirstChunkSize], iSecondChunkSize );

          m_iWritePtr = iSecondChunkSize;
        }
        bResult = TRUE;
      }
      else 
      {
        CLog::Log(LOGDEBUG,"%s, buffer overflow! max size: %d. trying to write: %d", __FUNCTION__, GetMaxWriteSize(), nBufLen);
      }
    }
    ::LeaveCriticalSection(&m_critSection );
    return bResult;
  }

BOOL CRingBuffer::ReadBinary( char * pBuf, int nBufLen )
  {
    ::EnterCriticalSection(&m_critSection );
    BOOL bResult = FALSE;
    {
      if ( nBufLen <= GetMaxReadSize() )
      {
        // easy case, no wrapping
        if ( m_iReadPtr + nBufLen < m_nBufSize )
        {
          CopyMemory( pBuf, &m_pBuf[m_iReadPtr], nBufLen );
          m_iReadPtr += nBufLen;
        }
        else // harder case, buffer wraps
        {
          int iFirstChunkSize = m_nBufSize - m_iReadPtr;
          int iSecondChunkSize = nBufLen - iFirstChunkSize;

          CopyMemory( pBuf, &m_pBuf[m_iReadPtr], iFirstChunkSize );
          if (iSecondChunkSize > 0)
            CopyMemory( &pBuf[iFirstChunkSize], &m_pBuf[0], iSecondChunkSize );

          m_iReadPtr = iSecondChunkSize;
        }
        bResult = TRUE;
      }
      else 
      {
        CLog::Log(LOGDEBUG,"%s, buffer underflow! max size: %d. trying to read: %d", __FUNCTION__, GetMaxReadSize(), nBufLen);
      }
    }
    ::LeaveCriticalSection(&m_critSection );
    return bResult;
  }

BOOL CRingBuffer::PeakBinary( char * pBuf, int nBufLen )
  {
    ::EnterCriticalSection(&m_critSection );
    BOOL bResult = FALSE;
    int iPrevReadPtr = m_iReadPtr;
    {
      if ( nBufLen <= GetMaxReadSize() )
      {
        // easy case, no wrapping
        if ( m_iReadPtr + nBufLen < m_nBufSize )
        {
          CopyMemory( pBuf, &m_pBuf[m_iReadPtr], nBufLen );
          m_iReadPtr += nBufLen;
        }
        else // harder case, buffer wraps
        {
          int iFirstChunkSize = m_nBufSize - m_iReadPtr;
          int iSecondChunkSize = nBufLen - iFirstChunkSize;

          CopyMemory( pBuf, &m_pBuf[m_iReadPtr], iFirstChunkSize );
          if (iSecondChunkSize > 0)
            CopyMemory( &pBuf[iFirstChunkSize], &m_pBuf[0], iSecondChunkSize );

          m_iReadPtr = iSecondChunkSize;
        }
        bResult = TRUE;
      }
    }
    m_iReadPtr = iPrevReadPtr;
    ::LeaveCriticalSection(&m_critSection );
    return bResult;
  }

BOOL CRingBuffer::SkipBytes( int nBufLen )
  {
    ::EnterCriticalSection(&m_critSection );
    BOOL bResult = FALSE;
    {
      if ( nBufLen < 0 )
      {
        if (m_iReadPtr + nBufLen >= 0)
        {
          m_iReadPtr += nBufLen;
          bResult = TRUE;
        }
        else 
        {
          CLog::Log(LOGWARNING, "%s - request negative skip which is not supported.", __FUNCTION__);
        }
      }
      else if ( nBufLen <= GetMaxReadSize() )
      {
        // easy case, no wrapping
        if ( m_iReadPtr + nBufLen < m_nBufSize )
        {
          m_iReadPtr += nBufLen;
        }
        else // harder case, buffer wraps
        {
          int iFirstChunkSize = m_nBufSize - m_iReadPtr;
          int iSecondChunkSize = nBufLen - iFirstChunkSize;

          m_iReadPtr = iSecondChunkSize;
        }
        bResult = TRUE;
      }
    }
    ::LeaveCriticalSection(&m_critSection );
    return bResult;
  }
