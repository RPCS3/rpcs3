/////////////////////////////////////////////////////////////////////////////
// Name:        fs_mem.h
// Purpose:     in-memory file system
// Author:      Vaclav Slavik
// Copyright:   (c) 2000 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_FS_MEM_H_
#define _WX_FS_MEM_H_

#include "wx/defs.h"

#if wxUSE_FILESYSTEM

#include "wx/filesys.h"

#if wxUSE_GUI
    class WXDLLIMPEXP_FWD_CORE wxBitmap;
    class WXDLLIMPEXP_FWD_CORE wxImage;
#endif // wxUSE_GUI

// ----------------------------------------------------------------------------
// wxMemoryFSHandlerBase
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxMemoryFSHandlerBase : public wxFileSystemHandler
{
public:
    wxMemoryFSHandlerBase();
    virtual ~wxMemoryFSHandlerBase();

    // Add file to list of files stored in memory. Stored data (bitmap, text or
    // raw data) will be copied into private memory stream and available under
    // name "memory:" + filename
    static void AddFile(const wxString& filename, const wxString& textdata);
    static void AddFile(const wxString& filename, const void *binarydata, size_t size);
#if wxABI_VERSION >= 20805
    static void AddFileWithMimeType(const wxString& filename,
                                    const wxString& textdata,
                                    const wxString& mimetype);
    static void AddFileWithMimeType(const wxString& filename,
                                    const void *binarydata, size_t size,
                                    const wxString& mimetype);
#endif // wxABI_VERSION >= 20805

    // Remove file from memory FS and free occupied memory
    static void RemoveFile(const wxString& filename);

    virtual bool CanOpen(const wxString& location);
    virtual wxFSFile* OpenFile(wxFileSystem& fs, const wxString& location);
    virtual wxString FindFirst(const wxString& spec, int flags = 0);
    virtual wxString FindNext();

protected:
    static bool CheckHash(const wxString& filename);
    static wxHashTable *m_Hash;
};

// ----------------------------------------------------------------------------
// wxMemoryFSHandler
// ----------------------------------------------------------------------------

#if wxUSE_GUI

// add GUI-only operations to the base class
class WXDLLIMPEXP_CORE wxMemoryFSHandler : public wxMemoryFSHandlerBase
{
public:
    // bring the base class versions into the scope, otherwise they would be
    // inaccessible in wxMemoryFSHandler
    // (unfortunately "using" can't be used as gcc 2.95 doesn't have it...)
    static void AddFile(const wxString& filename, const wxString& textdata)
    {
        wxMemoryFSHandlerBase::AddFile(filename, textdata);
    }

    static void AddFile(const wxString& filename,
                        const void *binarydata,
                        size_t size)
    {
        wxMemoryFSHandlerBase::AddFile(filename, binarydata, size);
    }
#if wxABI_VERSION >= 20805
    static void AddFileWithMimeType(const wxString& filename,
                                    const wxString& textdata,
                                    const wxString& mimetype)
    {
        wxMemoryFSHandlerBase::AddFileWithMimeType(filename,
                                                   textdata,
                                                   mimetype);
    }
    static void AddFileWithMimeType(const wxString& filename,
                                    const void *binarydata, size_t size,
                                    const wxString& mimetype)
    {
        wxMemoryFSHandlerBase::AddFileWithMimeType(filename,
                                                   binarydata, size,
                                                   mimetype);
    }
#endif // wxABI_VERSION >= 20805

#if wxUSE_IMAGE
    static void AddFile(const wxString& filename,
                        const wxImage& image,
                        long type);

    static void AddFile(const wxString& filename,
                        const wxBitmap& bitmap,
                        long type);
#endif // wxUSE_IMAGE

};

#else // !wxUSE_GUI

// just the same thing as the base class in wxBase
class WXDLLIMPEXP_BASE wxMemoryFSHandler : public wxMemoryFSHandlerBase
{
};

#endif // wxUSE_GUI/!wxUSE_GUI

#endif // wxUSE_FILESYSTEM

#endif // _WX_FS_MEM_H_

