/////////////////////////////////////////////////////////////////////////////
// Name:        wx/volume.h
// Purpose:     wxFSVolume - encapsulates system volume information
// Author:      George Policello
// Modified by:
// Created:     28 Jan 02
// RCS-ID:      $Id: volume.h 39399 2006-05-28 23:08:31Z ABX $
// Copyright:   (c) 2002 George Policello
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// wxFSVolume represents a volume/drive/mount point in a file system
// ----------------------------------------------------------------------------

#ifndef _WX_FSVOLUME_H_
#define _WX_FSVOLUME_H_

#include "wx/defs.h"

#if wxUSE_FSVOLUME

#include "wx/arrstr.h"

// the volume flags
enum
{
    // is the volume mounted?
    wxFS_VOL_MOUNTED = 0x0001,

    // is the volume removable (floppy, CD, ...)?
    wxFS_VOL_REMOVABLE = 0x0002,

    // read only? (otherwise read write)
    wxFS_VOL_READONLY = 0x0004,

    // network resources
    wxFS_VOL_REMOTE = 0x0008
};

// the volume types
enum wxFSVolumeKind
{
    wxFS_VOL_FLOPPY,
    wxFS_VOL_DISK,
    wxFS_VOL_CDROM,
    wxFS_VOL_DVDROM,
    wxFS_VOL_NETWORK,
    wxFS_VOL_OTHER,
    wxFS_VOL_MAX
};

class WXDLLIMPEXP_BASE wxFSVolumeBase
{
public:
    // return the array containing the names of the volumes
    //
    // only the volumes with the flags such that
    //  (flags & flagsSet) == flagsSet && !(flags & flagsUnset)
    // are returned (by default, all mounted ones)
    static wxArrayString GetVolumes(int flagsSet = wxFS_VOL_MOUNTED,
                                    int flagsUnset = 0);

    // stop execution of GetVolumes() called previously (should be called from
    // another thread, of course)
    static void CancelSearch();

    // create the volume object with this name (should be one of those returned
    // by GetVolumes()).
    wxFSVolumeBase();
    wxFSVolumeBase(const wxString& name);
    bool Create(const wxString& name);

    // accessors
    // ---------

    // is this a valid volume?
    bool IsOk() const;

    // kind of this volume?
    wxFSVolumeKind GetKind() const;

    // flags of this volume?
    int GetFlags() const;

    // can we write to this volume?
    bool IsWritable() const { return !(GetFlags() & wxFS_VOL_READONLY); }

    // get the name of the volume and the name which should be displayed to the
    // user
    wxString GetName() const { return m_volName; }
    wxString GetDisplayName() const { return m_dispName; }

    // TODO: operatios (Mount(), Unmount(), Eject(), ...)?

protected:
    // the internal volume name
    wxString m_volName;

    // the volume name as it is displayed to the user
    wxString m_dispName;

    // have we been initialized correctly?
    bool m_isOk;
};

#if wxUSE_GUI

#include "wx/icon.h"
#include "wx/iconbndl.h" // only for wxIconArray

enum wxFSIconType
{
    wxFS_VOL_ICO_SMALL = 0,
    wxFS_VOL_ICO_LARGE,
    wxFS_VOL_ICO_SEL_SMALL,
    wxFS_VOL_ICO_SEL_LARGE,
    wxFS_VOL_ICO_MAX
};

// wxFSVolume adds GetIcon() to wxFSVolumeBase
class WXDLLIMPEXP_CORE wxFSVolume : public wxFSVolumeBase
{
public:
    wxFSVolume() : wxFSVolumeBase() { InitIcons(); }
    wxFSVolume(const wxString& name) : wxFSVolumeBase(name) { InitIcons(); }

    wxIcon GetIcon(wxFSIconType type) const;

private:
    void InitIcons();

    // the different icons for this volume (created on demand)
    wxIconArray m_icons;
};

#else // !wxUSE_GUI

// wxFSVolume is the same thing as wxFSVolume in wxBase
typedef wxFSVolumeBase wxFSVolume;

#endif // wxUSE_GUI/!wxUSE_GUI

#endif // wxUSE_FSVOLUME

#endif // _WX_FSVOLUME_H_
