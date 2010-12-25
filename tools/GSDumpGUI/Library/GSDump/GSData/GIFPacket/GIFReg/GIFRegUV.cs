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
            u.U = (LowData & 0x3FFF) / 16d;
            u.V = ((LowData & 0x3FFF00000000) >> 32) / 16d;
            return u;
        }
    }
}
