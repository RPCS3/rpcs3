/////////////////////////////////////////////////////////////////////////////
// Name:        wx/mimetype.h
// Purpose:     classes and functions to manage MIME types
// Author:      Vadim Zeitlin
// Modified by:
//  Chris Elliott (biol75@york.ac.uk) 5 Dec 00: write support for Win32
// Created:     23.09.98
// RCS-ID:      $Id: mimetype.h 61872 2009-09-09 22:37:05Z VZ $
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence (part of wxExtra library)
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_MIMETYPE_H_
#define _WX_MIMETYPE_H_

// ----------------------------------------------------------------------------
// headers and such
// ----------------------------------------------------------------------------

#include "wx/defs.h"

#if wxUSE_MIMETYPE

// the things we really need
#include "wx/string.h"
#include "wx/dynarray.h"
#include "wx/arrstr.h"

// fwd decls
class WXDLLIMPEXP_FWD_BASE wxIconLocation;
class WXDLLIMPEXP_FWD_BASE wxFileTypeImpl;
class WXDLLIMPEXP_FWD_BASE wxMimeTypesManagerImpl;

// these constants define the MIME informations source under UNIX and are used
// by wxMimeTypesManager::Initialize()
enum wxMailcapStyle
{
    wxMAILCAP_STANDARD = 1,
    wxMAILCAP_NETSCAPE = 2,
    wxMAILCAP_KDE = 4,
    wxMAILCAP_GNOME = 8,

    wxMAILCAP_ALL = 15
};

/*
    TODO: would it be more convenient to have this class?

class WXDLLIMPEXP_BASE wxMimeType : public wxString
{
public:
    // all string ctors here

    wxString GetType() const { return BeforeFirst(wxT('/')); }
    wxString GetSubType() const { return AfterFirst(wxT('/')); }

    void SetSubType(const wxString& subtype)
    {
        *this = GetType() + wxT('/') + subtype;
    }

    bool Matches(const wxMimeType& wildcard)
    {
        // implement using wxMimeTypesManager::IsOfType()
    }
};

*/

// wxMimeTypeCommands stores the verbs defined for the given MIME type with
// their values
class WXDLLIMPEXP_BASE wxMimeTypeCommands
{
public:
    wxMimeTypeCommands() {}

    wxMimeTypeCommands(const wxArrayString& verbs,
                       const wxArrayString& commands)
        : m_verbs(verbs),
          m_commands(commands)
    {
    }

    // add a new verb with the command or replace the old value
    void AddOrReplaceVerb(const wxString& verb, const wxString& cmd);
    void Add(const wxString& s)
    {
        m_verbs.Add(s.BeforeFirst(wxT('=')));
        m_commands.Add(s.AfterFirst(wxT('=')));
    }

    // access the commands
    size_t GetCount() const { return m_verbs.GetCount(); }
    const wxString& GetVerb(size_t n) const { return m_verbs[n]; }
    const wxString& GetCmd(size_t n) const { return m_commands[n]; }

    bool HasVerb(const wxString& verb) const
        { return m_verbs.Index(verb) != wxNOT_FOUND; }

    // returns empty string and wxNOT_FOUND in idx if no such verb
    wxString GetCommandForVerb(const wxString& verb, size_t *idx = NULL) const;

    // get a "verb=command" string
    wxString GetVerbCmd(size_t n) const;

private:
    wxArrayString m_verbs;
    wxArrayString m_commands;
};

// ----------------------------------------------------------------------------
// wxFileTypeInfo: static container of information accessed via wxFileType.
//
// This class is used with wxMimeTypesManager::AddFallbacks() and Associate()
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxFileTypeInfo
{
public:
    // ctors
        // a normal item
    wxFileTypeInfo(const wxChar *mimeType,
                   const wxChar *openCmd,
                   const wxChar *printCmd,
                   const wxChar *desc,
                   // the other parameters form a NULL terminated list of
                   // extensions
                   ...);

        // the array elements correspond to the parameters of the ctor above in
        // the same order
    wxFileTypeInfo(const wxArrayString& sArray);

        // invalid item - use this to terminate the array passed to
        // wxMimeTypesManager::AddFallbacks
    wxFileTypeInfo() { }

    // test if this object can be used
    bool IsValid() const { return !m_mimeType.empty(); }

    // setters
        // set the icon info
    void SetIcon(const wxString& iconFile, int iconIndex = 0)
    {
        m_iconFile = iconFile;
        m_iconIndex = iconIndex;
    }
        // set the short desc
    void SetShortDesc(const wxString& shortDesc) { m_shortDesc = shortDesc; }

    // accessors
        // get the MIME type
    const wxString& GetMimeType() const { return m_mimeType; }
        // get the open command
    const wxString& GetOpenCommand() const { return m_openCmd; }
        // get the print command
    const wxString& GetPrintCommand() const { return m_printCmd; }
        // get the short description (only used under Win32 so far)
    const wxString& GetShortDesc() const { return m_shortDesc; }
        // get the long, user visible description
    const wxString& GetDescription() const { return m_desc; }
        // get the array of all extensions
    const wxArrayString& GetExtensions() const { return m_exts; }
    size_t GetExtensionsCount() const {return m_exts.GetCount(); }
        // get the icon info
    const wxString& GetIconFile() const { return m_iconFile; }
    int GetIconIndex() const { return m_iconIndex; }

private:
    wxString m_mimeType,    // the MIME type in "type/subtype" form
             m_openCmd,     // command to use for opening the file (%s allowed)
             m_printCmd,    // command to use for printing the file (%s allowed)
             m_shortDesc,   // a short string used in the registry
             m_desc;        // a free form description of this file type

    // icon stuff
    wxString m_iconFile;    // the file containing the icon
    int      m_iconIndex;   // icon index in this file

    wxArrayString m_exts;   // the extensions which are mapped on this filetype


#if 0 // TODO
    // the additional (except "open" and "print") command names and values
    wxArrayString m_commandNames,
                  m_commandValues;
#endif // 0
};

WX_DECLARE_USER_EXPORTED_OBJARRAY(wxFileTypeInfo, wxArrayFileTypeInfo,
                                  WXDLLIMPEXP_BASE);

// ----------------------------------------------------------------------------
// wxFileType: gives access to all information about the files of given type.
//
// This class holds information about a given "file type". File type is the
// same as MIME type under Unix, but under Windows it corresponds more to an
// extension than to MIME type (in fact, several extensions may correspond to a
// file type). This object may be created in many different ways and depending
// on how it was created some fields may be unknown so the return value of all
// the accessors *must* be checked!
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxFileType
{
friend class WXDLLIMPEXP_FWD_BASE wxMimeTypesManagerImpl;  // it has access to m_impl

public:
    // An object of this class must be passed to Get{Open|Print}Command. The
    // default implementation is trivial and doesn't know anything at all about
    // parameters, only filename and MIME type are used (so it's probably ok for
    // Windows where %{param} is not used anyhow)
    class MessageParameters
    {
    public:
        // ctors
        MessageParameters() { }
        MessageParameters(const wxString& filename,
                          const wxString& mimetype = wxEmptyString)
            : m_filename(filename), m_mimetype(mimetype) { }

        // accessors (called by GetOpenCommand)
            // filename
        const wxString& GetFileName() const { return m_filename; }
            // mime type
        const wxString& GetMimeType() const { return m_mimetype; }

        // override this function in derived class
        virtual wxString GetParamValue(const wxString& WXUNUSED(name)) const
            { return wxEmptyString; }

        // virtual dtor as in any base class
        virtual ~MessageParameters() { }

    protected:
        wxString m_filename, m_mimetype;
    };

    // ctor from static data
    wxFileType(const wxFileTypeInfo& ftInfo);

    // accessors: all of them return true if the corresponding information
    // could be retrieved/found, false otherwise (and in this case all [out]
    // parameters are unchanged)
        // return the MIME type for this file type
    bool GetMimeType(wxString *mimeType) const;
    bool GetMimeTypes(wxArrayString& mimeTypes) const;
        // fill passed in array with all extensions associated with this file
        // type
    bool GetExtensions(wxArrayString& extensions);
        // get the icon corresponding to this file type and of the given size
    bool GetIcon(wxIconLocation *iconloc) const;
    bool GetIcon(wxIconLocation *iconloc,
                 const MessageParameters& params) const;
        // get a brief file type description ("*.txt" => "text document")
    bool GetDescription(wxString *desc) const;

    // get the command to be used to open/print the given file.
        // get the command to execute the file of given type
    bool GetOpenCommand(wxString *openCmd,
                        const MessageParameters& params) const;
        // a simpler to use version of GetOpenCommand() -- it only takes the
        // filename and returns an empty string on failure
    wxString GetOpenCommand(const wxString& filename) const;
        // get the command to print the file of given type
    bool GetPrintCommand(wxString *printCmd,
                         const MessageParameters& params) const;


        // return the number of commands defined for this file type, 0 if none
    size_t GetAllCommands(wxArrayString *verbs, wxArrayString *commands,
                          const wxFileType::MessageParameters& params) const;

    // set an arbitrary command, ask confirmation if it already exists and
    // overwriteprompt is true
    bool SetCommand(const wxString& cmd, const wxString& verb,
        bool overwriteprompt = true);

    bool SetDefaultIcon(const wxString& cmd = wxEmptyString, int index = 0);


    // remove the association for this filetype from the system MIME database:
    // notice that it will only work if the association is defined in the user
    // file/registry part, we will never modify the system-wide settings
    bool Unassociate();

    // operations
        // expand a string in the format of GetOpenCommand (which may contain
        // '%s' and '%t' format specificators for the file name and mime type
        // and %{param} constructions).
    static wxString ExpandCommand(const wxString& command,
                                  const MessageParameters& params);

    // dtor (not virtual, shouldn't be derived from)
    ~wxFileType();

private:
    // default ctor is private because the user code never creates us
    wxFileType();

    // no copy ctor/assignment operator
    wxFileType(const wxFileType&);
    wxFileType& operator=(const wxFileType&);

    // the static container of wxFileType data: if it's not NULL, it means that
    // this object is used as fallback only
    const wxFileTypeInfo *m_info;

    // the object which implements the real stuff like reading and writing
    // to/from system MIME database
    wxFileTypeImpl *m_impl;
};

//----------------------------------------------------------------------------
// wxMimeTypesManagerFactory
//----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxMimeTypesManagerFactory
{
public:
    wxMimeTypesManagerFactory() {}
    virtual ~wxMimeTypesManagerFactory() {}

    virtual wxMimeTypesManagerImpl *CreateMimeTypesManagerImpl();

    static void Set( wxMimeTypesManagerFactory *factory );
    static wxMimeTypesManagerFactory *Get();
    
private:
    static wxMimeTypesManagerFactory *m_factory;
};

// ----------------------------------------------------------------------------
// wxMimeTypesManager: interface to system MIME database.
//
// This class accesses the information about all known MIME types and allows
// the application to retrieve information (including how to handle data of
// given type) about them.
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxMimeTypesManager
{
public:
    // static helper functions
    // -----------------------

        // check if the given MIME type is the same as the other one: the
        // second argument may contain wildcards ('*'), but not the first. If
        // the types are equal or if the mimeType matches wildcard the function
        // returns true, otherwise it returns false
    static bool IsOfType(const wxString& mimeType, const wxString& wildcard);

    // ctor
    wxMimeTypesManager();

    // NB: the following 2 functions are for Unix only and don't do anything
    //     elsewhere

    // loads data from standard files according to the mailcap styles
    // specified: this is a bitwise OR of wxMailcapStyle values
    //
    // use the extraDir parameter if you want to look for files in another
    // directory
    void Initialize(int mailcapStyle = wxMAILCAP_ALL,
                    const wxString& extraDir = wxEmptyString);

    // and this function clears all the data from the manager
    void ClearData();

    // Database lookup: all functions return a pointer to wxFileType object
    // whose methods may be used to query it for the information you're
    // interested in. If the return value is !NULL, caller is responsible for
    // deleting it.
        // get file type from file extension
    wxFileType *GetFileTypeFromExtension(const wxString& ext);
        // get file type from MIME type (in format <category>/<format>)
    wxFileType *GetFileTypeFromMimeType(const wxString& mimeType);

    // other operations: return true if there were no errors or false if there
    // were some unrecognized entries (the good entries are always read anyhow)
    //
    // FIXME: These ought to be private ??

        // read in additional file (the standard ones are read automatically)
        // in mailcap format (see mimetype.cpp for description)
        //
        // 'fallback' parameter may be set to true to avoid overriding the
        // settings from other, previously parsed, files by this one: normally,
        // the files read most recently would override the older files, but with
        // fallback == true this won't happen

    bool ReadMailcap(const wxString& filename, bool fallback = false);
        // read in additional file in mime.types format
    bool ReadMimeTypes(const wxString& filename);

    // enumerate all known MIME types
    //
    // returns the number of retrieved file types
    size_t EnumAllFileTypes(wxArrayString& mimetypes);

    // these functions can be used to provide default values for some of the
    // MIME types inside the program itself (you may also use
    // ReadMailcap(filenameWithDefaultTypes, true /* use as fallback */) to
    // achieve the same goal, but this requires having this info in a file).
    //
    // The filetypes array should be terminated by either NULL entry or an
    // invalid wxFileTypeInfo (i.e. the one created with default ctor)
    void AddFallbacks(const wxFileTypeInfo *filetypes);
    void AddFallback(const wxFileTypeInfo& ft) { m_fallbacks.Add(ft); }

    // create or remove associations

        // create a new association using the fields of wxFileTypeInfo (at least
        // the MIME type and the extension should be set)
        // if the other fields are empty, the existing values should be left alone
    wxFileType *Associate(const wxFileTypeInfo& ftInfo);

        // undo Associate()
    bool Unassociate(wxFileType *ft) ;

    // dtor (not virtual, shouldn't be derived from)
    ~wxMimeTypesManager();

private:
    // no copy ctor/assignment operator
    wxMimeTypesManager(const wxMimeTypesManager&);
    wxMimeTypesManager& operator=(const wxMimeTypesManager&);

    // the fallback info which is used if the information is not found in the
    // real system database
    wxArrayFileTypeInfo m_fallbacks;

    // the object working with the system MIME database
    wxMimeTypesManagerImpl *m_impl;

    // if m_impl is NULL, create one
    void EnsureImpl();

    friend class wxMimeTypeCmnModule;
};


// ----------------------------------------------------------------------------
// global variables
// ----------------------------------------------------------------------------

// the default mime manager for wxWidgets programs
extern WXDLLIMPEXP_DATA_BASE(wxMimeTypesManager *) wxTheMimeTypesManager;

#endif // wxUSE_MIMETYPE

#endif
  //_WX_MIMETYPE_H_
