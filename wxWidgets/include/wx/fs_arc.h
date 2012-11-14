/////////////////////////////////////////////////////////////////////////////
// Name:        wx/fs_arc.h
// Purpose:     Archive file system
// Author:      Vaclav Slavik, Mike Wetherell
// Copyright:   (c) 1999 Vaclav Slavik, (c) 2006 Mike Wetherell
// CVS-ID:      $Id: fs_arc.h 42713 2006-10-30 11:56:12Z ABX $
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_FS_ARC_H_
#define _WX_FS_ARC_H_

#include "wx/defs.h"

#if wxUSE_FS_ARCHIVE

#include "wx/filesys.h"
#include "wx/hashmap.h"

WX_DECLARE_STRING_HASH_MAP(int, wxArchiveFilenameHashMap);

//---------------------------------------------------------------------------
// wxArchiveFSHandler
//---------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxArchiveFSHandler : public wxFileSystemHandler
{
public:
    wxArchiveFSHandler();
    virtual bool CanOpen(const wxString& location);
    virtual wxFSFile* OpenFile(wxFileSystem& fs, const wxString& location);
    virtual wxString FindFirst(const wxString& spec, int flags = 0);
    virtual wxString FindNext();
    void Cleanup();
    virtual ~wxArchiveFSHandler();

private:
    class wxArchiveFSCache *m_cache;
    wxFileSystem m_fs;

    // these vars are used by FindFirst/Next:
    class wxArchiveFSCacheData *m_Archive;
    struct wxArchiveFSEntry *m_FindEntry;
    wxString m_Pattern, m_BaseDir, m_ZipFile;
    bool m_AllowDirs, m_AllowFiles;
    wxArchiveFilenameHashMap *m_DirsFound;

    wxString DoFind();

    DECLARE_NO_COPY_CLASS(wxArchiveFSHandler)
    DECLARE_DYNAMIC_CLASS(wxArchiveFSHandler)
};

#endif // wxUSE_FS_ARCHIVE

#endif // _WX_FS_ARC_H_
