//////////////////////////////////////////////////////////////////////
// RingBuffer.h: Interface and implementation of CRingBuffer.
//////////////////////////////////////////////////////////////////////
//
// CRingBuffer - An MFC ring buffer class.
//
// Purpose:
//     This class was designed to be a fast way of collecting incoming
//     data from a socket, then retreiving it one text line at a time.
//     It intentionally does no dynamic buffer allocation/deallocation
//     during runtime, so as to avoid memory fragmentation and other
//     issues. It is currently used in POP3 and SMTP servers created
//     by Stardust Software.
//
// Author:
//     Written by Larry Antram (larrya@sdust.com)
//     Copyright � 2001-2002 Stardust Software. All Rights Reserved.
//     http://www.stardustsoftware.com
//
// Legal Notice:
//     This code may be used in compiled form in any way you desire
//     and may be redistributed unmodified by any means PROVIDING it
//     is not sold for profit without the authors written consent, and
//     providing that this entire notice and the authors name and all
//     copyright notices remains intact.
//
//     This file is provided "as is" with no expressed or implied
//     warranty. The author accepts no liability for any damage/loss
//     of business that this product may cause.
//
// History:
//     Dec 23 2001 - Initial creation.
//     May 15 2002 - Posted to CodeProject.com.
//
// Sample Usage:
//
//     //
//     // Initialization
//     //
//
//     using namespace Stardust;
//
//     #define INCOMING_RING_BUFFER_SIZE (1024*16) // or whatever
//
//     CRingBuffer m_ringbuf;
//     m_ringbuf.Create( INCOMING_RING_BUFFER_SIZE + 1 );
//
//     char m_chInBuf[ INCOMING_BUFFER_SIZE + 1 ];
//
//     ...
//
//     //
//     // Then later, upon receiving data...
//     //
//
//     int iNumberOfBytesRead = 0;
//
//     while( READ_INCOMING(m_chInBuf,INCOMING_BUFFER_SIZE,&iNumberOfBytesRead,0)
//     && iNumberOfBytesRead > 0 )
//     {
//          // add incoming data to the ring buffer
//          m_ringbuf.WriteBinary(m_chInBuf,iNumberOfBytesRead);
//
//          // pull it back out one line at a time, and distribute it
//          CString strLine;
//          while( m_ringbuf.ReadTextLine(strLine) )
//          {
//              strLine.TrimRight("\r\n");
//
//              if( !ON_INCOMING( strLine ) )
//                  return FALSE;
//          }
//     }
//
//     // Fall out, until more incoming data is available...
//
// Notes:
//     In the above example, READ_INCOMING and ON_INCOMING are
//     placeholders for functions, that read and accept incoming data,
//     respectively.
//
//     READ_INCOMING receives raw data from the socket.
//
//     ON_INCOMING accepts incoming data from the socket, one line at
//     a time.
//
//////////////////////////////////////////////////////////////////////

#ifndef __RingBuffer_h
#define __RingBuffer_h
#include "utils/CriticalSection.h"
#include "utils/SingleLock.h"
#include "utils/log.h"
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef _LINUX
#include "PlatformInclude.h"
#endif

class CRingBuffer
{
protected:
  ///////////////////////////////////////////////////////////////////
  // Protected Member Variables
  //
  char * m_pBuf;
  int m_nBufSize;   // the size of the ring buffer
  int m_iReadPtr;   // the read pointer
  int m_iWritePtr;  // the write pointer

  mutable CRITICAL_SECTION m_critSection;

public:
  ///////////////////////////////////////////////////////////////////
  // Constructor
  //
  CRingBuffer();

  ///////////////////////////////////////////////////////////////////
  // Destructor
  //
  virtual ~CRingBuffer();

  ///////////////////////////////////////////////////////////////////
  // Method: Create
  // Purpose: Initializes the ring buffer for use.
  // Parameters:
  //     [in] iBufSize -- maximum size of the ring buffer
  // Return Value: TRUE if successful, otherwise FALSE.
  //
  BOOL Create( int iBufSize );

  int Size();

  ///////////////////////////////////////////////////////////////////
  // Method: Destroy
  // Purpose: Cleans up ring buffer by freeing memory and resetting
  //     member variables to original state.
  // Parameters: (None)
  // Return Value: (None)
  //
  void Destroy();

  ///////////////////////////////////////////////////////////////////
  // Method: Clear
  // Purpose: Reset the buffer
  // Parameters: (None)
  // Return Value: (None)
  //
  void Clear();

  ///////////////////////////////////////////////////////////////////
  // Method: Lock
  // Purpose: enables user to retrieve the internal lock
  // Parameters: (None)
  // Return Value: (None)
  //
  void Lock();

  ///////////////////////////////////////////////////////////////////
  // Method: Unlock
  // Purpose: enables user to release the internal lock
  // Parameters: (None)
  // Return Value: (None)
  //
  void Unlock();

  ///////////////////////////////////////////////////////////////////
  // Method: Append
  // Purpose: Appends the content of the provided RingBuffer 
  // Parameters: [in] buf - the buffer to append 
  // Return Value: TRUE if successful. otherwise FALSE
  //
  BOOL Append(const CRingBuffer &buf);

  ///////////////////////////////////////////////////////////////////
  // Method: ReadBinary
  // Purpose: Reads (and extracts) data from the ring buffer to another ring buffer
  // Parameters:
  //     [out] buf - destination ring buffer.
  //     [in] nBufLen - Size of the data to be read (in bytes).
  // Return Value: TRUE upon success, otherwise FALSE.
  //
  BOOL ReadBinary( CRingBuffer &buf, int nBufLen );

  ///////////////////////////////////////////////////////////////////
  // Method: Copy
  // Purpose: Copies the content of the provided RingBuffer 
  // Parameters: [in] buf - the buffer to append 
  // Return Value: TRUE if successful. otherwise FALSE
  //
  BOOL Copy(const CRingBuffer &buf);

  ///////////////////////////////////////////////////////////////////
  // Method: GetMaxReadSize
  // Purpose: Returns the amount of data (in bytes) available for
  //     reading from the buffer.
  // Parameters: (None)
  // Return Value: Amount of data (in bytes) available for reading.
  //
  int GetMaxReadSize() const;

  ///////////////////////////////////////////////////////////////////
  // Method: GetMaxWriteSize
  // Purpose: Returns the amount of space (in bytes) available for
  //     writing into the buffer.
  // Parameters: (None)
  // Return Value: Amount of space (in bytes) available for writing.
  //
  int GetMaxWriteSize() const;

  ///////////////////////////////////////////////////////////////////
  // Method: WriteBinary
  // Purpose: Writes binary data into the ring buffer.
  // Parameters:
  //     [in] pBuf - Pointer to the data to write.
  //     [in] nBufLen - Size of the data to write (in bytes).
  // Return Value: TRUE upon success, otherwise FALSE.
  //
  BOOL WriteBinary(const char * pBuf, int nBufLen );

  ///////////////////////////////////////////////////////////////////
  // Method: ReadBinary
  // Purpose: Reads (and extracts) data from the ring buffer.
  // Parameters:
  //     [in/out] pBuf - Pointer to where read data will be stored.
  //     [in] nBufLen - Size of the data to be read (in bytes).
  // Return Value: TRUE upon success, otherwise FALSE.
  //
  BOOL ReadBinary( char * pBuf, int nBufLen );

  BOOL PeakBinary( char * pBuf, int nBufLen );

  BOOL SkipBytes( int nBufLen );
};

#endif//__RingBuffer_h

///////////////////////////////////////////////////////////////////////////////
// End of file
///////////////////////////////////////////////////////////////////////////////
