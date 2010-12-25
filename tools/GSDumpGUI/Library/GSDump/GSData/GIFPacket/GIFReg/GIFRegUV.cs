using System;
using System.Collections.Generic;
using System.Text;

namespace GSDumpGUI
{
    public class GIFRegUV : GIFReg
    {
        public double U;
        public double V;

        static public GIFReg Unpack(UInt64 LowData, UInt64 HighData, bool PlainFormat)
        {
            GIFRegUV u = new GIFRegUV();
            u.Descriptor = GIFRegDescriptor.UV;
            u.U = GIFReg.GetBit(LowData, 0, 14) / 16d;
            u.V = GIFReg.GetBit(LowData, 32, 14) / 16d;
            return u;
        }
    }
}
