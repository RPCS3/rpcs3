#pragma once

namespace vm { using namespace ps3; }

std::map<s32, std::pair<s32, s32>> converters;

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

// CodePages
enum {
	L10N_UTF8 = 0,
	L10N_UTF16,
	L10N_UTF32,
	L10N_UCS2,
	L10N_UCS4,
	L10N_ISO_8859_1,
	L10N_ISO_8859_2,
	L10N_ISO_8859_3,
	L10N_ISO_8859_4,
	L10N_ISO_8859_5,
	L10N_ISO_8859_6,
	L10N_ISO_8859_7,
	L10N_ISO_8859_8,
	L10N_ISO_8859_9,
	L10N_ISO_8859_10,
	L10N_ISO_8859_11,
	L10N_ISO_8859_13,
	L10N_ISO_8859_14,
	L10N_ISO_8859_15,
	L10N_ISO_8859_16,
	L10N_CODEPAGE_437,
	L10N_CODEPAGE_850,
	L10N_CODEPAGE_863,
	L10N_CODEPAGE_866,
	L10N_CODEPAGE_932,
	L10N_CODEPAGE_936,
	L10N_CODEPAGE_949,
	L10N_CODEPAGE_950,
	L10N_CODEPAGE_1251,
	L10N_CODEPAGE_1252,
	L10N_EUC_CN,
	L10N_EUC_JP,
	L10N_EUC_KR,
	L10N_ISO_2022_JP,
	L10N_ARIB,
	L10N_HZ,
	L10N_GB18030,
	L10N_RIS_506,
	//FW 3.10 and above
	L10N_CODEPAGE_852,
	L10N_CODEPAGE_1250,
	L10N_CODEPAGE_737,
	L10N_CODEPAGE_1253,
	L10N_CODEPAGE_857,
	L10N_CODEPAGE_1254,
	L10N_CODEPAGE_775,
	L10N_CODEPAGE_1257,
	L10N_CODEPAGE_855,
	L10N_CODEPAGE_858,
	L10N_CODEPAGE_860,
	L10N_CODEPAGE_861,
	L10N_CODEPAGE_865,
	L10N_CODEPAGE_869,
	_L10N_CODE_
};

static void insertConverters()
{
	converters.clear();
	converters[1] = std::make_pair(L10N_UTF8, L10N_UTF16);
	converters[2] = std::make_pair(L10N_UTF8, L10N_UTF32);
	converters[3] = std::make_pair(L10N_UTF8, L10N_UCS2);
	converters[5] = std::make_pair(L10N_UTF8, L10N_ISO_8859_1);
	converters[6] = std::make_pair(L10N_UTF8, L10N_ISO_8859_2);
	converters[7] = std::make_pair(L10N_UTF8, L10N_ISO_8859_3);
	converters[8] = std::make_pair(L10N_UTF8, L10N_ISO_8859_4);
	converters[9] = std::make_pair(L10N_UTF8, L10N_ISO_8859_5);
	converters[10] = std::make_pair(L10N_UTF8, L10N_ISO_8859_6);
	converters[11] = std::make_pair(L10N_UTF8, L10N_ISO_8859_7);
	converters[12] = std::make_pair(L10N_UTF8, L10N_ISO_8859_8);
	converters[13] = std::make_pair(L10N_UTF8, L10N_ISO_8859_9);
	converters[14] = std::make_pair(L10N_UTF8, L10N_ISO_8859_10);
	converters[15] = std::make_pair(L10N_UTF8, L10N_ISO_8859_11);
	converters[16] = std::make_pair(L10N_UTF8, L10N_ISO_8859_13);
	converters[17] = std::make_pair(L10N_UTF8, L10N_ISO_8859_14);
	converters[18] = std::make_pair(L10N_UTF8, L10N_ISO_8859_15);
	converters[19] = std::make_pair(L10N_UTF8, L10N_ISO_8859_16);
	converters[20] = std::make_pair(L10N_UTF8, L10N_CODEPAGE_437);
	converters[21] = std::make_pair(L10N_UTF8, L10N_CODEPAGE_850);
	converters[22] = std::make_pair(L10N_UTF8, L10N_CODEPAGE_863);
	converters[23] = std::make_pair(L10N_UTF8, L10N_CODEPAGE_866);
	converters[24] = std::make_pair(L10N_UTF8, L10N_CODEPAGE_932);
	converters[25] = std::make_pair(L10N_UTF8, L10N_CODEPAGE_936);
	converters[26] = std::make_pair(L10N_UTF8, L10N_CODEPAGE_949);
	converters[27] = std::make_pair(L10N_UTF8, L10N_CODEPAGE_950);
	converters[28] = std::make_pair(L10N_UTF8, L10N_CODEPAGE_1251);
	converters[29] = std::make_pair(L10N_UTF8, L10N_CODEPAGE_1252);
	converters[30] = std::make_pair(L10N_UTF8, L10N_EUC_CN);
	converters[31] = std::make_pair(L10N_UTF8, L10N_EUC_JP);
	converters[32] = std::make_pair(L10N_UTF8, L10N_EUC_KR);
	converters[33] = std::make_pair(L10N_UTF8, L10N_ISO_2022_JP);
	converters[34] = std::make_pair(L10N_UTF8, L10N_ARIB);
	converters[35] = std::make_pair(L10N_UTF8, L10N_HZ);
	converters[36] = std::make_pair(L10N_UTF8, L10N_GB18030);
	converters[37] = std::make_pair(L10N_UTF8, L10N_RIS_506);
	converters[38] = std::make_pair(L10N_UTF8, L10N_CODEPAGE_852);
	converters[39] = std::make_pair(L10N_UTF8, L10N_CODEPAGE_1250);
	converters[40] = std::make_pair(L10N_UTF8, L10N_CODEPAGE_737);
	converters[41] = std::make_pair(L10N_UTF8, L10N_CODEPAGE_1253);
	converters[42] = std::make_pair(L10N_UTF8, L10N_CODEPAGE_857);
	converters[43] = std::make_pair(L10N_UTF8, L10N_CODEPAGE_1254);
	converters[44] = std::make_pair(L10N_UTF8, L10N_CODEPAGE_775);
	converters[45] = std::make_pair(L10N_UTF8, L10N_CODEPAGE_1257);
	converters[46] = std::make_pair(L10N_UTF8, L10N_CODEPAGE_855);
	converters[47] = std::make_pair(L10N_UTF8, L10N_CODEPAGE_858);
	converters[48] = std::make_pair(L10N_UTF8, L10N_CODEPAGE_860);
	converters[49] = std::make_pair(L10N_UTF8, L10N_CODEPAGE_861);
	converters[50] = std::make_pair(L10N_UTF8, L10N_CODEPAGE_865);
	converters[51] = std::make_pair(L10N_UTF8, L10N_CODEPAGE_869);
	converters[65536] = std::make_pair(L10N_UTF16, L10N_UTF8);
	converters[65538] = std::make_pair(L10N_UTF16, L10N_UTF32);
	converters[65539] = std::make_pair(L10N_UTF16, L10N_UCS2);
	converters[131072] = std::make_pair(L10N_UTF32, L10N_UTF8);
	converters[131073] = std::make_pair(L10N_UTF32, L10N_UTF32);
	converters[131075] = std::make_pair(L10N_UTF32, L10N_UCS2);
	converters[196608] = std::make_pair(L10N_UCS2, L10N_UTF8);
	converters[196609] = std::make_pair(L10N_UCS2, L10N_UTF16);
	converters[196610] = std::make_pair(L10N_UCS2, L10N_UTF32);
	converters[196613] = std::make_pair(L10N_UCS2, L10N_ISO_8859_1);
	converters[196614] = std::make_pair(L10N_UCS2, L10N_ISO_8859_2);
	converters[196615] = std::make_pair(L10N_UCS2, L10N_ISO_8859_3);
	converters[196616] = std::make_pair(L10N_UCS2, L10N_ISO_8859_4);
	converters[196617] = std::make_pair(L10N_UCS2, L10N_ISO_8859_5);
	converters[196618] = std::make_pair(L10N_UCS2, L10N_ISO_8859_6);
	converters[196619] = std::make_pair(L10N_UCS2, L10N_ISO_8859_7);
	converters[196620] = std::make_pair(L10N_UCS2, L10N_ISO_8859_8);
	converters[196621] = std::make_pair(L10N_UCS2, L10N_ISO_8859_9);
	converters[196622] = std::make_pair(L10N_UCS2, L10N_ISO_8859_10);
	converters[196623] = std::make_pair(L10N_UCS2, L10N_ISO_8859_11);
	converters[196624] = std::make_pair(L10N_UCS2, L10N_ISO_8859_13);
	converters[196625] = std::make_pair(L10N_UCS2, L10N_ISO_8859_14);
	converters[196626] = std::make_pair(L10N_UCS2, L10N_ISO_8859_15);
	converters[196627] = std::make_pair(L10N_UCS2, L10N_ISO_8859_16);
	converters[196628] = std::make_pair(L10N_UCS2, L10N_CODEPAGE_437);
	converters[196629] = std::make_pair(L10N_UCS2, L10N_CODEPAGE_850);
	converters[196630] = std::make_pair(L10N_UCS2, L10N_CODEPAGE_863);
	converters[196631] = std::make_pair(L10N_UCS2, L10N_CODEPAGE_866);
	converters[196632] = std::make_pair(L10N_UCS2, L10N_CODEPAGE_932);
	converters[196633] = std::make_pair(L10N_UCS2, L10N_CODEPAGE_936);
	converters[196634] = std::make_pair(L10N_UCS2, L10N_CODEPAGE_949);
	converters[196635] = std::make_pair(L10N_UCS2, L10N_CODEPAGE_950);
	converters[196636] = std::make_pair(L10N_UCS2, L10N_CODEPAGE_1251);
	converters[196637] = std::make_pair(L10N_UCS2, L10N_CODEPAGE_1252);
	converters[196638] = std::make_pair(L10N_UCS2, L10N_EUC_CN);
	converters[196639] = std::make_pair(L10N_UCS2, L10N_EUC_JP);
	converters[196640] = std::make_pair(L10N_UCS2, L10N_EUC_KR);
	converters[196641] = std::make_pair(L10N_UCS2, L10N_ISO_2022_JP);
	converters[196642] = std::make_pair(L10N_UCS2, L10N_ARIB);
	converters[196643] = std::make_pair(L10N_UCS2, L10N_HZ);
	converters[196644] = std::make_pair(L10N_UCS2, L10N_GB18030);
	converters[196645] = std::make_pair(L10N_UCS2, L10N_RIS_506);
	converters[196646] = std::make_pair(L10N_UCS2, L10N_CODEPAGE_852);
	converters[196647] = std::make_pair(L10N_UCS2, L10N_CODEPAGE_1250);
	converters[196648] = std::make_pair(L10N_UCS2, L10N_CODEPAGE_737);
	converters[196649] = std::make_pair(L10N_UCS2, L10N_CODEPAGE_1253);
	converters[196650] = std::make_pair(L10N_UCS2, L10N_CODEPAGE_857);
	converters[196651] = std::make_pair(L10N_UCS2, L10N_CODEPAGE_1254);
	converters[196652] = std::make_pair(L10N_UCS2, L10N_CODEPAGE_775);
	converters[196653] = std::make_pair(L10N_UCS2, L10N_CODEPAGE_1257);
	converters[196654] = std::make_pair(L10N_UCS2, L10N_CODEPAGE_855);
	converters[196655] = std::make_pair(L10N_UCS2, L10N_CODEPAGE_858);
	converters[196656] = std::make_pair(L10N_UCS2, L10N_CODEPAGE_860);
	converters[196657] = std::make_pair(L10N_UCS2, L10N_CODEPAGE_861);
	converters[196658] = std::make_pair(L10N_UCS2, L10N_CODEPAGE_865);
	converters[196659] = std::make_pair(L10N_UCS2, L10N_CODEPAGE_869);
	converters[327680] = std::make_pair(L10N_ISO_8859_1, L10N_UTF8);
	converters[327683] = std::make_pair(L10N_ISO_8859_1, L10N_UCS2);
	converters[393216] = std::make_pair(L10N_ISO_8859_2, L10N_UTF8);
	converters[393219] = std::make_pair(L10N_ISO_8859_2, L10N_UCS2);
	converters[458752] = std::make_pair(L10N_ISO_8859_3, L10N_UTF8);
	converters[458755] = std::make_pair(L10N_ISO_8859_3, L10N_UCS2);
	converters[524288] = std::make_pair(L10N_ISO_8859_4, L10N_UTF8);
	converters[524291] = std::make_pair(L10N_ISO_8859_4, L10N_UCS2);
	converters[589824] = std::make_pair(L10N_ISO_8859_5, L10N_UTF8);
	converters[589827] = std::make_pair(L10N_ISO_8859_5, L10N_UCS2);
	converters[655360] = std::make_pair(L10N_ISO_8859_6, L10N_UTF8);
	converters[655363] = std::make_pair(L10N_ISO_8859_6, L10N_UCS2);
	converters[720896] = std::make_pair(L10N_ISO_8859_7, L10N_UTF8);
	converters[720899] = std::make_pair(L10N_ISO_8859_7, L10N_UCS2);
	converters[786432] = std::make_pair(L10N_ISO_8859_8, L10N_UTF8);
	converters[786435] = std::make_pair(L10N_ISO_8859_8, L10N_UCS2);
	converters[851968] = std::make_pair(L10N_ISO_8859_9, L10N_UTF8);
	converters[851971] = std::make_pair(L10N_ISO_8859_9, L10N_UCS2);
	converters[917504] = std::make_pair(L10N_ISO_8859_10, L10N_UTF8);
	converters[917507] = std::make_pair(L10N_ISO_8859_10, L10N_UCS2);
	converters[983040] = std::make_pair(L10N_ISO_8859_11, L10N_UTF8);
	converters[983043] = std::make_pair(L10N_ISO_8859_11, L10N_UCS2);
	converters[1048576] = std::make_pair(L10N_ISO_8859_13, L10N_UTF8);
	converters[1048579] = std::make_pair(L10N_ISO_8859_13, L10N_UCS2);
	converters[1114112] = std::make_pair(L10N_ISO_8859_14, L10N_UTF8);
	converters[1114115] = std::make_pair(L10N_ISO_8859_14, L10N_UCS2);
	converters[1179648] = std::make_pair(L10N_ISO_8859_15, L10N_UTF8);
	converters[1179651] = std::make_pair(L10N_ISO_8859_15, L10N_UCS2);
	converters[1245184] = std::make_pair(L10N_ISO_8859_16, L10N_UTF8);
	converters[1245187] = std::make_pair(L10N_ISO_8859_16, L10N_UCS2);
	converters[1310720] = std::make_pair(L10N_CODEPAGE_437, L10N_UTF8);
	converters[1310723] = std::make_pair(L10N_CODEPAGE_437, L10N_UCS2);
	converters[1376256] = std::make_pair(L10N_CODEPAGE_850, L10N_UTF8);
	converters[1376259] = std::make_pair(L10N_CODEPAGE_850, L10N_UCS2);
	converters[1441792] = std::make_pair(L10N_CODEPAGE_863, L10N_UTF8);
	converters[1441795] = std::make_pair(L10N_CODEPAGE_863, L10N_UCS2);
	converters[1507328] = std::make_pair(L10N_CODEPAGE_866, L10N_UTF8);
	converters[1507331] = std::make_pair(L10N_CODEPAGE_866, L10N_UCS2);
	converters[1572864] = std::make_pair(L10N_CODEPAGE_932, L10N_UTF8);
	converters[1572867] = std::make_pair(L10N_CODEPAGE_932, L10N_UCS2);
	converters[1638400] = std::make_pair(L10N_CODEPAGE_936, L10N_UTF8);
	converters[1638403] = std::make_pair(L10N_CODEPAGE_936, L10N_UCS2);
	converters[1703936] = std::make_pair(L10N_CODEPAGE_949, L10N_UTF8);
	converters[1703939] = std::make_pair(L10N_CODEPAGE_949, L10N_UCS2);
	converters[1769472] = std::make_pair(L10N_CODEPAGE_950, L10N_UTF8);
	converters[1769475] = std::make_pair(L10N_CODEPAGE_950, L10N_UCS2);
	converters[1835008] = std::make_pair(L10N_CODEPAGE_1251, L10N_UTF8);
	converters[1835011] = std::make_pair(L10N_CODEPAGE_1251, L10N_UCS2);
	converters[1900544] = std::make_pair(L10N_CODEPAGE_1252, L10N_UTF8);
	converters[1900547] = std::make_pair(L10N_CODEPAGE_1252, L10N_UCS2);
	converters[1966080] = std::make_pair(L10N_EUC_CN, L10N_UTF8);
	converters[1966083] = std::make_pair(L10N_EUC_CN, L10N_UCS2);
	converters[2031616] = std::make_pair(L10N_EUC_JP, L10N_UTF8);
	converters[2031619] = std::make_pair(L10N_EUC_JP, L10N_UCS2);
	converters[2097152] = std::make_pair(L10N_EUC_KR, L10N_UTF8);
	converters[2097155] = std::make_pair(L10N_EUC_KR, L10N_UCS2);
	converters[2162688] = std::make_pair(L10N_ISO_2022_JP, L10N_UTF8);
	converters[2162691] = std::make_pair(L10N_ISO_2022_JP, L10N_UCS2);
	converters[2228224] = std::make_pair(L10N_ARIB, L10N_UTF8);
	converters[2228227] = std::make_pair(L10N_ARIB, L10N_UCS2);
	converters[2293760] = std::make_pair(L10N_HZ, L10N_UTF8);
	converters[2293763] = std::make_pair(L10N_HZ, L10N_UCS2);
	converters[2359296] = std::make_pair(L10N_GB18030, L10N_UTF8);
	converters[2359299] = std::make_pair(L10N_GB18030, L10N_UCS2);
	converters[2424832] = std::make_pair(L10N_RIS_506, L10N_UTF8);
	converters[2424835] = std::make_pair(L10N_RIS_506, L10N_UCS2);
	converters[2490368] = std::make_pair(L10N_CODEPAGE_852, L10N_UTF8);
	converters[2490371] = std::make_pair(L10N_CODEPAGE_852, L10N_UCS2);
	converters[2555904] = std::make_pair(L10N_CODEPAGE_1250, L10N_UTF8);
	converters[2555907] = std::make_pair(L10N_CODEPAGE_1250, L10N_UCS2);
	converters[2621440] = std::make_pair(L10N_CODEPAGE_737, L10N_UTF8);
	converters[2621443] = std::make_pair(L10N_CODEPAGE_737, L10N_UCS2);
	converters[2686976] = std::make_pair(L10N_CODEPAGE_1253, L10N_UTF8);
	converters[2686979] = std::make_pair(L10N_CODEPAGE_1253, L10N_UCS2);
	converters[2752512] = std::make_pair(L10N_CODEPAGE_857, L10N_UTF8);
	converters[2752515] = std::make_pair(L10N_CODEPAGE_857, L10N_UCS2);
	converters[2818048] = std::make_pair(L10N_CODEPAGE_1254, L10N_UTF8);
	converters[2818051] = std::make_pair(L10N_CODEPAGE_1254, L10N_UCS2);
	converters[2883584] = std::make_pair(L10N_CODEPAGE_775, L10N_UTF8);
	converters[2883587] = std::make_pair(L10N_CODEPAGE_775, L10N_UCS2);
	converters[2949120] = std::make_pair(L10N_CODEPAGE_1257, L10N_UTF8);
	converters[2949123] = std::make_pair(L10N_CODEPAGE_1257, L10N_UCS2);
	converters[3014656] = std::make_pair(L10N_CODEPAGE_855, L10N_UTF8);
	converters[3014659] = std::make_pair(L10N_CODEPAGE_855, L10N_UCS2);
	converters[3080192] = std::make_pair(L10N_CODEPAGE_858, L10N_UTF8);
	converters[3080195] = std::make_pair(L10N_CODEPAGE_858, L10N_UCS2);
	converters[3145728] = std::make_pair(L10N_CODEPAGE_860, L10N_UTF8);
	converters[3145731] = std::make_pair(L10N_CODEPAGE_860, L10N_UCS2);
	converters[3211264] = std::make_pair(L10N_CODEPAGE_861, L10N_UTF8);
	converters[3211267] = std::make_pair(L10N_CODEPAGE_861, L10N_UCS2);
	converters[3276800] = std::make_pair(L10N_CODEPAGE_865, L10N_UTF8);
	converters[3276803] = std::make_pair(L10N_CODEPAGE_865, L10N_UCS2);
	converters[3342336] = std::make_pair(L10N_CODEPAGE_869, L10N_UTF8);
	converters[3342339] = std::make_pair(L10N_CODEPAGE_869, L10N_UCS2);
}
