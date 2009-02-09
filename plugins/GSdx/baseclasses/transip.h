//------------------------------------------------------------------------------
// File: TransIP.h
//
// Desc: DirectShow base classes - defines classes from which simple
//       Transform-In-Place filters may be derived.
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------


//
// The difference between this and Transfrm.h is that Transfrm copies the data.
//
// It assumes the filter has one input and one output stream, and has no
// interest in memory management, interface negotiation or anything else.
//
// Derive your class from this, and supply Transform and the media type/format
// negotiation functions. Implement that class, compile and link and
// you're done.


#ifndef __TRANSIP__
#define __TRANSIP__

// ======================================================================
// This is the com object that represents a simple transform filter. It
// supports IBaseFilter, IMediaFilter and two pins through nested interfaces
// ======================================================================

class CTransInPlaceFilter;

// Several of the pin functions call filter functions to do the work,
// so you can often use the pin classes unaltered, just overriding the
// functions in CTransInPlaceFilter.  If that's not enough and you want
// to derive your own pin class, override GetPin in the filter to supply
// your own pin classes to the filter.

// ==================================================
// Implements the input pin
// ==================================================

class CTransInPlaceInputPin : public CTransformInputPin
{

protected:
    CTransInPlaceFilter * const m_pTIPFilter;    // our filter
    BOOL                 m_bReadOnly;    // incoming stream is read only

public:

    CTransInPlaceInputPin(
        TCHAR               *pObjectName,
        CTransInPlaceFilter *pFilter,
        HRESULT             *phr,
        LPCWSTR              pName);

    // --- IMemInputPin -----

    // Provide an enumerator for media types by getting one from downstream
    STDMETHODIMP EnumMediaTypes( IEnumMediaTypes **ppEnum );

    // Say whether media type is acceptable.
    HRESULT CheckMediaType(const CMediaType* pmt);

    // Return our upstream allocator
    STDMETHODIMP GetAllocator(IMemAllocator ** ppAllocator);

    // get told which allocator the upstream output pin is actually
    // going to use.
    STDMETHODIMP NotifyAllocator(IMemAllocator * pAllocator,
                                 BOOL bReadOnly);

    // Allow the filter to see what allocator we have
    // N.B. This does NOT AddRef
    IMemAllocator * PeekAllocator() const
        {  return m_pAllocator; }

    // Pass this on downstream if it ever gets called.
    STDMETHODIMP GetAllocatorRequirements(ALLOCATOR_PROPERTIES *pProps);

    HRESULT CompleteConnect(IPin *pReceivePin);

    inline const BOOL ReadOnly() { return m_bReadOnly ; }

};  // CTransInPlaceInputPin

// ==================================================
// Implements the output pin
// ==================================================

class CTransInPlaceOutputPin : public CTransformOutputPin
{

protected:
    // m_pFilter points to our CBaseFilter
    CTransInPlaceFilter * const m_pTIPFilter;

public:

    CTransInPlaceOutputPin(
        TCHAR               *pObjectName,
        CTransInPlaceFilter *pFilter,
        HRESULT             *phr,
        LPCWSTR              pName);


    // --- CBaseOutputPin ------------

    // negotiate the allocator and its buffer size/count
    // Insists on using our own allocator.  (Actually the one upstream of us).
    // We don't override this - instead we just agree the default
    // then let the upstream filter decide for itself on reconnect
    // virtual HRESULT DecideAllocator(IMemInputPin * pPin, IMemAllocator ** pAlloc);

    // Provide a media type enumerator.  Get it from upstream.
    STDMETHODIMP EnumMediaTypes( IEnumMediaTypes **ppEnum );

    // Say whether media type is acceptable.
    HRESULT CheckMediaType(const CMediaType* pmt);

    //  This just saves the allocator being used on the output pin
    //  Also called by input pin's GetAllocator()
    void SetAllocator(IMemAllocator * pAllocator);

    IMemInputPin * ConnectedIMemInputPin()
        { return m_pInputPin; }

    // Allow the filter to see what allocator we have
    // N.B. This does NOT AddRef
    IMemAllocator * PeekAllocator() const
        {  return m_pAllocator; }

    HRESULT CompleteConnect(IPin *pReceivePin);

};  // CTransInPlaceOutputPin


class AM_NOVTABLE CTransInPlaceFilter : public CTransformFilter
{

public:

    // map getpin/getpincount for base enum of pins to owner
    // override this to return more specialised pin objects

    virtual CBasePin *GetPin(int n);

public:

    //  Set bModifiesData == false if your derived filter does
    //  not modify the data samples (for instance it's just copying
    //  them somewhere else or looking at the timestamps).

    CTransInPlaceFilter(TCHAR *, LPUNKNOWN, REFCLSID clsid, HRESULT *,
                        bool bModifiesData = true);
#ifdef UNICODE
    CTransInPlaceFilter(CHAR *, LPUNKNOWN, REFCLSID clsid, HRESULT *,
                        bool bModifiesData = true);
#endif
    // The following are defined to avoid undefined pure virtuals.
    // Even if they are never called, they will give linkage warnings/errors

    // We override EnumMediaTypes to bypass the transform class enumerator
    // which would otherwise call this.
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType)
        {   DbgBreak("CTransInPlaceFilter::GetMediaType should never be called");
            return E_UNEXPECTED;
        }

    // This is called when we actually have to provide out own allocator.
    HRESULT DecideBufferSize(IMemAllocator*, ALLOCATOR_PROPERTIES *);

    // The functions which call this in CTransform are overridden in this
    // class to call CheckInputType with the assumption that the type
    // does not change.  In Debug builds some calls will be made and
    // we just ensure that they do not assert.
    HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
    {
        return S_OK;
    };


    // =================================================================
    // ----- You may want to override this -----------------------------
    // =================================================================

    HRESULT CompleteConnect(PIN_DIRECTION dir,IPin *pReceivePin);

    // chance to customize the transform process
    virtual HRESULT Receive(IMediaSample *pSample);

    // =================================================================
    // ----- You MUST override these -----------------------------------
    // =================================================================

    virtual HRESULT Transform(IMediaSample *pSample) PURE;

    // this goes in the factory template table to create new instances
    // static CCOMObject * CreateInstance(LPUNKNOWN, HRESULT *);


#ifdef PERF
    // Override to register performance measurement with a less generic string
    // You should do this to avoid confusion with other filters
    virtual void RegisterPerfId()
         {m_idTransInPlace = MSR_REGISTER(TEXT("TransInPlace"));}
#endif // PERF


// implementation details

protected:

    IMediaSample * CTransInPlaceFilter::Copy(IMediaSample *pSource);

#ifdef PERF
    int m_idTransInPlace;                 // performance measuring id
#endif // PERF
    bool  m_bModifiesData;                // Does this filter change the data?

    // these hold our input and output pins

    friend class CTransInPlaceInputPin;
    friend class CTransInPlaceOutputPin;

    CTransInPlaceInputPin  *InputPin() const
    {
        return (CTransInPlaceInputPin *)m_pInput;
    };
    CTransInPlaceOutputPin *OutputPin() const
    {
        return (CTransInPlaceOutputPin *)m_pOutput;
    };

    //  Helper to see if the input and output types match
    BOOL TypesMatch()
    {
        return InputPin()->CurrentMediaType() ==
               OutputPin()->CurrentMediaType();
    }

    //  Are the input and output allocators different?
    BOOL UsingDifferentAllocators() const
    {
        return InputPin()->PeekAllocator() != OutputPin()->PeekAllocator();
    }
}; // CTransInPlaceFilter

#endif /* __TRANSIP__ */

