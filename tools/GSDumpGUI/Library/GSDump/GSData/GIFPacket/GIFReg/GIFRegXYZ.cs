using System;
using System.Collections.Generic;
using System.Text;

namespace GSDumpGUI
{
    public class GIFRegXYZ : GIFReg
    {
        public double X;
        public double Y;
        public UInt32 Z;
        public bool ADC;

        static public GIFReg Unpack(UInt64 LowData, UInt64 HighData, bool PlainFormat)
        {
            GIFRegXYZ xf = new GIFRegXYZ();
            xf.ADC = GIFReg.GetBit(HighData, 47, 1) == 1;
            xf.Descriptor = (xf.ADC == true ? GIFRegDescriptor.XYZ3 : GIFRegDescriptor.XYZ2);
            xf.X = GIFReg.GetBit(LowData, 0, 16) / 16d;
            xf.Y = GIFReg.GetBit(LowData, 32, 16) / 16d;
            xf.Z = (UInt32)(GIFReg.GetBit(HighData, 4, 24));
            return xf;
        }
    }
}
