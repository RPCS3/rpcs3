using System;
using System.Collections.Generic;
using System.Text;

namespace GSDumpGUI
{
    public class GIFPrim : GIFUtil
    {
        public UInt32 Prim;
        public UInt32 IIP;
        public UInt32 TME;
        public UInt32 FGE;
        public UInt32 ABE;
        public UInt32 AA1;
        public UInt32 FST;
        public UInt32 CTXT;
        public UInt32 FIX;

        static internal GIFPrim ExtractGIFPrim(UInt32 rawValue)
        {
            GIFPrim pri = new GIFPrim();
            pri.Prim = (rawValue & 0x007);
            pri.IIP = (rawValue & 0x008) >> 3;
            pri.TME = (rawValue & 0x010) >> 4;
            pri.FGE = (rawValue & 0x020) >> 5;
            pri.ABE = (rawValue & 0x040) >> 6;
            pri.AA1 = (rawValue & 0x080) >> 7;
            pri.FST = (rawValue & 0x100) >> 8;
            pri.CTXT = (rawValue & 0x200) >> 9;
            pri.FIX = (rawValue & 0x400) >> 10;
            return pri;
        }
    }

    public enum GS_PRIM
    {
        GS_POINTLIST = 0,
        GS_LINELIST = 1,
        GS_LINESTRIP = 2,
        GS_TRIANGLELIST = 3,
        GS_TRIANGLESTRIP = 4,
        GS_TRIANGLEFAN = 5,
        GS_SPRITE = 6,
        GS_INVALID = 7,
    }
}
