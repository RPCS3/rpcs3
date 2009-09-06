///////////////////////////////////////////////////////////////////////////////
// Name:        wx/univ/chkconf.h
// Purpose:     wxUniversal-specific configuration options checks
// Author:      Vadim Zeitlin
// Created:     2006-09-28 (extracted from wx/chkconf.h)
// RCS-ID:      $Id: chkconf.h 41494 2006-09-28 22:55:28Z VZ $
// Copyright:   (c) 2006 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_UNIV_CHKCONF_H_
#define _WX_UNIV_CHKCONF_H_

#if (wxUSE_COMBOBOX || wxUSE_MENUS) && !wxUSE_POPUPWIN
#    ifdef wxABORT_ON_CONFIG_ERROR
#        error "wxUSE_POPUPWIN must be defined to use comboboxes/menus"
#    else
#        undef wxUSE_POPUPWIN
#        define wxUSE_POPUPWIN 1
#    endif
#endif

#if wxUSE_COMBOBOX
#   if !wxUSE_LISTBOX
#        ifdef wxABORT_ON_CONFIG_ERROR
#            error "wxComboBox requires wxListBox in wxUniversal"
#        else
#            undef wxUSE_LISTBOX
#            define wxUSE_LISTBOX 1
#        endif
#   endif
#endif /* wxUSE_COMBOBOX */

#if wxUSE_RADIOBTN
#   if !wxUSE_CHECKBOX
#        ifdef wxABORT_ON_CONFIG_ERROR
#            error "wxUSE_RADIOBTN requires wxUSE_CHECKBOX in wxUniversal"
#        else
#            undef wxUSE_CHECKBOX
#            define wxUSE_CHECKBOX 1
#        endif
#   endif
#endif /* wxUSE_RADIOBTN */

#if wxUSE_TEXTCTRL
#    if !wxUSE_CARET
#        ifdef wxABORT_ON_CONFIG_ERROR
#            error "wxTextCtrl requires wxCaret in wxUniversal"
#        else
#            undef wxUSE_CARET
#            define wxUSE_CARET 1
#        endif
#    endif /* wxUSE_CARET */

#    if !wxUSE_SCROLLBAR
#        ifdef wxABORT_ON_CONFIG_ERROR
#            error "wxTextCtrl requires wxScrollBar in wxUniversal"
#        else
#            undef wxUSE_SCROLLBAR
#            define wxUSE_SCROLLBAR 1
#        endif
#    endif /* wxUSE_SCROLLBAR */
#endif /* wxUSE_TEXTCTRL */


/* Themes checks */
#ifndef wxUSE_ALL_THEMES
#   ifdef wxABORT_ON_CONFIG_ERROR
#       error "wxUSE_ALL_THEMES must be defined"
#   else
#       define wxUSE_ALL_THEMES 1
#   endif
#endif /* wxUSE_ALL_THEMES */

#ifndef wxUSE_THEME_GTK
#   ifdef wxABORT_ON_CONFIG_ERROR
#       error "wxUSE_THEME_GTK must be defined"
#   else
#       define wxUSE_THEME_GTK 1
#   endif
#endif /* wxUSE_THEME_GTK */

#ifndef wxUSE_THEME_METAL
#   ifdef wxABORT_ON_CONFIG_ERROR
#       error "wxUSE_THEME_METAL must be defined"
#   else
#       define wxUSE_THEME_METAL 1
#   endif
#endif /* wxUSE_THEME_METAL */

#ifndef wxUSE_THEME_MONO
#   ifdef wxABORT_ON_CONFIG_ERROR
#       error "wxUSE_THEME_MONO must be defined"
#   else
#       define wxUSE_THEME_MONO 1
#   endif
#endif /* wxUSE_THEME_MONO */

#ifndef wxUSE_THEME_WIN32
#   ifdef wxABORT_ON_CONFIG_ERROR
#       error "wxUSE_THEME_WIN32 must be defined"
#   else
#       define wxUSE_THEME_WIN32 1
#   endif
#endif /* wxUSE_THEME_WIN32 */

#if !wxUSE_ALL_THEMES && wxUSE_THEME_METAL && !wxUSE_THEME_WIN32
#   ifdef wxABORT_ON_CONFIG_ERROR
#       error "Metal theme requires Win32 one"
#   else
#       undef wxUSE_THEME_WIN32
#       define wxUSE_THEME_WIN32 1
#   endif
#endif /* wxUSE_THEME_METAL && !wxUSE_THEME_WIN32 */

#endif /* _WX_UNIV_CHKCONF_H_ */

