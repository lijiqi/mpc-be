/*
 * $Id: WVSource.h 1 2012-05-05 01:42:55Z kinjalka $
 *
 * Adaptation for MPC-BE (C) 2012 Dmitry "Vortex" Koteroff (vortex@light-alloy.ru, http://light-alloy.ru)
 *
 * This file is part of MPC-BE and Light Alloy.
 * YOU CANNOT USE THIS FILE WITHOUT AUTHOR PERMISSION!
 *
 * MPC-BE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * MPC-BE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "../../parser/BaseSplitter/BaseSplitter.h"

#include "..\..\..\thirdparty\wavpacklib\wavpack\wputils.h"
#include "..\..\..\thirdparty\wavpacklib\wavpack_common.h"
#include "..\..\..\thirdparty\wavpacklib\wavpack_frame.h"
#include "..\..\..\thirdparty\wavpacklib\wavpack_parser.h"

#define WavPackSplitterName   L"Light Alloy/MPC WavPack Source"

// B5554304-3C9A-40A1-8E82-8C8CFBED56C0
static const GUID CLSID_WavPackSplitter = 
    { 0xd8cf6a42, 0x3e09, 0x4922, { 0xa4, 0x52, 0x21, 0xdf, 0xf1, 0xb, 0xee, 0xba } };

// Flag that identify additionnal block data
// It's correction data in case of WavPack
#define AM_STREAM_BLOCK_ADDITIONNAL 0x80000001

typedef struct {
	short version;
} wavpack_codec_private_data;

typedef struct {
    stream_reader iocallback;
    IAsyncReader *pReader;
    LONGLONG StreamPos;
    LONGLONG StreamLen;
} IAsyncCallBackWrapper;

IAsyncCallBackWrapper* IAsyncCallBackWrapper_new(IAsyncReader *pReader);
void IAsyncCallBackWrapper_free(IAsyncCallBackWrapper* iacw);

//-----------------------------------------------------------------------------

class CWavPackSplitterFilter;

enum Command {CMD_RESET, CMD_RUN, CMD_STOP, CMD_EXIT};

class CWavPackSplitterFilterInputPin : public CBaseInputPin,
                               public CAMThread                             
{
    friend class CWavPackSplitterFilter;

public:
    CWavPackSplitterFilterInputPin(CWavPackSplitterFilter *pParentFilter, CCritSec *pLock, HRESULT * phr);
    virtual ~CWavPackSplitterFilterInputPin();

    HRESULT CheckMediaType(const CMediaType *pmt);
    CMediaType& CurrentMediaType() { return m_mt; };

    HRESULT CheckConnect(IPin* pPin);
    HRESULT BreakConnect(void);
    HRESULT CompleteConnect(IPin *pReceivePin); 

    HRESULT Active();
    HRESULT Inactive();

    STDMETHODIMP BeginFlush();
    STDMETHODIMP EndFlush();

    HRESULT DoSeeking(REFERENCE_TIME rtStart);

protected:
    DWORD ThreadProc();
    HRESULT DoProcessingLoop();
    HRESULT DeliverOneFrame(WavPack_parser* wpp);

    CWavPackSplitterFilter *m_pParentFilter;
    IAsyncReader *m_pReader;
    IAsyncCallBackWrapper *m_pIACBW;
    WavPack_parser *m_pWavPackParser;
    
    BOOL m_bAbort;
    BOOL m_bDiscontinuity;
};

//-----------------------------------------------------------------------------

class CWavPackSplitterFilterOutputPin : public CBaseOutputPin,
                                public IMediaSeeking
{
    friend class CWavPackSplitterFilter;

public:
    CWavPackSplitterFilterOutputPin(CWavPackSplitterFilter *pParentFilter, CCritSec *pLock,
        HRESULT * phr);

    DECLARE_IUNKNOWN
        
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv)
    {
        if (riid == IID_IMediaSeeking) {
            return GetInterface((IMediaSeeking *)this, ppv);
        }
        return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
    }

    HRESULT CheckMediaType(const CMediaType *pmt);
    CMediaType& CurrentMediaType() { return m_mt; }
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
    HRESULT DecideBufferSize(IMemAllocator * pAlloc, ALLOCATOR_PROPERTIES *pProp);


    // --- IMediaSeeking ---    
    STDMETHODIMP IsFormatSupported(const GUID * pFormat);
    STDMETHODIMP QueryPreferredFormat(GUID *pFormat);
    STDMETHODIMP SetTimeFormat(const GUID * pFormat);
    STDMETHODIMP IsUsingTimeFormat(const GUID * pFormat);
    STDMETHODIMP GetTimeFormat(GUID *pFormat);
    STDMETHODIMP GetDuration(LONGLONG *pDuration);
    STDMETHODIMP GetStopPosition(LONGLONG *pStop);
    STDMETHODIMP GetCurrentPosition(LONGLONG *pCurrent);
    STDMETHODIMP GetCapabilities(DWORD * pCapabilities);
    STDMETHODIMP CheckCapabilities(DWORD * pCapabilities);
    STDMETHODIMP ConvertTimeFormat(LONGLONG * pTarget, const GUID * pTargetFormat,
        LONGLONG Source, const GUID * pSourceFormat);   
    STDMETHODIMP SetPositions( LONGLONG * pCurrent, DWORD CurrentFlags,
        LONGLONG * pStop, DWORD StopFlags);
    STDMETHODIMP GetPositions(LONGLONG * pCurrent, LONGLONG * pStop);
    STDMETHODIMP GetAvailable(LONGLONG * pEarliest, LONGLONG * pLatest);
    STDMETHODIMP SetRate(double dRate);
    STDMETHODIMP GetRate(double * pdRate);
    STDMETHODIMP GetPreroll(LONGLONG *pPreroll);

protected:
    CWavPackSplitterFilter *m_pParentFilter;    
};

//-----------------------------------------------------------------------------

class __declspec(uuid("B5554304-3C9A-40A1-8E82-8C8CFBED56C0"))
		CWavPackSplitterFilter : public CBaseFilter
{

public :
    DECLARE_IUNKNOWN
    static CUnknown *WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr); 

    CWavPackSplitterFilter(LPUNKNOWN lpunk, HRESULT *phr);
    virtual ~CWavPackSplitterFilter();
 
    // ----- CBaseFilter -----
    int GetPinCount();
    CBasePin *GetPin(int n);
    STDMETHODIMP Stop(void);
    STDMETHODIMP Pause(void);
    STDMETHODIMP Run(REFERENCE_TIME tStart);
    STDMETHODIMP JoinFilterGraph(IFilterGraph *pGraph, LPCWSTR pName);

    HRESULT BeginFlush();
    HRESULT EndFlush();

protected:
    CCritSec m_Lock;

    friend class CWavPackSplitterFilterInputPin;
    friend class CWavPackSplitterFilterOutputPin;

    CWavPackSplitterFilterInputPin* m_pInputPin;
    CWavPackSplitterFilterOutputPin* m_pOutputPin;

    REFERENCE_TIME m_rtStart, m_rtDuration, m_rtStop;
    DWORD m_dwSeekingCaps;
    double m_dRateSeeking;

    void SetDuration(REFERENCE_TIME rtDuration);

    HRESULT DoSeeking();

    WavPack_parser *GetWavPackParser()
	{
		return m_pInputPin->m_pWavPackParser;
	}
};

