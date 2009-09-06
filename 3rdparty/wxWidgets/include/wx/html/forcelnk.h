/////////////////////////////////////////////////////////////////////////////
// Name:        forcelnk.h
// Purpose:     see bellow
// Author:      Vaclav Slavik
// RCS-ID:      $Id: forcelnk.h 35686 2005-09-25 18:46:14Z VZ $
// Copyright:   (c) Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

/*

DESCRPITON:

mod_*.cpp files contain handlers for tags. These files are modules - they contain
one wxTagModule class and it's OnInit() method is called from wxApp's init method.
The module is called even if you only link it into the executable, so everything
seems wonderful.

The problem is that we have these modules in LIBRARY and mod_*.cpp files contain
no method nor class which is known out of the module. So the linker won't
link these .o/.obj files into executable because it detected that it is not used
by the program.

To workaround this I introduced set of macros FORCE_LINK_ME and FORCE_LINK. These
macros are generic and are not limited to mod_*.cpp files. You may find them quite
useful somewhere else...

How to use them:
let's suppose you want to always link file foo.cpp and that you have module
always.cpp that is certainly always linked (e.g. the one with main() function
or htmlwin.cpp in wxHtml library).

Place FORCE_LINK_ME(foo) somewhere in foo.cpp and FORCE_LINK(foo) somewhere
in always.cpp
See mod_*.cpp and htmlwin.cpp for example :-)

*/


#ifndef _WX_FORCELNK_H_
#define _WX_FORCELNK_H_

#include "wx/link.h"

// compatibility defines
#define FORCE_LINK wxFORCE_LINK_MODULE
#define FORCE_LINK_ME wxFORCE_LINK_THIS_MODULE

#define FORCE_WXHTML_MODULES() \
    FORCE_LINK(m_layout) \
    FORCE_LINK(m_fonts) \
    FORCE_LINK(m_image) \
    FORCE_LINK(m_list) \
    FORCE_LINK(m_dflist) \
    FORCE_LINK(m_pre) \
    FORCE_LINK(m_hline) \
    FORCE_LINK(m_links) \
    FORCE_LINK(m_tables) \
    FORCE_LINK(m_style)


#endif // _WX_FORCELNK_H_
