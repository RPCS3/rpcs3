/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msdos/mimetype.h
// Purpose:     classes and functions to manage MIME types
// Author:      Vadim Zeitlin
// Modified by:
// Created:     23.09.98
// RCS-ID:      $Id: mimetype.h 39763 2006-06-16 09:31:12Z ABX $
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence (part of wxExtra library)
/////////////////////////////////////////////////////////////////////////////

#ifndef _MIMETYPE_IMPL_H
#define _MIMETYPE_IMPL_H

#include "wx/defs.h"
#include "wx/mimetype.h"


class wxMimeTypesManagerImpl
{
public :
    wxMimeTypesManagerImpl() { }

    // load all data into memory - done when it is needed for the first time
    void Initialize(int mailcapStyles = wxMAILCAP_STANDARD,
                    const wxString& extraDir = wxEmptyString);

    // and delete the data here
    void ClearData();

    // implement containing class functions
    wxFileType *GetFileTypeFromExtension(const wxString& ext);
    wxFileType *GetOrAllocateFileTypeFromExtension(const wxString& ext) ;
    wxFileType *GetFileTypeFromMimeType(const wxString& mimeType);

    size_t EnumAllFileTypes(wxArrayString& mimetypes);

    // this are NOPs under MacOS
    bool ReadMailcap(const wxString& WXUNUSED(filename), bool WXUNUSED(fallback) = TRUE) { return TRUE; }
    bool ReadMimeTypes(const wxString& WXUNUSED(filename)) { return TRUE; }

    void AddFallback(const wxFileTypeInfo& ft) { m_fallbacks.Add(ft); }

    // create a new filetype association
    wxFileType *Associate(const wxFileTypeInfo& ftInfo);
    // remove association
    bool Unassociate(wxFileType *ft);

    // create a new filetype with the given name and extension
    wxFileType *CreateFileType(const wxString& filetype, const wxString& ext);

private:
    wxArrayFileTypeInfo m_fallbacks;
};

class wxFileTypeImpl
{
public:
    // initialization functions
    // this is used to construct a list of mimetypes which match;
    // if built with GetFileTypeFromMimetype index 0 has the exact match and
    // index 1 the type / * match
    // if built with GetFileTypeFromExtension, index 0 has the mimetype for
    // the first extension found, index 1 for the second and so on

    void Init(wxMimeTypesManagerImpl *manager, size_t index)
    { m_manager = manager; m_index.Add(index); }

    // initialize us with our file type name
    void SetFileType(const wxString& strFileType)
        { m_strFileType = strFileType; }
    void SetExt(const wxString& ext)
        { m_ext = ext; }

    // implement accessor functions
    bool GetExtensions(wxArrayString& extensions);
    bool GetMimeType(wxString *mimeType) const;
    bool GetMimeTypes(wxArrayString& mimeTypes) const;
    bool GetIcon(wxIconLocation *iconLoc) const;
    bool GetDescription(wxString *desc) const;
    bool GetOpenCommand(wxString *openCmd,
                        const wxFileType::MessageParameters&) const
        { return GetCommand(openCmd, "open"); }
    bool GetPrintCommand(wxString *printCmd,
                         const wxFileType::MessageParameters&) const
        { return GetCommand(printCmd, "print"); }

    size_t GetAllCommands(wxArrayString * verbs, wxArrayString * commands,
                          const wxFileType::MessageParameters& params) const;

    // remove the record for this file type
    // probably a mistake to come here, use wxMimeTypesManager.Unassociate (ft) instead
    bool Unassociate(wxFileType *ft)
    {
        return m_manager->Unassociate(ft);
    }

    // set an arbitrary command, ask confirmation if it already exists and
    // overwriteprompt is TRUE
    bool SetCommand(const wxString& cmd, const wxString& verb, bool overwriteprompt = TRUE);
    bool SetDefaultIcon(const wxString& strIcon = wxEmptyString, int index = 0);

 private:
    // helper function
    bool GetCommand(wxString *command, const char *verb) const;

    wxMimeTypesManagerImpl *m_manager;
    wxArrayInt              m_index; // in the wxMimeTypesManagerImpl arrays
    wxString m_strFileType, m_ext;
};

#endif
  //_MIMETYPE_H

/* vi: set cin tw=80 ts=4 sw=4: */
