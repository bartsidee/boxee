/* 
 * $Id: DXVADecoder.h 1179 2009-07-22 15:01:57Z casimir666 $
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

#pragma once

#include <dxva2api.h>

typedef enum
{
	ENGINE_DXVA1,
	ENGINE_DXVA2
} DXVA_ENGINE;

typedef enum
{
	H264_VLD,
	VC1_VLD
} DXVAMode;

typedef enum
{
	PICT_TOP_FIELD     = 1,
	PICT_BOTTOM_FIELD  = 2,
	PICT_FRAME         = 3
} FF_FIELD_TYPE;

typedef enum
{
	I_TYPE  = 1, ///< Intra
	P_TYPE  = 2, ///< Predicted
	B_TYPE  = 3, ///< Bi-dir predicted
	S_TYPE  = 4, ///< S(GMC)-VOP MPEG4
	SI_TYPE = 5, ///< Switching Intra
	SP_TYPE = 6, ///< Switching Predicted
	BI_TYPE = 7
} FF_SLICE_TYPE;

typedef struct
{
	bool						bRefPicture;	// True if reference picture
	int							bInUse;			// Slot in use
	bool						bDisplayed;		// True if picture have been presented
	REFERENCE_TIME				rtStart;
	REFERENCE_TIME				rtStop;
	FF_FIELD_TYPE				n1FieldType;	// Top or bottom for the 1st field
	FF_SLICE_TYPE				nSliceType;
	int							nCodecSpecific;
	DWORD						dwDisplayCount;
} PICTURE_STORE;


#define MAX_COM_BUFFER				6		// Max uncompressed buffer for an Execute command (DXVA1)
#define COMP_BUFFER_COUNT			18

class CDVDVideoCodecDXVA;

class CDXVADecoder
{
public :
	// === Public functions
	virtual				   ~CDXVADecoder();
	DXVAMode				GetMode()		{ return m_nMode; };
	DXVA_ENGINE				GetEngine()		{ return m_nEngine; };
	void					AllocExecuteParams (int nSize);

	virtual HRESULT			DecodeFrame  (BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop) = NULL;
	virtual void			SetExtraData (BYTE* pDataIn, UINT nSize);
	virtual void			CopyBitstream(BYTE* pDXVABuffer, BYTE* pBuffer, UINT& nSize);
	virtual void			Flush();

protected :
	CDXVADecoder (CDVDVideoCodecDXVA* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, DXVAMode nMode, int nPicEntryNumber, DXVA2_ConfigPictureDecode* pDXVA2Config);

  CDVDVideoCodecDXVA*	m_pFilter;

	bool							m_bFlushed;
	int								m_nMaxWaiting;

	PICTURE_STORE*					m_pPictureStore;		// Store reference picture, and delayed B-frames
	int								m_nPicEntryNumber;		// Total number of picture in store
	int								m_nWaitingPics;			// Number of picture not yet displayed

	// === DXVA functions
	HRESULT						AddExecuteBuffer (DWORD CompressedBufferType, UINT nSize, void* pBuffer, UINT* pRealSize = NULL);
	HRESULT						GetDeliveryBuffer(REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, IDirect3DSurface9** ppSampleToDeliver);
	HRESULT						Execute();
	HRESULT						BeginFrame(IDirect3DSurface9* pSampleToDeliver);
	HRESULT						EndFrame(int nSurfaceIndex);
	HRESULT						QueryStatus(PVOID LPDXVAStatus, UINT nSize);
	BYTE						GetConfigIntraResidUnsigned();
	BYTE						GetConfigResidDiffAccelerator();

	// === Picture store functions
	bool					AddToStore (int nSurfaceIndex, bool bRefPicture, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, bool bIsField, FF_FIELD_TYPE nFieldType, FF_SLICE_TYPE nSliceType, int nCodecSpecific);
	void					UpdateStore (int nSurfaceIndex, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop);
	void					RemoveRefFrame (int nSurfaceIndex);
	HRESULT					DisplayNextFrame();
	HRESULT					GetFreeSurfaceIndex(int& nSurfaceIndex, IDirect3DSurface9** ppSampleToDeliver, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop);
	virtual int			FindOldestFrame();


private:
	DXVAMode						  m_nMode;
	DXVA_ENGINE						m_nEngine;

  IDirect3DSurface9*		m_pFieldSample;
	int								    m_nFieldSurface;

  DWORD							    m_dwDisplayCount;

	// === DXVA2 variables
	IDirectXVideoDecoder*	      m_pDirectXVideoDec;
	DXVA2_ConfigPictureDecode		m_DXVA2Config;
	DXVA2_DecodeExecuteParams		m_ExecuteParams;

	void					Init(CDVDVideoCodecDXVA* pFilter, DXVAMode nMode, int nPicEntryNumber);
	void					FreePictureSlot (int nSurfaceIndex);
};