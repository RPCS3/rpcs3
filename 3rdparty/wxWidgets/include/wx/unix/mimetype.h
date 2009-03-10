/////////////////////////////////////////////////////////////////////////////
// Name:        wx/unix/mimetype.h
// Purpose:     classes and functions to manage MIME types
// Author:      Vadim Zeitlin
// Modified by:
// Created:     23.09.98
// RCS-ID:      $Id: mimetype.h 43723 2006-11-30 13:24:32Z RR $
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence (part of wxExtra library)
/////////////////////////////////////////////////////////////////////////////

#ifndef _MIMETYPE_IMPL_H
#define _MIMETYPE_IMPL_H

#include "wx/mimetype.h"

#if wxUSE_MIMETYPE

class wxMimeTypeCommands;

WX_DEFINE_ARRAY_PTR(wxMimeTypeCommands *, wxMimeCommandsArray);

// this is the real wxMimeTypesManager for Unix
class WXDLLEXPORT wxMimeTypesManagerImpl
{
public:
    // ctor and dtor
    wxMimeTypesManagerImpl();
    virtual ~wxMimeTypesManagerImpl();

    // load all data into memory - done when it is needed for the first time
    void Initialize(int mailcapStyles = wxMAILCAP_ALL,
                    const wxString& extraDir = wxEmptyString);

    // and delete the data here
    void ClearData();

    // implement containing class functions
    wxFileType *GetFileTypeFromExtension(const wxString& ext);
    wxFileType *GetFileTypeFromMimeType(const wxString& mimeType);

    size_t EnumAllFileTypes(wxArrayString& mimetypes);

    bool ReadMailcap(const wxString& filename, bool fallback = FALSE);
    bool ReadMimeTypes(const wxString& filename);

    void AddFallback(const wxFileTypeInfo& filetype);

    // add information about the given mimetype
    void AddMimeTypeInfo(const wxString& mimetype,
                         const wxString& extensions,
                         const wxString& description);
    void AddMailcapInfo(const wxString& strType,
                        const wxString& strOpenCmd,
                        const wxString& strPrintCmd,
                        const wxString& strTest,
                        const wxString& strDesc);

    // add a new record to the user .mailcap/.mime.types files
    wxFileType *Associate(const wxFileTypeInfo& ftInfo);
    // remove association
    bool Unassociate(wxFileType *ft);

    // accessors
        // get the string containing space separated extensions for the given
        // file type
    wxString GetExtension(size_t index) { return m_aExtensions[index]; }

protected:
    void InitIfNeeded();

    wxArrayString m_aTypes,         // MIME types
                  m_aDescriptions,  // descriptions (just some text)
                  m_aExtensions,    // space separated list of extensions
                  m_aIcons;         // Icon filenames

    // verb=command pairs for this file type
    wxMimeCommandsArray m_aEntries;

    // are we initialized?
    bool m_initialized;

    // keep track of the files we had already loaded (this is a bitwise OR of
    // wxMailcapStyle values)
    int m_mailcapStylesInited;

    wxString GetCommand(const wxString &verb, size_t nIndex) const;

    // read Gnome files
    void LoadGnomeDataFromKeyFile(const wxString& filename,
                                  const wxArrayString& dirs);
    void LoadGnomeMimeTypesFromMimeFile(const wxString& filename);
    void LoadGnomeMimeFilesFromDir(const wxString& dirbase,
                                   const wxArrayString& dirs);
    void GetGnomeMimeInfo(const wxString& sExtraDir);

    // read KDE
    void LoadKDELinksForMimeSubtype(const wxString& dirbase,
                                    const wxString& subdir,
                                    const wxString& filename,
                                    const wxArrayString& icondirs);
    void LoadKDELinksForMimeType(const wxString& dirbase,
                                 const wxString& subdir,
                                 const wxArrayString& icondirs);
    void LoadKDELinkFilesFromDir(const wxString& dirbase,
                                 const wxArrayString& icondirs);
    void LoadKDEApp(const wxString& filename);
    void LoadKDEAppsFilesFromDir(const wxString& dirname);
    void GetKDEMimeInfo(const wxString& sExtraDir);

    // write KDE
    bool WriteKDEMimeFile(int index, bool delete_index);
    bool CheckKDEDirsExist(const wxString & sOK, const wxString& sTest);

    //read write Netscape and MetaMail formats
    void GetMimeInfo (const wxString& sExtraDir);
    bool WriteToMailCap (int index, bool delete_index);
    bool WriteToMimeTypes (int index, bool delete_index);
    bool WriteToNSMimeTypes (int index, bool delete_index);

    // ReadMailcap() helper
    bool ProcessOtherMailcapField(struct MailcapLineData& data,
                                  const wxString& curField);

    // functions used to do associations

    virtual int AddToMimeData(const wxString& strType,
                      const wxString& strIcon,
                      wxMimeTypeCommands *entry,
                      const wxArrayString& strExtensions,
                      const wxString& strDesc,
                      bool replaceExisting = TRUE);

    virtual bool DoAssociation(const wxString& strType,
                       const wxString& strIcon,
                       wxMimeTypeCommands *entry,
                       const wxArrayString& strExtensions,
                       const wxString& strDesc);

    virtual bool WriteMimeInfo(int nIndex, bool delete_mime );

    // give it access to m_aXXX variables
    friend class WXDLLEXPORT wxFileTypeImpl;
};



class WXDLLEXPORT wxFileTypeImpl
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

    // accessors
    bool GetExtensions(wxArrayString& extensions);
    bool GetMimeType(wxString *mimeType) const
        { *mimeType = m_manager->m_aTypes[m_index[0]]; return TRUE; }
    bool GetMimeTypes(wxArrayString& mimeTypes) const;
    bool GetIcon(wxIconLocation *iconLoc) const;

    bool GetDescription(wxString *desc) const
        { *desc = m_manager->m_aDescriptions[m_index[0]]; return TRUE; }

    bool GetOpenCommand(wxString *openCmd,
                        const wxFileType::MessageParameters& params) const
    {
        *openCmd = GetExpandedCommand(wxT("open"), params);
        return (! openCmd -> IsEmpty() );
    }

    bool GetPrintCommand(wxString *printCmd,
                         const wxFileType::MessageParameters& params) const
    {
        *printCmd = GetExpandedCommand(wxT("print"), params);
        return (! printCmd -> IsEmpty() );
    }

        // return the number of commands defined for this file type, 0 if none
    size_t GetAllCommands(wxArrayString *verbs, wxArrayString *commands,
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
    wxString
    GetExpandedCommand(const wxString & verb,
                       const wxFileType::MessageParameters& params) const;

    wxMimeTypesManagerImpl *m_manager;
    wxArrayInt              m_index; // in the wxMimeTypesManagerImpl arrays
};

#endif // wxUSE_MIMETYPE

#endif // _MIMETYPE_IMPL_H


