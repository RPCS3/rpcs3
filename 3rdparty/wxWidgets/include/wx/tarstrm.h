/////////////////////////////////////////////////////////////////////////////
// Name:        wx/tarstrm.h
// Purpose:     Streams for Tar files
// Author:      Mike Wetherell
// RCS-ID:      $Id: tarstrm.h 43887 2006-12-09 22:28:11Z MW $
// Copyright:   (c) 2004 Mike Wetherell
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_WXTARSTREAM_H__
#define _WX_WXTARSTREAM_H__

#include "wx/defs.h"

#if wxUSE_TARSTREAM

#include "wx/archive.h"
#include "wx/hashmap.h"


/////////////////////////////////////////////////////////////////////////////
// Constants

// TypeFlag values
enum {
    wxTAR_REGTYPE   = '0',      // regular file
    wxTAR_LNKTYPE   = '1',      // hard link
    wxTAR_SYMTYPE   = '2',      // symbolic link
    wxTAR_CHRTYPE   = '3',      // character special
    wxTAR_BLKTYPE   = '4',      // block special
    wxTAR_DIRTYPE   = '5',      // directory
    wxTAR_FIFOTYPE  = '6',      // named pipe
    wxTAR_CONTTYPE  = '7'       // contiguous file
};

// Archive Formats (use wxTAR_PAX, it's backward compatible)
enum wxTarFormat
{
    wxTAR_USTAR,                // POSIX.1-1990 tar format
    wxTAR_PAX                   // POSIX.1-2001 tar format
};


/////////////////////////////////////////////////////////////////////////////
// wxTarNotifier

class WXDLLIMPEXP_BASE wxTarNotifier
{
public:
    virtual ~wxTarNotifier() { }

    virtual void OnEntryUpdated(class wxTarEntry& entry) = 0;
};


/////////////////////////////////////////////////////////////////////////////
// Tar Entry - hold the meta data for a file in the tar

class WXDLLIMPEXP_BASE wxTarEntry : public wxArchiveEntry
{
public:
    wxTarEntry(const wxString& name = wxEmptyString,
               const wxDateTime& dt = wxDateTime::Now(),
               wxFileOffset size = wxInvalidOffset);
    virtual ~wxTarEntry();

    wxTarEntry(const wxTarEntry& entry);
    wxTarEntry& operator=(const wxTarEntry& entry);

    // Get accessors
    wxString     GetName(wxPathFormat format = wxPATH_NATIVE) const;
    wxString     GetInternalName() const        { return m_Name; }
    wxPathFormat GetInternalFormat() const      { return wxPATH_UNIX; }
    int          GetMode() const;
    int          GetUserId() const              { return m_UserId; }
    int          GetGroupId() const             { return m_GroupId; }
    wxFileOffset GetSize() const                { return m_Size; }
    wxFileOffset GetOffset() const              { return m_Offset; }
    wxDateTime   GetDateTime() const            { return m_ModifyTime; }
    wxDateTime   GetAccessTime() const          { return m_AccessTime; }
    wxDateTime   GetCreateTime() const          { return m_CreateTime; }
    int          GetTypeFlag() const            { return m_TypeFlag; }
    wxString     GetLinkName() const            { return m_LinkName; }
    wxString     GetUserName() const            { return m_UserName; }
    wxString     GetGroupName() const           { return m_GroupName; }
    int          GetDevMajor() const            { return m_DevMajor; }
    int          GetDevMinor() const            { return m_DevMinor; }

    // is accessors
    bool IsDir() const;
    bool IsReadOnly() const                     { return !(m_Mode & 0222); }

    // set accessors
    void SetName(const wxString& name, wxPathFormat format = wxPATH_NATIVE);
    void SetUserId(int id)                      { m_UserId = id; }
    void SetGroupId(int id)                     { m_GroupId = id; }
    void SetMode(int mode);
    void SetSize(wxFileOffset size)             { m_Size = size; }
    void SetDateTime(const wxDateTime& dt)      { m_ModifyTime = dt; }
    void SetAccessTime(const wxDateTime& dt)    { m_AccessTime = dt; }
    void SetCreateTime(const wxDateTime& dt)    { m_CreateTime = dt; }
    void SetTypeFlag(int type)                  { m_TypeFlag = type; }
    void SetLinkName(const wxString& link)      { m_LinkName = link; }
    void SetUserName(const wxString& user)      { m_UserName = user; }
    void SetGroupName(const wxString& group)    { m_GroupName = group; }
    void SetDevMajor(int dev)                   { m_DevMajor = dev; }
    void SetDevMinor(int dev)                   { m_DevMinor = dev; }

    // set is accessors
    void SetIsDir(bool isDir = true);
    void SetIsReadOnly(bool isReadOnly = true);

    static wxString GetInternalName(const wxString& name,
                                    wxPathFormat format = wxPATH_NATIVE,
                                    bool *pIsDir = NULL);

    wxTarEntry *Clone() const { return new wxTarEntry(*this); }

    void SetNotifier(wxTarNotifier& WXUNUSED(notifier)) { }

private:
    void SetOffset(wxFileOffset offset)         { m_Offset = offset; }

    virtual wxArchiveEntry* DoClone() const     { return Clone(); }

    wxString     m_Name;
    int          m_Mode;
    bool         m_IsModeSet;
    int          m_UserId;
    int          m_GroupId;
    wxFileOffset m_Size;
    wxFileOffset m_Offset;
    wxDateTime   m_ModifyTime;
    wxDateTime   m_AccessTime;
    wxDateTime   m_CreateTime;
    int          m_TypeFlag;
    wxString     m_LinkName;
    wxString     m_UserName;
    wxString     m_GroupName;
    int          m_DevMajor;
    int          m_DevMinor;

    friend class wxTarInputStream;

    DECLARE_DYNAMIC_CLASS(wxTarEntry)
};


/////////////////////////////////////////////////////////////////////////////
// wxTarInputStream

WX_DECLARE_STRING_HASH_MAP(wxString, wxTarHeaderRecords);

class WXDLLIMPEXP_BASE wxTarInputStream : public wxArchiveInputStream
{
public:
    typedef wxTarEntry entry_type;

    wxTarInputStream(wxInputStream& stream, wxMBConv& conv = wxConvLocal);
    wxTarInputStream(wxInputStream *stream, wxMBConv& conv = wxConvLocal);
    virtual ~wxTarInputStream();

    bool OpenEntry(wxTarEntry& entry);
    bool CloseEntry();

    wxTarEntry *GetNextEntry();

    wxFileOffset GetLength() const      { return m_size; }
    bool IsSeekable() const { return m_parent_i_stream->IsSeekable(); }

protected:
    size_t OnSysRead(void *buffer, size_t size);
    wxFileOffset OnSysTell() const      { return m_pos; }
    wxFileOffset OnSysSeek(wxFileOffset seek, wxSeekMode mode);

private:
    void Init();

    wxArchiveEntry *DoGetNextEntry()    { return GetNextEntry(); }
    bool OpenEntry(wxArchiveEntry& entry);
    bool IsOpened() const               { return m_pos != wxInvalidOffset; }

    wxStreamError ReadHeaders();
    bool ReadExtendedHeader(wxTarHeaderRecords*& recs);

    wxString GetExtendedHeader(const wxString& key) const;
    wxString GetHeaderPath() const;
    wxFileOffset GetHeaderNumber(int id) const;
    wxString GetHeaderString(int id) const;
    wxDateTime GetHeaderDate(const wxString& key) const;

    wxFileOffset m_pos;     // position within the current entry
    wxFileOffset m_offset;  // offset to the start of the entry's data
    wxFileOffset m_size;    // size of the current entry's data

    int m_sumType;
    int m_tarType;
    class wxTarHeaderBlock *m_hdr;
    wxTarHeaderRecords *m_HeaderRecs;
    wxTarHeaderRecords *m_GlobalHeaderRecs;

    DECLARE_NO_COPY_CLASS(wxTarInputStream)
};


/////////////////////////////////////////////////////////////////////////////
// wxTarOutputStream

class WXDLLIMPEXP_BASE wxTarOutputStream : public wxArchiveOutputStream
{
public:
    wxTarOutputStream(wxOutputStream& stream,
                      wxTarFormat format = wxTAR_PAX,
                      wxMBConv& conv = wxConvLocal);
    wxTarOutputStream(wxOutputStream *stream,
                      wxTarFormat format = wxTAR_PAX,
                      wxMBConv& conv = wxConvLocal);
    virtual ~wxTarOutputStream();

    bool PutNextEntry(wxTarEntry *entry);

    bool PutNextEntry(const wxString& name,
                      const wxDateTime& dt = wxDateTime::Now(),
                      wxFileOffset size = wxInvalidOffset);

    bool PutNextDirEntry(const wxString& name,
                         const wxDateTime& dt = wxDateTime::Now());

    bool CopyEntry(wxTarEntry *entry, wxTarInputStream& inputStream);
    bool CopyArchiveMetaData(wxTarInputStream& WXUNUSED(s)) { return true; }

    void Sync();
    bool CloseEntry();
    bool Close();

    bool IsSeekable() const { return m_parent_o_stream->IsSeekable(); }

    void SetBlockingFactor(int factor)  { m_BlockingFactor = factor; }
    int GetBlockingFactor() const       { return m_BlockingFactor; }

protected:
    size_t OnSysWrite(const void *buffer, size_t size);
    wxFileOffset OnSysTell() const      { return m_pos; }
    wxFileOffset OnSysSeek(wxFileOffset pos, wxSeekMode mode);

private:
    void Init(wxTarFormat format);

    bool PutNextEntry(wxArchiveEntry *entry);
    bool CopyEntry(wxArchiveEntry *entry, wxArchiveInputStream& stream);
    bool CopyArchiveMetaData(wxArchiveInputStream& WXUNUSED(s)) { return true; }
    bool IsOpened() const               { return m_pos != wxInvalidOffset; }

    bool WriteHeaders(wxTarEntry& entry);
    bool ModifyHeader();
    wxString PaxHeaderPath(const wxString& format, const wxString& path);

    void SetExtendedHeader(const wxString& key, const wxString& value);
    void SetHeaderPath(const wxString& name);
    bool SetHeaderNumber(int id, wxFileOffset n);
    void SetHeaderString(int id, const wxString& str);
    void SetHeaderDate(const wxString& key, const wxDateTime& datetime);

    wxFileOffset m_pos;     // position within the current entry
    wxFileOffset m_maxpos;  // max pos written
    wxFileOffset m_size;    // expected entry size

    wxFileOffset m_headpos; // offset within the file to the entry's header
    wxFileOffset m_datapos; // offset within the file to the entry's data

    wxFileOffset m_tarstart;// offset within the file to the tar
    wxFileOffset m_tarsize; // size of tar so far

    bool m_pax;
    int m_BlockingFactor;
    wxUint32 m_chksum;
    bool m_large;
    class wxTarHeaderBlock *m_hdr;
    class wxTarHeaderBlock *m_hdr2;
    char *m_extendedHdr;
    size_t m_extendedSize;
    wxString m_badfit;

    DECLARE_NO_COPY_CLASS(wxTarOutputStream)
};


/////////////////////////////////////////////////////////////////////////////
// Iterators

#if wxUSE_STL || defined WX_TEST_ARCHIVE_ITERATOR
typedef wxArchiveIterator<wxTarInputStream> wxTarIter;
typedef wxArchiveIterator<wxTarInputStream,
         std::pair<wxString, wxTarEntry*> > wxTarPairIter;
#endif


/////////////////////////////////////////////////////////////////////////////
// wxTarClassFactory

class WXDLLIMPEXP_BASE wxTarClassFactory : public wxArchiveClassFactory
{
public:
    typedef wxTarEntry        entry_type;
    typedef wxTarInputStream  instream_type;
    typedef wxTarOutputStream outstream_type;
    typedef wxTarNotifier     notifier_type;
#if wxUSE_STL || defined WX_TEST_ARCHIVE_ITERATOR
    typedef wxTarIter         iter_type;
    typedef wxTarPairIter     pairiter_type;
#endif

    wxTarClassFactory();

    wxTarEntry *NewEntry() const
        { return new wxTarEntry; }
    wxTarInputStream *NewStream(wxInputStream& stream) const
        { return new wxTarInputStream(stream, GetConv()); }
    wxTarOutputStream *NewStream(wxOutputStream& stream) const
        { return new wxTarOutputStream(stream, wxTAR_PAX, GetConv()); }
    wxTarInputStream *NewStream(wxInputStream *stream) const
        { return new wxTarInputStream(stream, GetConv()); }
    wxTarOutputStream *NewStream(wxOutputStream *stream) const
        { return new wxTarOutputStream(stream, wxTAR_PAX, GetConv()); }

    wxString GetInternalName(const wxString& name,
                             wxPathFormat format = wxPATH_NATIVE) const
        { return wxTarEntry::GetInternalName(name, format); }

    const wxChar * const *GetProtocols(wxStreamProtocolType type
                                       = wxSTREAM_PROTOCOL) const;

protected:
    wxArchiveEntry *DoNewEntry() const
        { return NewEntry(); }
    wxArchiveInputStream *DoNewStream(wxInputStream& stream) const
        { return NewStream(stream); }
    wxArchiveOutputStream *DoNewStream(wxOutputStream& stream) const
        { return NewStream(stream); }
    wxArchiveInputStream *DoNewStream(wxInputStream *stream) const
        { return NewStream(stream); }
    wxArchiveOutputStream *DoNewStream(wxOutputStream *stream) const
        { return NewStream(stream); }

private:
    DECLARE_DYNAMIC_CLASS(wxTarClassFactory)
};


#endif // wxUSE_TARSTREAM

#endif // _WX_WXTARSTREAM_H__
