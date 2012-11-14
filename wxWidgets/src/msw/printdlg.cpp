/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/printdlg.cpp
// Purpose:     wxPrintDialog, wxPageSetupDialog
// Author:      Julian Smart
// Modified by:
// Created:     04/01/98
// RCS-ID:      $Id: printdlg.cpp 62959 2009-12-21 10:04:31Z CE $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ===========================================================================
// declarations
// ===========================================================================

// ---------------------------------------------------------------------------
// headers
// ---------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// Don't use the Windows print dialog if we're in wxUniv mode and using
// the PostScript architecture
#if wxUSE_PRINTING_ARCHITECTURE && (!defined(__WXUNIVERSAL__) || !wxUSE_POSTSCRIPT_ARCHITECTURE_IN_MSW)

#ifndef WX_PRECOMP
    #include "wx/msw/wrapcdlg.h"
    #include "wx/app.h"
    #include "wx/dcprint.h"
    #include "wx/cmndata.h"
#endif

#include "wx/printdlg.h"
#include "wx/msw/printdlg.h"
#include "wx/paper.h"
#include "wx/msgdlg.h"

#include <stdlib.h>

#ifndef __WIN32__
    #include <print.h>
#endif

// smart pointer like class using OpenPrinter and ClosePrinter
class WinPrinter
{
public:
    // default ctor
    WinPrinter()
    {
        m_hPrinter = (HANDLE)NULL;
    }

    WinPrinter( const wxString& printerName )
    {
        Open( printerName );
    }

    ~WinPrinter()
    {
        Close();
    }

    BOOL Open( const wxString& printerName, LPPRINTER_DEFAULTS pDefault=(LPPRINTER_DEFAULTS)NULL )
    {
        Close();
        return OpenPrinter( (LPTSTR)printerName.wx_str(), &m_hPrinter, pDefault );
    }

    BOOL Close()
    {
        BOOL result = TRUE;
        if( m_hPrinter )
        {
            result = ClosePrinter( m_hPrinter );
            m_hPrinter = (HANDLE)NULL;
        }
        return result;
    }

    operator HANDLE() { return m_hPrinter; }
    operator bool() { return m_hPrinter != (HANDLE)NULL; }

private:
    HANDLE m_hPrinter;

    DECLARE_NO_COPY_CLASS(WinPrinter)
};

//----------------------------------------------------------------------------
// wxWindowsPrintNativeData
//----------------------------------------------------------------------------

#ifdef __WXDEBUG__
static wxString wxGetPrintDlgError()
{
    DWORD err = CommDlgExtendedError();
    wxString msg = wxT("Unknown");
    switch (err)
    {
        case CDERR_FINDRESFAILURE: msg = wxT("CDERR_FINDRESFAILURE"); break;
        case CDERR_INITIALIZATION: msg = wxT("CDERR_INITIALIZATION"); break;
        case CDERR_LOADRESFAILURE: msg = wxT("CDERR_LOADRESFAILURE"); break;
        case CDERR_LOADSTRFAILURE: msg = wxT("CDERR_LOADSTRFAILURE"); break;
        case CDERR_LOCKRESFAILURE: msg = wxT("CDERR_LOCKRESFAILURE"); break;
        case CDERR_MEMALLOCFAILURE: msg = wxT("CDERR_MEMALLOCFAILURE"); break;
        case CDERR_MEMLOCKFAILURE: msg = wxT("CDERR_MEMLOCKFAILURE"); break;
        case CDERR_NOHINSTANCE: msg = wxT("CDERR_NOHINSTANCE"); break;
        case CDERR_NOHOOK: msg = wxT("CDERR_NOHOOK"); break;
        case CDERR_NOTEMPLATE: msg = wxT("CDERR_NOTEMPLATE"); break;
        case CDERR_STRUCTSIZE: msg = wxT("CDERR_STRUCTSIZE"); break;
        case  PDERR_RETDEFFAILURE: msg = wxT("PDERR_RETDEFFAILURE"); break;
        case  PDERR_PRINTERNOTFOUND: msg = wxT("PDERR_PRINTERNOTFOUND"); break;
        case  PDERR_PARSEFAILURE: msg = wxT("PDERR_PARSEFAILURE"); break;
        case  PDERR_NODEVICES: msg = wxT("PDERR_NODEVICES"); break;
        case  PDERR_NODEFAULTPRN: msg = wxT("PDERR_NODEFAULTPRN"); break;
        case  PDERR_LOADDRVFAILURE: msg = wxT("PDERR_LOADDRVFAILURE"); break;
        case  PDERR_INITFAILURE: msg = wxT("PDERR_INITFAILURE"); break;
        case  PDERR_GETDEVMODEFAIL: msg = wxT("PDERR_GETDEVMODEFAIL"); break;
        case  PDERR_DNDMMISMATCH: msg = wxT("PDERR_DNDMMISMATCH"); break;
        case  PDERR_DEFAULTDIFFERENT: msg = wxT("PDERR_DEFAULTDIFFERENT"); break;
        case  PDERR_CREATEICFAILURE: msg = wxT("PDERR_CREATEICFAILURE"); break;
        default: break;
    }
    return msg;
}
#endif

static HGLOBAL wxCreateDevNames(const wxString& driverName, const wxString& printerName, const wxString& portName)
{
    HGLOBAL hDev = NULL;
    // if (!driverName.empty() && !printerName.empty() && !portName.empty())
    if (driverName.empty() && printerName.empty() && portName.empty())
    {
    }
    else
    {
        hDev = GlobalAlloc(GPTR, 4*sizeof(WORD)+
                           ( driverName.length() + 1 +
            printerName.length() + 1 +
                             portName.length()+1 ) * sizeof(wxChar) );
        LPDEVNAMES lpDev = (LPDEVNAMES)GlobalLock(hDev);
        lpDev->wDriverOffset = sizeof(WORD) * 4 / sizeof(wxChar);
        wxStrcpy((wxChar*)lpDev + lpDev->wDriverOffset, driverName);

        lpDev->wDeviceOffset = (WORD)( lpDev->wDriverOffset +
                                       driverName.length() + 1 );
        wxStrcpy((wxChar*)lpDev + lpDev->wDeviceOffset, printerName);

        lpDev->wOutputOffset = (WORD)( lpDev->wDeviceOffset +
                                       printerName.length() + 1 );
        wxStrcpy((wxChar*)lpDev + lpDev->wOutputOffset, portName);

        lpDev->wDefault = 0;

        GlobalUnlock(hDev);
    }

    return hDev;
}

IMPLEMENT_CLASS(wxWindowsPrintNativeData, wxPrintNativeDataBase)

wxWindowsPrintNativeData::wxWindowsPrintNativeData()
{
    m_devMode = (void*) NULL;
    m_devNames = (void*) NULL;
    m_customWindowsPaperId = 0;
}

wxWindowsPrintNativeData::~wxWindowsPrintNativeData()
{
    HGLOBAL hDevMode = (HGLOBAL)(DWORD) m_devMode;
    if ( hDevMode )
        GlobalFree(hDevMode);
    HGLOBAL hDevNames = (HGLOBAL)(DWORD) m_devNames;
    if ( hDevNames )
        GlobalFree(hDevNames);
}

bool wxWindowsPrintNativeData::IsOk() const
{
    return (m_devMode != NULL) ;
}

bool wxWindowsPrintNativeData::TransferTo( wxPrintData &data )
{
    HGLOBAL hDevMode = (HGLOBAL)(DWORD) m_devMode;
    HGLOBAL hDevNames = (HGLOBAL)(DWORD) m_devNames;

    if (!hDevMode)
    {
        return false;
    }
    else
    {
        LPDEVMODE devMode = (LPDEVMODE)GlobalLock(hDevMode);

        //// Orientation
        if (devMode->dmFields & DM_ORIENTATION)
            data.SetOrientation( devMode->dmOrientation );

        //// Collation
        if (devMode->dmFields & DM_COLLATE)
        {
            if (devMode->dmCollate == DMCOLLATE_TRUE)
                data.SetCollate( true );
            else
                data.SetCollate( false );
        }

        //// Number of copies
        if (devMode->dmFields & DM_COPIES)
            data.SetNoCopies( devMode->dmCopies );

        //// Bin
        if (devMode->dmFields & DM_DEFAULTSOURCE) {
            switch (devMode->dmDefaultSource) {
                case DMBIN_ONLYONE        : data.SetBin(wxPRINTBIN_ONLYONE       ); break;
                case DMBIN_LOWER          : data.SetBin(wxPRINTBIN_LOWER         ); break;
                case DMBIN_MIDDLE         : data.SetBin(wxPRINTBIN_MIDDLE        ); break;
                case DMBIN_MANUAL         : data.SetBin(wxPRINTBIN_MANUAL        ); break;
                case DMBIN_ENVELOPE       : data.SetBin(wxPRINTBIN_ENVELOPE      ); break;
                case DMBIN_ENVMANUAL      : data.SetBin(wxPRINTBIN_ENVMANUAL     ); break;
                case DMBIN_AUTO           : data.SetBin(wxPRINTBIN_AUTO          ); break;
                case DMBIN_TRACTOR        : data.SetBin(wxPRINTBIN_TRACTOR       ); break;
                case DMBIN_SMALLFMT       : data.SetBin(wxPRINTBIN_SMALLFMT      ); break;
                case DMBIN_LARGEFMT       : data.SetBin(wxPRINTBIN_LARGEFMT      ); break;
                case DMBIN_LARGECAPACITY  : data.SetBin(wxPRINTBIN_LARGECAPACITY ); break;
                case DMBIN_CASSETTE       : data.SetBin(wxPRINTBIN_CASSETTE      ); break;
                case DMBIN_FORMSOURCE     : data.SetBin(wxPRINTBIN_FORMSOURCE    ); break;
                default:
                    if (devMode->dmDefaultSource>=DMBIN_USER) {
                        data.SetBin((wxPrintBin)((devMode->dmDefaultSource)-DMBIN_USER+(int)wxPRINTBIN_USER));
                    } else {
                        data.SetBin(wxPRINTBIN_DEFAULT);
                    }
                    break;
            }
        } else {
            data.SetBin(wxPRINTBIN_DEFAULT);
        }
        if (devMode->dmFields & DM_MEDIATYPE)
        {
            wxASSERT( (int)devMode->dmMediaType != wxPRINTMEDIA_DEFAULT );
            data.SetMedia(devMode->dmMediaType);
        }
        //// Printer name
        if (devMode->dmDeviceName[0] != 0)
            // This syntax fixes a crash when using VS 7.1
            data.SetPrinterName( wxString(devMode->dmDeviceName, CCHDEVICENAME) );

        //// Colour
        if (devMode->dmFields & DM_COLOR)
        {
            if (devMode->dmColor == DMCOLOR_COLOR)
                data.SetColour( true );
            else
                data.SetColour( false );
        }
        else
            data.SetColour( true );

        //// Paper size
        // We don't know size of user defined paper and some buggy drivers
        // set both DM_PAPERSIZE and DM_PAPERWIDTH & DM_PAPERLENGTH. Since
        // dmPaperSize >= DMPAPER_USER wouldn't be in wxWin's database, this
        // code wouldn't set m_paperSize correctly.

        bool foundPaperSize = false;
        if ((devMode->dmFields & DM_PAPERSIZE) && (devMode->dmPaperSize < DMPAPER_USER))
        {
            if (wxThePrintPaperDatabase)
            {
                wxPrintPaperType* paper = wxThePrintPaperDatabase->FindPaperTypeByPlatformId(devMode->dmPaperSize);
                if (paper)
                {
                    data.SetPaperId( paper->GetId() );
                    data.SetPaperSize( wxSize(paper->GetWidth() / 10,paper->GetHeight() / 10) );
                    m_customWindowsPaperId = 0;
                    foundPaperSize = true;
                }
            }
            else
            {
                // Shouldn't really get here
                wxFAIL_MSG(wxT("Paper database wasn't initialized in wxPrintData::ConvertFromNative."));
                data.SetPaperId( wxPAPER_NONE );
                data.SetPaperSize( wxSize(0,0) );
                m_customWindowsPaperId = 0;

                GlobalUnlock(hDevMode);
                return false;
            }
        }

        if (!foundPaperSize)
        {
            if ((devMode->dmFields & DM_PAPERWIDTH) && (devMode->dmFields & DM_PAPERLENGTH))
            {
                // DEVMODE is in tenths of a millimeter
                data.SetPaperSize( wxSize(devMode->dmPaperWidth / 10, devMode->dmPaperLength / 10) );
                data.SetPaperId( wxPAPER_NONE );
                m_customWindowsPaperId = devMode->dmPaperSize;
            }
            else
            {
                // Often will reach this for non-standard paper sizes (sizes which
                // wouldn't be in wxWidget's paper database). Setting
                // m_customWindowsPaperId to devMode->dmPaperSize should be enough
                // to get this paper size working.
                data.SetPaperSize( wxSize(0,0) );
                data.SetPaperId( wxPAPER_NONE );
                m_customWindowsPaperId = devMode->dmPaperSize;
            }
        }

        //// Duplex

        if (devMode->dmFields & DM_DUPLEX)
        {
            switch (devMode->dmDuplex)
            {
                case DMDUP_HORIZONTAL:   data.SetDuplex( wxDUPLEX_HORIZONTAL ); break;
                case DMDUP_VERTICAL:     data.SetDuplex( wxDUPLEX_VERTICAL ); break;
                default:
                case DMDUP_SIMPLEX:      data.SetDuplex( wxDUPLEX_SIMPLEX ); break;
            }
        }
        else
            data.SetDuplex( wxDUPLEX_SIMPLEX );

        //// Quality

        if (devMode->dmFields & DM_PRINTQUALITY)
        {
            switch (devMode->dmPrintQuality)
            {
                case DMRES_MEDIUM:  data.SetQuality( wxPRINT_QUALITY_MEDIUM ); break;
                case DMRES_LOW:     data.SetQuality( wxPRINT_QUALITY_LOW ); break;
                case DMRES_DRAFT:   data.SetQuality( wxPRINT_QUALITY_DRAFT ); break;
                case DMRES_HIGH:    data.SetQuality( wxPRINT_QUALITY_HIGH ); break;
                default:
                {
                    // TODO: if the printer fills in the resolution in DPI, how
                    // will the application know if it's high, low, draft etc.??
                    //                    wxFAIL_MSG("Warning: DM_PRINTQUALITY was not one of the standard values.");
                    data.SetQuality( devMode->dmPrintQuality );
                    break;

                }
            }
        }
        else
            data.SetQuality( wxPRINT_QUALITY_HIGH );

        if (devMode->dmDriverExtra > 0)
            data.SetPrivData( (char *)devMode+devMode->dmSize, devMode->dmDriverExtra );
        else
            data.SetPrivData( NULL, 0 );

        GlobalUnlock(hDevMode);
    }

    if (hDevNames)
    {
        LPDEVNAMES lpDevNames = (LPDEVNAMES)GlobalLock(hDevNames);
        if (lpDevNames)
        {
            // TODO: Unicode-ification

            // Get the port name
            // port is obsolete in WIN32
            // m_printData.SetPortName((LPSTR)lpDevNames + lpDevNames->wDriverOffset);

            // Get the printer name
            wxString printerName = (LPTSTR)lpDevNames + lpDevNames->wDeviceOffset;

            // Not sure if we should check for this mismatch
//            wxASSERT_MSG( (m_printerName.empty() || (devName == m_printerName)), "Printer name obtained from DEVMODE and DEVNAMES were different!");

            if (!printerName.empty())
                data.SetPrinterName( printerName );

            GlobalUnlock(hDevNames);
        }
    }

    return true;
}

bool wxWindowsPrintNativeData::TransferFrom( const wxPrintData &data )
{
    HGLOBAL hDevMode = (HGLOBAL)(DWORD) m_devMode;
    HGLOBAL hDevNames = (HGLOBAL)(DWORD) m_devNames;
    WinPrinter printer;
    LPTSTR szPrinterName = (LPTSTR)data.GetPrinterName().wx_str();

    // From MSDN: How To Modify Printer Settings with the DocumentProperties() Function
    // The purpose of this is to fill the DEVMODE with privdata from printer driver.
    // If we have a printer name and OpenPrinter sucessfully returns
    // this replaces the PrintDlg function which creates the DEVMODE filled only with data from default printer.
    if ( !m_devMode && !data.GetPrinterName().IsEmpty() )
    {
        // Open printer
        if ( printer.Open( data.GetPrinterName() ) == TRUE )
        {
            DWORD dwNeeded, dwRet;

            // Step 1:
            // Allocate a buffer of the correct size.
            dwNeeded = DocumentProperties( NULL,
                printer,         // Handle to our printer.
                szPrinterName,   // Name of the printer.
                NULL,            // Asking for size, so
                NULL,            // these are not used.
                0 );             // Zero returns buffer size.

            LPDEVMODE tempDevMode = static_cast<LPDEVMODE>( GlobalAlloc( GMEM_FIXED | GMEM_ZEROINIT, dwNeeded ) );

            // Step 2:
            // Get the default DevMode for the printer
            dwRet = DocumentProperties( NULL,
                printer,
                szPrinterName,
                tempDevMode,     // The address of the buffer to fill.
                NULL,            // Not using the input buffer.
                DM_OUT_BUFFER ); // Have the output buffer filled.

            if ( dwRet != IDOK )
            {
                // If failure, cleanup
                GlobalFree( tempDevMode );
                printer.Close();
            }
            else
            {
                hDevMode = tempDevMode;
                m_devMode = hDevMode;
                tempDevMode = NULL;
            }
        }
    }

    if (!hDevMode)
    {
        // Use PRINTDLG as a way of creating a DEVMODE object
        PRINTDLG pd;

        // GNU-WIN32 has the wrong size PRINTDLG - can't work out why.
#ifdef __GNUWIN32__
        memset(&pd, 0, 66);
        pd.lStructSize    = 66;
#else
        memset(&pd, 0, sizeof(PRINTDLG));
#ifdef __WXWINCE__
        pd.cbStruct    = sizeof(PRINTDLG);
#else
        pd.lStructSize    = sizeof(PRINTDLG);
#endif
#endif

        pd.hwndOwner      = (HWND)NULL;
        pd.hDevMode       = NULL; // Will be created by PrintDlg
        pd.hDevNames      = NULL; // Ditto
        //pd.hInstance      = (HINSTANCE) wxGetInstance();

        pd.Flags          = PD_RETURNDEFAULT;
        pd.nCopies        = 1;

        // Fill out the DEVMODE structure
        // so we can use it as input in the 'real' PrintDlg
        if (!PrintDlg(&pd))
        {
            if ( pd.hDevMode )
                GlobalFree(pd.hDevMode);
            if ( pd.hDevNames )
                GlobalFree(pd.hDevNames);
            pd.hDevMode = NULL;
            pd.hDevNames = NULL;

#if defined(__WXDEBUG__) && defined(__WIN32__)
            wxString str(wxT("Printing error: "));
            str += wxGetPrintDlgError();
            wxLogDebug(str);
#endif
        }
        else
        {
            hDevMode = pd.hDevMode;
            m_devMode = (void*)(long) hDevMode;
            pd.hDevMode = NULL;

            // We'll create a new DEVNAMEs structure below.
            if ( pd.hDevNames )
                GlobalFree(pd.hDevNames);
            pd.hDevNames = NULL;

            // hDevNames = pd->hDevNames;
            // m_devNames = (void*)(long) hDevNames;
            // pd->hDevnames = NULL;

        }
    }

    if ( hDevMode )
    {
        LPDEVMODE devMode = (LPDEVMODE) GlobalLock(hDevMode);

        //// Orientation
        devMode->dmOrientation = (short)data.GetOrientation();

        //// Collation
        devMode->dmCollate = (data.GetCollate() ? DMCOLLATE_TRUE : DMCOLLATE_FALSE);
        devMode->dmFields |= DM_COLLATE;

        //// Number of copies
        devMode->dmCopies = (short)data.GetNoCopies();
        devMode->dmFields |= DM_COPIES;

        //// Printer name
        wxString name = data.GetPrinterName();
        if (!name.empty())
        {
            //int len = wxMin(31, m_printerName.Len());
            wxStrncpy((wxChar*)devMode->dmDeviceName,name.c_str(),31);
            devMode->dmDeviceName[31] = wxT('\0');
        }

        //// Colour
        if (data.GetColour())
            devMode->dmColor = DMCOLOR_COLOR;
        else
            devMode->dmColor = DMCOLOR_MONOCHROME;
        devMode->dmFields |= DM_COLOR;

        //// Paper size

        // Paper id has priority over paper size. If id is specified, then size
        // is ignored (as it can be filled in even for standard paper sizes)

        wxPrintPaperType *paperType = NULL;

        const wxPaperSize paperId = data.GetPaperId();
        if ( paperId != wxPAPER_NONE && wxThePrintPaperDatabase )
        {
            paperType = wxThePrintPaperDatabase->FindPaperType(paperId);
        }

        if ( paperType )
        {
            devMode->dmPaperSize = (short)paperType->GetPlatformId();
            devMode->dmFields |= DM_PAPERSIZE;
        }
        else // custom (or no) paper size
        {
            const wxSize paperSize = data.GetPaperSize();
            if ( paperSize != wxDefaultSize )
            {
                // Fall back on specifying the paper size explicitly
                if(m_customWindowsPaperId != 0)
                    devMode->dmPaperSize = m_customWindowsPaperId;
                else
                    devMode->dmPaperSize = DMPAPER_USER;
                devMode->dmPaperWidth = (short)(paperSize.x * 10);
                devMode->dmPaperLength = (short)(paperSize.y * 10);
                devMode->dmFields |= DM_PAPERWIDTH;
                devMode->dmFields |= DM_PAPERLENGTH;

                // A printer driver may or may not also want DM_PAPERSIZE to
                // be specified. Also, if the printer driver doesn't implement the DMPAPER_USER
                // size, then this won't work, and even if you found the correct id by
                // enumerating the driver's paper sizes, it probably won't change the actual size,
                // it'll just select that custom paper type with its own current setting.
                // For a discussion on this, see http://www.codeguru.com/forum/showthread.php?threadid=458617
                // Although m_customWindowsPaperId is intended to work around this, it's
                // unclear how it can help you set the custom paper size programmatically.
            }
            //else: neither paper type nor size specified, don't fill DEVMODE
            //      at all so that the system defaults are used
        }

        //// Duplex
        short duplex;
        switch (data.GetDuplex())
        {
            case wxDUPLEX_HORIZONTAL:
                duplex = DMDUP_HORIZONTAL;
                break;
            case wxDUPLEX_VERTICAL:
                duplex = DMDUP_VERTICAL;
                break;
            default:
            // in fact case wxDUPLEX_SIMPLEX:
                duplex = DMDUP_SIMPLEX;
                break;
        }
        devMode->dmDuplex = duplex;
        devMode->dmFields |= DM_DUPLEX;

        //// Quality

        short quality;
        switch (data.GetQuality())
        {
            case wxPRINT_QUALITY_MEDIUM:
                quality = DMRES_MEDIUM;
                break;
            case wxPRINT_QUALITY_LOW:
                quality = DMRES_LOW;
                break;
            case wxPRINT_QUALITY_DRAFT:
                quality = DMRES_DRAFT;
                break;
            case wxPRINT_QUALITY_HIGH:
                quality = DMRES_HIGH;
                break;
            default:
                quality = (short)data.GetQuality();
                devMode->dmYResolution = quality;
                devMode->dmFields |= DM_YRESOLUTION;
                break;
        }
        devMode->dmPrintQuality = quality;
        devMode->dmFields |= DM_PRINTQUALITY;

        if (data.GetPrivDataLen() > 0)
        {
            memcpy( (char *)devMode+devMode->dmSize, data.GetPrivData(), data.GetPrivDataLen() );
            devMode->dmDriverExtra = (WXWORD)data.GetPrivDataLen();
        }

        if (data.GetBin() != wxPRINTBIN_DEFAULT)
        {
            switch (data.GetBin())
            {
                case wxPRINTBIN_ONLYONE:        devMode->dmDefaultSource = DMBIN_ONLYONE;       break;
                case wxPRINTBIN_LOWER:          devMode->dmDefaultSource = DMBIN_LOWER;         break;
                case wxPRINTBIN_MIDDLE:         devMode->dmDefaultSource = DMBIN_MIDDLE;        break;
                case wxPRINTBIN_MANUAL:         devMode->dmDefaultSource = DMBIN_MANUAL;        break;
                case wxPRINTBIN_ENVELOPE:       devMode->dmDefaultSource = DMBIN_ENVELOPE;      break;
                case wxPRINTBIN_ENVMANUAL:      devMode->dmDefaultSource = DMBIN_ENVMANUAL;     break;
                case wxPRINTBIN_AUTO:           devMode->dmDefaultSource = DMBIN_AUTO;          break;
                case wxPRINTBIN_TRACTOR:        devMode->dmDefaultSource = DMBIN_TRACTOR;       break;
                case wxPRINTBIN_SMALLFMT:       devMode->dmDefaultSource = DMBIN_SMALLFMT;      break;
                case wxPRINTBIN_LARGEFMT:       devMode->dmDefaultSource = DMBIN_LARGEFMT;      break;
                case wxPRINTBIN_LARGECAPACITY:  devMode->dmDefaultSource = DMBIN_LARGECAPACITY; break;
                case wxPRINTBIN_CASSETTE:       devMode->dmDefaultSource = DMBIN_CASSETTE;      break;
                case wxPRINTBIN_FORMSOURCE:     devMode->dmDefaultSource = DMBIN_FORMSOURCE;    break;

                default:
                    devMode->dmDefaultSource = (short)(DMBIN_USER + data.GetBin() - wxPRINTBIN_USER); // 256 + data.GetBin() - 14 = 242 + data.GetBin()
                    break;
            }

            devMode->dmFields |= DM_DEFAULTSOURCE;
        }
        if (data.GetMedia() != wxPRINTMEDIA_DEFAULT)
        {
            devMode->dmMediaType = data.GetMedia();
            devMode->dmFields |= DM_MEDIATYPE;
        }
        if( printer )
        {
            // Step 3:
            // Merge the new settings with the old.
            // This gives the driver an opportunity to update any private
            // portions of the DevMode structure.
            DocumentProperties( NULL,
                printer,
                szPrinterName,
                (LPDEVMODE)hDevMode, // Reuse our buffer for output.
                (LPDEVMODE)hDevMode, // Pass the driver our changes
                DM_IN_BUFFER |       // Commands to Merge our changes and
                DM_OUT_BUFFER );     // write the result.
        }
        GlobalUnlock(hDevMode);
    }

    if ( hDevNames )
    {
        GlobalFree(hDevNames);
    }

    // TODO: I hope it's OK to pass some empty strings to DEVNAMES.
    m_devNames = (void*) (long) wxCreateDevNames(wxEmptyString, data.GetPrinterName(), wxEmptyString);

    return true;
}

// ---------------------------------------------------------------------------
// wxPrintDialog
// ---------------------------------------------------------------------------

IMPLEMENT_CLASS(wxWindowsPrintDialog, wxPrintDialogBase)

wxWindowsPrintDialog::wxWindowsPrintDialog(wxWindow *p, wxPrintDialogData* data)
{
    Create(p, data);
}

wxWindowsPrintDialog::wxWindowsPrintDialog(wxWindow *p, wxPrintData* data)
{
    wxPrintDialogData data2;
    if ( data )
        data2 = *data;

    Create(p, &data2);
}

bool wxWindowsPrintDialog::Create(wxWindow *p, wxPrintDialogData* data)
{
    m_dialogParent = p;
    m_printerDC = NULL;
    m_destroyDC = true;

    // MSW handle
    m_printDlg = NULL;

    if ( data )
        m_printDialogData = *data;

    return true;
}

wxWindowsPrintDialog::~wxWindowsPrintDialog()
{
    PRINTDLG *pd = (PRINTDLG *) m_printDlg;
    if (pd && pd->hDevMode)
        GlobalFree(pd->hDevMode);
    if ( pd )
        delete pd;

    if (m_destroyDC && m_printerDC)
        delete m_printerDC;
}

int wxWindowsPrintDialog::ShowModal()
{
    ConvertToNative( m_printDialogData );

    PRINTDLG *pd = (PRINTDLG*) m_printDlg;

    if (m_dialogParent)
        pd->hwndOwner = (HWND) m_dialogParent->GetHWND();
    else if (wxTheApp->GetTopWindow())
        pd->hwndOwner = (HWND) wxTheApp->GetTopWindow()->GetHWND();
    else
        pd->hwndOwner = 0;

    bool ret = (PrintDlg( pd ) != 0);

    pd->hwndOwner = 0;

    if ( ret && (pd->hDC) )
    {
        wxPrinterDC *pdc = new wxPrinterDC( (WXHDC) pd->hDC );
        m_printerDC = pdc;
        ConvertFromNative( m_printDialogData );
        return wxID_OK;
    }
    else
    {
        return wxID_CANCEL;
    }
}

wxDC *wxWindowsPrintDialog::GetPrintDC()
{
    if (m_printerDC)
    {
        m_destroyDC = false;
        return m_printerDC;
    }
    else
        return (wxPrinterDC*) NULL;
}

bool wxWindowsPrintDialog::ConvertToNative( wxPrintDialogData &data )
{
    wxWindowsPrintNativeData *native_data =
        (wxWindowsPrintNativeData *) data.GetPrintData().GetNativeData();
    data.GetPrintData().ConvertToNative();

    PRINTDLG *pd = (PRINTDLG*) m_printDlg;

    // Shouldn't have been defined anywhere
    if (pd)
        return false;

    pd = new PRINTDLG;
    memset( pd, 0, sizeof(PRINTDLG) );
    m_printDlg = (void*) pd;

    // GNU-WIN32 has the wrong size PRINTDLG - can't work out why.
#ifdef __GNUWIN32__
    pd->lStructSize    = 66;
#else
    pd->lStructSize    = sizeof(PRINTDLG);
#endif
    pd->hwndOwner      = (HWND)NULL;
    pd->hDevMode       = NULL; // Will be created by PrintDlg
    pd->hDevNames      = NULL; // Ditto

    pd->Flags          = PD_RETURNDEFAULT;
    pd->nCopies        = 1;

    // Pass the devmode data to the PRINTDLG structure, since it'll
    // be needed when PrintDlg is called.
    if (pd->hDevMode)
        GlobalFree(pd->hDevMode);

    // Pass the devnames data to the PRINTDLG structure, since it'll
    // be needed when PrintDlg is called.
    if (pd->hDevNames)
        GlobalFree(pd->hDevNames);

    pd->hDevMode = (HGLOBAL)(DWORD) native_data->GetDevMode();
    native_data->SetDevMode( (void*) NULL);

    // Shouldn't assert; we should be able to test Ok-ness at a higher level
    //wxASSERT_MSG( (pd->hDevMode), wxT("hDevMode must be non-NULL in ConvertToNative!"));

    pd->hDevNames = (HGLOBAL)(DWORD) native_data->GetDevNames();
    native_data->SetDevNames( (void*) NULL);


    pd->hDC = (HDC) NULL;
    pd->nFromPage = (WORD)data.GetFromPage();
    pd->nToPage = (WORD)data.GetToPage();
    pd->nMinPage = (WORD)data.GetMinPage();
    pd->nMaxPage = (WORD)data.GetMaxPage();
    pd->nCopies = (WORD)data.GetNoCopies();

    pd->Flags = PD_RETURNDC;

#ifdef __GNUWIN32__
    pd->lStructSize = 66;
#else
    pd->lStructSize = sizeof( PRINTDLG );
#endif

    pd->hwndOwner=(HWND)NULL;
//    pd->hDevNames=(HANDLE)NULL;
    pd->hInstance=(HINSTANCE)NULL;
    pd->lCustData = (LPARAM) NULL;
    pd->lpfnPrintHook = NULL;
    pd->lpfnSetupHook = NULL;
    pd->lpPrintTemplateName = NULL;
    pd->lpSetupTemplateName = NULL;
    pd->hPrintTemplate = (HGLOBAL) NULL;
    pd->hSetupTemplate = (HGLOBAL) NULL;

    if ( data.GetAllPages() )
        pd->Flags |= PD_ALLPAGES;
    if ( data.GetSelection() )
        pd->Flags |= PD_SELECTION;
    if ( data.GetCollate() )
        pd->Flags |= PD_COLLATE;
    if ( data.GetPrintToFile() )
        pd->Flags |= PD_PRINTTOFILE;
    if ( !data.GetEnablePrintToFile() )
        pd->Flags |= PD_DISABLEPRINTTOFILE;
    if ( !data.GetEnableSelection() )
        pd->Flags |= PD_NOSELECTION;
    if ( !data.GetEnablePageNumbers() )
        pd->Flags |= PD_NOPAGENUMS;
    else if ( (!data.GetAllPages()) && (!data.GetSelection()) && (data.GetFromPage() != 0) && (data.GetToPage() != 0))
        pd->Flags |= PD_PAGENUMS;
    if ( data.GetEnableHelp() )
        pd->Flags |= PD_SHOWHELP;
#if WXWIN_COMPATIBILITY_2_4
    if ( data.GetSetupDialog() )
        pd->Flags |= PD_PRINTSETUP;
#endif

    return true;
}

bool wxWindowsPrintDialog::ConvertFromNative( wxPrintDialogData &data )
{
    PRINTDLG *pd = (PRINTDLG*) m_printDlg;
    if ( pd == NULL )
        return false;

    wxWindowsPrintNativeData *native_data =
        (wxWindowsPrintNativeData *) data.GetPrintData().GetNativeData();

    // Pass the devmode data back to the wxPrintData structure where it really belongs.
    if (pd->hDevMode)
    {
        if (native_data->GetDevMode())
        {
            // Make sure we don't leak memory
            GlobalFree( (HGLOBAL)(DWORD) native_data->GetDevMode() );
        }
        native_data->SetDevMode( (void*)(long) pd->hDevMode );
        pd->hDevMode = NULL;
    }

    // Pass the devnames data back to the wxPrintData structure where it really belongs.
    if (pd->hDevNames)
    {
        if (native_data->GetDevNames())
        {
            // Make sure we don't leak memory
            GlobalFree((HGLOBAL)(DWORD) native_data->GetDevNames());
        }
        native_data->SetDevNames((void*)(long) pd->hDevNames);
        pd->hDevNames = NULL;
    }

    // Now convert the DEVMODE object, passed down from the PRINTDLG object,
    // into wxWidgets form.
    native_data->TransferTo( data.GetPrintData() );

    data.SetFromPage( pd->nFromPage );
    data.SetToPage( pd->nToPage );
    data.SetMinPage( pd->nMinPage );
    data.SetMaxPage( pd->nMaxPage );
    data.SetNoCopies( pd->nCopies );

    data.SetAllPages( (((pd->Flags & PD_PAGENUMS) != PD_PAGENUMS) && ((pd->Flags & PD_SELECTION) != PD_SELECTION)) );
    data.SetSelection( ((pd->Flags & PD_SELECTION) == PD_SELECTION) );
    data.SetCollate( ((pd->Flags & PD_COLLATE) == PD_COLLATE) );
    data.SetPrintToFile( ((pd->Flags & PD_PRINTTOFILE) == PD_PRINTTOFILE) );
    data.EnablePrintToFile( ((pd->Flags & PD_DISABLEPRINTTOFILE) != PD_DISABLEPRINTTOFILE) );
    data.EnableSelection( ((pd->Flags & PD_NOSELECTION) != PD_NOSELECTION) );
    data.EnablePageNumbers( ((pd->Flags & PD_NOPAGENUMS) != PD_NOPAGENUMS) );
    data.EnableHelp( ((pd->Flags & PD_SHOWHELP) == PD_SHOWHELP) );
#if WXWIN_COMPATIBILITY_2_4
    data.SetSetupDialog( ((pd->Flags & PD_PRINTSETUP) == PD_PRINTSETUP) );
#endif

    return true;
}

// ---------------------------------------------------------------------------
// wxWidnowsPageSetupDialog
// ---------------------------------------------------------------------------

IMPLEMENT_CLASS(wxWindowsPageSetupDialog, wxPageSetupDialogBase)

wxWindowsPageSetupDialog::wxWindowsPageSetupDialog()
{
    m_dialogParent = NULL;
    m_pageDlg = NULL;
}

wxWindowsPageSetupDialog::wxWindowsPageSetupDialog(wxWindow *p, wxPageSetupDialogData *data)
{
    Create(p, data);
}

bool wxWindowsPageSetupDialog::Create(wxWindow *p, wxPageSetupDialogData *data)
{
    m_dialogParent = p;
    m_pageDlg = NULL;

    if (data)
        m_pageSetupData = (*data);

    return true;
}

wxWindowsPageSetupDialog::~wxWindowsPageSetupDialog()
{
    PAGESETUPDLG *pd = (PAGESETUPDLG *)m_pageDlg;
    if ( pd && pd->hDevMode )
        GlobalFree(pd->hDevMode);
    if ( pd && pd->hDevNames )
        GlobalFree(pd->hDevNames);
    if ( pd )
        delete pd;
}

int wxWindowsPageSetupDialog::ShowModal()
{
    ConvertToNative( m_pageSetupData );

    PAGESETUPDLG *pd = (PAGESETUPDLG *) m_pageDlg;
    if (m_dialogParent)
        pd->hwndOwner = (HWND) m_dialogParent->GetHWND();
    else if (wxTheApp->GetTopWindow())
        pd->hwndOwner = (HWND) wxTheApp->GetTopWindow()->GetHWND();
    else
        pd->hwndOwner = 0;

    BOOL retVal = PageSetupDlg( pd ) ;
    pd->hwndOwner = 0;

    if (retVal)
    {
        ConvertFromNative( m_pageSetupData );

        return wxID_OK;
    }

    return wxID_CANCEL;
}

bool wxWindowsPageSetupDialog::ConvertToNative( wxPageSetupDialogData &data )
{
    wxWindowsPrintNativeData *native_data =
        (wxWindowsPrintNativeData *) data.GetPrintData().GetNativeData();
    data.GetPrintData().ConvertToNative();

    PAGESETUPDLG *pd = (PAGESETUPDLG*) m_pageDlg;

    // Shouldn't have been defined anywhere
    if (pd)
        return false;

    pd = new PAGESETUPDLG;
    pd->hDevMode = NULL;
    pd->hDevNames = NULL;
    m_pageDlg = (void *)pd;

    // Pass the devmode data (created in m_printData.ConvertToNative)
    // to the PRINTDLG structure, since it'll
    // be needed when PrintDlg is called.

    if (pd->hDevMode)
    {
        GlobalFree(pd->hDevMode);
        pd->hDevMode = NULL;
    }
    pd->hDevMode = (HGLOBAL) native_data->GetDevMode();
    native_data->SetDevMode( (void*) NULL );

    // Shouldn't assert; we should be able to test Ok-ness at a higher level
    //wxASSERT_MSG( (pd->hDevMode), wxT("hDevMode must be non-NULL in ConvertToNative!"));

    // Pass the devnames data (created in m_printData.ConvertToNative)
    // to the PRINTDLG structure, since it'll
    // be needed when PrintDlg is called.

    if (pd->hDevNames)
    {
        GlobalFree(pd->hDevNames);
        pd->hDevNames = NULL;
    }
    pd->hDevNames = (HGLOBAL) native_data->GetDevNames();
    native_data->SetDevNames((void*) NULL);

//        pd->hDevMode = GlobalAlloc(GMEM_MOVEABLE, sizeof(DEVMODE));

    pd->Flags = PSD_MARGINS|PSD_MINMARGINS;

    if ( data.GetDefaultMinMargins() )
        pd->Flags |= PSD_DEFAULTMINMARGINS;
    if ( !data.GetEnableMargins() )
        pd->Flags |= PSD_DISABLEMARGINS;
    if ( !data.GetEnableOrientation() )
        pd->Flags |= PSD_DISABLEORIENTATION;
    if ( !data.GetEnablePaper() )
        pd->Flags |= PSD_DISABLEPAPER;
    if ( !data.GetEnablePrinter() )
        pd->Flags |= PSD_DISABLEPRINTER;
    if ( data.GetDefaultInfo() )
        pd->Flags |= PSD_RETURNDEFAULT;
    if ( data.GetEnableHelp() )
        pd->Flags |= PSD_SHOWHELP;

    // We want the units to be in hundredths of a millimetre
    pd->Flags |= PSD_INHUNDREDTHSOFMILLIMETERS;

    pd->lStructSize = sizeof( PAGESETUPDLG );
    pd->hwndOwner=(HWND)NULL;
//    pd->hDevNames=(HWND)NULL;
    pd->hInstance=(HINSTANCE)NULL;
    //   PAGESETUPDLG is in hundreds of a mm
    pd->ptPaperSize.x = data.GetPaperSize().x * 100;
    pd->ptPaperSize.y = data.GetPaperSize().y * 100;

    pd->rtMinMargin.left = data.GetMinMarginTopLeft().x * 100;
    pd->rtMinMargin.top = data.GetMinMarginTopLeft().y * 100;
    pd->rtMinMargin.right = data.GetMinMarginBottomRight().x * 100;
    pd->rtMinMargin.bottom = data.GetMinMarginBottomRight().y * 100;

    pd->rtMargin.left = data.GetMarginTopLeft().x * 100;
    pd->rtMargin.top = data.GetMarginTopLeft().y * 100;
    pd->rtMargin.right = data.GetMarginBottomRight().x * 100;
    pd->rtMargin.bottom = data.GetMarginBottomRight().y * 100;

    pd->lCustData = 0;
    pd->lpfnPageSetupHook = NULL;
    pd->lpfnPagePaintHook = NULL;
    pd->hPageSetupTemplate = NULL;
    pd->lpPageSetupTemplateName = NULL;

/*
    if ( pd->hDevMode )
    {
        DEVMODE *devMode = (DEVMODE*) GlobalLock(pd->hDevMode);
        memset(devMode, 0, sizeof(DEVMODE));
        devMode->dmSize = sizeof(DEVMODE);
        devMode->dmOrientation = m_orientation;
        devMode->dmFields = DM_ORIENTATION;
        GlobalUnlock(pd->hDevMode);
    }
*/
    return true;
}

bool wxWindowsPageSetupDialog::ConvertFromNative( wxPageSetupDialogData &data )
{
    PAGESETUPDLG *pd = (PAGESETUPDLG *) m_pageDlg;
    if ( !pd )
        return false;

    wxWindowsPrintNativeData *native_data =
        (wxWindowsPrintNativeData *) data.GetPrintData().GetNativeData();

    // Pass the devmode data back to the wxPrintData structure where it really belongs.
    if (pd->hDevMode)
    {
        if (native_data->GetDevMode())
        {
            // Make sure we don't leak memory
            GlobalFree((HGLOBAL) native_data->GetDevMode());
        }
        native_data->SetDevMode( (void*) pd->hDevMode );
        pd->hDevMode = NULL;
    }

    // Isn't this superfluous? It's called again below.
    // data.GetPrintData().ConvertFromNative();

    // Pass the devnames data back to the wxPrintData structure where it really belongs.
    if (pd->hDevNames)
    {
        if (native_data->GetDevNames())
        {
            // Make sure we don't leak memory
            GlobalFree((HGLOBAL) native_data->GetDevNames());
        }
        native_data->SetDevNames((void*) pd->hDevNames);
        pd->hDevNames = NULL;
    }

    data.GetPrintData().ConvertFromNative();

    pd->Flags = PSD_MARGINS|PSD_MINMARGINS;

    data.SetDefaultMinMargins( ((pd->Flags & PSD_DEFAULTMINMARGINS) == PSD_DEFAULTMINMARGINS) );
    data.EnableMargins( ((pd->Flags & PSD_DISABLEMARGINS) != PSD_DISABLEMARGINS) );
    data.EnableOrientation( ((pd->Flags & PSD_DISABLEORIENTATION) != PSD_DISABLEORIENTATION) );
    data.EnablePaper( ((pd->Flags & PSD_DISABLEPAPER) != PSD_DISABLEPAPER) );
    data.EnablePrinter( ((pd->Flags & PSD_DISABLEPRINTER) != PSD_DISABLEPRINTER) );
    data.SetDefaultInfo( ((pd->Flags & PSD_RETURNDEFAULT) == PSD_RETURNDEFAULT) );
    data.EnableHelp( ((pd->Flags & PSD_SHOWHELP) == PSD_SHOWHELP) );

    //   PAGESETUPDLG is in hundreds of a mm
    if (data.GetPrintData().GetOrientation() == wxLANDSCAPE)
        data.SetPaperSize( wxSize(pd->ptPaperSize.y / 100, pd->ptPaperSize.x / 100) );
    else
        data.SetPaperSize( wxSize(pd->ptPaperSize.x / 100, pd->ptPaperSize.y / 100) );

    data.SetMinMarginTopLeft( wxPoint(pd->rtMinMargin.left / 100, pd->rtMinMargin.top / 100) );
    data.SetMinMarginBottomRight( wxPoint(pd->rtMinMargin.right / 100, pd->rtMinMargin.bottom / 100) );

    data.SetMarginTopLeft( wxPoint(pd->rtMargin.left / 100, pd->rtMargin.top / 100) );
    data.SetMarginBottomRight( wxPoint(pd->rtMargin.right / 100, pd->rtMargin.bottom / 100) );

    return true;
}

#endif
    // wxUSE_PRINTING_ARCHITECTURE
