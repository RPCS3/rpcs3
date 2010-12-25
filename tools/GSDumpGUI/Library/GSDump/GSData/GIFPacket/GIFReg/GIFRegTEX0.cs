using System;
using System.Collections.Generic;
using System.Text;

namespace GSDumpGUI
{
    public class GIFRegTEX0 : GIFReg
    {
        public int TBP0;
        public int TBW;
        public TEXPSM PSM;
        public int TW;
        public int TH;
        public TEXTCC TCC;
        public TEXTFX TFX;
        public int CBP;
        public TEXCPSM CPSM;
        public TEXCSM CSM;
        public int CSA;
        public int CLD;

        static public GIFReg Unpack(UInt64 LowData, UInt64 HighData, bool PlainFormat)
        {
            GIFRegTEX0 r = new GIFRegTEX0();
            r.Descriptor = GIFRegDescriptor.TEX0_1;
            r.TBP0 = (int)GetBit(LowData, 0, 14);
            r.TBW = (int)GetBit(LowData, 14, 6);
            r.PSM = (TEXPSM)(int)GetBit(LowData, 20, 6);
            r.TW = (int)GetBit(LowData, 26, 4);
            r.TH = (int)GetBit(LowData, 30, 4);
            r.TCC = (TEXTCC)(int)GetBit(LowData, 34, 1);
            r.TFX = (TEXTFX)(int)GetBit(LowData, 35, 2);
            r.CBP = (int)GetBit(LowData, 37, 14);
            r.CPSM = (TEXCPSM)(int)GetBit(LowData, 51, 4);
            r.CSM = (TEXCSM)(int)GetBit(LowData, 55, 1);
            r.CSA = (int)GetBit(LowData, 56, 5);
            r.CLD = (int)GetBit(LowData, 61, 3);
            return r;
        }
    }

    public enum TEXPSM
    {
        PSMCT32 = 0,
        PSMCT24 = 1,
        PSMCT16 = 2,
        PSMCT16S = 10,
        PSMT8 = 19,
        PSMT4 = 20,
        PSMT8H = 27,
        PSMT4HL = 36,
        PSMT4HH = 44,
        PSMZ32 = 48,
        PSMZ24 = 49,
        PSMZ16 = 50,
        PSMZ16S = 58
    }

    public enum TEXTCC
    {
        RGB = 0,
        RGBA = 1
    }

    public enum TEXTFX
    {
        MODULATE = 0,
        DECAL = 1,
        HIGHLIGHT = 2,
        HIGHLIGHT2 = 3
    }

    public enum TEXCPSM
    {
        PSMCT32 = 0,
        PSMCT16 = 2,
        PSMCT16S = 10
    }

    public enum TEXCSM
    {
        CSM1 = 0,
        CSM2 = 1
    }
}
