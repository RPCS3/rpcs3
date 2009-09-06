/////////////////////////////////////////////////////////////////////////////
// Name:        wx/link.h
// Purpose:     macros to force linking modules which might otherwise be
//              discarded by the linker
// Author:      Vaclav Slavik
// RCS-ID:      $Id: link.h 35722 2005-09-26 12:29:25Z VZ $
// Copyright:   (c) Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_LINK_H_
#define _WX_LINK_H_

// This must be part of the module you want to force:
#define wxFORCE_LINK_THIS_MODULE(module_name)                         \
                extern void _wx_link_dummy_func_##module_name ();     \
                void _wx_link_dummy_func_##module_name () { }


// And this must be somewhere where it certainly will be linked:
#define wxFORCE_LINK_MODULE(module_name)                              \
                extern void _wx_link_dummy_func_##module_name ();     \
                static struct wxForceLink##module_name                \
                {                                                     \
                    wxForceLink##module_name()                        \
                    {                                                 \
                        _wx_link_dummy_func_##module_name ();         \
                    }                                                 \
                } _wx_link_dummy_var_##module_name;


#endif // _WX_LINK_H_
