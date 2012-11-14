///////////////////////////////////////////////////////////////////////////////
// Name:        dbkeyg.h
// Purpose:     Generic key support for wxDbTable
// Author:      Roger Gammans
// Modified by:
// Created:
// RCS-ID:      $Id: dbkeyg.h 29077 2004-09-10 12:56:07Z ABX $
// Copyright:   (c) 1999 The Computer Surgery (roger@computer-surgery.co.uk)
// Licence:     wxWindows licence
//
// NOTE : There is no CPP file to go along with this
//
///////////////////////////////////////////////////////////////////////////////
// Branched From : gkey.h,v 1.3 2001/06/01 10:31:41
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_DBGKEY_H_
#define _WX_DBGKEY_H_

class GenericKey
{
public:
    GenericKey(void *blk, size_t sz)    { clone(blk,sz); }
    GenericKey(const GenericKey &ref)   { clone(ref.m_data,ref.m_sz); }
    ~GenericKey()                       { free(m_data); }

    void *GetBlk(void) const { return m_data; }

private:
    void clone(void *blk, size_t sz)
    {
        m_data = malloc(sz);
        memcpy(m_data,blk,sz);
        m_sz = sz;
    }

    void   *m_data;
    size_t  m_sz;
};

#endif // _WX_DBGKEY_H_
