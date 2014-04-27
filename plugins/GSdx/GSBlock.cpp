/*
 *	Copyright (C) 2007-2009 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GSBlock.h"

#if _M_SSE >= 0x501
const GSVector8i GSBlock::m_r16mask(0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15, 0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15);
#else
const GSVector4i GSBlock::m_r16mask(0, 1, 4, 5, 2, 3, 6, 7, 8, 9, 12, 13, 10, 11, 14, 15);
#endif
const GSVector4i GSBlock::m_r8mask(0, 4, 2, 6, 8, 12, 10, 14, 1, 5, 3, 7, 9, 13, 11, 15);
const GSVector4i GSBlock::m_r4mask(0, 1, 4, 5, 8, 9, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15);

#if _M_SSE >= 0x501
const GSVector8i GSBlock::m_xxxa(0x00008000);
const GSVector8i GSBlock::m_xxbx(0x00007c00);
const GSVector8i GSBlock::m_xgxx(0x000003e0);
const GSVector8i GSBlock::m_rxxx(0x0000001f);
#else
const GSVector4i GSBlock::m_xxxa(0x00008000);
const GSVector4i GSBlock::m_xxbx(0x00007c00);
const GSVector4i GSBlock::m_xgxx(0x000003e0);
const GSVector4i GSBlock::m_rxxx(0x0000001f);
#endif

const GSVector4i GSBlock::m_uw8hmask0(0, 0, 0, 0, 1, 1, 1, 1, 8, 8, 8, 8, 9, 9, 9, 9);
const GSVector4i GSBlock::m_uw8hmask1(2, 2, 2, 2, 3, 3, 3, 3, 10, 10, 10, 10, 11, 11, 11, 11);
const GSVector4i GSBlock::m_uw8hmask2(4, 4, 4, 4, 5, 5, 5, 5, 12, 12, 12, 12, 13, 13, 13, 13);
const GSVector4i GSBlock::m_uw8hmask3(6, 6, 6, 6, 7, 7, 7, 7, 14, 14, 14, 14, 15, 15, 15, 15);
