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
            GIFRegXYZF xf = new GIFRegXYZF();
            xf.ADC = ((HighData & 0x1000000000000) >> 46) == 1;
            xf.Descriptor = (xf.ADC == true ? GIFRegDescriptor.XYZ3 : GIFRegDescriptor.XYZ2);
            xf.X = (LowData & 0xFFFF) / 16d;
            xf.Y = ((LowData & 0xFFFF00000000) >> 32) / 16d;
            xf.Z = (UInt32)((HighData & 0xFFFFFF0) >> 4);
            return xf;
        }
    }
}
