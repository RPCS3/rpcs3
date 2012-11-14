///////////////////////////////////////////////////////////////////////////////
// Name:        wx/cmdline.h
// Purpose:     wxCmdLineParser and related classes for parsing the command
//              line options
// Author:      Vadim Zeitlin
// Modified by:
// Created:     04.01.00
// RCS-ID:      $Id: cmdline.h 49563 2007-10-31 20:46:21Z VZ $
// Copyright:   (c) 2000 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_CMDLINE_H_
#define _WX_CMDLINE_H_

#include "wx/defs.h"

#include "wx/string.h"
#include "wx/arrstr.h"

#if wxUSE_CMDLINE_PARSER

class WXDLLIMPEXP_FWD_BASE wxDateTime;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// by default, options are optional (sic) and each call to AddParam() allows
// one more parameter - this may be changed by giving non-default flags to it
enum
{
    wxCMD_LINE_OPTION_MANDATORY = 0x01, // this option must be given
    wxCMD_LINE_PARAM_OPTIONAL   = 0x02, // the parameter may be omitted
    wxCMD_LINE_PARAM_MULTIPLE   = 0x04, // the parameter may be repeated
    wxCMD_LINE_OPTION_HELP      = 0x08, // this option is a help request
    wxCMD_LINE_NEEDS_SEPARATOR  = 0x10  // must have sep before the value
};

// an option value or parameter may be a string (the most common case), a
// number or a date
enum wxCmdLineParamType
{
    wxCMD_LINE_VAL_STRING,  // should be 0 (default)
    wxCMD_LINE_VAL_NUMBER,
    wxCMD_LINE_VAL_DATE,
    wxCMD_LINE_VAL_NONE
};

// for constructing the cmd line description using Init()
enum wxCmdLineEntryType
{
    wxCMD_LINE_SWITCH,
    wxCMD_LINE_OPTION,
    wxCMD_LINE_PARAM,
    wxCMD_LINE_NONE         // to terminate the list
};

// ----------------------------------------------------------------------------
// wxCmdLineEntryDesc is a description of one command line
// switch/option/parameter
// ----------------------------------------------------------------------------

struct wxCmdLineEntryDesc
{
    wxCmdLineEntryType kind;
    const wxChar *shortName;
    const wxChar *longName;
    const wxChar *description;
    wxCmdLineParamType type;
    int flags;
};

// ----------------------------------------------------------------------------
// wxCmdLineParser is a class for parsing command line.
//
// It has the following features:
//
// 1. distinguishes options, switches and parameters; allows option grouping
// 2. allows both short and long options
// 3. automatically generates the usage message from the cmd line description
// 4. does type checks on the options values (number, date, ...)
//
// To use it you should:
//
// 1. construct it giving it the cmd line to parse and optionally its desc
// 2. construct the cmd line description using AddXXX() if not done in (1)
// 3. call Parse()
// 4. use GetXXX() to retrieve the parsed info
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxCmdLineParser
{
public:
    // ctors and initializers
    // ----------------------

    // default ctor or ctor giving the cmd line in either Unix or Win form
    wxCmdLineParser() { Init(); }
    wxCmdLineParser(int argc, char **argv) { Init(); SetCmdLine(argc, argv); }
#if wxUSE_UNICODE
    wxCmdLineParser(int argc, wxChar **argv) { Init(); SetCmdLine(argc, argv); }
#endif // wxUSE_UNICODE
    wxCmdLineParser(const wxString& cmdline) { Init(); SetCmdLine(cmdline); }

    // the same as above, but also gives the cmd line description - otherwise,
    // use AddXXX() later
    wxCmdLineParser(const wxCmdLineEntryDesc *desc)
        { Init(); SetDesc(desc); }
    wxCmdLineParser(const wxCmdLineEntryDesc *desc, int argc, char **argv)
        { Init(); SetCmdLine(argc, argv); SetDesc(desc); }
#if wxUSE_UNICODE
    wxCmdLineParser(const wxCmdLineEntryDesc *desc, int argc, wxChar **argv)
        { Init(); SetCmdLine(argc, argv); SetDesc(desc); }
#endif // wxUSE_UNICODE
    wxCmdLineParser(const wxCmdLineEntryDesc *desc, const wxString& cmdline)
        { Init(); SetCmdLine(cmdline); SetDesc(desc); }

    // set cmd line to parse after using one of the ctors which don't do it
    void SetCmdLine(int argc, char **argv);
#if wxUSE_UNICODE
    void SetCmdLine(int argc, wxChar **argv);
#endif // wxUSE_UNICODE
    void SetCmdLine(const wxString& cmdline);

    // not virtual, don't use this class polymorphically
    ~wxCmdLineParser();

    // set different parser options
    // ----------------------------

    // by default, '-' is switch char under Unix, '-' or '/' under Win:
    // switchChars contains all characters with which an option or switch may
    // start
    void SetSwitchChars(const wxString& switchChars);

    // long options are not POSIX-compliant, this option allows to disable them
    void EnableLongOptions(bool enable = true);
    void DisableLongOptions() { EnableLongOptions(false); }

    bool AreLongOptionsEnabled();

    // extra text may be shown by Usage() method if set by this function
    void SetLogo(const wxString& logo);

    // construct the cmd line description
    // ----------------------------------

    // take the cmd line description from the wxCMD_LINE_NONE terminated table
    void SetDesc(const wxCmdLineEntryDesc *desc);

    // a switch: i.e. an option without value
    void AddSwitch(const wxString& name, const wxString& lng = wxEmptyString,
                   const wxString& desc = wxEmptyString,
                   int flags = 0);

    // an option taking a value of the given type
    void AddOption(const wxString& name, const wxString& lng = wxEmptyString,
                   const wxString& desc = wxEmptyString,
                   wxCmdLineParamType type = wxCMD_LINE_VAL_STRING,
                   int flags = 0);

    // a parameter
    void AddParam(const wxString& desc = wxEmptyString,
                  wxCmdLineParamType type = wxCMD_LINE_VAL_STRING,
                  int flags = 0);

    // actions
    // -------

    // parse the command line, return 0 if ok, -1 if "-h" or "--help" option
    // was encountered and the help message was given or a positive value if a
    // syntax error occurred
    //
    // if showUsage is true, Usage() is called in case of syntax error or if
    // help was requested
    int Parse(bool showUsage = true);

    // give the usage message describing all program options
    void Usage();

    // get the command line arguments
    // ------------------------------

    // returns true if the given switch was found
    bool Found(const wxString& name) const;

    // returns true if an option taking a string value was found and stores the
    // value in the provided pointer
    bool Found(const wxString& name, wxString *value) const;

    // returns true if an option taking an integer value was found and stores
    // the value in the provided pointer
    bool Found(const wxString& name, long *value) const;

#if wxUSE_DATETIME
    // returns true if an option taking a date value was found and stores the
    // value in the provided pointer
    bool Found(const wxString& name, wxDateTime *value) const;
#endif // wxUSE_DATETIME

    // gets the number of parameters found
    size_t GetParamCount() const;

    // gets the value of Nth parameter (as string only for now)
    wxString GetParam(size_t n = 0u) const;

    // Resets switches and options
    void Reset();

    // break down the command line in arguments
    static wxArrayString ConvertStringToArgs(const wxChar *cmdline);

private:
    // get usage string
    wxString GetUsageString();

    // common part of all ctors
    void Init();

    struct wxCmdLineParserData *m_data;

    DECLARE_NO_COPY_CLASS(wxCmdLineParser)
};

#else // !wxUSE_CMDLINE_PARSER

// this function is always available (even if !wxUSE_CMDLINE_PARSER) because it
// is used by wxWin itself under Windows
class WXDLLIMPEXP_BASE wxCmdLineParser
{
public:
    static wxArrayString ConvertStringToArgs(const wxChar *cmdline);
};

#endif // wxUSE_CMDLINE_PARSER/!wxUSE_CMDLINE_PARSER

#endif // _WX_CMDLINE_H_

