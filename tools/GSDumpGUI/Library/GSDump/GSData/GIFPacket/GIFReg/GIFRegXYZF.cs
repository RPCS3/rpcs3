using System;
using System.Collections.Generic;
using System.Text;

namespace GSDumpGUI
{
    public class GIFRegXYZF : GIFReg
    {
        public double X;
        public double Y;
        public UInt32 Z;
        public UInt16 F;
        public bool ADC;

        static public GIFReg Unpack(UInt64 LowData, UInt64 HighData, bool PlainFormat)
        {
            GIFRegXYZ xf2 = new GIFRegXYZ();
            xf2.ADC = ((HighData & 0x1000000000000) >> 46) == 1;
            xf2.Descriptor = (xf2.ADC == true ? GIFRegDescriptor.XYZF3 : GIFRegDescriptor.XYZF2);
            xf2.X = (LowData & 0xFFFF) / 16d;
            xf2.Y = ((LowData & 0xFFFF00000000) >> 32) / 16d;
            xf2.Z = (UInt32)((HighData & 0xFFFFFF0) >> 4);
            xf2.F = (UInt16)((HighData & 0xFF000000000) >> 36);
            return xf2;
        }
    }
}
