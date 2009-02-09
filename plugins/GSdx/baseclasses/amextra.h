//------------------------------------------------------------------------------
// File: AMExtra.h
//
// Desc: DirectShow base classes.
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------


#ifndef __AMEXTRA__
#define __AMEXTRA__

// Simple rendered input pin
//
// NOTE if your filter queues stuff before rendering then it may not be
// appropriate to use this class
//
// In that case queue the end of stream condition until the last sample
// is actually rendered and flush the condition appropriately

class CRenderedInputPin : public CBaseInputPin
{
public:

    CRenderedInputPin(TCHAR *pObjectName,
                      CBaseFilter *pFilter,
                      CCritSec *pLock,
                      HRESULT *phr,
                      LPCWSTR pName);
#ifdef UNICODE
    CRenderedInputPin(CHAR *pObjectName,
                      CBaseFilter *pFilter,
                      CCritSec *pLock,
                      HRESULT *phr,
                      LPCWSTR pName);
#endif
    
    // Override methods to track end of stream state
    STDMETHODIMP EndOfStream();
    STDMETHODIMP EndFlush();

    HRESULT Active();
    HRESULT Run(REFERENCE_TIME tStart);

protected:

    // Member variables to track state
    BOOL m_bAtEndOfStream;      // Set by EndOfStream
    BOOL m_bCompleteNotified;   // Set when we notify for EC_COMPLETE

private:
    void DoCompleteHandling();
};

#endif // __AMEXTRA__

