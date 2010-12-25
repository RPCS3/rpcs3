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
            GIFRegXYZF xf2 = new GIFRegXYZF();
            xf2.ADC = GIFReg.GetBit(HighData, 47, 1) == 1;
            xf2.Descriptor = (xf2.ADC == true ? GIFRegDescriptor.XYZF3 : GIFRegDescriptor.XYZF2);
            xf2.X = GIFReg.GetBit(LowData, 0, 16) / 16d;
            xf2.Y = GIFReg.GetBit(LowData, 32, 16) / 16d;
            xf2.Z = (UInt32)(GIFReg.GetBit(HighData, 4, 24));
            xf2.F = (UInt16)(GIFReg.GetBit(HighData, 36, 8));
            return xf2;
        }
    }
}
