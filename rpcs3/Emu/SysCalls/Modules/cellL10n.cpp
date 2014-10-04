#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#include "cellL10n.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef _MSC_VER
#include <windows.h>
#include <wchar.h>
#include <codecvt>
#else
#include <iconv.h>
#endif

Module *cellL10n = nullptr;

int UTF16stoUTF8s(vm::lptrl<const char16_t> utf16, vm::ptr<be_t<u32>> utf16_len, vm::ptr<char> utf8, vm::ptr<be_t<u32>> utf8_len)
{
	cellL10n->Warning("UTF16stoUTF8s(utf16_addr=0x%x, utf16_len_addr=0x%x, utf8_addr=0x%x, utf8_len_addr=0x%x)",
		utf16.addr(), utf16_len.addr(), utf8.addr(), utf8_len.addr());

	std::u16string wstr = utf16.get_ptr(); // ???
	wstr.resize(*utf16_len); // TODO: Is this really the role of utf16_len in this function?
#ifdef _MSC_VER
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t> convert;
	std::string str = convert.to_bytes(wstr);

	if (*utf8_len < str.size())
	{
		*utf8_len = str.size();
		return DSTExhausted;
	}

	*utf8_len = str.size();
	memcpy(utf8.get_ptr(), str.c_str(), str.size());
#endif
	return ConversionOK;
}

int jstrchk(vm::ptr<const char> jstr)
{
	cellL10n->Warning("jstrchk(jstr_addr=0x%x) -> utf8", jstr.addr());

	return L10N_STR_UTF8;
}

//translate code id to code name. some codepage may has another name.
//If this makes your compilation fail, try replace the string code with one in "iconv -l"
bool _L10nCodeParse(int code, std::string& retCode)
{
	if ((code >= _L10N_CODE_) || (code < 0)) return false;
	switch (code)
	{
	//I don't know these Unicode Variants is LB or BE.
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
	case L10N_ISO_8859_13:  retCode = "ISO-8859-13";    return true;    //No ISO-8859-12 ha ha.
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
	case L10N_CODEPAGE_1251:retCode = "CP1251";         return true;    //CYRL
	case L10N_CODEPAGE_1252:retCode = "CP1252";         return true;    //ANSI
	case L10N_EUC_CN:       retCode = "EUC-CN";         return true;    //GB2312
	case L10N_EUC_JP:       retCode = "EUC-JP";         return true;
	case L10N_EUC_KR:       retCode = "EUC-KR";         return true;
	case L10N_ISO_2022_JP:  retCode = "ISO-2022-JP";    return true;
	case L10N_ARIB:         retCode = "ARABIC";         return true;    //TODO: think that should be ARABIC.
	case L10N_HZ:           retCode = "HZ";             return true;
	case L10N_GB18030:      retCode = "GB18030";        return true;
	case L10N_RIS_506:      retCode = "SHIFT-JIS";      return true;    //MusicShiftJIS, MS_KANJI
		//These are only supported with FW 3.10 and above
	case L10N_CODEPAGE_852: retCode = "CP852";          return true;
	case L10N_CODEPAGE_1250:retCode = "CP1250";         return true;    //EE
	case L10N_CODEPAGE_737: retCode = "CP737";          return true;
	case L10N_CODEPAGE_1253:retCode = "CP1253";         return true;    //Greek
	case L10N_CODEPAGE_857: retCode = "CP857";          return true;
	case L10N_CODEPAGE_1254:retCode = "CP1254";         return true;    //Turk
	case L10N_CODEPAGE_775: retCode = "CP775";          return true;
	case L10N_CODEPAGE_1257:retCode = "CP1257";         return true;    //WINBALTRIM
	case L10N_CODEPAGE_855: retCode = "CP855";          return true;
	case L10N_CODEPAGE_858: retCode = "CP858";          return true;
	case L10N_CODEPAGE_860: retCode = "CP860";          return true;
	case L10N_CODEPAGE_861: retCode = "CP861";          return true;
	case L10N_CODEPAGE_865: retCode = "CP865";          return true;
	case L10N_CODEPAGE_869: retCode = "CP869";          return true;
	default:                                            return false;
	}
}

//translate code id to code name.
//If this makes your compilation fail, try replace the string code with one in "iconv -l"
bool _L10nCodeParse(int code, unsigned int & retCode)
{
	retCode = 0;
	if ((code >= _L10N_CODE_) || (code < 0)) return false;
	switch (code)
	{
	case L10N_UTF8:         retCode = 65001;        return false;
	case L10N_UTF16:        retCode = 1200;         return false;	//1200=LE,1201=BE
	case L10N_UTF32:        retCode = 12000;        return false;	//12000=LE,12001=BE
	case L10N_UCS2:         retCode = 1200;         return false;	//Not in OEM, but just the same as UTF16
	case L10N_UCS4:         retCode = 12000;        return false;	//Not in OEM, but just the same as UTF32
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
	case L10N_ISO_8859_13:  retCode = 28603;        return true;    //No ISO-8859-12 ha ha.
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
	case L10N_CODEPAGE_1251:retCode = 1251;         return true;    //CYRL
	case L10N_CODEPAGE_1252:retCode = 1252;         return true;    //ANSI
	case L10N_EUC_CN:       retCode = 51936;        return true;    //GB2312
	case L10N_EUC_JP:       retCode = 51932;        return true;
	case L10N_EUC_KR:       retCode = 51949;        return true;
	case L10N_ISO_2022_JP:  retCode = 50222;        return true;
	//Maybe 708/720/864/1256/10004/20420/28596/
	case L10N_ARIB:         retCode = 20420;        return true;    //TODO: think that should be ARABIC.
	case L10N_HZ:           retCode = 52936;        return true;
	case L10N_GB18030:      retCode = 54936;        return true;
	case L10N_RIS_506:      retCode = 932;          return true;    //MusicShiftJIS, MS_KANJI, TODO: Code page
		//These are only supported with FW 3.10 and above
	case L10N_CODEPAGE_852: retCode = 852;          return true;
	case L10N_CODEPAGE_1250:retCode = 1250;         return true;    //EE
	case L10N_CODEPAGE_737: retCode = 737;          return true;
	case L10N_CODEPAGE_1253:retCode = 1253;         return true;    //Greek
	case L10N_CODEPAGE_857: retCode = 857;          return true;
	case L10N_CODEPAGE_1254:retCode = 1254;         return true;    //Turk
	case L10N_CODEPAGE_775: retCode = 775;          return true;
	case L10N_CODEPAGE_1257:retCode = 1257;         return true;    //WINBALTRIM
	case L10N_CODEPAGE_855: retCode = 855;          return true;
	case L10N_CODEPAGE_858: retCode = 858;          return true;
	case L10N_CODEPAGE_860: retCode = 860;          return true;
	case L10N_CODEPAGE_861: retCode = 861;          return true;
	case L10N_CODEPAGE_865: retCode = 865;          return true;
	case L10N_CODEPAGE_869: retCode = 869;          return true;
	default:                                        return false;
	}
}

//TODO: check and complete transforms. note: unicode to/from other Unicode Formats is needed.
#ifdef _MSC_VER

//Use code page to transform std::string to std::wstring.
int _OEM2Wide(unsigned int oem_code, const std::string src, std::wstring& dst)
{
	//Such length returned should include the '\0' character.
	int length = MultiByteToWideChar(oem_code, 0, src.c_str(), -1, NULL, 0);
	wchar_t *store = new wchar_t[length]();

	MultiByteToWideChar(oem_code, 0, src.c_str(), -1, (LPWSTR)store, length);
	std::wstring result(store);
	dst = result;

	delete[] store;
	store = nullptr;

	return length - 1;
}

//Use Code page to transform std::wstring to std::string.
int _Wide2OEM(unsigned int oem_code, const std::wstring src, std::string& dst)
{
	//Such length returned should include the '\0' character.
	int length = WideCharToMultiByte(oem_code, 0, src.c_str(), -1, NULL, 0, NULL, NULL);
	char *store = new char[length]();

	WideCharToMultiByte(oem_code, 0, src.c_str(), -1, store, length, NULL, NULL);
	std::string result(store);
	dst = result;

	delete[] store;
	store = nullptr;

	return length - 1;
}

//Convert Codepage to Codepage (all char*)
std::string _OemToOem(unsigned int src_code, unsigned int dst_code, const std::string str)
{
	std::wstring wide; std::string result;
	_OEM2Wide(src_code, str, wide);
	_Wide2OEM(dst_code, wide, result);
	return result;
}

/*
//Original piece of code. and this is for windows using with _OEM2Wide,_Wide2OEM,_OemToOem.
//The Char -> Char Execution of this function has already been tested using VS and CJK text with encoding.
int _L10nConvertStr(int src_code, const void *src, size_t * src_len, int dst_code, void *dst, size_t * dst_len)
{
	unsigned int srcCode = 0, dstCode = 0;	//OEM code pages
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
//This is the one used with iconv library for linux/mac. Also char->char.
//I've tested the code with console apps using codeblocks.
int _L10nConvertStr(int src_code, const void* src, size_t * src_len, int dst_code, void * dst, size_t * dst_len)
{
	std::string srcCode, dstCode;
	int retValue = ConversionOK;
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

//TODO: Check the code in emulation. If support for UTF8/UTF16/UTF32/UCS2/UCS4 should use wider chars.. awful.
int L10nConvertStr(int src_code, vm::ptr<const void> src, vm::ptr<be_t<u32>> src_len, int dst_code, vm::ptr<void> dst, vm::ptr<be_t<u32>> dst_len)
{
	cellL10n->Error("L10nConvertStr(src_code=%d, srca_addr=0x%x, src_len_addr=0x%x, dst_code=%d, dst_addr=0x%x, dst_len_addr=0x%x)",
		src_code, src.addr(), src_len.addr(), dst_code, dst.addr(), dst_len.addr());
	//cellL10n->Todo("L10nConvertStr: 1st char at dst: 0x%x", *((char*)src.get_ptr()));
#ifdef _MSC_VER
	unsigned int srcCode = 0, dstCode = 0;	//OEM code pages
	bool src_page_converted = _L10nCodeParse(src_code, srcCode);	//Check if code is in list.
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
	int retValue = ConversionOK;
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
#endif
}

void cellL10n_init(Module *pxThis)
{
	cellL10n = pxThis;

	// NOTE: I think this module should be LLE'd instead of implementing all its functions

	// cellL10n->AddFunc(0x005200e6, UCS2toEUCJP);
	// cellL10n->AddFunc(0x01b0cbf4, l10n_convert);
	// cellL10n->AddFunc(0x0356038c, UCS2toUTF32);
	// cellL10n->AddFunc(0x05028763, jis2kuten);
	// cellL10n->AddFunc(0x058addc8, UTF8toGB18030);
	// cellL10n->AddFunc(0x060ee3b2, JISstoUTF8s);
	// cellL10n->AddFunc(0x07168a83, SjisZen2Han);
	// cellL10n->AddFunc(0x0bc386c8, ToSjisLower);
	// cellL10n->AddFunc(0x0bedf77d, UCS2toGB18030);
	// cellL10n->AddFunc(0x0bf867e2, HZstoUCS2s);
	// cellL10n->AddFunc(0x0ce278fd, UCS2stoHZs);
	// cellL10n->AddFunc(0x0d90a48d, UCS2stoSJISs);
	// cellL10n->AddFunc(0x0f624540, kuten2eucjp);
	// cellL10n->AddFunc(0x14ee3649, sjis2jis);
	// cellL10n->AddFunc(0x14f504b8, EUCKRstoUCS2s);
	// cellL10n->AddFunc(0x16eaf5f1, UHCstoEUCKRs);
	// cellL10n->AddFunc(0x1758053c, jis2sjis);
	// cellL10n->AddFunc(0x1906ce6b, jstrnchk);
	// cellL10n->AddFunc(0x1ac0d23d, L10nConvert);
	// cellL10n->AddFunc(0x1ae2acee, EUCCNstoUTF8s);
	// cellL10n->AddFunc(0x1cb1138f, GBKstoUCS2s);
	// cellL10n->AddFunc(0x1da42d70, eucjphan2zen);
	// cellL10n->AddFunc(0x1ec712e0, ToSjisHira);
	// cellL10n->AddFunc(0x1fb50183, GBKtoUCS2);
	// cellL10n->AddFunc(0x21948c03, eucjp2jis);
	// cellL10n->AddFunc(0x21aa3045, UTF32stoUTF8s);
	// cellL10n->AddFunc(0x24fd32a9, sjishan2zen);
	// cellL10n->AddFunc(0x256b6861, UCS2toSBCS);
	// cellL10n->AddFunc(0x262a5ae2, UTF8stoGBKs);
	// cellL10n->AddFunc(0x28724522, UTF8toUCS2);
	// cellL10n->AddFunc(0x2ad091c6, UCS2stoUTF8s);
	// cellL10n->AddFunc(0x2b84030c, EUCKRstoUTF8s);
	// cellL10n->AddFunc(0x2efa7294, UTF16stoUTF32s);
	// cellL10n->AddFunc(0x2f9eb543, UTF8toEUCKR);
	// cellL10n->AddFunc(0x317ab7c2, UTF16toUTF8);
	// cellL10n->AddFunc(0x32689828, ARIBstoUTF8s);
	// cellL10n->AddFunc(0x33435818, SJISstoUTF8s);
	// cellL10n->AddFunc(0x33f8b35c, sjiszen2han);
	// cellL10n->AddFunc(0x3968f176, ToEucJpLower);
	// cellL10n->AddFunc(0x398a3dee, MSJIStoUTF8);
	// cellL10n->AddFunc(0x3a20bc34, UCS2stoMSJISs);
	// cellL10n->AddFunc(0x3dabd5a7, EUCJPtoUTF8);
	// cellL10n->AddFunc(0x3df65b64, eucjp2sjis);
	// cellL10n->AddFunc(0x408a622b, ToEucJpHira);
	// cellL10n->AddFunc(0x41b4a5ae, UHCstoUCS2s);
	// cellL10n->AddFunc(0x41ccf033, ToEucJpKata);
	// cellL10n->AddFunc(0x42838145, HZstoUTF8s);
	// cellL10n->AddFunc(0x4931b44e, UTF8toMSJIS);
	// cellL10n->AddFunc(0x4b3bbacb, BIG5toUTF8);
	// cellL10n->AddFunc(0x511d386b, EUCJPstoSJISs);
	// cellL10n->AddFunc(0x52b7883f, UTF8stoBIG5s);
	// cellL10n->AddFunc(0x53558b6b, UTF16stoUCS2s);
	// cellL10n->AddFunc(0x53764725, UCS2stoGB18030s);
	// cellL10n->AddFunc(0x53c71ac2, EUCJPtoSJIS);
	// cellL10n->AddFunc(0x54f59807, EUCJPtoUCS2);
	// cellL10n->AddFunc(0x55f6921c, UCS2stoGBKs);
	// cellL10n->AddFunc(0x58246762, EUCKRtoUHC);
	// cellL10n->AddFunc(0x596df41c, UCS2toSJIS);
	// cellL10n->AddFunc(0x5a4ab223, MSJISstoUTF8s);
	// cellL10n->AddFunc(0x5ac783dc, EUCJPstoUTF8s);
	// cellL10n->AddFunc(0x5b684dfb, UCS2toBIG5);
	// cellL10n->AddFunc(0x5cd29270, UTF8stoEUCKRs);
	// cellL10n->AddFunc(0x5e1d9330, UHCstoUTF8s);
	// cellL10n->AddFunc(0x60ffa0ec, GB18030stoUCS2s);
	// cellL10n->AddFunc(0x6122e000, SJIStoUTF8);
	// cellL10n->AddFunc(0x6169f205, JISstoSJISs);
	// cellL10n->AddFunc(0x61fb9442, UTF8toUTF16);
	// cellL10n->AddFunc(0x62b36bcf, UTF8stoMSJISs);
	// cellL10n->AddFunc(0x63219199, EUCKRtoUTF8);
	// cellL10n->AddFunc(0x638c2fc1, SjisHan2Zen);
	// cellL10n->AddFunc(0x64a10ec8, UCS2toUTF16);
	// cellL10n->AddFunc(0x65444204, UCS2toMSJIS);
	// cellL10n->AddFunc(0x6621a82c, sjis2kuten);
	// cellL10n->AddFunc(0x6a6f25d1, UCS2toUHC);
	// cellL10n->AddFunc(0x6c62d879, UTF32toUCS2);
	// cellL10n->AddFunc(0x6de4b508, ToSjisUpper);
	// cellL10n->AddFunc(0x6e0705c4, UTF8toEUCJP);
	// cellL10n->AddFunc(0x6e5906fd, UCS2stoEUCJPs);
	// cellL10n->AddFunc(0x6fc530b3, UTF16toUCS2);
	// cellL10n->AddFunc(0x714a9b4a, UCS2stoUTF16s);
	// cellL10n->AddFunc(0x71804d64, UCS2stoEUCCNs);
	// cellL10n->AddFunc(0x72632e53, SBCSstoUTF8s);
	// cellL10n->AddFunc(0x73f2cd21, SJISstoJISs);
	// cellL10n->AddFunc(0x74496718, SBCStoUTF8);
	// cellL10n->AddFunc(0x74871fe0, UTF8toUTF32);
	cellL10n->AddFunc(0x750c363d, jstrchk);
	// cellL10n->AddFunc(0x7c5bde1c, UHCtoEUCKR);
	// cellL10n->AddFunc(0x7c912bda, kuten2jis);
	// cellL10n->AddFunc(0x7d07a1c2, UTF8toEUCCN);
	// cellL10n->AddFunc(0x8171c1cc, EUCCNtoUTF8);
	// cellL10n->AddFunc(0x82d5ecdf, EucJpZen2Han);
	// cellL10n->AddFunc(0x8555fe15, UTF32stoUTF16s);
	// cellL10n->AddFunc(0x860fc741, GBKtoUTF8);
	// cellL10n->AddFunc(0x867f7b8b, ToEucJpUpper);
	// cellL10n->AddFunc(0x88f8340b, UCS2stoJISs);
	// cellL10n->AddFunc(0x89236c86, UTF8stoGB18030s);
	// cellL10n->AddFunc(0x8a56f148, EUCKRstoUHCs);
	// cellL10n->AddFunc(0x8ccdba38, UTF8stoUTF32s);
	// cellL10n->AddFunc(0x8f472054, UTF8stoEUCCNs);
	// cellL10n->AddFunc(0x90e9b5d2, EUCJPstoUCS2s);
	// cellL10n->AddFunc(0x91a99765, UHCtoUCS2);
	cellL10n->AddFunc(0x931ff25a, L10nConvertStr);
	// cellL10n->AddFunc(0x949bb14c, GBKstoUTF8s);
	// cellL10n->AddFunc(0x9557ac9b, UTF8toUHC);
	// cellL10n->AddFunc(0x9768b6d3, UTF32toUTF8);
	// cellL10n->AddFunc(0x9874020d, sjis2eucjp);
	// cellL10n->AddFunc(0x9a0e7d23, UCS2toEUCCN);
	// cellL10n->AddFunc(0x9a13d6b8, UTF8stoUHCs);
	// cellL10n->AddFunc(0x9a72059d, EUCKRtoUCS2);
	// cellL10n->AddFunc(0x9b1210c6, UTF32toUTF16);
	// cellL10n->AddFunc(0x9cd8135b, EUCCNstoUCS2s);
	// cellL10n->AddFunc(0x9ce52809, SBCSstoUCS2s);
	// cellL10n->AddFunc(0x9cf1ab77, UTF8stoJISs);
	// cellL10n->AddFunc(0x9d14dc46, ToSjisKata);
	// cellL10n->AddFunc(0x9dcde367, jis2eucjp);
	// cellL10n->AddFunc(0x9ec52258, BIG5toUCS2);
	// cellL10n->AddFunc(0xa0d463c0, UCS2toGBK);
	// cellL10n->AddFunc(0xa19fb9de, UTF16toUTF32);
	// cellL10n->AddFunc(0xa298cad2, l10n_convert_str);
	// cellL10n->AddFunc(0xa34fa0eb, EUCJPstoJISs);
	// cellL10n->AddFunc(0xa5146299, UTF8stoARIBs);
	// cellL10n->AddFunc(0xa609f3e9, JISstoEUCJPs);
	// cellL10n->AddFunc(0xa60ff5c9, EucJpHan2Zen);
	// cellL10n->AddFunc(0xa963619c, isEucJpKigou);
	// cellL10n->AddFunc(0xa9a76fb8, UCS2toUTF8);
	// cellL10n->AddFunc(0xaf18d499, GB18030toUCS2);
	// cellL10n->AddFunc(0xb3361be6, UHCtoUTF8);
	// cellL10n->AddFunc(0xb6e45343, MSJIStoUCS2);
	// cellL10n->AddFunc(0xb7cef4a6, UTF8toGBK);
	// cellL10n->AddFunc(0xb7e08f7a, kuten2sjis);
	// cellL10n->AddFunc(0xb9cf473d, UTF8toSBCS);
	// cellL10n->AddFunc(0xbdd44ee3, SJIStoUCS2);
	// cellL10n->AddFunc(0xbe42e661, eucjpzen2han);
	// cellL10n->AddFunc(0xbe8d5485, UCS2stoARIBs);
	// cellL10n->AddFunc(0xbefe3869, isSjisKigou);
	// cellL10n->AddFunc(0xc62b758d, UTF8stoEUCJPs);
	// cellL10n->AddFunc(0xc7bdcb4c, UCS2toEUCKR);
	// cellL10n->AddFunc(0xc944fa56, SBCStoUCS2);
	// cellL10n->AddFunc(0xc9b78f58, MSJISstoUCS2s);
	// cellL10n->AddFunc(0xcc1633cc, l10n_get_converter);
	// cellL10n->AddFunc(0xd02ef83d, GB18030stoUTF8s);
	// cellL10n->AddFunc(0xd8721e2c, SJISstoEUCJPs);
	// cellL10n->AddFunc(0xd8cb24cb, UTF32stoUCS2s);
	// cellL10n->AddFunc(0xd990858b, BIG5stoUTF8s);
	// cellL10n->AddFunc(0xd9fb1224, EUCCNtoUCS2);
	// cellL10n->AddFunc(0xda67b37f, UTF8stoSBCSs);
	// cellL10n->AddFunc(0xdc54886c, UCS2stoEUCKRs);
	// cellL10n->AddFunc(0xdd5ebdeb, UTF8stoSJISs);
	// cellL10n->AddFunc(0xdefa1c17, UTF8stoHZs);
	// cellL10n->AddFunc(0xe2eabb32, eucjp2kuten);
	// cellL10n->AddFunc(0xe6d9e234, UTF8toBIG5);
	// cellL10n->AddFunc(0xe6f5711b, UTF16stoUTF8s);
	// cellL10n->AddFunc(0xe956dc64, JISstoUCS2s);
	// cellL10n->AddFunc(0xeabc3d00, GB18030toUTF8);
	// cellL10n->AddFunc(0xeb3dc670, UTF8toSJIS);
	// cellL10n->AddFunc(0xeb41cc68, ARIBstoUCS2s);
	// cellL10n->AddFunc(0xeb685b83, UCS2stoUTF32s);
	// cellL10n->AddFunc(0xebae29c0, UCS2stoSBCSs);
	// cellL10n->AddFunc(0xee6c6a39, UCS2stoBIG5s);
	// cellL10n->AddFunc(0xf1dcfa71, UCS2stoUHCs);
	// cellL10n->AddFunc(0xf439728e, SJIStoEUCJP);
	// cellL10n->AddFunc(0xf7681b9a, UTF8stoUTF16s);
	// cellL10n->AddFunc(0xf9b1896d, SJISstoUCS2s);
	// cellL10n->AddFunc(0xfa4a675a, BIG5stoUCS2s);
	// cellL10n->AddFunc(0xfdbf6ac5, UTF8stoUCS2s);
}
