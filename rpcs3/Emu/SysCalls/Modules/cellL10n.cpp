#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef _MSC_VER
typedef int HostCode;
#else
#include <iconv.h>
typedef const char *HostCode;
#endif

#include "cellL10n.h"

extern Module<> cellL10n;

// Translate code id to code name. some codepage may has another name.
// If this makes your compilation fail, try replace the string code with one in "iconv -l"
bool _L10nCodeParse(s32 code, HostCode& retCode)
{
#ifdef _MSC_VER
	retCode = 0;
	if ((code >= _L10N_CODE_) || (code < 0)) return false;
	switch (code)
	{
	case L10N_UTF8:         retCode = 65001;        return false;
	case L10N_UTF16:        retCode = 1200;         return false; // 1200=LE,1201=BE
	case L10N_UTF32:        retCode = 12000;        return false; // 12000=LE,12001=BE
	case L10N_UCS2:         retCode = 1200;         return false; // Not in OEM, but just the same as UTF16
	case L10N_UCS4:         retCode = 12000;        return false; // Not in OEM, but just the same as UTF32
	//All OEM Code Pages are Multi-Byte, not wchar_t,u16,u32.
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
	case L10N_ISO_8859_13:  retCode = 28603;        return true; // No ISO-8859-12 ha ha.
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
	case L10N_CODEPAGE_1251:retCode = 1251;         return true; // CYRL
	case L10N_CODEPAGE_1252:retCode = 1252;         return true; // ANSI
	case L10N_EUC_CN:       retCode = 51936;        return true; // GB2312
	case L10N_EUC_JP:       retCode = 51932;        return true;
	case L10N_EUC_KR:       retCode = 51949;        return true;
	case L10N_ISO_2022_JP:  retCode = 50222;        return true;
	// Maybe 708/720/864/1256/10004/20420/28596/
	case L10N_ARIB:         retCode = 20420;        return true; // TODO: think that should be ARABIC.
	case L10N_HZ:           retCode = 52936;        return true;
	case L10N_GB18030:      retCode = 54936;        return true;
	case L10N_RIS_506:      retCode = 932;          return true; // MusicShiftJIS, MS_KANJI, TODO: Code page
	// These are only supported with FW 3.10 and above
	case L10N_CODEPAGE_852: retCode = 852;          return true;
	case L10N_CODEPAGE_1250:retCode = 1250;         return true; // EE
	case L10N_CODEPAGE_737: retCode = 737;          return true;
	case L10N_CODEPAGE_1253:retCode = 1253;         return true; // Greek
	case L10N_CODEPAGE_857: retCode = 857;          return true;
	case L10N_CODEPAGE_1254:retCode = 1254;         return true; // Turk
	case L10N_CODEPAGE_775: retCode = 775;          return true;
	case L10N_CODEPAGE_1257:retCode = 1257;         return true; // WINBALTRIM
	case L10N_CODEPAGE_855: retCode = 855;          return true;
	case L10N_CODEPAGE_858: retCode = 858;          return true;
	case L10N_CODEPAGE_860: retCode = 860;          return true;
	case L10N_CODEPAGE_861: retCode = 861;          return true;
	case L10N_CODEPAGE_865: retCode = 865;          return true;
	case L10N_CODEPAGE_869: retCode = 869;          return true;
	default:                                        return false;
	}
#else
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
	case L10N_ISO_8859_13:  retCode = "ISO-8859-13";    return true; // No ISO-8859-12 ha ha.
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
	case L10N_CODEPAGE_1251:retCode = "CP1251";         return true; // CYRL
	case L10N_CODEPAGE_1252:retCode = "CP1252";         return true; // ANSI
	case L10N_EUC_CN:       retCode = "EUC-CN";         return true; // GB2312
	case L10N_EUC_JP:       retCode = "EUC-JP";         return true;
	case L10N_EUC_KR:       retCode = "EUC-KR";         return true;
	case L10N_ISO_2022_JP:  retCode = "ISO-2022-JP";    return true;
	case L10N_ARIB:         retCode = "ARABIC";         return true; // TODO: think that should be ARABIC.
	case L10N_HZ:           retCode = "HZ";             return true;
	case L10N_GB18030:      retCode = "GB18030";        return true;
	case L10N_RIS_506:      retCode = "SHIFT-JIS";      return true; // MusicShiftJIS, MS_KANJI
	// These are only supported with FW 3.10 and above
	case L10N_CODEPAGE_852: retCode = "CP852";          return true;
	case L10N_CODEPAGE_1250:retCode = "CP1250";         return true; // EE
	case L10N_CODEPAGE_737: retCode = "CP737";          return true;
	case L10N_CODEPAGE_1253:retCode = "CP1253";         return true; // Greek
	case L10N_CODEPAGE_857: retCode = "CP857";          return true;
	case L10N_CODEPAGE_1254:retCode = "CP1254";         return true; // Turk
	case L10N_CODEPAGE_775: retCode = "CP775";          return true;
	case L10N_CODEPAGE_1257:retCode = "CP1257";         return true; // WINBALTRIM
	case L10N_CODEPAGE_855: retCode = "CP855";          return true;
	case L10N_CODEPAGE_858: retCode = "CP858";          return true;
	case L10N_CODEPAGE_860: retCode = "CP860";          return true;
	case L10N_CODEPAGE_861: retCode = "CP861";          return true;
	case L10N_CODEPAGE_865: retCode = "CP865";          return true;
	case L10N_CODEPAGE_869: retCode = "CP869";          return true;
	default:                                            return false;
	}
#endif
}

#ifdef _MSC_VER

// Use code page to transform std::string to std::wstring.
s32 _OEM2Wide(HostCode oem_code, const std::string src, std::wstring& dst)
{
	//Such length returned should include the '\0' character.
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
s32 _Wide2OEM(HostCode oem_code, const std::wstring src, std::string& dst)
{
	//Such length returned should include the '\0' character.
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
std::string _OemToOem(HostCode src_code, HostCode dst_code, const std::string str)
{
	std::wstring wide; std::string result;
	_OEM2Wide(src_code, str, wide);
	_Wide2OEM(dst_code, wide, result);
	return result;
}

#endif

s32 _ConvertStr(s32 src_code, const void *src, s32 src_len, s32 dst_code, void *dst, s32 *dst_len, bool allowIncomplete)
{
	HostCode srcCode = 0, dstCode = 0;	//OEM code pages
	bool src_page_converted = _L10nCodeParse(src_code, srcCode);	//Check if code is in list.
	bool dst_page_converted = _L10nCodeParse(dst_code, dstCode);

	if (((!src_page_converted) && (srcCode == 0))
		|| ((!dst_page_converted) && (dstCode == 0)))
		return ConverterUnknown;

#ifdef _MSC_VER
	std::string wrapped_source = std::string(static_cast<const char *>(src), src_len);
	std::string target = _OemToOem(srcCode, dstCode, wrapped_source);

	if (dst != NULL)
	{
		if (target.length() > *dst_len) return DSTExhausted;
		memcpy(dst, target.c_str(), target.length());
	}
	*dst_len = target.length();

	return ConversionOK;
#else
	s32 retValue = ConversionOK;
	iconv_t ict = iconv_open(dstCode, srcCode);
	size_t srcLen = src_len;
	if (dst != NULL)
	{
		size_t dstLen = *dst_len;
		size_t ictd = iconv(ict, (char **)&src, &srcLen, (char **)&dst, &dstLen);
		*dst_len -= dstLen;
		if (ictd == -1)
		{
			if (errno == EILSEQ)
				retValue = SRCIllegal;  //Invalid multi-byte sequence
			else if (errno == E2BIG)
				retValue = DSTExhausted;//Not enough space
			else if (errno == EINVAL)
			{
				if (allowIncomplete)
					*dst_len = -1;  // TODO: correct value?
				else
					retValue = SRCIllegal;
			}
		}
	}
	else
	{
		*dst_len = 0;
		char buf[16];
		while (srcLen > 0)
		{
			char *bufPtr = buf;
			size_t bufLeft = sizeof(buf);
			size_t ictd = iconv(ict, (char **)&src, &srcLen, (char **)&bufPtr, &bufLeft);
			*dst_len += sizeof(buf) - bufLeft;
			if (ictd == -1 && errno != E2BIG)
			{
				if (errno == EILSEQ)
					retValue = SRCIllegal;
				else if (errno == EINVAL)
				{
					if (allowIncomplete)
						*dst_len = -1;  // TODO: correct value?
					else
						retValue = SRCIllegal;
				}
				break;
			}
		}
	}
	iconv_close(ict);
	return retValue;
#endif
}

s32 _L10nConvertStr(s32 src_code, vm::cptr<void> src, vm::cptr<s32> src_len, s32 dst_code, vm::ptr<void> dst, vm::ptr<s32> dst_len)
{
	s32 dstLen = *dst_len;
	s32 result = _ConvertStr(src_code, src.get_ptr(), *src_len, dst_code, dst == vm::null ? NULL : dst.get_ptr(), &dstLen, false);
	*dst_len = dstLen;
	return result;
}

s32 _L10nConvertChar(s32 src_code, const void *src, s32 src_len, s32 dst_code, vm::ptr<void> dst, vm::ptr<s32> dst_len)
{
	s32 dstLen = 0x7FFFFFFF;
	s32 result = _ConvertStr(src_code, src, src_len, dst_code, dst.get_ptr(), &dstLen, true);
	*dst_len = dstLen;
	return result;
}

s32 _L10nConvertCharNoResult(s32 src_code, const void *src, s32 src_len, s32 dst_code, vm::ptr<void> dst)
{
	s32 dstLen = 0x7FFFFFFF;
	s32 result = _ConvertStr(src_code, src, src_len, dst_code, dst.get_ptr(), &dstLen, true);
	return dstLen;
}

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

s32 SJISstoUTF8s(vm::cptr<void> src, vm::cptr<s32> src_len, vm::ptr<void> dst, vm::ptr<s32> dst_len)
{
	cellL10n.warning("SJISstoUTF8s(src=*0x%x, src_len=*0x%x, dst=*0x%x, dst_len=*0x%x)", src, src_len, dst, dst_len);
	return _L10nConvertStr(L10N_CODEPAGE_932, src, src_len, L10N_UTF8, dst, dst_len);
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

s32 UCS2toSJIS(u16 ch, vm::ptr<void> dst)
{
	cellL10n.warning("UCS2toSJIS(ch=%d, dst=*0x%x)", ch, dst);
	return _L10nConvertCharNoResult(L10N_UTF8, &ch, sizeof(ch), L10N_CODEPAGE_932, dst);
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

s32 jstrchk(vm::cptr<void> jstr)
{
	cellL10n.warning("jstrchk(jstr=*0x%x) -> utf8", jstr);

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

s32 L10nConvertStr(s32 src_code, vm::cptr<void> src, vm::ptr<s32> src_len, s32 dst_code, vm::ptr<void> dst, vm::ptr<s32> dst_len)
{
	cellL10n.error("L10nConvertStr(src_code=%d, src=*0x%x, src_len=*0x%x, dst_code=%d, dst=*0x%x, dst_len=*0x%x)", src_code, src, src_len, dst_code, dst, dst_len);
	return _L10nConvertStr(src_code, src, src_len, dst_code, dst, dst_len);
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

s32 l10n_convert_str(s32 cd, vm::cptr<void> src, vm::ptr<s32> src_len, vm::ptr<void> dst, vm::ptr<s32> dst_len)
{
	cellL10n.warning("l10n_convert_str(cd=%d, src=*0x%x, src_len=*0x%x, dst=*0x%x, dst_len=*0x%x)", cd, src, src_len, dst, dst_len);

	s32 src_code = cd >> 16;
	s32 dst_code = cd & 0xffff;

	return _L10nConvertStr(src_code, src, src_len, dst_code, dst, dst_len);
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
	cellL10n.warning("l10n_get_converter(src_code=%d, dst_code=%d)", src_code, dst_code);
	return (src_code << 16) | dst_code;
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

s32 UTF16stoUTF8s(vm::cptr<u16> utf16, vm::ref<s32> utf16_len, vm::ptr<u8> utf8, vm::ref<s32> utf8_len)
{
	cellL10n.error("UTF16stoUTF8s(utf16=*0x%x, utf16_len=*0x%x, utf8=*0x%x, utf8_len=*0x%x)", utf16, utf16_len, utf8, utf8_len);

	const u32 max_len = utf8_len; utf8_len = 0;

	for (u32 i = 0, len = 0; i < utf16_len; i++, utf8_len = len)
	{
		const u16 ch = utf16[i];

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
				*utf8++ = static_cast<u8>(ch);
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

s32 UTF8toSJIS(u8 ch, vm::ptr<void> dst, vm::ptr<s32> dst_len)
{
	cellL10n.warning("UTF8toSJIS(ch=%d, dst=*0x%x, dst_len=*0x%x)", ch, dst, dst_len);
	return _L10nConvertChar(L10N_UTF8, &ch, sizeof(ch), L10N_CODEPAGE_932, dst, dst_len);
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

s32 SJISstoUCS2s(vm::cptr<void> src, vm::cptr<s32> src_len, vm::ptr<void> dst, vm::ptr<s32> dst_len)
{
	cellL10n.warning("SJISstoUCS2s(src=*0x%x, src_len=*0x%x, dst=*0x%x, dst_len=*0x%x)", src, src_len, dst, dst_len);
	return _L10nConvertStr(L10N_CODEPAGE_932, src, src_len, L10N_UCS2, dst, dst_len);
}

s32 BIG5stoUCS2s()
{
	throw EXCEPTION("");
}

s32 UTF8stoUCS2s()
{
	throw EXCEPTION("");
}


Module<> cellL10n("cellL10n", []()
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
