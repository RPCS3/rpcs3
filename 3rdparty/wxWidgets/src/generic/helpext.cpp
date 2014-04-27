/////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/helpext.cpp
// Purpose:     an external help controller for wxWidgets
// Author:      Karsten Ballueder
// Modified by:
// Created:     04/01/98
// RCS-ID:      $Id: helpext.cpp 38857 2006-04-20 07:31:44Z ABX $
// Copyright:   (c) Karsten Ballueder
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_HELP && !defined(__WXWINCE__) && (!defined(__WXMAC__) || defined(__WXMAC_OSX__))

#ifndef WX_PRECOMP
    #include "wx/list.h"
    #include "wx/string.h"
    #include "wx/utils.h"
    #include "wx/intl.h"
    #include "wx/msgdlg.h"
    #include "wx/choicdlg.h"
    #include "wx/log.h"
#endif

#include "wx/filename.h"
#include "wx/textfile.h"
#include "wx/generic/helpext.h"

#include <stdio.h>
#include <ctype.h>
#include <sys/stat.h>

#if !defined(__WINDOWS__) && !defined(__OS2__)
    #include   <unistd.h>
#endif

#ifdef __WINDOWS__
#include "wx/msw/mslu.h"
#endif

#ifdef __WXMSW__
#include <windows.h>
#include "wx/msw/winundef.h"
#endif

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

/// Name for map file.
#define WXEXTHELP_MAPFILE   _T("wxhelp.map")

/// Character introducing comments/documentation field in map file.
#define WXEXTHELP_COMMENTCHAR   ';'

#define CONTENTS_ID   0

IMPLEMENT_CLASS(wxExtHelpController, wxHelpControllerBase)

/// Name of environment variable to set help browser.
#define   WXEXTHELP_ENVVAR_BROWSER   wxT("WX_HELPBROWSER")
/// Is browser a netscape browser?
#define   WXEXTHELP_ENVVAR_BROWSERISNETSCAPE wxT("WX_HELPBROWSER_NS")

/**
   This class implements help via an external browser.
   It requires the name of a directory containing the documentation
   and a file mapping numerical Section numbers to relative URLS.
*/

wxExtHelpController::wxExtHelpController(wxWindow* parentWindow)
                   : wxHelpControllerBase(parentWindow)
{
   m_MapList = NULL;
   m_NumOfEntries = 0;
   m_BrowserIsNetscape = false;

   wxChar *browser = wxGetenv(WXEXTHELP_ENVVAR_BROWSER);
   if (browser)
   {
      m_BrowserName = browser;
      browser = wxGetenv(WXEXTHELP_ENVVAR_BROWSERISNETSCAPE);
      m_BrowserIsNetscape = browser && (wxAtoi(browser) != 0);
   }
}

wxExtHelpController::~wxExtHelpController()
{
   DeleteList();
}

void wxExtHelpController::SetBrowser(const wxString& browsername, bool isNetscape)
{
   m_BrowserName = browsername;
   m_BrowserIsNetscape = isNetscape;
}

// Set viewer: new, generic name for SetBrowser
void wxExtHelpController::SetViewer(const wxString& viewer, long flags)
{
    SetBrowser(viewer, (flags & wxHELP_NETSCAPE) != 0);
}

bool wxExtHelpController::DisplayHelp(const wxString &relativeURL)
{
    // construct hte URL to open -- it's just a file
    wxString url(_T("file://") + m_helpDir);
    url << wxFILE_SEP_PATH << relativeURL;

    // use the explicit browser program if specified
    if ( !m_BrowserName.empty() )
    {
        if ( m_BrowserIsNetscape )
        {
            wxString command;
            command << m_BrowserName
                    << wxT(" -remote openURL(") << url << wxT(')');
            if ( wxExecute(command, wxEXEC_SYNC) != -1 )
                return true;
        }

        if ( wxExecute(m_BrowserName + _T(' ') + url, wxEXEC_SYNC) != -1 )
            return true;
    }
    //else: either no browser explicitly specified or we failed to open it

    // just use default browser
    return wxLaunchDefaultBrowser(url);
}

class wxExtHelpMapEntry : public wxObject
{
public:
   int      id;
   wxString url;
   wxString doc;
   wxExtHelpMapEntry(int iid, wxString const &iurl, wxString const &idoc)
      { id = iid; url = iurl; doc = idoc; }
};

void wxExtHelpController::DeleteList()
{
   if (m_MapList)
   {
      wxList::compatibility_iterator node = m_MapList->GetFirst();
      while (node)
      {
         delete (wxExtHelpMapEntry *)node->GetData();
         m_MapList->Erase(node);
         node = m_MapList->GetFirst();
      }

      delete m_MapList;
      m_MapList = (wxList*) NULL;
   }
}

// This must be called to tell the controller where to find the documentation.
//  @param file - NOT a filename, but a directory name.
//  @return true on success
bool wxExtHelpController::Initialize(const wxString& file)
{
   return LoadFile(file);
}

bool wxExtHelpController::ParseMapFileLine(const wxString& line)
{
    const wxChar *p = line.c_str();

    // skip whitespace
    while ( isascii(*p) && isspace(*p) )
        p++;

    // skip empty lines and comments
    if ( *p == _T('\0') || *p == WXEXTHELP_COMMENTCHAR )
        return true;

    // the line is of the form "num url" so we must have an integer now
    wxChar *end;
    const unsigned long id = wxStrtoul(p, &end, 0);

    if ( end == p )
        return false;

    p = end;
    while ( isascii(*p) && isspace(*p) )
        p++;

    // next should be the URL
    wxString url;
    url.reserve(line.length());
    while ( isascii(*p) && !isspace(*p) )
        url += *p++;

    while ( isascii(*p) && isspace(*p) )
        p++;

    // and finally the optional description of the entry after comment
    wxString doc;
    if ( *p == WXEXTHELP_COMMENTCHAR )
    {
        p++;
        while ( isascii(*p) && isspace(*p) )
            p++;
        doc = p;
    }

    m_MapList->Append(new wxExtHelpMapEntry(id, url, doc));
    m_NumOfEntries++;

    return true;
}

// file is a misnomer as it's the name of the base help directory
bool wxExtHelpController::LoadFile(const wxString& file)
{
    wxFileName helpDir(wxFileName::DirName(file));
    helpDir.MakeAbsolute();

    bool dirExists = false;

#if wxUSE_INTL
    // If a locale is set, look in file/localename, i.e. If passed
    // "/usr/local/myapp/help" and the current wxLocale is set to be "de", then
    // look in "/usr/local/myapp/help/de/" first and fall back to
    // "/usr/local/myapp/help" if that doesn't exist.
    const wxLocale * const loc = wxGetLocale();
    if ( loc )
    {
        wxString locName = loc->GetName();

        // the locale is in general of the form xx_YY.zzzz, try the full firm
        // first and then also more general ones
        wxFileName helpDirLoc(helpDir);
        helpDirLoc.AppendDir(locName);
        dirExists = helpDirLoc.DirExists();

        if ( ! dirExists )
        {
            // try without encoding
            const wxString locNameWithoutEncoding = locName.BeforeLast(_T('.'));
            if ( !locNameWithoutEncoding.empty() )
            {
                helpDirLoc = helpDir;
                helpDirLoc.AppendDir(locNameWithoutEncoding);
                dirExists = helpDirLoc.DirExists();
            }
        }

        if ( !dirExists )
        {
            // try without country part
            wxString locNameWithoutCountry = locName.BeforeLast(_T('_'));
            if ( !locNameWithoutCountry.empty() )
            {
                helpDirLoc = helpDir;
                helpDirLoc.AppendDir(locNameWithoutCountry);
                dirExists = helpDirLoc.DirExists();
            }
        }

        if ( dirExists )
            helpDir = helpDirLoc;
    }
#endif // wxUSE_INTL

    if ( ! dirExists && !helpDir.DirExists() )
    {
        wxLogError(_("Help directory \"%s\" not found."),
                   helpDir.GetFullPath().c_str());
        return false;
    }

    const wxFileName mapFile(helpDir.GetFullPath(), WXEXTHELP_MAPFILE);
    if ( ! mapFile.FileExists() )
    {
        wxLogError(_("Help file \"%s\" not found."),
                   mapFile.GetFullPath().c_str());
        return false;
    }

    DeleteList();
    m_MapList = new wxList;
    m_NumOfEntries = 0;

    wxTextFile input;
    if ( !input.Open(mapFile.GetFullPath()) )
        return false;

    for ( wxString& line = input.GetFirstLine();
          !input.Eof();
          line = input.GetNextLine() )
    {
        if ( !ParseMapFileLine(line) )
        {
            wxLogWarning(_("Line %lu of map file \"%s\" has invalid syntax, skipped."),
                         (unsigned long)input.GetCurrentLine(),
                         mapFile.GetFullPath().c_str());
        }
    }

    if ( !m_NumOfEntries )
    {
        wxLogError(_("No valid mappings found in the file \"%s\"."),
                   mapFile.GetFullPath().c_str());
        return false;
    }

    m_helpDir = helpDir.GetFullPath(); // now it's valid
    return true;
}


bool wxExtHelpController::DisplayContents()
{
   if (! m_NumOfEntries)
      return false;

   wxString contents;
   wxList::compatibility_iterator node = m_MapList->GetFirst();
   wxExtHelpMapEntry *entry;
   while (node)
   {
      entry = (wxExtHelpMapEntry *)node->GetData();
      if (entry->id == CONTENTS_ID)
      {
         contents = entry->url;
         break;
      }

      node = node->GetNext();
   }

   bool rc = false;
   wxString file;
   file << m_helpDir << wxFILE_SEP_PATH << contents;
   if (file.Contains(wxT('#')))
      file = file.BeforeLast(wxT('#'));
   if (contents.length() && wxFileExists(file))
      rc = DisplaySection(CONTENTS_ID);

   // if not found, open homemade toc:
   return rc ? true : KeywordSearch(wxEmptyString);
}

bool wxExtHelpController::DisplaySection(int sectionNo)
{
   if (! m_NumOfEntries)
      return false;

   wxBusyCursor b; // display a busy cursor
   wxList::compatibility_iterator node = m_MapList->GetFirst();
   wxExtHelpMapEntry *entry;
   while (node)
   {
      entry = (wxExtHelpMapEntry *)node->GetData();
      if (entry->id == sectionNo)
         return DisplayHelp(entry->url);
      node = node->GetNext();
   }

   return false;
}

bool wxExtHelpController::DisplaySection(const wxString& section)
{
    bool isFilename = (section.Find(wxT(".htm")) != -1);

    if (isFilename)
        return DisplayHelp(section);
    else
        return KeywordSearch(section);
}

bool wxExtHelpController::DisplayBlock(long blockNo)
{
   return DisplaySection((int)blockNo);
}

bool wxExtHelpController::KeywordSearch(const wxString& k,
                                   wxHelpSearchMode WXUNUSED(mode))
{
   if (! m_NumOfEntries)
      return false;

   wxString *choices = new wxString[m_NumOfEntries];
   wxString *urls = new wxString[m_NumOfEntries];

   int          idx = 0;
   bool         rc = false;
   bool         showAll = k.empty();

   wxList::compatibility_iterator node = m_MapList->GetFirst();

   {
        // display a busy cursor
        wxBusyCursor b;
        wxString compA, compB;
        wxExtHelpMapEntry *entry;

        // we compare case insensitive
        if (! showAll)
        {
            compA = k;
            compA.LowerCase();
        }

        while (node)
        {
            entry = (wxExtHelpMapEntry *)node->GetData();
            compB = entry->doc;

            bool testTarget = ! compB.empty();
            if (testTarget && ! showAll)
            {
                compB.LowerCase();
                testTarget = compB.Contains(compA);
            }

            if (testTarget)
            {
                urls[idx] = entry->url;
                // doesn't work:
                // choices[idx] = (**i).doc.Contains((**i).doc.Before(WXEXTHELP_COMMENTCHAR));
                //if (choices[idx].empty()) // didn't contain the ';'
                //   choices[idx] = (**i).doc;
                choices[idx] = wxEmptyString;
                for (int j=0; ; j++)
                {
                    wxChar targetChar = entry->doc.c_str()[j];
                    if ((targetChar == 0) || (targetChar == WXEXTHELP_COMMENTCHAR))
                        break;

                    choices[idx] << targetChar;
                }

                idx++;
            }

            node = node->GetNext();
        }
    }

    switch (idx)
    {
    case 0:
        wxMessageBox(_("No entries found."));
        break;

    case 1:
        rc = DisplayHelp(urls[0]);
        break;

    default:
        idx = wxGetSingleChoiceIndex(
            showAll ? _("Help Index") : _("Relevant entries:"),
            showAll ? _("Help Index") : _("Entries found"),
            idx, choices);
        if (idx >= 0)
            rc = DisplayHelp(urls[idx]);
        break;
    }

    delete [] urls;
    delete [] choices;

    return rc;
}


bool wxExtHelpController::Quit()
{
   return true;
}

void wxExtHelpController::OnQuit()
{
}

#endif // wxUSE_HELP
