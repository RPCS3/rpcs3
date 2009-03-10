/*
 * wx/msw/htmlhelp.h
 * Copyright 2004 Jacek Caban
 *
 * Originally written for the Wine project, and issued under
 * the wxWindows License by kind permission of the author.
 *
 * License:     wxWindows License
 */

#ifndef __HTMLHELP_H__
#define __HTMLHELP_H__

#define HH_DISPLAY_TOPIC        0x00
#define HH_HELP_FINDER          0x00
#define HH_DISPLAY_TOC          0x01
#define HH_DISPLAY_INDEX        0x02
#define HH_DISPLAY_SEARCH       0x03
#define HH_SET_WIN_TYPE         0x04
#define HH_GET_WIN_TYPE         0x05
#define HH_GET_WIN_HANDLE       0x06
#define HH_ENUM_INFO_TYPE       0x07
#define HH_SET_INFO_TYPE        0x08
#define HH_SYNC                 0x09
#define HH_RESERVED1            0x0A
#define HH_RESERVED2            0x0B
#define HH_RESERVED3            0x0C
#define HH_KEYWORD_LOOKUP       0x0D
#define HH_DISPLAY_TEXT_POPUP   0x0E
#define HH_HELP_CONTEXT         0x0F
#define HH_TP_HELP_CONTEXTMENU  0x10
#define HH_TP_HELP_WM_HELP      0x11
#define HH_CLOSE_ALL            0x12
#define HH_ALINK_LOOKUP         0x13
#define HH_GET_LAST_ERROR       0x14
#define HH_ENUM_CATEGORY        0x15
#define HH_ENUM_CATEGORY_IT     0x16
#define HH_RESET_IT_FILTER      0x17
#define HH_SET_INCLUSIVE_FILTER 0x18
#define HH_SET_EXCLUSIVE_FILTER 0x19
#define HH_INITIALIZE           0x1C
#define HH_UNINITIALIZE         0x1D
#define HH_PRETRANSLATEMESSAGE  0xFD
#define HH_SET_GLOBAL_PROPERTY  0xFC

#define HHWIN_PROP_TAB_AUTOHIDESHOW  0x00000001
#define HHWIN_PROP_ONTOP             0x00000002
#define HHWIN_PROP_NOTITLEBAR        0x00000004
#define HHWIN_PROP_NODEF_STYLES      0x00000008
#define HHWIN_PROP_NODEF_EXSTYLES    0x00000010
#define HHWIN_PROP_TRI_PANE          0x00000020
#define HHWIN_PROP_NOTB_TEXT         0x00000040
#define HHWIN_PROP_POST_QUIT         0x00000080
#define HHWIN_PROP_AUTO_SYNC         0x00000100
#define HHWIN_PROP_TRACKING          0x00000200
#define HHWIN_PROP_TAB_SEARCH        0x00000400
#define HHWIN_PROP_TAB_HISTORY       0x00000800
#define HHWIN_PROP_TAB_FAVORITES     0x00001000
#define HHWIN_PROP_CHANGE_TITLE      0x00002000
#define HHWIN_PROP_NAV_ONLY_WIN      0x00004000
#define HHWIN_PROP_NO_TOOLBAR        0x00008000
#define HHWIN_PROP_MENU              0x00010000
#define HHWIN_PROP_TAB_ADVSEARCH     0x00020000
#define HHWIN_PROP_USER_POS          0x00040000
#define HHWIN_PROP_TAB_CUSTOM1       0x00080000
#define HHWIN_PROP_TAB_CUSTOM2       0x00100000
#define HHWIN_PROP_TAB_CUSTOM3       0x00200000
#define HHWIN_PROP_TAB_CUSTOM4       0x00400000
#define HHWIN_PROP_TAB_CUSTOM5       0x00800000
#define HHWIN_PROP_TAB_CUSTOM6       0x01000000
#define HHWIN_PROP_TAB_CUSTOM7       0x02000000
#define HHWIN_PROP_TAB_CUSTOM8       0x04000000
#define HHWIN_PROP_TAB_CUSTOM9       0x08000000
#define HHWIN_TB_MARGIN              0x10000000

#define HHWIN_PARAM_PROPERTIES     0x00000002
#define HHWIN_PARAM_STYLES         0x00000004
#define HHWIN_PARAM_EXSTYLES       0x00000008
#define HHWIN_PARAM_RECT           0x00000010
#define HHWIN_PARAM_NAV_WIDTH      0x00000020
#define HHWIN_PARAM_SHOWSTATE      0x00000040
#define HHWIN_PARAM_INFOTYPES      0x00000080
#define HHWIN_PARAM_TB_FLAGS       0x00000100
#define HHWIN_PARAM_EXPANSION      0x00000200
#define HHWIN_PARAM_TABPOS         0x00000400
#define HHWIN_PARAM_TABORDER       0x00000800
#define HHWIN_PARAM_HISTORY_COUNT  0x00001000
#define HHWIN_PARAM_CUR_TAB        0x00002000

#define HHWIN_BUTTON_EXPAND      0x00000002
#define HHWIN_BUTTON_BACK        0x00000004
#define HHWIN_BUTTON_FORWARD     0x00000008
#define HHWIN_BUTTON_STOP        0x00000010
#define HHWIN_BUTTON_REFRESH     0x00000020
#define HHWIN_BUTTON_HOME        0x00000040
#define HHWIN_BUTTON_BROWSE_FWD  0x00000080
#define HHWIN_BUTTON_BROWSE_BCK  0x00000100
#define HHWIN_BUTTON_NOTES       0x00000200
#define HHWIN_BUTTON_CONTENTS    0x00000400
#define HHWIN_BUTTON_SYNC        0x00000800
#define HHWIN_BUTTON_OPTIONS     0x00001000
#define HHWIN_BUTTON_PRINT       0x00002000
#define HHWIN_BUTTON_INDEX       0x00004000
#define HHWIN_BUTTON_SEARCH      0x00008000
#define HHWIN_BUTTON_HISTORY     0x00010000
#define HHWIN_BUTTON_FAVORITES   0x00020000
#define HHWIN_BUTTON_JUMP1       0x00040000
#define HHWIN_BUTTON_JUMP2       0x00080000
#define HHWIN_BUTTON_ZOOM        0x00100000
#define HHWIN_BUTTON_TOC_NEXT    0x00200000
#define HHWIN_BUTTON_TOC_PREV    0x00400000

#define HHWIN_DEF_BUTTONS  \
    (HHWIN_BUTTON_EXPAND | HHWIN_BUTTON_BACK | HHWIN_BUTTON_OPTIONS | HHWIN_BUTTON_PRINT)

#define IDTB_EXPAND       200
#define IDTB_CONTRACT     201
#define IDTB_STOP         202
#define IDTB_REFRESH      203
#define IDTB_BACK         204
#define IDTB_HOME         205
#define IDTB_SYNC         206
#define IDTB_PRINT        207
#define IDTB_OPTIONS      208
#define IDTB_FORWARD      209
#define IDTB_NOTES        210
#define IDTB_BROWSE_FWD   211
#define IDTB_BROWSE_BACK  212
#define IDTB_CONTENTS     213
#define IDTB_INDEX        214
#define IDTB_SEARCH       215
#define IDTB_HISTORY      216
#define IDTB_FAVORITES    217
#define IDTB_JUMP1        218
#define IDTB_JUMP2        219
#define IDTB_CUSTOMIZE    221
#define IDTB_ZOOM         222
#define IDTB_TOC_NEXT     223
#define IDTB_TOC_PREV     224

#define HHN_FIRST          (0U-860U)
#define HHN_LAST           (0U-879U)
#define HHN_NAVCOMPLETE    HHN_FIRST
#define HHN_TRACK          (HHN_FIRST-1)
#define HHN_WINDOW_CREATE  (HHN_FIRST-2)


#ifdef __cplusplus
extern "C" {
#endif

typedef struct tagHH_NOTIFY {
    NMHDR hdr;
    PCSTR pszurl;
} HH_NOTIFY;

typedef struct tagHH_POPUPA {
    int       cbStruct;
    HINSTANCE hinst;
    UINT      idString;
    LPCSTR    pszText;
    POINT     pt;
    COLORREF  clrForeground;
    COLORREF  clrBackground;
    RECT      rcMargins;
    LPCSTR    pszFont;
} HH_POPUPA;

typedef struct tagHH_POPUPW {
    int       cbStruct;
    HINSTANCE hinst;
    UINT      idString;
    LPCWSTR   pszText;
    POINT     pt;
    COLORREF  clrForeground;
    COLORREF  clrBackground;
    RECT      rcMargins;
    LPCWSTR   pszFont;
} HH_POPUPW;

#ifdef _UNICODE
typedef HH_POPUPW HH_POPUP;
#else
typedef HH_POPUPA HH_POPUP;
#endif

typedef struct tagHH_ALINKA {
    int    cbStruct;
    BOOL   fReserved;
    LPCSTR pszKeywords;
    LPCSTR pszUrl;
    LPCSTR pszMsgText;
    LPCSTR pszMsgTitle;
    LPCSTR pszWindow;
    BOOL   fIndexOnFail;
} HH_ALINKA;

typedef struct tagHH_ALINKW {
    int     cbStruct;
    BOOL    fReserved;
    LPCWSTR pszKeywords;
    LPCWSTR pszUrl;
    LPCWSTR pszMsgText;
    LPCWSTR pszMsgTitle;
    LPCWSTR pszWindow;
    BOOL    fIndexOnFail;
} HH_ALINKW;

#ifdef _UNICODE
typedef HH_ALINKW HH_ALINK;
typedef HH_ALINKW HH_AKLINK;
#else
typedef HH_ALINKA HH_ALINK;
typedef HH_ALINKA HH_AKLINK;
#endif

enum {
    HHWIN_NAVTYPE_TOC,
    HHWIN_NAVTYPE_INDEX,
    HHWIN_NAVTYPE_SEARCH,
    HHWIN_NAVTYPE_FAVORITES,
    HHWIN_NAVTYPE_HISTORY,
    HHWIN_NAVTYPE_AUTHOR,
    HHWIN_NAVTYPE_CUSTOM_FIRST = 11
};

enum {
    IT_INCLUSIVE,
    IT_EXCLUSIVE,
    IT_HIDDEN
};

typedef struct tagHH_ENUM_IT {
    int    cbStruct;
    int    iType;
    LPCSTR pszCatName;
    LPCSTR pszITName;
    LPCSTR pszITDescription;
} HH_ENUM_IT, *PHH_ENUM_IT;

typedef struct tagHH_ENUM_CAT {
    int    cbStruct;
    LPCSTR pszCatName;
    LPCSTR pszCatDescription;
} HH_ENUM_CAT, *PHH_ENUM_CAT;

typedef struct tagHH_SET_INFOTYPE {
    int    cbStruct;
    LPCSTR pszCatName;
    LPCSTR pszInfoTypeName;
} HH_SET_INFOTYPE;

typedef DWORD HH_INFOTYPE, *PHH_INFOTYPE;

enum {
    HHWIN_NAVTAB_TOP,
    HHWIN_NAVTAB_LEFT,
    HHWIN_NAVTAB_BOTTOM
};

#define HH_MAX_TABS 19

enum {
    HH_TAB_CONTENTS,
    HH_TAB_INDEX,
    HH_TAB_SEARCH,
    HH_TAB_FAVORITES,
    HH_TAB_HISTORY,
    HH_TAB_AUTHOR,
    HH_TAB_CUSTOM_FIRST = 11,
    HH_TAB_CUSTOM_LAST = HH_MAX_TABS
};

#define HH_MAX_TABS_CUSTOM        (HH_TAB_CUSTOM_LAST-HH_TAB_CUSTOM_FIRST+1)
#define HH_FTS_DEFAULT_PROXIMITY  -1

typedef struct tagHH_FTS_QUERYA {
    int    cbStruct;
    BOOL   fUniCodeStrings;
    LPCSTR pszSearchQuery;
    LONG   iProximity;
    BOOL   fStemmedSearch;
    BOOL   fTitleOnly;
    BOOL   fExecute;
    LPCSTR pszWindow;
} HH_FTS_QUERYA;

typedef struct tagHH_FTS_QUERYW {
    int     cbStruct;
    BOOL    fUniCodeStrings;
    LPCWSTR pszSearchQuery;
    LONG    iProximity;
    BOOL    fStemmedSearch;
    BOOL    fTitleOnly;
    BOOL    fExecute;
    LPCWSTR pszWindow;
} HH_FTS_QUERYW;

#ifdef _UNICODE
typedef HH_FTS_QUERYW HH_FTS_QUERY;
#else
typedef HH_FTS_QUERYA HH_FTS_QUERY;
#endif

typedef struct tagHH_WINTYPEA {
    int          cbStruct;
    BOOL         fUniCodeStrings;
    LPCSTR       pszType;
    DWORD        fsValidMembers;
    DWORD        fsWinProperties;
    LPCSTR       pszCaption;
    DWORD        dwStyles;
    DWORD        dwExStyles;
    RECT         rcWindowPos;
    int          nShowState;
    HWND         hwndHelp;
    HWND         hwndCaller;
    PHH_INFOTYPE paInfoTypes;
    HWND         hwndToolBar;
    HWND         hwndNavigation;
    HWND         hwndHTML;
    int          iNavWidth;
    RECT         rcHTML;
    LPCSTR       pszToc;
    LPCSTR       pszIndex;
    LPCSTR       pszFile;
    LPCSTR       pszHome;
    DWORD        fsToolBarFlags;
    BOOL         fNotExpanded;
    int          curNavType;
    int          tabpos;
    int          idNotify;
    BYTE         tabOrder[HH_MAX_TABS+1];
    int          cHistory;
    LPCSTR       pszJump1;
    LPCSTR       pszJump2;
    LPCSTR       pszUrlJump1;
    LPCSTR       pszUrlJump2;
    RECT         rcMinSize;
    int          cbInfoTypes;
    LPCSTR       pszCustomTabs;
} HH_WINTYPEA, *PHH_WINTYPEA;

typedef struct tagHH_WINTYPEW {
    int          cbStruct;
    BOOL         fUniCodeStrings;
    LPCWSTR      pszType;
    DWORD        fsValidMembers;
    DWORD        fsWinProperties;
    LPCWSTR      pszCaption;
    DWORD        dwStyles;
    DWORD        dwExStyles;
    RECT         rcWindowPos;
    int          nShowState;
    HWND         hwndHelp;
    HWND         hwndCaller;
    PHH_INFOTYPE paInfoTypes;
    HWND         hwndToolBar;
    HWND         hwndNavigation;
    HWND         hwndHTML;
    int          iNavWidth;
    RECT         rcHTML;
    LPCWSTR      pszToc;
    LPCWSTR      pszIndex;
    LPCWSTR      pszFile;
    LPCWSTR      pszHome;
    DWORD        fsToolBarFlags;
    BOOL         fNotExpanded;
    int          curNavType;
    int          tabpos;
    int          idNotify;
    BYTE         tabOrder[HH_MAX_TABS+1];
    int          cHistory;
    LPCWSTR      pszJump1;
    LPCWSTR      pszJump2;
    LPCWSTR      pszUrlJump1;
    LPCWSTR      pszUrlJump2;
    RECT         rcMinSize;
    int          cbInfoTypes;
    LPCWSTR      pszCustomTabs;
} HH_WINTYPEW, *PHH_WINTYPEW;

#ifdef _UNICODE
typedef HH_WINTYPEW HH_WINTYPE;
#else
typedef HH_WINTYPEA HH_WINTYPE;
#endif

enum {
    HHACT_TAB_CONTENTS,
    HHACT_TAB_INDEX,
    HHACT_TAB_SEARCH,
    HHACT_TAB_HISTORY,
    HHACT_TAB_FAVORITES,
    HHACT_EXPAND,
    HHACT_CONTRACT,
    HHACT_BACK,
    HHACT_FORWARD,
    HHACT_STOP,
    HHACT_REFRESH,
    HHACT_HOME,
    HHACT_SYNC,
    HHACT_OPTIONS,
    HHACT_PRINT,
    HHACT_HIGHLIGHT,
    HHACT_CUSTOMIZE,
    HHACT_JUMP1,
    HHACT_JUMP2,
    HHACT_ZOOM,
    HHACT_TOC_NEXT,
    HHACT_TOC_PREV,
    HHACT_NOTES,
    HHACT_LAST_ENUM
};

typedef struct tagHH_NTRACKA {
    NMHDR        hdr;
    PCSTR        pszCurUrl;
    int          idAction;
    PHH_WINTYPEA phhWinType;
} HH_NTRACKA;

typedef struct tagHH_NTRACKW {
    NMHDR        hdr;
    PCSTR        pszCurUrl;
    int          idAction;
    PHH_WINTYPEW phhWinType;
} HH_NTRACKW;

#ifdef _UNICODE
typedef HH_NTRACKW HH_NTRACK;
#else
typedef HH_NTRACKA HH_NTRACK;
#endif

HWND WINAPI HtmlHelpA(HWND,LPCSTR,UINT,DWORD);
HWND WINAPI HtmlHelpA(HWND,LPCSTR,UINT,DWORD);
#define HtmlHelp WINELIB_NAME_AW(HtmlHelp)

#define ATOM_HTMLHELP_API_ANSI    (LPTSTR)14
#define ATOM_HTMLHELP_API_UNICODE (LPTSTR)15

typedef enum tagHH_GPROPID {
    HH_GPROPID_SINGLETHREAD     = 1,
    HH_GPROPID_TOOLBAR_MARGIN   = 2,
    HH_GPROPID_UI_LANGUAGE      = 3,
    HH_GPROPID_CURRENT_SUBSET   = 4,
    HH_GPROPID_CONTENT_LANGUAGE = 5
} HH_GPROPID;

#ifdef __WIDL_OAIDL_H

typedef struct tagHH_GLOBAL_PROPERTY
{
    HH_GPROPID  id;
    VARIANT     var;
} HH_GLOBAL_PROPERTY ;

#endif /* __WIDL_OAIDL_H */

#ifdef __cplusplus
}
#endif

#endif /* __HTMLHELP_H__ */
