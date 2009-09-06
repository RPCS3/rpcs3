/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/fontmgrcmn.cpp
// Purpose:     font management for ports that don't have their own
// Author:      Vaclav Slavik
// Created:     2006-11-18
// RCS-ID:      $Id: fontmgrcmn.cpp 54757 2008-07-21 17:34:48Z VZ $
// Copyright:   (c) 2001-2002 SciTech Software, Inc. (www.scitechsoft.com)
//              (c) 2006 REA Elektronik GmbH
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "wx/private/fontmgr.h"

#include "wx/listimpl.cpp"
#include "wx/hashmap.h"

WX_DECLARE_LIST(wxFontInstance, wxFontInstanceList);
WX_DEFINE_LIST(wxFontInstanceList)
WX_DEFINE_LIST(wxFontBundleList)

WX_DECLARE_HASH_MAP(wxString, wxFontBundle*,
                    wxStringHash, wxStringEqual,
                    wxFontBundleHashBase);
// in STL build, hash class is typedef to a template, so it can't be forward
// declared, as we do; solve it by having a dummy class:
class wxFontBundleHash : public wxFontBundleHashBase
{
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxFontFaceBase
// ----------------------------------------------------------------------------

wxFontFaceBase::wxFontFaceBase()
    : m_refCnt(0)
{
    m_instances = new wxFontInstanceList;
    m_instances->DeleteContents(true);
}

wxFontFaceBase::~wxFontFaceBase()
{
    delete m_instances;
}

void wxFontFaceBase::Acquire()
{
    m_refCnt++;
}

void wxFontFaceBase::Release()
{
    if ( --m_refCnt == 0 )
    {
        m_instances->Clear();
    }
}

wxFontInstance *wxFontFaceBase::GetFontInstance(float ptSize, bool aa)
{
    wxASSERT_MSG( m_refCnt > 0, _T("font library not loaded!") );

    for ( wxFontInstanceList::const_iterator i = m_instances->begin();
          i != m_instances->end(); ++i )
    {
        if ( (*i)->GetPointSize() == ptSize && (*i)->IsAntiAliased() == aa )
            return *i;
    }

    wxFontInstance *i = CreateFontInstance(ptSize, aa);
    m_instances->Append(i);
    return i;
}

// ----------------------------------------------------------------------------
// wxFontBundleBase
// ----------------------------------------------------------------------------

wxFontBundleBase::wxFontBundleBase()
{
    for (int i = 0; i < FaceType_Max; i++)
        m_faces[i] = NULL;
}

wxFontBundleBase::~wxFontBundleBase()
{
    for (int i = 0; i < FaceType_Max; i++)
        delete m_faces[i];
}

wxFontFace *wxFontBundleBase::GetFace(FaceType type) const
{
    wxFontFace *f = m_faces[type];

    wxCHECK_MSG( f, NULL, _T("no such face in font bundle") );

    f->Acquire();

    return f;
}

wxFontFace *
wxFontBundleBase::GetFaceForFont(const wxFontMgrFontRefData& font) const
{
    wxASSERT_MSG( font.GetFaceName().empty() || font.GetFaceName() == GetName(),
                  _T("calling GetFaceForFont for incompatible font") );

    int type = FaceType_Regular;

    if ( font.GetWeight() == wxBOLD )
        type |= FaceType_Bold;

    // FIXME -- this should read "if ( font->GetStyle() == wxITALIC )",
    // but since MGL neither DFB supports slant, we try to display it with
    // italic face (better than nothing...)
    if ( font.GetStyle() == wxITALIC || font.GetStyle() == wxSLANT )
    {
        if ( HasFace((FaceType)(type | FaceType_Italic)) )
            type |= FaceType_Italic;
    }

    if ( !HasFace((FaceType)type) )
    {
        for (int i = 0; i < FaceType_Max; i++)
        {
            if ( HasFace((FaceType)i) )
                return GetFace((FaceType)i);
        }

        wxFAIL_MSG( _T("no face") );
        return NULL;
    }

    return GetFace((FaceType)type);
}

// ----------------------------------------------------------------------------
// wxFontsManagerBase
// ----------------------------------------------------------------------------

wxFontsManager *wxFontsManagerBase::ms_instance = NULL;

wxFontsManagerBase::wxFontsManagerBase()
{
    m_hash = new wxFontBundleHash();
    m_list = new wxFontBundleList;
    m_list->DeleteContents(true);
}

wxFontsManagerBase::~wxFontsManagerBase()
{
    delete m_hash;
    delete m_list;
}

/* static */
wxFontsManager *wxFontsManagerBase::Get()
{
    if ( !ms_instance )
        ms_instance = new wxFontsManager();
    return ms_instance;
}

/* static */
void wxFontsManagerBase::CleanUp()
{
    wxDELETE(ms_instance);
}

wxFontBundle *wxFontsManagerBase::GetBundle(const wxString& name) const
{
    return (*m_hash)[name.Lower()];
}

wxFontBundle *
wxFontsManagerBase::GetBundleForFont(const wxFontMgrFontRefData& font) const
{
    wxFontBundle *bundle = NULL;

    wxString facename = font.GetFaceName();
    if ( !facename.empty() )
        bundle = GetBundle(facename);

    if ( !bundle )
    {
        facename = GetDefaultFacename((wxFontFamily)font.GetFamily());
        if ( !facename.empty() )
            bundle = GetBundle(facename);
    }

    if ( !bundle )
    {
       if ( m_list->GetFirst() )
           bundle = m_list->GetFirst()->GetData();
       else
           wxFAIL_MSG(wxT("Fatal error, no fonts available!"));
    }

    return bundle;
}

void wxFontsManagerBase::AddBundle(wxFontBundle *bundle)
{
    (*m_hash)[bundle->GetName().Lower()] = bundle;
    m_list->Append(bundle);
}


// ----------------------------------------------------------------------------
// wxFontMgrFontRefData
// ----------------------------------------------------------------------------

wxFontMgrFontRefData::wxFontMgrFontRefData(int size,
                                           int family,
                                           int style,
                                           int weight,
                                           bool underlined,
                                           const wxString& faceName,
                                           wxFontEncoding encoding)
{
    if ( family == wxDEFAULT )
        family = wxSWISS;
    if ( style == wxDEFAULT )
        style = wxNORMAL;
    if ( weight == wxDEFAULT )
        weight = wxNORMAL;
    if ( size == wxDEFAULT )
        size = 12;

    m_info.family = (wxFontFamily)family;
    m_info.faceName = faceName;
    m_info.style = (wxFontStyle)style;
    m_info.weight = (wxFontWeight)weight;
    m_info.pointSize = size;
    m_info.underlined = underlined;
    m_info.encoding = encoding;

    m_noAA = false;

    m_fontFace = NULL;
    m_fontBundle = NULL;
    m_fontValid = false;
}

wxFontMgrFontRefData::wxFontMgrFontRefData(const wxFontMgrFontRefData& data)
{
    m_info = data.m_info;
    m_noAA = data.m_noAA;

    m_fontFace = data.m_fontFace;
    m_fontBundle = data.m_fontBundle;
    m_fontValid = data.m_fontValid;
    if ( m_fontFace )
        m_fontFace->Acquire();
}

wxFontMgrFontRefData::~wxFontMgrFontRefData()
{
    if ( m_fontFace )
        m_fontFace->Release();
}

wxFontBundle *wxFontMgrFontRefData::GetFontBundle() const
{
    wxConstCast(this, wxFontMgrFontRefData)->EnsureValidFont();
    return m_fontBundle;
}

wxFontInstance *
wxFontMgrFontRefData::GetFontInstance(float scale, bool antialiased) const
{
    wxConstCast(this, wxFontMgrFontRefData)->EnsureValidFont();
    return m_fontFace->GetFontInstance(m_info.pointSize * scale,
                                       antialiased && !m_noAA);
}

void wxFontMgrFontRefData::SetPointSize(int pointSize)
{
    m_info.pointSize = pointSize;
    m_fontValid = false;
}

void wxFontMgrFontRefData::SetFamily(int family)
{
    m_info.family = (wxFontFamily)family;
    m_fontValid = false;
}

void wxFontMgrFontRefData::SetStyle(int style)
{
    m_info.style = (wxFontStyle)style;
    m_fontValid = false;
}

void wxFontMgrFontRefData::SetWeight(int weight)
{
    m_info.weight = (wxFontWeight)weight;
    m_fontValid = false;
}

void wxFontMgrFontRefData::SetFaceName(const wxString& faceName)
{
    m_info.faceName = faceName;
    m_fontValid = false;
}

void wxFontMgrFontRefData::SetUnderlined(bool underlined)
{
    m_info.underlined = underlined;
    m_fontValid = false;
}

void wxFontMgrFontRefData::SetEncoding(wxFontEncoding encoding)
{
    m_info.encoding = encoding;
    m_fontValid = false;
}

void wxFontMgrFontRefData::SetNoAntiAliasing(bool no)
{
    m_noAA = no;
}


void wxFontMgrFontRefData::EnsureValidFont()
{
    if ( !m_fontValid )
    {
        wxFontFace *old = m_fontFace;

        m_fontBundle = wxFontsManager::Get()->GetBundleForFont(*this);
        m_fontFace = m_fontBundle->GetFaceForFont(*this);

        if ( old )
            old->Release();
    }
}
