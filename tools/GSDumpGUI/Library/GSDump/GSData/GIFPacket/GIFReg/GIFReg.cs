using System;
using System.Collections.Generic;
using System.Text;

namespace GSDumpGUI
{
    [Serializable]
    abstract public class GIFReg : GIFUtil, IGifData
    {
        public GIFRegDescriptor Descriptor;
        public UInt64 LowData, HighData;
        public bool PackedFormat;

        private GIFReg() { }

        public GIFReg(byte addr, UInt64 LowData, UInt64 HighData, bool PackedFormat)
        {
            this.LowData = LowData;
            this.HighData = HighData;
            this.PackedFormat = PackedFormat;
        }

        abstract public new String ToString();
    }

    public enum GIFRegDescriptor
    {
        PRIM = 0x00,
        RGBAQ = 0x01,
        ST = 0x02,
        UV = 0x03,
        XYZF2 = 0x04,
        XYZ2 = 0x05,
        TEX0_1 = 0x06,
        TEX0_2 = 0x07,
        CLAMP_1 = 0x08,
        CLAMP_2 = 0x09,
        FOG = 0x0a,
        XYZF3 = 0x0c,
        XYZ3 = 0x0d,
        AD = 0x0e,
        NOP = 0x0f, // actually, 0xf is the standard GIF NOP and 0x7f is the standard GS NOP, but all unregistered addresses act as NOPs... probably
        TEX1_1 = 0x14,
        TEX1_2 = 0x15,
        TEX2_1 = 0x16,
        TEX2_2 = 0x17,
        XYOFFSET_1 = 0x18,
        XYOFFSET_2 = 0x19,
        PRMODECONT = 0x1a,
        PRMODE = 0x1b,
        TEXCLUT = 0x1c,
        SCANMSK = 0x22,
        MIPTBP1_1 = 0x34,
        MIPTBP1_2 = 0x35,
        MIPTBP2_1 = 0x36,
        MIPTBP2_2 = 0x37,
        TEXA = 0x3b,
        FOGCOL = 0x3d,
        TEXFLUSH = 0x3f,
        SCISSOR_1 = 0x40,
        SCISSOR_2 = 0x41,
        ALPHA_1 = 0x42,
        ALPHA_2 = 0x43,
        DIMX = 0x44,
        DTHE = 0x45,
        COLCLAMP = 0x46,
        TEST_1 = 0x47,
        TEST_2 = 0x48,
        PABE = 0x49,
        FBA_1 = 0x4a,
        FBA_2 = 0x4b,
        FRAME_1 = 0x4c,
        FRAME_2 = 0x4d,
        ZBUF_1 = 0x4e,
        ZBUF_2 = 0x4f,
        BITBLTBUF = 0x50,
        TRXPOS = 0x51,
        TRXREG = 0x52,
        TRXDIR = 0x53,
        HWREG = 0x54,
        SIGNAL = 0x60,
        FINISH = 0x61,
        LABEL = 0x62,
    }

}
