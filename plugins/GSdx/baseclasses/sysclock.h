//------------------------------------------------------------------------------
// File: SysClock.h
//
// Desc: DirectShow base classes - defines a system clock implementation of
//       IReferenceClock.
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------


#ifndef __SYSTEMCLOCK__
#define __SYSTEMCLOCK__

//
// Base clock.  Uses timeGetTime ONLY
// Uses most of the code in the base reference clock.
// Provides GetTime
//

class CSystemClock : public CBaseReferenceClock, public IAMClockAdjust, public IPersist
{
public:
    // We must be able to create an instance of ourselves
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);
    CSystemClock(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr);

    DECLARE_IUNKNOWN

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,void ** ppv);

    // Yield up our class id so that we can be persisted
    // Implement required Ipersist method
    STDMETHODIMP GetClassID(CLSID *pClsID);

    //  IAMClockAdjust methods
    STDMETHODIMP SetClockDelta(REFERENCE_TIME rtDelta);
}; //CSystemClock

#endif /* __SYSTEMCLOCK__ */
