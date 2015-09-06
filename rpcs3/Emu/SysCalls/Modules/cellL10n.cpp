#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#ifdef _MSC_VER
#include <windows.h>
#else
#include <iconv.h>
#endif

#include "cellL10n.h"

extern Module cellL10n;

// Converts *src_code and *dst_code to converter ID
inline static const s32 GetConverter(u32 src_code, u32 dst_code)
{
	switch (src_code)
	{
	case L10N_UTF8:
		switch (dst_code)
		{
		case L10N_UTF16:         return 1;
		case L10N_UTF32:         return 2;
		case L10N_UCS2:          return 3;
		case L10N_ISO_8859_1:    return 5;
		case L10N_ISO_8859_2:    return 6;
		case L10N_ISO_8859_3:    return 7;
		case L10N_ISO_8859_4:    return 8;
		case L10N_ISO_8859_5:    return 9;
		case L10N_ISO_8859_6:    return 10;
		case L10N_ISO_8859_7:    return 11;
		case L10N_ISO_8859_8:    return 12;
		case L10N_ISO_8859_9:    return 13;
		case L10N_ISO_8859_10:   return 14;
		case L10N_ISO_8859_11:   return 15;
		case L10N_ISO_8859_13:   return 16;
		case L10N_ISO_8859_14:   return 17;
		case L10N_ISO_8859_15:   return 18;
		case L10N_ISO_8859_16:   return 19;
		case L10N_CODEPAGE_437:  return 20;
		case L10N_CODEPAGE_850:  return 21;
		case L10N_CODEPAGE_863:  return 22;
		case L10N_CODEPAGE_866:  return 23;
		case L10N_CODEPAGE_932:  return 24;
		case L10N_CODEPAGE_936:  return 25;
		case L10N_CODEPAGE_949:  return 26;
		case L10N_CODEPAGE_950:  return 27;
		case L10N_CODEPAGE_1251: return 28;
		case L10N_CODEPAGE_1252: return 29;
		case L10N_EUC_CN:        return 30;
		case L10N_EUC_JP:        return 31;
		case L10N_EUC_KR:        return 32;
		case L10N_ISO_2022_JP:   return 33;
		case L10N_ARIB:          return 34;
		case L10N_HZ:            return 35;
		case L10N_GB18030:       return 36;
		case L10N_RIS_506:       return 37;
		case L10N_CODEPAGE_852:  return 38;
		case L10N_CODEPAGE_1250: return 39;
		case L10N_CODEPAGE_737:  return 40;
		case L10N_CODEPAGE_1253: return 41;
		case L10N_CODEPAGE_857:  return 42;
		case L10N_CODEPAGE_1254: return 43;
		case L10N_CODEPAGE_775:  return 44;
		case L10N_CODEPAGE_1257: return 45;
		case L10N_CODEPAGE_855:  return 46;
		case L10N_CODEPAGE_858:  return 47;
		case L10N_CODEPAGE_860:  return 48;
		case L10N_CODEPAGE_861:  return 49;
		case L10N_CODEPAGE_865:  return 50;
		case L10N_CODEPAGE_869:  return 51;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_UTF16:
		switch (dst_code)
		{
		case L10N_UTF8:  return 65536;
		case L10N_UTF32: return 65538;
		case L10N_UCS2:  return 65539;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_UTF32:
		switch (dst_code)
		{
		case L10N_UTF8:  return 131072;
		case L10N_UTF16: return 131073;
		case L10N_UCS2:  return 131075;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_UCS2:
		switch (dst_code)
		{
		case L10N_UTF8:          return 196608;
		case L10N_UTF16:         return 196609;
		case L10N_UTF32:         return 196610;
		case L10N_ISO_8859_1:    return 196613;
		case L10N_ISO_8859_2:    return 196614;
		case L10N_ISO_8859_3:    return 196615;
		case L10N_ISO_8859_4:    return 196616;
		case L10N_ISO_8859_5:    return 196617;
		case L10N_ISO_8859_6:    return 196618;
		case L10N_ISO_8859_7:    return 196619;
		case L10N_ISO_8859_8:    return 196620;
		case L10N_ISO_8859_9:    return 196621;
		case L10N_ISO_8859_10:   return 196622;
		case L10N_ISO_8859_11:   return 196623;
		case L10N_ISO_8859_13:   return 196624;
		case L10N_ISO_8859_14:   return 196625;
		case L10N_ISO_8859_15:   return 196626;
		case L10N_ISO_8859_16:   return 196627;
		case L10N_CODEPAGE_437:  return 196628;
		case L10N_CODEPAGE_850:  return 196629;
		case L10N_CODEPAGE_863:  return 196630;
		case L10N_CODEPAGE_866:  return 196631;
		case L10N_CODEPAGE_932:  return 196632;
		case L10N_CODEPAGE_936:  return 196633;
		case L10N_CODEPAGE_949:  return 196634;
		case L10N_CODEPAGE_950:  return 196635;
		case L10N_CODEPAGE_1251: return 196636;
		case L10N_CODEPAGE_1252: return 196637;
		case L10N_EUC_CN:        return 196638;
		case L10N_EUC_JP:        return 196639;
		case L10N_EUC_KR:        return 196640;
		case L10N_ISO_2022_JP:   return 196641;
		case L10N_ARIB:          return 196642;
		case L10N_HZ:            return 196643;
		case L10N_GB18030:       return 196644;
		case L10N_RIS_506:       return 196645;
		case L10N_CODEPAGE_852:  return 196646;
		case L10N_CODEPAGE_1250: return 196647;
		case L10N_CODEPAGE_737:  return 196648;
		case L10N_CODEPAGE_1253: return 196649;
		case L10N_CODEPAGE_857:  return 196650;
		case L10N_CODEPAGE_1254: return 196651;
		case L10N_CODEPAGE_775:  return 196652;
		case L10N_CODEPAGE_1257: return 196653;
		case L10N_CODEPAGE_855:  return 196654;
		case L10N_CODEPAGE_858:  return 196655;
		case L10N_CODEPAGE_860:  return 196656;
		case L10N_CODEPAGE_861:  return 196657;
		case L10N_CODEPAGE_865:  return 196658;
		case L10N_CODEPAGE_869:  return 196659;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_ISO_8859_1:
		switch (dst_code)
		{
		case L10N_UTF8: return 327680;
		case L10N_UCS2: return 327683;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_ISO_8859_2:
		switch (dst_code)
		{
		case L10N_UTF8: return 393216;
		case L10N_UCS2: return 393219;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_ISO_8859_3:
		switch (dst_code)
		{
		case L10N_UTF8: return 458752;
		case L10N_UCS2: return 458755;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_ISO_8859_4:
		switch (dst_code)
		{
		case L10N_UTF8: return 524288;
		case L10N_UCS2: return 524291;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_ISO_8859_5:
		switch (dst_code)
		{
		case L10N_UTF8: return 589824;
		case L10N_UCS2: return 589827;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_ISO_8859_6:
		switch (dst_code)
		{
		case L10N_UTF8: return 655360;
		case L10N_UCS2: return 655363;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_ISO_8859_7:
		switch (dst_code)
		{
		case L10N_UTF8: return 720896;
		case L10N_UCS2: return 720899;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_ISO_8859_8:
		switch (dst_code)
		{
		case L10N_UTF8: return 786432;
		case L10N_UCS2: return 786435;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_ISO_8859_9:
		switch (dst_code)
		{
		case L10N_UTF8: return 851968;
		case L10N_UCS2: return 851971;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_ISO_8859_10:
		switch (dst_code)
		{
		case L10N_UTF8: return 917504;
		case L10N_UCS2: return 917507;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_ISO_8859_11:
		switch (dst_code)
		{
		case L10N_UTF8: return 983040;
		case L10N_UCS2: return 983043;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_ISO_8859_13:
		switch (dst_code)
		{
		case L10N_UTF8: return 1048576;
		case L10N_UCS2: return 1048579;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_ISO_8859_14:
		switch (dst_code)
		{
		case L10N_UTF8: return 1114112;
		case L10N_UCS2: return 1114115;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_ISO_8859_15:
		switch (dst_code)
		{
		case L10N_UTF8: return 1179648;
		case L10N_UCS2: return 1179651;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_ISO_8859_16:
		switch (dst_code)
		{
		case L10N_UTF8: return 1245184;
		case L10N_UCS2: return 1245187;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_CODEPAGE_437:
		switch (dst_code)
		{
		case L10N_UTF8: return 1310720;
		case L10N_UCS2: return 1310723;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_CODEPAGE_850:
		switch (dst_code)
		{
		case L10N_UTF8: return 1376256;
		case L10N_UCS2: return 1376259;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_CODEPAGE_863:
		switch (dst_code)
		{
		case L10N_UTF8: return 1441792;
		case L10N_UCS2: return 1441795;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_CODEPAGE_866:
		switch (dst_code)
		{
		case L10N_UTF8: return 1507328;
		case L10N_UCS2: return 1507331;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_CODEPAGE_932:
		switch (dst_code)
		{
		case L10N_UTF8: return 1572864;
		case L10N_UCS2: return 1572867;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_CODEPAGE_936:
		switch (dst_code)
		{
		case L10N_UTF8: return 1638400;
		case L10N_UCS2: return 1638403;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_CODEPAGE_949:
		switch (dst_code)
		{
		case L10N_UTF8: return 1703936;
		case L10N_UCS2: return 1703939;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_CODEPAGE_950:
		switch (dst_code)
		{
		case L10N_UTF8: return 1769472;
		case L10N_UCS2: return 1769475;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_CODEPAGE_1251:
		switch (dst_code)
		{
		case L10N_UTF8: return 1835008;
		case L10N_UCS2: return 1835011;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_CODEPAGE_1252:
		switch (dst_code)
		{
		case L10N_UTF8: return 1900544;
		case L10N_UCS2: return 1900547;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_EUC_CN:
		switch (dst_code)
		{
		case L10N_UTF8: return 1966080;
		case L10N_UCS2: return 1966083;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_EUC_JP:
		switch (dst_code)
		{
		case L10N_UTF8: return 2031616;
		case L10N_UCS2: return 2031619;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_EUC_KR:
		switch (dst_code)
		{
		case L10N_UTF8: return 2097152;
		case L10N_UCS2: return 2097155;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_ISO_2022_JP:
		switch (dst_code)
		{
		case L10N_UTF8: return 2162688;
		case L10N_UCS2: return 2162691;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_ARIB:
		switch (dst_code)
		{
		case L10N_UTF8: return 2228224;
		case L10N_UCS2: return 2228227;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_HZ:
		switch (dst_code)
		{
		case L10N_UTF8: return 2293760;
		case L10N_UCS2: return 2293763;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_GB18030:
		switch (dst_code)
		{
		case L10N_UTF8: return 2359296;
		case L10N_UCS2: return 2359299;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_RIS_506:
		switch (dst_code)
		{
		case L10N_UTF8: return 2424832;
		case L10N_UCS2: return 2424835;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_CODEPAGE_852:
		switch (dst_code)
		{
		case L10N_UTF8: return 2490368;
		case L10N_UCS2: return 2490371;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_CODEPAGE_1250:
		switch (dst_code)
		{
		case L10N_UTF8: return 2555904;
		case L10N_UCS2: return 2555907;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_CODEPAGE_737:
		switch (dst_code)
		{
		case L10N_UTF8: return 2621440;
		case L10N_UCS2: return 2621443;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_CODEPAGE_1253:
		switch (dst_code)
		{
		case L10N_UTF8: return 2686976;
		case L10N_UCS2: return 2686979;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_CODEPAGE_857:
		switch (dst_code)
		{
		case L10N_UTF8: return 2752512;
		case L10N_UCS2: return 2752515;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_CODEPAGE_1254:
		switch (dst_code)
		{
		case L10N_UTF8: return 2818048;
		case L10N_UCS2: return 2818051;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_CODEPAGE_775:
		switch (dst_code)
		{
		case L10N_UTF8: return 2883584;
		case L10N_UCS2: return 2883587;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_CODEPAGE_1257:
		switch (dst_code)
		{
		case L10N_UTF8: return 2949120;
		case L10N_UCS2: return 2949123;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_CODEPAGE_855:
		switch (dst_code)
		{
		case L10N_UTF8: return 3014656;
		case L10N_UCS2: return 3014659;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_CODEPAGE_858:
		switch (dst_code)
		{
		case L10N_UTF8: return 3080192;
		case L10N_UCS2: return 3080195;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_CODEPAGE_860:
		switch (dst_code)
		{
		case L10N_UTF8: return 3145728;
		case L10N_UCS2: return 3145731;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_CODEPAGE_861:
		switch (dst_code)
		{
		case L10N_UTF8: return 3211264;
		case L10N_UCS2: return 3211267;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_CODEPAGE_865:
		switch (dst_code)
		{
		case L10N_UTF8: return 3276800;
		case L10N_UCS2: return 3276803;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	case L10N_CODEPAGE_869:
		switch (dst_code)
		{
		case L10N_UTF8: return 3342336;
		case L10N_UCS2: return 3342339;
		default: cellL10n.Error("GetConverter(): Unknown/improper *dst_code %d.", dst_code); break;
		}
	default: cellL10n.Error("GetConverter(): Unknown/improper *src_code %d.", src_code);
	}

	return -1;
}

// Converts converter ID back to *src_code and *dst_code.
inline static const s32 GetConverterSrcAndDst(s32 converter, s32 *src_code, s32 *dst_code)
{
	switch (converter)
	{
	case 1:  *src_code = L10N_UTF8; *dst_code = L10N_UTF16;              return CELL_OK;
	case 2:  *src_code = L10N_UTF8; *dst_code = L10N_UTF32;              return CELL_OK;
	case 3:  *src_code = L10N_UTF8; *dst_code = L10N_UCS2;               return CELL_OK;
	case 5:  *src_code = L10N_UTF8; *dst_code = L10N_ISO_8859_1;         return CELL_OK;
	case 6:  *src_code = L10N_UTF8; *dst_code = L10N_ISO_8859_2;         return CELL_OK;
	case 7:  *src_code = L10N_UTF8; *dst_code = L10N_ISO_8859_3;         return CELL_OK;
	case 8:  *src_code = L10N_UTF8; *dst_code = L10N_ISO_8859_4;         return CELL_OK;
	case 9:  *src_code = L10N_UTF8; *dst_code = L10N_ISO_8859_5;         return CELL_OK;
	case 10: *src_code = L10N_UTF8; *dst_code = L10N_ISO_8859_6;         return CELL_OK;
	case 11: *src_code = L10N_UTF8; *dst_code = L10N_ISO_8859_7;         return CELL_OK;
	case 12: *src_code = L10N_UTF8; *dst_code = L10N_ISO_8859_8;         return CELL_OK;
	case 13: *src_code = L10N_UTF8; *dst_code = L10N_ISO_8859_9;         return CELL_OK;
	case 14: *src_code = L10N_UTF8; *dst_code = L10N_ISO_8859_10;        return CELL_OK;
	case 15: *src_code = L10N_UTF8; *dst_code = L10N_ISO_8859_11;        return CELL_OK;
	case 16: *src_code = L10N_UTF8; *dst_code = L10N_ISO_8859_13;        return CELL_OK;
	case 17: *src_code = L10N_UTF8; *dst_code = L10N_ISO_8859_14;        return CELL_OK;
	case 18: *src_code = L10N_UTF8; *dst_code = L10N_ISO_8859_15;        return CELL_OK;
	case 19: *src_code = L10N_UTF8; *dst_code = L10N_ISO_8859_16;        return CELL_OK;
	case 20: *src_code = L10N_UTF8; *dst_code = L10N_CODEPAGE_437;       return CELL_OK;
	case 21: *src_code = L10N_UTF8; *dst_code = L10N_CODEPAGE_850;       return CELL_OK;
	case 22: *src_code = L10N_UTF8; *dst_code = L10N_CODEPAGE_863;       return CELL_OK;
	case 23: *src_code = L10N_UTF8; *dst_code = L10N_CODEPAGE_866;       return CELL_OK;
	case 24: *src_code = L10N_UTF8; *dst_code = L10N_CODEPAGE_932;       return CELL_OK;
	case 25: *src_code = L10N_UTF8; *dst_code = L10N_CODEPAGE_936;       return CELL_OK;
	case 26: *src_code = L10N_UTF8; *dst_code = L10N_CODEPAGE_949;       return CELL_OK;
	case 27: *src_code = L10N_UTF8; *dst_code = L10N_CODEPAGE_950;       return CELL_OK;
	case 28: *src_code = L10N_UTF8; *dst_code = L10N_CODEPAGE_1251;      return CELL_OK;
	case 29: *src_code = L10N_UTF8; *dst_code = L10N_CODEPAGE_1252;      return CELL_OK;
	case 30: *src_code = L10N_UTF8; *dst_code = L10N_EUC_CN;             return CELL_OK;
	case 31: *src_code = L10N_UTF8; *dst_code = L10N_EUC_JP;             return CELL_OK;
	case 32: *src_code = L10N_UTF8; *dst_code = L10N_EUC_KR;             return CELL_OK;
	case 33: *src_code = L10N_UTF8; *dst_code = L10N_ISO_2022_JP;        return CELL_OK;
	case 34: *src_code = L10N_UTF8; *dst_code = L10N_ARIB;               return CELL_OK;
	case 35: *src_code = L10N_UTF8; *dst_code = L10N_HZ;                 return CELL_OK;
	case 36: *src_code = L10N_UTF8; *dst_code = L10N_GB18030;            return CELL_OK;
	case 37: *src_code = L10N_UTF8; *dst_code = L10N_RIS_506;            return CELL_OK;
	case 38: *src_code = L10N_UTF8; *dst_code = L10N_CODEPAGE_852;       return CELL_OK;
	case 39: *src_code = L10N_UTF8; *dst_code = L10N_CODEPAGE_1250;      return CELL_OK;
	case 40: *src_code = L10N_UTF8; *dst_code = L10N_CODEPAGE_737;       return CELL_OK;
	case 41: *src_code = L10N_UTF8; *dst_code = L10N_CODEPAGE_1253;      return CELL_OK;
	case 42: *src_code = L10N_UTF8; *dst_code = L10N_CODEPAGE_857;       return CELL_OK;
	case 43: *src_code = L10N_UTF8; *dst_code = L10N_CODEPAGE_1254;      return CELL_OK;
	case 44: *src_code = L10N_UTF8; *dst_code = L10N_CODEPAGE_775;       return CELL_OK;
	case 45: *src_code = L10N_UTF8; *dst_code = L10N_CODEPAGE_1257;      return CELL_OK;
	case 46: *src_code = L10N_UTF8; *dst_code = L10N_CODEPAGE_855;       return CELL_OK;
	case 47: *src_code = L10N_UTF8; *dst_code = L10N_CODEPAGE_858;       return CELL_OK;
	case 48: *src_code = L10N_UTF8; *dst_code = L10N_CODEPAGE_860;       return CELL_OK;
	case 49: *src_code = L10N_UTF8; *dst_code = L10N_CODEPAGE_861;       return CELL_OK;
	case 50: *src_code = L10N_UTF8; *dst_code = L10N_CODEPAGE_865;       return CELL_OK;
	case 51: *src_code = L10N_UTF8; *dst_code = L10N_CODEPAGE_869;       return CELL_OK;
	case 65536: *src_code = L10N_UTF16; *dst_code = L10N_UTF8;           return CELL_OK;
	case 65538: *src_code = L10N_UTF16; *dst_code = L10N_UTF32;          return CELL_OK;
	case 65539: *src_code = L10N_UTF16; *dst_code = L10N_UCS2;           return CELL_OK;
	case 131072: *src_code = L10N_UTF32; *dst_code = L10N_UTF8;          return CELL_OK;
	case 131073: *src_code = L10N_UTF32; *dst_code = L10N_UTF32;         return CELL_OK;
	case 131075: *src_code = L10N_UTF32; *dst_code = L10N_UCS2;          return CELL_OK;
	case 196608: *src_code = L10N_UCS2; *dst_code = L10N_UTF8;           return CELL_OK;
	case 196609: *src_code = L10N_UCS2; *dst_code = L10N_UTF16;          return CELL_OK;
	case 196610: *src_code = L10N_UCS2; *dst_code = L10N_UTF32;          return CELL_OK;
	case 196613: *src_code = L10N_UCS2; *dst_code = L10N_ISO_8859_1;     return CELL_OK;
	case 196614: *src_code = L10N_UCS2; *dst_code = L10N_ISO_8859_2;     return CELL_OK;
	case 196615: *src_code = L10N_UCS2; *dst_code = L10N_ISO_8859_3;     return CELL_OK;
	case 196616: *src_code = L10N_UCS2; *dst_code = L10N_ISO_8859_4;     return CELL_OK;
	case 196617: *src_code = L10N_UCS2; *dst_code = L10N_ISO_8859_5;     return CELL_OK;
	case 196618: *src_code = L10N_UCS2; *dst_code = L10N_ISO_8859_6;     return CELL_OK;
	case 196619: *src_code = L10N_UCS2; *dst_code = L10N_ISO_8859_7;     return CELL_OK;
	case 196620: *src_code = L10N_UCS2; *dst_code = L10N_ISO_8859_8;     return CELL_OK;
	case 196621: *src_code = L10N_UCS2; *dst_code = L10N_ISO_8859_9;     return CELL_OK;
	case 196622: *src_code = L10N_UCS2; *dst_code = L10N_ISO_8859_10;    return CELL_OK;
	case 196623: *src_code = L10N_UCS2; *dst_code = L10N_ISO_8859_11;    return CELL_OK;
	case 196624: *src_code = L10N_UCS2; *dst_code = L10N_ISO_8859_13;    return CELL_OK;
	case 196625: *src_code = L10N_UCS2; *dst_code = L10N_ISO_8859_14;    return CELL_OK;
	case 196626: *src_code = L10N_UCS2; *dst_code = L10N_ISO_8859_15;    return CELL_OK;
	case 196627: *src_code = L10N_UCS2; *dst_code = L10N_ISO_8859_16;    return CELL_OK;
	case 196628: *src_code = L10N_UCS2; *dst_code = L10N_CODEPAGE_437;   return CELL_OK;
	case 196629: *src_code = L10N_UCS2; *dst_code = L10N_CODEPAGE_850;   return CELL_OK;
	case 196630: *src_code = L10N_UCS2; *dst_code = L10N_CODEPAGE_863;   return CELL_OK;
	case 196631: *src_code = L10N_UCS2; *dst_code = L10N_CODEPAGE_866;   return CELL_OK;
	case 196632: *src_code = L10N_UCS2; *dst_code = L10N_CODEPAGE_932;   return CELL_OK;
	case 196633: *src_code = L10N_UCS2; *dst_code = L10N_CODEPAGE_936;   return CELL_OK;
	case 196634: *src_code = L10N_UCS2; *dst_code = L10N_CODEPAGE_949;   return CELL_OK;
	case 196635: *src_code = L10N_UCS2; *dst_code = L10N_CODEPAGE_950;   return CELL_OK;
	case 196636: *src_code = L10N_UCS2; *dst_code = L10N_CODEPAGE_1251;  return CELL_OK;
	case 196637: *src_code = L10N_UCS2; *dst_code = L10N_CODEPAGE_1252;  return CELL_OK;
	case 196638: *src_code = L10N_UCS2; *dst_code = L10N_EUC_CN;         return CELL_OK;
	case 196639: *src_code = L10N_UCS2; *dst_code = L10N_EUC_JP;         return CELL_OK;
	case 196640: *src_code = L10N_UCS2; *dst_code = L10N_EUC_KR;         return CELL_OK;
	case 196641: *src_code = L10N_UCS2; *dst_code = L10N_ISO_2022_JP;    return CELL_OK;
	case 196642: *src_code = L10N_UCS2; *dst_code = L10N_ARIB;           return CELL_OK;
	case 196643: *src_code = L10N_UCS2; *dst_code = L10N_HZ;             return CELL_OK;
	case 196644: *src_code = L10N_UCS2; *dst_code = L10N_GB18030;        return CELL_OK;
	case 196645: *src_code = L10N_UCS2; *dst_code = L10N_RIS_506;        return CELL_OK;
	case 196646: *src_code = L10N_UCS2; *dst_code = L10N_CODEPAGE_852;   return CELL_OK;
	case 196647: *src_code = L10N_UCS2; *dst_code = L10N_CODEPAGE_1250;  return CELL_OK;
	case 196648: *src_code = L10N_UCS2; *dst_code = L10N_CODEPAGE_737;   return CELL_OK;
	case 196649: *src_code = L10N_UCS2; *dst_code = L10N_CODEPAGE_1253;  return CELL_OK;
	case 196650: *src_code = L10N_UCS2; *dst_code = L10N_CODEPAGE_857;   return CELL_OK;
	case 196651: *src_code = L10N_UCS2; *dst_code = L10N_CODEPAGE_1254;  return CELL_OK;
	case 196652: *src_code = L10N_UCS2; *dst_code = L10N_CODEPAGE_775;   return CELL_OK;
	case 196653: *src_code = L10N_UCS2; *dst_code = L10N_CODEPAGE_1257;  return CELL_OK;
	case 196654: *src_code = L10N_UCS2; *dst_code = L10N_CODEPAGE_855;   return CELL_OK;
	case 196655: *src_code = L10N_UCS2; *dst_code = L10N_CODEPAGE_858;   return CELL_OK;
	case 196656: *src_code = L10N_UCS2; *dst_code = L10N_CODEPAGE_860;   return CELL_OK;
	case 196657: *src_code = L10N_UCS2; *dst_code = L10N_CODEPAGE_861;   return CELL_OK;
	case 196658: *src_code = L10N_UCS2; *dst_code = L10N_CODEPAGE_865;   return CELL_OK;
	case 196659: *src_code = L10N_UCS2; *dst_code = L10N_CODEPAGE_869;   return CELL_OK;
	case 327680: *src_code = L10N_ISO_8859_1; *dst_code = L10N_UTF8;     return CELL_OK;
	case 327683: *src_code = L10N_ISO_8859_1; *dst_code = L10N_UCS2;     return CELL_OK;
	case 393216: *src_code = L10N_ISO_8859_2; *dst_code = L10N_UTF8;     return CELL_OK;
	case 393219: *src_code = L10N_ISO_8859_2; *dst_code = L10N_UCS2;     return CELL_OK;
	case 458752: *src_code = L10N_ISO_8859_3; *dst_code = L10N_UTF8;     return CELL_OK;
	case 458755: *src_code = L10N_ISO_8859_3; *dst_code = L10N_UCS2;     return CELL_OK;
	case 524288: *src_code = L10N_ISO_8859_4; *dst_code = L10N_UTF8;     return CELL_OK;
	case 524291: *src_code = L10N_ISO_8859_4; *dst_code = L10N_UCS2;     return CELL_OK;
	case 589824: *src_code = L10N_ISO_8859_5; *dst_code = L10N_UTF8;     return CELL_OK;
	case 589827: *src_code = L10N_ISO_8859_5; *dst_code = L10N_UCS2;     return CELL_OK;
	case 655360: *src_code = L10N_ISO_8859_6; *dst_code = L10N_UTF8;     return CELL_OK;
	case 655363: *src_code = L10N_ISO_8859_6; *dst_code = L10N_UCS2;     return CELL_OK;
	case 720896: *src_code = L10N_ISO_8859_7; *dst_code = L10N_UTF8;     return CELL_OK;
	case 720899: *src_code = L10N_ISO_8859_7; *dst_code = L10N_UCS2;     return CELL_OK;
	case 786432: *src_code = L10N_ISO_8859_8; *dst_code = L10N_UTF8;     return CELL_OK;
	case 786435: *src_code = L10N_ISO_8859_8; *dst_code = L10N_UCS2;     return CELL_OK;
	case 851968: *src_code = L10N_ISO_8859_9; *dst_code = L10N_UTF8;     return CELL_OK;
	case 851971: *src_code = L10N_ISO_8859_9; *dst_code = L10N_UCS2;     return CELL_OK;
	case 917504: *src_code = L10N_ISO_8859_10; *dst_code = L10N_UTF8;    return CELL_OK;
	case 917507: *src_code = L10N_ISO_8859_10; *dst_code = L10N_UCS2;    return CELL_OK;
	case 983040: *src_code = L10N_ISO_8859_11; *dst_code = L10N_UTF8;    return CELL_OK;
	case 983043: *src_code = L10N_ISO_8859_11; *dst_code = L10N_UCS2;    return CELL_OK;
	case 1048576: *src_code = L10N_ISO_8859_13; *dst_code = L10N_UTF8;   return CELL_OK;
	case 1048579: *src_code = L10N_ISO_8859_13; *dst_code = L10N_UCS2;   return CELL_OK;
	case 1114112: *src_code = L10N_ISO_8859_14; *dst_code = L10N_UTF8;   return CELL_OK;
	case 1114115: *src_code = L10N_ISO_8859_14; *dst_code = L10N_UCS2;   return CELL_OK;
	case 1179648: *src_code = L10N_ISO_8859_15; *dst_code = L10N_UTF8;   return CELL_OK;
	case 1179651: *src_code = L10N_ISO_8859_15; *dst_code = L10N_UCS2;   return CELL_OK;
	case 1245184: *src_code = L10N_ISO_8859_16; *dst_code = L10N_UTF8;   return CELL_OK;
	case 1245187: *src_code = L10N_ISO_8859_16; *dst_code = L10N_UCS2;   return CELL_OK;
	case 1310720: *src_code = L10N_CODEPAGE_437; *dst_code = L10N_UTF8;  return CELL_OK;
	case 1310723: *src_code = L10N_CODEPAGE_437; *dst_code = L10N_UCS2;  return CELL_OK;
	case 1376256: *src_code = L10N_CODEPAGE_850; *dst_code = L10N_UTF8;  return CELL_OK;
	case 1376259: *src_code = L10N_CODEPAGE_850; *dst_code = L10N_UCS2;  return CELL_OK;
	case 1441792: *src_code = L10N_CODEPAGE_863; *dst_code = L10N_UTF8;  return CELL_OK;
	case 1441795: *src_code = L10N_CODEPAGE_863; *dst_code = L10N_UCS2;  return CELL_OK;
	case 1507328: *src_code = L10N_CODEPAGE_866; *dst_code = L10N_UTF8;  return CELL_OK;
	case 1507331: *src_code = L10N_CODEPAGE_866; *dst_code = L10N_UCS2;  return CELL_OK;
	case 1572864: *src_code = L10N_CODEPAGE_932; *dst_code = L10N_UTF8;  return CELL_OK;
	case 1572867: *src_code = L10N_CODEPAGE_932; *dst_code = L10N_UCS2;  return CELL_OK;
	case 1638400: *src_code = L10N_CODEPAGE_936; *dst_code = L10N_UTF8;  return CELL_OK;
	case 1638403: *src_code = L10N_CODEPAGE_936; *dst_code = L10N_UCS2;  return CELL_OK;
	case 1703936: *src_code = L10N_CODEPAGE_949; *dst_code = L10N_UTF8;  return CELL_OK;
	case 1703939: *src_code = L10N_CODEPAGE_949; *dst_code = L10N_UCS2;  return CELL_OK;
	case 1769472: *src_code = L10N_CODEPAGE_950; *dst_code = L10N_UTF8;  return CELL_OK;
	case 1769475: *src_code = L10N_CODEPAGE_950; *dst_code = L10N_UCS2;  return CELL_OK;
	case 1835008: *src_code = L10N_CODEPAGE_1251; *dst_code = L10N_UTF8; return CELL_OK;
	case 1835011: *src_code = L10N_CODEPAGE_1251; *dst_code = L10N_UCS2; return CELL_OK;
	case 1900544: *src_code = L10N_CODEPAGE_1252; *dst_code = L10N_UTF8; return CELL_OK;
	case 1900547: *src_code = L10N_CODEPAGE_1252; *dst_code = L10N_UCS2; return CELL_OK;
	case 1966080: *src_code = L10N_EUC_CN; *dst_code = L10N_UTF8;        return CELL_OK;
	case 1966083: *src_code = L10N_EUC_CN; *dst_code = L10N_UCS2;        return CELL_OK;
	case 2031616: *src_code = L10N_EUC_JP; *dst_code = L10N_UTF8;        return CELL_OK;
	case 2031619: *src_code = L10N_EUC_JP; *dst_code = L10N_UCS2;        return CELL_OK;
	case 2097152: *src_code = L10N_EUC_KR; *dst_code = L10N_UTF8;        return CELL_OK;
	case 2097155: *src_code = L10N_EUC_KR; *dst_code = L10N_UCS2;        return CELL_OK;
	case 2162688: *src_code = L10N_ISO_2022_JP; *dst_code = L10N_UTF8;   return CELL_OK;
	case 2162691: *src_code = L10N_ISO_2022_JP; *dst_code = L10N_UCS2;   return CELL_OK;
	case 2228224: *src_code = L10N_ARIB; *dst_code = L10N_UTF8;          return CELL_OK;
	case 2228227: *src_code = L10N_ARIB; *dst_code = L10N_UCS2;          return CELL_OK;
	case 2293760: *src_code = L10N_HZ; *dst_code = L10N_UTF8;            return CELL_OK;
	case 2293763: *src_code = L10N_HZ; *dst_code = L10N_UCS2;            return CELL_OK;
	case 2359296: *src_code = L10N_GB18030; *dst_code = L10N_UTF8;       return CELL_OK;
	case 2359299: *src_code = L10N_GB18030; *dst_code = L10N_UCS2;       return CELL_OK;
	case 2424832: *src_code = L10N_RIS_506; *dst_code = L10N_UTF8;       return CELL_OK;
	case 2424835: *src_code = L10N_RIS_506; *dst_code = L10N_UCS2;       return CELL_OK;
	case 2490368: *src_code = L10N_CODEPAGE_852; *dst_code = L10N_UTF8;  return CELL_OK;
	case 2490371: *src_code = L10N_CODEPAGE_852; *dst_code = L10N_UCS2;  return CELL_OK;
	case 2555904: *src_code = L10N_CODEPAGE_1250; *dst_code = L10N_UTF8; return CELL_OK;
	case 2555907: *src_code = L10N_CODEPAGE_1250; *dst_code = L10N_UCS2; return CELL_OK;
	case 2621440: *src_code = L10N_CODEPAGE_737; *dst_code = L10N_UTF8;  return CELL_OK;
	case 2621443: *src_code = L10N_CODEPAGE_737; *dst_code = L10N_UCS2;  return CELL_OK;
	case 2686976: *src_code = L10N_CODEPAGE_1253; *dst_code = L10N_UTF8; return CELL_OK;
	case 2686979: *src_code = L10N_CODEPAGE_1253; *dst_code = L10N_UCS2; return CELL_OK;
	case 2752512: *src_code = L10N_CODEPAGE_857; *dst_code = L10N_UTF8;  return CELL_OK;
	case 2752515: *src_code = L10N_CODEPAGE_857; *dst_code = L10N_UCS2;  return CELL_OK;
	case 2818048: *src_code = L10N_CODEPAGE_1254; *dst_code = L10N_UTF8; return CELL_OK;
	case 2818051: *src_code = L10N_CODEPAGE_1254; *dst_code = L10N_UCS2; return CELL_OK;
	case 2883584: *src_code = L10N_CODEPAGE_775; *dst_code = L10N_UTF8;  return CELL_OK;
	case 2883587: *src_code = L10N_CODEPAGE_775; *dst_code = L10N_UCS2;  return CELL_OK;
	case 2949120: *src_code = L10N_CODEPAGE_1257; *dst_code = L10N_UTF8; return CELL_OK;
	case 2949123: *src_code = L10N_CODEPAGE_1257; *dst_code = L10N_UCS2; return CELL_OK;
	case 3014656: *src_code = L10N_CODEPAGE_855; *dst_code = L10N_UTF8;  return CELL_OK;
	case 3014659: *src_code = L10N_CODEPAGE_855; *dst_code = L10N_UCS2;  return CELL_OK;
	case 3080192: *src_code = L10N_CODEPAGE_858; *dst_code = L10N_UTF8;  return CELL_OK;
	case 3080195: *src_code = L10N_CODEPAGE_858; *dst_code = L10N_UCS2;  return CELL_OK;
	case 3145728: *src_code = L10N_CODEPAGE_860; *dst_code = L10N_UTF8;  return CELL_OK;
	case 3145731: *src_code = L10N_CODEPAGE_860; *dst_code = L10N_UCS2;  return CELL_OK;
	case 3211264: *src_code = L10N_CODEPAGE_861; *dst_code = L10N_UTF8;  return CELL_OK;
	case 3211267: *src_code = L10N_CODEPAGE_861; *dst_code = L10N_UCS2;  return CELL_OK;
	case 3276800: *src_code = L10N_CODEPAGE_865; *dst_code = L10N_UTF8;  return CELL_OK;
	case 3276803: *src_code = L10N_CODEPAGE_865; *dst_code = L10N_UCS2;  return CELL_OK;
	case 3342336: *src_code = L10N_CODEPAGE_869; *dst_code = L10N_UTF8;  return CELL_OK;
	case 3342339: *src_code = L10N_CODEPAGE_869; *dst_code = L10N_UCS2;  return CELL_OK;
	default: cellL10n.Error("GetConverterSrcAndDst(): Unknown/improper converter %d.", converter);
	}
	return -1;
}

// translate code id to code name. some codepage may has another name.
// If this makes your compilation fail, try replace the string code with one in "iconv -l"
bool _L10nCodeParse(s32 code, std::string& retCode)
{
	if ((code >= _L10N_CODE_) || (code < 0)) return false;
	switch (code)
	{
	// I don't know these Unicode Variants is LB or BE.
	case L10N_UTF8:         retCode = "UTF-8";          return true;
	case L10N_UTF16:        retCode = "UTF-16";         return true;
	case L10N_UTF32:        retCode = "UTF-32";         return true;
	case L10N_UCS2:         retCode = "UCS-2";          return true;
	case L10N_UCS4:         retCode = "UCS-4";          return true;
	case L10N_ISO_8859_1:   retCode = "ISO-8859-1";     return true;
	case L10N_ISO_8859_2:   retCode = "ISO-8859-2";     return true;
	case L10N_ISO_8859_3:   retCode = "ISO-8859-3";     return true;
	case L10N_ISO_8859_4:   retCode = "ISO-8859-4";     return true;
	case L10N_ISO_8859_5:   retCode = "ISO-8859-5";     return true;
	case L10N_ISO_8859_6:   retCode = "ISO-8859-6";     return true;
	case L10N_ISO_8859_7:   retCode = "ISO-8859-7";     return true;
	case L10N_ISO_8859_8:   retCode = "ISO-8859-8";     return true;
	case L10N_ISO_8859_9:   retCode = "ISO-8859-9";     return true;
	case L10N_ISO_8859_10:  retCode = "ISO-8859-10";    return true;
	case L10N_ISO_8859_11:  retCode = "ISO-8859-11";    return true;
	case L10N_ISO_8859_13:  retCode = "ISO-8859-13";    return true;    // No ISO-8859-12 ha ha.
	case L10N_ISO_8859_14:  retCode = "ISO-8859-14";    return true;
	case L10N_ISO_8859_15:  retCode = "ISO-8859-15";    return true;
	case L10N_ISO_8859_16:  retCode = "ISO-8859-16";    return true;
	case L10N_CODEPAGE_437: retCode = "CP437";          return true;
	case L10N_CODEPAGE_850: retCode = "CP850";          return true;
	case L10N_CODEPAGE_863: retCode = "CP863";          return true;
	case L10N_CODEPAGE_866: retCode = "CP866";          return true;
	case L10N_CODEPAGE_932: retCode = "CP932";          return true;
	case L10N_CODEPAGE_936: retCode = "CP936";          return true;
	case L10N_CODEPAGE_949: retCode = "CP949";          return true;
	case L10N_CODEPAGE_950: retCode = "CP950";          return true;
	case L10N_CODEPAGE_1251:retCode = "CP1251";         return true;    // CYRL
	case L10N_CODEPAGE_1252:retCode = "CP1252";         return true;    // ANSI
	case L10N_EUC_CN:       retCode = "EUC-CN";         return true;    // GB2312
	case L10N_EUC_JP:       retCode = "EUC-JP";         return true;
	case L10N_EUC_KR:       retCode = "EUC-KR";         return true;
	case L10N_ISO_2022_JP:  retCode = "ISO-2022-JP";    return true;
	case L10N_ARIB:         retCode = "ARABIC";         return true;    // TODO: think that should be ARABIC.
	case L10N_HZ:           retCode = "HZ";             return true;
	case L10N_GB18030:      retCode = "GB18030";        return true;
	case L10N_RIS_506:      retCode = "SHIFT-JIS";      return true;    // MusicShiftJIS, MS_KANJI
	//These are only supported with FW 3.10 and above
	case L10N_CODEPAGE_852: retCode = "CP852";          return true;
	case L10N_CODEPAGE_1250:retCode = "CP1250";         return true;    // EE
	case L10N_CODEPAGE_737: retCode = "CP737";          return true;
	case L10N_CODEPAGE_1253:retCode = "CP1253";         return true;    // Greek
	case L10N_CODEPAGE_857: retCode = "CP857";          return true;
	case L10N_CODEPAGE_1254:retCode = "CP1254";         return true;    // Turk
	case L10N_CODEPAGE_775: retCode = "CP775";          return true;
	case L10N_CODEPAGE_1257:retCode = "CP1257";         return true;    // WINBALTRIM
	case L10N_CODEPAGE_855: retCode = "CP855";          return true;
	case L10N_CODEPAGE_858: retCode = "CP858";          return true;
	case L10N_CODEPAGE_860: retCode = "CP860";          return true;
	case L10N_CODEPAGE_861: retCode = "CP861";          return true;
	case L10N_CODEPAGE_865: retCode = "CP865";          return true;
	case L10N_CODEPAGE_869: retCode = "CP869";          return true;
	default:                                            return false;
	}
}

// translate code id to code name.
// If this makes your compilation fail, try replace the string code with one in "iconv -l"
bool _L10nCodeParse(s32 code, u32& retCode)
{
	retCode = 0;
	if ((code >= _L10N_CODE_) || (code < 0)) return false;
	switch (code)
	{
	case L10N_UTF8:         retCode = 65001;        return false;
	case L10N_UTF16:        retCode = 1200;         return false;	// 1200=LE,1201=BE
	case L10N_UTF32:        retCode = 12000;        return false;	// 12000=LE,12001=BE
	case L10N_UCS2:         retCode = 1200;         return false;	// Not in OEM, but just the same as UTF16
	case L10N_UCS4:         retCode = 12000;        return false;	// Not in OEM, but just the same as UTF32
	// All OEM Code Pages are Multi-Byte, not wchar_t, u16, u32.
	case L10N_ISO_8859_1:   retCode = 28591;        return true;
	case L10N_ISO_8859_2:   retCode = 28592;        return true;
	case L10N_ISO_8859_3:   retCode = 28593;        return true;
	case L10N_ISO_8859_4:   retCode = 28594;        return true;
	case L10N_ISO_8859_5:   retCode = 28595;        return true;
	case L10N_ISO_8859_6:   retCode = 28596;        return true;
	case L10N_ISO_8859_7:   retCode = 28597;        return true;
	case L10N_ISO_8859_8:   retCode = 28598;        return true;
	case L10N_ISO_8859_9:   retCode = 28599;        return true;
	case L10N_ISO_8859_10:  retCode = 28600;        return true;
	case L10N_ISO_8859_11:  retCode = 28601;        return true;
	case L10N_ISO_8859_13:  retCode = 28603;        return true;    // No ISO-8859-12 ha ha.
	case L10N_ISO_8859_14:  retCode = 28604;        return true;
	case L10N_ISO_8859_15:  retCode = 28605;        return true;
	case L10N_ISO_8859_16:  retCode = 28606;        return true;
	case L10N_CODEPAGE_437: retCode = 437;          return true;
	case L10N_CODEPAGE_850: retCode = 850;          return true;
	case L10N_CODEPAGE_863: retCode = 863;          return true;
	case L10N_CODEPAGE_866: retCode = 866;          return true;
	case L10N_CODEPAGE_932: retCode = 932;          return true;
	case L10N_CODEPAGE_936: retCode = 936;          return true;
	case L10N_CODEPAGE_949: retCode = 949;          return true;
	case L10N_CODEPAGE_950: retCode = 950;          return true;
	case L10N_CODEPAGE_1251:retCode = 1251;         return true;    // CYRL
	case L10N_CODEPAGE_1252:retCode = 1252;         return true;    // ANSI
	case L10N_EUC_CN:       retCode = 51936;        return true;    // GB2312
	case L10N_EUC_JP:       retCode = 51932;        return true;
	case L10N_EUC_KR:       retCode = 51949;        return true;
	case L10N_ISO_2022_JP:  retCode = 50222;        return true;
	// Maybe 708/720/864/1256/10004/20420/28596/
	case L10N_ARIB:         retCode = 20420;        return true;    // TODO: think that should be ARABIC.
	case L10N_HZ:           retCode = 52936;        return true;
	case L10N_GB18030:      retCode = 54936;        return true;
	case L10N_RIS_506:      retCode = 932;          return true;    // MusicShiftJIS, MS_KANJI, TODO: Code page
	// These are only supported with FW 3.10 and above
	case L10N_CODEPAGE_852: retCode = 852;          return true;
	case L10N_CODEPAGE_1250:retCode = 1250;         return true;    // EE
	case L10N_CODEPAGE_737: retCode = 737;          return true;
	case L10N_CODEPAGE_1253:retCode = 1253;         return true;    // Greek
	case L10N_CODEPAGE_857: retCode = 857;          return true;
	case L10N_CODEPAGE_1254:retCode = 1254;         return true;    // Turk
	case L10N_CODEPAGE_775: retCode = 775;          return true;
	case L10N_CODEPAGE_1257:retCode = 1257;         return true;    // WINBALTRIM
	case L10N_CODEPAGE_855: retCode = 855;          return true;
	case L10N_CODEPAGE_858: retCode = 858;          return true;
	case L10N_CODEPAGE_860: retCode = 860;          return true;
	case L10N_CODEPAGE_861: retCode = 861;          return true;
	case L10N_CODEPAGE_865: retCode = 865;          return true;
	case L10N_CODEPAGE_869: retCode = 869;          return true;
	default:                                        return false;
	}
}

// TODO: check and complete transforms. note: unicode to/from other Unicode Formats is needed.
#ifdef _MSC_VER

// Use code page to transform std::string to std::wstring.
s32 _OEM2Wide(u32 oem_code, const std::string src, std::wstring& dst)
{
	// Such length returned should include the '\0' character.
	s32 length = MultiByteToWideChar(oem_code, 0, src.c_str(), -1, NULL, 0);
	wchar_t *store = new wchar_t[length]();

	MultiByteToWideChar(oem_code, 0, src.c_str(), -1, (LPWSTR)store, length);
	std::wstring result(store);
	dst = result;

	delete[] store;
	store = nullptr;

	return length - 1;
}

// Use Code page to transform std::wstring to std::string.
s32 _Wide2OEM(u32 oem_code, const std::wstring src, std::string& dst)
{
	// Such length returned should include the '\0' character.
	s32 length = WideCharToMultiByte(oem_code, 0, src.c_str(), -1, NULL, 0, NULL, NULL);
	char *store = new char[length]();

	WideCharToMultiByte(oem_code, 0, src.c_str(), -1, store, length, NULL, NULL);
	std::string result(store);
	dst = result;

	delete[] store;
	store = nullptr;

	return length - 1;
}

// Convert Codepage to Codepage (all char*)
std::string _OemToOem(u32 src_code, u32 dst_code, const std::string str)
{
	std::wstring wide; std::string result;
	_OEM2Wide(src_code, str, wide);
	_Wide2OEM(dst_code, wide, result);
	return result;
}

/*
// Original piece of code. and this is for windows using with _OEM2Wide,_Wide2OEM,_OemToOem.
// The Char -> Char Execution of this function has already been tested using VS and CJK text with encoding.
s32 _L10nConvertStr(s32 src_code, const void *src, size_t * src_len, s32 dst_code, void *dst, size_t * dst_len)
{
	u32 srcCode = 0, dstCode = 0;	//OEM code pages
	bool src_page_converted = _L10nCodeParse(src_code, srcCode);	//Check if code is in list.
	bool dst_page_converted = _L10nCodeParse(dst_code, dstCode);

	if (((!src_page_converted) && (srcCode == 0))
		|| ((!dst_page_converted) && (dstCode == 0)))
		return ConverterUnknown;

	if (strnlen_s((char*)src, *src_len) != *src_len) return SRCIllegal;
	//std::string wrapped_source = (char*)Memory.VirtualToRealAddr(src.addr());
	std::string wrapped_source((char*)src);
	//if (wrapped_source.length != src_len.GetValue()) return SRCIllegal;
	std::string target = _OemToOem(srcCode, dstCode, wrapped_source);

	if (target.length() > *dst_len) return DSTExhausted;

	Memory.WriteString(dst.addr(), target);

	return ConversionOK;
}
// This is the one used with iconv library for linux/mac. Also char->char.
// I've tested the code with console apps using codeblocks.
s32 _L10nConvertStr(s32 src_code, const void* src, size_t * src_len, s32 dst_code, void * dst, size_t * dst_len)
{
	std::string srcCode, dstCode;
	s32 retValue = ConversionOK;
	if ((_L10nCodeParse(src_code, srcCode)) && (_L10nCodeParse(dst_code, dstCode)))
	{
		iconv_t ict = iconv_open(srcCode.c_str(), dstCode.c_str());
		//char *srcBuf = (char*)Memory.VirtualToRealAddr(src.addr());
		//char *dstBuf = (char*)Memory.VirtualToRealAddr(dst.addr());
		char *srcBuf = (char*)src, *dstBuf = (char*)dst;
		size_t srcLen = *src_len, dstLen = *dst_len;
		size_t ictd = iconv(ict, &srcBuf, &srcLen, &dstBuf, &dstLen);
		if (ictd != *src_len)
		{
			if (errno == EILSEQ)
				retValue = SRCIllegal;  //Invalid multi-byte sequence
			else if (errno == E2BIG)
				retValue = DSTExhausted;//Not enough space
			else if (errno == EINVAL)
				retValue = SRCIllegal;
		}
		iconv_close(ict);
		//retValue = ConversionOK;
	}
	else retValue = ConverterUnknown;
	return retValue;
}*/
#endif

s32 UCS2toEUCJP()
{
	throw EXCEPTION("");
}

s32 l10n_convert()
{
	throw EXCEPTION("");
}

s32 UCS2toUTF32()
{
	throw EXCEPTION("");
}

s32 jis2kuten()
{
	throw EXCEPTION("");
}

s32 UTF8toGB18030()
{
	throw EXCEPTION("");
}

s32 JISstoUTF8s()
{
	throw EXCEPTION("");
}

s32 SjisZen2Han()
{
	throw EXCEPTION("");
}

s32 ToSjisLower()
{
	throw EXCEPTION("");
}

s32 UCS2toGB18030()
{
	throw EXCEPTION("");
}

s32 HZstoUCS2s()
{
	throw EXCEPTION("");
}

s32 UCS2stoHZs()
{
	throw EXCEPTION("");
}

s32 UCS2stoSJISs()
{
	throw EXCEPTION("");
}

s32 kuten2eucjp()
{
	throw EXCEPTION("");
}

s32 sjis2jis()
{
	throw EXCEPTION("");
}

s32 EUCKRstoUCS2s()
{
	throw EXCEPTION("");
}

s32 UHCstoEUCKRs()
{
	throw EXCEPTION("");
}

s32 jis2sjis()
{
	throw EXCEPTION("");
}

s32 jstrnchk()
{
	throw EXCEPTION("");
}

s32 L10nConvert()
{
	throw EXCEPTION("");
}

s32 EUCCNstoUTF8s()
{
	throw EXCEPTION("");
}

s32 GBKstoUCS2s()
{
	throw EXCEPTION("");
}

s32 eucjphan2zen()
{
	throw EXCEPTION("");
}

s32 ToSjisHira()
{
	throw EXCEPTION("");
}

s32 GBKtoUCS2()
{
	throw EXCEPTION("");
}

s32 eucjp2jis()
{
	throw EXCEPTION("");
}

s32 UTF32stoUTF8s()
{
	throw EXCEPTION("");
}

s32 sjishan2zen()
{
	throw EXCEPTION("");
}

s32 UCS2toSBCS()
{
	throw EXCEPTION("");
}

s32 UTF8stoGBKs()
{
	throw EXCEPTION("");
}

s32 UTF8toUCS2()
{
	throw EXCEPTION("");
}

s32 UCS2stoUTF8s()
{
	throw EXCEPTION("");
}

s32 EUCKRstoUTF8s()
{
	throw EXCEPTION("");
}

s32 UTF16stoUTF32s()
{
	throw EXCEPTION("");
}

s32 UTF8toEUCKR()
{
	throw EXCEPTION("");
}

s32 UTF16toUTF8()
{
	throw EXCEPTION("");
}

s32 ARIBstoUTF8s()
{
	throw EXCEPTION("");
}

s32 SJISstoUTF8s()
{
	throw EXCEPTION("");
}

s32 sjiszen2han()
{
	throw EXCEPTION("");
}

s32 ToEucJpLower()
{
	throw EXCEPTION("");
}

s32 MSJIStoUTF8()
{
	throw EXCEPTION("");
}

s32 UCS2stoMSJISs()
{
	throw EXCEPTION("");
}

s32 EUCJPtoUTF8()
{
	throw EXCEPTION("");
}

s32 eucjp2sjis()
{
	throw EXCEPTION("");
}

s32 ToEucJpHira()
{
	throw EXCEPTION("");
}

s32 UHCstoUCS2s()
{
	throw EXCEPTION("");
}

s32 ToEucJpKata()
{
	throw EXCEPTION("");
}

s32 HZstoUTF8s()
{
	throw EXCEPTION("");
}

s32 UTF8toMSJIS()
{
	throw EXCEPTION("");
}

s32 BIG5toUTF8()
{
	throw EXCEPTION("");
}

s32 EUCJPstoSJISs()
{
	throw EXCEPTION("");
}

s32 UTF8stoBIG5s()
{
	throw EXCEPTION("");
}

s32 UTF16stoUCS2s()
{
	throw EXCEPTION("");
}

s32 UCS2stoGB18030s()
{
	throw EXCEPTION("");
}

s32 EUCJPtoSJIS()
{
	throw EXCEPTION("");
}

s32 EUCJPtoUCS2()
{
	throw EXCEPTION("");
}

s32 UCS2stoGBKs()
{
	throw EXCEPTION("");
}

s32 EUCKRtoUHC()
{
	throw EXCEPTION("");
}

s32 UCS2toSJIS()
{
	throw EXCEPTION("");
}

s32 MSJISstoUTF8s()
{
	throw EXCEPTION("");
}

s32 EUCJPstoUTF8s()
{
	throw EXCEPTION("");
}

s32 UCS2toBIG5()
{
	throw EXCEPTION("");
}

s32 UTF8stoEUCKRs()
{
	throw EXCEPTION("");
}

s32 UHCstoUTF8s()
{
	throw EXCEPTION("");
}

s32 GB18030stoUCS2s()
{
	throw EXCEPTION("");
}

s32 SJIStoUTF8()
{
	throw EXCEPTION("");
}

s32 JISstoSJISs()
{
	throw EXCEPTION("");
}

s32 UTF8toUTF16()
{
	throw EXCEPTION("");
}

s32 UTF8stoMSJISs()
{
	throw EXCEPTION("");
}

s32 EUCKRtoUTF8()
{
	throw EXCEPTION("");
}

s32 SjisHan2Zen()
{
	throw EXCEPTION("");
}

s32 UCS2toUTF16()
{
	throw EXCEPTION("");
}

s32 UCS2toMSJIS()
{
	throw EXCEPTION("");
}

s32 sjis2kuten()
{
	throw EXCEPTION("");
}

s32 UCS2toUHC()
{
	throw EXCEPTION("");
}

s32 UTF32toUCS2()
{
	throw EXCEPTION("");
}

s32 ToSjisUpper()
{
	throw EXCEPTION("");
}

s32 UTF8toEUCJP()
{
	throw EXCEPTION("");
}

s32 UCS2stoEUCJPs()
{
	throw EXCEPTION("");
}

s32 UTF16toUCS2()
{
	throw EXCEPTION("");
}

s32 UCS2stoUTF16s()
{
	throw EXCEPTION("");
}

s32 UCS2stoEUCCNs()
{
	throw EXCEPTION("");
}

s32 SBCSstoUTF8s()
{
	throw EXCEPTION("");
}

s32 SJISstoJISs()
{
	throw EXCEPTION("");
}

s32 SBCStoUTF8()
{
	throw EXCEPTION("");
}

s32 UTF8toUTF32()
{
	throw EXCEPTION("");
}

s32 jstrchk(vm::cptr<char> jstr)
{
	cellL10n.Warning("jstrchk(jstr=*0x%x) -> utf8", jstr);

	return L10N_STR_UTF8;
}

s32 UHCtoEUCKR()
{
	throw EXCEPTION("");
}

s32 kuten2jis()
{
	throw EXCEPTION("");
}

s32 UTF8toEUCCN()
{
	throw EXCEPTION("");
}

s32 EUCCNtoUTF8()
{
	throw EXCEPTION("");
}

s32 EucJpZen2Han()
{
	throw EXCEPTION("");
}

s32 UTF32stoUTF16s()
{
	throw EXCEPTION("");
}

s32 GBKtoUTF8()
{
	throw EXCEPTION("");
}

s32 ToEucJpUpper()
{
	throw EXCEPTION("");
}

s32 UCS2stoJISs()
{
	throw EXCEPTION("");
}

s32 UTF8stoGB18030s()
{
	throw EXCEPTION("");
}

s32 EUCKRstoUHCs()
{
	throw EXCEPTION("");
}

s32 UTF8stoUTF32s()
{
	throw EXCEPTION("");
}

s32 UTF8stoEUCCNs()
{
	throw EXCEPTION("");
}

s32 EUCJPstoUCS2s()
{
	throw EXCEPTION("");
}

s32 UHCtoUCS2()
{
	throw EXCEPTION("");
}

// TODO: is this correct? Check it over at some point.
s32 L10nConvertStr(s32 src_code, vm::cptr<void> src, vm::ptr<u32> src_len, s32 dst_code, vm::ptr<void> dst, vm::ptr<u32> dst_len)
{
	cellL10n.Error("L10nConvertStr(src_code=%d, srca=*0x%x, src_len=*0x%x, dst_code=%d, dst=*0x%x, dst_len=*0x%x)", src_code, src, src_len, dst_code, dst, dst_len);
#ifdef _MSC_VER
	u32 srcCode = 0, dstCode = 0; // OEM code pages
	bool src_page_converted = _L10nCodeParse(src_code, srcCode); // Check if code is in list.
	bool dst_page_converted = _L10nCodeParse(dst_code, dstCode);

	if (((!src_page_converted) && (srcCode == 0))
		|| ((!dst_page_converted) && (dstCode == 0)))
		return ConverterUnknown;

	//if (strnlen_s((char*)src, *src_len) != *src_len) return SRCIllegal;
	std::string wrapped_source = (char*)src.get_ptr();
	//std::string wrapped_source((char*)src);
	if (wrapped_source.length() != *src_len) return SRCIllegal;
	std::string target = _OemToOem(srcCode, dstCode, wrapped_source);

	if (target.length() > *dst_len) return DSTExhausted;

	memcpy(dst.get_ptr(), target.c_str(), target.size());

	return ConversionOK;
#else
	std::string srcCode, dstCode;
	s32 retValue = ConversionOK;
	if ((_L10nCodeParse(src_code, srcCode)) && (_L10nCodeParse(dst_code, dstCode)))
	{
		iconv_t ict = iconv_open(srcCode.c_str(), dstCode.c_str());
		char *srcBuf = (char*)src.get_ptr();
		char *dstBuf = (char*)dst.get_ptr();
		//char *srcBuf = (char*)src, *dstBuf = (char*)dst;
		//size_t srcLen = *src_len, dstLen = *dst_len;
		size_t srcLen = *src_len, dstLen = *dst_len;
		size_t ictd = iconv(ict, &srcBuf, &srcLen, &dstBuf, &dstLen);
		if (ictd != *src_len)//if (ictd != *src_len)
		{
			if (errno == EILSEQ)
				retValue = SRCIllegal;  // Invalid multi-byte sequence
			else if (errno == E2BIG)
				retValue = DSTExhausted;// Not enough space
			else if (errno == EINVAL)
				retValue = SRCIllegal;
		}
		iconv_close(ict);
		//retValue = ConversionOK;
	}
	else retValue = ConverterUnknown;
	return retValue;
#endif
}

s32 GBKstoUTF8s()
{
	throw EXCEPTION("");
}

s32 UTF8toUHC()
{
	throw EXCEPTION("");
}

s32 UTF32toUTF8()
{
	throw EXCEPTION("");
}

s32 sjis2eucjp()
{
	throw EXCEPTION("");
}

s32 UCS2toEUCCN()
{
	throw EXCEPTION("");
}

s32 UTF8stoUHCs()
{
	throw EXCEPTION("");
}

s32 EUCKRtoUCS2()
{
	throw EXCEPTION("");
}

s32 UTF32toUTF16()
{
	throw EXCEPTION("");
}

s32 EUCCNstoUCS2s()
{
	throw EXCEPTION("");
}

s32 SBCSstoUCS2s()
{
	throw EXCEPTION("");
}

s32 UTF8stoJISs()
{
	throw EXCEPTION("");
}

s32 ToSjisKata()
{
	throw EXCEPTION("");
}

s32 jis2eucjp()
{
	throw EXCEPTION("");
}

s32 BIG5toUCS2()
{
	throw EXCEPTION("");
}

s32 UCS2toGBK()
{
	throw EXCEPTION("");
}

s32 UTF16toUTF32()
{
	throw EXCEPTION("");
}

s32 l10n_convert_str(s32 cd, vm::cptr<void> src, vm::ptr<u32> src_len, vm::ptr<void> dst, vm::ptr<u32> dst_len)
{
	cellL10n.Warning("l10n_convert_str(cd=%d, src=*0x%x, src_len=*0x%x, dst=*0x%x, dst_len=*0x%x)", cd, src, src_len, dst, dst_len);

	s32 src_code, dst_code;
	s32 ret = GetConverterSrcAndDst(cd, &src_code, &dst_code);

	if (ret != CELL_OK)
	{
		return ConverterUnknown;
	}

	return L10nConvertStr(src_code, src, src_len, dst_code, dst, dst_len);
}

s32 EUCJPstoJISs()
{
	throw EXCEPTION("");
}

s32 UTF8stoARIBs()
{
	throw EXCEPTION("");
}

s32 JISstoEUCJPs()
{
	throw EXCEPTION("");
}

s32 EucJpHan2Zen()
{
	throw EXCEPTION("");
}

s32 isEucJpKigou()
{
	throw EXCEPTION("");
}

s32 UCS2toUTF8()
{
	throw EXCEPTION("");
}

s32 GB18030toUCS2()
{
	throw EXCEPTION("");
}

s32 UHCtoUTF8()
{
	throw EXCEPTION("");
}

s32 MSJIStoUCS2()
{
	throw EXCEPTION("");
}

s32 UTF8toGBK()
{
	throw EXCEPTION("");
}

s32 kuten2sjis()
{
	throw EXCEPTION("");
}

s32 UTF8toSBCS()
{
	throw EXCEPTION("");
}

s32 SJIStoUCS2()
{
	throw EXCEPTION("");
}

s32 eucjpzen2han()
{
	throw EXCEPTION("");
}

s32 UCS2stoARIBs()
{
	throw EXCEPTION("");
}

s32 isSjisKigou()
{
	throw EXCEPTION("");
}

s32 UTF8stoEUCJPs()
{
	throw EXCEPTION("");
}

s32 UCS2toEUCKR()
{
	throw EXCEPTION("");
}

s32 SBCStoUCS2()
{
	throw EXCEPTION("");
}

s32 MSJISstoUCS2s()
{
	throw EXCEPTION("");
}

s32 l10n_get_converter(u32 src_code, u32 dst_code)
{
	cellL10n.Warning("l10n_get_converter(src_code=%d, dst_code=%d)", src_code, dst_code);
	return GetConverter(src_code, dst_code);
}

s32 GB18030stoUTF8s()
{
	throw EXCEPTION("");
}

s32 SJISstoEUCJPs()
{
	throw EXCEPTION("");
}

s32 UTF32stoUCS2s()
{
	throw EXCEPTION("");
}

s32 BIG5stoUTF8s()
{
	throw EXCEPTION("");
}

s32 EUCCNtoUCS2()
{
	throw EXCEPTION("");
}

s32 UTF8stoSBCSs()
{
	throw EXCEPTION("");
}

s32 UCS2stoEUCKRs()
{
	throw EXCEPTION("");
}

s32 UTF8stoSJISs()
{
	throw EXCEPTION("");
}

s32 UTF8stoHZs()
{
	throw EXCEPTION("");
}

s32 eucjp2kuten()
{
	throw EXCEPTION("");
}

s32 UTF8toBIG5()
{
	throw EXCEPTION("");
}

s32 UTF16stoUTF8s(vm::cptr<char16_t> utf16, vm::ref<u32> utf16_len, vm::ptr<char> utf8, vm::ref<u32> utf8_len)
{
	cellL10n.Error("UTF16stoUTF8s(utf16=*0x%x, utf16_len=*0x%x, utf8=*0x%x, utf8_len=*0x%x)", utf16, utf16_len, utf8, utf8_len);

	const u32 max_len = utf8_len; utf8_len = 0;

	for (u32 i = 0, len = 0; i < utf16_len; i++, utf8_len = len)
	{
		const char16_t ch = utf16[i];

		// increase required length (TODO)
		len = len + 1;

		// validate character (TODO)
		//if ()
		//{
		//	utf16_len -= i;
		//	return SRCIllegal;
		//}

		if (utf8 != vm::null)
		{
			if (len > max_len)
			{
				utf16_len -= i;
				return DSTExhausted;
			}

			if (ch <= 0x7f)
			{
				*utf8++ = static_cast<char>(ch);
			}
			else
			{
				*utf8++ = '?'; // TODO
			}
		}
	}

	return ConversionOK;
}

s32 JISstoUCS2s()
{
	throw EXCEPTION("");
}

s32 GB18030toUTF8()
{
	throw EXCEPTION("");
}

s32 UTF8toSJIS()
{
	throw EXCEPTION("");
}

s32 ARIBstoUCS2s()
{
	throw EXCEPTION("");
}

s32 UCS2stoUTF32s()
{
	throw EXCEPTION("");
}

s32 UCS2stoSBCSs()
{
	throw EXCEPTION("");
}

s32 UCS2stoBIG5s()
{
	throw EXCEPTION("");
}

s32 UCS2stoUHCs()
{
	throw EXCEPTION("");
}

s32 SJIStoEUCJP()
{
	throw EXCEPTION("");
}

s32 UTF8stoUTF16s()
{
	throw EXCEPTION("");
}

s32 SJISstoUCS2s()
{
	throw EXCEPTION("");
}

s32 BIG5stoUCS2s()
{
	throw EXCEPTION("");
}

s32 UTF8stoUCS2s()
{
	throw EXCEPTION("");
}


Module cellL10n("cellL10n", []()
{
	REG_FUNC(cellL10n, UCS2toEUCJP);
	REG_FUNC(cellL10n, l10n_convert);
	REG_FUNC(cellL10n, UCS2toUTF32);
	REG_FUNC(cellL10n, jis2kuten);
	REG_FUNC(cellL10n, UTF8toGB18030);
	REG_FUNC(cellL10n, JISstoUTF8s);
	REG_FUNC(cellL10n, SjisZen2Han);
	REG_FUNC(cellL10n, ToSjisLower);
	REG_FUNC(cellL10n, UCS2toGB18030);
	REG_FUNC(cellL10n, HZstoUCS2s);
	REG_FUNC(cellL10n, UCS2stoHZs);
	REG_FUNC(cellL10n, UCS2stoSJISs);
	REG_FUNC(cellL10n, kuten2eucjp);
	REG_FUNC(cellL10n, sjis2jis);
	REG_FUNC(cellL10n, EUCKRstoUCS2s);
	REG_FUNC(cellL10n, UHCstoEUCKRs);
	REG_FUNC(cellL10n, jis2sjis);
	REG_FUNC(cellL10n, jstrnchk);
	REG_FUNC(cellL10n, L10nConvert);
	REG_FUNC(cellL10n, EUCCNstoUTF8s);
	REG_FUNC(cellL10n, GBKstoUCS2s);
	REG_FUNC(cellL10n, eucjphan2zen);
	REG_FUNC(cellL10n, ToSjisHira);
	REG_FUNC(cellL10n, GBKtoUCS2);
	REG_FUNC(cellL10n, eucjp2jis);
	REG_FUNC(cellL10n, UTF32stoUTF8s);
	REG_FUNC(cellL10n, sjishan2zen);
	REG_FUNC(cellL10n, UCS2toSBCS);
	REG_FUNC(cellL10n, UTF8stoGBKs);
	REG_FUNC(cellL10n, UTF8toUCS2);
	REG_FUNC(cellL10n, UCS2stoUTF8s);
	REG_FUNC(cellL10n, EUCKRstoUTF8s);
	REG_FUNC(cellL10n, UTF16stoUTF32s);
	REG_FUNC(cellL10n, UTF8toEUCKR);
	REG_FUNC(cellL10n, UTF16toUTF8);
	REG_FUNC(cellL10n, ARIBstoUTF8s);
	REG_FUNC(cellL10n, SJISstoUTF8s);
	REG_FUNC(cellL10n, sjiszen2han);
	REG_FUNC(cellL10n, ToEucJpLower);
	REG_FUNC(cellL10n, MSJIStoUTF8);
	REG_FUNC(cellL10n, UCS2stoMSJISs);
	REG_FUNC(cellL10n, EUCJPtoUTF8);
	REG_FUNC(cellL10n, eucjp2sjis);
	REG_FUNC(cellL10n, ToEucJpHira);
	REG_FUNC(cellL10n, UHCstoUCS2s);
	REG_FUNC(cellL10n, ToEucJpKata);
	REG_FUNC(cellL10n, HZstoUTF8s);
	REG_FUNC(cellL10n, UTF8toMSJIS);
	REG_FUNC(cellL10n, BIG5toUTF8);
	REG_FUNC(cellL10n, EUCJPstoSJISs);
	REG_FUNC(cellL10n, UTF8stoBIG5s);
	REG_FUNC(cellL10n, UTF16stoUCS2s);
	REG_FUNC(cellL10n, UCS2stoGB18030s);
	REG_FUNC(cellL10n, EUCJPtoSJIS);
	REG_FUNC(cellL10n, EUCJPtoUCS2);
	REG_FUNC(cellL10n, UCS2stoGBKs);
	REG_FUNC(cellL10n, EUCKRtoUHC);
	REG_FUNC(cellL10n, UCS2toSJIS);
	REG_FUNC(cellL10n, MSJISstoUTF8s);
	REG_FUNC(cellL10n, EUCJPstoUTF8s);
	REG_FUNC(cellL10n, UCS2toBIG5);
	REG_FUNC(cellL10n, UTF8stoEUCKRs);
	REG_FUNC(cellL10n, UHCstoUTF8s);
	REG_FUNC(cellL10n, GB18030stoUCS2s);
	REG_FUNC(cellL10n, SJIStoUTF8);
	REG_FUNC(cellL10n, JISstoSJISs);
	REG_FUNC(cellL10n, UTF8toUTF16);
	REG_FUNC(cellL10n, UTF8stoMSJISs);
	REG_FUNC(cellL10n, EUCKRtoUTF8);
	REG_FUNC(cellL10n, SjisHan2Zen);
	REG_FUNC(cellL10n, UCS2toUTF16);
	REG_FUNC(cellL10n, UCS2toMSJIS);
	REG_FUNC(cellL10n, sjis2kuten);
	REG_FUNC(cellL10n, UCS2toUHC);
	REG_FUNC(cellL10n, UTF32toUCS2);
	REG_FUNC(cellL10n, ToSjisUpper);
	REG_FUNC(cellL10n, UTF8toEUCJP);
	REG_FUNC(cellL10n, UCS2stoEUCJPs);
	REG_FUNC(cellL10n, UTF16toUCS2);
	REG_FUNC(cellL10n, UCS2stoUTF16s);
	REG_FUNC(cellL10n, UCS2stoEUCCNs);
	REG_FUNC(cellL10n, SBCSstoUTF8s);
	REG_FUNC(cellL10n, SJISstoJISs);
	REG_FUNC(cellL10n, SBCStoUTF8);
	REG_FUNC(cellL10n, UTF8toUTF32);
	REG_FUNC(cellL10n, jstrchk);
	REG_FUNC(cellL10n, UHCtoEUCKR);
	REG_FUNC(cellL10n, kuten2jis);
	REG_FUNC(cellL10n, UTF8toEUCCN);
	REG_FUNC(cellL10n, EUCCNtoUTF8);
	REG_FUNC(cellL10n, EucJpZen2Han);
	REG_FUNC(cellL10n, UTF32stoUTF16s);
	REG_FUNC(cellL10n, GBKtoUTF8);
	REG_FUNC(cellL10n, ToEucJpUpper);
	REG_FUNC(cellL10n, UCS2stoJISs);
	REG_FUNC(cellL10n, UTF8stoGB18030s);
	REG_FUNC(cellL10n, EUCKRstoUHCs);
	REG_FUNC(cellL10n, UTF8stoUTF32s);
	REG_FUNC(cellL10n, UTF8stoEUCCNs);
	REG_FUNC(cellL10n, EUCJPstoUCS2s);
	REG_FUNC(cellL10n, UHCtoUCS2);
	REG_FUNC(cellL10n, L10nConvertStr);
	REG_FUNC(cellL10n, GBKstoUTF8s);
	REG_FUNC(cellL10n, UTF8toUHC);
	REG_FUNC(cellL10n, UTF32toUTF8);
	REG_FUNC(cellL10n, sjis2eucjp);
	REG_FUNC(cellL10n, UCS2toEUCCN);
	REG_FUNC(cellL10n, UTF8stoUHCs);
	REG_FUNC(cellL10n, EUCKRtoUCS2);
	REG_FUNC(cellL10n, UTF32toUTF16);
	REG_FUNC(cellL10n, EUCCNstoUCS2s);
	REG_FUNC(cellL10n, SBCSstoUCS2s);
	REG_FUNC(cellL10n, UTF8stoJISs);
	REG_FUNC(cellL10n, ToSjisKata);
	REG_FUNC(cellL10n, jis2eucjp);
	REG_FUNC(cellL10n, BIG5toUCS2);
	REG_FUNC(cellL10n, UCS2toGBK);
	REG_FUNC(cellL10n, UTF16toUTF32);
	REG_FUNC(cellL10n, l10n_convert_str);
	REG_FUNC(cellL10n, EUCJPstoJISs);
	REG_FUNC(cellL10n, UTF8stoARIBs);
	REG_FUNC(cellL10n, JISstoEUCJPs);
	REG_FUNC(cellL10n, EucJpHan2Zen);
	REG_FUNC(cellL10n, isEucJpKigou);
	REG_FUNC(cellL10n, UCS2toUTF8);
	REG_FUNC(cellL10n, GB18030toUCS2);
	REG_FUNC(cellL10n, UHCtoUTF8);
	REG_FUNC(cellL10n, MSJIStoUCS2);
	REG_FUNC(cellL10n, UTF8toGBK);
	REG_FUNC(cellL10n, kuten2sjis);
	REG_FUNC(cellL10n, UTF8toSBCS);
	REG_FUNC(cellL10n, SJIStoUCS2);
	REG_FUNC(cellL10n, eucjpzen2han);
	REG_FUNC(cellL10n, UCS2stoARIBs);
	REG_FUNC(cellL10n, isSjisKigou);
	REG_FUNC(cellL10n, UTF8stoEUCJPs);
	REG_FUNC(cellL10n, UCS2toEUCKR);
	REG_FUNC(cellL10n, SBCStoUCS2);
	REG_FUNC(cellL10n, MSJISstoUCS2s);
	REG_FUNC(cellL10n, l10n_get_converter);
	REG_FUNC(cellL10n, GB18030stoUTF8s);
	REG_FUNC(cellL10n, SJISstoEUCJPs);
	REG_FUNC(cellL10n, UTF32stoUCS2s);
	REG_FUNC(cellL10n, BIG5stoUTF8s);
	REG_FUNC(cellL10n, EUCCNtoUCS2);
	REG_FUNC(cellL10n, UTF8stoSBCSs);
	REG_FUNC(cellL10n, UCS2stoEUCKRs);
	REG_FUNC(cellL10n, UTF8stoSJISs);
	REG_FUNC(cellL10n, UTF8stoHZs);
	REG_FUNC(cellL10n, eucjp2kuten);
	REG_FUNC(cellL10n, UTF8toBIG5);
	REG_FUNC(cellL10n, UTF16stoUTF8s);
	REG_FUNC(cellL10n, JISstoUCS2s);
	REG_FUNC(cellL10n, GB18030toUTF8);
	REG_FUNC(cellL10n, UTF8toSJIS);
	REG_FUNC(cellL10n, ARIBstoUCS2s);
	REG_FUNC(cellL10n, UCS2stoUTF32s);
	REG_FUNC(cellL10n, UCS2stoSBCSs);
	REG_FUNC(cellL10n, UCS2stoBIG5s);
	REG_FUNC(cellL10n, UCS2stoUHCs);
	REG_FUNC(cellL10n, SJIStoEUCJP);
	REG_FUNC(cellL10n, UTF8stoUTF16s);
	REG_FUNC(cellL10n, SJISstoUCS2s);
	REG_FUNC(cellL10n, BIG5stoUCS2s);
	REG_FUNC(cellL10n, UTF8stoUCS2s);
});
