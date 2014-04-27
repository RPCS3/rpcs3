/////////////////////////////////////////////////////////////////////////////
// Name:        fs_arc.cpp
// Purpose:     wxArchive file system
// Author:      Vaclav Slavik, Mike Wetherell
// Copyright:   (c) 1999 Vaclav Slavik, (c) 2006 Mike Wetherell
// CVS-ID:      $Id: fs_arc.cpp 51495 2008-02-01 16:44:56Z MW $
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#if wxUSE_FS_ARCHIVE

#include "wx/fs_arc.h"

#ifndef WXPRECOMP
    #include "wx/intl.h"
    #include "wx/log.h"
#endif

#if WXWIN_COMPATIBILITY_2_6 && wxUSE_ZIPSTREAM
    #include "wx/zipstrm.h"
#else
    #include "wx/archive.h"
#endif

#include "wx/private/fileback.h"

//---------------------------------------------------------------------------
// wxArchiveFSCacheDataImpl
//
// Holds the catalog of an archive file, and if it is being read from a
// non-seekable stream, a copy of its backing file.
//
// This class is actually the reference counted implementation for the
// wxArchiveFSCacheData class below. It was done that way to allow sharing
// between instances of wxFileSystem, though that's a feature not used in this
// version.
//---------------------------------------------------------------------------

WX_DECLARE_STRING_HASH_MAP(wxArchiveEntry*, wxArchiveFSEntryHash);

struct wxArchiveFSEntry
{
    wxArchiveEntry *entry;
    wxArchiveFSEntry *next;
};

class wxArchiveFSCacheDataImpl
{
public:
    wxArchiveFSCacheDataImpl(const wxArchiveClassFactory& factory,
                             const wxBackingFile& backer);
    wxArchiveFSCacheDataImpl(const wxArchiveClassFactory& factory,
                             wxInputStream *stream);

    ~wxArchiveFSCacheDataImpl();

    void Release() { if (--m_refcount == 0) delete this; }
    wxArchiveFSCacheDataImpl *AddRef() { m_refcount++; return this; }

    wxArchiveEntry *Get(const wxString& name);
    wxInputStream *NewStream() const;

    wxArchiveFSEntry *GetNext(wxArchiveFSEntry *fse);

private:
    wxArchiveFSEntry *AddToCache(wxArchiveEntry *entry);
    void CloseStreams();

    int m_refcount;

    wxArchiveFSEntryHash m_hash;
    wxArchiveFSEntry *m_begin;
    wxArchiveFSEntry **m_endptr;

    wxBackingFile m_backer;
    wxInputStream *m_stream;
    wxArchiveInputStream *m_archive;
};

wxArchiveFSCacheDataImpl::wxArchiveFSCacheDataImpl(
        const wxArchiveClassFactory& factory,
        const wxBackingFile& backer)
 :  m_refcount(1),
    m_begin(NULL),
    m_endptr(&m_begin),
    m_backer(backer),
    m_stream(new wxBackedInputStream(backer)),
    m_archive(factory.NewStream(*m_stream))
{
}

wxArchiveFSCacheDataImpl::wxArchiveFSCacheDataImpl(
        const wxArchiveClassFactory& factory,
        wxInputStream *stream)
 :  m_refcount(1),
    m_begin(NULL),
    m_endptr(&m_begin),
    m_stream(stream),
    m_archive(factory.NewStream(*m_stream))
{
}

wxArchiveFSCacheDataImpl::~wxArchiveFSCacheDataImpl()
{
    WX_CLEAR_HASH_MAP(wxArchiveFSEntryHash, m_hash);

    wxArchiveFSEntry *entry = m_begin;

    while (entry)
    {
        wxArchiveFSEntry *next = entry->next;
        delete entry;
        entry = next;
    }

    CloseStreams();
}

wxArchiveFSEntry *wxArchiveFSCacheDataImpl::AddToCache(wxArchiveEntry *entry)
{
    m_hash[entry->GetName(wxPATH_UNIX)] = entry;
    wxArchiveFSEntry *fse = new wxArchiveFSEntry;
    *m_endptr = fse;
    (*m_endptr)->entry = entry;
    (*m_endptr)->next = NULL;
    m_endptr = &(*m_endptr)->next;
    return fse;
}

void wxArchiveFSCacheDataImpl::CloseStreams()
{
    delete m_archive;
    m_archive = NULL;
    delete m_stream;
    m_stream = NULL;
}

wxArchiveEntry *wxArchiveFSCacheDataImpl::Get(const wxString& name)
{
    wxArchiveFSEntryHash::iterator it = m_hash.find(name);

    if (it != m_hash.end())
        return it->second;

    if (!m_archive)
        return NULL;

    wxArchiveEntry *entry;

    while ((entry = m_archive->GetNextEntry()) != NULL)
    {
        AddToCache(entry);

        if (entry->GetName(wxPATH_UNIX) == name)
            return entry;
    }

    CloseStreams();

    return NULL;
}

wxInputStream* wxArchiveFSCacheDataImpl::NewStream() const
{
    if (m_backer)
        return new wxBackedInputStream(m_backer);
    else
        return NULL;
}

wxArchiveFSEntry *wxArchiveFSCacheDataImpl::GetNext(wxArchiveFSEntry *fse)
{
    wxArchiveFSEntry *next = fse ? fse->next : m_begin;

    if (!next && m_archive)
    {
        wxArchiveEntry *entry = m_archive->GetNextEntry();

        if (entry)
            next = AddToCache(entry);
        else
            CloseStreams();
    }

    return next;
}

//---------------------------------------------------------------------------
// wxArchiveFSCacheData
//
// This is the inteface for wxArchiveFSCacheDataImpl above. Holds the catalog
// of an archive file, and if it is being read from a non-seekable stream, a
// copy of its backing file.
//---------------------------------------------------------------------------

class wxArchiveFSCacheData
{
public:
    wxArchiveFSCacheData() : m_impl(NULL) { }
    wxArchiveFSCacheData(const wxArchiveClassFactory& factory,
                         const wxBackingFile& backer);
    wxArchiveFSCacheData(const wxArchiveClassFactory& factory,
                         wxInputStream *stream);

    wxArchiveFSCacheData(const wxArchiveFSCacheData& data);
    wxArchiveFSCacheData& operator=(const wxArchiveFSCacheData& data);

    ~wxArchiveFSCacheData() { if (m_impl) m_impl->Release(); }

    wxArchiveEntry *Get(const wxString& name) { return m_impl->Get(name); }
    wxInputStream *NewStream() const { return m_impl->NewStream(); }
    wxArchiveFSEntry *GetNext(wxArchiveFSEntry *fse)
        { return m_impl->GetNext(fse); }

private:
    wxArchiveFSCacheDataImpl *m_impl;
};

wxArchiveFSCacheData::wxArchiveFSCacheData(
        const wxArchiveClassFactory& factory,
        const wxBackingFile& backer)
  : m_impl(new wxArchiveFSCacheDataImpl(factory, backer))
{
}

wxArchiveFSCacheData::wxArchiveFSCacheData(
        const wxArchiveClassFactory& factory,
        wxInputStream *stream)
  : m_impl(new wxArchiveFSCacheDataImpl(factory, stream))
{
}

wxArchiveFSCacheData::wxArchiveFSCacheData(const wxArchiveFSCacheData& data)
  : m_impl(data.m_impl ? data.m_impl->AddRef() : NULL)
{
}

wxArchiveFSCacheData& wxArchiveFSCacheData::operator=(
        const wxArchiveFSCacheData& data)
{
    if (data.m_impl != m_impl)
    {
        if (m_impl)
            m_impl->Release();

        m_impl = data.m_impl;

        if (m_impl)
            m_impl->AddRef();
    }

    return *this;
}

//---------------------------------------------------------------------------
// wxArchiveFSCache
//
// wxArchiveFSCacheData caches a single archive, and this class holds a
// collection of them to cache all the archives accessed by this instance
// of wxFileSystem.
//---------------------------------------------------------------------------

WX_DECLARE_STRING_HASH_MAP(wxArchiveFSCacheData, wxArchiveFSCacheDataHash);

class wxArchiveFSCache
{
public:
    wxArchiveFSCache() { }
    ~wxArchiveFSCache() { }

    wxArchiveFSCacheData* Add(const wxString& name,
                              const wxArchiveClassFactory& factory,
                              wxInputStream *stream);

    wxArchiveFSCacheData *Get(const wxString& name);

private:
    wxArchiveFSCacheDataHash m_hash;
};

wxArchiveFSCacheData* wxArchiveFSCache::Add(
        const wxString& name,
        const wxArchiveClassFactory& factory,
        wxInputStream *stream)
{
    wxArchiveFSCacheData& data = m_hash[name];

    if (stream->IsSeekable())
        data = wxArchiveFSCacheData(factory, stream);
    else
        data = wxArchiveFSCacheData(factory, wxBackingFile(stream));

    return &data;
}

wxArchiveFSCacheData *wxArchiveFSCache::Get(const wxString& name)
{
    wxArchiveFSCacheDataHash::iterator it;

    if ((it = m_hash.find(name)) != m_hash.end())
        return &it->second;

    return NULL;
}

//----------------------------------------------------------------------------
// wxArchiveFSHandler
//----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxArchiveFSHandler, wxFileSystemHandler)

wxArchiveFSHandler::wxArchiveFSHandler()
 :  wxFileSystemHandler()
{
    m_Archive = NULL;
    m_FindEntry = NULL;
    m_ZipFile = m_Pattern = m_BaseDir = wxEmptyString;
    m_AllowDirs = m_AllowFiles = true;
    m_DirsFound = NULL;
    m_cache = NULL;
}

wxArchiveFSHandler::~wxArchiveFSHandler()
{
    Cleanup();
    delete m_cache;
}

void wxArchiveFSHandler::Cleanup()
{
    wxDELETE(m_DirsFound);
}

bool wxArchiveFSHandler::CanOpen(const wxString& location)
{
    wxString p = GetProtocol(location);
    return wxArchiveClassFactory::Find(p) != NULL;
}

wxFSFile* wxArchiveFSHandler::OpenFile(
        wxFileSystem& WXUNUSED(fs),
        const wxString& location)
{
    wxString right = GetRightLocation(location);
    wxString left = GetLeftLocation(location);
    wxString protocol = GetProtocol(location);
    wxString key = left + wxT("#") + protocol + wxT(":");

    if (right.Contains(wxT("./")))
    {
        if (right.GetChar(0) != wxT('/')) right = wxT('/') + right;
        wxFileName rightPart(right, wxPATH_UNIX);
        rightPart.Normalize(wxPATH_NORM_DOTS, wxT("/"), wxPATH_UNIX);
        right = rightPart.GetFullPath(wxPATH_UNIX);
    }

    if (right.GetChar(0) == wxT('/')) right = right.Mid(1);

    if (!m_cache)
        m_cache = new wxArchiveFSCache;

    const wxArchiveClassFactory *factory;
    factory = wxArchiveClassFactory::Find(protocol);
    if (!factory)
        return NULL;

    wxArchiveFSCacheData *cached = m_cache->Get(key);
    if (!cached)
    {
        wxFSFile *leftFile = m_fs.OpenFile(left);
        if (!leftFile)
            return NULL;
        cached = m_cache->Add(key, *factory, leftFile->DetachStream());
        delete leftFile;
    }

    wxArchiveEntry *entry = cached->Get(right);
    if (!entry)
        return NULL;

    wxInputStream *leftStream = cached->NewStream();
    if (!leftStream)
    {
        wxFSFile *leftFile = m_fs.OpenFile(left);
        if (!leftFile)
            return NULL;
        leftStream = leftFile->DetachStream();
        delete leftFile;
    }

    wxArchiveInputStream *s = factory->NewStream(leftStream);
    s->OpenEntry(*entry);

    if (s && s->IsOk())
    {
#if WXWIN_COMPATIBILITY_2_6 && wxUSE_ZIPSTREAM
        if (factory->IsKindOf(CLASSINFO(wxZipClassFactory)))
            ((wxZipInputStream*)s)->m_allowSeeking = true;
#endif // WXWIN_COMPATIBILITY_2_6

        return new wxFSFile(s,
                            key + right,
                            GetMimeTypeFromExt(location),
                            GetAnchor(location)
#if wxUSE_DATETIME
                            , entry->GetDateTime()
#endif // wxUSE_DATETIME
                            );
    }

    delete s;
    return NULL;
}

wxString wxArchiveFSHandler::FindFirst(const wxString& spec, int flags)
{
    wxString right = GetRightLocation(spec);
    wxString left = GetLeftLocation(spec);
    wxString protocol = GetProtocol(spec);
    wxString key = left + wxT("#") + protocol + wxT(":");

    if (!right.empty() && right.Last() == wxT('/')) right.RemoveLast();

    if (!m_cache)
        m_cache = new wxArchiveFSCache;

    const wxArchiveClassFactory *factory;
    factory = wxArchiveClassFactory::Find(protocol);
    if (!factory)
        return wxEmptyString;

    m_Archive = m_cache->Get(key);
    if (!m_Archive)
    {
        wxFSFile *leftFile = m_fs.OpenFile(left);
        if (!leftFile)
            return wxEmptyString;
        m_Archive = m_cache->Add(key, *factory, leftFile->DetachStream());
        delete leftFile;
    }

    m_FindEntry = NULL;

    switch (flags)
    {
        case wxFILE:
            m_AllowDirs = false, m_AllowFiles = true; break;
        case wxDIR:
            m_AllowDirs = true, m_AllowFiles = false; break;
        default:
            m_AllowDirs = m_AllowFiles = true; break;
    }

    m_ZipFile = key;

    m_Pattern = right.AfterLast(wxT('/'));
    m_BaseDir = right.BeforeLast(wxT('/'));
    if (m_BaseDir.StartsWith(wxT("/")))
        m_BaseDir = m_BaseDir.Mid(1);

    if (m_Archive)
    {
        if (m_AllowDirs)
        {
            delete m_DirsFound;
            m_DirsFound = new wxArchiveFilenameHashMap();
            if (right.empty())  // allow "/" to match the archive root
                return spec;
        }
        return DoFind();
    }
    return wxEmptyString;
}

wxString wxArchiveFSHandler::FindNext()
{
    if (!m_Archive) return wxEmptyString;
    return DoFind();
}

wxString wxArchiveFSHandler::DoFind()
{
    wxString namestr, dir, filename;
    wxString match = wxEmptyString;

    while (match == wxEmptyString)
    {
        m_FindEntry = m_Archive->GetNext(m_FindEntry);

        if (!m_FindEntry)
        {
            m_Archive = NULL;
            m_FindEntry = NULL;
            break;
        }
        namestr = m_FindEntry->entry->GetName(wxPATH_UNIX);

        if (m_AllowDirs)
        {
            dir = namestr.BeforeLast(wxT('/'));
            while (!dir.empty())
            {
                if( m_DirsFound->find(dir) == m_DirsFound->end() )
                {
                    (*m_DirsFound)[dir] = 1;
                    filename = dir.AfterLast(wxT('/'));
                    dir = dir.BeforeLast(wxT('/'));
                    if (!filename.empty() && m_BaseDir == dir &&
                                wxMatchWild(m_Pattern, filename, false))
                        match = m_ZipFile + dir + wxT("/") + filename;
                }
                else
                    break; // already tranversed
            }
        }

        filename = namestr.AfterLast(wxT('/'));
        dir = namestr.BeforeLast(wxT('/'));
        if (m_AllowFiles && !filename.empty() && m_BaseDir == dir &&
                            wxMatchWild(m_Pattern, filename, false))
            match = m_ZipFile + namestr;
    }

    return match;
}

#endif // wxUSE_FS_ARCHIVE
