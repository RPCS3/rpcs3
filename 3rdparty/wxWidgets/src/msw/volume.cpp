///////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/volume.cpp
// Purpose:     wxFSVolume - encapsulates system volume information
// Author:      George Policello
// Modified by:
// Created:     28 Jan 02
// RCS-ID:      $Id: volume.cpp 41903 2006-10-10 17:37:02Z PC $
// Copyright:   (c) 2002 George Policello
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_FSVOLUME

#include "wx/volume.h"

#ifndef WX_PRECOMP
    #if wxUSE_GUI
        #include "wx/icon.h"
    #endif
    #include "wx/intl.h"
    #include "wx/hashmap.h"
#endif // WX_PRECOMP

#include "wx/dir.h"
#include "wx/dynlib.h"
#include "wx/arrimpl.cpp"

#include <shellapi.h>
#include <shlobj.h>
#include "wx/msw/missing.h"

#if wxUSE_BASE

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Dynamic library function defs.
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#if wxUSE_DYNLIB_CLASS
static wxDynamicLibrary s_mprLib;
#endif

typedef DWORD (WINAPI* WNetOpenEnumPtr)(DWORD, DWORD, DWORD, LPNETRESOURCE, LPHANDLE);
typedef DWORD (WINAPI* WNetEnumResourcePtr)(HANDLE, LPDWORD, LPVOID, LPDWORD);
typedef DWORD (WINAPI* WNetCloseEnumPtr)(HANDLE);

static WNetOpenEnumPtr s_pWNetOpenEnum;
static WNetEnumResourcePtr s_pWNetEnumResource;
static WNetCloseEnumPtr s_pWNetCloseEnum;

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Globals/Statics
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
static long s_cancelSearch = FALSE;

struct FileInfo
{
    FileInfo(unsigned flag=0, wxFSVolumeKind type=wxFS_VOL_OTHER) :
        m_flags(flag), m_type(type) {}

    FileInfo(const FileInfo& other) { *this = other; }
    FileInfo& operator=(const FileInfo& other)
    {
        m_flags = other.m_flags;
        m_type = other.m_type;
        return *this;
    }

    unsigned m_flags;
    wxFSVolumeKind m_type;
};
WX_DECLARE_STRING_HASH_MAP(FileInfo, FileInfoMap);
// Cygwin bug (?) destructor for global s_fileInfo is called twice...
static FileInfoMap& GetFileInfoMap()
{
    static FileInfoMap s_fileInfo(25);

    return s_fileInfo;
}
#define s_fileInfo (GetFileInfoMap())

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Local helper functions.
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//=============================================================================
// Function: GetBasicFlags
// Purpose: Set basic flags, primarily wxFS_VOL_REMOTE and wxFS_VOL_REMOVABLE.
// Notes: - Local and mapped drives are mounted by definition.  We have no
//          way to determine mounted status of network drives, so assume that
//          all drives are mounted, and let the caller decide otherwise.
//        - Other flags are 'best guess' from type of drive.  The system will
//          not report the file attributes with any degree of accuracy.
//=============================================================================
static unsigned GetBasicFlags(const wxChar* filename)
{
    unsigned flags = wxFS_VOL_MOUNTED;

    //----------------------------------
    // 'Best Guess' based on drive type.
    //----------------------------------
    wxFSVolumeKind type;
    switch(GetDriveType(filename))
    {
    case DRIVE_FIXED:
        type = wxFS_VOL_DISK;
        break;

    case DRIVE_REMOVABLE:
        flags |= wxFS_VOL_REMOVABLE;
        type = wxFS_VOL_FLOPPY;
        break;

    case DRIVE_CDROM:
        flags |= wxFS_VOL_REMOVABLE | wxFS_VOL_READONLY;
        type = wxFS_VOL_CDROM;
        break;

    case DRIVE_REMOTE:
        flags |= wxFS_VOL_REMOTE;
        type = wxFS_VOL_NETWORK;
        break;

    case DRIVE_NO_ROOT_DIR:
        flags &= ~wxFS_VOL_MOUNTED;
        type = wxFS_VOL_OTHER;
        break;

    default:
        type = wxFS_VOL_OTHER;
        break;
    }

    //-----------------------------------------------------------------------
    // The following most likely will not modify anything not set above,
    // and will not work at all for network shares or empty CD ROM drives.
    // But it is a good check if the Win API ever gets better about reporting
    // this information.
    //-----------------------------------------------------------------------
    SHFILEINFO fi;
    long rc;
    rc = SHGetFileInfo(filename, 0, &fi, sizeof(fi), SHGFI_ATTRIBUTES );
    if (!rc)
    {
        wxLogError(_("Cannot read typename from '%s'!"), filename);
    }
    else
    {
        if (fi.dwAttributes & SFGAO_READONLY)
            flags |= wxFS_VOL_READONLY;
        if (fi.dwAttributes & SFGAO_REMOVABLE)
            flags |= wxFS_VOL_REMOVABLE;
    }

    //------------------
    // Flags are cached.
    //------------------
    s_fileInfo[filename] = FileInfo(flags, type);

    return flags;
} // GetBasicFlags

//=============================================================================
// Function: FilteredAdd
// Purpose: Add a file to the list if it meets the filter requirement.
// Notes: - See GetBasicFlags for remarks about the Mounted flag.
//=============================================================================
static bool FilteredAdd(wxArrayString& list, const wxChar* filename,
                        unsigned flagsSet, unsigned flagsUnset)
{
    bool accept = true;
    unsigned flags = GetBasicFlags(filename);

    if (flagsSet & wxFS_VOL_MOUNTED && !(flags & wxFS_VOL_MOUNTED))
        accept = false;
    else if (flagsUnset & wxFS_VOL_MOUNTED && (flags & wxFS_VOL_MOUNTED))
        accept = false;
    else if (flagsSet & wxFS_VOL_REMOVABLE && !(flags & wxFS_VOL_REMOVABLE))
        accept = false;
    else if (flagsUnset & wxFS_VOL_REMOVABLE && (flags & wxFS_VOL_REMOVABLE))
        accept = false;
    else if (flagsSet & wxFS_VOL_READONLY && !(flags & wxFS_VOL_READONLY))
        accept = false;
    else if (flagsUnset & wxFS_VOL_READONLY && (flags & wxFS_VOL_READONLY))
        accept = false;
    else if (flagsSet & wxFS_VOL_REMOTE && !(flags & wxFS_VOL_REMOTE))
        accept = false;
    else if (flagsUnset & wxFS_VOL_REMOTE && (flags & wxFS_VOL_REMOTE))
        accept = false;

    // Add to the list if passed the filter.
    if (accept)
        list.Add(filename);

    return accept;
} // FilteredAdd

//=============================================================================
// Function: BuildListFromNN
// Purpose: Append or remove items from the list
// Notes: - There is no way to find all disconnected NN items, or even to find
//          all items while determining which are connected and not.  So this
//          function will find either all items or connected items.
//=============================================================================
static void BuildListFromNN(wxArrayString& list, NETRESOURCE* pResSrc,
                            unsigned flagsSet, unsigned flagsUnset)
{
    HANDLE hEnum;
    int rc;

    //-----------------------------------------------
    // Scope may be all drives or all mounted drives.
    //-----------------------------------------------
    unsigned scope = RESOURCE_GLOBALNET;
    if (flagsSet & wxFS_VOL_MOUNTED)
        scope = RESOURCE_CONNECTED;

    //----------------------------------------------------------------------
    // Enumerate all items, adding only non-containers (ie. network shares).
    // Containers cause a recursive call to this function for their own
    // enumeration.
    //----------------------------------------------------------------------
    if (rc = s_pWNetOpenEnum(scope, RESOURCETYPE_DISK, 0, pResSrc, &hEnum), rc == NO_ERROR)
    {
        DWORD count = 1;
        DWORD size = 256;
        NETRESOURCE* pRes = (NETRESOURCE*)malloc(size);
        memset(pRes, 0, sizeof(NETRESOURCE));
        while (rc = s_pWNetEnumResource(hEnum, &count, pRes, &size), rc == NO_ERROR || rc == ERROR_MORE_DATA)
        {
            if (s_cancelSearch)
                break;

            if (rc == ERROR_MORE_DATA)
            {
                pRes = (NETRESOURCE*)realloc(pRes, size);
                count = 1;
            }
            else if (count == 1)
            {
                // Enumerate the container.
                if (pRes->dwUsage & RESOURCEUSAGE_CONTAINER)
                {
                    BuildListFromNN(list, pRes, flagsSet, flagsUnset);
                }

                // Add the network share.
                else
                {
                    wxString filename(pRes->lpRemoteName);

                    if (!filename.empty())
                    {
                        if (filename.Last() != '\\')
                            filename.Append('\\');

                        // The filter function will not know mounted from unmounted, and neither do we unless
                        // we are iterating using RESOURCE_CONNECTED, in which case they all are mounted.
                        // Volumes on disconnected servers, however, will correctly show as unmounted.
                        FilteredAdd(list, filename, flagsSet, flagsUnset&~wxFS_VOL_MOUNTED);
                        if (scope == RESOURCE_GLOBALNET)
                            s_fileInfo[filename].m_flags &= ~wxFS_VOL_MOUNTED;
                    }
                }
            }
            else if (count == 0)
                break;
        }
        free(pRes);
        s_pWNetCloseEnum(hEnum);
    }
} // BuildListFromNN

//=============================================================================
// Function: CompareFcn
// Purpose: Used to sort the NN list alphabetically, case insensitive.
//=============================================================================
static int CompareFcn(const wxString& first, const wxString& second)
{
    return wxStricmp(first.c_str(), second.c_str());
} // CompareFcn

//=============================================================================
// Function: BuildRemoteList
// Purpose: Append Network Neighborhood items to the list.
// Notes: - Mounted gets transalated into Connected.  FilteredAdd is told
//          to ignore the Mounted flag since we need to handle it in a weird
//          way manually.
//        - The resulting list is sorted alphabetically.
//=============================================================================
static bool BuildRemoteList(wxArrayString& list, NETRESOURCE* pResSrc,
                            unsigned flagsSet, unsigned flagsUnset)
{
    // NN query depends on dynamically loaded library.
    if (!s_pWNetOpenEnum || !s_pWNetEnumResource || !s_pWNetCloseEnum)
    {
        wxLogError(_("Failed to load mpr.dll."));
        return false;
    }

    // Don't waste time doing the work if the flags conflict.
    if (flagsSet & wxFS_VOL_MOUNTED && flagsUnset & wxFS_VOL_MOUNTED)
        return false;

    //----------------------------------------------
    // Generate the list according to the flags set.
    //----------------------------------------------
    BuildListFromNN(list, pResSrc, flagsSet, flagsUnset);
    list.Sort(CompareFcn);

    //-------------------------------------------------------------------------
    // If mounted only is requested, then we only need one simple pass.
    // Otherwise, we need to build a list of all NN volumes and then apply the
    // list of mounted drives to it.
    //-------------------------------------------------------------------------
    if (!(flagsSet & wxFS_VOL_MOUNTED))
    {
        // generate.
        wxArrayString mounted;
        BuildListFromNN(mounted, pResSrc, flagsSet | wxFS_VOL_MOUNTED, flagsUnset & ~wxFS_VOL_MOUNTED);
        mounted.Sort(CompareFcn);

        // apply list from bottom to top to preserve indexes if removing items.
        ssize_t iList = list.GetCount()-1;
        for (ssize_t iMounted = mounted.GetCount()-1; iMounted >= 0 && iList >= 0; iMounted--)
        {
            int compare;
            wxString all(list[iList]);
            wxString mount(mounted[iMounted]);

            while (compare =
                     wxStricmp(list[iList].c_str(), mounted[iMounted].c_str()),
                   compare > 0 && iList >= 0)
            {
                iList--;
                all = list[iList];
            }


            if (compare == 0)
            {
                // Found the element.  Remove it or mark it mounted.
                if (flagsUnset & wxFS_VOL_MOUNTED)
                    list.RemoveAt(iList);
                else
                    s_fileInfo[list[iList]].m_flags |= wxFS_VOL_MOUNTED;

            }

            iList--;
        }
    }

    return true;
} // BuildRemoteList

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// wxFSVolume
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//=============================================================================
// Function: GetVolumes
// Purpose: Generate and return a list of all volumes (drives) available.
// Notes:
//=============================================================================
wxArrayString wxFSVolumeBase::GetVolumes(int flagsSet, int flagsUnset)
{
    ::InterlockedExchange(&s_cancelSearch, FALSE);     // reset

#if wxUSE_DYNLIB_CLASS
    if (!s_mprLib.IsLoaded() && s_mprLib.Load(_T("mpr.dll")))
    {
#ifdef UNICODE
        s_pWNetOpenEnum = (WNetOpenEnumPtr)s_mprLib.GetSymbol(_T("WNetOpenEnumW"));
        s_pWNetEnumResource = (WNetEnumResourcePtr)s_mprLib.GetSymbol(_T("WNetEnumResourceW"));
#else
        s_pWNetOpenEnum = (WNetOpenEnumPtr)s_mprLib.GetSymbol(_T("WNetOpenEnumA"));
        s_pWNetEnumResource = (WNetEnumResourcePtr)s_mprLib.GetSymbol(_T("WNetEnumResourceA"));
#endif
        s_pWNetCloseEnum = (WNetCloseEnumPtr)s_mprLib.GetSymbol(_T("WNetCloseEnum"));
    }
#endif

    wxArrayString list;

    //-------------------------------
    // Local and mapped drives first.
    //-------------------------------
    // Allocate the required space for the API call.
    const DWORD chars = GetLogicalDriveStrings(0, NULL);
    TCHAR* buf = new TCHAR[chars+1];

    // Get the list of drives.
    GetLogicalDriveStrings(chars, buf);

    // Parse the list into an array, applying appropriate filters.
    TCHAR *pVol;
    pVol = buf;
    while (*pVol)
    {
        FilteredAdd(list, pVol, flagsSet, flagsUnset);
        pVol = pVol + wxStrlen(pVol) + 1;
    }

    // Cleanup.
    delete[] buf;

    //---------------------------
    // Network Neighborhood next.
    //---------------------------

    // not exclude remote and not removable
    if (!(flagsUnset & wxFS_VOL_REMOTE) &&
        !(flagsSet & wxFS_VOL_REMOVABLE)
       )
    {
        // The returned list will be sorted alphabetically.  We don't pass
        // our in since we don't want to change to order of the local drives.
        wxArrayString nn;
        if (BuildRemoteList(nn, 0, flagsSet, flagsUnset))
        {
            for (size_t idx = 0; idx < nn.GetCount(); idx++)
                list.Add(nn[idx]);
        }
    }

    return list;
} // GetVolumes

//=============================================================================
// Function: CancelSearch
// Purpose: Instruct an active search to stop.
// Notes: - This will only sensibly be called by a thread other than the one
//          performing the search.  This is the only thread-safe function
//          provided by the class.
//=============================================================================
void wxFSVolumeBase::CancelSearch()
{
    ::InterlockedExchange(&s_cancelSearch, TRUE);
} // CancelSearch

//=============================================================================
// Function: constructor
// Purpose: default constructor
//=============================================================================
wxFSVolumeBase::wxFSVolumeBase()
{
    m_isOk = false;
} // wxVolume

//=============================================================================
// Function: constructor
// Purpose: constructor that calls Create
//=============================================================================
wxFSVolumeBase::wxFSVolumeBase(const wxString& name)
{
    Create(name);
} // wxVolume

//=============================================================================
// Function: Create
// Purpose: Finds, logs in, etc. to the request volume.
//=============================================================================
bool wxFSVolumeBase::Create(const wxString& name)
{
    // assume fail.
    m_isOk = false;

    // supplied.
    m_volName = name;

    // Display name.
    SHFILEINFO fi;
    long rc = SHGetFileInfo(m_volName, 0, &fi, sizeof(fi), SHGFI_DISPLAYNAME);
    if (!rc)
    {
        wxLogError(_("Cannot read typename from '%s'!"), m_volName.c_str());
        return m_isOk;
    }
    m_dispName = fi.szDisplayName;

    // all tests passed.
    return m_isOk = true;
} // Create

//=============================================================================
// Function: IsOk
// Purpose: returns true if the volume is legal.
// Notes: For fixed disks, it must exist.  For removable disks, it must also
//        be present.  For Network Shares, it must also be logged in, etc.
//=============================================================================
bool wxFSVolumeBase::IsOk() const
{
    return m_isOk;
} // IsOk

//=============================================================================
// Function: GetKind
// Purpose: Return the type of the volume.
//=============================================================================
wxFSVolumeKind wxFSVolumeBase::GetKind() const
{
    if (!m_isOk)
        return wxFS_VOL_OTHER;

    FileInfoMap::iterator itr = s_fileInfo.find(m_volName);
    if (itr == s_fileInfo.end())
        return wxFS_VOL_OTHER;

    return itr->second.m_type;
}

//=============================================================================
// Function: GetFlags
// Purpose: Return the caches flags for this volume.
// Notes: - Returns -1 if no flags were cached.
//=============================================================================
int wxFSVolumeBase::GetFlags() const
{
    if (!m_isOk)
        return -1;

    FileInfoMap::iterator itr = s_fileInfo.find(m_volName);
    if (itr == s_fileInfo.end())
        return -1;

    return itr->second.m_flags;
} // GetFlags

#endif // wxUSE_BASE

// ============================================================================
// wxFSVolume
// ============================================================================

#if wxUSE_GUI

void wxFSVolume::InitIcons()
{
    m_icons.Alloc(wxFS_VOL_ICO_MAX);
    wxIcon null;
    for (int idx = 0; idx < wxFS_VOL_ICO_MAX; idx++)
        m_icons.Add(null);
}

//=============================================================================
// Function: GetIcon
// Purpose: return the requested icon.
//=============================================================================

wxIcon wxFSVolume::GetIcon(wxFSIconType type) const
{
    wxCHECK_MSG( type >= 0 && (size_t)type < m_icons.GetCount(), wxNullIcon,
                 _T("wxFSIconType::GetIcon(): invalid icon index") );

    // Load on demand.
    if (m_icons[type].IsNull())
    {
        UINT flags = SHGFI_ICON;
        switch (type)
        {
        case wxFS_VOL_ICO_SMALL:
            flags |= SHGFI_SMALLICON;
            break;

        case wxFS_VOL_ICO_LARGE:
            flags |= SHGFI_SHELLICONSIZE;
            break;

        case wxFS_VOL_ICO_SEL_SMALL:
            flags |= SHGFI_SMALLICON | SHGFI_OPENICON;
            break;

        case wxFS_VOL_ICO_SEL_LARGE:
            flags |= SHGFI_SHELLICONSIZE | SHGFI_OPENICON;
            break;

        case wxFS_VOL_ICO_MAX:
            wxFAIL_MSG(_T("wxFS_VOL_ICO_MAX is not valid icon type"));
            break;
        }

        SHFILEINFO fi;
        long rc = SHGetFileInfo(m_volName, 0, &fi, sizeof(fi), flags);
        m_icons[type].SetHICON((WXHICON)fi.hIcon);
        if (!rc || !fi.hIcon)
            wxLogError(_("Cannot load icon from '%s'."), m_volName.c_str());
    }

    return m_icons[type];
} // GetIcon

#endif // wxUSE_GUI

#endif // wxUSE_FSVOLUME
