#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

// Requires GCC 4.10 apparently..
#ifdef _MSC_VER
#include <codecvt>
#endif

void cellL10n_init();
Module cellL10n(0x001e, cellL10n_init);

// L10nResult
enum
{
	ConversionOK,
	SRCIllegal,
	DSTExhausted,
	ConverterUnknown,
};

// detection result
enum
{
	L10N_STR_UNKNOWN = (1 << 0),
	L10N_STR_ASCII   = (1 << 1),
	L10N_STR_JIS     = (1 << 2),
	L10N_STR_EUCJP   = (1 << 3),
	L10N_STR_SJIS    = (1 << 4),
	L10N_STR_UTF8    = (1 << 5),
	L10N_STR_ILLEGAL = (1 << 16),
	L10N_STR_ERROR   = (1 << 17),
};

int UTF16stoUTF8s(mem16_ptr_t utf16, mem64_t utf16_len, mem8_ptr_t utf8, mem64_t utf8_len)
{
	cellL10n.Warning("UTF16stoUTF8s(utf16_addr=0x%x, utf16_len_addr=0x%x, utf8_addr=0x%x, utf8_len_addr=0x%x)",
		utf16.GetAddr(), utf16_len.GetAddr(), utf8.GetAddr(), utf8_len.GetAddr());

	if (!utf16.IsGood() || !utf16_len.IsGood() || !utf8_len.IsGood())
		return SRCIllegal;

	std::u16string wstr =(char16_t*)Memory.VirtualToRealAddr(utf16);
	wstr.resize(utf16_len.GetValue()); // TODO: Is this really the role of utf16_len in this function?
#ifdef _MSC_VER
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t> convert;
	std::string str = convert.to_bytes(wstr);

	if (!utf8.IsGood() || utf8_len.GetValue() < str.size())
	{
		utf8_len = str.size();
		return DSTExhausted;
	}

	utf8_len = str.size();
	Memory.WriteString(utf8, str.c_str());
#endif
	return ConversionOK;
}

int jstrchk(mem8_ptr_t jstr)
{
	if (!jstr.IsGood())
		cellL10n.Error("jstrchk(jstr_addr=0x%x): invalid address", jstr.GetAddr());
	else if (jstr[0])
		cellL10n.Log("jstrchk(jstr_addr=0x%x): utf-8: [%s]", jstr.GetAddr(), Memory.ReadString(jstr.GetAddr()).c_str());
	else
		cellL10n.Log("jstrchk(jstr_addr=0x%x): empty string", jstr.GetAddr());

	return L10N_STR_UTF8;
}

void cellL10n_init()
{
	// NOTE: I think this module should be LLE'd instead of implementing all its functions

	// cellL10n.AddFunc(0x005200e6, UCS2toEUCJP);
	// cellL10n.AddFunc(0x01b0cbf4, l10n_convert);
	// cellL10n.AddFunc(0x0356038c, UCS2toUTF32);
	// cellL10n.AddFunc(0x05028763, jis2kuten);
	// cellL10n.AddFunc(0x058addc8, UTF8toGB18030);
	// cellL10n.AddFunc(0x060ee3b2, JISstoUTF8s);
	// cellL10n.AddFunc(0x07168a83, SjisZen2Han);
	// cellL10n.AddFunc(0x0bc386c8, ToSjisLower);
	// cellL10n.AddFunc(0x0bedf77d, UCS2toGB18030);
	// cellL10n.AddFunc(0x0bf867e2, HZstoUCS2s);
	// cellL10n.AddFunc(0x0ce278fd, UCS2stoHZs);
	// cellL10n.AddFunc(0x0d90a48d, UCS2stoSJISs);
	// cellL10n.AddFunc(0x0f624540, kuten2eucjp);
	// cellL10n.AddFunc(0x14ee3649, sjis2jis);
	// cellL10n.AddFunc(0x14f504b8, EUCKRstoUCS2s);
	// cellL10n.AddFunc(0x16eaf5f1, UHCstoEUCKRs);
	// cellL10n.AddFunc(0x1758053c, jis2sjis);
	// cellL10n.AddFunc(0x1906ce6b, jstrnchk);
	// cellL10n.AddFunc(0x1ac0d23d, L10nConvert);
	// cellL10n.AddFunc(0x1ae2acee, EUCCNstoUTF8s);
	// cellL10n.AddFunc(0x1cb1138f, GBKstoUCS2s);
	// cellL10n.AddFunc(0x1da42d70, eucjphan2zen);
	// cellL10n.AddFunc(0x1ec712e0, ToSjisHira);
	// cellL10n.AddFunc(0x1fb50183, GBKtoUCS2);
	// cellL10n.AddFunc(0x21948c03, eucjp2jis);
	// cellL10n.AddFunc(0x21aa3045, UTF32stoUTF8s);
	// cellL10n.AddFunc(0x24fd32a9, sjishan2zen);
	// cellL10n.AddFunc(0x256b6861, UCS2toSBCS);
	// cellL10n.AddFunc(0x262a5ae2, UTF8stoGBKs);
	// cellL10n.AddFunc(0x28724522, UTF8toUCS2);
	// cellL10n.AddFunc(0x2ad091c6, UCS2stoUTF8s);
	// cellL10n.AddFunc(0x2b84030c, EUCKRstoUTF8s);
	// cellL10n.AddFunc(0x2efa7294, UTF16stoUTF32s);
	// cellL10n.AddFunc(0x2f9eb543, UTF8toEUCKR);
	// cellL10n.AddFunc(0x317ab7c2, UTF16toUTF8);
	// cellL10n.AddFunc(0x32689828, ARIBstoUTF8s);
	// cellL10n.AddFunc(0x33435818, SJISstoUTF8s);
	// cellL10n.AddFunc(0x33f8b35c, sjiszen2han);
	// cellL10n.AddFunc(0x3968f176, ToEucJpLower);
	// cellL10n.AddFunc(0x398a3dee, MSJIStoUTF8);
	// cellL10n.AddFunc(0x3a20bc34, UCS2stoMSJISs);
	// cellL10n.AddFunc(0x3dabd5a7, EUCJPtoUTF8);
	// cellL10n.AddFunc(0x3df65b64, eucjp2sjis);
	// cellL10n.AddFunc(0x408a622b, ToEucJpHira);
	// cellL10n.AddFunc(0x41b4a5ae, UHCstoUCS2s);
	// cellL10n.AddFunc(0x41ccf033, ToEucJpKata);
	// cellL10n.AddFunc(0x42838145, HZstoUTF8s);
	// cellL10n.AddFunc(0x4931b44e, UTF8toMSJIS);
	// cellL10n.AddFunc(0x4b3bbacb, BIG5toUTF8);
	// cellL10n.AddFunc(0x511d386b, EUCJPstoSJISs);
	// cellL10n.AddFunc(0x52b7883f, UTF8stoBIG5s);
	// cellL10n.AddFunc(0x53558b6b, UTF16stoUCS2s);
	// cellL10n.AddFunc(0x53764725, UCS2stoGB18030s);
	// cellL10n.AddFunc(0x53c71ac2, EUCJPtoSJIS);
	// cellL10n.AddFunc(0x54f59807, EUCJPtoUCS2);
	// cellL10n.AddFunc(0x55f6921c, UCS2stoGBKs);
	// cellL10n.AddFunc(0x58246762, EUCKRtoUHC);
	// cellL10n.AddFunc(0x596df41c, UCS2toSJIS);
	// cellL10n.AddFunc(0x5a4ab223, MSJISstoUTF8s);
	// cellL10n.AddFunc(0x5ac783dc, EUCJPstoUTF8s);
	// cellL10n.AddFunc(0x5b684dfb, UCS2toBIG5);
	// cellL10n.AddFunc(0x5cd29270, UTF8stoEUCKRs);
	// cellL10n.AddFunc(0x5e1d9330, UHCstoUTF8s);
	// cellL10n.AddFunc(0x60ffa0ec, GB18030stoUCS2s);
	// cellL10n.AddFunc(0x6122e000, SJIStoUTF8);
	// cellL10n.AddFunc(0x6169f205, JISstoSJISs);
	// cellL10n.AddFunc(0x61fb9442, UTF8toUTF16);
	// cellL10n.AddFunc(0x62b36bcf, UTF8stoMSJISs);
	// cellL10n.AddFunc(0x63219199, EUCKRtoUTF8);
	// cellL10n.AddFunc(0x638c2fc1, SjisHan2Zen);
	// cellL10n.AddFunc(0x64a10ec8, UCS2toUTF16);
	// cellL10n.AddFunc(0x65444204, UCS2toMSJIS);
	// cellL10n.AddFunc(0x6621a82c, sjis2kuten);
	// cellL10n.AddFunc(0x6a6f25d1, UCS2toUHC);
	// cellL10n.AddFunc(0x6c62d879, UTF32toUCS2);
	// cellL10n.AddFunc(0x6de4b508, ToSjisUpper);
	// cellL10n.AddFunc(0x6e0705c4, UTF8toEUCJP);
	// cellL10n.AddFunc(0x6e5906fd, UCS2stoEUCJPs);
	// cellL10n.AddFunc(0x6fc530b3, UTF16toUCS2);
	// cellL10n.AddFunc(0x714a9b4a, UCS2stoUTF16s);
	// cellL10n.AddFunc(0x71804d64, UCS2stoEUCCNs);
	// cellL10n.AddFunc(0x72632e53, SBCSstoUTF8s);
	// cellL10n.AddFunc(0x73f2cd21, SJISstoJISs);
	// cellL10n.AddFunc(0x74496718, SBCStoUTF8);
	// cellL10n.AddFunc(0x74871fe0, UTF8toUTF32);
	cellL10n.AddFunc(0x750c363d, jstrchk);
	// cellL10n.AddFunc(0x7c5bde1c, UHCtoEUCKR);
	// cellL10n.AddFunc(0x7c912bda, kuten2jis);
	// cellL10n.AddFunc(0x7d07a1c2, UTF8toEUCCN);
	// cellL10n.AddFunc(0x8171c1cc, EUCCNtoUTF8);
	// cellL10n.AddFunc(0x82d5ecdf, EucJpZen2Han);
	// cellL10n.AddFunc(0x8555fe15, UTF32stoUTF16s);
	// cellL10n.AddFunc(0x860fc741, GBKtoUTF8);
	// cellL10n.AddFunc(0x867f7b8b, ToEucJpUpper);
	// cellL10n.AddFunc(0x88f8340b, UCS2stoJISs);
	// cellL10n.AddFunc(0x89236c86, UTF8stoGB18030s);
	// cellL10n.AddFunc(0x8a56f148, EUCKRstoUHCs);
	// cellL10n.AddFunc(0x8ccdba38, UTF8stoUTF32s);
	// cellL10n.AddFunc(0x8f472054, UTF8stoEUCCNs);
	// cellL10n.AddFunc(0x90e9b5d2, EUCJPstoUCS2s);
	// cellL10n.AddFunc(0x91a99765, UHCtoUCS2);
	// cellL10n.AddFunc(0x931ff25a, L10nConvertStr);
	// cellL10n.AddFunc(0x949bb14c, GBKstoUTF8s);
	// cellL10n.AddFunc(0x9557ac9b, UTF8toUHC);
	// cellL10n.AddFunc(0x9768b6d3, UTF32toUTF8);
	// cellL10n.AddFunc(0x9874020d, sjis2eucjp);
	// cellL10n.AddFunc(0x9a0e7d23, UCS2toEUCCN);
	// cellL10n.AddFunc(0x9a13d6b8, UTF8stoUHCs);
	// cellL10n.AddFunc(0x9a72059d, EUCKRtoUCS2);
	// cellL10n.AddFunc(0x9b1210c6, UTF32toUTF16);
	// cellL10n.AddFunc(0x9cd8135b, EUCCNstoUCS2s);
	// cellL10n.AddFunc(0x9ce52809, SBCSstoUCS2s);
	// cellL10n.AddFunc(0x9cf1ab77, UTF8stoJISs);
	// cellL10n.AddFunc(0x9d14dc46, ToSjisKata);
	// cellL10n.AddFunc(0x9dcde367, jis2eucjp);
	// cellL10n.AddFunc(0x9ec52258, BIG5toUCS2);
	// cellL10n.AddFunc(0xa0d463c0, UCS2toGBK);
	// cellL10n.AddFunc(0xa19fb9de, UTF16toUTF32);
	// cellL10n.AddFunc(0xa298cad2, l10n_convert_str);
	// cellL10n.AddFunc(0xa34fa0eb, EUCJPstoJISs);
	// cellL10n.AddFunc(0xa5146299, UTF8stoARIBs);
	// cellL10n.AddFunc(0xa609f3e9, JISstoEUCJPs);
	// cellL10n.AddFunc(0xa60ff5c9, EucJpHan2Zen);
	// cellL10n.AddFunc(0xa963619c, isEucJpKigou);
	// cellL10n.AddFunc(0xa9a76fb8, UCS2toUTF8);
	// cellL10n.AddFunc(0xaf18d499, GB18030toUCS2);
	// cellL10n.AddFunc(0xb3361be6, UHCtoUTF8);
	// cellL10n.AddFunc(0xb6e45343, MSJIStoUCS2);
	// cellL10n.AddFunc(0xb7cef4a6, UTF8toGBK);
	// cellL10n.AddFunc(0xb7e08f7a, kuten2sjis);
	// cellL10n.AddFunc(0xb9cf473d, UTF8toSBCS);
	// cellL10n.AddFunc(0xbdd44ee3, SJIStoUCS2);
	// cellL10n.AddFunc(0xbe42e661, eucjpzen2han);
	// cellL10n.AddFunc(0xbe8d5485, UCS2stoARIBs);
	// cellL10n.AddFunc(0xbefe3869, isSjisKigou);
	// cellL10n.AddFunc(0xc62b758d, UTF8stoEUCJPs);
	// cellL10n.AddFunc(0xc7bdcb4c, UCS2toEUCKR);
	// cellL10n.AddFunc(0xc944fa56, SBCStoUCS2);
	// cellL10n.AddFunc(0xc9b78f58, MSJISstoUCS2s);
	// cellL10n.AddFunc(0xcc1633cc, l10n_get_converter);
	// cellL10n.AddFunc(0xd02ef83d, GB18030stoUTF8s);
	// cellL10n.AddFunc(0xd8721e2c, SJISstoEUCJPs);
	// cellL10n.AddFunc(0xd8cb24cb, UTF32stoUCS2s);
	// cellL10n.AddFunc(0xd990858b, BIG5stoUTF8s);
	// cellL10n.AddFunc(0xd9fb1224, EUCCNtoUCS2);
	// cellL10n.AddFunc(0xda67b37f, UTF8stoSBCSs);
	// cellL10n.AddFunc(0xdc54886c, UCS2stoEUCKRs);
	// cellL10n.AddFunc(0xdd5ebdeb, UTF8stoSJISs);
	// cellL10n.AddFunc(0xdefa1c17, UTF8stoHZs);
	// cellL10n.AddFunc(0xe2eabb32, eucjp2kuten);
	// cellL10n.AddFunc(0xe6d9e234, UTF8toBIG5);
	cellL10n.AddFunc(0xe6f5711b, UTF16stoUTF8s);
	// cellL10n.AddFunc(0xe956dc64, JISstoUCS2s);
	// cellL10n.AddFunc(0xeabc3d00, GB18030toUTF8);
	// cellL10n.AddFunc(0xeb3dc670, UTF8toSJIS);
	// cellL10n.AddFunc(0xeb41cc68, ARIBstoUCS2s);
	// cellL10n.AddFunc(0xeb685b83, UCS2stoUTF32s);
	// cellL10n.AddFunc(0xebae29c0, UCS2stoSBCSs);
	// cellL10n.AddFunc(0xee6c6a39, UCS2stoBIG5s);
	// cellL10n.AddFunc(0xf1dcfa71, UCS2stoUHCs);
	// cellL10n.AddFunc(0xf439728e, SJIStoEUCJP);
	// cellL10n.AddFunc(0xf7681b9a, UTF8stoUTF16s);
	// cellL10n.AddFunc(0xf9b1896d, SJISstoUCS2s);
	// cellL10n.AddFunc(0xfa4a675a, BIG5stoUCS2s);
	// cellL10n.AddFunc(0xfdbf6ac5, UTF8stoUCS2s);
}
