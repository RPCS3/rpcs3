/////////////////////////////////////////////////////////////////////////////
// Name:        tarstrm.cpp
// Purpose:     Streams for Tar files
// Author:      Mike Wetherell
// RCS-ID:      $Id: tarstrm.cpp 63302 2010-01-28 21:46:09Z MW $
// Copyright:   (c) 2004 Mike Wetherell
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
  #pragma hdrstop
#endif

#if wxUSE_TARSTREAM

#include "wx/tarstrm.h"

#ifndef WX_PRECOMP
    #include "wx/intl.h"
    #include "wx/log.h"
    #include "wx/utils.h"
#endif

#include "wx/buffer.h"
#include "wx/datetime.h"
#include "wx/ptr_scpd.h"
#include "wx/filename.h"
#include "wx/thread.h"

#include <ctype.h>

#ifdef __UNIX__
#include <pwd.h>
#include <grp.h>
#endif


/////////////////////////////////////////////////////////////////////////////
// constants

enum {
    TAR_NAME,
    TAR_MODE,
    TAR_UID,
    TAR_GID,
    TAR_SIZE,
    TAR_MTIME,
    TAR_CHKSUM,
    TAR_TYPEFLAG,
    TAR_LINKNAME,
    TAR_MAGIC,
    TAR_VERSION,
    TAR_UNAME,
    TAR_GNAME,
    TAR_DEVMAJOR,
    TAR_DEVMINOR,
    TAR_PREFIX,
    TAR_UNUSED,
    TAR_NUMFIELDS
};

enum {
    TAR_BLOCKSIZE = 512
};

// checksum type
enum {
    SUM_UNKNOWN,
    SUM_UNSIGNED,
    SUM_SIGNED
};

// type of input tar
enum {
    TYPE_OLDTAR,    // fields after TAR_LINKNAME are invalid
    TYPE_GNUTAR,    // all fields except TAR_PREFIX are valid
    TYPE_USTAR      // all fields are valid
};

// signatures
static const char *USTAR_MAGIC   = "ustar";
static const char *USTAR_VERSION = "00";
static const char *GNU_MAGIC     = "ustar ";
static const char *GNU_VERION    = " ";

IMPLEMENT_DYNAMIC_CLASS(wxTarEntry, wxArchiveEntry)
IMPLEMENT_DYNAMIC_CLASS(wxTarClassFactory, wxArchiveClassFactory)


/////////////////////////////////////////////////////////////////////////////
// Class factory

static wxTarClassFactory g_wxTarClassFactory;

wxTarClassFactory::wxTarClassFactory()
{
    if (this == &g_wxTarClassFactory)
        PushFront();
}

const wxChar * const *
wxTarClassFactory::GetProtocols(wxStreamProtocolType type) const
{
    static const wxChar *protocols[] = { _T("tar"), NULL };
    static const wxChar *mimetypes[] = { _T("application/x-tar"), NULL };
    static const wxChar *fileexts[]  = { _T(".tar"), NULL };
    static const wxChar *empty[]     = { NULL };

    switch (type) {
        case wxSTREAM_PROTOCOL: return protocols;
        case wxSTREAM_MIMETYPE: return mimetypes;
        case wxSTREAM_FILEEXT:  return fileexts;
        default:                return empty;
    }
}


/////////////////////////////////////////////////////////////////////////////
// tar header block

typedef wxFileOffset wxTarNumber;

struct wxTarField { const wxChar *name; int pos; };

class wxTarHeaderBlock
{
public:
    wxTarHeaderBlock()
        { memset(data, 0, sizeof(data)); }
    wxTarHeaderBlock(const wxTarHeaderBlock& hb)
        { memcpy(data, hb.data, sizeof(data)); }

    bool Read(wxInputStream& in);
    bool Write(wxOutputStream& out);
    inline bool WriteField(wxOutputStream& out, int id);

    bool IsAllZeros() const;
    wxUint32 Sum(bool SignedSum = false);
    wxUint32 SumField(int id);

    char *Get(int id) { return data + fields[id].pos + id; }
    static size_t Len(int id) { return fields[id + 1].pos - fields[id].pos; }
    static const wxChar *Name(int id) { return fields[id].name; }
    static size_t Offset(int id) { return fields[id].pos; }

    bool SetOctal(int id, wxTarNumber n);
    wxTarNumber GetOctal(int id);
    bool SetPath(const wxString& name, wxMBConv& conv);

private:
    char data[TAR_BLOCKSIZE + TAR_NUMFIELDS];
    static const wxTarField fields[];
    static void check();
};

wxDEFINE_SCOPED_PTR_TYPE(wxTarHeaderBlock)

// A table giving the field names and offsets in a tar header block
const wxTarField wxTarHeaderBlock::fields[] =
{
    { _T("name"), 0 },       // 100
    { _T("mode"), 100 },     // 8
    { _T("uid"), 108 },      // 8
    { _T("gid"), 116 },      // 8
    { _T("size"), 124 },     // 12
    { _T("mtime"), 136 },    // 12
    { _T("chksum"), 148 },   // 8
    { _T("typeflag"), 156 }, // 1
    { _T("linkname"), 157 }, // 100
    { _T("magic"), 257 },    // 6
    { _T("version"), 263 },  // 2
    { _T("uname"), 265 },    // 32
    { _T("gname"), 297 },    // 32
    { _T("devmajor"), 329 }, // 8
    { _T("devminor"), 337 }, // 8
    { _T("prefix"), 345 },   // 155
    { _T("unused"), 500 },   // 12
    { NULL, TAR_BLOCKSIZE }
};

void wxTarHeaderBlock::check()
{
#if 0
    wxCOMPILE_TIME_ASSERT(
        WXSIZEOF(fields) == TAR_NUMFIELDS + 1,
        Wrong_number_of_elements_in_fields_table
    );
#endif
}

bool wxTarHeaderBlock::IsAllZeros() const
{
    const char *p = data;
    for (size_t i = 0; i < sizeof(data); i++)
        if (p[i])
            return false;
    return true;
}

wxUint32 wxTarHeaderBlock::Sum(bool SignedSum /*=false*/)
{
    // the chksum field itself should be blanks during the calculation
    memset(Get(TAR_CHKSUM), ' ', Len(TAR_CHKSUM));
    const char *p = data;
    wxUint32 n = 0;

    if (SignedSum)
        for (size_t i = 0; i < sizeof(data); i++)
            n += (signed char)p[i];
    else
        for (size_t i = 0; i < sizeof(data); i++)
            n += (unsigned char)p[i];

    return n;
}

wxUint32 wxTarHeaderBlock::SumField(int id)
{
    unsigned char *p = (unsigned char*)Get(id);
    unsigned char *q = p + Len(id);
    wxUint32 n = 0;

    while (p < q)
        n += *p++;

    return n;
}

bool wxTarHeaderBlock::Read(wxInputStream& in)
{
    bool ok = true;

    for (int id = 0; id < TAR_NUMFIELDS && ok; id++)
        ok = in.Read(Get(id), Len(id)).LastRead() == Len(id);

    return ok;
}

bool wxTarHeaderBlock::Write(wxOutputStream& out)
{
    bool ok = true;

    for (int id = 0; id < TAR_NUMFIELDS && ok; id++)
        ok = WriteField(out, id);

    return ok;
}

inline bool wxTarHeaderBlock::WriteField(wxOutputStream& out, int id)
{
    return out.Write(Get(id), Len(id)).LastWrite() == Len(id);
}

wxTarNumber wxTarHeaderBlock::GetOctal(int id)
{
    wxTarNumber n = 0;
    const char *p = Get(id);
    while (*p == ' ')
        p++;
    while (*p >= '0' && *p < '8')
        n = (n << 3) | (*p++ - '0');
    return n;
}

bool wxTarHeaderBlock::SetOctal(int id, wxTarNumber n)
{
    // set an octal field, return true if the number fits
    char *field = Get(id);
    char *p = field + Len(id);
    *--p = 0;
    while (p > field) {
        *--p = char('0' + (n & 7));
        n >>= 3;
    }
    return n == 0;
}

bool wxTarHeaderBlock::SetPath(const wxString& name, wxMBConv& conv)
{
    bool badconv = false;

#if wxUSE_UNICODE
    wxCharBuffer nameBuf = name.mb_str(conv);

    // if the conversion fails make an approximation
    if (!nameBuf) {
        badconv = true;
        size_t len = name.length();
        wxCharBuffer approx(len);
        for (size_t i = 0; i < len; i++)
            approx.data()[i] = name[i] & ~0x7F ? '_' : name[i];
        nameBuf = approx;
    }

    const char *mbName = nameBuf;
#else
    const char *mbName = name.c_str();
    (void)conv;
#endif

    bool fits;
    bool notGoingToFit = false;
    size_t len = strlen(mbName);
    size_t maxname = Len(TAR_NAME);
    size_t maxprefix = Len(TAR_PREFIX);
    size_t i = 0;
    size_t nexti = 0;

    for (;;) {
        fits = i < maxprefix && len - i <= maxname;

        if (!fits) {
            const char *p = strchr(mbName + i, '/');
            if (p)
                nexti = p - mbName + 1;
            if (!p || nexti - 1 > maxprefix)
                notGoingToFit = true;
        }

        if (fits || notGoingToFit) {
            strncpy(Get(TAR_NAME), mbName + i, maxname);
            if (i > 0)
                strncpy(Get(TAR_PREFIX), mbName, i - 1);
            break;
        }

        i = nexti;
    }

    return fits && !badconv;
}


/////////////////////////////////////////////////////////////////////////////
// Some helpers

static wxFileOffset RoundUpSize(wxFileOffset size, int factor = 1)
{
    wxFileOffset chunk = TAR_BLOCKSIZE * factor;
    return ((size + chunk - 1) / chunk) * chunk;
}

#ifdef __UNIX__

static wxString wxTarUserName(int uid)
{
    struct passwd *ppw;

#ifdef HAVE_GETPWUID_R
#if defined HAVE_SYSCONF && defined _SC_GETPW_R_SIZE_MAX
    long pwsize = sysconf(_SC_GETPW_R_SIZE_MAX);
    size_t bufsize(wxMin(wxMax(1024l, pwsize), 32768l));
#else
    size_t bufsize = 1024;
#endif
    wxCharBuffer buf(bufsize);
    struct passwd pw;

    memset(&pw, 0, sizeof(pw));
    if (getpwuid_r(uid, &pw, buf.data(), bufsize, &ppw) == 0 && pw.pw_name)
        return wxString(pw.pw_name, wxConvLibc);
#else
    if ((ppw = getpwuid(uid)) != NULL)
        return wxString(ppw->pw_name, wxConvLibc);
#endif
    return _("unknown");
}

static wxString wxTarGroupName(int gid)
{
    struct group *pgr;
#ifdef HAVE_GETGRGID_R
#if defined HAVE_SYSCONF && defined _SC_GETGR_R_SIZE_MAX
    long grsize = sysconf(_SC_GETGR_R_SIZE_MAX);
    size_t bufsize(wxMin(wxMax(1024l, grsize), 32768l));
#else
    size_t bufsize = 1024;
#endif
    wxCharBuffer buf(bufsize);
    struct group gr;

    memset(&gr, 0, sizeof(gr));
    if (getgrgid_r(gid, &gr, buf.data(), bufsize, &pgr) == 0 && gr.gr_name)
        return wxString(gr.gr_name, wxConvLibc);
#else
    if ((pgr = getgrgid(gid)) != NULL)
        return wxString(pgr->gr_name, wxConvLibc);
#endif
    return _("unknown");
}

#endif  // __UNIX__

// Cache the user and group names since getting them can be expensive,
// get both names and ids at the same time.
//
struct wxTarUser
{
    wxTarUser();
    ~wxTarUser() { delete [] uname; delete [] gname; }

    int uid;
    int gid;

    wxChar *uname;
    wxChar *gname;
};

wxTarUser::wxTarUser()
{
#ifdef __UNIX__
    uid = getuid();
    gid = getgid();
    wxString usr = wxTarUserName(uid);
    wxString grp = wxTarGroupName(gid);
#else
    uid = 0;
    gid = 0;
    wxString usr = wxGetUserId();
    wxString grp = _("unknown");
#endif

    uname = new wxChar[usr.length() + 1];
    wxStrcpy(uname, usr.c_str());

    gname = new wxChar[grp.length() + 1];
    wxStrcpy(gname, grp.c_str());
}

static const wxTarUser& wxGetTarUser()
{
#if wxUSE_THREADS
    static wxCriticalSection cs;
    wxCriticalSectionLocker lock(cs);
#endif
    static wxTarUser tu;
    return tu;
}

// ignore the size field for entry types 3, 4, 5 and 6
//
static inline wxFileOffset GetDataSize(const wxTarEntry& entry)
{
    switch (entry.GetTypeFlag()) {
        case wxTAR_CHRTYPE:
        case wxTAR_BLKTYPE:
        case wxTAR_DIRTYPE:
        case wxTAR_FIFOTYPE:
            return 0;
        default:
            return entry.GetSize();
    }
}


/////////////////////////////////////////////////////////////////////////////
// Tar Entry
// Holds all the meta-data for a file in the tar

wxTarEntry::wxTarEntry(const wxString& name /*=wxEmptyString*/,
                       const wxDateTime& dt /*=wxDateTime::Now()*/,
                       wxFileOffset size    /*=0*/)
  : m_Mode(0644),
    m_IsModeSet(false),
    m_UserId(wxGetTarUser().uid),
    m_GroupId(wxGetTarUser().gid),
    m_Size(size),
    m_Offset(wxInvalidOffset),
    m_ModifyTime(dt),
    m_TypeFlag(wxTAR_REGTYPE),
    m_UserName(wxGetTarUser().uname),
    m_GroupName(wxGetTarUser().gname),
    m_DevMajor(~0),
    m_DevMinor(~0)
{
    if (!name.empty())
        SetName(name);
}

wxTarEntry::~wxTarEntry()
{
}

wxTarEntry::wxTarEntry(const wxTarEntry& e)
  : wxArchiveEntry(),
    m_Name(e.m_Name),
    m_Mode(e.m_Mode),
    m_IsModeSet(e.m_IsModeSet),
    m_UserId(e.m_UserId),
    m_GroupId(e.m_GroupId),
    m_Size(e.m_Size),
    m_Offset(e.m_Offset),
    m_ModifyTime(e.m_ModifyTime),
    m_AccessTime(e.m_AccessTime),
    m_CreateTime(e.m_CreateTime),
    m_TypeFlag(e.m_TypeFlag),
    m_LinkName(e.m_LinkName),
    m_UserName(e.m_UserName),
    m_GroupName(e.m_GroupName),
    m_DevMajor(e.m_DevMajor),
    m_DevMinor(e.m_DevMinor)
{
}

wxTarEntry& wxTarEntry::operator=(const wxTarEntry& e)
{
    if (&e != this) {
        m_Name = e.m_Name;
        m_Mode = e.m_Mode;
        m_IsModeSet = e.m_IsModeSet;
        m_UserId = e.m_UserId;
        m_GroupId = e.m_GroupId;
        m_Size = e.m_Size;
        m_Offset = e.m_Offset;
        m_ModifyTime = e.m_ModifyTime;
        m_AccessTime = e.m_AccessTime;
        m_CreateTime = e.m_CreateTime;
        m_TypeFlag = e.m_TypeFlag;
        m_LinkName = e.m_LinkName;
        m_UserName = e.m_UserName;
        m_GroupName = e.m_GroupName;
        m_DevMajor = e.m_DevMajor;
        m_DevMinor = e.m_DevMinor;
    }
    return *this;
}

wxString wxTarEntry::GetName(wxPathFormat format /*=wxPATH_NATIVE*/) const
{
    bool isDir = IsDir() && !m_Name.empty();

    // optimisations for common (and easy) cases
    switch (wxFileName::GetFormat(format)) {
        case wxPATH_DOS:
        {
            wxString name(isDir ? m_Name + _T("\\") : m_Name);
            for (size_t i = 0; i < name.length(); i++)
                if (name[i] == _T('/'))
                    name[i] = _T('\\');
            return name;
        }

        case wxPATH_UNIX:
            return isDir ? m_Name + _T("/") : m_Name;

        default:
            ;
    }

    wxFileName fn;

    if (isDir)
        fn.AssignDir(m_Name, wxPATH_UNIX);
    else
        fn.Assign(m_Name, wxPATH_UNIX);

    return fn.GetFullPath(format);
}

void wxTarEntry::SetName(const wxString& name, wxPathFormat format)
{
    bool isDir;
    m_Name = GetInternalName(name, format, &isDir);
    SetIsDir(isDir);
}

// Static - Internally tars and zips use forward slashes for the path
// separator, absolute paths aren't allowed, and directory names have a
// trailing slash.  This function converts a path into this internal format,
// but without a trailing slash for a directory.
//
wxString wxTarEntry::GetInternalName(const wxString& name,
                                     wxPathFormat format /*=wxPATH_NATIVE*/,
                                     bool *pIsDir        /*=NULL*/)
{
    wxString internal;

    if (wxFileName::GetFormat(format) != wxPATH_UNIX)
        internal = wxFileName(name, format).GetFullPath(wxPATH_UNIX);
    else
        internal = name;

    bool isDir = !internal.empty() && internal.Last() == '/';
    if (pIsDir)
        *pIsDir = isDir;
    if (isDir)
        internal.erase(internal.length() - 1);

    while (!internal.empty() && *internal.begin() == '/')
        internal.erase(0, 1);
    while (!internal.empty() && internal.compare(0, 2, _T("./")) == 0)
        internal.erase(0, 2);
    if (internal == _T(".") || internal == _T(".."))
        internal = wxEmptyString;

    return internal;
}

bool wxTarEntry::IsDir() const
{
    return m_TypeFlag == wxTAR_DIRTYPE;
}

void wxTarEntry::SetIsDir(bool isDir)
{
    if (isDir)
        m_TypeFlag = wxTAR_DIRTYPE;
    else if (m_TypeFlag == wxTAR_DIRTYPE)
        m_TypeFlag = wxTAR_REGTYPE;
}

void wxTarEntry::SetIsReadOnly(bool isReadOnly)
{
    if (isReadOnly)
        m_Mode &= ~0222;
    else
        m_Mode |= 0200;
}

int wxTarEntry::GetMode() const
{
    if (m_IsModeSet || !IsDir())
        return m_Mode;
    else
        return m_Mode | 0111;

}

void wxTarEntry::SetMode(int mode)
{
    m_Mode = mode & 07777;
    m_IsModeSet = true;
}


/////////////////////////////////////////////////////////////////////////////
// Input stream

wxDECLARE_SCOPED_PTR(wxTarEntry, wxTarEntryPtr_)
wxDEFINE_SCOPED_PTR (wxTarEntry, wxTarEntryPtr_)

wxTarInputStream::wxTarInputStream(wxInputStream& stream,
                                   wxMBConv& conv /*=wxConvLocal*/)
 :  wxArchiveInputStream(stream, conv)
{
    Init();
}

wxTarInputStream::wxTarInputStream(wxInputStream *stream,
                                   wxMBConv& conv /*=wxConvLocal*/)
 :  wxArchiveInputStream(stream, conv)
{
    Init();
}

void wxTarInputStream::Init()
{
    m_pos = wxInvalidOffset;
    m_offset = 0;
    m_size = wxInvalidOffset;
    m_sumType = SUM_UNKNOWN;
    m_tarType = TYPE_USTAR;
    m_hdr = new wxTarHeaderBlock;
    m_HeaderRecs = NULL;
    m_GlobalHeaderRecs = NULL;
    m_lasterror = m_parent_i_stream->GetLastError();
}

wxTarInputStream::~wxTarInputStream()
{
    delete m_hdr;
    delete m_HeaderRecs;
    delete m_GlobalHeaderRecs;
}

wxTarEntry *wxTarInputStream::GetNextEntry()
{
    m_lasterror = ReadHeaders();

    if (!IsOk())
        return NULL;

    wxTarEntryPtr_ entry(new wxTarEntry);

    entry->SetMode(GetHeaderNumber(TAR_MODE));
    entry->SetUserId(GetHeaderNumber(TAR_UID));
    entry->SetGroupId(GetHeaderNumber(TAR_UID));
    entry->SetSize(GetHeaderNumber(TAR_SIZE));

    entry->SetOffset(m_offset);

    entry->SetDateTime(GetHeaderDate(_T("mtime")));
    entry->SetAccessTime(GetHeaderDate(_T("atime")));
    entry->SetCreateTime(GetHeaderDate(_T("ctime")));

    entry->SetTypeFlag(*m_hdr->Get(TAR_TYPEFLAG));
    bool isDir = entry->IsDir();

    entry->SetLinkName(GetHeaderString(TAR_LINKNAME));

    if (m_tarType != TYPE_OLDTAR) {
        entry->SetUserName(GetHeaderString(TAR_UNAME));
        entry->SetGroupName(GetHeaderString(TAR_GNAME));

        entry->SetDevMajor(GetHeaderNumber(TAR_DEVMAJOR));
        entry->SetDevMinor(GetHeaderNumber(TAR_DEVMINOR));
    }

    entry->SetName(GetHeaderPath(), wxPATH_UNIX);
    if (isDir)
        entry->SetIsDir();

    if (m_HeaderRecs)
        m_HeaderRecs->clear();

    m_size = GetDataSize(*entry);
    m_pos = 0;

    return entry.release();
}

bool wxTarInputStream::OpenEntry(wxTarEntry& entry)
{
    wxFileOffset offset = entry.GetOffset();

    if (GetLastError() != wxSTREAM_READ_ERROR
            && m_parent_i_stream->IsSeekable()
            && m_parent_i_stream->SeekI(offset) == offset)
    {
        m_offset = offset;
        m_size = GetDataSize(entry);
        m_pos = 0;
        m_lasterror = wxSTREAM_NO_ERROR;
        return true;
    } else {
        m_lasterror = wxSTREAM_READ_ERROR;
        return false;
    }
}

bool wxTarInputStream::OpenEntry(wxArchiveEntry& entry)
{
    wxTarEntry *tarEntry = wxStaticCast(&entry, wxTarEntry);
    return tarEntry ? OpenEntry(*tarEntry) : false;
}

bool wxTarInputStream::CloseEntry()
{
    if (m_lasterror == wxSTREAM_READ_ERROR)
        return false;
    if (!IsOpened())
        return true;

    wxFileOffset size = RoundUpSize(m_size);
    wxFileOffset remainder = size - m_pos;

    if (remainder && m_parent_i_stream->IsSeekable()) {
        wxLogNull nolog;
        if (m_parent_i_stream->SeekI(remainder, wxFromCurrent)
                != wxInvalidOffset)
            remainder = 0;
    }

    if (remainder) {
        const int BUFSIZE = 8192;
        wxCharBuffer buf(BUFSIZE);

        while (remainder > 0 && m_parent_i_stream->IsOk())
            remainder -= m_parent_i_stream->Read(
                    buf.data(), wxMin(BUFSIZE, remainder)).LastRead();
    }

    m_pos = wxInvalidOffset;
    m_offset += size;
    m_lasterror = m_parent_i_stream->GetLastError();

    return IsOk();
}

wxStreamError wxTarInputStream::ReadHeaders()
{
    if (!CloseEntry())
        return wxSTREAM_READ_ERROR;

    bool done = false;

    while (!done) {
        m_hdr->Read(*m_parent_i_stream);
        if (m_parent_i_stream->Eof())
            wxLogError(_("incomplete header block in tar"));
        if (!*m_parent_i_stream)
            return wxSTREAM_READ_ERROR;
        m_offset += TAR_BLOCKSIZE;

        // an all-zero header marks the end of the tar
        if (m_hdr->IsAllZeros())
            return wxSTREAM_EOF;

        // the checksum is supposed to be the unsigned sum of the header bytes,
        // but there have been versions of tar that used the signed sum, so
        // accept that too, but only if used throughout.
        wxUint32 chksum = m_hdr->GetOctal(TAR_CHKSUM);
        bool ok = false;

        if (m_sumType != SUM_SIGNED) {
            ok = chksum == m_hdr->Sum();
            if (m_sumType == SUM_UNKNOWN)
                m_sumType = ok ? SUM_UNSIGNED : SUM_SIGNED;
        }
        if (m_sumType == SUM_SIGNED)
            ok = chksum == m_hdr->Sum(true);
        if (!ok) {
            wxLogError(_("checksum failure reading tar header block"));
            return wxSTREAM_READ_ERROR;
        }

        if (strcmp(m_hdr->Get(TAR_MAGIC), USTAR_MAGIC) == 0)
            m_tarType = TYPE_USTAR;
        else if (strcmp(m_hdr->Get(TAR_MAGIC), GNU_MAGIC) == 0 &&
                 strcmp(m_hdr->Get(TAR_VERSION), GNU_VERION) == 0)
            m_tarType = TYPE_GNUTAR;
        else
            m_tarType = TYPE_OLDTAR;

        if (m_tarType != TYPE_USTAR)
            break;

        switch (*m_hdr->Get(TAR_TYPEFLAG)) {
            case 'g': ReadExtendedHeader(m_GlobalHeaderRecs); break;
            case 'x': ReadExtendedHeader(m_HeaderRecs); break;
            default:  done = true;
        }
    }

    return wxSTREAM_NO_ERROR;
}

wxString wxTarInputStream::GetExtendedHeader(const wxString& key) const
{
    wxTarHeaderRecords::iterator it;

    // look at normal extended header records first
    if (m_HeaderRecs) {
        it = m_HeaderRecs->find(key);
        if (it != m_HeaderRecs->end())
            return wxString(it->second.wc_str(wxConvUTF8), GetConv());
    }

    // if not found, look at the global header records
    if (m_GlobalHeaderRecs) {
        it = m_GlobalHeaderRecs->find(key);
        if (it != m_GlobalHeaderRecs->end())
            return wxString(it->second.wc_str(wxConvUTF8), GetConv());
    }

    return wxEmptyString;
}

wxString wxTarInputStream::GetHeaderPath() const
{
    wxString path;

    if ((path = GetExtendedHeader(_T("path"))) != wxEmptyString)
        return path;

    path = wxString(m_hdr->Get(TAR_NAME), GetConv());
    if (m_tarType != TYPE_USTAR)
        return path;

    const char *prefix = m_hdr->Get(TAR_PREFIX);
    return *prefix ? wxString(prefix, GetConv()) + _T("/") + path : path;
}

wxDateTime wxTarInputStream::GetHeaderDate(const wxString& key) const
{
    wxString value;

    // try extended header, stored as decimal seconds since the epoch
    if ((value = GetExtendedHeader(key)) != wxEmptyString) {
        wxLongLong ll;
        ll.Assign(wxAtof(value) * 1000.0);
        return ll;
    }

    if (key == _T("mtime"))
        return wxLongLong(m_hdr->GetOctal(TAR_MTIME)) * 1000L;

    return wxDateTime();
}

wxTarNumber wxTarInputStream::GetHeaderNumber(int id) const
{
    wxString value;

    if ((value = GetExtendedHeader(m_hdr->Name(id))) != wxEmptyString) {
        wxTarNumber n = 0;
        const wxChar *p = value;
        while (*p == ' ')
            p++;
        while (isdigit(*p))
            n = n * 10 + (*p++ - '0');
        return n;
    } else {
        return m_hdr->GetOctal(id);
    }
}

wxString wxTarInputStream::GetHeaderString(int id) const
{
    wxString value;

    if ((value = GetExtendedHeader(m_hdr->Name(id))) != wxEmptyString)
        return value;

    return wxString(m_hdr->Get(id), GetConv());
}

// An extended header consists of one or more records, each constructed:
// "%d %s=%s\n", <length>, <keyword>, <value>
// <length> is the byte length, <keyword> and <value> are UTF-8

bool wxTarInputStream::ReadExtendedHeader(wxTarHeaderRecords*& recs)
{
    if (!recs)
        recs = new wxTarHeaderRecords;

    // round length up to a whole number of blocks
    size_t len = m_hdr->GetOctal(TAR_SIZE);
    size_t size = RoundUpSize(len);

    // read in the whole header since it should be small
    wxCharBuffer buf(size);
    size_t lastread = m_parent_i_stream->Read(buf.data(), size).LastRead();
    if (lastread < len)
        len = lastread;
    buf.data()[len] = 0;
    m_offset += lastread;

    size_t recPos, recSize;
    bool ok = true;

    for (recPos = 0; recPos < len; recPos += recSize) {
        char *pRec = buf.data() + recPos;
        char *p = pRec;

        // read the record size (byte count in ascii decimal)
        recSize = 0;
        while (isdigit((unsigned char) *p))
            recSize = recSize * 10 + *p++ - '0';

        // validity checks
        if (recPos + recSize > len)
            break;
        if (recSize < p - pRec + (size_t)3 || *p != ' '
                || pRec[recSize - 1] != '\012') {
            ok = false;
            continue;
        }

        // replace the final '\n' with a nul, to terminate value
        pRec[recSize - 1] = 0;
        // the key is here, following the space
        char *pKey = ++p;

        // look forward for the '=', the value follows
        while (*p && *p != '=')
            p++;
        if (!*p) {
            ok = false;
            continue;
        }
        // replace the '=' with a nul, to terminate the key
        *p++ = 0;

        wxString key(wxConvUTF8.cMB2WC(pKey), GetConv());
        wxString value(wxConvUTF8.cMB2WC(p), GetConv());

        // an empty value unsets a previously given value
        if (value.empty())
            recs->erase(key);
        else
            (*recs)[key] = value;
    }

    if (!ok || recPos < len || size != lastread) {
        wxLogWarning(_("invalid data in extended tar header"));
        return false;
    }

    return true;
}

wxFileOffset wxTarInputStream::OnSysSeek(wxFileOffset pos, wxSeekMode mode)
{
    if (!IsOpened()) {
        wxLogError(_("tar entry not open"));
        m_lasterror = wxSTREAM_READ_ERROR;
    }
    if (!IsOk())
        return wxInvalidOffset;

    switch (mode) {
        case wxFromStart:   break;
        case wxFromCurrent: pos += m_pos; break;
        case wxFromEnd:     pos += m_size; break;
    }

    if (pos < 0 || m_parent_i_stream->SeekI(m_offset + pos) == wxInvalidOffset)
        return wxInvalidOffset;

    m_pos = pos;
    return m_pos;
}

size_t wxTarInputStream::OnSysRead(void *buffer, size_t size)
{
    if (!IsOpened()) {
        wxLogError(_("tar entry not open"));
        m_lasterror = wxSTREAM_READ_ERROR;
    }
    if (!IsOk() || !size)
        return 0;

    if (m_pos >= m_size)
        size = 0;
    else if (m_pos + size > m_size + (size_t)0)
        size = m_size - m_pos;

    size_t lastread = m_parent_i_stream->Read(buffer, size).LastRead();
    m_pos += lastread;

    if (m_pos >= m_size) {
        m_lasterror = wxSTREAM_EOF;
    } else if (!m_parent_i_stream->IsOk()) {
        // any other error will have been reported by the underlying stream
        if (m_parent_i_stream->Eof())
            wxLogError(_("unexpected end of file"));
        m_lasterror = wxSTREAM_READ_ERROR;
    }

    return lastread;
}


/////////////////////////////////////////////////////////////////////////////
// Output stream

wxTarOutputStream::wxTarOutputStream(wxOutputStream& stream,
                                     wxTarFormat format /*=wxTAR_PAX*/,
                                     wxMBConv& conv     /*=wxConvLocal*/)
 :  wxArchiveOutputStream(stream, conv)
{
    Init(format);
}

wxTarOutputStream::wxTarOutputStream(wxOutputStream *stream,
                                     wxTarFormat format /*=wxTAR_PAX*/,
                                     wxMBConv& conv     /*=wxConvLocal*/)
 :  wxArchiveOutputStream(stream, conv)
{
    Init(format);
}

void wxTarOutputStream::Init(wxTarFormat format)
{
    m_pos = wxInvalidOffset;
    m_maxpos = wxInvalidOffset;
    m_size = wxInvalidOffset;
    m_headpos = wxInvalidOffset;
    m_datapos = wxInvalidOffset;
    m_tarstart = wxInvalidOffset;
    m_tarsize = 0;
    m_pax = format == wxTAR_PAX;
    m_BlockingFactor = m_pax ? 10 : 20;
    m_chksum = 0;
    m_large = false;
    m_hdr = new wxTarHeaderBlock;
    m_hdr2 = NULL;
    m_extendedHdr = NULL;
    m_extendedSize = 0;
    m_lasterror = m_parent_o_stream->GetLastError();
}

wxTarOutputStream::~wxTarOutputStream()
{
    if (m_tarsize)
        Close();
    delete m_hdr;
    delete m_hdr2;
    delete [] m_extendedHdr;
}

bool wxTarOutputStream::PutNextEntry(wxTarEntry *entry)
{
    wxTarEntryPtr_ e(entry);

    if (!CloseEntry())
        return false;

    if (!m_tarsize) {
        wxLogNull nolog;
        m_tarstart = m_parent_o_stream->TellO();
    }

    if (m_tarstart != wxInvalidOffset)
        m_headpos = m_tarstart + m_tarsize;

    if (WriteHeaders(*e)) {
        m_pos = 0;
        m_maxpos = 0;
        m_size = GetDataSize(*e);
        if (m_tarstart != wxInvalidOffset)
            m_datapos = m_tarstart + m_tarsize;

        // types that are not allowd any data
        const char nodata[] = {
            wxTAR_LNKTYPE, wxTAR_SYMTYPE, wxTAR_CHRTYPE, wxTAR_BLKTYPE,
            wxTAR_DIRTYPE, wxTAR_FIFOTYPE, 0
        };
        int typeflag = e->GetTypeFlag();

        // pax does now allow data for wxTAR_LNKTYPE
        if (!m_pax || typeflag != wxTAR_LNKTYPE)
            if (strchr(nodata, typeflag) != NULL)
                CloseEntry();
    }

    return IsOk();
}

bool wxTarOutputStream::PutNextEntry(const wxString& name,
                                     const wxDateTime& dt,
                                     wxFileOffset size)
{
    return PutNextEntry(new wxTarEntry(name, dt, size));
}

bool wxTarOutputStream::PutNextDirEntry(const wxString& name,
                                        const wxDateTime& dt)
{
    wxTarEntry *entry = new wxTarEntry(name, dt);
    entry->SetIsDir();
    return PutNextEntry(entry);
}

bool wxTarOutputStream::PutNextEntry(wxArchiveEntry *entry)
{
    wxTarEntry *tarEntry = wxStaticCast(entry, wxTarEntry);
    if (!tarEntry)
        delete entry;
    return PutNextEntry(tarEntry);
}

bool wxTarOutputStream::CopyEntry(wxTarEntry *entry,
                                  wxTarInputStream& inputStream)
{
    if (PutNextEntry(entry))
        Write(inputStream);
    return IsOk() && inputStream.Eof();
}

bool wxTarOutputStream::CopyEntry(wxArchiveEntry *entry,
                                  wxArchiveInputStream& inputStream)
{
    if (PutNextEntry(entry))
        Write(inputStream);
    return IsOk() && inputStream.Eof();
}

bool wxTarOutputStream::CloseEntry()
{
    if (!IsOpened())
        return true;

    if (m_pos < m_maxpos) {
        wxASSERT(m_parent_o_stream->IsSeekable());
        m_parent_o_stream->SeekO(m_datapos + m_maxpos);
        m_lasterror = m_parent_o_stream->GetLastError();
        m_pos = m_maxpos;
    }

    if (IsOk()) {
        wxFileOffset size = RoundUpSize(m_pos);
        if (size > m_pos) {
            memset(m_hdr, 0, size - m_pos);
            m_parent_o_stream->Write(m_hdr, size - m_pos);
            m_lasterror = m_parent_o_stream->GetLastError();
        }
        m_tarsize += size;
    }

    if (IsOk() && m_pos != m_size)
        ModifyHeader();

    m_pos = wxInvalidOffset;
    m_maxpos = wxInvalidOffset;
    m_size = wxInvalidOffset;
    m_headpos = wxInvalidOffset;
    m_datapos = wxInvalidOffset;

    return IsOk();
}

bool wxTarOutputStream::Close()
{
    if (!CloseEntry())
        return false;

    memset(m_hdr, 0, sizeof(*m_hdr));
    int count = (RoundUpSize(m_tarsize + 2 * TAR_BLOCKSIZE, m_BlockingFactor)
                    - m_tarsize) / TAR_BLOCKSIZE;
    while (count--)
        m_parent_o_stream->Write(m_hdr, TAR_BLOCKSIZE);

    m_tarsize = 0;
    m_tarstart = wxInvalidOffset;
    m_lasterror = m_parent_o_stream->GetLastError();
    return IsOk();
}

bool wxTarOutputStream::WriteHeaders(wxTarEntry& entry)
{
    memset(m_hdr, 0, sizeof(*m_hdr));

    SetHeaderPath(entry.GetName(wxPATH_UNIX));

    SetHeaderNumber(TAR_MODE, entry.GetMode());
    SetHeaderNumber(TAR_UID, entry.GetUserId());
    SetHeaderNumber(TAR_GID, entry.GetGroupId());

    if (entry.GetSize() == wxInvalidOffset)
        entry.SetSize(0);
    m_large = !SetHeaderNumber(TAR_SIZE, entry.GetSize());

    SetHeaderDate(_T("mtime"), entry.GetDateTime());
    if (entry.GetAccessTime().IsValid())
        SetHeaderDate(_T("atime"), entry.GetAccessTime());
    if (entry.GetCreateTime().IsValid())
        SetHeaderDate(_T("ctime"), entry.GetCreateTime());

    *m_hdr->Get(TAR_TYPEFLAG) = char(entry.GetTypeFlag());

    strcpy(m_hdr->Get(TAR_MAGIC), USTAR_MAGIC);
    strcpy(m_hdr->Get(TAR_VERSION), USTAR_VERSION);

    SetHeaderString(TAR_LINKNAME, entry.GetLinkName());
    SetHeaderString(TAR_UNAME, entry.GetUserName());
    SetHeaderString(TAR_GNAME, entry.GetGroupName());

    if (~entry.GetDevMajor())
        SetHeaderNumber(TAR_DEVMAJOR, entry.GetDevMajor());
    if (~entry.GetDevMinor())
        SetHeaderNumber(TAR_DEVMINOR, entry.GetDevMinor());

    m_chksum = m_hdr->Sum();
    m_hdr->SetOctal(TAR_CHKSUM, m_chksum);
    if (!m_large)
        m_chksum -= m_hdr->SumField(TAR_SIZE);

    // The main header is now fully prepared so we know what extended headers
    // (if any) will be needed. Output any extended headers before writing
    // the main header.
    if (m_extendedHdr && *m_extendedHdr) {
        wxASSERT(m_pax);
        // the extended headers are written to the tar as a file entry,
        // so prepare a regular header block for the pseudo-file.
        if (!m_hdr2)
            m_hdr2 = new wxTarHeaderBlock;
        memset(m_hdr2, 0, sizeof(*m_hdr2));

        // an old tar that doesn't understand extended headers will
        // extract it as a file, so give these fields reasonable values
        // so that the user will have access to read and remove it.
        m_hdr2->SetPath(PaxHeaderPath(_T("%d/PaxHeaders.%p/%f"),
                                      entry.GetName(wxPATH_UNIX)), GetConv());
        m_hdr2->SetOctal(TAR_MODE, 0600);
        strcpy(m_hdr2->Get(TAR_UID), m_hdr->Get(TAR_UID));
        strcpy(m_hdr2->Get(TAR_GID), m_hdr->Get(TAR_GID));
        size_t length = strlen(m_extendedHdr);
        m_hdr2->SetOctal(TAR_SIZE, length);
        strcpy(m_hdr2->Get(TAR_MTIME), m_hdr->Get(TAR_MTIME));
        *m_hdr2->Get(TAR_TYPEFLAG) = 'x';
        strcpy(m_hdr2->Get(TAR_MAGIC), USTAR_MAGIC);
        strcpy(m_hdr2->Get(TAR_VERSION), USTAR_VERSION);
        strcpy(m_hdr2->Get(TAR_UNAME), m_hdr->Get(TAR_UNAME));
        strcpy(m_hdr2->Get(TAR_GNAME), m_hdr->Get(TAR_GNAME));

        m_hdr2->SetOctal(TAR_CHKSUM, m_hdr2->Sum());

        m_hdr2->Write(*m_parent_o_stream);
        m_tarsize += TAR_BLOCKSIZE;

        size_t rounded = RoundUpSize(length);
        memset(m_extendedHdr + length, 0, rounded - length);
        m_parent_o_stream->Write(m_extendedHdr, rounded);
        m_tarsize += rounded;

        *m_extendedHdr = 0;

        // update m_headpos which is used to seek back to fix up the file
        // length if it is not known in advance
        if (m_tarstart != wxInvalidOffset)
            m_headpos = m_tarstart + m_tarsize;
    }

    // if don't have extended headers just report error
    if (!m_badfit.empty()) {
        wxASSERT(!m_pax);
        wxLogWarning(_("%s did not fit the tar header for entry '%s'"),
                     m_badfit.c_str(), entry.GetName().c_str());
        m_badfit.clear();
    }

    m_hdr->Write(*m_parent_o_stream);
    m_tarsize += TAR_BLOCKSIZE;
    m_lasterror = m_parent_o_stream->GetLastError();

    return IsOk();
}

wxString wxTarOutputStream::PaxHeaderPath(const wxString& format,
                                          const wxString& path)
{
    wxString d = path.BeforeLast(_T('/'));
    wxString f = path.AfterLast(_T('/'));
    wxString ret;

    if (d.empty())
        d = _T(".");

    ret.reserve(format.length() + path.length() + 16);

    size_t begin = 0;
    size_t end;

    for (;;) {
        end = format.find('%', begin);
        if (end == wxString::npos || end + 1 >= format.length())
            break;
        ret << format.substr(begin, end - begin);
        switch (format[end + 1]) {
            case 'd': ret << d; break;
            case 'f': ret << f; break;
            case 'p': ret << wxGetProcessId(); break;
            case '%': ret << _T("%"); break;
        }
        begin = end + 2;
    }

    ret << format.substr(begin);

    return ret;
}

bool wxTarOutputStream::ModifyHeader()
{
    wxFileOffset originalPos = wxInvalidOffset;
    wxFileOffset sizePos = wxInvalidOffset;

    if (!m_large && m_headpos != wxInvalidOffset
            && m_parent_o_stream->IsSeekable())
    {
        wxLogNull nolog;
        originalPos = m_parent_o_stream->TellO();
        if (originalPos != wxInvalidOffset)
            sizePos =
                m_parent_o_stream->SeekO(m_headpos + m_hdr->Offset(TAR_SIZE));
    }

    if (sizePos == wxInvalidOffset || !m_hdr->SetOctal(TAR_SIZE, m_pos)) {
        wxLogError(_("incorrect size given for tar entry"));
        m_lasterror = wxSTREAM_WRITE_ERROR;
        return false;
    }

    m_chksum += m_hdr->SumField(TAR_SIZE);
    m_hdr->SetOctal(TAR_CHKSUM, m_chksum);
    wxFileOffset sumPos = m_headpos + m_hdr->Offset(TAR_CHKSUM);

    return
        m_hdr->WriteField(*m_parent_o_stream, TAR_SIZE) &&
        m_parent_o_stream->SeekO(sumPos) == sumPos &&
        m_hdr->WriteField(*m_parent_o_stream, TAR_CHKSUM) &&
        m_parent_o_stream->SeekO(originalPos) == originalPos;
}

void wxTarOutputStream::SetHeaderPath(const wxString& name)
{
    if (!m_hdr->SetPath(name, GetConv()) || (m_pax && !name.IsAscii()))
        SetExtendedHeader(_T("path"), name);
}

bool wxTarOutputStream::SetHeaderNumber(int id, wxTarNumber n)
{
    if (m_hdr->SetOctal(id, n)) {
        return true;
    } else {
        SetExtendedHeader(m_hdr->Name(id), wxLongLong(n).ToString());
        return false;
    }
}

void wxTarOutputStream::SetHeaderString(int id, const wxString& str)
{
    strncpy(m_hdr->Get(id), str.mb_str(GetConv()), m_hdr->Len(id));
    if (str.length() > m_hdr->Len(id))
        SetExtendedHeader(m_hdr->Name(id), str);
}

void wxTarOutputStream::SetHeaderDate(const wxString& key,
                                      const wxDateTime& datetime)
{
    wxLongLong ll = datetime.IsValid() ? datetime.GetValue() : wxLongLong(0);
    wxLongLong secs = ll / 1000L;

    if (key != _T("mtime")
        || !m_hdr->SetOctal(TAR_MTIME, wxTarNumber(secs.GetValue()))
        || secs <= 0 || secs >= 0x7fffffff)
    {
        wxString str;
        if (ll >= LONG_MIN && ll <= LONG_MAX) {
            str.Printf(_T("%g"), ll.ToLong() / 1000.0);
        } else {
            str = ll.ToString();
            str.insert(str.end() - 3, '.');
        }
        SetExtendedHeader(key, str);
    }
}

void wxTarOutputStream::SetExtendedHeader(const wxString& key,
                                          const wxString& value)
{
    if (m_pax) {
        const wxWX2WCbuf wide_key = key.wc_str(GetConv());
        const wxCharBuffer utf_key = wxConvUTF8.cWC2MB(wide_key);

        const wxWX2WCbuf wide_value = value.wc_str(GetConv());
        const wxCharBuffer utf_value = wxConvUTF8.cWC2MB(wide_value);

        // a small buffer to format the length field in
        char buf[32];
        // length of "99<space><key>=<value>\n"
        unsigned long length = strlen(utf_value) + strlen(utf_key) + 5;
        sprintf(buf, "%lu", length);
        // the length includes itself
        size_t lenlen = strlen(buf);
        if (lenlen != 2) {
            length += lenlen - 2;
            sprintf(buf, "%lu", length);
            if (strlen(buf) > lenlen)
                sprintf(buf, "%lu", ++length);
        }

        // reallocate m_extendedHdr if it's not big enough
        if (m_extendedSize < length) {
            size_t rounded = RoundUpSize(length);
            m_extendedSize <<= 1;
            if (rounded > m_extendedSize)
                m_extendedSize = rounded;
            char *oldHdr = m_extendedHdr;
            m_extendedHdr = new char[m_extendedSize];
            if (oldHdr) {
                strcpy(m_extendedHdr, oldHdr);
                delete oldHdr;
            } else {
                *m_extendedHdr = 0;
            }
        }

        // append the new record
        char *append = strchr(m_extendedHdr, 0);
        sprintf(append, "%s %s=%s\012", buf,
                (const char*)utf_key, (const char*)utf_value);
    }
    else {
        // if not pax then make a list of fields to report as errors
        if (!m_badfit.empty())
            m_badfit += _T(", ");
        m_badfit += key;
    }
}

void wxTarOutputStream::Sync()
{
    m_parent_o_stream->Sync();
}

wxFileOffset wxTarOutputStream::OnSysSeek(wxFileOffset pos, wxSeekMode mode)
{
    if (!IsOpened()) {
        wxLogError(_("tar entry not open"));
        m_lasterror = wxSTREAM_WRITE_ERROR;
    }
    if (!IsOk() || m_datapos == wxInvalidOffset)
        return wxInvalidOffset;

    switch (mode) {
        case wxFromStart:   break;
        case wxFromCurrent: pos += m_pos; break;
        case wxFromEnd:     pos += m_maxpos; break;
    }

    if (pos < 0 || m_parent_o_stream->SeekO(m_datapos + pos) == wxInvalidOffset)
        return wxInvalidOffset;

    m_pos = pos;
    return m_pos;
}

size_t wxTarOutputStream::OnSysWrite(const void *buffer, size_t size)
{
    if (!IsOpened()) {
        wxLogError(_("tar entry not open"));
        m_lasterror = wxSTREAM_WRITE_ERROR;
    }
    if (!IsOk() || !size)
        return 0;

    size_t lastwrite = m_parent_o_stream->Write(buffer, size).LastWrite();
    m_pos += lastwrite;
    if (m_pos > m_maxpos)
        m_maxpos = m_pos;

    if (lastwrite != size)
        m_lasterror = wxSTREAM_WRITE_ERROR;

    return lastwrite;
}

#endif // wxUSE_TARSTREAM
