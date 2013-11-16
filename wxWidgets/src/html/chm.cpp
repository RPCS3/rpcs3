/////////////////////////////////////////////////////////////////////////////
// Name:        src/html/chm.cpp
// Purpose:     CHM (Help) support for wxHTML
// Author:      Markus Sinner
// Copyright:   (c) 2003 Herd Software Development
// CVS-ID:      $Id: chm.cpp 65784 2010-10-08 11:16:54Z MW $
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_LIBMSPACK

#include <mspack.h>

#ifndef WXPRECOMP
    #include "wx/intl.h"
    #include "wx/log.h"
    #include "wx/module.h"
#endif

#include "wx/filesys.h"
#include "wx/mstream.h"
#include "wx/wfstream.h"

#include "wx/html/forcelnk.h"
FORCE_LINK_ME(wxhtml_chm_support)

// ----------------------------------------------------------------------------
/// wxChmTools
/// <p>
/// this class is used to abstract access to CHM-Archives
/// with library mspack written by Stuart Caie
/// http://www.kyz.uklinux.net/libmspack/
// ----------------------------------------------------------------------------
class wxChmTools
{
public:
    /// constructor
    wxChmTools(const wxFileName &archive);
    /// destructor
    ~wxChmTools();

    /// Generate error-string for error-code
    static const wxString ChmErrorMsg(int error);

    /// get an array of archive-member-filenames
    const wxArrayString *GetFileNames()
    {
        return m_fileNames;
    };

    /// get the name of the archive representated by this class
    const wxString GetArchiveName()
    {
        return m_chmFileName;
    };

    /// Find a file in the archive
    const wxString Find(const wxString& pattern,
                        const wxString& startfrom = wxEmptyString);

    /// Extract a file in the archive into a file
    size_t Extract(const wxString& pattern, const wxString& filename);

    /// check archive for a file
    bool Contains(const wxString& pattern);

    /// get a string for the last error which occurred
    const wxString GetLastErrorMessage();

    /// Last Error
    int m_lasterror;

private:
    // these vars are used by FindFirst/Next:
    wxString m_chmFileName;
    char *m_chmFileNameANSI;

    /// mspack-pointer to mschmd_header
    struct mschmd_header *m_archive;
    /// mspack-pointer to mschm_decompressor
    struct mschm_decompressor *m_decompressor;

    /// Array of filenames in archive
    wxArrayString * m_fileNames;

    /// Internal function to get filepointer
    struct mschmd_file *GetMschmdFile(const wxString& pattern);
};


/***
 * constructor
 *
 * @param archive The filename of the archive to open
 */
wxChmTools::wxChmTools(const wxFileName &archive)
{
    m_chmFileName = archive.GetFullPath();

    wxASSERT_MSG( !m_chmFileName.empty(), _T("empty archive name") );

    m_archive = NULL;
    m_decompressor = NULL;
    m_fileNames = NULL;
    m_lasterror = 0;

    struct mschmd_header *chmh;
    struct mschm_decompressor *chmd;
    struct mschmd_file *file;

    // Create decompressor
    chmd =  mspack_create_chm_decompressor(NULL);
    m_decompressor = (struct mschm_decompressor *) chmd;

    // NB: we must make a copy of the string because chmd->open won't call
    //     strdup() [libmspack-20030726], which would cause crashes in
    //     Unicode build when mb_str() returns temporary buffer
    m_chmFileNameANSI = strdup((const char*)m_chmFileName.mb_str(wxConvFile));

    // Open the archive and store it in class:
    if ( (chmh = chmd->open(chmd, (char*)m_chmFileNameANSI)) )
    {
        m_archive = chmh;

        // Create Filenamearray
        m_fileNames = new wxArrayString;

        // Store Filenames in array
        for (file = chmh->files; file; file = file->next)
        {
            m_fileNames->Add(wxString::FromAscii(file->filename));
        }
    }
    else
    {
        wxLogError(_("Failed to open CHM archive '%s'."),
                   archive.GetFullPath().c_str());
        m_lasterror = (chmd->last_error(chmd));
        return;
    }
}


/***
 * Destructor
 */
wxChmTools::~wxChmTools()
{
    struct mschm_decompressor *chmd = m_decompressor;
    struct mschmd_header      *chmh = m_archive;

    delete m_fileNames;

    // Close Archive
    if (chmh && chmd)
        chmd->close(chmd, chmh);

    free(m_chmFileNameANSI);

    // Destroy Decompressor
    if (chmd)
        mspack_destroy_chm_decompressor(chmd);
}



/**
 * Checks if the given pattern matches to any
 * filename stored in archive
 *
 * @param  pattern The filename pattern, may include '*' and/or '?'
 * @return true, if any file matching pattern has been found,
 *         false if not
 */
bool wxChmTools::Contains(const wxString& pattern)
{
    int count;
    wxString pattern_tmp = wxString(pattern).MakeLower();

    // loop through filearay
    if ( m_fileNames && (count = m_fileNames->GetCount()) > 0 )
    {
        for (int i = 0; i < count; i++)
        {
            wxString tmp = m_fileNames->Item(i).MakeLower();
            if ( tmp.Matches(pattern_tmp) || tmp.Mid(1).Matches(pattern_tmp))
                return true;
        }
    }

    return false;
}



/**
 * Find()
 *
 * Finds the next file descibed by a pattern in the archive, starting
 * the file given by second parameter
 *
 * @param pattern   The file-pattern to search for. May contain '*' and/or '?'
 * @param startfrom The filename which the search should start after
 * @returns         The full pathname of the found file
 */
const wxString wxChmTools::Find(const wxString& pattern,
                                const wxString& startfrom)
{
    int count;
    wxString tmp;
    wxString pattern_tmp(pattern);
    wxString startfrom_tmp(startfrom);
    pattern_tmp.MakeLower();
    startfrom_tmp.MakeLower();

    if ( m_fileNames && (count = m_fileNames->GetCount()) > 0 )
    {
        for (int i = 0; i < count; i++)
        {
            tmp = m_fileNames->Item(i).MakeLower();
            // if we find the string where the search should began
            if ( tmp.Matches(startfrom_tmp) ||
                 tmp.Mid(1).Matches(startfrom_tmp) )
                continue;
            if ( tmp.Matches(pattern_tmp) ||
                 tmp.Mid(1).Matches(pattern_tmp) )
            {
                return tmp;
            }
        }
    }

    return wxEmptyString;
}


/**
 * Extract ()
 *
 * extracts the first hit of pattern to the given position
 *
 * @param pattern  A filename pattern (may contain * and ? chars)
 * @param filename The FileName where to temporary extract the file to
 * @return 0 at no file extracted<br>
 *         number of bytes extracted else
 */
size_t wxChmTools::Extract(const wxString& pattern, const wxString& filename)
{
    struct mschm_decompressor *d = m_decompressor;
    struct mschmd_header      *h = m_archive;
    struct mschmd_file        *f;

    wxString tmp;
    wxString pattern_tmp = (wxString(pattern)).MakeLower();

    for (f = h->files; f; f = f->next)
    {
        tmp = wxString::FromAscii(f->filename).MakeLower();
        if ( tmp.Matches(pattern_tmp) ||
             tmp.Mid(1).Matches(pattern_tmp) )
        {
            // ignore leading '/'
            if (d->extract(d, f,
                           (char*)(const char*)filename.mb_str(wxConvFile)))
            {
                // Error
                m_lasterror = d->last_error(d);
                wxLogError(_("Could not extract %s into %s: %s"),
                           wxString::FromAscii(f->filename).c_str(),
                           filename.c_str(),
                           ChmErrorMsg(m_lasterror).c_str());
                return 0;
            }
            else
            {
                return (size_t) f->length;
            }
        }
    }

    return 0;
}



/**
 * Find a file by pattern
 *
 * @param  pattern A filename pattern (may contain * and ? chars)
 * @return A pointer to the file (mschmd_file*)
 */
struct mschmd_file *wxChmTools::GetMschmdFile(const wxString& pattern_orig)
{
    struct mschmd_file *f;
    struct mschmd_header *h = (struct mschmd_header *) m_archive;
    wxString tmp;
    wxString pattern = wxString(pattern_orig).MakeLower();

    for (f = h->files; f; f = f->next)
    {
        tmp = wxString::FromAscii(f->filename).MakeLower();
        if ( tmp.Matches(pattern) || tmp.Mid(1).Matches(pattern) )
        {
            // ignore leading '/'
            return f;
        }
    }

    return NULL;
}

const wxString wxChmTools::GetLastErrorMessage()
{
    return ChmErrorMsg(m_lasterror);
}

const wxString wxChmTools::ChmErrorMsg(int error)
{
    switch (error)
    {
        case MSPACK_ERR_OK:
            return _("no error");
        case MSPACK_ERR_ARGS:
            return _("bad arguments to library function");
        case MSPACK_ERR_OPEN:
            return _("error opening file");
        case MSPACK_ERR_READ:
            return _("read error");
        case MSPACK_ERR_WRITE:
            return _("write error");
        case MSPACK_ERR_SEEK:
            return _("seek error");
        case MSPACK_ERR_NOMEMORY:
            return _("out of memory");
        case MSPACK_ERR_SIGNATURE:
            return _("bad signature");
        case MSPACK_ERR_DATAFORMAT:
            return _("error in data format");
        case MSPACK_ERR_CHECKSUM:
            return _("checksum error");
        case MSPACK_ERR_CRUNCH:
            return _("compression error");
        case MSPACK_ERR_DECRUNCH:
            return _("decompression error");
    }
    return _("unknown error");
}


// ---------------------------------------------------------------------------
/// wxChmInputStream
// ---------------------------------------------------------------------------

class wxChmInputStream : public wxInputStream
{
public:
    /// Constructor
    wxChmInputStream(const wxString& archive,
                     const wxString& file, bool simulate = false);
    /// Destructor
    virtual ~wxChmInputStream();

    /// Return the size of the accessed file in archive
    virtual size_t GetSize() const { return m_size; }
    /// End of Stream?
    virtual bool Eof() const;
    /// Set simulation-mode of HHP-File (if non is found)
    void SimulateHHP(bool sim) { m_simulateHHP = sim; }

protected:
    /// See wxInputStream
    virtual size_t OnSysRead(void *buffer, size_t bufsize);
    /// See wxInputStream
    virtual wxFileOffset OnSysSeek(wxFileOffset seek, wxSeekMode mode);
    /// See wxInputStream
    virtual wxFileOffset OnSysTell() const { return m_pos; }

private:
    size_t m_size;
    wxFileOffset m_pos;
    bool m_simulateHHP;

    char * m_content;
    wxInputStream * m_contentStream;

    void CreateHHPStream();
    bool CreateFileStream(const wxString& pattern);
    // this void* is handle of archive . I'm sorry it is void and not proper
    // type but I don't want to make unzip.h header public.


    // locates the file and returns a mspack_file *
    mspack_file *LocateFile(wxString filename);

    // should store pointer to current file
    mspack_file *m_file;

    // The Chm-Class for extracting the data
    wxChmTools *m_chm;

    wxString m_fileName;
};


/**
 * Constructor
 * @param archive  The name of the .chm archive. Remember that archive must
 *                 be local file accesible via fopen, fread functions!
 * @param filename The Name of the file to be extracted from archive
 * @param simulate if true than class should simulate .HHP-File based on #SYSTEM
 *                 if false than class does nothing if it doesnt find .hhp
 */
wxChmInputStream::wxChmInputStream(const wxString& archive,
                                   const wxString& filename, bool simulate)
    : wxInputStream()
{
    m_pos = 0;
    m_size = 0;
    m_content = NULL;
    m_contentStream = NULL;
    m_lasterror = wxSTREAM_NO_ERROR;
    m_chm = new wxChmTools (wxFileName(archive));
    m_file = NULL;
    m_fileName = wxString(filename).MakeLower();
    m_simulateHHP = simulate;

    if ( !m_chm->Contains(m_fileName) )
    {
        // if the file could not be located, but was *.hhp, than we create
        // the content of the hhp-file on the fly and store it for reading
        // by the application
        if ( m_fileName.Find(_T(".hhp")) != wxNOT_FOUND && m_simulateHHP )
        {
            // now we open an hhp-file
            CreateHHPStream();
        }
        else
        {
            wxLogError(_("Could not locate file '%s'."), filename.c_str());
            m_lasterror = wxSTREAM_READ_ERROR;
            return;
        }
    }
    else
    {   // file found
        CreateFileStream(m_fileName);
    }
}


wxChmInputStream::~wxChmInputStream()
{
    delete m_chm;

    delete m_contentStream;

    if (m_content)
    {
        free (m_content);
        m_content=NULL;
    }
}

bool wxChmInputStream::Eof() const
{
    return (m_content==NULL ||
            m_contentStream==NULL ||
            m_contentStream->Eof() ||
            m_pos>m_size);
}



size_t wxChmInputStream::OnSysRead(void *buffer, size_t bufsize)
{
    if ( m_pos >= m_size )
    {
        m_lasterror = wxSTREAM_EOF;
        return 0;
    }
    m_lasterror = wxSTREAM_NO_ERROR;

    // If the rest to read from the stream is less
    // than the buffer size, than only read the rest
    if ( m_pos + bufsize > m_size )
        bufsize = m_size - m_pos;

    m_contentStream->SeekI(m_pos);
    m_contentStream->Read(buffer, bufsize);
    m_pos +=bufsize;
    m_contentStream->SeekI(m_pos);
    return bufsize;
}




wxFileOffset wxChmInputStream::OnSysSeek(wxFileOffset seek, wxSeekMode mode)
{
    wxString mode_str = wxEmptyString;

    if ( !m_contentStream || m_contentStream->Eof() )
    {
        m_lasterror = wxSTREAM_EOF;
        return 0;
    }
    m_lasterror = wxSTREAM_NO_ERROR;

    wxFileOffset nextpos;

    switch ( mode )
    {
        case wxFromCurrent:
            nextpos = seek + m_pos;
            break;
        case wxFromStart:
            nextpos = seek;
            break;
        case wxFromEnd:
            nextpos = m_size - 1 + seek;
            break;
        default:
            nextpos = m_pos;
            break; /* just to fool compiler, never happens */
    }
    m_pos=nextpos;

    // Set current position on stream
    m_contentStream->SeekI(m_pos);
    return m_pos;
}



/**
 * Help Browser tries to read the contents of the
 * file by interpreting a .hhp file in the Archiv.
 * For .chm doesnt include such a file, we need
 * to rebuild the information based on stored
 * system-files.
 */
void
wxChmInputStream::CreateHHPStream()
{
    wxFileName file;
    bool topic = false;
    bool hhc = false;
    bool hhk = false;
    wxInputStream *i;
    wxMemoryOutputStream *out;
    const char *tmp;

    // Try to open the #SYSTEM-File and create the HHP File out of it
    // see http://bonedaddy.net/pabs3/chmspec/0.1.2/Internal.html#SYSTEM
    if ( ! m_chm->Contains(_T("/#SYSTEM")) )
    {
#ifdef DEBUG
        wxLogDebug(_("Archive doesnt contain #SYSTEM file"));
#endif
        return;
    }
    else
    {
        file = wxFileName(_T("/#SYSTEM"));
    }

    if ( CreateFileStream(_T("/#SYSTEM")) )
    {
        // New stream for writing a memory area to simulate the
        // .hhp-file
        out = new wxMemoryOutputStream();

        tmp = "[OPTIONS]\r\n";
        out->Write((const void *) tmp, strlen(tmp));

        wxUint16 code;
        wxUint16 len;
        void *buf;

        // use the actual stream for reading
        i = m_contentStream;

        /* Now read the contents, and try to get the needed information */

        // First 4 Bytes are Version information, skip
        i->SeekI(4);

        while (!i->Eof())
        {
            // Read #SYSTEM-Code and length
            i->Read(&code, 2);
            code = wxUINT16_SWAP_ON_BE( code ) ;
            i->Read(&len, 2);
            len = wxUINT16_SWAP_ON_BE( len ) ;
            // data
            buf = malloc(len);
            i->Read(buf, len);

            switch (code)
            {
                case 0: // CONTENTS_FILE
                    if (len)
                    {
                        tmp = "Contents file=";
                        hhc=true;
                    }
                    break;
                case 1: // INDEX_FILE
                    tmp = "Index file=";
                    hhk = true;
                    break;
                case 2: // DEFAULT_TOPIC
                    tmp = "Default Topic=";
                    topic = true;
                    break;
                case 3: // TITLE
                    tmp = "Title=";
                    break;
                //       case 6: // COMPILED_FILE
                //         tmp = "Compiled File=";
                //         break;
                case 7: // COMPILED_FILE
                    tmp = "Binary Index=YES\r\n";
                    out->Write( (const void *) tmp, strlen(tmp));
                    tmp = NULL;
                    break;
                case 4: // STRUCT SYSTEM INFO
                    tmp = NULL ;
                    if ( len >= 28 )
                    {
                        char *structptr = (char*) buf ;
                        // LCID at position 0
                        wxUint32 dummy = *((wxUint32 *)(structptr+0)) ;
                        wxUint32 lcid = wxUINT32_SWAP_ON_BE( dummy ) ;
                        char msg[64];
                        int len = sprintf(msg, "Language=0x%X\r\n", lcid) ;
                        if (len > 0)
                            out->Write(msg, len) ;
                    }
                    break ;
                default:
                    tmp=NULL;
            }

            if (tmp)
            {
                out->Write((const void *) tmp, strlen(tmp));
                out->Write(buf, strlen((char*)buf));
                out->Write("\r\n", 2);
            }

            free(buf);
            buf=NULL;
        }


        // Free the old data which wont be used any more
        delete m_contentStream;
        if (m_content)
            free (m_content);

        // Now add entries which are missing
        if ( !hhc && m_chm->Contains(_T("*.hhc")) )
        {
            tmp = "Contents File=*.hhc\r\n";
            out->Write((const void *) tmp, strlen(tmp));
        }

        if ( !hhk && m_chm->Contains(_T("*.hhk")) )
        {
            tmp = "Index File=*.hhk\r\n";
            out->Write((const void *) tmp, strlen(tmp));
        }

        // Now copy the Data from the memory
        out->SeekO(0, wxFromEnd);
        m_size = out->TellO();
        out->SeekO(0, wxFromStart);
        m_content = (char *) malloc (m_size+1);
        out->CopyTo(m_content, m_size);
        m_content[m_size]='\0';
        m_size++;
        m_contentStream = new wxMemoryInputStream(m_content, m_size);

        delete out;
    }
}


/**
 * Creates a Stream pointing to a virtual file in
 * the current archive
 */
bool wxChmInputStream::CreateFileStream(const wxString& pattern)
{
    wxFileInputStream * fin;
    wxString tmpfile = wxFileName::CreateTempFileName(_T("chmstrm"));

    if ( tmpfile.empty() )
    {
        wxLogError(_("Could not create temporary file '%s'"), tmpfile.c_str());
        return false;
    }

    // try to extract the file
    if ( m_chm->Extract(pattern, tmpfile) <= 0 )
    {
        wxLogError(_("Extraction of '%s' into '%s' failed."),
                   pattern.c_str(), tmpfile.c_str());
        if ( wxFileExists(tmpfile) )
            wxRemoveFile(tmpfile);
        return false;
    }
    else
    {
        // Open a filestream to extracted file
        fin = new wxFileInputStream(tmpfile);
        m_size = fin->GetSize();
        m_content = (char *) malloc(m_size+1);
        fin->Read(m_content, m_size);
        m_content[m_size]='\0';

        wxRemoveFile(tmpfile);

        delete fin;

        m_contentStream = new wxMemoryInputStream (m_content, m_size);

        return m_contentStream->IsOk();
    }
}



// ----------------------------------------------------------------------------
// wxChmFSHandler
// ----------------------------------------------------------------------------

class wxChmFSHandler : public wxFileSystemHandler
{
public:
    /// Constructor and Destructor
    wxChmFSHandler();
    virtual ~wxChmFSHandler();

    /// Is able to open location?
    virtual bool CanOpen(const wxString& location);
    /// Open a file
    virtual wxFSFile* OpenFile(wxFileSystem& fs, const wxString& location);
    /// Find first occurrence of spec
    virtual wxString FindFirst(const wxString& spec, int flags = 0);
    /// Find next occurrence of spec
    virtual wxString FindNext();

private:
    int m_lasterror;
    wxString m_pattern;
    wxString m_found;
    wxChmTools * m_chm;
};

wxChmFSHandler::wxChmFSHandler() : wxFileSystemHandler()
{
    m_lasterror=0;
    m_pattern=wxEmptyString;
    m_found=wxEmptyString;
    m_chm=NULL;
}

wxChmFSHandler::~wxChmFSHandler()
{
    if (m_chm)
        delete m_chm;
}

bool wxChmFSHandler::CanOpen(const wxString& location)
{
    wxString p = GetProtocol(location);
    return (p == _T("chm")) &&
           (GetProtocol(GetLeftLocation(location)) == _T("file"));
}

wxFSFile* wxChmFSHandler::OpenFile(wxFileSystem& WXUNUSED(fs),
                                   const wxString& location)
{
    wxString right = GetRightLocation(location);
    wxString left = GetLeftLocation(location);

    wxInputStream *s;

    int index;

    if ( GetProtocol(left) != _T("file") )
    {
        wxLogError(_("CHM handler currently supports only local files!"));
        return NULL;
    }

    // Work around javascript
    wxString tmp = wxString(right);
    if ( tmp.MakeLower().Contains(_T("javascipt")) && tmp.Contains(_T("\'")) )
    {
        right = right.AfterFirst(_T('\'')).BeforeLast(_T('\''));
    }

    // now work on the right location
    if (right.Contains(_T("..")))
    {
        wxFileName abs(right);
        abs.MakeAbsolute(_T("/"));
        right = abs.GetFullPath();
    }

    // a workaround for absolute links to root
    if ( (index=right.Index(_T("//"))) != wxNOT_FOUND )
    {
        right=wxString(right.Mid(index+1));
        wxLogWarning(_("Link contained '//', converted to absolute link."));
    }

    wxFileName leftFilename = wxFileSystem::URLToFileName(left);
    if (!leftFilename.FileExists())
        return NULL;

    // Open a stream to read the content of the chm-file
    s = new wxChmInputStream(leftFilename.GetFullPath(), right, true);

    wxString mime = GetMimeTypeFromExt(location);

    if ( s )
    {
        return new wxFSFile(s,
                            left + _T("#chm:") + right,
                            mime,
                            GetAnchor(location),
                            wxDateTime(leftFilename.GetModificationTime()));
    }

    delete s;
    return NULL;
}



/**
 * Doku see wxFileSystemHandler
 */
wxString wxChmFSHandler::FindFirst(const wxString& spec, int flags)
{
    wxString right = GetRightLocation(spec);
    wxString left = GetLeftLocation(spec);
    wxString nativename = wxFileSystem::URLToFileName(left).GetFullPath();

    if ( GetProtocol(left) != _T("file") )
    {
        wxLogError(_("CHM handler currently supports only local files!"));
        return wxEmptyString;
    }

    m_chm = new wxChmTools(wxFileName(nativename));
    m_pattern = right.AfterLast(_T('/'));

    wxString m_found = m_chm->Find(m_pattern);

    // now fake around hhp-files which are not existing in projects...
    if (m_found.empty() &&
        m_pattern.Contains(_T(".hhp")) &&
        !m_pattern.Contains(_T(".hhp.cached")))
    {
        m_found.Printf(_T("%s#chm:%s.hhp"),
                       left.c_str(), m_pattern.BeforeLast(_T('.')).c_str());
    }

    return m_found;

}



wxString wxChmFSHandler::FindNext()
{
    if (m_pattern.empty())
        return wxEmptyString;
    else
        return m_chm->Find(m_pattern, m_found);
}

// ---------------------------------------------------------------------------
// wxModule to register CHM handler
// ---------------------------------------------------------------------------

class wxChmSupportModule : public wxModule
{
    DECLARE_DYNAMIC_CLASS(wxChmSupportModule)

public:
    virtual bool OnInit()
    {
        wxFileSystem::AddHandler(new wxChmFSHandler);
        return true;
    }
    virtual void OnExit() {}
}
;

IMPLEMENT_DYNAMIC_CLASS(wxChmSupportModule, wxModule)

#endif // wxUSE_LIBMSPACK
