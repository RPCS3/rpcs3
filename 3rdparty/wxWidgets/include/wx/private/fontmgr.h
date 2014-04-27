/////////////////////////////////////////////////////////////////////////////
// Name:        wx/private/fontmgr.h
// Purpose:     font management for ports that don't have their own
// Author:      Vaclav Slavik
// Created:     2006-11-18
// RCS-ID:      $Id: fontmgr.h 43855 2006-12-07 08:57:44Z PC $
// Copyright:   (c) 2001-2002 SciTech Software, Inc. (www.scitechsoft.com)
//              (c) 2006 REA Elektronik GmbH
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_PRIVATE_FONTMGR_H_
#define _WX_PRIVATE_FONTMGR_H_

#include "wx/list.h"
#include "wx/fontutil.h"

class wxFontsManager;
class wxFontInstance;
class wxFontInstanceList;
class wxFontFace;
class wxFontBundle;
class wxFontBundleHash;
class wxFontMgrFontRefData;

WX_DECLARE_LIST(wxFontBundle, wxFontBundleList);

/**
    This class represents single font face with set parameters (point size,
    antialiasing).
 */
class wxFontInstanceBase
{
protected:
    wxFontInstanceBase(float ptSize, bool aa) : m_ptSize(ptSize), m_aa(aa) {}
    virtual ~wxFontInstanceBase() {}

public:
    float GetPointSize() const { return m_ptSize; }
    bool IsAntiAliased() const { return m_aa; }

protected:
    float m_ptSize;
    bool m_aa;
};


/// This class represents loaded font face (bundle+weight+italics).
class wxFontFaceBase
{
protected:
    /// Ctor. Creates object with reference count = 0, Acquire() must be
    /// called after the object is created.
    wxFontFaceBase();
    virtual ~wxFontFaceBase();

public:
    /// Increases reference count of the face
    virtual void Acquire();

    /**
        Decreases reference count of the face. Call this when you no longer
        use the object returned by wxFontBundle. Note that this doesn't destroy
        the object, but only optionally shuts it down, so it's possible to
        call Acquire() and Release() more than once.
     */
    virtual void Release();

    /**
        Returns instance of the font at given size.

        @param ptSize   point size of the font to create; note that this is
                        a float and not integer, it should be wxFont's point
                        size multipled by wxDC's scale factor
        @param aa       should the font be antialiased?
     */
    virtual wxFontInstance *GetFontInstance(float ptSize, bool aa);

protected:
    /// Called to create a new instance of the font by GetFontInstance() if
    /// it wasn't found it cache.
    virtual wxFontInstance *CreateFontInstance(float ptSize, bool aa) = 0;

protected:
    unsigned m_refCnt;
    wxFontInstanceList *m_instances;
};

/**
    This class represents font bundle. Font bundle is set of faces that have
    the same name, but differ in weight and italics.
 */
class wxFontBundleBase
{
public:
    wxFontBundleBase();
    virtual ~wxFontBundleBase();

    /// Returns name of the bundle
    virtual wxString GetName() const = 0;

    /// Returns true if the font is fixe-width
    virtual bool IsFixed() const = 0;

    /// Type of faces in the bundle
    enum FaceType
    {
        // NB: values of these constants are set so that it's possible to
        //     make OR-combinations of them and still get valid enum element
        FaceType_Regular       = 0,
        FaceType_Italic        = 1,
        FaceType_Bold          = 2,
        FaceType_BoldItalic    = FaceType_Italic | FaceType_Bold,

        FaceType_Max
    };

    /// Returns true if the given face is available
    bool HasFace(FaceType type) const { return m_faces[type] != NULL; }

    /**
        Returns font face object that can be used to render font of given type.

        Note that this method can only be called if HasFace(type) returns true.

        Acquire() was called on the returned object, you must call Release()
        when you stop using it.
     */
    wxFontFace *GetFace(FaceType type) const;

    /**
        Returns font face object that can be used to render given font.

        Acquire() was called on the returned object, you must call Release()
        when you stop using it.
     */
    wxFontFace *GetFaceForFont(const wxFontMgrFontRefData& font) const;

protected:
    wxFontFace *m_faces[FaceType_Max];
};


/**
    Base class for wxFontsManager class, which manages the list of all
    available fonts and their loaded instances.
 */
class wxFontsManagerBase
{
protected:
    wxFontsManagerBase();
    virtual ~wxFontsManagerBase();

public:
    /// Returns the font manager singleton, creating it if it doesn't exist
    static wxFontsManager *Get();

    /// Called by wxApp to shut down the manager
    static void CleanUp();

    /// Returns list of all available font bundles
    const wxFontBundleList& GetBundles() const { return *m_list; }

    /**
        Returns object representing font bundle with the given name.

        The returned object is owned by wxFontsManager, you must not delete it.
     */
    wxFontBundle *GetBundle(const wxString& name) const;

    /**
        Returns object representing font bundle that can be used to render
        given font.

        The returned object is owned by wxFontsManager, you must not delete it.
     */
    wxFontBundle *GetBundleForFont(const wxFontMgrFontRefData& font) const;

    /// This method must be called by derived
    void AddBundle(wxFontBundle *bundle);

    /// Returns default facename for given wxFont family
    virtual wxString GetDefaultFacename(wxFontFamily family) const = 0;

private:
    wxFontBundleHash *m_hash;
    wxFontBundleList *m_list;

protected:
    static wxFontsManager *ms_instance;
};



#if defined(__WXMGL__)
    #include "wx/mgl/private/fontmgr.h"
#elif defined(__WXDFB__)
    #include "wx/dfb/private/fontmgr.h"
#endif



/// wxFontMgrFontRefData implementation using wxFontsManager classes
class wxFontMgrFontRefData : public wxObjectRefData
{
public:
    wxFontMgrFontRefData(int size = wxDEFAULT,
                  int family = wxDEFAULT,
                  int style = wxDEFAULT,
                  int weight = wxDEFAULT,
                  bool underlined = false,
                  const wxString& faceName = wxEmptyString,
                  wxFontEncoding encoding = wxFONTENCODING_DEFAULT);
    wxFontMgrFontRefData(const wxFontMgrFontRefData& data);
    ~wxFontMgrFontRefData();

    wxFontBundle *GetFontBundle() const;
    wxFontInstance *GetFontInstance(float scale, bool antialiased) const;

    bool IsFixedWidth() const { return GetFontBundle()->IsFixed(); }

    const wxNativeFontInfo *GetNativeFontInfo() const { return &m_info; }

    int GetPointSize() const { return m_info.pointSize; }
    wxString GetFaceName() const { return m_info.faceName; }
    int GetFamily() const { return m_info.family; }
    int GetStyle() const { return m_info.style; }
    int GetWeight() const { return m_info.weight; }
    bool GetUnderlined() const { return m_info.underlined; }
    wxFontEncoding GetEncoding() const { return m_info.encoding; }

    void SetPointSize(int pointSize);
    void SetFamily(int family);
    void SetStyle(int style);
    void SetWeight(int weight);
    void SetFaceName(const wxString& faceName);
    void SetUnderlined(bool underlined);
    void SetEncoding(wxFontEncoding encoding);

    // Unofficial API, don't use
    void SetNoAntiAliasing(bool no);
    bool GetNoAntiAliasing() const { return m_noAA; }

private:
    void EnsureValidFont();

    wxNativeFontInfo  m_info;
    bool              m_noAA;

    wxFontFace       *m_fontFace;
    wxFontBundle     *m_fontBundle;
    bool              m_fontValid;
};

#endif // _WX_PRIVATE_FONTMGR_H_
