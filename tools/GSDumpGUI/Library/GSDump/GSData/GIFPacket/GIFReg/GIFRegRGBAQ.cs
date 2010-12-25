using System;
using System.Collections.Generic;
using System.Text;

namespace GSDumpGUI
{
    public class GIFRegRGBAQ : GIFReg
    {
        public int R;
        public int G;
        public int B;
        public int A;

        static public  GIFReg Unpack(UInt64 LowData, UInt64 HighData, bool PlainFormat)
        {
            GIFRegRGBAQ r = new GIFRegRGBAQ();
            r.Descriptor = GIFRegDescriptor.RGBAQ;
            r.R = (int)GIFReg.GetBit(LowData, 0, 8);
            r.G = (int)GIFReg.GetBit(LowData, 32, 8);
            r.B = (int)GIFReg.GetBit(HighData, 0, 8);
            r.A = (int)GIFReg.GetBit(HighData, 32, 8);
            return r;
        }
    }
}
