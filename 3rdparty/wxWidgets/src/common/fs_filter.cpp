/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/fs_filter.cpp
// Purpose:     wxFilter file system handler
// Author:      Mike Wetherell
// Copyright:   (c) 2006 Mike Wetherell
// CVS-ID:      $Id: fs_filter.cpp 42514 2006-10-27 10:47:13Z MW $
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_FILESYSTEM

#include "wx/fs_filter.h"

#ifndef WXPRECOMP
#endif

#include "wx/ptr_scpd.h"

wxDEFINE_SCOPED_PTR_TYPE(wxFSFile)
wxDEFINE_SCOPED_PTR_TYPE(wxInputStream)

//----------------------------------------------------------------------------
// wxFilterFSHandler
//----------------------------------------------------------------------------

bool wxFilterFSHandler::CanOpen(const wxString& location)
{
    return wxFilterClassFactory::Find(GetProtocol(location)) != NULL;
}

wxFSFile* wxFilterFSHandler::OpenFile(
        wxFileSystem& fs,
        const wxString& location)
{
    wxString right = GetRightLocation(location);
    if (!right.empty())
        return NULL;

    wxString protocol = GetProtocol(location);
    const wxFilterClassFactory *factory = wxFilterClassFactory::Find(protocol);
    if (!factory)
        return NULL;

    wxString left = GetLeftLocation(location);
    wxFSFilePtr leftFile(fs.OpenFile(left));
    if (!leftFile.get())
        return NULL;

    wxInputStreamPtr leftStream(leftFile->DetachStream());
    if (!leftStream.get() || !leftStream->IsOk())
        return NULL;

    wxInputStreamPtr stream(factory->NewStream(leftStream.release()));

    // The way compressed streams are supposed to be served is e.g.:
    //  Content-type: application/postscript
    //  Content-encoding: gzip
    // So the mime type should be just the mime type of the lhs. However check
    // whether the mime type is that of this compression format (e.g.
    // application/gzip). If so pop any extension and try GetMimeTypeFromExt,
    // e.g. if it were '.ps.gz' pop the '.gz' and try looking up '.ps'
    wxString mime = leftFile->GetMimeType();
    if (factory->CanHandle(mime, wxSTREAM_MIMETYPE))
        mime = GetMimeTypeFromExt(factory->PopExtension(left));

    return new wxFSFile(stream.release(),
                        left + wxT("#") + protocol + wxT(":") + right,
                        mime,
                        GetAnchor(location)
#if wxUSE_DATETIME
                        , leftFile->GetModificationTime()
#endif // wxUSE_DATETIME
                       );
}

wxString wxFilterFSHandler::FindFirst(const wxString& WXUNUSED(spec), int WXUNUSED(flags))
{
    return wxEmptyString;
}

wxString wxFilterFSHandler::FindNext()
{
    return wxEmptyString;
}

#endif //wxUSE_FILESYSTEM
