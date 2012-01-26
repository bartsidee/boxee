/* 
 * $Id: DXVADecoder.cpp 1207 2009-08-02 15:35:14Z casimir666 $
 *
 * (C) 2006-2007 see AUTHORS
 *
 * This file is part of mplayerc.
 *
 * Mplayerc is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mplayerc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifdef HAS_DX

#include <dxva2api.h>
#include "DVDVideoCodecDXVA.h"
#include "DXVADecoderH264.h"
#include "system.h"
#include "log.h"

extern "C"
{
	#include "FfmpegContext.h"
}


#define TRACE(x) CLog::Log(LOGDEBUG, x)

#define MAX_RETRY_ON_PENDING		50
#define DO_DXVA_PENDING_LOOP(x)		nTry = 0; \
									while (FAILED(hr = x) && nTry<MAX_RETRY_ON_PENDING) \
									{ \
										if (hr != E_PENDING) break; \
										TRACE ("Pending loop %d\n", nTry); \
										Sleep(1); \
										nTry++; \
									}


CDXVADecoder::CDXVADecoder (CDVDVideoCodecDXVA* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, DXVAMode nMode, int nPicEntryNumber, DXVA2_ConfigPictureDecode* pDXVA2Config)	
{
	m_nEngine			= ENGINE_DXVA2;
	m_pDirectXVideoDec	= pDirectXVideoDec;
	memcpy (&m_DXVA2Config, pDXVA2Config, sizeof(DXVA2_ConfigPictureDecode));

	Init (pFilter, nMode, nPicEntryNumber);
};

CDXVADecoder::~CDXVADecoder()
{
	SAFE_DELETE_ARRAY (m_pPictureStore);
	SAFE_DELETE_ARRAY (m_ExecuteParams.pCompressedBuffers);
}

void CDXVADecoder::Init(CDVDVideoCodecDXVA* pFilter, DXVAMode nMode, int nPicEntryNumber)
{
	m_pFilter			= pFilter;
	m_nMode				= nMode;
	m_nPicEntryNumber	= nPicEntryNumber;
	m_pPictureStore		= new PICTURE_STORE[nPicEntryNumber];

	memset (&m_ExecuteParams, 0, sizeof(m_ExecuteParams));
	Flush();
}

// === Public functions
void CDXVADecoder::AllocExecuteParams (int nSize)
{
	m_ExecuteParams.pCompressedBuffers	= new DXVA2_DecodeBufferDesc[nSize];

	for (int i=0; i<nSize; i++)
		memset (&m_ExecuteParams.pCompressedBuffers[i], 0, sizeof(DXVA2_DecodeBufferDesc));
}

void CDXVADecoder::SetExtraData (BYTE* pDataIn, UINT nSize)
{
	// Extradata is codec dependant
	UNREFERENCED_PARAMETER (pDataIn);
	UNREFERENCED_PARAMETER (nSize);
}

void CDXVADecoder::CopyBitstream(BYTE* pDXVABuffer, BYTE* pBuffer, UINT& nSize)
{
	memcpy (pDXVABuffer, (BYTE*)pBuffer, nSize);
}

void CDXVADecoder::Flush()
{
	TRACE ("CDXVADecoder::Flush\n");
	for (int i=0; i<m_nPicEntryNumber; i++)
	{
		m_pPictureStore[i].bRefPicture	= false;
		m_pPictureStore[i].bInUse		= false;
		m_pPictureStore[i].bDisplayed	= false;
		m_pPictureStore[i].nCodecSpecific = -1;
		m_pPictureStore[i].dwDisplayCount = 0;
    m_pPictureStore[i].rtStart = 0;
    m_pPictureStore[i].rtStop = 0;
	}

	m_nWaitingPics	= 0;
	m_bFlushed		= true;
	m_nFieldSurface = -1;
	m_dwDisplayCount= 1;
	m_pFieldSample	= NULL;
}

// === DXVA functions

HRESULT CDXVADecoder::AddExecuteBuffer (DWORD CompressedBufferType, UINT nSize, void* pBuffer, UINT* pRealSize)
{
	HRESULT			hr			= E_INVALIDARG;
  
	DWORD			dwNumMBs	= 0;
	BYTE*			pDXVABuffer;

	//if (CompressedBufferType != DXVA2_PictureParametersBufferType && CompressedBufferType != DXVA2_InverseQuantizationMatrixBufferType)
	//	dwNumMBs = FFGetMBNumber (m_pFilter->GetAVCtx());

  UINT						nDXVASize;
  hr = m_pDirectXVideoDec->GetBuffer (CompressedBufferType, (void**)&pDXVABuffer, &nDXVASize);
	ASSERT (nSize <= nDXVASize);

	if (SUCCEEDED (hr) && (nSize <= nDXVASize))
	{
	  if (CompressedBufferType == DXVA2_BitStreamDateBufferType)
		  CopyBitstream (pDXVABuffer, (BYTE*)pBuffer, nSize);
		else
			memcpy (pDXVABuffer, (BYTE*)pBuffer, nSize);

	  m_ExecuteParams.pCompressedBuffers[m_ExecuteParams.NumCompBuffers].CompressedBufferType = CompressedBufferType;
		m_ExecuteParams.pCompressedBuffers[m_ExecuteParams.NumCompBuffers].DataSize				= nSize;
		m_ExecuteParams.pCompressedBuffers[m_ExecuteParams.NumCompBuffers].NumMBsInBuffer		= dwNumMBs;
		m_ExecuteParams.NumCompBuffers++;
  }

  if (pRealSize) *pRealSize = nSize;

  return hr;
}


HRESULT CDXVADecoder::GetDeliveryBuffer(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, IDirect3DSurface9** ppSampleToDeliver)
{
	HRESULT					hr = E_INVALIDARG;
 
  /*
	CComPtr<IMediaSample>	pNewSample;

	// Change aspect ratio for DXVA2
	if (m_nEngine == ENGINE_DXVA2)
	{
		m_pFilter->UpdateAspectRatio();
		m_pFilter->ReconnectOutput(m_pFilter->PictWidthRounded(), m_pFilter->PictHeightRounded(), true, m_pFilter->PictWidth(), m_pFilter->PictHeight());
	}
	hr		= m_pFilter->GetOutputPin()->GetDeliveryBuffer(&pNewSample, 0, 0, 0);

	if (SUCCEEDED (hr))
	{
		pNewSample->SetTime(&rtStart, &rtStop);
		pNewSample->SetMediaTime(NULL, NULL);
		*ppSampleToDeliver = pNewSample.Detach();
	}
  */
 
	return hr;
}

HRESULT CDXVADecoder::Execute()
{
	HRESULT		hr = E_INVALIDARG;

	for (DWORD i=0; i<m_ExecuteParams.NumCompBuffers; i++)
	{
	  hr = m_pDirectXVideoDec->ReleaseBuffer (m_ExecuteParams.pCompressedBuffers[i].CompressedBufferType);
		ASSERT (SUCCEEDED (hr));
	}

	hr = m_pDirectXVideoDec->Execute(&m_ExecuteParams);
	m_ExecuteParams.NumCompBuffers	= 0;

	return hr;
}

HRESULT CDXVADecoder::QueryStatus(PVOID LPDXVAStatus, UINT nSize)
{
	HRESULT						hr = E_INVALIDARG;
  
	DXVA2_DecodeExecuteParams	ExecuteParams;
	DXVA2_DecodeExtensionData	ExtensionData;
	DWORD						dwFunction = 0x07000000;

  memset (&ExecuteParams, 0, sizeof(ExecuteParams));
	memset (&ExtensionData, 0, sizeof(ExtensionData));
	ExecuteParams.pExtensionData		= &ExtensionData;
	ExtensionData.pPrivateOutputData	= LPDXVAStatus;
	ExtensionData.PrivateOutputDataSize	= nSize;
	ExtensionData.Function				= 7;
	hr = m_pDirectXVideoDec->Execute(&ExecuteParams);

	return hr;
}

HRESULT CDXVADecoder::BeginFrame(IDirect3DSurface9* pSampleToDeliver)
{
	HRESULT				hr   = E_INVALIDARG;

	int					nTry = 0;

	for (int i=0; i<20; i++)
	{
	  IDirect3DSurface9*	pDecoderRenderTarget = pSampleToDeliver;
		if (pDecoderRenderTarget)
		{
			DO_DXVA_PENDING_LOOP (m_pDirectXVideoDec->BeginFrame(pDecoderRenderTarget, NULL));
		}
			
		// For slow accelerator wait a little...
		if (SUCCEEDED (hr)) break;
		Sleep(1);
	}

	return hr;
}


HRESULT CDXVADecoder::EndFrame(int nSurfaceIndex)
{
	HRESULT		hr		= E_INVALIDARG;

	hr = m_pDirectXVideoDec->EndFrame(NULL);
	
	return hr;
}

// === Picture store functions
bool CDXVADecoder::AddToStore (int nSurfaceIndex, bool bRefPicture, 
							   REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, bool bIsField, 
							   FF_FIELD_TYPE nFieldType, FF_SLICE_TYPE nSliceType, int nCodecSpecific)
{
	if (bIsField && (m_nFieldSurface == -1))
	{
		m_nFieldSurface = nSurfaceIndex;
		m_pPictureStore[nSurfaceIndex].n1FieldType		= nFieldType;
		m_pPictureStore[nSurfaceIndex].rtStart			= rtStart;
		m_pPictureStore[nSurfaceIndex].rtStop			= rtStop;
		m_pPictureStore[nSurfaceIndex].nCodecSpecific	= nCodecSpecific;
		return false;
	}
	else
	{
		//TRACE ("Add Stor: %10I64d - %10I64d   Ind = %d  Codec=%d\n", rtStart, rtStop, nSurfaceIndex, nCodecSpecific);
   
		ASSERT (!m_pPictureStore[nSurfaceIndex].bInUse);
		ASSERT (nSurfaceIndex < m_nPicEntryNumber);

		m_pPictureStore[nSurfaceIndex].bRefPicture		= bRefPicture;
		m_pPictureStore[nSurfaceIndex].bInUse			= true;
		m_pPictureStore[nSurfaceIndex].bDisplayed		= false;
		m_pPictureStore[nSurfaceIndex].nSliceType		= nSliceType;

		if (!bIsField)
		{
			m_pPictureStore[nSurfaceIndex].rtStart			= rtStart;
			m_pPictureStore[nSurfaceIndex].rtStop			= rtStop;
			m_pPictureStore[nSurfaceIndex].n1FieldType		= nFieldType;
			m_pPictureStore[nSurfaceIndex].nCodecSpecific	= nCodecSpecific;
		}
	
		m_nFieldSurface	= -1;
		m_nWaitingPics++;
   
		return true;
	}
}

void CDXVADecoder::UpdateStore (int nSurfaceIndex, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
	ASSERT ((nSurfaceIndex < m_nPicEntryNumber) && m_pPictureStore[nSurfaceIndex].bInUse && !m_pPictureStore[nSurfaceIndex].bDisplayed);
	m_pPictureStore[nSurfaceIndex].rtStart			= rtStart;
	m_pPictureStore[nSurfaceIndex].rtStop			= rtStop;
}

void CDXVADecoder::RemoveRefFrame (int nSurfaceIndex)
{
	ASSERT ((nSurfaceIndex < m_nPicEntryNumber) && m_pPictureStore[nSurfaceIndex].bInUse);

	m_pPictureStore[nSurfaceIndex].bRefPicture	= false;
	if (m_pPictureStore[nSurfaceIndex].bDisplayed)
		FreePictureSlot (nSurfaceIndex);	
}


int CDXVADecoder::FindOldestFrame()
{
	REFERENCE_TIME		rtMin	= _I64_MAX;
	int					nPos	= -1;
 
	// TODO : find better solution...
	if (m_nWaitingPics > m_nMaxWaiting)
	{
		for (int i=0; i<m_nPicEntryNumber; i++)
		{
			if (!m_pPictureStore[i].bDisplayed &&
				 m_pPictureStore[i].bInUse &&
				(m_pPictureStore[i].rtStart < rtMin))
			{
				rtMin	= m_pPictureStore[i].rtStart;
				nPos	= i;
			}
		}
	}
 
	return nPos;
}

HRESULT CDXVADecoder::DisplayNextFrame()
{
	HRESULT						hr = S_FALSE;
	int							  nPicIndex;

	nPicIndex = FindOldestFrame();
 
	if (nPicIndex != -1)
	{
		if (m_pPictureStore[nPicIndex].rtStart >= 0)
		{
			hr = m_pFilter->Deliver(nPicIndex, m_pPictureStore[nPicIndex].rtStart, m_pPictureStore[nPicIndex].rtStop);		
		}

		m_pPictureStore[nPicIndex].bDisplayed		= true;
		if (!m_pPictureStore[nPicIndex].bRefPicture) 
			FreePictureSlot (nPicIndex);
	}
 
	return hr;
}

HRESULT CDXVADecoder::GetFreeSurfaceIndex(int& nSurfaceIndex, IDirect3DSurface9** ppSampleToDeliver, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop)
{
  HRESULT		hr			 = E_UNEXPECTED;
  int			nPos		 = -1;
  DWORD		dwMinDisplay = MAXDWORD;

  for (int i=0; i<m_nPicEntryNumber; i++)
  {
    if (!m_pPictureStore[i].bInUse && m_pPictureStore[i].dwDisplayCount < dwMinDisplay)
    {
      dwMinDisplay = m_pPictureStore[i].dwDisplayCount;
      nPos  = i;
    }
  } 

  if (nPos != -1)
  {
    nSurfaceIndex = nPos;
    m_pFilter->GetSufrace(nSurfaceIndex, ppSampleToDeliver);
    return S_OK;
  }

  // Something is wrong...
  ASSERT(FALSE); 
  return E_UNEXPECTED;
}


void CDXVADecoder::FreePictureSlot (int nSurfaceIndex)
{

//	TRACE ("Free    : %d\n", nSurfaceIndex);
	m_pPictureStore[nSurfaceIndex].dwDisplayCount = m_dwDisplayCount++;
	m_pPictureStore[nSurfaceIndex].bInUse		= false;
	m_pPictureStore[nSurfaceIndex].bDisplayed	= false;
	m_pPictureStore[nSurfaceIndex].nCodecSpecific = -1;
	m_nWaitingPics--; 

 // m_pFilter->AddFrame(nSurfaceIndex);
}


BYTE CDXVADecoder::GetConfigResidDiffAccelerator()
{
	return m_DXVA2Config.ConfigResidDiffAccelerator;
}


BYTE CDXVADecoder::GetConfigIntraResidUnsigned()
{
	return m_DXVA2Config.ConfigIntraResidUnsigned;
}

#endif
